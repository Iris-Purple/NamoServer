#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectManager.h"
#include "Monster.h"
#include "Arrow.h"
#include "DataManager.h"
#include "Monster.h"

Room::Room(int32 roomId) : _roomId(roomId) { }

Room::~Room() {	}

void Room::Init(int mapId)
{
	_map.LoadMap(mapId);

	MonsterRef monster = ObjectManager::Instance().Add<Monster>();
	monster->SetCellPos(Vector2Int(5, 5));
	HandleEnterGame(monster);

	DoTimer(50, &Room::Update);
}

void Room::Update()
{
	for (auto [_, monster] : _monsters)
	{
		monster->Update();
	}

	for (auto [_, projectile] : _projectiles)
	{
		projectile->Update();
	}

	// 50ms 후 다시 Update 호출
	DoTimer(50, &Room::Update);
}

bool Room::HandleEnterGame(GameObjectRef gameObject)
{
	if (gameObject == nullptr)
		return false;

	//WRITE_LOCK;
	auto type = ObjectManager::GetObjectTypeById(gameObject->Id());
	if (type == Protocol::GameObjectType::PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(gameObject);
		EnterPlayer(player);
	}
	else if (type == Protocol::GameObjectType::MONSTER)
	{
		MonsterRef monster = static_pointer_cast<Monster>(gameObject);

 		monster->_room.store(static_pointer_cast<Room>(shared_from_this()));
		_monsters.insert(make_pair(gameObject->Id(), monster));
		_map.ApplyMove(monster, Vector2Int{ monster->PosInfo()->posx(), monster->PosInfo()->posy() });
	}
	else if (type == Protocol::GameObjectType::PROJECTILE)
	{
		ProjectileRef projectile = static_pointer_cast<Projectile>(gameObject);
		projectile->_room.store(static_pointer_cast<Room>(shared_from_this()));
		_projectiles.insert(make_pair(gameObject->Id(), projectile));
	}


	{
		Protocol::S2C_SPAWN spawnPkt;
		spawnPkt.add_objects()->CopyFrom(gameObject->_objInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		for (auto& item : _players)
		{
			if (item.second->Id() == gameObject->Id())
				continue;

			if (auto session = item.second->session.lock())
				session->Send(sendBuffer);
		}
	}


	return true;
}

bool Room::HandleLeaveGame(int32 objectId)
{
	auto type = ObjectManager::GetObjectTypeById(objectId);

	if (type == Protocol::GameObjectType::PLAYER)
	{
		LeavePlayer(objectId);
	}
	else if (type == Protocol::GameObjectType::MONSTER)
	{
		MonsterRef monster = _monsters[objectId];
		
		_monsters.erase(objectId);
		_map.ApplyLeave(monster);
		monster->_room.store(weak_ptr<Room>());
	}
	else if (type == Protocol::GameObjectType::PROJECTILE)
	{
		ProjectileRef projectile = _projectiles[objectId];

		_projectiles.erase(objectId);
		projectile->_room.store(weak_ptr<Room>());
	}

	{
		Protocol::S2C_DESPAWN despawnPkt;
		despawnPkt.add_objectids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		for (auto& item : _players)
		{
			if (item.second->Id() == objectId)
				continue;

			if (auto session = item.second->session.lock())
			{
				session->Send(sendBuffer);
			}

		}
	}

	return true;
}

bool Room::EnterPlayer(PlayerRef player)
{
	if (_players.find(player->Id()) != _players.end())
		return false;

	_players.insert(make_pair(player->Id(), player));
	player->_room.store(static_pointer_cast<Room>(shared_from_this()));

	_map.ApplyMove(player, Vector2Int{ player->PosInfo()->posx(), player->PosInfo()->posy() });
	{

		Protocol::S2C_ENTER_GAME enterGamePkt;
		enterGamePkt.mutable_player()->CopyFrom(player->_objInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
		{
			session->Send(sendBuffer);
			cout << "EnterPlayerId: " << player->Id() << endl;
		}

	}

	{
		Protocol::S2C_SPAWN spawnPkt;
		for (auto& item : _players)
		{
			if (item.second->Id() == player->Id()) continue;

			spawnPkt.add_objects()->CopyFrom(item.second->_objInfo);
		}
		for (auto& item : _monsters)
			spawnPkt.add_objects()->CopyFrom(item.second->_objInfo);

		for (auto& item : _projectiles)
			spawnPkt.add_objects()->CopyFrom(item.second->_objInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->session.lock())
		{
			session->Send(sendBuffer);
		}
	}

	return true;
}

bool Room::LeavePlayer(int32 objectId)
{
	if (_players.find(objectId) == _players.end())
		return true;

	PlayerRef player = _players[objectId];
	_players.erase(objectId);

	_map.ApplyLeave(player);
	player->_room.store(weak_ptr<Room>());

	{
		Protocol::S2C_LEAVE_GAME leaveGamePkt;
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
		{
			cout << "LeavePlayerId: " << objectId << endl;
			session->Send(sendBuffer);
		}
	}
	return true;
}

void Room::HandleMove(PlayerRef player, Protocol::C2S_MOVE pkt)
{
	if (player == nullptr)
		return;

	const auto& movePosInfo = pkt.posinfo();

	auto posInfo = player->PosInfo();
	if (movePosInfo.posx() != posInfo->posx() || movePosInfo.posy() != posInfo->posy())
	{
		if (_map.CanGo(Vector2Int{ movePosInfo.posx(), movePosInfo.posy() }) == false)
			return;
	}
	posInfo->set_state(movePosInfo.state());
	posInfo->set_movedir(movePosInfo.movedir());

	_map.ApplyMove(player, Vector2Int{ movePosInfo.posx(), movePosInfo.posy() });

	Protocol::S2C_MOVE resPkt;
	resPkt.set_objectid(player->Id());
	resPkt.mutable_posinfo()->CopyFrom(pkt.posinfo());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	Broadcast(sendBuffer, player->Id());
}

void Room::HandleSkill(PlayerRef player, Protocol::C2S_SKILL pkt)
{
	if (player == nullptr)
		return;

	auto posInfo = player->PosInfo();
	if (posInfo->state() != Protocol::CreatureState::Idle)
		return;


	posInfo->set_state(Protocol::CreatureState::Skill);

	int32 skillId = pkt.info().skillid();
	Protocol::S2C_SKILL resPkt;
	resPkt.set_objectid(player->Id());
	resPkt.mutable_info()->set_skillid(skillId);
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	Broadcast(sendBuffer);

	const Data::Skill* skillData = DataManager::Instance().GetSkill(skillId);
	if (skillData == nullptr)
		return;

	switch (skillData->skillType)
	{
	case Protocol::SkillType::SKILL_AUTO:
		{
			Vector2Int skillPos = player->GetFrontCellPos(posInfo->movedir());
			GameObjectRef target = _map.Find(skillPos);
			if (target != nullptr)
			{
				cout << "Hit GameObject !" << endl;
			}
		}
		break;

	case Protocol::SkillType::SKILL_PROJECTILE:
		{
			ArrowRef arrow = ObjectManager::Instance().Add<Arrow>();
			cout << "Arrow Created : " << arrow->Id() << endl;
			if (arrow == nullptr)
				return;

			arrow->_owner = player;
			arrow->Data = *skillData;

			auto posInfo = arrow->PosInfo();
			posInfo->set_state(Protocol::CreatureState::Moving);
			posInfo->set_movedir(player->PosInfo()->movedir());
			posInfo->set_posx(player->PosInfo()->posx());
			posInfo->set_posy(player->PosInfo()->posy());

			arrow->StatInfo()->set_speed(skillData->projectile->speed);

			HandleEnterGame(arrow);
		}
		break;
	}
}

PlayerRef Room::FindPlayer(function<bool(GameObjectRef)> condition)
{
	for (auto& [id, player] : _players)
	{
		if (condition(player))
			return player;
	}
	return nullptr;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player->Id() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}
