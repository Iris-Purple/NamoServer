# Example8

## Thread Local Storage란?

각 스레드가 **독립적인 복사본**을 가지는 변수 저장 방식입니다. C++11부터 `thread_local` 키워드로 선언합니다.

```cpp
`*// 일반 전역 변수 - 모든 스레드가 공유*
int32 sharedValue = 0;  *// 위험! race condition 발생 가능// TLS 변수 - 각 스레드마다 독립적인 복사본*
thread_local int32 LThreadId = 0;  *// 안전! 각자 가짐*`

```

```cpp
*// 1. TLS 변수 선언*
thread_local int32 LThreadId = 0;
```

이 한 줄이 핵심입니다! `thread_local` 키워드로 선언하면:

- 각 스레드가 **자기만의 LThreadId 변수**를 가짐
- 스레드 A의 LThreadId와 스레드 B의 LThreadId는 **완전히 별개**
- Lock 없이도 안전하게 사용 가능

```cpp
void ThreadMain(int32 threadId)
{
    *// 2. 각 스레드가 자기 ID를 저장*
    LThreadId = threadId;  *// 다른 스레드에 영향 없음!*
    
    while (true)
    {
        *// 3. 자기 스레드의 ID만 출력*
        cout << "Hi! I am Thread " << LThreadId << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}
```

**핵심 포인트**:

- Thread 1이 `LThreadId = 1`로 설정해도
- Thread 2의 `LThreadId`는 영향받지 않음
- 각자 독립적!

```cpp
int main()
{
    vector<thread> threads;
    
    *// 10개 스레드 생성*
    for (int32 i = 0; i < 10; i++)
    {
        int32 threadId = i + 1;
        threads.push_back(thread(ThreadMain, threadId));
    }
    
    for (thread& t : threads)
        t.join();
}
```

**실행 결과**:
```
Hi! I am Thread 1
Hi! I am Thread 2
Hi! I am Thread 3
...
Hi! I am Thread 10
```

각 스레드가 자기 ID를 정확하게 출력합니다!

## TLS가 없다면?

비교를 위해 일반 전역 변수를 사용하면:

```cpp
*// TLS 없이 일반 전역 변수 사용*
int32 threadId = 0;  *// 모든 스레드가 공유!*

void ThreadMain(int32 id)
{
    threadId = id;  *// 여러 스레드가 동시에 수정 - 위험!*
    
    while (true)
    {
        *// 예상: Thread 1은 1을 출력// 현실: 다른 스레드가 바꿔버려서 예측 불가능!*
        cout << "Hi! I am Thread " << threadId << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}
```

**문제점**:

- Thread 1이 `threadId = 1` 설정
- Thread 2가 즉시 `threadId = 2`로 덮어씀
- Thread 1이 출력할 때 `threadId`는 이미 2!
- **Race Condition** 발생!

## 실전 예제: 게임 서버에서 활용

### 1. 스레드별 임시 버퍼

```cpp
*// 각 스레드가 자기만의 버퍼를 가짐 (lock 불필요)*
thread_local std::vector<uint8_t> packetBuffer;

void ProcessPacket(const char* data, size_t len)
{
    *// 자기 스레드의 버퍼 사용 - 안전!*
    packetBuffer.clear();
    packetBuffer.insert(packetBuffer.end(), data, data + len);
    
    *// 패킷 처리...*
}
```

### 2. 스레드별 랜덤 생성기

```cpp
*// 각 스레드가 독립적인 난수 생성기 보유*
thread_local std::mt19937 randomEngine(std::random_device{}());

int GetRandomDamage(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(randomEngine);  *// Thread-safe!*
}
```

## TLS vs Atomic vs Mutex

```cpp
*// 1. TLS - 각자 독립적, 가장 빠름*
thread_local int counter = 0;
counter++;  *// Lock 불필요// 2. Atomic - 공유하지만 안전, 중간 성능*
std::atomic<int> counter{0};
counter++;  *// Lock-free atomic 연산// 3. Mutex - 공유하며 보호, 느림*
int counter = 0;
std::mutex mtx;
{
    std::lock_guard<std::mutex> lock(mtx);
    counter++;
}
```

## 주의사항

```cpp
thread_local int value = 0;

void WorkerThread()
{
    value = 100;
    
    *// 주의: 다른 스레드의 TLS 변수는 접근 불가능!// 각 스레드는 자기 것만 볼 수 있음*
}

int main()
{
    thread t(WorkerThread);
    t.join();
    
    *// Main 스레드의 value는 여전히 0// Worker 스레드의 value가 100이었던 것*
    cout << value << endl;  *// 출력: 0*
}
```

## 정리

**Thread Local Storage를 사용하는 이유**:

1. **안전함**: Race condition 걱정 없음
2. **빠름**: Lock이나 atomic 연산 불필요
3. **편리함**: 전역 변수처럼 어디서든 접근 가능

**사용 시나리오**:

- 스레드 식별 정보 (ID, 이름)
- 스레드별 임시 버퍼
- 스레드별 난수 생성기
- 스레드별 성능 통계