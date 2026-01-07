# 패킷 수신 흐름 상세 가이드

> 클라이언트에서 보낸 패킷이 `ServerPacketHandler`에 도착하기까지의 여정

---

## 전체 흐름 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        클라이언트 패킷 전송                               │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  1단계: IOCP 완료 감지                                                   │
│  ─────────────────────                                                  │
│  IocpCore::Dispatch() → GetQueuedCompletionStatus()                    │
│  "야, 소켓에서 데이터 왔어!"                                             │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  2단계: 이벤트 분류                                                      │
│  ─────────────────                                                      │
│  Session::Dispatch() → EventType::Recv 감지                            │
│  "Recv 이벤트네? ProcessRecv() 호출하자"                                 │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  3단계: 수신 데이터 처리                                                  │
│  ─────────────────────                                                  │
│  Session::ProcessRecv() → RecvBuffer에 데이터 축적                      │
│  "받은 데이터를 버퍼에 쌓고, OnRecv() 호출!"                              │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  4단계: 패킷 조립 & 보안 검증                                             │
│  ─────────────────────────────                                          │
│  PacketSession::OnRecv()                                                │
│  - 패킷 경계 확인 (size 필드)                                            │
│  - 암호화 시: HMAC 검증 → 복호화                                         │
│  - Sequence 번호 검증 (리플레이 방지)                                    │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  5단계: 게임 세션 처리                                                    │
│  ─────────────────────                                                  │
│  GameSession::OnRecvPacket()                                            │
│  - Rate Limiting 체크                                                   │
│  - 모니터링 통계 갱신                                                    │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  6단계: 패킷 핸들러 라우팅                                                │
│  ─────────────────────────                                              │
│  ServerPacketHandler::HandlePacket()                                    │
│  - 패킷 ID로 핸들러 함수 찾기                                            │
│  - Protobuf 역직렬화                                                    │
│  - 비즈니스 로직 실행                                                    │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1단계: IOCP 완료 감지

**파일:** `ServerCore/IocpCore.cpp:25-52`

```cpp
bool IocpCore::Dispatch(uint32 timeoutMs)
{
    DWORD numOfBytes = 0;
    ULONG_PTR key = 0;
    IocpEvent* iocpEvent = nullptr;

    // Windows 커널에게 "완료된 I/O 있어?" 라고 물어봄
    if (::GetQueuedCompletionStatus(
            _iocpHandle,
            OUT &numOfBytes,    // 받은 바이트 수
            OUT &key,
            OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent),
            timeoutMs))
    {
        // I/O 완료! → 해당 세션에게 처리 위임
        IocpObjectRef iocpObject = iocpEvent->owner;
        iocpObject->Dispatch(iocpEvent, numOfBytes);  // ← 2단계로
    }
    // ...
}
```

### 핵심 포인트
- `GetQueuedCompletionStatus()`는 **블로킹 함수**
- 완료된 I/O가 있을 때까지 대기 (timeout 지정 가능)
- `iocpEvent`에 어떤 종류의 이벤트인지 정보가 담겨있음

---

## 2단계: 이벤트 분류

**파일:** `ServerCore/Session.cpp:84-103`

```cpp
void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    switch (iocpEvent->eventType)
    {
    case EventType::Connect:
        ProcessConnect();
        break;
    case EventType::Disconnect:
        ProcessDisconnect();
        break;
    case EventType::Recv:
        ProcessRecv(numOfBytes);  // ← 우리가 관심있는 부분!
        break;
    case EventType::Send:
        ProcessSend(numOfBytes);
        break;
    }
}
```

### 이벤트 종류

| EventType | 설명 |
|-----------|------|
| `Connect` | 서버에 연결 완료 (클라이언트 모드) |
| `Disconnect` | 연결 해제 완료 |
| `Recv` | **데이터 수신 완료** ← 이번 주제 |
| `Send` | 데이터 송신 완료 |

---

## 3단계: 수신 데이터 처리

**파일:** `ServerCore/Session.cpp:245-274`

```cpp
void Session::ProcessRecv(int32 numOfBytes)
{
    _recvEvent.owner = nullptr;  // 참조 해제

    // 0바이트 수신 = 상대방이 연결 끊음
    if (numOfBytes == 0)
    {
        Disconnect(L"Recv 0");
        return;
    }

    // RecvBuffer에 "이만큼 썼다" 알림
    if (_recvBuffer.OnWrite(numOfBytes) == false)
    {
        Disconnect(L"OnWrite Overflow");
        return;
    }

    // ★ 핵심: 상위 클래스(PacketSession)의 OnRecv 호출
    int32 dataSize = _recvBuffer.DataSize();
    int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);

    // 처리한 만큼 읽기 커서 이동
    if (processLen < 0 || dataSize < processLen || _recvBuffer.OnRead(processLen) == false)
    {
        Disconnect(L"OnRead Overflow");
        return;
    }

    _recvBuffer.Clean();  // 버퍼 정리

    RegisterRecv();  // 다음 수신 대기 등록
}
```

