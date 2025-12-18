#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{
public:
	GameObjectRef _owner;
	uint64 _nextMoveTick = 0;

public:
	Arrow() = default;
	virtual ~Arrow() = default;
	virtual void Update() override;
	virtual void OnDamaged(GameObjectRef attacker, int damage) override;
};

