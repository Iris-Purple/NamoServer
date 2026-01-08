# Protobuf 설치

> **Note:** Server/Protocol 소스 코드를 다운로드 받으면 아래 기능은 이미 완료된 상태로 git에 포함되어 있습니다.

## 1. 필수 다운로드

### Protobuf 3.17
- https://github.com/protocolbuffers/protobuf/tree/3.17.x
- `Download ZIP` 클릭

### CMake
- https://github.com/Kitware/CMake/releases/download/v3.31.10/cmake-3.31.10-windows-x86_64.msi

## 2. Protobuf 빌드 (CMake)

CMake GUI를 실행하고 아래와 같이 설정합니다.

| 항목 | 경로 |
|------|------|
| Where is the source code | `D:/work/CPP_Server/Protocol/download/protobuf-3.17.x/cmake` |
| Where to build the binaries | `D:/work/CPP_Server/Protocol/download/solution` |

### CMake 설정
- `protobuf_BUILD_PROTOC_BINARIES` 체크
- 나머지 옵션은 전부 제외
- `Generate` 클릭

## 3. Visual Studio 빌드

1. `Protocol/download/solution/protobuf.sln` 프로젝트 실행
2. `ALL_BUILD` 프로젝트 선택
3. **Debug**, **Release** 둘 다 빌드

빌드 완료 후 `solution` 디렉토리에 `Debug`, `Release` 폴더가 생성됩니다.

## 4. 파일 복사

### protoc.exe 복사
```
solution/Debug/protoc.exe → D:\work\CPP_Server\Protocol\
```
> `protoc.exe`는 `.proto` 파일을 해석하여 사용하는 언어에 맞게 코드를 생성합니다.

### 라이브러리 복사

| 빌드 타입 | 소스 | 대상 |
|-----------|------|------|
| Debug | `libprotobufd.lib`, `libprotobufd.pdb` | `D:\work\CPP_Server\Protocol\Protocol\Debug` |
| Release | `libprotobuf.lib` | `D:\work\CPP_Server\Protocol\Protocol\Release` |

### Google 헤더 복사
```
D:\work\CPP_Server\Protocol\download\protobuf-3.17.x\src\google
    → D:\work\CPP_Server\Protocol\Protocol\
```

## 5. 패킷 핸들러 자동 생성

`Protocol/PacketGenerator` 디렉토리에는 패킷 핸들러를 자동으로 생성하는 Python 코드가 있습니다.

### 필수 설치
- Python 3.13.x
- jinja2 라이브러리
  ```bash
  pip install jinja2
  ```

### 생성 방식
`Templates/PacketHandler.h` 템플릿과 `Protocol.proto`를 기반으로 서버/클라이언트 프로토콜 헤더를 생성합니다.

### 실행
```bash
GenPackets.bat
```

### 생성되는 파일
| 파일 | 설명 |
|------|------|
| `GameServer/Protocol/Protobuf/*.cpp, *.h` | Protobuf C++ 파일 |
| `GameServer/ServerPacketHandler.h` | 서버 패킷 핸들러 |
| `GameServer/DummyClient/ClientPacketHandler.h` | 클라이언트 패킷 핸들러 |
