#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S2C_ENTER_GAME(PacketSessionRef& session, Protocol::S2C_ENTER_GAME& pkt)
{
	cout << "client recv S2C_ENTER_GAME" << endl;
	auto p = pkt.player();
	cout << "player name: " << p.name() << endl;
	cout << "player x, y: " << p.posinfo().posx() << ", " << p.posinfo().posy() << endl;

	
	this_thread::sleep_for(chrono::milliseconds(1000));
	Protocol::C2S_MOVE resPkt;
	Protocol::PositionInfo* posInfo = resPkt.mutable_posinfo();
	posInfo->set_state(Protocol::CreatureState::Moving);
	posInfo->set_movedir(Protocol::MoveDir::Left);
	posInfo->set_posx(10);
	posInfo->set_posy(10);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);
	
	return true;
}
bool Handle_S2C_LEAVE_GAME(PacketSessionRef& session, Protocol::S2C_LEAVE_GAME& pkt)
{
	return true;
}

bool Handle_S2C_SPAWN(PacketSessionRef& session, Protocol::S2C_SPAWN& pkt)
{
	cout << "client recv S2C_SPAWN" << endl;
	return false;
}

bool Handle_S2C_DESPAWN(PacketSessionRef& session, Protocol::S2C_DESPAWN& pkt)
{
	return false;
}

bool Handle_S2C_MOVE(PacketSessionRef& session, Protocol::S2C_MOVE& pkt)
{
	return false;
}

