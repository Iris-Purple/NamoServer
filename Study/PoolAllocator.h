#pragma once
#include "pch.h"
#include <Windows.h>


#define xalloc(size)		PoolAllocator::Alloc(size)
#define xrelease(ptr)		PoolAllocator::Release(ptr)

class PoolAllocator
{
public:
	static void* Alloc(int32 size);
	static void		Release(void* ptr);
};