### RecvBuffer 구조

```
수신 전:
┌───────────────────────────────────────────┐
│ [이전데이터][       빈 공간              ] │
│      ↑                                    │
│   ReadPos                                 │
└───────────────────────────────────────────┘

수신 후:
┌───────────────────────────────────────────┐
│ [이전데이터][새로운 데이터][  빈 공간    ] │
│      ↑            ↑                       │
│   ReadPos      WritePos                   │
└───────────────────────────────────────────┘
```

---

## 4단계: 패킷 조립 & 보안 검증

**파일:** `ServerCore/Session.cpp:393-507` (PacketSession::OnRecv)

이 단계가 가장 복잡합니다. 여러 작업을 수행합니다:

### 4-1. 패킷 경계 확인

```cpp
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
    int32 processLen = 0;

    while (true)
    {
        int32 dataSize = len - processLen;

        // 최소한 size(2바이트)는 있어야 함
        if (dataSize < sizeof(uint16))
            break;

        // 패킷 크기 읽기
        uint16 packetSize = *(reinterpret_cast<uint16*>(&buffer[processLen]));

        // 전체 패킷이 아직 안 왔으면 대기
        if (dataSize < packetSize)
            break;

        // ... 패킷 처리
    }
}
```

### 왜 경계 확인이 필요한가?

TCP는 **스트림** 프로토콜이라서 패킷 경계가 없습니다:

```
보낸 것:  [패킷A][패킷B][패킷C]

받을 수 있는 상황들:
  상황1: [패킷A][패킷B][패킷C]     ← 운 좋게 한번에
  상황2: [패킷A][패킷B의 절반]     ← 패킷B가 잘림!
  상황3: [패킷A의 절반]            ← 패킷A도 잘림!
```

그래서 `size` 필드를 먼저 읽고, 그만큼 데이터가 있는지 확인합니다.

---

### 4-2. 암호화 처리 (선택적)

```cpp
// 암호화가 켜져있으면
if (GEncryptionEnabled && _crypto)
{
    // 패킷 구조: [size(2)][encrypted][HMAC(32)]
    int32 encryptedPayloadSize = packetSize - sizeof(uint16) - HMAC_SIZE;

    BYTE* encryptedData = &buffer[processLen + sizeof(uint16)];
    BYTE* receivedHmac = &buffer[processLen + sizeof(uint16) + encryptedPayloadSize];

    // ① HMAC 검증 (무결성 확인)
    if (!_crypto->VerifyHMAC(encryptedData, encryptedPayloadSize, receivedHmac))
    {
        cout << "HMAC verification failed - packet tampered!" << endl;
        return -1;  // 연결 끊김
    }

    // ② 복호화
    int32 decryptedLen = _crypto->Decrypt(
        encryptedData,
        encryptedPayloadSize,
        _decryptBuffer + sizeof(uint16),
        sizeof(_decryptBuffer) - sizeof(uint16)
    );

    // ... 복호화된 데이터 사용
}
```

### 암호화 패킷 구조

```
평문 패킷:
┌────────┬────────┬───────┬──────────┬──────────────┐
│  size  │   id   │ flags │ sequence │   payload    │
│ 2bytes │ 2bytes │ 1byte │  4bytes  │   N bytes    │
└────────┴────────┴───────┴──────────┴──────────────┘

암호화 패킷:
┌────────┬─────────────────────────────────┬──────────┐
│  size  │        encrypted data           │   HMAC   │
│ 2bytes │   (id+flags+sequence+payload)   │ 32bytes  │
└────────┴─────────────────────────────────┴──────────┘
```

---

### 4-3. Sequence 검증 (리플레이 공격 방지)

```cpp
PacketHeader* header = reinterpret_cast<PacketHeader*>(packetData);

if (header->flags & PKT_FLAG_HAS_SEQUENCE)
{
    if (header->sequence == _recvSeq)
    {
        // 같은 시퀀스 → 재전송 요청으로 간주
        if (_lastResponse)
            Send(_lastResponse);  // 캐시된 응답 재전송
        continue;
    }
    else if (header->sequence < _recvSeq)
    {
        // 이전 시퀀스 → 리플레이 공격!
        cout << "Replay attack detected!" << endl;
        return -1;
    }

    _recvSeq = header->sequence;  // 시퀀스 갱신
}
```

### 리플레이 공격이란?

```
해커가 정상 패킷을 캡처해서 다시 보내는 공격:

정상 플레이어: [스킬사용 seq=5] ────────────→ 서버
해커 (도청):    [스킬사용 seq=5] 캡처!
해커 (재전송):  [스킬사용 seq=5] ────────────→ 서버
                                               ↓
서버: "seq=5는 이미 처리했는데? 리플레이 공격이다!" → 차단
```

---

## 5단계: 게임 세션 처리

**파일:** `GameServer/GameSession.cpp:38-54`

