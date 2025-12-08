#include "pch.h"
#include "spinLock.h"


void spinLock::lock()
{
	bool expected = false;
	bool desired = true;

	while (_locked.compare_exchange_strong(expected, desired) == false)
	{
		expected = false;
	}
}
void spinLock::unlock()
{
	_locked.store(false);
}