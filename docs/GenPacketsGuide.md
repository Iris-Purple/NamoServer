# GenPackets.bat 빌드 스크립트 가이드

> Protocol 정의부터 배포까지 자동화하는 빌드 스크립트

---

## 전체 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     GenPackets.bat 실행 흐름                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌─────────────────┐                                                   │
│   │  .proto 파일들   │  Protocol 정의 (원본)                            │
│   │  Enum.proto     │                                                   │
│   │  Struct.proto   │                                                   │
│   │  Protocol.proto │                                                   │
│   └────────┬────────┘                                                   │
│            │                                                             │
│            ▼                                                             │
│   ┌─────────────────┐                                                   │
│   │   protoc.exe    │  Protocol Buffer 컴파일러                         │
│   └────────┬────────┘                                                   │
│            │                                                             │
│      ┌─────┴─────┐                                                      │
│      ▼           ▼                                                      │
│  ┌───────┐  ┌───────┐                                                  │
│  │ C++   │  │ C#    │  생성된 코드                                      │
│  │ .cc   │  │ .cs   │                                                   │
│  │ .h    │  │       │                                                   │
│  └───┬───┘  └───┬───┘                                                  │
│      │          │                                                        │
│      ▼          ▼                                                        │
│  ┌────────────────────────────────────────────────────────────────┐    │
│  │                        배포 (XCOPY)                             │    │
│  │  • GameServer/Protocol/     ← C++ 코드                         │    │
│  │  • DummyClient/Protocol/    ← C++ 코드                         │    │
│  │  • Client/Assets/.../Packet/ ← C# 코드                         │    │
│  └────────────────────────────────────────────────────────────────┘    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 디렉토리 구조

```
Protocol/
├── GenPackets.bat              ← 이 스크립트
├── protoc.exe                  ← Protobuf 컴파일러
│
├── Protocol/                   ← .proto 정의 파일들
│   ├── Enum.proto             # 열거형 정의
│   ├── Struct.proto           # 공통 구조체 정의
│   ├── Protocol.proto         # 패킷 메시지 정의
│   │
│   ├── cpp_output/            # C++ 생성 코드 (자동 생성)
│   │   ├── Enum.pb.cc
│   │   ├── Enum.pb.h
│   │   ├── Struct.pb.cc
│   │   ├── Struct.pb.h
│   │   ├── Protocol.pb.cc
│   │   └── Protocol.pb.h
│   │
│   ├── csharp_output/         # C# 생성 코드 (자동 생성)
│   │   ├── Enum.cs
│   │   ├── Struct.cs
│   │   └── Protocol.cs
│   │
│   ├── google/                # Protobuf 소스 코드 (Include용)
│   │   └── protobuf/
│   │       └── *.h, *.cc
│   │
│   └── Lib/                   # Protobuf 라이브러리
│       ├── Debug/
│       │   ├── libprotobufd.lib
│       │   └── libprotobufd.pdb
│       └── Release/
│           └── libprotobuf.lib
│
├── PacketGenerator/            ← PacketHandler 자동 생성
│   ├── PacketGenerator.py
│   ├── ProtoParser.py
│   └── Templates/
│       └── PacketHandler.h
│
└── Data/                       ← 게임 데이터 파일
    ├── StatData.json
    └── SkillData.json
```

---

## 스크립트 단계별 분석

### 1단계: 기존 생성 파일 삭제

```batch
:: c++ 출력용 *.cc, *.h 파일 전부 삭제
DEL /Q /F "Protocol\cpp_output\*.*"
```

**목적**: 이전 빌드 결과물 정리 (깨끗한 빌드 보장)

---

### 2단계: Protobuf 컴파일

```batch
:: C++ 코드 생성
protoc.exe -I=./Protocol --cpp_out=./Protocol/cpp_output ./Protocol/*.proto

:: C# 코드 생성
protoc.exe -I=./Protocol --csharp_out=./Protocol/csharp_output ./Protocol/*.proto
```

**동작 원리:**

