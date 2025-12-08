#include "pch.h"
#include "example7.h"
#include "ThreadManager.h"

mutex example7::m;
queue<int32> example7::q;

// CV는 User-Level Object
condition_variable example7::cv;

void Producer()
{
	while (true)
	{
		// 1) Lock을 잡고
		// 2) 공유 변수 값을 수정
		// 3) Lock을 풀고
		// 4) 조건변수 통해 다른 스레드에게 통지

		{
			unique_lock<mutex> lock(example7::m);
			example7::q.push(100);
		}
		example7::cv.notify_one(); // wait 중인 스레드가 있으면 1개를 깨운다
		this_thread::sleep_for(chrono::milliseconds(10));
	}
}

void Consumer()
{
	while (true)
	{
		unique_lock<mutex> lock(example7::m);
		example7::cv.wait(lock, []() { return example7::q.empty() == false;  });
		// 1) Lock 잡고
		// 2) 조건 확인 
		// - 만족하면 => 빠져 나와서 아래 코드 실행
		// - 만족 못하면 => Lock을 풀어주고 대기 상태

		// 그런데 notify_one 을 했으면 항상 조건식을 만족하는거 아닐까?
		// Spurious Wakeup

		int data = example7::q.front();
		example7::q.pop();
		cout << data << endl;

	}
}

void example7::Do()
{
	GThreadManager->Launch([]()
		{
			Producer();
		});
	GThreadManager->Launch([]()
		{
			Consumer();
		});

	GThreadManager->Join();
}