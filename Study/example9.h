#pragma once

#include "Example9Queue.h"

class example9
{
public:
	static void Do();

	static Example9Queue<int32> lockQ;
	static void Push();
	static void Pop();
};

