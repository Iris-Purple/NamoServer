#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

// DummyClient.cpp에서 정의된 함수
extern void AddActiveSession(PacketSessionRef session);

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S2C_ENTER_GAME(PacketSessionRef& session, Protocol::S2C_ENTER_GAME& pkt)
{
	cout << "client recv S2C_ENTER_GAME" << endl;

	// 활성 세션 목록에 등록 (main 스레드에서 랜덤 딜레이로 Move 패킷 전송)
	AddActiveSession(session);

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

