# 서버 환경 설정 (config.json) 가이드

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

| 항목 | 값 |
|------|-----|
| **타입** | string |
| **필수 여부** | ✅ 필수 |
| **기본값** | 없음 |
| **예시** | `"../Common/Data/"` |

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

| 항목 | 값 |
|------|-----|
| **타입** | boolean |
| **필수 여부** | ⚪ 선택 |
| **기본값** | `false` |
| **예시** | `true` 또는 `false` |

**설명:**
클라이언트-서버 간 패킷 암호화 활성화 여부입니다.

```
encryptionEnabled: false
┌──────────────┐                    ┌──────────────┐
│   클라이언트  │ ── 평문 패킷 ──▶  │    서버      │
└──────────────┘                    └──────────────┘
⚠️ 개발/테스트 환경에서 사용

encryptionEnabled: true
┌──────────────┐                    ┌──────────────┐
│   클라이언트  │ ── AES 암호화 ──▶ │    서버      │
└──────────────┘                    └──────────────┘
✅ 운영 환경에서 사용
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

| 항목 | 값 |
|------|-----|
| **타입** | string |
| **필수 여부** | ⚪ 선택 (암호화 사용 시 권장) |
| **기본값** | 내장 기본 키 |
| **길이** | **정확히 16바이트** (AES-128) |
| **예시** | `"NamoServerKey123"` |

**설명:**
AES-128 암호화에 사용되는 비밀 키입니다.

```
⚠️ 주의사항:
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

**올바른 키 예시:**
```
"NamoServerKey123"  ← 16글자 ✅
"MySecretKey12345"  ← 16글자 ✅
"ShortKey"          ← 8글자 ❌ (기본 키 사용됨)
"TooLongKeyValue123456" ← 20글자 ❌ (기본 키 사용됨)
```

---

### 4. roomCount

| 항목 | 값 |
|------|-----|
| **타입** | integer |
| **필수 여부** | ⚪ 선택 |
| **기본값** | 0 |
| **예시** | `5` |

**설명:**
서버 시작 시 생성할 게임 방(Room)의 개수입니다.

```
roomCount: 5 설정 시:

┌─────────────────────────────────────────────────┐
│                   GameServer                     │
│  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
│  │Room 0 │ │Room 1 │ │Room 2 │ │Room 3 │ │Room 4 │
│  └───────┘ └───────┘ └───────┘ └───────┘ └───────┘
└─────────────────────────────────────────────────┘
```

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

| 항목 | 값 |
|------|-----|
| **타입** | integer |
| **필수 여부** | ⚪ 선택 |
| **기본값** | 0 |
| **예시** | `3` |

**설명:**
각 방(Room)에 생성할 몬스터의 개수입니다.

```
monsterCount: 3 설정 시:

Room 0                Room 1                Room 2
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│ 🐉 Monster1 │      │ 🐉 Monster1 │      │ 🐉 Monster1 │
│ 🐉 Monster2 │      │ 🐉 Monster2 │      │ 🐉 Monster2 │
│ 🐉 Monster3 │      │ 🐉 Monster3 │      │ 🐉 Monster3 │
└─────────────┘      └─────────────┘      └─────────────┘
```

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

---

## 설정 로드 흐름

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     서버 시작 시 설정 로드 과정                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. GameServer.cpp main() 시작                                          │
│           │                                                              │
│           ▼                                                              │
│  ┌─────────────────────────────────────┐                                │
│  │ ConfigManager::Instance().LoadConfig() │                              │
│  └─────────────────────────────────────┘                                │
│           │                                                              │
│           ▼                                                              │
│  ┌─────────────────────────────────────┐                                │
│  │ config.json 파일 열기               │                                │
│  │ (없으면 에러 출력 후 기본값 사용)   │                                │
│  └─────────────────────────────────────┘                                │
│           │                                                              │
│           ▼                                                              │
│  ┌─────────────────────────────────────┐                                │
│  │ JSON 파싱 → ServerConfig 구조체    │                                │
│  └─────────────────────────────────────┘                                │
│           │                                                              │
│           ▼                                                              │
│  ┌─────────────────────────────────────┐                                │
│  │ 전역 변수 설정                       │                                │
│  │ • GEncryptionEnabled = true/false  │                                │
│  │ • GEncryptionKey = "..."           │                                │
│  └─────────────────────────────────────┘                                │
│           │                                                              │
│           ▼                                                              │
│  ┌─────────────────────────────────────┐                                │
│  │ 콘솔에 설정 내용 출력               │                                │
│  └─────────────────────────────────────┘                                │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

