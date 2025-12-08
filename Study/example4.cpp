#include "pch.h"
#include "example4.h"
#include "ThreadManager.h"



mutex example4::mtx;         // 보호할 공유 뮤텍스
int example4::shared_counter = 0; // 여러 스레드에서 접근할 공유 변수


void increment_counter(int id) {
    // lock_guard 생성 시 뮤텍스 잠김, 블록이 끝나면 자동으로 해제
    std::lock_guard<std::mutex> guard(example4::mtx);

    for (int i = 0; i < 5000; i++)
    {
        example4::shared_counter += 1; // 크리티컬 섹션 (공유 자원 접근)
    }
    std::cout << "Thread " << id << " finish " << std::endl;
}

void example4::Do()
{
    for (int i = 0; i < 10; i++)
    {
        GThreadManager->Launch([i]()
            {
                increment_counter(i);
            });
    }
		
    GThreadManager->Join();

    std::cout << "shared_counter: " << shared_counter << std::endl;
}