#include "pch.h"
#include "GameObject.h"
#include "Map.h"


GameObject::GameObject(Protocol::GameObjectType objType) : _objType(objType)
{ }


int32 GameObject::GetId()
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

const Protocol::PositionInfo& GameObject::GetPosInfo() const
{
	return _objInfo.posinfo();
}
Protocol::PositionInfo* GameObject::MutablePosInfo()
{
	return _objInfo.mutable_posinfo();
}


Vector2Int GameObject::GetCellPos()
{
	return Vector2Int(
		_objInfo.posinfo().posx(),
		_objInfo.posinfo().posy()
	);
}
void GameObject::SetCellPos(const Vector2Int& pos)
{
	_objInfo.mutable_posinfo()->set_posx(pos.x);
	_objInfo.mutable_posinfo()->set_posy(pos.y);
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
	auto posInfo = GetPosInfo();
	return GetFrontCellPos(posInfo.movedir());
}