**콘솔 출력 예시:**
```
[ConfigManager] Config loaded
  - dataPath: ../Common/Data/
  - encryptionEnabled: true
  - encryptionKey: (loaded from config)
  - roomCount: 5
  - monsterCount: 3
```

---

## 환경별 설정 예시

### 개발 환경 (Development)

```json
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": false,
    "roomCount": 1,
    "monsterCount": 1
}
```

- 암호화 비활성화 → 패킷 디버깅 용이
- 방/몬스터 최소화 → 빠른 테스트

---

### 테스트 환경 (QA/Staging)

```json
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": true,
    "encryptionKey": "TestServerKey123",
    "roomCount": 3,
    "monsterCount": 5
}
```

- 암호화 활성화 → 운영 환경과 동일하게 테스트
- 적당한 방/몬스터 수

---

### 운영 환경 (Production)

```json
{
    "dataPath": "/opt/gameserver/data/",
    "encryptionEnabled": true,
    "encryptionKey": "Pr0dK3y!@#$5678",
    "roomCount": 10,
    "monsterCount": 10
}
```

- ⚠️ 암호화 키는 반드시 변경!
- 절대 경로 사용 권장
- 방/몬스터 수 증가

---

## 코드 구조

### ConfigManager 싱글톤

**파일:** `GameServer/ConfigManager.h`

```cpp
class ConfigManager
{
public:
    // 싱글톤 인스턴스
    static ConfigManager& Instance()
    {
        static ConfigManager instance;
        return instance;
    }

    // 설정 로드
    void LoadConfig(const string& path = "config.json");

    // Getter 함수들
    const string& GetDataPath() const;
    bool GetEncryptionEnabled() const;
    const string& GetEncryptionKey() const;
    const int& GetRoomCount() const;
    const int& GetMonsterCount() const;

private:
    ServerConfig _config;
};
```

### 사용 예시

```cpp
// 서버 시작 시
ConfigManager::Instance().LoadConfig();

// 설정 값 사용
string dataPath = ConfigManager::Instance().GetDataPath();
bool useEncryption = ConfigManager::Instance().GetEncryptionEnabled();
int rooms = ConfigManager::Instance().GetRoomCount();
```

---

## 주의사항

### 1. 파일 위치

```
✅ 올바른 위치:
GameServer/
├── GameServer.exe
└── config.json        ← 실행 파일과 같은 폴더

❌ 잘못된 위치:
GameServer/
├── GameServer.exe
└── Config/
    └── config.json    ← 찾을 수 없음!
```

### 2. JSON 문법

```json
// ❌ 잘못된 예시 (마지막 쉼표)
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": true,  ← 마지막 항목 뒤에 쉼표 금지!
}

// ✅ 올바른 예시
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": true
}
```

### 3. 암호화 키 보안

```
⚠️ 중요: config.json을 Git에 커밋하지 마세요!

.gitignore에 추가:
GameServer/config.json

대신 예시 파일 제공:
GameServer/config.example.json
```

---

## 새 설정 항목 추가하기

### 1단계: ServerConfig 구조체 수정

```cpp
// ConfigManager.h
struct ServerConfig
{
    string dataPath;
    bool encryptionEnabled = false;
    string encryptionKey;
    int roomCount;
    int monsterCount;
    int maxPlayers = 100;  // ← 새 항목 추가
};
```

### 2단계: from_json 함수 수정

```cpp
// ConfigManager.h
inline void from_json(const json& j, ServerConfig& config)
{
    // ... 기존 코드 ...

    // 새 항목 파싱 (선택적)
    if (j.contains("maxPlayers"))
        j.at("maxPlayers").get_to(config.maxPlayers);
}
```

### 3단계: Getter 함수 추가

```cpp
// ConfigManager.h
class ConfigManager
{
public:
    const int& GetMaxPlayers() const { return _config.maxPlayers; }
};
```

### 4단계: config.json 업데이트

```json
{
    "dataPath": "../Common/Data/",
    "encryptionEnabled": true,
    "encryptionKey": "NamoServerKey123",
    "roomCount": 5,
    "monsterCount": 3,
    "maxPlayers": 100
}
```

---

## 요약

| 설정 항목 | 타입 | 필수 | 설명 |
|----------|------|------|------|
| `dataPath` | string | ✅ | 게임 데이터 파일 경로 |
| `encryptionEnabled` | bool | ⚪ | 패킷 암호화 활성화 |
| `encryptionKey` | string | ⚪ | AES-128 키 (16바이트) |
| `roomCount` | int | ⚪ | 생성할 방 개수 |
| `monsterCount` | int | ⚪ | 방당 몬스터 개수 |

---

> 문서 작성일: 2026-01-08
> 대상 프로젝트: NamoServer
