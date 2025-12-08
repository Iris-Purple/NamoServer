#pragma once

#include "spinLock.h"

class example6
{
public:
	static void Do();

public:
	static int32 sum;
	static spinLock spLock;
};

