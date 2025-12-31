#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "GameSession.h"

// 부하테스트 설정
const int32 CLIENT_COUNT = 1;		// 총 클라이언트 수
const int32 BATCH_SIZE = 50;			// 한 번에 입장시킬 클라이언트 수
const int32 BATCH_INTERVAL_SEC = 30;	// 배치 간격 (초)
const int32 WORKER_THREAD_COUNT = 5;	// IOCP 워커 스레드 수

int main()
{
	// 서버와 동일한 설정 필요 (config.json 값과 일치해야 함)
	GEncryptionEnabled = false;   // 암호화 ON/OFF

	ClientPacketHandler::Init();

	this_thread::sleep_for(1s);

	ClientServiceRef service = make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		CLIENT_COUNT);

	// IOCP Core 등록만 (Start 호출 안함)
	// service->Start()는 한번에 모든 세션 연결하므로 사용 안함

	// IOCP 워커 스레드 시작
	for (int32 i = 0; i < WORKER_THREAD_COUNT; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	// 배치 연결 스레드: 100명씩 10초 간격으로 입장
	GThreadManager->Launch([=]()
		{
			int32 connectedCount = 0;
			while (connectedCount < CLIENT_COUNT)
			{
				int32 batchCount = min(BATCH_SIZE, CLIENT_COUNT - connectedCount);

				cout << "[Batch] Connecting " << batchCount << " clients... ("
					 << connectedCount << " -> " << connectedCount + batchCount << ")" << endl;

				for (int32 i = 0; i < batchCount; i++)
				{
					SessionRef session = service->CreateSession();
					if (session)
						session->Connect();
				}

				connectedCount += batchCount;

				if (connectedCount < CLIENT_COUNT)
				{
					cout << "[Batch] Waiting " << BATCH_INTERVAL_SEC << " seconds..." << endl;
					this_thread::sleep_for(chrono::seconds(BATCH_INTERVAL_SEC));
				}
			}
			cout << "[Batch] All " << CLIENT_COUNT << " clients connected!" << endl;
		});

	// 부하테스트: 활성 세션들에게 랜덤 딜레이로 C2S_MOVE 전송
	while (true)
	{
		{
			lock_guard<mutex> lock(g_sessionLock);
			for (auto& session : g_activeSessions)
			{
				auto gameSession = static_pointer_cast<GameSession>(session);
				if (gameSession->IsTimeToSend())
				{
					gameSession->SendMoveOrAttack();
				}
			}
		}

		this_thread::sleep_for(50ms);  // 폴링 주기 50ms
	}

	GThreadManager->Join();
}
