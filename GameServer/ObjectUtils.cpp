#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "GameSession.h"

atomic<int32> ObjectUtils::player_idGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{

	/*
	// ID »ý¼º±â
	const int32 newId = player_idGenerator.fetch_add(1);

	PlayerRef player = make_shared<Player>();
	player->playerInfo->set_playerid(newId);
	player->session = session;
	session->player.store(player);

	return player;
	*/
	return nullptr;
}
