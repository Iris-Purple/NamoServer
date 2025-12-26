#include "pch.h"
#include "Monster.h"
#include "Room.h"
#include "Player.h"
#include "DataManager.h"

Monster::Monster() : GameObject(Protocol::GameObjectType::MONSTER)
{
	_level = 1;
	_hp = _maxHp = 100;
	_speed = 5.0f;

	_state = Protocol::CreatureState::Idle;
}

void Monster::Update()
{
	switch (_state)
	{
	case Protocol::CreatureState::Idle:
		UpdateIdle();
		break;
	case Protocol::CreatureState::Moving:
		UpdateMoving();
		break;
	case Protocol::CreatureState::Skill:
		UpdateSkill();
		break;
	case Protocol::CreatureState::Dead:
		UpdateDead();
		break;
	}
}

void Monster::UpdateIdle()
{
	auto getTickCount = ::GetTickCount64();
	if (_nextSearchTick > getTickCount)
		return;
	_nextSearchTick = getTickCount + 1000;

	RoomRef room = _room.load().lock();
	PlayerRef target = room->FindPlayer([&](GameObjectRef obj) {
		Vector2Int dir = obj->GetCellPos() - GetCellPos();
		return dir.CellDistFromZero() < _searchCellDist;
	});
	if (target == nullptr)
		return;

	_target = target;
	_state = Protocol::CreatureState::Moving;
}
void Monster::UpdateMoving()
{
	auto room = _room.load().lock();
	if (room == nullptr)
		return;

	auto getTickCount = ::GetTickCount64();
	if (_nextMoveTick > getTickCount)
		return;
	int moveTick = (int)(1000 / _speed);
	_nextMoveTick = getTickCount + moveTick;

	if (_target == nullptr || _target->_room.load().lock() != room)
	{
		_target = nullptr;
		_state = Protocol::CreatureState::Idle;
		BroadcastMove(room, ToPositionInfo());
		return;
	}

	// 너무 멀리 있음
	Vector2Int dir = _target->GetCellPos() - GetCellPos();
	int dist = dir.CellDistFromZero();
	if (dist == 0 || dist > _chaseCellDist)
	{
		_target = nullptr;
		_state = Protocol::CreatureState::Idle;
		BroadcastMove(room, ToPositionInfo());
		return;
	}

	// 스킬 
	if (dist <= _skillRange && (dir.x == 0 || dir.y == 0))
	{
		_coolTick = 0;
		_state = Protocol::CreatureState::Skill;
		return;
	}

	// 길찾는 중 멀리있음
	vector<Vector2Int> path = room->_map.FindPath(GetCellPos(), _target->GetCellPos(), false);
	if (path.size() < 2 || path.size() > _chaseCellDist + 1)
	{
		_target = nullptr;
		_state = Protocol::CreatureState::Idle;
		BroadcastMove(room, ToPositionInfo());
		return;
	}
	
	// 이동
	_moveDir = GetDirFromVec(path[1] - GetCellPos());
	if (room->_map.ApplyMove(static_pointer_cast<Monster>(shared_from_this()), path[1]))
	{
		BroadcastMove(room, ToPositionInfo());
		return;
	}
}

void Monster::BroadcastMove(RoomRef room, Protocol::PositionInfo posInfo)
{
	// 다른 플레이어 알려준다
	Protocol::S2C_MOVE resPkt;
	resPkt.set_objectid(_objectId);
	resPkt.mutable_posinfo()->CopyFrom(posInfo);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	room->Broadcast(sendBuffer);
}

void Monster::UpdateSkill()
{
	auto room = _room.load().lock();
	if (room == nullptr)
		return;

	if (_coolTick == 0)
	{
		// 유효한 타겟인지
		if (_target == nullptr || _hp == 0)
		{
			_target = nullptr;
			_state = Protocol::CreatureState::Idle;
			BroadcastMove(room, ToPositionInfo());
			return;
		}


		// 스킬이 사용가능한지
		Vector2Int dir = (_target->GetCellPos() - GetCellPos());
		int dist = dir.CellDistFromZero();
		bool canUseSkill = (dist <= _skillRange && (dir.x == 0 || dir.y == 0));
		if (canUseSkill == false)
		{
			_state = Protocol::CreatureState::Moving;
			BroadcastMove(room, ToPositionInfo());
			return;
		}
		
		// 타게팅 방향 주시
		auto lookDir = GetDirFromVec(dir);
		if (_moveDir != lookDir)
		{
			_moveDir = lookDir;
			BroadcastMove(room, ToPositionInfo());
		}

		auto skillData = DataManager::Instance().GetSkill(1);

		// 데미지 판정
		_target->OnDamaged(static_pointer_cast<Monster>(shared_from_this()), skillData->damage + _attack);
		
		// 스킬 사용 Broadcast
		Protocol::S2C_SKILL resPkt;
		resPkt.set_objectid(_objectId);
		resPkt.mutable_info()->set_skillid(skillData->id);
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
		room->Broadcast(sendBuffer);

		// 스킬 쿨타임 적용
		int coolTick = (int)(1000 * skillData->cooldown);
		_coolTick = ::GetTickCount64() + coolTick;
	}

	if (_coolTick > GetTickCount64())
		return;

	_coolTick = 0;
}
void Monster::UpdateDead() 
{ 
	cout << "Monster::UpdateDead()......" << endl;
}