```
입력: Protocol/*.proto (Enum.proto, Struct.proto, Protocol.proto)
         │
         ▼
    ┌─────────────┐
    │ protoc.exe  │
    └─────────────┘
         │
    ┌────┴────┐
    ▼         ▼
C++ 출력   C# 출력
*.pb.h     *.cs
*.pb.cc
```

**생성되는 파일 예시:**

| 입력 | C++ 출력 | C# 출력 |
|------|----------|---------|
| `Enum.proto` | `Enum.pb.h`, `Enum.pb.cc` | `Enum.cs` |
| `Struct.proto` | `Struct.pb.h`, `Struct.pb.cc` | `Struct.cs` |
| `Protocol.proto` | `Protocol.pb.h`, `Protocol.pb.cc` | `Protocol.cs` |

---

### 3단계: PacketHandler 자동 생성

```batch
PUSHD PacketGenerator
    :: 클라이언트용 핸들러 생성 (S2C 수신, C2S 송신)
    python PacketGenerator.py --path=../Protocol/Protocol.proto --output=ClientPacketHandler --recv=S2C_ --send=C2S_

    :: 서버용 핸들러 생성 (C2S 수신, S2C 송신)
    python PacketGenerator.py --path=../Protocol/Protocol.proto --output=ServerPacketHandler --recv=C2S_ --send=S2C_
POPD
```

**PacketGenerator의 역할:**

```
Protocol.proto 분석
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│  메시지 분류                                               │
│  ┌─────────────────┐    ┌─────────────────┐              │
│  │ C2S_ 접두사     │    │ S2C_ 접두사     │              │
│  │ (클라이언트→서버)│    │ (서버→클라이언트)│              │
│  │ C2S_ENTER_GAME  │    │ S2C_ENTER_GAME  │              │
│  │ C2S_MOVE        │    │ S2C_MOVE        │              │
│  │ C2S_SKILL       │    │ S2C_SPAWN       │              │
│  └─────────────────┘    └─────────────────┘              │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│  ServerPacketHandler.h 생성                                │
│  • C2S_ 패킷들의 Handle 함수 선언                         │
│  • S2C_ 패킷들의 MakeSendBuffer 함수 생성                 │
└───────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────┐
│  ClientPacketHandler.h 생성                                │
│  • S2C_ 패킷들의 Handle 함수 선언                         │
│  • C2S_ 패킷들의 MakeSendBuffer 함수 생성                 │
└───────────────────────────────────────────────────────────┘
```

**생성된 코드 예시:**

```cpp
// ServerPacketHandler.h (자동 생성)
class ServerPacketHandler
{
public:
    // 수신 핸들러 등록
    static void Init()
    {
        GPacketHandler[PKT_C2S_ENTER_GAME] = ...;
        GPacketHandler[PKT_C2S_MOVE] = ...;
        GPacketHandler[PKT_C2S_SKILL] = ...;
    }

    // 송신 버퍼 생성
    static SendBufferRef MakeSendBuffer(Protocol::S2C_ENTER_GAME& pkt);
    static SendBufferRef MakeSendBuffer(Protocol::S2C_MOVE& pkt);
    static SendBufferRef MakeSendBuffer(Protocol::S2C_SPAWN& pkt);
};
```

---

### 4단계: GameServer에 배포

```batch
:: C++ 변환된 프로토콜 게임서버에 복사
DEL /Q /F "..\GameServer\Protocol\*.*"
XCOPY /Y "Protocol\cpp_output\*.*" "..\GameServer\Protocol\"

:: 패킷핸들러 게임서버 복사
XCOPY /Y "PacketGenerator\ServerPacketHandler.h" "..\GameServer\"
```

**배포 결과:**

```
GameServer/
├── Protocol/
│   ├── Enum.pb.h          ← 복사됨
│   ├── Enum.pb.cc         ← 복사됨
│   ├── Struct.pb.h        ← 복사됨
│   ├── Struct.pb.cc       ← 복사됨
│   ├── Protocol.pb.h      ← 복사됨
│   └── Protocol.pb.cc     ← 복사됨
└── ServerPacketHandler.h  ← 복사됨
```

---

### 5단계: DummyClient에 배포

