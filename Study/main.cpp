
#include "pch.h"
#include <iostream>
//#include "example3.h"
//#include "example4.h"
//#include "example5.h"
//#include "example6.h"
//#include "example7.h"
//#include "example8.h"
//#include "example9.h"
//#include "example10.h"
//#include "example11.h"
//#include "example12.h"
//#include "example13.h"
//#include "example14.h"

class A
{
public:
	void ProcessRecv()
	{
		OnRecv();
	}
	virtual void OnRecv()
	{
		cout << "A.OnRecv()" << endl;
	}
};
class B : public A
{
public:
	void OnRecv() sealed
	{
		cout << "B.OnRecv()" << endl;;
		OnRecvPacket();
	}
	virtual void OnRecvPacket() = 0;

};
class C : public B
{
public:
	virtual void OnRecvPacket() override
	{
		cout << "C.OnRecvPacket()" << endl;
	}
};

int main()
{
	class C ccc;
	ccc.ProcessRecv();

	//example3::Do();
	//example4::Do();
	//example5::Do();
	//example6::Do();
	//example7::Do();
	//example8::Do();
	//example9::Do();
	//example10::Do();
	//example11::Do();
	//example12::Do();
	//example13::Do();
	//example14::Do();

	

	std::cin.get();  // Enter 누를 때까지 대기
	return 0;
}

