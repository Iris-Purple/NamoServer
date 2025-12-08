#include "pch.h"
#include "example12.h"
#include "example13.h"



class Knight
{
public:
    Knight() { cout << "Knight()" << endl; }
    ~Knight() { cout << "~Knight()" << endl; }

    int _hp = 10;
};

void example13::Do()
{
    // 100개 Knight 객체를 담는 벡터
    Vector<Knight> knights(100);

    // 각 Knight마다 독립된 페이지에 할당됨
    knights[0]._hp = 100;
    knights[99]._hp = 200;
}