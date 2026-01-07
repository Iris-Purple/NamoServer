# 패킷 송신 흐름 상세 가이드

> 서버에서 클라이언트로 패킷을 전송하는 과정

---

## 전체 흐름 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│  1단계: 게임 로직에서 패킷 전송 결정                                      │
│  ───────────────────────────────                                        │
│  Room::HandleMove(), Room::Broadcast() 등                              │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  2단계: SendBuffer 생성                                                  │
│  ─────────────────────                                                  │
│  ServerPacketHandler::MakeSendBuffer()                                  │
│  - Protobuf 메시지 → 바이너리 직렬화                                     │
│  - PacketHeader 붙이기                                                  │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  3단계: 세션에 전송 요청                                                  │
│  ─────────────────────                                                  │
│  Session::Send()                                                        │
│  - Sequence 번호 설정 (필요시)                                           │
│  - 암호화 (활성화시)                                                     │
│  - SendQueue에 추가                                                     │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  4단계: IOCP에 송신 등록                                                  │
│  ─────────────────────                                                  │
│  Session::RegisterSend()                                                │
│  - SendQueue에서 버퍼들 꺼내기                                           │
│  - WSASend() 비동기 호출                                                │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  5단계: 송신 완료 처리                                                    │
│  ─────────────────────                                                  │
│  Session::ProcessSend()                                                 │
│  - 송신 버퍼 정리                                                       │
│  - 대기 중인 버퍼 있으면 다시 RegisterSend()                             │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         클라이언트 수신 완료                              │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1단계: 게임 로직에서 패킷 전송 결정

**파일:** `GameServer/Room.cpp`

게임 로직에서 클라이언트에게 정보를 보내야 할 때 패킷 전송이 시작됩니다.

### 예시 1: 플레이어 이동 브로드캐스트

```cpp
// Room.cpp
void Room::HandleMove(PlayerRef player, Protocol::C2S_MOVE pkt)
{
    // ... 이동 처리 로직 ...

    // ① Protobuf 메시지 생성
    Protocol::S2C_MOVE resPkt;
    resPkt.set_objectid(player->_objectId);
    resPkt.mutable_posinfo()->CopyFrom(player->ToPositionInfo());

    // ② SendBuffer 생성
    SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);

    // ③ 모든 플레이어에게 브로드캐스트
    Broadcast(sendBuffer, player->_objectId);
}
```

---

## 2단계: SendBuffer 생성

**파일:** `GameServer/ServerPacketHandler.h`

```cpp
template<typename T>
static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
{
    // ① Protobuf 메시지 크기 계산
    const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
    const uint16 packetSize = dataSize + sizeof(PacketHeader);

    // ② SendBuffer 생성 (필요한 크기만큼)
    SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);

    // ③ 버퍼 앞부분에 PacketHeader 작성
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
    header->size = packetSize;           // 전체 패킷 크기
    header->id = pktId;                  // 패킷 ID (예: PKT_S2C_MOVE)
    header->flags = NeedsSequence(pktId) ? PKT_FLAG_HAS_SEQUENCE : 0;
    header->sequence = 0;                // Send()에서 설정됨

    // ④ 헤더 뒤에 Protobuf 데이터 직렬화
    pkt.SerializeToArray(&header[1], dataSize);

    // ⑤ 쓰기 완료 표시
    sendBuffer->Close(packetSize);

    return sendBuffer;
}
```

### SendBuffer 메모리 구조

```
SendBuffer 생성 후:
┌───────────────────────────────────────────────────────┐
│ [PacketHeader (9bytes)] [Protobuf 직렬화 데이터]       │
└───────────────────────────────────────────────────────┘

PacketHeader 상세:
┌────────┬────────┬───────┬──────────┐
│  size  │   id   │ flags │ sequence │
│ 2bytes │ 2bytes │ 1byte │  4bytes  │
└────────┴────────┴───────┴──────────┘
    │         │       │         │
    │         │       │         └── 리플레이 방지 (Send에서 설정)
    │         │       └── PKT_FLAG_HAS_SEQUENCE 등
    │         └── 패킷 종류 (예: 2003 = S2C_MOVE)
    └── 전체 패킷 크기
```

---

## 3단계: 세션에 전송 요청

**파일:** `ServerCore/Session.cpp`

