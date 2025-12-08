#include "pch.h"
#include "Room.h"
#include "RoomManager.h"


RoomRef RoomManager::Add()
{
	RoomRef room = make_shared<Room>();
	
	USE_LOCK;
	_rooms.emplace(_roomId, room);
	_roomId++;

	return room;
}

bool RoomManager::Remove(int32 roomId)
{
	USE_LOCK;
	return _rooms.erase(roomId) ? true : false;
}

RoomRef RoomManager::Find(int32 roomId)
{
	USE_LOCK;
	auto it = _rooms.find(roomId);
	return it != _rooms.end() ? it->second : nullptr;
}
