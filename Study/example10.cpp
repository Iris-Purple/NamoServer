#include "pch.h"
#include "example10.h"
#include "ThreadManager.h"
#include "example10Lock.h"

using namespace example_10;


class RwLock
{
private:
	Lock _locks[1];

public:
	int32 TestRead()
	{
		ReadLockGuard readLock(_locks[0]);
		if (_queue.empty())
			return -1;

		return _queue.front();
	}
	void TestPush()
	{
		WriteLockGuard writeLock(_locks[0]);
		_queue.push(rand() % 100);

	}
	void TestPop()
	{
		WriteLockGuard writeLock(_locks[0]);

		if (_queue.empty() == false)
			_queue.pop();
	}

private:
	queue<int32> _queue;
};
static RwLock rwLock;

static void ThreadWrite()
{
	static atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);

	while (true)
	{
		rwLock.TestPush();
		this_thread::sleep_for(chrono::milliseconds(1000));
		rwLock.TestPop();
	}
}
void ThreadRead()
{
	static atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);

	while (true)
	{
		int32 value = rwLock.TestRead();
		cout << value << endl;
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}

void example10::Do()
{
	GThreadManager->Launch([]()
		{
			ThreadWrite();
		});
	GThreadManager->Launch([]()
		{
			ThreadRead();
		});

	GThreadManager->Join();
}