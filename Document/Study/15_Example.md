# Example15

## IOCP란 무엇인가?

### 🎯 IOCP (I/O Completion Port) 정의

**Input/Output Completion Port**는 Windows에서 제공하는 가장 강력한 비동기 I/O 모델입니다.

### 🏭 핵심 아이디어: 작업 완료 통지 중앙화

`일반 모델:                    IOCP 모델:
Thread1 → Socket1            모든 Socket → IOCP → Worker Threads
Thread2 → Socket2                          ↓
Thread3 → Socket3                    중앙 완료 큐
   ↓                                      ↓
개별 처리 (비효율)              효율적 분배`

### 💡 비유로 이해하기

`일반 모델 = 각 고객마다 전담 직원 배치 (비효율)
IOCP = 중앙 접수처 + 여러 처리 직원 (효율적)

[고객들] → [접수처(IOCP)] → [처리 직원들(Worker Threads)]
                ↓
          작업 완료 통지`

---

## IOCP의 핵심 개념

### 🔑 주요 구성 요소

### 1️⃣ **Completion Port (CP)**

```cpp
HANDLE iocpHandle = ::CreateIoCompletionPort(
    INVALID_HANDLE_VALUE,  *// 새 CP 생성*
    NULL,                  *// 기존 CP (없음)*
    0,                     *// Completion Key*
    0                      *// 동시 실행 스레드 수 (0=자동)*
);
```

### 2️⃣ **Completion Key**

```cpp
*// 소켓을 CP에 등록할 때 Session 포인터를 Key로 사용*
::CreateIoCompletionPort(
    (HANDLE)clientSocket,   *// 등록할 핸들*
    iocpHandle,            *// CP 핸들*
    (ULONG_PTR)session,    *// Key: 나중에 구분용*
    0
);
```

### 3️⃣ **Overlapped 구조체**

```cpp
struct OverlappedEx {
    WSAOVERLAPPED overlapped = {};  *// 필수 (첫 번째 멤버)*
    int32 type = 0;                 *// 추가 정보 (READ/WRITE 구분)*
};
```

### 4️⃣ **GetQueuedCompletionStatus**

```cpp
BOOL ret = ::GetQueuedCompletionStatus(
    iocpHandle,           *// CP 핸들*
    &bytesTransferred,    *// 전송된 바이트*
    (ULONG_PTR*)&session, *// Completion Key (세션 찾기)*
    (LPOVERLAPPED*)&overlappedEx,  *// Overlapped 구조체*
    INFINITE              *// 대기 시간*
);
```

---

## 코드 구조 분석

### 📁 전체 구조

`Main Thread                 Worker Threads (5개)
    │                            │
    ├─ Socket 생성               ├─ IOCP 대기
    ├─ Bind/Listen              ├─ 완료 통지 처리
    ├─ Accept (대기)            └─ 다음 Recv 예약
    ├─ Session 생성
    ├─ IOCP 등록
    └─ 첫 Recv 예약`

### 🔍 주요 구조체 분석

### Session 구조체

```cpp
struct Session {
    SOCKET socket = INVALID_SOCKET;  *// 클라이언트 소켓*
    char recvBuffer[BUFSIZE] = {};   *// 수신 버퍼*
    int32 recvBytes = 0;              *// 수신된 바이트 수*
};
```

### OverlappedEx 구조체

```cpp
struct OverlappedEx {
    WSAOVERLAPPED overlapped = {};  *// Windows Overlapped 구조체*
    int32 type = 0;                 *// I/O 작업 타입*
};
```

### 📝 코드 흐름 분석

```cpp
int main() {
    *// 1. 초기화*
    WSAStartup();
    
    *// 2. 리스닝 소켓 생성*
    SOCKET listenSocket = socket();
    bind();
    listen();
    
    *// 3. IOCP 생성*
    HANDLE iocpHandle = CreateIoCompletionPort();
    
    *// 4. Worker 스레드 생성 (5개)*
    for (int i = 0; i < 5; i++) {
        workers.push_back(thread(WorkerThreadMain));
    }
    
    *// 5. Accept 루프*
    while (true) {
        SOCKET clientSocket = accept();
        Session* session = new Session();
        
        *// 6. IOCP에 소켓 등록*
        CreateIoCompletionPort(clientSocket, iocpHandle, session);
        
        *// 7. 첫 Recv 예약*
        OverlappedEx* overlappedEx = new OverlappedEx();
        WSARecv(clientSocket, ..., &overlappedEx->overlapped);
    }
}
```

---

## 상세 동작 과정

### 🔄 전체 플로우

`1. 클라이언트 접속
      ↓
2. Accept → Session 생성
      ↓
3. IOCP에 소켓 등록
      ↓
4. WSARecv 호출 (비동기 수신 예약)
      ↓
5. 데이터 도착
      ↓
6. IOCP가 Worker Thread에 통지
      ↓
7. GetQueuedCompletionStatus 반환
      ↓
8. 데이터 처리
      ↓
9. 다시 WSARecv 호출 (계속 수신)`

### 🎬 시나리오별 동작

### 시나리오 1: 클라이언트 접속

`Main Thread:
1. accept() 대기 중...
2. 클라이언트 접속! → clientSocket 생성
3. Session 객체 생성
4. CreateIoCompletionPort로 IOCP 등록
5. WSARecv로 첫 수신 예약`

### 시나리오 2: 데이터 수신

`Worker Thread:
1. GetQueuedCompletionStatus 대기 중...
2. 데이터 도착! → 함수 반환
3. session과 overlappedEx 포인터 획득
4. bytesTransferred 만큼 데이터 처리
5. WSARecv로 다음 수신 예약`

### ⚡ 비동기 처리의 핵심

```cpp
*// WSARecv 호출 시점*
WSARecv(socket, ..., &overlapped, NULL);
*// ↑ 여기서 즉시 리턴! (비동기)// 실제 데이터는 나중에 도착// 나중에 Worker Thread에서*
GetQueuedCompletionStatus(...);
*// ↑ 데이터 도착하면 여기서 깨어남*
```