# NamoServer 네트워크 구조 가이드

> 신입 프로그래머를 위한 게임 서버 네트워크 아키텍처 문서

---

## 목차

1. [프로젝트 전체 구조](#1-프로젝트-전체-구조)
2. [네트워크 기반 기술 - IOCP](#2-네트워크-기반-기술---iocp)
3. [세션(Session) 계층 구조](#3-세션session-계층-구조)
4. [서버 서비스 아키텍처](#4-서버-서비스-아키텍처)
5. [패킷 구조와 프로토콜](#5-패킷-구조와-프로토콜)
6. [패킷 송수신 흐름](#6-패킷-송수신-흐름)
7. [게임 로직 연동 (Room & JobQueue)](#7-게임-로직-연동-room--jobqueue)
8. [보안 및 안정성](#8-보안-및-안정성)

---

## 1. 프로젝트 전체 구조

### 1.1 디렉토리 구성

```
NamoServer/
├── ServerCore/          # 네트워크 핵심 라이브러리 (C++)
│   ├── Session.h/cpp    # 기본 세션 클래스
│   ├── Listener.h/cpp   # 클라이언트 수락 담당
│   ├── Service.h/cpp    # 서비스 (Server/Client)
│   ├── IocpCore.h/cpp   # IOCP 핸들링
│   ├── IocpEvent.h      # 각종 이벤트 정의
│   ├── RecvBuffer.h/cpp # 수신 버퍼
│   └── SendBuffer.h/cpp # 송신 버퍼
│
├── GameServer/          # 게임 서버 로직 (C++)
│   ├── GameSession.h/cpp      # 게임 세션 클래스
│   ├── ServerPacketHandler.h  # 패킷 처리
│   ├── Room.h/cpp             # 게임 룸
│   ├── Player.h/cpp           # 플레이어 객체
│   └── Monster.h/cpp          # 몬스터 객체
│
├── Protocol/            # 프로토콜 정의 (Protobuf)
│   └── Protocol/
│       ├── Protocol.proto     # 패킷 정의
│       ├── Enum.proto         # 열거형
│       ├── Struct.proto       # 구조체
│       ├── cpp_output/        # C++ 코드 생성
│       └── csharp_output/     # C# 코드 생성
│
├── Common/              # 공용 데이터
├── DummyClient/         # 테스트용 클라이언트
└── Libraries/           # 외부 라이브러리
```

### 1.2 핵심 모듈 소개

| 모듈 | 역할 | 주요 파일 |
|------|------|----------|
| **ServerCore** | 네트워크 엔진 (재사용 가능) | Session, IocpCore, Service |
| **GameServer** | 게임 비즈니스 로직 | GameSession, Room, Player |
| **Protocol** | 클라이언트-서버 통신 규약 | *.proto 파일들 |

---

## 2. 네트워크 기반 기술 - IOCP

### 2.1 IOCP(I/O Completion Port) 개념

IOCP는 Windows에서 제공하는 **고성능 비동기 I/O 메커니즘**입니다.

```
전통적인 방식 (스레드 per 클라이언트)
┌─────────┐  ┌─────────┐  ┌─────────┐
│Thread 1 │  │Thread 2 │  │Thread 3 │  ... 1000개 스레드 필요
│Client 1 │  │Client 2 │  │Client 3 │
└─────────┘  └─────────┘  └─────────┘

IOCP 방식 (스레드 풀)
┌─────────────────────────────────────┐
│           IOCP (커널)               │
│  1000개 소켓의 I/O 완료를 관리      │
└─────────────────────────────────────┘
        │         │         │
   ┌────┴───┐ ┌───┴────┐ ┌──┴─────┐
   │Worker 1│ │Worker 2│ │Worker 3│  ... 5개 스레드로 처리
   └────────┘ └────────┘ └─────────┘
```

### 2.2 핵심 클래스

**IocpCore** (`ServerCore/IocpCore.h`)
```cpp
class IocpCore
{
public:
    HANDLE GetHandle() { return _iocpHandle; }

    bool Register(IocpObjectRef iocpObject);  // 소켓을 IOCP에 등록
    bool Dispatch(uint32 timeoutMs = INFINITE);  // 완료된 I/O 처리

private:
    HANDLE _iocpHandle;  // IOCP 핸들
};
```

**IocpEvent** (`ServerCore/IocpEvent.h`)
```cpp
enum class EventType : uint8
{
    Connect,     // 연결 완료
    Disconnect,  // 연결 해제
    Accept,      // 클라이언트 수락
    Recv,        // 데이터 수신 완료
    Send         // 데이터 송신 완료
};
```

### 2.3 비동기 I/O의 장점

- **높은 동시 접속 처리**: 적은 스레드로 수천 개 연결 관리
- **효율적인 CPU 사용**: I/O 대기 시간에 다른 작업 처리
- **확장성**: 하드웨어 추가 없이 더 많은 클라이언트 수용

---

## 3. 세션(Session) 계층 구조

### 3.1 상속 관계

```
IocpObject (IOCP 이벤트 처리 인터페이스)
    │
    └── Session (기본 세션 - 소켓 연결 관리)
            │
            └── PacketSession (패킷 단위 처리)
                    │
                    └── GameSession (게임 로직 연동)
```

### 3.2 각 계층별 책임

**Session** (`ServerCore/Session.h`)
- 소켓 생성/연결/해제
- 버퍼 관리 (RecvBuffer, SendBuffer)
- 암호화/복호화 처리

```cpp
class Session : public IocpObject
{
public:
    void Send(SendBufferRef sendBuffer);
    void Disconnect(const WCHAR* cause);

protected:
    virtual void OnConnected() { }
    virtual void OnDisconnected() { }
    virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
    virtual void OnSend(int32 len) { }
};
```

**PacketSession** (`ServerCore/Session.h:127`)
- 패킷 경계 처리 (헤더의 size 필드 기반)
- 시퀀스 번호 검증
- 패킷 핸들러 호출

```cpp
class PacketSession : public Session
{
protected:
    virtual int32 OnRecv(BYTE* buffer, int32 len) sealed;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) = 0;
};
```

**GameSession** (`GameServer/GameSession.h`)
- 플레이어 객체 연결
- Rate Limiting
- Ping/Pong 관리

```cpp
class GameSession : public PacketSession
{
public:
    atomic<shared_ptr<Player>> myPlayer;

protected:
    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;

private:
    RateLimiter _rateLimiter;
};
```

### 3.3 세션 생명주기

```
1. 클라이언트 연결 요청
       │
       ▼
2. Listener::ProcessAccept()
   └── 새 GameSession 생성
       │
       ▼
3. GameSession::OnConnected()
   ├── SessionManager에 등록
   ├── 암호화 초기화
   └── Rate Limiter 설정
       │
       ▼
4. 통신 루프
   ├── OnRecv() → 패킷 처리
   └── Send() → 응답 전송
       │
       ▼
5. GameSession::OnDisconnected()
   ├── 플레이어 정리 (Room에서 제거)
   ├── 객체 정리 (ObjectManager에서 제거)
   └── SessionManager에서 제거
```

---

## 4. 서버 서비스 아키텍처

### 4.1 Service 클래스 구조

```cpp
// ServerCore/Service.h
class Service : public enable_shared_from_this<Service>
{
public:
    virtual bool Start() = 0;
    SessionRef CreateSession();
    void AddSession(SessionRef session);
    void ReleaseSession(SessionRef session);

protected:
    IocpCoreRef _iocpCore;
    SessionFactory _sessionFactory;  // 세션 생성 팩토리 함수
    int32 _maxSessionCount;
};
```

### 4.2 ServerService vs ClientService

| 구분 | ServerService | ClientService |
|------|---------------|---------------|
| 역할 | 클라이언트 접속 대기 | 다른 서버로 연결 |
| Listener | 포함 (Accept 처리) | 없음 |
| 사용 예시 | 게임 서버 | 서버 간 통신 |

### 4.3 서버 초기화 코드

```cpp
// GameServer/GameServer.cpp
int main()
{
    // 1. 서버 서비스 생성
    ServerServiceRef service = make_shared<ServerService>(
        NetAddress(L"127.0.0.1", 7777),  // 바인딩 주소
        make_shared<IocpCore>(),          // IOCP 코어
        [=]() { return make_shared<GameSession>(); },  // 세션 팩토리
        100  // 최대 세션 수
    );

    // 2. 서비스 시작 (Listener 활성화)
    service->Start();

    // 3. Worker Thread 실행 (5개)
    for (int32 i = 0; i < 5; i++)
    {
        GThreadManager->Launch([&service]() {
            while (true)
            {
                // IOCP 이벤트 처리
                service->GetIocpCore()->Dispatch(10);

                // JobQueue 작업 처리
                ThreadManager::DistributeReservedJobs();
                ThreadManager::DoGlobalQueueWork();
            }
        });
    }
}
```

### 4.4 전체 아키텍처 다이어그램

```
┌─────────────────────────────────────────────────────────────┐
│                      GameServer                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐     ┌─────────────────────────────────┐  │
│  │   Listener   │────▶│         IOCP Core               │  │
│  │ (Port 7777)  │     │  ┌─────┬─────┬─────┬─────┬────┐ │  │
│  └──────────────┘     │  │ W1  │ W2  │ W3  │ W4  │ W5 │ │  │
│         │             │  └──┬──┴──┬──┴──┬──┴──┬──┴──┬─┘ │  │
│         │             └─────┼─────┼─────┼─────┼─────┼───┘  │
│         ▼                   │     │     │     │     │      │
│  ┌──────────────┐           ▼     ▼     ▼     ▼     ▼      │
│  │ GameSession  │◀─────────────────────────────────────    │
│  │   Pool       │                                          │
│  └──────────────┘     ┌─────────────────────────────────┐  │
│         │             │              Room                │  │
│         └────────────▶│  ┌────────┐  ┌────────┐         │  │
│                       │  │Player 1│  │Player 2│  ...    │  │
│                       │  └────────┘  └────────┘         │  │
│                       └─────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. 패킷 구조와 프로토콜

### 5.1 PacketHeader 구조

```cpp
// ServerCore/Session.h:116-124
#pragma pack(push, 1)
struct PacketHeader
{
    uint16 size;      // 전체 패킷 크기 (헤더 포함)
    uint16 id;        // 패킷 ID (어떤 종류의 패킷인지)
    uint8  flags;     // 플래그 (시퀀스 포함 여부 등)
    uint32 sequence;  // 시퀀스 번호 (리플레이 공격 방지)
};
#pragma pack(pop)
// 총 9바이트 (패딩 없음)
```

**패킷 메모리 레이아웃:**
```
┌────────┬────────┬───────┬──────────┬─────────────────┐
│  size  │   id   │ flags │ sequence │   payload       │
│ 2bytes │ 2bytes │ 1byte │  4bytes  │  (Protobuf)     │
└────────┴────────┴───────┴──────────┴─────────────────┘
│◄─────────── header (9bytes) ────────▶│◄── body ─────▶│
```

### 5.2 패킷 ID 체계

```cpp
// Protocol/Protocol/Protocol.proto
enum MsgId
{
    // 1000번대: 게임 진입/퇴장
    PKT_C2S_ENTER_GAME = 1000;  // 클라이언트 → 서버: 게임 입장
    PKT_S2C_ENTER_GAME = 1001;  // 서버 → 클라이언트: 입장 응답
    PKT_S2C_LEAVE_GAME = 1002;  // 서버 → 클라이언트: 퇴장 알림
    PKT_S2C_PING       = 1003;  // 서버 → 클라이언트: 핑
    PKT_C2S_PONG       = 1004;  // 클라이언트 → 서버: 퐁

    // 2000번대: 게임플레이
    PKT_S2C_SPAWN      = 2000;  // 오브젝트 생성
    PKT_S2C_DESPAWN    = 2001;  // 오브젝트 제거
    PKT_C2S_MOVE       = 2002;  // 이동 요청
    PKT_S2C_MOVE       = 2003;  // 이동 브로드캐스트
    PKT_C2S_SKILL      = 2004;  // 스킬 사용
    PKT_S2C_SKILL      = 2005;  // 스킬 브로드캐스트
    PKT_S2C_CHANGE_HP  = 2006;  // HP 변경
    PKT_S2C_DIE        = 2007;  // 사망
}
```

### 5.3 Protobuf 메시지 정의

**공통 구조체** (`Protocol/Protocol/Struct.proto`)
```protobuf
message PositionInfo {
    CreatureState state = 1;  // Idle, Moving, Skill, Dead
    MoveDir moveDir = 2;      // Up, Down, Left, Right
    int32 posX = 3;
    int32 posY = 4;
}

message StatInfo {
    int32 level = 1;
    int32 hp = 2;
    int32 maxHp = 3;
    int32 attack = 4;
    float speed = 5;
    int32 totalExp = 6;
}

message ObjectInfo {
    int32 objectId = 1;
    string name = 2;
    PositionInfo posInfo = 3;
    StatInfo statInfo = 4;
}
```

**패킷 메시지** (`Protocol/Protocol/Protocol.proto`)
```protobuf
// 이동 요청
message C2S_MOVE {
    PositionInfo posInfo = 1;
}

// 이동 브로드캐스트
message S2C_MOVE {
    int32 objectId = 1;
    PositionInfo posInfo = 2;
}

// 오브젝트 생성
message S2C_SPAWN {
    repeated ObjectInfo objects = 1;
}

// 스킬 사용
message C2S_SKILL {
    SkillInfo info = 1;
}
```

### 5.4 패킷 네이밍 규칙

- `C2S_` : Client to Server (클라이언트 → 서버 요청)
- `S2C_` : Server to Client (서버 → 클라이언트 응답/브로드캐스트)

---

## 6. 패킷 송수신 흐름

### 6.1 수신 흐름 (클라이언트 → 서버)

```
┌─────────────┐
│  클라이언트  │
└──────┬──────┘
       │ TCP 패킷 전송
       ▼
┌──────────────────────────────────────────────────────────┐
│                      서버 측                              │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  1. IOCP Dispatch() - RecvEvent 완료 감지                │
│         │                                                │
│         ▼                                                │
│  2. Session::ProcessRecv()                               │
│     └── RecvBuffer에 데이터 축적                         │
│         │                                                │
│         ▼                                                │
│  3. PacketSession::OnRecv()                              │
│     ├── 패킷 헤더 파싱 (size, id 확인)                   │
│     ├── 완전한 패킷인지 확인 (size <= 받은 데이터)       │
│     ├── 복호화 (암호화 활성화 시)                        │
│     └── 시퀀스 번호 검증                                 │
│         │                                                │
│         ▼                                                │
│  4. GameSession::OnRecvPacket()                          │
│     └── Rate Limiter 체크                                │
│         │                                                │
│         ▼                                                │
│  5. ServerPacketHandler::HandlePacket()                  │
│     └── 패킷 ID로 핸들러 함수 라우팅                     │
│         │                                                │
│         ▼                                                │
│  6. Handle_C2S_MOVE() 등 개별 핸들러                     │
│     ├── Protobuf ParseFromArray()                        │
│     └── Room::DoAsync() - JobQueue에 작업 등록           │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### 6.2 송신 흐름 (서버 → 클라이언트)

```cpp
// 1. 메시지 생성
Protocol::S2C_MOVE movePkt;
movePkt.set_objectid(player->_objectId);
movePkt.mutable_posinfo()->CopyFrom(player->_posInfo);

// 2. SendBuffer 생성 (헤더 + Protobuf 직렬화)
auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);

// 3. 브로드캐스트 (같은 Room의 모든 플레이어에게)
room->Broadcast(sendBuffer);

// 내부적으로:
// - Session::Send() 호출
// - SendQueue에 버퍼 추가
// - IOCP SendEvent 등록
// - 커널이 비동기로 전송 완료
```

### 6.3 ServerPacketHandler 구조

```cpp
// GameServer/ServerPacketHandler.h
class ServerPacketHandler
{
public:
    static void Init()
    {
        // 패킷 ID별 핸들러 등록
        _handlers[PKT_C2S_ENTER_GAME] = Handle_C2S_ENTER_GAME;
        _handlers[PKT_C2S_MOVE] = Handle_C2S_MOVE;
        _handlers[PKT_C2S_SKILL] = Handle_C2S_SKILL;
        _handlers[PKT_C2S_PONG] = Handle_C2S_PONG;
    }

    static bool HandlePacket(PacketSessionRef session, BYTE* buffer, int32 len)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        return _handlers[header->id](session, buffer, len);
    }

    // 개별 핸들러들
    static bool Handle_C2S_ENTER_GAME(PacketSessionRef session, BYTE* buffer, int32 len);
    static bool Handle_C2S_MOVE(PacketSessionRef session, BYTE* buffer, int32 len);
    static bool Handle_C2S_SKILL(PacketSessionRef session, BYTE* buffer, int32 len);

    // SendBuffer 생성 헬퍼
    template<typename T>
    static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId);

private:
    static unordered_map<uint16, PacketHandler> _handlers;
};
```

---

## 7. 게임 로직 연동 (Room & JobQueue)

### 7.1 Room 개념

Room은 같은 공간에 있는 플레이어들을 그룹화하는 단위입니다.

```cpp
// GameServer/Room.h
class Room : public JobQueue
{
public:
    void HandleEnterGame(PlayerRef player);
    void HandleLeaveGame(int32 objectId);
    void HandleMove(PlayerRef player, Protocol::C2S_MOVE pkt);
    void HandleSkill(PlayerRef player, Protocol::C2S_SKILL pkt);

    void Broadcast(SendBufferRef sendBuffer);  // 전체 전송
    void SendPing();  // 정기 핑

private:
    map<int32, PlayerRef> _players;
    map<int32, MonsterRef> _monsters;

    static const uint64 _pingInterval = 10000;  // 10초
    static const uint64 _pongTimeout = 3000;    // 3초
};
```

### 7.2 JobQueue를 통한 비동기 처리

**문제점**: 여러 Worker Thread가 동시에 Room 데이터에 접근하면 Race Condition 발생

**해결책**: JobQueue 패턴

```cpp
// 패킷 핸들러에서
bool Handle_C2S_MOVE(PacketSessionRef session, BYTE* buffer, int32 len)
{
    auto gameSession = static_pointer_cast<GameSession>(session);
    auto player = gameSession->myPlayer.load();

    Protocol::C2S_MOVE pkt;
    pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader));

    // 직접 처리하지 않고 JobQueue에 등록
    RoomManager::Instance().Find(1)->DoAsync(&Room::HandleMove, player, pkt);

    return true;
}

// Room에서 순차적으로 처리
void Room::HandleMove(PlayerRef player, Protocol::C2S_MOVE pkt)
{
    // 이 함수는 단일 스레드에서만 실행됨 (안전)
    player->_posInfo = pkt.posinfo();

    // 다른 플레이어들에게 브로드캐스트
    Protocol::S2C_MOVE movePkt;
    movePkt.set_objectid(player->_objectId);
    movePkt.mutable_posinfo()->CopyFrom(pkt.posinfo());

    Broadcast(ServerPacketHandler::MakeSendBuffer(movePkt));
}
```

### 7.3 브로드캐스트 메커니즘

```cpp
void Room::Broadcast(SendBufferRef sendBuffer)
{
    for (auto& [id, player] : _players)
    {
        auto session = player->_ownerSession.lock();
        if (session)
            session->Send(sendBuffer);
    }
}
```

```
       ┌─────────────────────────────────────┐
       │              Room                    │
       │  ┌────────┐  ┌────────┐  ┌────────┐ │
       │  │Player A│  │Player B│  │Player C│ │
       │  └───┬────┘  └───┬────┘  └───┬────┘ │
       └──────┼───────────┼───────────┼──────┘
              │           │           │
              ▼           ▼           ▼
         ┌────────┐  ┌────────┐  ┌────────┐
         │Session │  │Session │  │Session │
         │   A    │  │   B    │  │   C    │
         └───┬────┘  └───┬────┘  └───┬────┘
             │           │           │
    ─────────┴───────────┴───────────┴─────────▶ 네트워크
```

---

## 8. 보안 및 안정성

### 8.1 AES 암호화

```cpp
// ServerCore/Session.h:85
class Session
{
protected:
    AESCrypto* _crypto = nullptr;
};

// 전역 설정
extern bool GEncryptionEnabled;

// 송신 시 암호화
void Session::Send(SendBufferRef sendBuffer)
{
    if (GEncryptionEnabled && _crypto)
    {
        sendBuffer = EncryptBuffer(sendBuffer);
    }
    // ... 전송 로직
}

// 수신 시 복호화
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
    if (GEncryptionEnabled && _crypto)
    {
        buffer = DecryptBuffer(buffer, len);
    }
    // ... 패킷 처리
}
```

### 8.2 Sequence 번호 (리플레이 공격 방지)

```cpp
// 송신 시
void Session::Send(SendBufferRef sendBuffer)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Data());

    if (header->flags & PKT_FLAG_HAS_SEQUENCE)
    {
        header->sequence = ++_sendSeq;  // 시퀀스 증가
        _lastResponse = sendBuffer;      // 재전송용 캐시
    }
}

// 수신 시 검증
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    if (header->flags & PKT_FLAG_HAS_SEQUENCE)
    {
        if (header->sequence <= _lastRecvSeq)
        {
            // 이미 처리한 패킷 (리플레이 공격 의심)
            return -1;
        }
        _lastRecvSeq = header->sequence;
    }
}
```

### 8.3 Rate Limiting (DDoS 방어)

```cpp
// GameServer/GameSession.cpp
void GameSession::OnConnected()
{
    // 패킷별 속도 제한 설정
    _rateLimiter.AddRule(PKT_C2S_MOVE, 10);   // 이동: 초당 10회
    _rateLimiter.AddRule(PKT_C2S_SKILL, 5);   // 스킬: 초당 5회
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // Rate Limit 체크
    if (_rateLimiter.CheckRateLimit(header->id) == false)
    {
        Disconnect(L"Rate limit exceeded");
        return;
    }

    ServerPacketHandler::HandlePacket(
        static_pointer_cast<PacketSession>(shared_from_this()),
        buffer, len);
}
```

### 8.4 Ping/Pong 연결 상태 확인

```cpp
// Room.h
static const uint64 _pingInterval = 10000;  // 10초마다 핑
static const uint64 _pongTimeout = 3000;    // 3초 내 응답 필요

// Room.cpp
void Room::SendPing()
{
    uint64 now = GetTickCount64();

    for (auto& [id, player] : _players)
    {
        auto session = static_pointer_cast<GameSession>(player->_ownerSession.lock());
        if (!session) continue;

        // 퐁 응답 체크
        if (session->_lastPingTime > 0 &&
            now - session->_pongTime > _pongTimeout)
        {
            session->Disconnect(L"Pong timeout");
            continue;
        }

        // 핑 전송
        Protocol::S2C_PING pingPkt;
        pingPkt.set_timestamp(now);
        session->Send(ServerPacketHandler::MakeSendBuffer(pingPkt));
        session->_lastPingTime = now;
    }
}
```

**타임라인:**
```
서버                              클라이언트
  │                                   │
  │──── S2C_PING (timestamp) ────────▶│
  │     _lastPingTime = now           │
  │                                   │
  │◀──── C2S_PONG (timestamp) ────────│
  │     _pongTime = now               │
  │                                   │
  │ (3초 이내에 PONG이 오지 않으면)   │
  │     Disconnect()                  │
```

---

## 부록: 주요 파일 경로

| 구분 | 파일 경로 |
|------|----------|
| IOCP 코어 | `ServerCore/IocpCore.h` |
| 세션 기본 | `ServerCore/Session.h` |
| 서비스 관리 | `ServerCore/Service.h` |
| 리스너 | `ServerCore/Listener.h` |
| 게임 세션 | `GameServer/GameSession.h` |
| 패킷 핸들러 | `GameServer/ServerPacketHandler.h` |
| 게임 룸 | `GameServer/Room.h` |
| 프로토콜 정의 | `Protocol/Protocol/Protocol.proto` |
| 열거형 | `Protocol/Protocol/Enum.proto` |
| 구조체 | `Protocol/Protocol/Struct.proto` |

---

## 성능 특성 요약

| 항목 | 값 | 설명 |
|------|-----|------|
| 최대 세션 수 | 100 | 설정 가능 |
| Worker Thread | 5개 | IOCP 처리 |
| RecvBuffer 크기 | 64KB | 세션당 |
| Ping 간격 | 10초 | 연결 확인 |
| Pong 타임아웃 | 3초 | 응답 대기 |
| Rate Limit (이동) | 10/초 | 스팸 방지 |
| Rate Limit (스킬) | 5/초 | 스팸 방지 |

---

> 문서 작성일: 2026-01-07
> 대상 프로젝트: NamoServer
