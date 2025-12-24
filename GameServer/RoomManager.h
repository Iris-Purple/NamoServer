#pragma once

class RoomManager
{
public:
	static RoomManager& Instance()
	{
		static RoomManager instance;
		return instance;
	}

	RoomManager(const RoomManager&) = delete;
	RoomManager& operator=(const RoomManager&) = delete;

	RoomRef Add(int mapId);
	bool Remove(int32 roomId);
	RoomRef Find(int32 roomId);
	int32 GetRoomCount();
	int32 GetTotalPlayerCount();

private:
	RoomManager() = default;
	~RoomManager() = default;

	USE_LOCK;
	unordered_map<int32, RoomRef> _rooms;
	int32 _roomId = 1;
};