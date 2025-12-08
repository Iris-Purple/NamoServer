#include "pch.h"
#include "example2.h"
#include "ThreadManager.h"

atomic<int32> example2::sum = 0;
int example2::localSum = 0;


void example2::Add()
{
	for (int32 i = 0; i < 1000000; i++)
	{
		example2::sum.fetch_add(1);
		localSum++;
	}
}
void example2::Sub()
{
	for (int32 i = 0; i < 1000000; i++)
	{
		example2::sum.fetch_sub(1);
		localSum--;
	}
}

void example2::Do()
{
	example2 t;
	t.Add();
	t.Sub();
	cout <<"sum: " << example2::sum << endl;


	GThreadManager->Launch([&t]()
		{
			t.Add();
		});
	GThreadManager->Launch([&t]()
		{
			t.Sub();
		});

	GThreadManager->Join();
	cout << "thread sum: " << example2::sum << endl;
	cout << "thread localSum: " << localSum << endl;
}