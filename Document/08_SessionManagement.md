# Session Management

## 개요

클라이언트 연결(세션)을 관리하는 방법을 다룹니다.

## 주요 내용

### 세션이란?

- 클라이언트와 서버 간의 연결 상태
- 연결당 하나의 세션 객체 생성
- 세션별 상태 정보 관리

### 세션 클래스 구조

```cpp
class Session
{
public:
    void Connect();
    void Disconnect();
    void Send(SendBuffer* buffer);
    void Recv();

private:
    SOCKET _socket;
    uint64 _sessionId;
    RecvBuffer _recvBuffer;
};
```

### 세션 매니저

```cpp
class SessionManager
{
public:
    Session* CreateSession();
    void ReleaseSession(Session* session);
    Session* FindSession(uint64 sessionId);
    void Broadcast(SendBuffer* buffer);

private:
    std::map<uint64, Session*> _sessions;
    std::mutex _lock;
};
```

### 세션 생명주기

1. **Connect** - 연결 수립, 세션 생성
2. **Active** - 패킷 송수신
3. **Disconnect** - 연결 종료, 리소스 정리

## 관련 예제

- [exampleServer](../Study/exampleServer.cpp) - 기본 연결 수락

## 참고 자료

- Session Pool 패턴
