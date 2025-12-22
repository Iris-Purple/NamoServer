#pragma once

#include "Map.h"

class Room : public enable_shared_from_this<Room>
{
public:
	Room(int32 roomId);
	virtual ~Room();

	void Init(int mapId);
	void Update();
	bool HandleEnterGame(GameObjectRef gameObject);
	bool HandleLeaveGame(int32 objectId);
	void HandleMove(PlayerRef player, const Protocol::C2S_MOVE& pkt);
	void HandleSkill(PlayerRef player, const Protocol::C2S_SKILL& pkt);

	PlayerRef FindPlayer(function<bool(GameObjectRef)> condition);
private:
	bool EnterPlayer(PlayerRef player);
	bool LeavePlayer(int32 objectId);

	USE_LOCK;

public:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
private:
	unordered_map<int32, PlayerRef> _players;
	unordered_map<int32, MonsterRef> _monsters;
	unordered_map<int32, ProjectileRef> _projectiles;
	vector<int32> _removeProjectileIds;
	vector<int32> _removeMonsterIds;
	int _roomId;
	
public:
	Map _map;
};

