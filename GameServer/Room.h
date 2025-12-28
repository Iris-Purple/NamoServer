#pragma once

#include "Map.h"
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	Room(int32 roomId);
	virtual ~Room();

	void Init(int mapId);
	void Update();

	bool HandleEnterGame(GameObjectRef gameObject);
	bool HandleLeaveGame(int32 objectId);
	void HandleMove(PlayerRef player, Protocol::C2S_MOVE pkt);
	void HandleSkill(PlayerRef player, Protocol::C2S_SKILL pkt);

	PlayerRef FindPlayer(function<bool(GameObjectRef)> condition);
	int32 GetPlayerCount() const { return static_cast<int32>(_players.size()); }
private:
	bool EnterPlayer(PlayerRef player);
	bool LeavePlayer(int32 objectId);

public:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
private:
	unordered_map<int32, PlayerRef> _players;
	unordered_map<int32, MonsterRef> _monsters;
	unordered_map<int32, ProjectileRef> _projectiles;
	int _roomId;
	int _monsterCount = 5;
	
public:
	Map _map;

private:
	static const uint64 _pingInterval = 10000;
	static const uint64 _pongTimeout = 3000;
public:
	void SendPing();
};

