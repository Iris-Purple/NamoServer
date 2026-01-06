# IOCP (I/O Completion Port)

## 개요

Windows 환경에서 고성능 비동기 I/O를 처리하기 위한 기술입니다.

## 주요 내용

### IOCP란?

- Windows에서 제공하는 비동기 I/O 모델
- 스레드 풀을 효율적으로 관리
- 수천 개의 동시 연결 처리 가능

### 동작 방식

1. Completion Port 생성
2. 소켓을 Completion Port에 연결
3. 비동기 I/O 요청 (WSARecv, WSASend)
4. Worker Thread에서 완료 통지 수신
5. 완료된 작업 처리

### 핵심 API

```cpp
// Completion Port 생성
CreateIoCompletionPort();

// 완료 통지 대기
GetQueuedCompletionStatus();

// 비동기 수신
WSARecv();

// 비동기 송신
WSASend();
```

## 관련 예제

- Study 디렉토리 예제 참조

## 참고 자료

- MSDN IOCP Documentation
