# Worker Thread와 DoWorkerJob 상세 가이드

> 게임 서버의 핵심 엔진: 멀티스레드 작업 처리 구조

---

## 전체 구조 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          GameServer                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   Main Thread                                                            │
│   ┌─────────────────────────────────────────────────────────────┐       │
│   │  서버 초기화 → Worker Thread 생성 → Join (대기)              │       │
│   └─────────────────────────────────────────────────────────────┘       │
│                                                                          │
│   Worker Thread Pool (5개)                                               │
│   ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐          │
│   │Worker 1 │ │Worker 2 │ │Worker 3 │ │Worker 4 │ │Worker 5 │          │
│   │         │ │         │ │         │ │         │ │         │          │
│   │ DoWorkerJob() 무한 루프                                    │          │
│   └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘          │
│        │           │           │           │           │                │
│        └───────────┴───────────┼───────────┴───────────┘                │
│                                │                                         │
│                    ┌───────────┴───────────┐                            │
│                    │      공유 자원들       │                            │
│                    │ • IOCP Core           │                            │
│                    │ • GlobalQueue         │                            │
│                    │ • JobTimer            │                            │
│                    └───────────────────────┘                            │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## DoWorkerJob 코드 분석

**파일:** `GameServer/GameServer.cpp:22-37`

```cpp
enum
{
    WORKER_TICK = 64  // 64ms 단위로 작업 배분
};

void DoWorkerJob(ServerServiceRef& service)
{
    while (true)  // 무한 루프
    {
        // ① 이번 틱의 종료 시간 설정
        LEndTickCount = ::GetTickCount64() + WORKER_TICK;

        // ② 네트워크 I/O 처리 (IOCP)
        service->GetIocpCore()->Dispatch(10);

        // ③ 예약된 일감 처리 (Timer)
        ThreadManager::DistributeReservedJobs();

        // ④ 글로벌 큐 작업 처리
        ThreadManager::DoGlobalQueueWork();
    }
}
```

### 한 사이클 흐름도

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     DoWorkerJob 한 사이클 (64ms)                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────┐                                                   │
│  │ LEndTickCount =  │  "이번 틱은 64ms 후에 끝나!"                       │
│  │ now + 64ms       │                                                   │
│  └────────┬─────────┘                                                   │
│           │                                                              │
│           ▼                                                              │
│  ┌──────────────────┐                                                   │
│  │ IOCP Dispatch    │  "10ms 동안 네트워크 I/O 처리"                    │
│  │ (timeout: 10ms)  │  → 패킷 수신/송신 완료 이벤트 처리                │
│  └────────┬─────────┘                                                   │
│           │                                                              │
│           ▼                                                              │
│  ┌──────────────────┐                                                   │
│  │ DistributeReserved│  "예약된 타이머 작업 분배"                        │
│  │ Jobs()           │  → 실행 시간이 된 Job들을 JobQueue로 이동         │
│  └────────┬─────────┘                                                   │
│           │                                                              │
│           ▼                                                              │
│  ┌──────────────────┐                                                   │
│  │ DoGlobalQueue    │  "글로벌 큐의 JobQueue들 실행"                    │
│  │ Work()           │  → LEndTickCount까지 계속 처리                    │
│  └────────┬─────────┘                                                   │
│           │                                                              │
│           ▼                                                              │
│       다음 사이클                                                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1단계: 틱 종료 시간 설정

```cpp
LEndTickCount = ::GetTickCount64() + WORKER_TICK;  // WORKER_TICK = 64ms
```

### LEndTickCount의 역할

```
시간 →
├────────────────── 64ms ──────────────────┤
│                                           │
now                                    LEndTickCount
│                                           │
│  "이 시간 안에 최대한 많은 작업을 처리해!" │
│                                           │
└───────────────────────────────────────────┘

왜 64ms인가?
• 60 FPS 기준 = 16.6ms per frame
• 64ms = 약 4프레임 분량
• 너무 짧으면: 오버헤드 증가
• 너무 길면: 작업 지연, 응답성 저하
```

### TLS (Thread Local Storage) 변수

```cpp
// CoreTLS.h에 정의
thread_local uint64 LEndTickCount;     // 이번 틱 종료 시간
thread_local JobQueue* LCurrentJobQueue; // 현재 처리 중인 JobQueue
thread_local uint32 LThreadId;          // 스레드 고유 ID
```

