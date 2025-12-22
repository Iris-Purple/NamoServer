#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "RoomManager.h"
#include "Room.h"
#include "ObjectManager.h"
#include "Player.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	cout << "GameSession::OnDisconnected" << endl;

	int32 objectId = myPlayer.load()->Id();
	RoomRef room = RoomManager::Instance().Find(1);
	if (room)
		room->DoAsync(&Room::HandleLeaveGame, objectId);

	ObjectManager::Instance().Remove(objectId);

	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId �뿪 üũ
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}