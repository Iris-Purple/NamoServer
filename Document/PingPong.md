# PING/PONG 연결 상태 확인

> 클라이언트 연결 상태를 주기적으로 확인하는 하트비트(Heartbeat) 메커니즘

---

## 왜 PING/PONG이 필요한가?

### 문제 상황

```
┌─────────────┐                              ┌─────────────┐
│  클라이언트  │ ──── TCP 연결 ────────────▶ │    서버     │
└─────────────┘                              └─────────────┘
       │
       │ (갑자기 전원 OFF, 네트워크 끊김)
       │
       ✖ 연결이 끊어졌지만...

       서버는 모른다! (TCP는 즉시 감지 못함)
       └── 좀비 세션이 계속 남아있음
       └── 메모리 낭비
       └── 다른 플레이어에게 유령으로 보임
```

### 해결책: PING/PONG

```
서버: "야, 살아있어?" (PING)
          │
          ▼
클라이언트: "응, 살아있어!" (PONG)
          │
          └── 3초 안에 응답 없으면 → 연결 끊김 처리
```

---

## 설정 값

**파일:** `GameServer/Room.h`

```cpp
static const uint64 _pingInterval = 10000;  // 10초 (10,000ms)
static const uint64 _pongTimeout = 3000;    // 3초 (3,000ms)
```

| 설정 | 값 | 의미 |
|------|-----|------|
| `_pingInterval` | 10초 | PING 전송 간격 |
| `_pongTimeout` | 3초 | PONG 응답 대기 시간 |

---

### 패킷 정의

```protobuf
// Protocol.proto
message S2C_PING {
    uint64 timestamp = 1;  // 서버 시간 (옵션)
}

message C2S_PONG {
    uint64 timestamp = 1;  // 서버가 보낸 시간 그대로 반환 (옵션)
}
```

---

## 코드 상세 분석

### 1. PING 전송 (서버 → 클라이언트)

**파일:** `GameServer/Room.cpp`

```cpp
void Room::SendPing()
{
    uint64 now = ::GetTickCount64();
    vector<int32> timeoutPlayers;

    // 모든 플레이어 순회
    for (auto& [id, player] : _players)
    {
        if (auto session = player->session.lock())
        {
            uint64 lastPing = session->_lastPingTime.load();
            uint64 pongTime = session->_pongTime.load();

            // ① 이전 PING에 대한 PONG 체크 (첫 PING 제외)
            if (lastPing > 0)
            {
                // PONG 안 왔거나, 3초 초과
                if (pongTime == 0 || (pongTime - lastPing) > _pongTimeout)
                {
                    timeoutPlayers.push_back(id);
                    continue;
                }
            }

            // ② 새 PING 준비
            session->_lastPingTime.store(now);
            session->_pongTime.store(0);  // 리셋
        }
    }

    // ③ 타임아웃 처리
    for (int32 id : timeoutPlayers)
    {
        if (auto player = _players[id])
        {
            if (auto session = player->session.lock())
            {
                session->Disconnect(L"Pong timeout");
            }
        }
    }

    // ④ PING 패킷 전송 (모든 플레이어에게)
    Protocol::S2C_PING ping;
    ping.set_timestamp(now);
    SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(ping);
    Broadcast(sendBuffer);

    // ⑤ 다음 PING 예약 (10초 후)
    DoTimer(_pingInterval, &Room::SendPing);
}
```

---

### 2. PONG 수신 (클라이언트 → 서버)

**파일:** `GameServer/ServerPacketHandler.cpp:37-42`

```cpp
bool Handle_C2S_PONG(GameSessionRef& session, Protocol::C2S_PONG& pkt)
{
    uint64 expected = 0;
    session->_pongTime.compare_exchange_strong(expected, ::GetTickCount64());
    return true;
}
```
---

### 3. PING 시작 (서버 초기화)

**파일:** `GameServer/Room.cpp`

```cpp
void Room::Init(int mapId)
{
    // ... 맵 초기화 ...

    DoTimer(50, &Room::Update);
    DoTimer(_pingInterval, &Room::SendPing);  // ← 첫 PING 예약 (10초 후)
}
```

