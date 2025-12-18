#include "pch.h"
#include "Player.h"

Player::Player() : GameObject(Protocol::GameObjectType::PLAYER)
{ 
	StatInfo()->set_speed(10.0f);
}

Player::~Player() 
{ 
	cout << "~Player" << endl;
}


void Player::OnDamaged(GameObjectRef attacker, int damage)
{
	cout << "TODO damage : " << damage << endl;
}