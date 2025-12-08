#pragma once

class ObjectUtils
{
public:
	static PlayerRef CreatePlayer(GameSessionRef session);

private:
	static atomic<int32> player_idGenerator;
};

