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
private:
	bool isAttack = false;

public:
	~ServerSession()
	{
		//cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		//cout << "OnConnected" << endl;

		// 암호화 초기화 (GEncryptionEnabled가 true일 때만 실제 동작)
		InitEncryption(GEncryptionKey, 16);

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
	int32 _posX = 0;
	int32 _posY = 0;

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

	void SendMoveOrAttack()
	{
		if (isAttack)
		{
			SendAttackPacket();
		}
		else
		{
			SendMovePacket();
		}

		isAttack = !isAttack;
	}
	void SendAttackPacket()
	{
		Protocol::C2S_SKILL pkt;
		pkt.mutable_info()->set_skillid(2);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
		ResetDelay();
	}
	void SendMovePacket()
	{
		// 랜덤 방향 선택
		static uniform_int_distribution<> dirDist(0, 3);
		Protocol::MoveDir dir = static_cast<Protocol::MoveDir>(dirDist(g_gen));

		// 방향에 따라 위치 변경
		switch (dir)
		{
		case Protocol::MoveDir::Up:    _posY++; break;
		case Protocol::MoveDir::Down:  _posY--; break;
		case Protocol::MoveDir::Left:  _posX--; break;
		case Protocol::MoveDir::Right: _posX++; break;
		}

		Protocol::C2S_MOVE pkt;
		Protocol::PositionInfo* posInfo = pkt.mutable_posinfo();
		posInfo->set_state(Protocol::CreatureState::Moving);
		posInfo->set_movedir(dir);
		posInfo->set_posx(_posX);
		posInfo->set_posy(_posY);
		//cout << "SendMove: " << _posX << ", " << _posY << endl;
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);

		this_thread::sleep_for(100ms);
		posInfo->set_state(Protocol::CreatureState::Idle);
		sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
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
		[=]() { return make_shared<ServerSession>(); },
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
				auto serverSession = static_pointer_cast<ServerSession>(session);
				if (serverSession->IsTimeToSend())
				{
					serverSession->SendMoveOrAttack();
				}
			}
		}

		this_thread::sleep_for(50ms);  // 폴링 주기 50ms
	}

	GThreadManager->Join();
}
