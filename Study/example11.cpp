#include "pch.h"
#include "example11.h"
#include "DeadLockProfiler.h"
#include "ThreadManager.h"

// DeadLockProfiler 인스턴스
static DeadLockProfiler g_deadlockProfiler;

// 테스트용 뮤텍스들
static std::mutex mutexA;
static std::mutex mutexB;
static std::mutex mutexC;

// RAII 방식의 Lock Wrapper 클래스
class LockGuard {
public:
    LockGuard(std::mutex& m, const char* name)
        : _mutex(m), _name(name) {
        g_deadlockProfiler.PushLock(_name);
        _mutex.lock();
    }

    ~LockGuard() {
        _mutex.unlock();
        g_deadlockProfiler.PopLock(_name);
    }

private:
    std::mutex& _mutex;
    const char* _name;
};

// 데드락 시나리오 1: A->B, B->C, C->A (3개 락의 순환)
void Thread1_Scenario1() {
    for (int i = 0; i < 1; ++i) {
        {
            LockGuard lock1(mutexA, "MutexA");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            LockGuard lock2(mutexB, "MutexB");
            std::cout << "Thread1: Got A->B" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
void Thread2_Scenario1() {
    for (int i = 0; i < 1; ++i) {
        {
            LockGuard lock1(mutexB, "MutexB");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            LockGuard lock2(mutexC, "MutexC");
            std::cout << "Thread2: Got B->C" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Thread3_Scenario1() {
    for (int i = 0; i < 1; ++i) {
        {
            LockGuard lock1(mutexC, "MutexC");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            LockGuard lock2(mutexA, "MutexA");  // 여기서 순환 발생!
            std::cout << "Thread3: Got C->A" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
void Thread_Scenario1()
{
    GThreadManager->Launch([]()
        {
            Thread1_Scenario1();
        });
    GThreadManager->Launch([]()
        {
            Thread2_Scenario1();
        });
    GThreadManager->Launch([]()
        {
            Thread3_Scenario1();
        });
    GThreadManager->Join();
}
void example11::Do()
{
    Thread_Scenario1();
}