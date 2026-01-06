# Example5

RAII 를 사용하여  자원 관리도 필요하지만   데드락 상황은  발생할 수 있습니다

C++에서 락(lock)의 **데드락(Deadlock)**은 주로 여러 스레드가 
**서로 다른 뮤텍스(mutex)를 상호 배타적으로 점유하고, 상대방이 점유한 뮤텍스를 얻기 위해 무한정 대기**할 때 발생합니다. 

**데드락이 발생하는 시나리오 (Locking Order Issue)**

가장 흔한 데드락 시나리오는 두 개의 스레드가 두 개의 뮤텍스를 서로 반대 순서로 잠그려고 할 때 발생합니다.

예를 들어, `ResourceA`와 `ResourceB`라는 두 개의 자원이 있고 각각 `mutexA`, `mutexB`로 보호된다고 가정해 보겠습니다.

- **스레드 1:** `mutexA`를 잠근 후, `mutexB`를 잠그려고 시도합니다.
- **스레드 2:** `mutexB`를 잠근 후, `mutexA`를 잠그려고 시도합니다.

만약 두 스레드가 거의 동시에 실행되어 스레드 1은 `mutexA`를 성공적으로 잠그고, 스레드 2는 `mutexB`를 성공적으로 잠근 상태에서 서로 상대방의 뮤텍스를 기다리게 되면 데드락이 발생합니다.

Example5   코드 참고

```cpp
void A::CallB(B& b) {
    std::lock_guard<std::mutex> lock1(m);          // A 잠금
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock2(b.m);        // B 잠금 시도
    std::cout << "A called B\n";
}

void B::CallA(A& a) {
    std::lock_guard<std::mutex> lock1(m);      // B 잠금
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lock2(a.m);    // A 잠금 시도
    std::cout << "B called A\n";
}

```