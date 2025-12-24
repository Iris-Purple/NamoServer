#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include <tchar.h>
#include "Job.h"
#include "RoomManager.h"
#include "Room.h"
#include "DataManager.h"
#include "ConfigManager.h"
#include "ServerMonitor.h"


enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

int main()
{
	ConfigManager::Instance().LoadConfig();
	const string& configPath = ConfigManager::Instance().GetDataPath();
	DataManager::Instance().Init(configPath);

	RoomManager::Instance().Add(1);

	ServerPacketHandler::Init();

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		100);

	ASSERT_CRASH(service->Start());

	// 서버 모니터링 설정 및 시작
	ServerMonitor::Instance().SetSessionCountGetter([&service]() {
		return service->GetCurrentSessionCount();
	});
	ServerMonitor::Instance().SetMaxSessionCountGetter([&service]() {
		return service->GetMaxSessionCount();
	});
	ServerMonitor::Instance().Start(5000); // 5초 간격으로 모니터링

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

	// Main Thread
	cout << "Listen Server....." << endl << endl;

	GThreadManager->Join();
}
