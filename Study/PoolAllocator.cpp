#include "pch.h"
#include "PoolAllocator.h"
#include "Memory.h"



void* PoolAllocator::Alloc(int32 size)
{
	return ExMemory->Allocate(size);
}

void PoolAllocator::Release(void* ptr)
{
	ExMemory->Release(ptr);
}