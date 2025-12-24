#include "pch.h"
#include "Room.h"
#include "RoomManager.h"


RoomRef RoomManager::Add(int mapId)
{
	RoomRef room = make_shared<Room>(_roomId);
	
	room->DoAsync(&Room::Init, mapId);
	
	WRITE_LOCK;
	_rooms.emplace(_roomId, room);
	_roomId++;
	cout << "RoomManager  Add() called" << endl;
	return room;
}

bool RoomManager::Remove(int32 roomId)
{
	WRITE_LOCK;
	return _rooms.erase(roomId) ? true : false;
}

RoomRef RoomManager::Find(int32 roomId)
{
	WRITE_LOCK;
	auto it = _rooms.find(roomId);
	return it != _rooms.end() ? it->second : nullptr;
}

int32 RoomManager::GetRoomCount()
{
	WRITE_LOCK;
	return static_cast<int32>(_rooms.size());
}

int32 RoomManager::GetTotalPlayerCount()
{
	WRITE_LOCK;
	int32 total = 0;
	for (auto& [id, room] : _rooms)
	{
		total += room->GetPlayerCount();
	}
	return total;
}
