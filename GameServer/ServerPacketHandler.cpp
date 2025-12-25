#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "ObjectUtils.h"
#include "RoomManager.h"
#include "Room.h"
#include "GameSession.h"
#include "Player.h"
#include "ObjectManager.h"




PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	return false;
}

bool Handle_C2S_ENTER_GAME(PacketSessionRef& session, Protocol::C2S_ENTER_GAME& pkt)
{
	//cout << "C2S_ENTER_GAME  called!" << endl;

	PlayerRef player = ObjectManager::Instance().Add<Player>();
	player->Create(session);


	static_pointer_cast<GameSession>(session)->myPlayer.store(player);

	RoomRef room = RoomManager::Instance().Find(1);
	room->DoAsync(&Room::HandleEnterGame, static_pointer_cast<GameObject>(player));

	return true;
}

bool Handle_C2S_MOVE(PacketSessionRef& session, Protocol::C2S_MOVE& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	

	PlayerRef myPlayer = gameSession->myPlayer.load();
	if (myPlayer == nullptr)
		return false;
	RoomRef room = myPlayer->_room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleMove, myPlayer, pkt);
	//cout << myPlayer->Id() << " : C2S_MOVE(" << pkt.posinfo().posx() << ", " << pkt.posinfo().posy() << ")" << endl;
	return true;
}

bool Handle_C2S_SKILL(PacketSessionRef& session, Protocol::C2S_SKILL& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef myPlayer = gameSession->myPlayer.load();
	if (myPlayer == nullptr)
		return false;
	RoomRef room = myPlayer->_room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleSkill, myPlayer, pkt);
	//cout << myPlayer->Id() << " : C2S_SKILL" << endl;
	return true;
}
