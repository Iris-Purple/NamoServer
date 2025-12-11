#pragma once

class GameSession;
class Room;

class Player : public enable_shared_from_this<Player>
{
	
public:
	Player(int32 playerId);
	virtual ~Player();
	int32 _playerId;

public:
	unique_ptr<Protocol::PlayerInfo> info;
	weak_ptr<GameSession> session;

public:
	atomic<weak_ptr<Room>> room;
};

