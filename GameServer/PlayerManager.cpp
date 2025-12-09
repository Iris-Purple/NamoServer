#include "pch.h"
#include "PlayerManager.h"
#include "Player.h"

PlayerRef PlayerManager::Add()
{
	USE_LOCK;
	PlayerRef player = make_shared<Player>(_playerId);
	_players.emplace(_playerId, player);
	_playerId++;
	
	return player;
}

bool PlayerManager::Remove(int32 playerId)
{
	USE_LOCK;
	return _players.erase(playerId) ? true : false;
}

PlayerRef PlayerManager::Find(int32 playerId)
{
	USE_LOCK;
	auto it = _players.find(playerId);
	return it != _players.end() ? it->second : nullptr;
}

