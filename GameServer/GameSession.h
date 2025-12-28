#pragma once
#include "Session.h"

class Player;

class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	atomic<PlayerRef> myPlayer;

public:
	atomic<uint64> _lastPingTime = 0;
	atomic<uint64> _pongTime = 0;
};