#include "pch.h"
#include "ObjectManager.h"
#include "Player.h"


bool ObjectManager::Remove(int32 objectId)
{
	Protocol::GameObjectType objectType = GetObjectTypeById(objectId);

	USE_LOCK;
	if (objectType == Protocol::GameObjectType::PLAYER)
	{
		return _players.erase(objectId) > 0;
	}
	return false;
}

PlayerRef ObjectManager::Find(int32 objectId)
{
	Protocol::GameObjectType objectType = GetObjectTypeById(objectId);

	USE_LOCK;
	if (objectType == Protocol::GameObjectType::PLAYER)
	{
		auto it = _players.find(objectId);
		if (it != _players.end())
			return it->second;
	}
	return nullptr;
}