```cpp
void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    // 모니터링: TPS 카운트
    ServerMonitor::Instance().OnTransaction();

    PacketSessionRef session = GetPacketSessionRef();
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // ★ Rate Limiting 체크
    if (_rateLimiter.CheckRateLimit(header->id) == false)
    {
        cout << "[RateLimit] Exceeded! PacketId=" << header->id << endl;
        Disconnect(L"Rate limit exceeded");
        return;
    }

    // 모든 검증 통과! → 패킷 핸들러로
    ServerPacketHandler::HandlePacket(session, buffer, len);
}
```

### Rate Limiting 설정 (OnConnected에서)

```cpp
void GameSession::OnConnected()
{
    // ...
    _rateLimiter.AddRule(PKT_C2S_MOVE, 10);  // 이동: 초당 10회 제한
}
```

### 왜 Rate Limiting이 필요한가?

```
악성 클라이언트가 1초에 1000번 이동 패킷을 보내면?
→ 서버 과부하 (DoS 공격)
→ 다른 플레이어들 렉 발생

Rate Limiting:
"이동은 초당 10번만 허용!" → 11번째부터는 연결 끊음
```

---

## 6단계: 패킷 핸들러 라우팅

**파일:** `GameServer/ServerPacketHandler.cpp`

```cpp
// 전역 핸들러 테이블
PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 초기화 시 핸들러 등록
void ServerPacketHandler::Init()
{
    GPacketHandler[PKT_C2S_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::C2S_ENTER_GAME>(Handle_C2S_ENTER_GAME, session, buffer, len);
    };
    GPacketHandler[PKT_C2S_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::C2S_MOVE>(Handle_C2S_MOVE, session, buffer, len);
    };
    // ...
}

// 패킷 처리 진입점
bool ServerPacketHandler::HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    return GPacketHandler[header->id](session, buffer, len);  // ID로 핸들러 찾아서 호출
}
```

### 실제 핸들러 예시 (Handle_C2S_MOVE)

```cpp
bool Handle_C2S_MOVE(GameSessionRef& session, Protocol::C2S_MOVE& pkt)
{
    // 1. 플레이어 객체 가져오기
    PlayerRef myPlayer = session->myPlayer.load();
    if (myPlayer == nullptr)
        return false;

    // 2. 플레이어가 속한 Room 가져오기
    RoomRef room = myPlayer->_room.load().lock();
    if (room == nullptr)
        return false;

    // 3. Room의 JobQueue에 작업 등록 (비동기 처리)
    room->DoAsync(&Room::HandleMove, myPlayer, pkt);

    return true;
}
```

---

## 전체 코드 흐름 (라인 번호 포함)

| 단계 | 파일 | 라인 | 함수명 |
|------|------|------|--------|
| 1 | `ServerCore/IocpCore.cpp` | 25-52 | `IocpCore::Dispatch()` |
| 2 | `ServerCore/Session.cpp` | 84-103 | `Session::Dispatch()` |
| 3 | `ServerCore/Session.cpp` | 245-274 | `Session::ProcessRecv()` |
| 4 | `ServerCore/Session.cpp` | 393-507 | `PacketSession::OnRecv()` |
| 5 | `GameServer/GameSession.cpp` | 38-54 | `GameSession::OnRecvPacket()` |
| 6 | `GameServer/ServerPacketHandler.cpp` | 15-70 | `HandlePacket()` → `Handle_C2S_*()` |

---

## 시퀀스 다이어그램

```
     Worker Thread              Session 계층                  Game 계층
          │                         │                            │
          │ GetQueuedCompletion     │                            │
          │ Status() 반환           │                            │
          │─────────────────────────>                            │
          │                         │                            │
          │                    Dispatch()                        │
          │                    (EventType 분류)                  │
          │                         │                            │
          │                  ProcessRecv()                       │
          │                  (버퍼에 데이터 축적)                 │
          │                         │                            │
          │               PacketSession::OnRecv()                │
          │               ┌─────────────────────┐                │
          │               │ • 패킷 경계 확인     │                │
          │               │ • HMAC 검증         │                │
          │               │ • 복호화            │                │
          │               │ • Sequence 검증     │                │
          │               └─────────────────────┘                │
          │                         │                            │
          │                         │ OnRecvPacket()             │
          │                         │───────────────────────────>│
          │                         │                            │
          │                         │               ┌────────────┴───────────┐
          │                         │               │ • Rate Limit 체크       │
          │                         │               │ • ServerPacketHandler  │
          │                         │               │ • Handle_C2S_MOVE()    │
          │                         │               │ • Room::DoAsync()      │
          │                         │               └────────────────────────┘
          │                         │                            │
          │                  RegisterRecv()                      │
          │                  (다음 수신 대기)                     │
          │                         │                            │
```

---

## 요약: 한 줄 정리

```
IOCP감지 → 이벤트분류 → 버퍼축적 → 패킷조립/보안검증 → Rate Limit → 핸들러실행
```

각 단계마다 검증에 실패하면 **즉시 연결이 끊어집니다** (보안).

---

> 문서 작성일: 2026-01-07
> 대상 프로젝트: NamoServer
