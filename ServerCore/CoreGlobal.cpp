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


// 패킷 암호화 설정
bool GEncryptionEnabled = false;  // 기본값: OFF (개발 중에는 false, 배포 시 true)

// 암호화 키 (AES-128: 16바이트) - 실제 배포 시 안전한 키로 변경 필요
BYTE GEncryptionKey[16] = {
	0x4E, 0x61, 0x6D, 0x6F, 0x53, 0x65, 0x72, 0x76,  // "NamoServ"
	0x65, 0x72, 0x4B, 0x65, 0x79, 0x31, 0x32, 0x33   // "erKey123"
};

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