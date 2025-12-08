#include "pch.h"
#include "example5.h"
#include "ThreadManager.h"

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

void example5::Do()
{
    A a;
    B b;
    GThreadManager->Launch([&]()
        {
            a.CallB(b);
        });

    GThreadManager->Launch([&]()
        {
            b.CallA(a);
        });

    GThreadManager->Join();

    std::cout << "Done!\n";  // 이 줄은 절대 실행 안 됨!
}