#include "pch.h"
#include "example3.h"
#include "ThreadManager.h"

vector<int32> example3::v;
mutex example3::m;

void Push()
{
	for (int32 i = 0; i < 10000; i++)
	{
		example3::v.push_back(i);
	}
}
void PushLock()
{
	for (int32 i = 0; i < 10000; i++)
	{
		example3::m.lock();
		example3::v.push_back(i);
		example3::m.unlock();
	}
}

void example3::VectorException()
{
	for (int i = 0; i < 2; i++)
	{
		GThreadManager->Launch([]()
			{
				Push();
			});
	}

	GThreadManager->Join();
	cout << v.size() << endl;
}

void example3::VectorSizeError()
{
	v.reserve(20000);
	VectorException();
}

void example3::MutexExample()
{
	for (int i = 0; i < 2; i++)
	{
		GThreadManager->Launch([]()
			{
				PushLock();
			});
	}

	GThreadManager->Join();
	cout << v.size() << endl;
}

void example3::Do()
{
	// VectorException();
	// VectorSizeError();

	MutexExample();
}