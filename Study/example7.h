#pragma once
class example7
{
public:
	static void Do();

	static mutex m;
	static queue<int32> q;

	// CV´Â User-Level Object
	static condition_variable cv;
};

