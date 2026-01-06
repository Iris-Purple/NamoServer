# Lock & 동기화

## 개요

멀티스레드 환경에서 공유 자원을 안전하게 접근하는 방법을 다룹니다.

## 주요 내용

### Lock의 종류

| 종류 | 특징 | 용도 |
|------|------|------|
| Mutex | 커널 레벨, 무거움 | 프로세스 간 동기화 |
| Critical Section | 유저 레벨, 가벼움 | 프로세스 내 동기화 |
| SpinLock | 바쁜 대기 | 짧은 임계 영역 |
| RWLock | 읽기/쓰기 분리 | 읽기 많은 상황 |

### SpinLock 구현

```cpp
class SpinLock
{
public:
    void lock()
    {
        while (_locked.exchange(true))
        {
            // 스핀 대기
        }
    }

    void unlock()
    {
        _locked.store(false);
    }

private:
    std::atomic<bool> _locked = false;
};
```

## 관련 예제

- [spinLock](../Study/spinLock.cpp)
- [example10Lock](../Study/example10Lock.cpp)

## 참고 자료

- C++ std::mutex, std::lock_guard
