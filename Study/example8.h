#pragma once
#include <windows.h>

class example8
{
public:
	static void Do();

	static thread_local int32 LThreadId;
	static void ThreadMain(int32 threadId);
};

