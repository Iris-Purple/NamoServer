#pragma once
#include <stack>

extern thread_local uint32				LThreadId;
extern thread_local uint64				LEndTickCount;

extern thread_local class JobQueue* LCurrentJobQueue;

// 레이턴시 측정용 - 패킷 수신 시작 시간 
extern thread_local std::chrono::steady_clock::time_point LRecvStartTime;