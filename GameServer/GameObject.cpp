#include "pch.h"
#include "GameObject.h"
#include "Map.h"
#include "Room.h"
#include "Monster.h"

GameObject::GameObject(Protocol::GameObjectType objType) : _objType(objType)
{ }

Protocol::ObjectInfo GameObject::ToObjectInfo() const 
{
	Protocol::ObjectInfo info;
	info.set_objectid(_objectId);
	info.set_name(_name);
	*info.mutable_posinfo() = ToPositionInfo();
	*info.mutable_statinfo() = ToStatInfo();
	return info;
}


Protocol::PositionInfo GameObject::ToPositionInfo() const
{
	Protocol::PositionInfo info;
	info.set_state(_state);
	info.set_movedir(_moveDir);
	info.set_posx(_posX);
	info.set_posy(_posY);
	return info;
}
Protocol::StatInfo GameObject::ToStatInfo() const
{
	Protocol::StatInfo info;
	info.set_level(_level);
	info.set_hp(_hp);
	info.set_maxhp(_maxHp);
	info.set_attack(_attack);
	info.set_speed(_speed);
	info.set_totalexp(_totalExp);
	return info;
}
void  GameObject::FromObjectInfo(const Protocol::ObjectInfo& info)
{
	_objectId = info.objectid();
	_name = info.name();
	if (info.has_posinfo())
		FromPositionInfo(info.posinfo());
	if (info.has_statinfo())
		FromStatInfo(info.statinfo());
}
void GameObject::FromPositionInfo(const Protocol::PositionInfo& info)
{
	_state = info.state();
	_moveDir = info.movedir();
	_posX = info.posx();
	_posY = info.posy();
}
void GameObject::FromStatInfo(const Protocol::StatInfo& info)
{
	_level = info.level();
	_hp = info.hp();
	_maxHp = info.maxhp();
	_attack = info.attack();
	_speed = info.speed();
	_totalExp = info.totalexp();
}


Vector2Int GameObject::GetCellPos()
{
	return Vector2Int(
		_posX,
		_posY
	);
}
void GameObject::SetCellPos(const Vector2Int& pos)
{
	_posX = pos.x;
	_posY = pos.y;
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
	return GetFrontCellPos(_moveDir);
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

	_hp = std::max(_hp - damage, 0);

	Protocol::S2C_CHANGE_HP resPkt;
	resPkt.set_objectid(_objectId);
	resPkt.set_hp(_hp);
	
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	room->Broadcast(sendBuffer);
	
	if (_hp <= 0)
	{
		cout << "OnDeaded attacker Id: " << attacker->_objectId << endl;
		OnDead(attacker);
	}
}

void GameObject::OnDead(GameObjectRef attacker)
{
	RoomRef room = _room.load().lock();
	if (room == nullptr)
		return;

	Protocol::S2C_DIE resPkt;
	resPkt.set_objectid(_objectId);
	resPkt.set_attackerid(attacker->_objectId);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	room->Broadcast(sendBuffer);
	
	room->HandleLeaveGame(_objectId);

	// attacker가 Monster면 타겟 초기화
	if (attacker->_objType == Protocol::GameObjectType::MONSTER)
	{
		MonsterRef monster = static_pointer_cast<Monster>(attacker);
		monster->_target = nullptr;
	}

	// stat 초기화
	_hp = _maxHp;
	_state = Protocol::CreatureState::Idle;
	_moveDir = Protocol::MoveDir::Down;
	_posX = 0;
	_posY = 0;

	room->HandleEnterGame(shared_from_this());
}