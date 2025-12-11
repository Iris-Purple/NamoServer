#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "ObjectUtils.h"
#include "RoomManager.h"
#include "Room.h"
#include "GameSession.h"
#include "Player.h"
#include "PlayerManager.h"




PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	//GRoom->HandleEnterPlayerLocked(make_shared<Player>());
	// TODO : Log
	return false;
}

bool Handle_C2S_ENTER_GAME(PacketSessionRef& session, Protocol::C2S_ENTER_GAME& pkt)
{
	cout << "C2S_ENTER_GAME  called!" << endl;

	PlayerRef player = PlayerManager::Instance().Add();
	static_pointer_cast<GameSession>(session)->myPlayer.store(player);

	player->session = static_pointer_cast<GameSession>(session);
	player->info->set_playerid(player->_playerId);
	player->info->set_name("John");
	
	RoomRef room = RoomManager::Instance().Find(1);
	room->HandleEnterPlayerLocked(player);

	return true;
}

bool Handle_C2S_MOVE(PacketSessionRef& session, Protocol::C2S_MOVE& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	cout << "C2S_MOVE (" << pkt.posinfo().posx() << ", " << pkt.posinfo().posy() << ")" << endl;

	PlayerRef myPlayer = gameSession->myPlayer.load();
	if (myPlayer == nullptr)
		return false;
	RoomRef myRoom = myPlayer->room.load().lock();
	if (myRoom == nullptr)
		return false;

	// TODO : 검증

	// 일단 서버에서 좌표 이동
	// TODO myPlayer 이동관련 매서드 추가해서 관리하자
	myPlayer->info->mutable_posinfo()->CopyFrom(pkt.posinfo());


	// 다른 플레이어한테도 알려준다
	Protocol::S2C_MOVE resPkt;
	resPkt.set_playerid(gameSession->myPlayer.load()->_playerId);
	resPkt.mutable_posinfo()->CopyFrom(pkt.posinfo());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(resPkt);
	// 나를 제외한 다른 유저에게 이동패킷 전달
	myRoom->Broadcast(sendBuffer, myPlayer->_playerId);

	return true;
}

bool Handle_C2S_SKILL(PacketSessionRef& session, Protocol::C2S_SKILL& pkt)
{
	return true;
}

/*
bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	// TODO : DB 에서 Account 정보 가져온다
	// TODO : DB에서 유저 정보 가져온다
	Protocol::S_LOGIN loginPkt;

	for (int32 i = 0; i < 3; i++)
	{
		Protocol::PlayerInfo* player = loginPkt.add_players();
		player->set_x(Utils::GetRandom(0.f, 100.f));
		player->set_y(Utils::GetRandom(0.f, 100.f));
		player->set_z(Utils::GetRandom(0.f, 100.f));
		player->set_yaw(Utils::GetRandom(0.f, 100.f));
	}
	loginPkt.set_success(true);

	SEND_PACKET(loginPkt);

	return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	// 플레이어 생성
	PlayerRef player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));

	// 방에 입장
	GRoom->HandleEnterPlayerLocked(player);
	return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->HandleLeavePlayerLocked(player);
	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	return true;
}
*/