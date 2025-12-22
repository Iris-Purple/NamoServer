#include "pch.h"
#include "GameObject.h"
#include "Map.h"
#include "Room.h"


GameObject::GameObject(Protocol::GameObjectType objType) : _objType(objType)
{ }


int32 GameObject::Id()
{
	return _objInfo.objectid();
}
void GameObject::SetId(int32 id)
{
	_objInfo.set_objectid(id);
}

Protocol::GameObjectType GameObject::GetObjectType()
{
	return _objType;
}

void GameObject::SetObjectType(Protocol::GameObjectType objType)
{
	_objType = objType;
}

Protocol::PositionInfo* GameObject::PosInfo()
{
	return _objInfo.mutable_posinfo();
}

Protocol::StatInfo* GameObject::StatInfo()
{
	return _objInfo.mutable_statinfo();
}


Vector2Int GameObject::GetCellPos()
{
	return Vector2Int(
		PosInfo()->posx(),
		PosInfo()->posy()
	);
}
void GameObject::SetCellPos(const Vector2Int& pos)
{
	PosInfo()->set_posx(pos.x);
	PosInfo()->set_posy(pos.y);
}

Vector2Int GameObject::GetFrontCellPos(const Protocol::MoveDir& dir)
{
	Vector2Int cellPos = GetCellPos();
	switch (dir)
	{
	case Protocol::MoveDir::Up:
		cellPos += Vector2Int::Up();
		break;
	case Protocol::MoveDir::Down:
		cellPos += Vector2Int::Down();
		break;
	case Protocol::MoveDir::Left:
		cellPos += Vector2Int::Left();
		break;
	case Protocol::MoveDir::Right:
		cellPos += Vector2Int::Right();
		break;
	}

	return cellPos;
}
Vector2Int GameObject::GetFrontCellPos()
{
	return GetFrontCellPos(PosInfo()->movedir());
}

Protocol::MoveDir GameObject::GetDirFromVec(Vector2Int dir)
{
	if (dir.x > 0)
		return Protocol::MoveDir::Right;
	else if (dir.x < 0)
		return Protocol::MoveDir::Left;
	else if (dir.y > 0)
		return Protocol::MoveDir::Up;
	else
		return Protocol::MoveDir::Down;

}

void GameObject::OnDamaged(GameObjectRef attacker, int damage)
{
	RoomRef room = _room.load().lock();
	if (room == nullptr)
		return;

	auto stat = StatInfo();
	int32 hp = stat->hp();
	hp = std::max(hp - damage, 0);
	stat->set_hp(hp);


	Protocol::S2C_CHANGE_HP resPkt;
	resPkt.set_objectid(Id());
	resPkt.set_hp(stat->hp());
	
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	room->Broadcast(sendBuffer);
	

	if (stat->hp() <= 0)
	{
		OnDead(attacker);
	}
}

void GameObject::OnDead(GameObjectRef attacker)
{
	RoomRef room = _room.load().lock();
	if (room == nullptr)
		return;

	Protocol::S2C_DIE resPkt;
	resPkt.set_objectid(Id());
	resPkt.set_attackerid(attacker->Id());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	room->Broadcast(sendBuffer);
	
	room->HandleLeaveGame(Id());

	auto stat = StatInfo();
	stat->set_hp(stat->maxhp());

	auto posInfo = PosInfo();
	posInfo->set_state(Protocol::CreatureState::Idle);
	posInfo->set_movedir(Protocol::MoveDir::Down);
	posInfo->set_posx(0);
	posInfo->set_posy(0);

	room->HandleEnterGame(shared_from_this());
}