#include "pch.h"
#include "ClientPacketHandler.h"
#include "GameSession.h"
#include "BufferReader.h"
#include "GameSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S2C_ENTER_GAME(GameSessionRef& session, Protocol::S2C_ENTER_GAME& pkt)
{
	//cout << "client recv S2C_ENTER_GAME" << endl;

	// 활성 세션 목록에 등록 (main 스레드에서 랜덤 딜레이로 Move 패킷 전송) 
	AddActiveSession(session);

	return true;
}
bool Handle_S2C_PING(GameSessionRef& session, Protocol::S2C_PING& pkt)
{
	Protocol::C2S_PONG pong;
	pong.set_timestamp(pkt.timestamp());

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pong);
	session->Send(sendBuffer);
	return true;
}
bool Handle_S2C_LEAVE_GAME(GameSessionRef& session, Protocol::S2C_LEAVE_GAME& pkt)
{
	return true;
}

bool Handle_S2C_SPAWN(GameSessionRef& session, Protocol::S2C_SPAWN& pkt)
{
	//cout << "client recv S2C_SPAWN" << endl;
	return true;
}

bool Handle_S2C_DESPAWN(GameSessionRef& session, Protocol::S2C_DESPAWN& pkt)
{
	return true;
}

bool Handle_S2C_MOVE(GameSessionRef& session, Protocol::S2C_MOVE& pkt)
{
	return true;
}

bool Handle_S2C_SKILL(GameSessionRef& session, Protocol::S2C_SKILL& pkt)
{
	return true;
}

bool Handle_S2C_CHANGE_HP(GameSessionRef& session, Protocol::S2C_CHANGE_HP& pkt)
{
	return true; 
}
bool Handle_S2C_DIE(GameSessionRef& session, Protocol::S2C_DIE& pkt)
{
	return true;
}

