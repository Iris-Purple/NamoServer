#include "pch.h"
#include "example14.h"
#include "Memory.h"
#include "PoolAllocator.h"




class Knight
{
public:
	int32 _hp = rand() % 1000;
};


void example14::Do()
{
	Knight* knight = xnew<Knight>();

	cout << knight->_hp << endl;

	this_thread::sleep_for(chrono::milliseconds(100));

	xdelete(knight);

}