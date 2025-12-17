#pragma once

struct Vector2Int;

class GameObject : public enable_shared_from_this<GameObject>
{
protected:
	Protocol::GameObjectType _objType = Protocol::GameObjectType::NONE;
	
public:
	atomic<weak_ptr<Room>> _room;
	Protocol::ObjectInfo _objInfo;

public:
	GameObject(Protocol::GameObjectType objType);
	~GameObject() = default;

	int32 GetId();
	void SetId(int32 id);

	Protocol::GameObjectType GetObjectType();
	void SetObjectType(Protocol::GameObjectType objType);

	const Protocol::PositionInfo& GetPosInfo() const;
	Protocol::PositionInfo* MutablePosInfo();

	Vector2Int GetCellPos();
	void SetCellPos(const Vector2Int& pos);

	Vector2Int GetFrontCellPos(const Protocol::MoveDir& dir);
	Vector2Int GetFrontCellPos();
};

