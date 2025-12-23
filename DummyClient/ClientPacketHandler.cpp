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
	
	this_thread::sleep_for(chrono::milliseconds(5000));
	cout << "client send C2S_MOVE" << endl;
	Protocol::C2S_MOVE resPkt;
	Protocol::PositionInfo* posInfo = resPkt.mutable_posinfo();
	posInfo->set_state(Protocol::CreatureState::Moving);
	posInfo->set_movedir(Protocol::MoveDir::Left);
	posInfo->set_posx(10);
	posInfo->set_posy(10);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(resPkt);
	session->Send(sendBuffer);
	
	///////////
	this_thread::sleep_for(chrono::milliseconds(5000));
	cout << "client send C2S_SKILL" << endl;
	Protocol::C2S_SKILL res2Pkt;
	Protocol::SkillInfo* info = res2Pkt.mutable_info();
	info->set_skillid(1);
	sendBuffer = ClientPacketHandler::MakeSendBuffer(res2Pkt);
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
	return true;
}

bool Handle_S2C_DESPAWN(PacketSessionRef& session, Protocol::S2C_DESPAWN& pkt)
{
	return true;
}

bool Handle_S2C_MOVE(PacketSessionRef& session, Protocol::S2C_MOVE& pkt)
{
	return true;
}

bool Handle_S2C_SKILL(PacketSessionRef& session, Protocol::S2C_SKILL& pkt)
{
	return true;
}

bool Handle_S2C_CHANGE_HP(PacketSessionRef& session, Protocol::S2C_CHANGE_HP& pkt)\
{
	return true;
}
bool Handle_S2C_DIE(PacketSessionRef& session, Protocol::S2C_DIE& pkt)
{
	return true;
}

