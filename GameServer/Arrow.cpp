#include "pch.h"
#include "Arrow.h"
#include "Map.h"
#include "Room.h"

void Arrow::Update()
{
	RoomRef room = _room.load().lock();
	if (_owner == nullptr || room == nullptr)
		return;
	
	auto getTickCount = ::GetTickCount64();
	if (_nextMoveTick >= getTickCount)
		return;

	_nextMoveTick = getTickCount + 50;

	Vector2Int destPos = GetFrontCellPos();
	if (room->_map.CanGo(destPos))
	{
		cout << "Arrow move : " << destPos.x << "," << destPos.y << endl;
		// memory 위치 갱신 
		SetCellPos(destPos);

		Protocol::S2C_MOVE pkt;
		pkt.set_objectid(GetId());
		pkt.mutable_posinfo()->CopyFrom(GetPosInfo());
		
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		room->Broadcast(sendBuffer);
	}
	else
	{
		GameObjectRef target = room->_map.Find(destPos);
		if (target != nullptr)
		{
			// TODO 피격 판정
		}

		// 소멸
		cout << "Arrow Remove" << endl;
		room->HandleLeaveGame(GetId());
	}
}