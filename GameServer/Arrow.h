#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{
public:
	GameObjectRef _owner;
	int64 _nextMoveTick = 0;

public:
	Arrow() = default;
	virtual ~Arrow() = default;
	virtual void Update() override;
};

