#pragma once
#include <Windows.h>
#include "example12.h"

class example13
{
public:
	static void Do();
};

template<typename T>
class StlAllocator
{
public:
	using value_type = T;

	StlAllocator() {}

	template<typename Other>
	StlAllocator(const StlAllocator<Other>&) {}

	T* allocate(size_t count)
	{
		const int32 size = static_cast<int32> (count * sizeof(T));
		return static_cast<T*>(StompAllocator::Alloc(size));
	}

	void deallocate(T* ptr, size_t count)
	{
		StompAllocator::Release(ptr);
	}
};


template<typename Type>
using Vector = vector<Type, StlAllocator<Type>>;

template<typename Type>
using List = list<Type, StlAllocator<Type>>;

template<typename Key, typename Type, typename Pred = less<Key>>
using Map = map<Key, Type, Pred, StlAllocator<pair<const Key, Type>>>;

template<typename Key, typename Pred = less<Key>>
using Set = set<Key, Pred, StlAllocator<Key>>;


