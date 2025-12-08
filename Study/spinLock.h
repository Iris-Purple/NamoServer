#pragma once

class spinLock
{
public:
	void lock();
	void unlock();

private:
	atomic<bool> _locked = false;
};