```cpp
void Session::Send(SendBufferRef sendBuffer)
{
    // ① 연결 상태 확인
    if (IsConnected() == false)
        return;

    // ② Sequence 설정 (필요한 패킷만)
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
    if (header->flags & PKT_FLAG_HAS_SEQUENCE)
    {
        _lastResponse = sendBuffer;      // 재전송용 캐시
        header->sequence = ++_sendSeq;   // 시퀀스 번호 증가
    }

    // ③ 암호화 (활성화된 경우)
    if (GEncryptionEnabled && _crypto)
    {
        sendBuffer = EncryptBuffer(sendBuffer);
        if (sendBuffer == nullptr)
            return;  // 암호화 실패
    }

    // ④ SendQueue에 추가
    bool registerSend = false;
    {
        WRITE_LOCK;

        _sendQueue.push(sendBuffer);

        // 현재 전송 중이 아니면 전송 시작
        if (_sendRegistered.exchange(true) == false)
            registerSend = true;

        if (registerSend)
            RegisterSend();
    }
}
```

### Send() 동작 흐름도

```
Session::Send(sendBuffer) 호출
         │
         ▼
    ┌────────────┐
    │ 연결됨?     │──── No ──→ return (무시)
    └────────────┘
         │ Yes
         ▼
    ┌────────────┐
    │ Sequence   │──── Yes ──→ _sendSeq++, 캐시 저장
    │ 필요?      │
    └────────────┘
         │ No
         ▼
    ┌────────────┐
    │ 암호화     │──── Yes ──→ EncryptBuffer()
    │ 활성화?    │
    └────────────┘
         │
         ▼
    ┌────────────┐
    │ SendQueue  │
    │ 에 추가    │
    └────────────┘
         │
         ▼
    ┌────────────┐
    │ 이미       │──── Yes ──→ return (기존 전송에 합류)
    │ 전송 중?   │
    └────────────┘
         │ No
         ▼
    RegisterSend() 호출 ──→ 4단계로
```

### 왜 SendQueue를 사용하는가?

```
Send()가 여러 스레드에서 동시에 호출될 수 있음:

Thread A: Send(패킷1)  ─┬─→ SendQueue: [패킷1, 패킷2, 패킷3]
Thread B: Send(패킷2)  ─┤
Thread C: Send(패킷3)  ─┘

→ 큐에 쌓고, 한 번의 WSASend()로 모아서 전송
→ 시스템 콜 횟수 감소 = 성능 향상
```

---

## 4단계: IOCP에 송신 등록

**파일:** `ServerCore/Session.cpp`

```cpp
void Session::RegisterSend()
{
    // ① 연결 상태 확인
    if (IsConnected() == false)
        return;

    // ② SendEvent 초기화
    _sendEvent.Init();
    _sendEvent.owner = shared_from_this();  // 참조 카운트 증가

    // ③ SendQueue에서 모든 버퍼 꺼내기
    {
        while (_sendQueue.empty() == false)
        {
            SendBufferRef sendBuffer = _sendQueue.front();
            _sendQueue.pop();
            _sendEvent.sendBuffers.push_back(sendBuffer);
        }
    }

    // ④ WSABUF 배열 준비 (Scatter-Gather I/O)
    vector<WSABUF> wsaBufs;
    wsaBufs.reserve(_sendEvent.sendBuffers.size());
    for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
    {
        WSABUF wsaBuf;
        wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
        wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
        wsaBufs.push_back(wsaBuf);
    }

    // ⑤ WSASend() 비동기 호출
    DWORD numOfBytes = 0;
    if (SOCKET_ERROR == ::WSASend(
            _socket,
            wsaBufs.data(),
            static_cast<DWORD>(wsaBufs.size()),
            OUT &numOfBytes,
            0,
            &_sendEvent,      // OVERLAPPED 구조체
            nullptr))
    {
        int32 errorCode = ::WSAGetLastError();
        if (errorCode != WSA_IO_PENDING)  // 진짜 에러
        {
            HandleError(errorCode);
            // ... 정리 작업
        }
        // WSA_IO_PENDING은 정상 (비동기 진행 중)
    }
}
```

### Scatter-Gather I/O 개념

```
일반적인 방식 (느림):
┌──────────┐   ┌──────────┐   ┌──────────┐
│ 패킷 A   │   │ 패킷 B   │   │ 패킷 C   │
└────┬─────┘   └────┬─────┘   └────┬─────┘
     │ send()       │ send()       │ send()
     ▼              ▼              ▼
   커널           커널           커널
   (3번 호출!)

Scatter-Gather 방식 (빠름):
┌──────────┐   ┌──────────┐   ┌──────────┐
│ 패킷 A   │   │ 패킷 B   │   │ 패킷 C   │
└────┬─────┘   └────┬─────┘   └────┬─────┘
     │              │              │
     └──────────────┼──────────────┘
                    ▼
               WSASend() (1번 호출!)
               WSABUF[] = [A, B, C]
                    ▼
                  커널
```

### WSASend() 동작

