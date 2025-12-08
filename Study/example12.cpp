#include "pch.h"
#include "example12.h"


void* StompAllocator::Alloc(int32 size)
{
    const int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    const int64 dataOffset = pageCount * PAGE_SIZE - size;
    void* baseAddress = ::VirtualAlloc(NULL, pageCount * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return static_cast<void*>(static_cast<int8*>(baseAddress) + dataOffset);
}

void StompAllocator::Release(void* ptr)
{
    const int64 address = reinterpret_cast<int64>(ptr);
    const int64 baseAddress = address - (address % PAGE_SIZE);

    ::VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
}


class Knight
{
public:
    Knight() { cout << "Knight()" << endl; }
    ~Knight() { cout << "~Knight()" << endl; }

    int _hp;
};
class Player { };
void example12::Do()
{
    // memory 오염
    Knight* knight = static_cast<Knight*>(StompAllocator::Alloc(sizeof(Knight)));
    StompAllocator::Release(knight);
    knight->_hp = 200;
    
    // delete 후 메모리 침범
    //Knight* knight = new Knight();
    //delete knight;
    //knight->_hp = 200;

    // memory overflow
    //Knight* knight = static_cast<Knight*>(StompAllocator::Alloc(sizeof(Player)));
    //knight->_hp = 200;

    // 표준 new 메모리 침범
    //Knight* knight = (Knight*)(new Player);
    //knight->_hp = 20;
}