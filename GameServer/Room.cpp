#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectManager.h"
#include "Monster.h"
#include "Arrow.h"


Room::Room(int32 roomId) : _roomId(roomId) { }

Room::~Room() {	}

void Room::Init(int mapId)
{
	_map.LoadMap(mapId);
}

void Room::Update()
{
	WRITE_LOCK;
	for (auto [_, projectile] : _projectiles)
	{
		projectile->Update();
	}
	for (int32 id : _removeProjectileIds)
		_projectiles.erase(id);

	_removeProjectileIds.clear();
}

bool Room::HandleEnterGame(GameObjectRef gameObject)
{
	if (gameObject == nullptr)
		return false;

	auto type = ObjectManager::GetObjectTypeById(gameObject->GetId());
	if (type == Protocol::GameObjectType::PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(gameObject);
		// TODO return 처리
		EnterPlayer(player);
	}
	else if (type == Protocol::GameObjectType::MONSTER)
	{
		MonsterRef monster = static_pointer_cast<Monster>(gameObject);
		monster->_room.store(shared_from_this());
		_monsters.insert(make_pair(gameObject->GetId(), monster));
		
	}
	else if (type == Protocol::GameObjectType::PROJECTILE)
	{
		ProjectileRef projectile = static_pointer_cast<Projectile>(gameObject);
		projectile->_room.store(shared_from_this());
		_projectiles.insert(make_pair(gameObject->GetId(), projectile));
	}

	// 입장 사실을 다른 플레이어에게 알린다
	{
		Protocol::S2C_SPAWN spawnPkt;
		spawnPkt.add_objects()->CopyFrom(gameObject->_objInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		for (auto& item : _players)
		{
			if (item.second->GetId() == gameObject->GetId())
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
		// TODO return 처리
		LeavePlayer(objectId);
	}
	else if (type == Protocol::GameObjectType::MONSTER)
	{
		MonsterRef monster = _monsters[objectId];
		monster->_room.store(weak_ptr<Room>());
		_monsters.erase(objectId);

		_map.ApplyLeave(monster);
	}
	else if (type == Protocol::GameObjectType::PROJECTILE)
	{
		ProjectileRef projectile = _projectiles[objectId];
		projectile->_room.store(weak_ptr<Room>());
		_removeProjectileIds.push_back(objectId);
		//_projectiles.erase(objectId);
	}

	// 퇴장 사실을 알린다
	{
		Protocol::S2C_DESPAWN despawnPkt;
		despawnPkt.add_objectids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		for (auto& item : _players)
		{
			if (item.second->GetId() == objectId) 
				continue;
		
			if (auto session = item.second->session.lock())
			{
				//cout << "플레이어: " << player->_playerId << " 퇴장함을 플레이어: " << item.second->_playerId << " 알려줍니다" << endl;
				session->Send(sendBuffer);
			}
				
		}
	}
	
	return true;
}

bool Room::EnterPlayer(PlayerRef player)
{
	// 이미 player 존재함
	if (_players.find(player->_objInfo.objectid()) != _players.end())
		return false;

	_players.insert(make_pair(player->_objInfo.objectid(), player));
	player->_room.store(shared_from_this());

	// 입장 사실을 신입 플레이어에게 알린다
	{
		
		Protocol::S2C_ENTER_GAME enterGamePkt;
		enterGamePkt.mutable_player()->CopyFrom(player->_objInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
		{
			session->Send(sendBuffer);
			cout << "EnterPlayerId: " << player->GetId() << endl;
		}

	}
	// 기존에 입장한 플레이어 목록을 신입 플레이어한테 알려준다
	{
		Protocol::S2C_SPAWN spawnPkt;
		for (auto& item : _players)
		{
			if (item.second->GetId() == player->GetId()) continue;

			spawnPkt.add_objects()->CopyFrom(item.second->_objInfo);
		}

		// 현재 혼자있을 때도 전송중...
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
	// 없다면 문제가 있다
	if (_players.find(objectId) == _players.end())
		return true;

	PlayerRef player = _players[objectId];
	player->_room.store(weak_ptr<Room>());
	_players.erase(objectId);
	
	_map.ApplyLeave(player);

	// 퇴장 사실을 본인에게 알린다
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

void Room::HandleMove(PlayerRef player, const Protocol::C2S_MOVE& pkt)
{
	if (player == nullptr)
		return;

	WRITE_LOCK;
	// TODO 이동 검증
	//cout << "HandleMove posInfo state: " << pkt.posinfo().posx() <<"," << pkt.posinfo().posy() << " : " << pkt.posinfo().state() << endl;

	// 다른 좌표로 이동할 경우 , 갈 수 있는지 체크
	const auto& movePosInfo = pkt.posinfo();
	//cout << "player posInfo state: " << player->GetPosInfo().posx() << "," << player->GetPosInfo().posy() << " : " << player->GetPosInfo().state() << endl;
	if (movePosInfo.posx() != player->GetPosInfo().posx() || movePosInfo.posy() != player->GetPosInfo().posy())
	{
		if (_map.CanGo(Vector2Int{ movePosInfo.posx(), movePosInfo.posy() }) == false)
			return;
	}

	auto posInfo = player->MutablePosInfo();
	posInfo->set_state(movePosInfo.state());
	posInfo->set_movedir(movePosInfo.movedir());

	_map.ApplyMove(player, Vector2Int{ movePosInfo.posx(), movePosInfo.posy() });

	// 다른 플레이어한테도 알려준다
	Protocol::S2C_MOVE resPkt;
	resPkt.set_objectid(player->GetId());
	resPkt.mutable_posinfo()->CopyFrom(pkt.posinfo());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	Broadcast(sendBuffer, player->GetId());
}

void Room::HandleSkill(PlayerRef player, const Protocol::C2S_SKILL& pkt)
{
	if (player == nullptr)
		return;

	WRITE_LOCK;
	
	auto posInfo = player->MutablePosInfo();
	if (posInfo->state() != Protocol::CreatureState::Idle)
		return;
	
	// TODO 스킬 사용 가능 여부 체크
	

	posInfo->set_state(Protocol::CreatureState::Skill);

	int32 skillId = pkt.info().skillid();
	// 다른 플레이어한테도 알려준다
	Protocol::S2C_SKILL resPkt;
	resPkt.set_objectid(player->_objInfo.objectid());

	Protocol::SkillInfo* skill = new Protocol::SkillInfo();
	skill->set_skillid(skillId);
	resPkt.set_allocated_info(skill);
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	Broadcast(sendBuffer);
	
	if (skillId == 1) 
	{
		// TODO 데미지 판정
		Vector2Int skillPos = player->GetFrontCellPos(posInfo->movedir());
		GameObjectRef target = _map.Find(skillPos);
		if (target != nullptr)
		{
			cout << "Hit GameObject !" << endl;
		}
	}
	else if (skillId == 2)
	{
		ArrowRef arrow = ObjectManager::Instance().Add<Arrow>();
		cout << "Arrow Created : " << arrow->GetId() << endl;
		if (arrow == nullptr)
			return;
		
		arrow->_owner = player;
		arrow->MutablePosInfo()->set_state(Protocol::CreatureState::Moving);
		arrow->MutablePosInfo()->set_movedir(player->GetPosInfo().movedir());
		arrow->MutablePosInfo()->set_posx(player->GetPosInfo().posx());
		arrow->MutablePosInfo()->set_posy(player->GetPosInfo().posy());
		HandleEnterGame(arrow);
	}
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player->_objInfo.objectid() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}
