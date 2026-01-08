# GenPackets.bat 빌드 스크립트 가이드

> Protocol 정의부터 배포까지 자동화하는 빌드 스크립트
> 1. 패킷이 변경되면  GenPackts.bat 을 실행하여  배포
> 2. 게임데이터 변경시  GenPackts.bat 을 실행하여 배포
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

---

### 4단계: GameServer에 배포

```batch
:: C++ 변환된 프로토콜 게임서버에 복사
DEL /Q /F "..\GameServer\Protocol\*.*"
XCOPY /Y "Protocol\cpp_output\*.*" "..\GameServer\Protocol\"

:: 패킷핸들러 게임서버 복사
XCOPY /Y "PacketGenerator\ServerPacketHandler.h" "..\GameServer\"
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

