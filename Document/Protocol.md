## 패킷 구조와 프로토콜

### 1 PacketHeader 구조

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

### 2 패킷 ID 체계

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

### 3 Protobuf 메시지 정의

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

### 4 패킷 네이밍 규칙

- `C2S_` : Client to Server (클라이언트 → 서버 요청)
- `S2C_` : Server to Client (서버 → 클라이언트 응답/브로드캐스트)