```batch
:: C++ 변환된 프로토콜 더미클라이언트에 복사
DEL /Q /F "..\DummyClient\Protocol\*.*"
XCOPY /Y "Protocol\cpp_output\*.*" "..\DummyClient\Protocol\"

:: 패킷핸들러 더미클라이언트 복사
XCOPY /Y "PacketGenerator\ClientPacketHandler.h" "..\DummyClient\"
```

**배포 결과:**

```
DummyClient/
├── Protocol/
│   ├── Enum.pb.h          ← 복사됨
│   ├── Enum.pb.cc         ← 복사됨
│   ├── Struct.pb.h        ← 복사됨
│   ├── Struct.pb.cc       ← 복사됨
│   ├── Protocol.pb.h      ← 복사됨
│   └── Protocol.pb.cc     ← 복사됨
└── ClientPacketHandler.h  ← 복사됨
```

---

### 6단계: Unity 클라이언트에 배포 (C#)

```batch
:: C# 변환된 프로토콜 클라이언트에 복사
XCOPY /Y "Protocol\csharp_output\*.*" "..\..\Client\Assets\Scripts\Packet"
```

**배포 결과:**

```
Client/Assets/Scripts/Packet/
├── Enum.cs       ← 복사됨
├── Struct.cs     ← 복사됨
└── Protocol.cs   ← 복사됨
```

---

### 7단계: Protobuf 라이브러리 배포 (최초 1회)

```batch
:: protobuf 빌드에 필요한 소스코드 복사 (없을 때만)
IF NOT EXIST "..\Libraries\Include\google" XCOPY /Y /S /E /I "Protocol\google" "..\Libraries\Include\google"

:: 라이브러리 Debug 복사 (없을 때만)
IF NOT EXIST "..\Libraries\Libs\Protobuf\Debug\libprotobufd.lib" COPY /Y "Protocol\Lib\Debug\libprotobufd.lib" "..\Libraries\Libs\Protobuf\Debug\libprotobufd.lib"
IF NOT EXIST "..\Libraries\Libs\Protobuf\Debug\libprotobufd.pdb" COPY /Y "Protocol\Lib\Debug\libprotobufd.pdb" "..\Libraries\Libs\Protobuf\Debug\libprotobufd.pdb"

:: 라이브러리 Release 복사 (없을 때만)
IF NOT EXIST "..\Libraries\Libs\Protobuf\Release\libprotobuf.lib" COPY /Y "Protocol\Lib\Release\libprotobuf.lib" "..\Libraries\Libs\Protobuf\Release\libprotobuf.lib"
```

**배포 결과 (최초 실행 시):**

```
Libraries/
├── Include/
│   └── google/
│       └── protobuf/
│           └── *.h (헤더 파일들)
│
└── Libs/
    └── Protobuf/
        ├── Debug/
        │   ├── libprotobufd.lib   ← 디버그용 라이브러리
        │   └── libprotobufd.pdb   ← 디버그 심볼
        └── Release/
            └── libprotobuf.lib    ← 릴리즈용 라이브러리
```

**IF NOT EXIST 조건:**
- 이미 파일이 있으면 복사하지 않음
- 최초 환경 설정 시에만 동작
- 라이브러리 버전 실수로 덮어쓰기 방지

---

### 8단계: 게임 데이터 배포

```batch
:: StatData.json, SkillData.json 등 복사
XCOPY /Y "Data\*.*" "..\Common\Data\"
XCOPY /Y "Data\*.*" "..\..\Client\Assets\Resources\Data\"
```

**배포 결과:**

```
Common/Data/
├── StatData.json    ← 서버용
└── SkillData.json   ← 서버용

Client/Assets/Resources/Data/
├── StatData.json    ← 클라이언트용
└── SkillData.json   ← 클라이언트용
```

**데이터 파일 예시 (StatData.json):**

```json
{
  "stats": [
    {
      "level": 1,
      "maxHp": 100,
      "attack": 10,
      "speed": 5.0,
      "totalExp": 0
    },
    {
      "level": 2,
      "maxHp": 150,
      "attack": 15,
      "speed": 5.5,
      "totalExp": 100
    }
  ]
}
```

---

