#include "pch.h"
#include "Player.h"

Player::Player(int32 playerId)
	: _playerId(playerId), info(std::make_unique<Protocol::PlayerInfo>())
{
}

Player::~Player() 
{ 
	cout << "~Player" << endl;
}
