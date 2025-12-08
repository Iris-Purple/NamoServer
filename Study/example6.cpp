#include "pch.h"
#include "example6.h"
#include "spinLock.h"
#include "ThreadManager.h"


int32 example6::sum = 0;
spinLock example6::spLock;

void Add()
{
	for (int32 i = 0; i < 100000; i++)
	{
		lock_guard<spinLock> guard(example6::spLock);
		example6::sum++;
	}
}
void Sub()
{
	for (int32 i = 0; i < 100000; i++)
	{
		lock_guard<spinLock> guard(example6::spLock);
		example6::sum--;
	}
}




void example6::Do()
{
    GThreadManager->Launch([]()
        {
			Add();
        });
	GThreadManager->Launch([]()
		{
			Sub();
		});
	
	GThreadManager->Join();

	cout << "sum : " << sum << endl;
}
