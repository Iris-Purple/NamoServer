#pragma once
#include "Protocol/Protocol.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C2S_ENTER_GAME = 1000,
	PKT_S2C_ENTER_GAME = 1001,
	PKT_S2C_LEAVE_GAME = 1002,
	PKT_S2C_SPAWN = 2000,
	PKT_S2C_DESPAWN = 2001,
	PKT_C2S_MOVE = 2002,
	PKT_S2C_MOVE = 2003,
	PKT_C2S_SKILL = 2004,
	PKT_S2C_SKILL = 2005,
};

// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_S2C_ENTER_GAME(PacketSessionRef& session, Protocol::S2C_ENTER_GAME& pkt);
bool Handle_S2C_LEAVE_GAME(PacketSessionRef& session, Protocol::S2C_LEAVE_GAME& pkt);
bool Handle_S2C_SPAWN(PacketSessionRef& session, Protocol::S2C_SPAWN& pkt);
bool Handle_S2C_DESPAWN(PacketSessionRef& session, Protocol::S2C_DESPAWN& pkt);
bool Handle_S2C_MOVE(PacketSessionRef& session, Protocol::S2C_MOVE& pkt);
bool Handle_S2C_SKILL(PacketSessionRef& session, Protocol::S2C_SKILL& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S2C_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_ENTER_GAME>(Handle_S2C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_S2C_LEAVE_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_LEAVE_GAME>(Handle_S2C_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_S2C_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_SPAWN>(Handle_S2C_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S2C_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_DESPAWN>(Handle_S2C_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S2C_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_MOVE>(Handle_S2C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S2C_SKILL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S2C_SKILL>(Handle_S2C_SKILL, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C2S_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_C2S_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::C2S_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_C2S_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::C2S_SKILL& pkt) { return MakeSendBuffer(pkt, PKT_C2S_SKILL); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};