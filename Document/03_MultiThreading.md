# MultiThreading

## 개요

게임 서버에서 멀티스레딩을 활용하는 방법을 다룹니다.

## 주요 내용

### 왜 멀티스레딩인가?

- 다중 클라이언트 동시 처리
- CPU 코어 활용 극대화
- 응답성 향상

### 스레드 생성

```cpp
#include <thread>

void WorkerThread()
{
    // 작업 처리
}

int main()
{
    std::thread t(WorkerThread);
    t.join();
}
```

### 주의사항

- Race Condition (경쟁 상태)
- Dead Lock (교착 상태)
- 공유 자원 접근 시 동기화 필요

## 관련 예제

- Study 디렉토리 예제 참조

## 다음 단계

- [Lock & 동기화](04_Lock.md)
