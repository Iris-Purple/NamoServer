#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"


RoomRef GRoom = make_shared<Room>();

Room::Room()
{
}

Room::~Room()
{
	
}

bool Room::HandleEnterPlayerLocked(PlayerRef player)
{
	WRITE_LOCK;

	bool success = EnterPlayer(player);

	player->playerInfo->set_posx(0);
	player->playerInfo->set_posy(0);	
	// 입장 사실을 신입 플레이어에게 알린다
	{
		Protocol::S2C_ENTER_GAME enterGamePkt;
		
		Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
		playerInfo->CopyFrom(*player->playerInfo);
		enterGamePkt.set_allocated_player(playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// 입장 사실을 다른 플레이어에게 알린다
	{
		/*
		Protocol::S_SPAWN spawnPkt;

		Protocol::PlayerInfo* playerInfo = spawnPkt.add_players();
		playerInfo->CopyFrom(*player->playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		this->Broadcast(sendBuffer, player->playerInfo->object_id());
		*/
	}

	// 기존에 입장한 플레이어 목록을 신입 플레이어한테 알려준다
	{
		/*
		Protocol::S_SPAWN spawnPkt;
		
		for (auto& item : _players)
		{
			Protocol::PlayerInfo* playerInfo = spawnPkt.add_players();
			playerInfo->CopyFrom(*item.second->playerInfo);
		}

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
		*/
	}

	return success;
}

bool Room::HandleLeavePlayerLocked(PlayerRef player)
{
	return true;
	/*
	if (player == nullptr)
		return false;

	WRITE_LOCK;

	const uint64 objectId = player->playerInfo->object_id();
	bool success = LeavePlayer(objectId);
	
	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// 퇴장 사실을 알린다
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}
	
	return success;
	*/
}

bool Room::EnterPlayer(PlayerRef player)
{
	// 이미 player 존재함
	if (_players.find(player->playerInfo->playerid()) != _players.end())
		return false;

	_players.insert(make_pair(player->playerInfo->playerid(), player));
	player->room.store(shared_from_this());
	return true;
}

bool Room::LeavePlayer(uint64 objectId)
{
	// 없다면 문제가 있다
	if (_players.find(objectId) == _players.end())
		return true;

	PlayerRef player = _players[objectId];
	player->room.store(weak_ptr<Room>());
	_players.erase(objectId);
	
	return true;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player->playerInfo->playerid() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}
