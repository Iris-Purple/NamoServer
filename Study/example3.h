#pragma once

class example3
{
public:
	static void Do();

	static void VectorException();
	static void VectorSizeError();
	static void MutexExample();

public:
	static vector<int32> v;
	static mutex m;
};


