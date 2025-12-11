#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"


//RoomRef GRoom = make_shared<Room>();

Room::Room(int32 roomId) : _roomId(roomId) { }

Room::~Room() {	}

bool Room::HandleEnterPlayerLocked(PlayerRef player)
{
	WRITE_LOCK;

	if (EnterPlayer(player) == false)
		return false;

	auto posInfo = new Protocol::PositionInfo();
	posInfo->set_state(Protocol::CreatureState::Idle);
	posInfo->set_movedir(Protocol::MoveDir::None);
	posInfo->set_posx(0);
	posInfo->set_posy(0);
	player->info->set_allocated_posinfo(posInfo);
	
	// 입장 사실을 신입 플레이어에게 알린다
	{
		Protocol::S2C_ENTER_GAME enterGamePkt;
		
		Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
		playerInfo->CopyFrom(*player->info);
		enterGamePkt.set_allocated_player(playerInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
		{
			session->Send(sendBuffer);
			cout << "신입 플레이어 입장 : " << player->_playerId << endl;
		}
			
	}
	// 기존에 입장한 플레이어 목록을 신입 플레이어한테 알려준다
	{
		Protocol::S2C_SPAWN spawnPkt;
		for (auto& item : _players)
		{
			if (item.second->_playerId == player->_playerId) continue;
				
			Protocol::PlayerInfo* playerInfo = spawnPkt.add_players();
			cout << "기존 플레이어 id: " << item.second->_playerId << endl;
			playerInfo->CopyFrom(*item.second->info);
		}
		
		// 현재 혼자있을 때도 전송중...
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->session.lock())
		{
			cout << "기존 플레이어 id를  신규유저한테 알려줬어요: " << player->_playerId << endl;
			session->Send(sendBuffer);
		}
			
	}
	// 입장 사실을 다른 플레이어에게 알린다
	{
		Protocol::S2C_SPAWN spawnPkt;
		Protocol::PlayerInfo* playerInfo = spawnPkt.add_players();
		playerInfo->CopyFrom(*player->info);
		cout << "입장 사실을 다른 플레이어에게 알린다" << endl;
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		for (auto& item : _players)
		{
			if (item.second->_playerId == player->_playerId) continue;
			if (auto session = item.second->session.lock())
				session->Send(sendBuffer);
		}
	}

	return true;
}

bool Room::HandleLeavePlayerLocked(PlayerRef player)
{
	if (player == nullptr)
		return false;

	WRITE_LOCK;

	const uint64 objectId = player->info->playerid();
	if (LeavePlayer(objectId) == false)
		return false;

	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	{
		Protocol::S2C_LEAVE_GAME leaveGamePkt;
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
		{
			cout << "LeaveGame Packet 전달: " << player->_playerId << endl;
			session->Send(sendBuffer);
		}
			
	}

	// 퇴장 사실을 알린다
	{
		Protocol::S2C_DESPAWN despawnPkt;
		despawnPkt.add_playerids(player->info->playerid());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		for (auto& item : _players)
		{
			if (item.second->_playerId == player->_playerId) continue;
		
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
	if (_players.find(player->info->playerid()) != _players.end())
		return false;

	_players.insert(make_pair(player->info->playerid(), player));
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
		if (player->info->playerid() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}
