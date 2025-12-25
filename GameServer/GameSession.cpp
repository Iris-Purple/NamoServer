#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "RoomManager.h"
#include "Room.h"
#include "ObjectManager.h"
#include "Player.h"
#include "ServerMonitor.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	cout << "GameSession::OnDisconnected" << endl;

	int32 objectId = myPlayer.load()->_objectId;
	RoomRef room = RoomManager::Instance().Find(1);
	if (room)
		room->DoAsync(&Room::HandleLeaveGame, objectId);

	ObjectManager::Instance().Remove(objectId);

	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerMonitor::Instance().OnPacketRecv(len);
	ServerMonitor::Instance().OnPacketProcessStart();

	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	ServerPacketHandler::HandlePacket(session, buffer, len);

	ServerMonitor::Instance().OnPacketProcessEnd();
}

void GameSession::OnSend(int32 len)
{
	ServerMonitor::Instance().OnPacketSend(len);
}