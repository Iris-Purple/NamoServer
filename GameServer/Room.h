#pragma once

class Room : public enable_shared_from_this<Room>
{
public:
	Room(int32 roomId);
	virtual ~Room();

	bool HandleEnterPlayerLocked(PlayerRef player);
	bool HandleLeavePlayerLocked(PlayerRef player);

private:
	bool EnterPlayer(PlayerRef player);
	bool LeavePlayer(uint64 objectId);

	USE_LOCK;

public:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
private:
	unordered_map<uint64, PlayerRef> _players;
	int _roomId;
};

//extern RoomRef GRoom;
