# 서버 환경 설정 (config.json)

> 게임 서버의 동작을 제어하는 설정 파일

---

## 파일 위치

```
GameServer/
├── GameServer.exe     ← 실행 파일
├── config.json        ← 설정 파일 (같은 폴더에 위치해야 함)
└── ...
```

---

## 설정 파일 구조

**파일:** `GameServer/config.json`

```json
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": true,
    "encryptionKey": "NamoServerKey123",
    "roomCount": 5,
    "monsterCount": 3
}
```

---

## 설정 항목 상세

### 1. dataPath

**설명:**
게임 데이터 파일(StatData.json, SkillData.json 등)이 위치한 경로입니다.

```
GameServer.exe 실행 위치 기준 상대 경로:
GameServer/
    └── (실행) → ../Common/Data/ 참조
             ↓
Common/
    └── Data/
        ├── StatData.json
        └── SkillData.json
```

**사용처:**
```cpp
// GameServer.cpp
const string& configPath = ConfigManager::Instance().GetDataPath();
DataManager::Instance().Init(configPath);  // 데이터 로드
```

---

### 2. encryptionEnabled

**설명:**
클라이언트-서버 간 패킷 암호화 활성화 여부입니다.

```
encryptionEnabled: false
┌──────────────┐                    ┌──────────────┐
│   클라이언트  │ ── 평문 패킷 ──▶  │    서버      │
└──────────────┘                    └──────────────┘
⚠ 개발/테스트 환경에서 사용

encryptionEnabled: true
┌──────────────┐                    ┌──────────────┐
│   클라이언트  │ ── AES 암호화 ──▶ │    서버      │
└──────────────┘                    └──────────────┘
운영 환경에서 사용
```

**사용처:**
```cpp
// ConfigManager.cpp
GEncryptionEnabled = _config.encryptionEnabled;  // 전역 변수에 설정

// Session.cpp - 패킷 송수신 시
if (GEncryptionEnabled && _crypto)
{
    sendBuffer = EncryptBuffer(sendBuffer);  // 암호화 적용
}
```

---

### 3. encryptionKey

**설명:**
AES-128 암호화에 사용되는 비밀 키입니다.

```
주의사항:
• 정확히 16글자(16바이트)여야 합니다
• 서버와 클라이언트가 동일한 키를 사용해야 합니다
• 운영 환경에서는 반드시 기본값을 변경하세요!
```

**키 길이 검증:**
```cpp
// ConfigManager.cpp
if (!_config.encryptionKey.empty() && _config.encryptionKey.length() == 16)
{
    memcpy(GEncryptionKey, _config.encryptionKey.c_str(), 16);  // 적용
}
else if (!_config.encryptionKey.empty())
{
    // WARNING: 16바이트가 아니면 기본 키 사용
    cerr << "encryptionKey must be exactly 16 bytes!" << endl;
}
```


---

### 4. roomCount

**설명:**
서버 시작 시 생성할 게임 방(Room)의 개수입니다.

**사용처:**
```cpp
// GameServer.cpp
const int roomCount = ConfigManager::Instance().GetRoomCount();
for (int i = 0; i < roomCount; i++)
{
    RoomManager::Instance().Add(i);  // 방 생성
}
```

---

### 5. monsterCount

**설명:**
각 방(Room)에 생성할 몬스터의 개수입니다.

**사용처:**
```cpp
// Room.cpp
Room::Room(int32 roomId) : _roomId(roomId)
{
    _monsterCount = ConfigManager::Instance().GetMonsterCount();
}

void Room::Init(int mapId)
{
    for (int i = 1; i <= _monsterCount; i++)
    {
        MonsterRef monster = ObjectManager::Instance().Add<Monster>();
        monster->SetCellPos(Vector2Int(i, i));
        HandleEnterGame(monster);
    }
}
```

