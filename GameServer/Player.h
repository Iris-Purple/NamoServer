#pragma once
#include "GameObject.h"

class GameSession;

class Player : public GameObject
{
	
public:
	Player();
	virtual ~Player();

public:
	weak_ptr<GameSession> session;

public:
    

    
};

