#include "pch.h"
#include "example8.h"
#include "ThreadManager.h"


thread_local int32 example8::LThreadId = 0;

void example8::ThreadMain(int32 threadId)
{
	LThreadId = threadId;
	while (true)
	{
		cout << "Hi! I am Thread " << LThreadId << endl;
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
}
void example8::Do()
{
	vector<thread> threads;
	for (int32 i = 0; i < 10; i++)
	{
		int32 threadId = i + 1;
		GThreadManager->Launch([threadId]()
			{
				ThreadMain(threadId);
			});
	}
	
	GThreadManager->Join();
}