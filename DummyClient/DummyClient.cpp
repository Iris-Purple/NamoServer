#include "pch.h"
#include <iostream>
#include <random>
#include <mutex>
#include <set>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ClientPacketHandler.h"

char sendData[] = "Hello World";

// 활성 세션 관리
mutex g_sessionLock;
set<PacketSessionRef> g_activeSessions;

// 랜덤 딜레이 생성기
random_device g_rd;
mt19937 g_gen(g_rd());
uniform_int_distribution<> g_delayDist(500, 2000); // 0.5초 ~ 2초

// 전방 선언
void RemoveActiveSession(PacketSessionRef session);

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		cout << "OnConnected" << endl;

		Protocol::C2S_ENTER_GAME pkt;
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// TODO : packetId 대역 체크
		ClientPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		cout << "Disconnected" << endl;
		RemoveActiveSession(GetPacketSessionRef());
	}

public:
	// 부하테스트용
	chrono::steady_clock::time_point _lastSendTime;
	int32 _nextDelayMs = 0;

	void ResetDelay()
	{
		_lastSendTime = chrono::steady_clock::now();
		_nextDelayMs = g_delayDist(g_gen);
	}

	bool IsTimeToSend()
	{
		auto now = chrono::steady_clock::now();
		auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - _lastSendTime).count();
		return elapsed >= _nextDelayMs;
	}

	void SendMovePacket()
	{
		Protocol::C2S_MOVE pkt;
		Protocol::PositionInfo* posInfo = pkt.mutable_posinfo();
		posInfo->set_state(Protocol::CreatureState::Moving);

		// 랜덤 방향
		static uniform_int_distribution<> dirDist(0, 3);
		posInfo->set_movedir(static_cast<Protocol::MoveDir>(dirDist(g_gen)));
		posInfo->set_posx(0);
		posInfo->set_posy(0);

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);

		ResetDelay();
	}
};

// 세션 관리 함수 (ServerSession 클래스 정의 이후)
void AddActiveSession(PacketSessionRef session)
{
	lock_guard<mutex> lock(g_sessionLock);
	g_activeSessions.insert(session);

	// 초기 딜레이 설정
	auto serverSession = static_pointer_cast<ServerSession>(session);
	serverSession->ResetDelay();
}

void RemoveActiveSession(PacketSessionRef session)
{
	lock_guard<mutex> lock(g_sessionLock);
	g_activeSessions.erase(session);
}

// 부하테스트 설정
const int32 CLIENT_COUNT = 100;		// 클라이언트 수
const int32 WORKER_THREAD_COUNT = 4;	// IOCP 워커 스레드 수

int main()
{
	ClientPacketHandler::Init();

	this_thread::sleep_for(1s);

	ClientServiceRef service = make_shared<ClientService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<ServerSession>(); },
		CLIENT_COUNT);

	ASSERT_CRASH(service->Start());

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

	// 부하테스트: 활성 세션들에게 랜덤 딜레이로 C2S_MOVE 전송
	while (true)
	{
		{
			lock_guard<mutex> lock(g_sessionLock);
			for (auto& session : g_activeSessions)
			{
				auto serverSession = static_pointer_cast<ServerSession>(session);
				if (serverSession->IsTimeToSend())
				{
					serverSession->SendMovePacket();
				}
			}
		}

		this_thread::sleep_for(50ms);  // 폴링 주기 50ms
	}

	GThreadManager->Join();
}