각 Worker Thread가 독립적인 값을 가짐 (충돌 없음).

---

## 2단계: IOCP Dispatch

**파일:** `ServerCore/IocpCore.cpp:25-52`

```cpp
service->GetIocpCore()->Dispatch(10);  // 10ms timeout
```

### 동작 원리

```cpp
bool IocpCore::Dispatch(uint32 timeoutMs)
{
    DWORD numOfBytes = 0;
    ULONG_PTR key = 0;
    IocpEvent* iocpEvent = nullptr;

    // Windows 커널에 완료된 I/O 요청
    if (::GetQueuedCompletionStatus(
            _iocpHandle,
            OUT &numOfBytes,
            OUT &key,
            OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent),
            timeoutMs))  // 10ms 대기
    {
        // 완료된 I/O 처리
        IocpObjectRef iocpObject = iocpEvent->owner;
        iocpObject->Dispatch(iocpEvent, numOfBytes);
    }
    // ...
}
```

### 처리되는 이벤트들

| 이벤트 타입 | 처리 내용 |
|------------|----------|
| `Accept` | 새 클라이언트 연결 수락 |
| `Recv` | 패킷 수신 → 핸들러 실행 |
| `Send` | 패킷 송신 완료 확인 |
| `Disconnect` | 연결 해제 처리 |

### 중요: 패킷 핸들러와의 연결

```
IOCP Dispatch
     │
     ▼
RecvEvent 처리
     │
     ▼
PacketSession::OnRecv()
     │
     ▼
GameSession::OnRecvPacket()
     │
     ▼
ServerPacketHandler::HandlePacket()
     │
     ▼
Handle_C2S_MOVE() 등
     │
     ▼
Room::DoAsync(&Room::HandleMove, ...)  ← JobQueue에 작업 등록!
```

**핵심**: 네트워크 I/O 처리 중에 게임 로직 Job이 JobQueue에 쌓임.

---

## 3단계: 예약된 일감 분배 (JobTimer)

**파일:** `ServerCore/ThreadManager.cpp:71-76`

```cpp
void ThreadManager::DistributeReservedJobs()
{
    const uint64 now = ::GetTickCount64();
    GJobTimer->Distribute(now);
}
```

### JobTimer 구조

**파일:** `ServerCore/JobTimer.h`

```cpp
struct TimerItem
{
    uint64 executeTick = 0;   // 실행할 시간
    JobData* jobData = nullptr;  // 실행할 작업
};

class JobTimer
{
    priority_queue<TimerItem> _items;  // 실행 시간순 정렬
};
```

### 타이머 등록 예시

```cpp
// Room.cpp:32-33
void Room::Init(int mapId)
{
    // 50ms마다 Update 호출
    DoTimer(50, &Room::Update);

    // 10초마다 Ping 전송
    DoTimer(_pingInterval, &Room::SendPing);
}
```

### Distribute 동작

```
JobTimer (priority_queue)
┌─────────────────────────────────────────────────────────┐
│  [100ms, Update] [150ms, Ping] [200ms, Update] ...      │
└─────────────────────────────────────────────────────────┘
         │
         │ now = 120ms
         ▼
    ┌────────────┐
    │ 100ms ≤ now│ → Yes! → JobQueue에 Push
    └────────────┘
    ┌────────────┐
    │ 150ms ≤ now│ → No → 다음 틱에
    └────────────┘

결과: [100ms, Update]가 Room의 JobQueue로 이동
```

---

## 4단계: 글로벌 큐 작업 처리

**파일:** `ServerCore/ThreadManager.cpp:55-69`

```cpp
void ThreadManager::DoGlobalQueueWork()
{
    while (true)
    {
        uint64 now = ::GetTickCount64();

        // ① 틱 시간 초과 체크
        if (now > LEndTickCount)
            break;

        // ② GlobalQueue에서 JobQueue 꺼내기
        JobQueueRef jobQueue = GGlobalQueue->Pop();
        if (jobQueue == nullptr)
            break;

        // ③ JobQueue 실행
        jobQueue->Execute();
    }
}
```

### GlobalQueue란?

