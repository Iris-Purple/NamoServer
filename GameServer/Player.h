#pragma once
#include "GameObject.h"

class GameSession;

class Player : public GameObject
{
	
public:
	Player();
	virtual ~Player();

	virtual void OnDamaged(GameObjectRef attacker, int damage) override;


public:
	weak_ptr<GameSession> session;

};

