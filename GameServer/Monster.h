#pragma once
#include "GameObject.h"

class Monster : public GameObject
{
public:
	Monster();

public:
	virtual void Update() override;
	
public:
	int _searchCellDist = 10;
	uint64 _nextSearchTick = 0;
	PlayerRef _target;

	uint64 _nextMoveTick = 0;
	int _chaseCellDist = 20;
	int _skillRange = 1;
	uint64 _coolTick = 0;

protected:
	virtual void UpdateIdle();
	virtual void UpdateMoving();
	virtual void UpdateSkill();
	virtual void UpdateDead();

private:
	void BroadcastMove(RoomRef room, Protocol::PositionInfo posInfo);
};

