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
	_objInfo.set_name("Player_" + std::to_string(Id()));
	auto posInfo = PosInfo();
	posInfo->set_state(Protocol::CreatureState::Idle);
	posInfo->set_movedir(Protocol::MoveDir::Down);
	posInfo->set_posx(0);
	posInfo->set_posy(0);

	auto statInfo = StatInfo();
	const auto stat = DataManager::Instance().GetStat(1);
	statInfo->set_level(stat->level);
	statInfo->set_hp(stat->maxHp);
	statInfo->set_maxhp(stat->maxHp);
	statInfo->set_attack(stat->attack);
	statInfo->set_speed(stat->speed);
	statInfo->set_totalexp(stat->totalExp);
}


void Player::OnDamaged(GameObjectRef attacker, int damage)
{
	GameObject::OnDamaged(attacker, damage);
}

void Player::OnDead(GameObjectRef attacker)
{
	GameObject::OnDead(attacker);
}