```
GlobalQueue = 실행 대기 중인 JobQueue들의 대기열

┌──────────────────────────────────────────────────────────┐
│  GlobalQueue                                              │
│  ┌────────┐ ┌────────┐ ┌────────┐                        │
│  │ Room 1 │→│ Room 2 │→│ Room 3 │→ ...                   │
│  │JobQueue│ │JobQueue│ │JobQueue│                        │
│  └────────┘ └────────┘ └────────┘                        │
└──────────────────────────────────────────────────────────┘
      ↑
      │ Pop()
      │
  Worker Thread가 하나씩 꺼내서 Execute()
```

### JobQueue::Execute() 동작

**파일:** `ServerCore/JobQueue.cpp:32-70`

```cpp
void JobQueue::Execute()
{
    LCurrentJobQueue = this;  // TLS에 현재 처리 중인 큐 기록

    while (true)
    {
        // 모든 Job 꺼내기
        vector<JobRef> jobs;
        _jobs.PopAll(OUT jobs);

        // 각 Job 실행
        for (int32 i = 0; i < jobCount; i++)
        {
            jobs[i]->Execute();  // 실제 게임 로직 실행!
        }

        // 더 이상 Job이 없으면 종료
        if (_jobCount.fetch_sub(jobCount) == jobCount)
        {
            LCurrentJobQueue = nullptr;
            return;
        }

        // 틱 시간 초과 시 GlobalQueue에 다시 넣고 종료
        if (now >= LEndTickCount)
        {
            LCurrentJobQueue = nullptr;
            GGlobalQueue->Push(shared_from_this());
            break;
        }
    }
}
```

---

## 전체 작업 흐름 예시

### 시나리오: 플레이어 이동 처리

```
1. 클라이언트가 C2S_MOVE 패킷 전송
           │
           ▼
2. [Worker Thread A] IOCP Dispatch에서 RecvEvent 감지
           │
           ▼
3. ServerPacketHandler::Handle_C2S_MOVE() 실행
           │
           ▼
4. room->DoAsync(&Room::HandleMove, player, pkt)
   └── Room의 JobQueue에 Job 추가
   └── JobQueue가 비어있었으면 → GlobalQueue에 Push
           │
           ▼
5. [Worker Thread B] DoGlobalQueueWork()에서 Room JobQueue Pop
           │
           ▼
6. JobQueue::Execute() → Room::HandleMove() 실행
           │
           ▼
7. 이동 처리 → S2C_MOVE 브로드캐스트
```

### 타임라인 다이어그램

```
시간 →   0ms        10ms       20ms       50ms       64ms
          │          │          │          │          │
Worker A  │──Dispatch─│──────────│──────────│──────────│
          │  (패킷처리)│          │          │          │
          │     │     │          │          │          │
          │     ▼     │          │          │          │
          │ JobQueue  │          │          │          │
          │   Push    │          │          │          │
          │          │          │          │          │
Worker B  │──────────│─Dispatch──│──────────│──────────│
          │          │          │          │          │
Worker C  │──────────│──────────│─DoGlobal─│──────────│
          │          │          │  Queue   │          │
          │          │          │    │     │          │
          │          │          │    ▼     │          │
          │          │          │ Execute  │          │
          │          │          │(HandleMove)         │
          │          │          │          │          │
          └──────────┴──────────┴──────────┴──────────┘
                         64ms 틱 종료 → 다음 사이클
```

---

## 왜 이런 구조인가?

### 1. 작업 분리 (Separation of Concerns)

```
┌─────────────────────────────────────────────────────────┐
│              DoWorkerJob의 3가지 역할 분리               │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ① IOCP Dispatch                                        │
│     └── 네트워크 I/O (외부 이벤트)                       │
│                                                          │
│  ② DistributeReservedJobs                               │
│     └── 타이머 작업 (시간 기반 이벤트)                   │
│                                                          │
│  ③ DoGlobalQueueWork                                    │
│     └── 게임 로직 (내부 작업)                            │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### 2. 부하 분산 (Load Balancing)

```
5개의 Worker Thread가 GlobalQueue를 공유

Room 1의 Job이 많으면?
→ 여러 Worker가 나눠서 처리 (시간 분할)

Worker A: Room1 Execute (0~64ms 분량)
    ↓ 시간 초과
Worker B: Room1 Execute (64~128ms 분량)
    ↓ 시간 초과
