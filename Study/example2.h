#pragma once


class example2
{
public:
	static void Do();

private:
	static atomic<int32> sum;
	static int localSum;
	void Add();
	void Sub();
};

