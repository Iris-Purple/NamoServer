# DeadLock

## 개요

데드락의 원인과 탐지/예방 방법을 다룹니다.

## 주요 내용

### 데드락이란?

두 개 이상의 스레드가 서로가 가진 자원을 기다리며 무한 대기하는 상태

### 데드락 발생 조건 (4가지 모두 충족 시)

1. **상호 배제** - 자원은 한 번에 하나의 스레드만 사용
2. **점유 대기** - 자원을 가진 채 다른 자원 대기
3. **비선점** - 다른 스레드의 자원을 빼앗을 수 없음
4. **순환 대기** - 스레드들이 원형으로 자원 대기

### 예방 방법

- Lock 순서 고정
- 타임아웃 설정
- 데드락 탐지 알고리즘 사용

### DeadLock Profiler

```cpp
class DeadLockProfiler
{
public:
    void PushLock(const char* name);
    void PopLock(const char* name);
    void CheckCycle();
};
```

## 관련 예제

- [DeadLockProfiler](../Study/DeadLockProfiler.cpp)

## 참고 자료

- 그래프 사이클 탐지 알고리즘
