#include "pch.h"
#include "Player.h"

Player::Player() : GameObject(Protocol::GameObjectType::PLAYER)
{ }

Player::~Player() 
{ 
	cout << "~Player" << endl;
}


