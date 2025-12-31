#pragma once
#include "Session.h"
#include "ClientPacketHandler.h"

// 외부 전역 변수 (DummyClient.cpp에서 정의)
extern std::random_device g_rd;
extern std::mt19937 g_gen;
extern std::uniform_int_distribution<> g_delayDist;

// 세션 관리 함수
void AddActiveSession(PacketSessionRef session);
void RemoveActiveSession(PacketSessionRef session);

class GameSession : public PacketSession
{
private:
	bool _isAttack = false;

public:
	~GameSession();

	virtual void OnConnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	virtual void OnDisconnected() override;

public:
	// 부하테스트용
	std::chrono::steady_clock::time_point _lastSendTime;
	int32 _nextDelayMs = 0;
	int32 _posX = 0;
	int32 _posY = 0;

	void ResetDelay();
	bool IsTimeToSend();
	void SendMoveOrAttack();
	void SendAttackPacket();
	void SendMovePacket();
};