```cpp
::WSASend(
    _socket,           // 소켓
    wsaBufs.data(),    // 버퍼 배열 (Scatter-Gather)
    wsaBufs.size(),    // 버퍼 개수
    &numOfBytes,       // 즉시 전송된 바이트 (보통 0)
    0,                 // 플래그
    &_sendEvent,       // OVERLAPPED (완료 시 IOCP 통지)
    nullptr            // 콜백 (IOCP 사용시 불필요)
);
```

**반환값:**
- `0`: 즉시 완료 (드묾)
- `SOCKET_ERROR` + `WSA_IO_PENDING`: **정상** - 비동기 진행 중
- `SOCKET_ERROR` + 다른 에러: 실패

---

## 5단계: 송신 완료 처리

**파일:** `ServerCore/Session.cpp`

IOCP가 송신 완료를 감지하면 `ProcessSend()`가 호출됩니다.

```cpp
void Session::ProcessSend(int32 numOfBytes)
{
    // ① 참조 해제
    _sendEvent.owner = nullptr;
    _sendEvent.sendBuffers.clear();  // SendBuffer들 해제

    // ② 0바이트 전송 = 연결 끊김
    if (numOfBytes == 0)
    {
        Disconnect(L"Send 0");
        return;
    }

    // ③ 콜백 호출 (상위 클래스에서 오버라이드 가능)
    OnSend(numOfBytes);

    // ④ 대기 중인 버퍼가 있으면 다시 전송
    WRITE_LOCK;
    if (_sendQueue.empty())
        _sendRegistered.store(false);  // 전송 완료
    else
        RegisterSend();  // 큐에 쌓인 것들 전송
}
```

### 연속 전송 흐름

```
시간 →

┌──────────────────────────────────────────────────────────────────┐
│  Send() Send() Send()    RegisterSend()    ProcessSend()        │
│    │      │      │            │                 │               │
│    ▼      ▼      ▼            ▼                 ▼               │
│  Queue: [A, B, C]  →    WSASend([A,B,C])  →   완료!            │
│                                                  │               │
│                                                  ▼               │
│                                            Queue 비었나?         │
│                                                  │               │
│                                      ┌───── Yes ─┴─ No ─────┐   │
│                                      ▼                       ▼   │
│                              _sendRegistered=false    RegisterSend()
│                              (대기 상태)              (다시 전송)  │
└──────────────────────────────────────────────────────────────────┘
```

---

### 암호화 전후 패킷 비교

```
암호화 전 (평문):
┌────────┬────────┬───────┬──────────┬──────────────┐
│  size  │   id   │ flags │ sequence │   payload    │
│ 2bytes │ 2bytes │ 1byte │  4bytes  │   N bytes    │
└────────┴────────┴───────┴──────────┴──────────────┘

암호화 후:
┌────────┬────────────────────────────────────┬──────────┐
│  size  │     AES 암호화된 데이터             │   HMAC   │
│ 2bytes │   (id+flags+sequence+payload)      │ 32bytes  │
└────────┴────────────────────────────────────┴──────────┘
           ↑                                    ↑
           └── 클라이언트만 복호화 가능 ──┘ 무결성 검증
```

---

## 브로드캐스트 패턴

### 패턴 1: 전체 브로드캐스트 (Ping)

```cpp
// Room.cpp:77-81
Protocol::S2C_PING ping;
ping.set_timestamp(now);
SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(ping);
Broadcast(sendBuffer);  // 모든 플레이어에게
```

### 패턴 2: 자기 제외 브로드캐스트 (이동)

```cpp
// Room.cpp:279-283
Protocol::S2C_MOVE resPkt;
resPkt.set_objectid(player->_objectId);
resPkt.mutable_posinfo()->CopyFrom(player->ToPositionInfo());
SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
Broadcast(sendBuffer, player->_objectId);  // 자기 빼고 전송
```

### 패턴 3: 특정 대상에게만 전송

```cpp
// Room.cpp:206-214
Protocol::S2C_ENTER_GAME enterGamePkt;
enterGamePkt.mutable_player()->CopyFrom(player->ToObjectInfo());
SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
if (auto session = player->session.lock())
{
    session->Send(sendBuffer);  // 한 명에게만
}
```

---

## 요약: 한 줄 정리

```
게임로직 → MakeSendBuffer(Protobuf직렬화) → Send(암호화,큐잉) → RegisterSend(WSASend) → ProcessSend(완료)
```

### 핵심 설계 원칙

1. **비동기 I/O**: WSASend()는 즉시 반환, 완료는 IOCP가 통지
2. **배치 전송**: SendQueue에 모아서 한 번에 WSASend() (Scatter-Gather)
3. **버퍼 재사용**: 같은 SendBuffer를 여러 세션에 전송 가능
4. **스레드 안전**: Lock으로 SendQueue 보호

