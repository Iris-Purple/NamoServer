#pragma once
#include "GameObject.h"

class Projectile : public GameObject
{
public:
	bool isDead = false;

public:
	Projectile();
	virtual void Update();
};


