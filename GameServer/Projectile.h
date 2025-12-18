#pragma once
#include "GameObject.h"
#include "Stat.h"

class Projectile : public GameObject
{
public:
	Data::Skill Data;

public:
	Projectile();
	virtual void Update();
};


