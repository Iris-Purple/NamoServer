# Example9

# C++ Thread-Safe Queue (LockQueue)

## 개요

멀티스레드 환경에서 여러 스레드가 동시에 큐에 접근할 때 발생할 수 있는 문제를 해결하는 **스레드 안전 큐(Thread-Safe Queue)** 구현입니다.

### 일반 큐의 문제점

```cpp
*// ❌ 위험한 코드*
std::queue<int> normalQueue;

*// Thread 1*
normalQueue.push(10);

*// Thread 2 (동시 실행)*
normalQueue.push(20);  *// 데이터 손상 가능!*
```

여러 스레드가 동시에 접근하면:

- 데이터 손상
- 프로그램 크래시
- 예측 불가능한 동작

### LockQueue의 해결책

```cpp
*// ✅ 안전한 코드*
LockQueue<int> safeQueue;

*// Thread 1*
safeQueue.Push(10);  *// 자동으로 잠금 처리// Thread 2 (동시 실행)*
safeQueue.Push(20);  *// Thread 1 완료까지 대기*
```

## 🔧 코드 개선 및 설명

먼저 누락된 헤더와 namespace를 추가한 완전한 코드입니다:

```cpp
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class LockQueue
{
public:
    LockQueue() { }
    
    *// 복사 방지 (스레드 안전성 보장)*
    LockQueue(const LockQueue&) = delete;
    LockQueue& operator=(const LockQueue&) = delete;
    
    *// 데이터 추가*
    void Push(T value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(std::move(value));
        _condVar.notify_one();  *// 대기 중인 스레드 깨우기*
    }
    
    *// 데이터 꺼내기 (시도)*
    bool TryPop(T& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty())
            return false;
        
        value = std::move(_queue.front());
        _queue.pop();
        return true;
    }
    
    *// 데이터 꺼내기 (대기)*
    void WaitPop(T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _condVar.wait(lock, [this] { return !_queue.empty(); });
        
        value = std::move(_queue.front());
        _queue.pop();
    }
    
private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _condVar;
};
```

## 핵심 구성 요소 설명

### 1. **mutex (뮤텍스)**

cpp

`std::mutex _mutex;`

- 화장실 문의 자물쇠와 같은 역할
- 한 번에 한 스레드만 접근 가능하도록 보장

### 2. **lock_guard**

cpp

`std::lock_guard<std::mutex> lock(_mutex);`

- 자동으로 잠금/해제 처리 (RAII 패턴)
- 함수 종료 시 자동으로 잠금 해제

### 3. **condition_variable**

cpp

`std::condition_variable _condVar;`

- 스레드 간 신호 전달
- 효율적인 대기/깨우기 메커니즘

## 실제 사용 예제

### 예제 1: 생산자-소비자 패턴

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    LockQueue<int> taskQueue;
    
    *// 생산자 스레드*
    std::thread producer([&taskQueue]() {
        for (int i = 1; i <= 5; i++) {
            std::cout << "생산: " << i << std::endl;
            taskQueue.Push(i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    *// 소비자 스레드*
    std::thread consumer([&taskQueue]() {
        for (int i = 0; i < 5; i++) {
            int value;
            taskQueue.WaitPop(value);  *// 데이터가 올 때까지 대기*
            std::cout << "소비: " << value << std::endl;
        }
    });
    
    producer.join();
    consumer.join();
    
    return 0;
}
```