#pragma once

class PlayerManager
{
public:
	static PlayerManager& Instance()
	{
		static PlayerManager instance;
		return instance;
	}

	PlayerManager(const PlayerManager&) = delete;
	PlayerManager& operator=(const PlayerManager&) = delete;

	PlayerRef Add();
	bool Remove(int32 playerId);
	PlayerRef Find(int32 playerId);

private:
	PlayerManager() = default;
	~PlayerManager() = default;

	USE_LOCK;
	unordered_map<int32, PlayerRef> _players;
	int32 _playerId = 1;
};

