#include "pch.h"
#include "example9.h"
#include "ThreadManager.h"
#include "example9Queue.h"

Example9Queue<int32> example9::lockQ;

void example9::Push()
{
	while (true)
	{
		int32 value = rand() % 100;
		lockQ.Push(value);
		this_thread::sleep_for(chrono::milliseconds(10));
	}
}
void example9::Pop()
{
	while (true)
	{
		int32 data = 0;
		if (lockQ.TryPop(OUT data))
			cout << data << endl;
	}
}
void example9::Do()
{
	GThreadManager->Launch([]()
		{
			Push();
		});
	GThreadManager->Launch([]()
		{
			Pop();
		});
	GThreadManager->Launch([]()
		{
			Pop();
		});

	GThreadManager->Join();
}