#include "pch.h"
#include "Room.h"
#include "RoomManager.h"


RoomRef RoomManager::Add(int mapId)
{
	RoomRef room = make_shared<Room>(_roomId);
	
	WRITE_LOCK;
	room->Init(mapId);
	
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
