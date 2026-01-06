# Network

## 개요

게임 서버의 네트워크 기초 개념을 다룹니다.

## 주요 내용

### TCP vs UDP

| 구분 | TCP | UDP |
|------|-----|-----|
| 연결 | 연결 지향 | 비연결 |
| 신뢰성 | 보장 | 미보장 |
| 순서 | 보장 | 미보장 |
| 속도 | 상대적 느림 | 빠름 |
| 용도 | 게임 로직, 채팅 | 실시간 위치 동기화 |

### 소켓 프로그래밍 기초

```cpp
// 소켓 생성
SOCKET socket = SocketUtils::CreateSocket();

// 바인딩
SocketUtils::BindAnyAddress(socket, 7777);

// 리슨
SocketUtils::Listen(socket);

// 연결 수락
SOCKET clientSocket = ::accept(socket, nullptr, nullptr);
```

## 관련 예제

- [exampleServer](../Study/exampleServer.cpp) - 기본 서버 예제

## 참고 자료

- Windows Socket API (Winsock2)