Worker C: Room1 Execute (나머지)
```

### 3. 스레드 안전성 (Thread Safety)

```
❌ 위험한 방식:
여러 Thread가 동시에 같은 Room 데이터 접근
→ Race Condition!

✅ 안전한 방식 (현재 구조):
Room의 Job들은 JobQueue에 의해 순차 실행
→ 한 번에 하나의 Thread만 Room 데이터 접근
```

---

## 핵심 상수와 튜닝

| 상수 | 값 | 의미 | 영향 |
|------|-----|------|------|
| `WORKER_TICK` | 64ms | 한 틱 길이 | 작업 분배 단위 |
| Worker Thread 수 | 5개 | 병렬 처리 능력 | CPU 코어 수에 맞춤 |
| IOCP timeout | 10ms | 네트워크 대기 시간 | 응답성 vs CPU 사용률 |

### 튜닝 가이드

```
WORKER_TICK 감소 (예: 32ms):
  ✅ 작업 응답성 향상
  ❌ 컨텍스트 스위칭 오버헤드 증가

WORKER_TICK 증가 (예: 128ms):
  ✅ 오버헤드 감소
  ❌ 작업 지연 증가, 한 JobQueue가 오래 점유

Worker Thread 수 증가:
  ✅ 병렬 처리 능력 향상
  ❌ 스레드 경쟁 증가, 메모리 사용량 증가
  💡 일반적으로 CPU 코어 수와 비슷하게 설정
```

---

## 코드 위치 정리

| 구성 요소 | 파일 | 핵심 함수/클래스 |
|----------|------|------------------|
| DoWorkerJob | `GameServer/GameServer.cpp:22-37` | `DoWorkerJob()` |
| Thread 관리 | `ServerCore/ThreadManager.cpp` | `Launch()`, `DoGlobalQueueWork()` |
| IOCP 처리 | `ServerCore/IocpCore.cpp` | `Dispatch()` |
| Job 큐 | `ServerCore/JobQueue.cpp` | `Push()`, `Execute()` |
| 글로벌 큐 | `ServerCore/GlobalQueue.h` | `Push()`, `Pop()` |
| 타이머 | `ServerCore/JobTimer.h` | `Reserve()`, `Distribute()` |
| TLS 변수 | `ServerCore/CoreTLS.h` | `LEndTickCount`, `LCurrentJobQueue` |

---

## 요약: 한 눈에 보기

```
┌────────────────────────────────────────────────────────────────┐
│                    DoWorkerJob 무한 루프                        │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  while (true)                                                   │
│  {                                                              │
│      LEndTickCount = now + 64ms;  // 이번 틱 마감 시간 설정     │
│                                                                 │
│      ┌─────────────────────────────────────────────────────┐   │
│      │ ① IOCP Dispatch (10ms)                              │   │
│      │    • 패킷 수신 → 핸들러 → JobQueue에 Job 쌓임       │   │
│      │    • 패킷 송신 완료 처리                            │   │
│      └─────────────────────────────────────────────────────┘   │
│                          ↓                                      │
│      ┌─────────────────────────────────────────────────────┐   │
│      │ ② DistributeReservedJobs                            │   │
│      │    • 타이머 만료된 Job → JobQueue로 이동            │   │
│      │    • 예: Update(), SendPing()                       │   │
│      └─────────────────────────────────────────────────────┘   │
│                          ↓                                      │
│      ┌─────────────────────────────────────────────────────┐   │
│      │ ③ DoGlobalQueueWork (남은 시간 동안)                │   │
│      │    • GlobalQueue에서 JobQueue 꺼내서 Execute        │   │
│      │    • 64ms 초과하면 다음 틱으로 미룸                 │   │
│      └─────────────────────────────────────────────────────┘   │
│  }                                                              │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

**핵심 포인트:**
1. **네트워크 → 게임 로직 연결**: IOCP에서 받은 패킷이 JobQueue를 통해 게임 로직으로 전달
2. **시간 기반 작업 분배**: 64ms 단위로 작업을 나눠 공정하게 처리
3. **멀티스레드 안전성**: JobQueue 덕분에 Room 데이터는 항상 단일 스레드에서 접근

---

> 문서 작성일: 2026-01-07
> 대상 프로젝트: NamoServer
