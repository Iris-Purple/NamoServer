#include "pch.h"
#include "Arrow.h"
#include "Map.h"
#include "Room.h"

void Arrow::Update()
{
	RoomRef room = _room.load().lock();
	if (Data.skillType == Protocol::SkillType::SKILL_NONE || _owner == nullptr || room == nullptr)
		return;

	auto getTickCount = ::GetTickCount64();
	if (_nextMoveTick >= getTickCount)
		return;

	long tick = (long)(1000 / Data.projectile->speed);
	_nextMoveTick = getTickCount + tick;

	Vector2Int destPos = GetFrontCellPos();
	if (room->_map.CanGo(destPos))
	{
		//cout << "Arrow move : " << destPos.x << "," << destPos.y << endl;
		SetCellPos(destPos);

		Protocol::S2C_MOVE pkt;
		pkt.set_objectid(_objectId);
		pkt.mutable_posinfo()->CopyFrom(ToPositionInfo());
		
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		room->Broadcast(sendBuffer);
	}
	else
	{
		GameObjectRef target = room->_map.Find(destPos);
		if (target != nullptr)
		{
			//cout << "arrow target match" << endl;
			target->OnDamaged(static_pointer_cast<Arrow>(shared_from_this()), Data.damage + _owner->_attack);
		}

		//cout << "Arrow Remove" << endl;
		room->DoAsync(&Room::HandleLeaveGame, _objectId);
	}
}

void Arrow::OnDamaged(GameObjectRef attacker, int damage)
{
}
