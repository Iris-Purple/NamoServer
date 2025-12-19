#pragma once
#include "GameObject.h"

class GameSession;

class Player : public GameObject
{
	
public:
	Player();
	virtual ~Player();

	void Create(PacketSessionRef packetSession);
	virtual void OnDamaged(GameObjectRef attacker, int damage) override;
	virtual void OnDead(GameObjectRef attacker) override;

public:
	weak_ptr<GameSession> session;

};

