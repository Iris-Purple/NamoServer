#pragma once
class example4
{
public:
	static void Do();

public:
	static std::mutex mtx;         // 보호할 공유 뮤텍스
	static int shared_counter; // 여러 스레드에서 접근할 공유 변수
};

