#pragma once

struct Vector2Int;

class GameObject : public enable_shared_from_this<GameObject>
{
public:
	// protobuf 변수
	Protocol::GameObjectType _objType = Protocol::GameObjectType::NONE;

	int32 _objectId = 0;
	string _name;
	
	// PositionInfo
	Protocol::CreatureState _state = Protocol::CreatureState::Idle;
	Protocol::MoveDir _moveDir = Protocol::MoveDir::Down;
	int32 _posX = 0;
	int32 _posY = 0;

	// StatInfo
	int32 _level = 1;
	int32 _hp = 0;
	int32 _maxHp = 0;
	int32 _attack = 0;
	float _speed = 0.f;
	int32 _totalExp = 0;

public:
	// Protobuf 변환
	Protocol::ObjectInfo ToObjectInfo() const;
	Protocol::PositionInfo ToPositionInfo() const;
	Protocol::StatInfo ToStatInfo() const;

	void FromObjectInfo(const Protocol::ObjectInfo& info);
	void FromPositionInfo(const Protocol::PositionInfo& info);
	void FromStatInfo(const Protocol::StatInfo& info);


public:
	atomic<weak_ptr<Room>> _room;

public:
	GameObject(Protocol::GameObjectType objType);
	~GameObject() = default;


	Vector2Int GetCellPos();
	void SetCellPos(const Vector2Int& pos);
	Vector2Int GetFrontCellPos(const Protocol::MoveDir& dir);
	Vector2Int GetFrontCellPos();

public:
	virtual void OnDamaged(GameObjectRef attacker, int damage);
	virtual void OnDead(GameObjectRef attacker);
	virtual void Update() { }

	static Protocol::MoveDir GetDirFromVec(Vector2Int dir);
};

