#include "pch.h"
#include "Player.h"
#include "DataManager.h"

Player::Player() : GameObject(Protocol::GameObjectType::PLAYER)
{ 
	
}

Player::~Player() 
{ 
	cout << "~Player" << endl;
}

void Player::Create(PacketSessionRef packetSession)
{
	session = static_pointer_cast<GameSession>(packetSession);
	_name = "Player_" + std::to_string(_objectId);

	_state = Protocol::CreatureState::Idle;
	_moveDir = Protocol::MoveDir::Down;
	_posX = _posY = 0;

	const auto stat = DataManager::Instance().GetStat(1);
	_level = stat->level;
	_hp = _maxHp = stat->maxHp;
	_attack = stat->attack;
	_speed = stat->speed;
	_totalExp = stat->totalExp;
}


void Player::OnDamaged(GameObjectRef attacker, int damage)
{
	GameObject::OnDamaged(attacker, damage);
}

void Player::OnDead(GameObjectRef attacker)
{
	GameObject::OnDead(attacker);
}