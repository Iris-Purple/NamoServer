#pragma once

#include "pch.h"
#include "ThreadManager.h"

/*
	스레드 10개를 생성해서  i 값을 넘겨받아 출력하는  기능
	Join 에서  10개 스레드를 종료될때 까지 기다린다
*/

inline void output(int num)
{
	cout << num << endl;
}
inline void Example1()
{
	for (int32 i = 0; i < 10; i++)
	{
		GThreadManager->Launch([i]()
			{
				output(i);
			});
	}
	
	GThreadManager->Join();
}


