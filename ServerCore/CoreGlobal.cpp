#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "JobStats.h"

ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;
JobStats* GJobStats = nullptr;


class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
		GJobStats = new JobStats();
		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
		delete GJobStats;
		SocketUtils::Clear();
	}
} GCoreGlobal;