## 전체 배포 맵

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     GenPackets.bat 배포 결과                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Protocol/                        배포 대상                              │
│  ├── Protocol/*.proto ─────────────────────────────────────────────┐   │
│  │                                                                   │   │
│  │   ┌─── protoc.exe ───┐                                          │   │
│  │   │                   │                                          │   │
│  │   ▼                   ▼                                          │   │
│  │  C++ 코드           C# 코드                                      │   │
│  │   │                   │                                          │   │
│  │   ├──────────────────────────────▶ GameServer/Protocol/         │   │
│  │   ├──────────────────────────────▶ DummyClient/Protocol/        │   │
│  │   └──────────────────────────────▶ Client/Assets/.../Packet/    │   │
│  │                                                                   │   │
│  ├── PacketGenerator/ ──────────────────────────────────────────┐  │   │
│  │   │                                                           │  │   │
│  │   ├── ServerPacketHandler.h ─────▶ GameServer/               │  │   │
│  │   └── ClientPacketHandler.h ─────▶ DummyClient/              │  │   │
│  │                                                                   │   │
│  ├── Protocol/google/ ──────────────▶ Libraries/Include/google/    │   │
│  │                                     (최초 1회)                   │   │
│  │                                                                   │   │
│  ├── Protocol/Lib/ ─────────────────▶ Libraries/Libs/Protobuf/     │   │
│  │                                     (최초 1회)                   │   │
│  │                                                                   │   │
│  └── Data/*.json ───────────────────▶ Common/Data/                 │   │
│                      └──────────────▶ Client/Assets/.../Data/      │   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 사용 방법

### 언제 실행해야 하나요?

| 상황 | GenPackets.bat 실행 필요 |
|------|-------------------------|
| `.proto` 파일 수정 | ✅ 필수 |
| 새 패킷 추가 | ✅ 필수 |
| `StatData.json` 수정 | ✅ 필수 |
| C++ 게임 로직만 수정 | ❌ 불필요 |
| 처음 프로젝트 클론 | ✅ 필수 |

### 실행 방법

```
1. Protocol 폴더로 이동
2. GenPackets.bat 더블클릭 (또는 명령 프롬프트에서 실행)
3. "계속하려면 아무 키나 누르십시오..." 메시지까지 대기
4. 에러 없으면 완료!
```

### 실행 결과 확인

```batch
:: 성공 시 출력 예시
Protocol\cpp_output\Enum.pb.cc
Protocol\cpp_output\Enum.pb.h
Protocol\cpp_output\Struct.pb.cc
...
1개 파일이 복사되었습니다.
1개 파일이 복사되었습니다.
...
```

---

## 문제 해결

### 1. protoc.exe 실행 에러

```
'protoc.exe'은(는) 내부 또는 외부 명령... 이 아닙니다.
```

**해결**: `Protocol/protoc.exe` 파일 존재 확인

### 2. Python 에러

```
'python'은(는) 내부 또는 외부 명령... 이 아닙니다.
```

**해결**: Python 설치 및 환경변수 PATH 추가

### 3. 파일 복사 실패

```
액세스가 거부되었습니다.
```

**해결**: Visual Studio에서 해당 프로젝트 닫기 (파일 잠금 해제)

---

## 요약

| 단계 | 작업 | 결과물 |
|------|------|--------|
| 1 | 기존 파일 삭제 | 깨끗한 빌드 환경 |
| 2 | protoc.exe 실행 | C++, C# 코드 생성 |
| 3 | PacketGenerator 실행 | Handler 코드 생성 |
| 4 | GameServer 배포 | 서버 빌드 준비 완료 |
| 5 | DummyClient 배포 | 테스트 클라이언트 준비 |
| 6 | Unity Client 배포 | 게임 클라이언트 준비 |
| 7 | 라이브러리 배포 | 빌드 환경 설정 (최초) |
| 8 | 데이터 파일 배포 | 게임 데이터 동기화 |

**핵심**: `.proto` 파일 수정 후에는 반드시 `GenPackets.bat` 실행!

---

> 문서 작성일: 2026-01-08
> 대상 프로젝트: NamoServer
