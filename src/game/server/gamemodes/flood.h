#ifndef GAME_SERVER_GAMEMODES_WATER_H
#define GAME_SERVER_GAMEMODES_WATER_H

#include <game/server/gamecontroller.h>

class CGameControllerFlood : public IGameController
{
public:
	CGameControllerFlood(class CGameContext *pGameServer);

	class CGameWorld *GameWorld();

    // IGameController override
	void Tick() override;
};

#endif // GAME_SERVER_GAMEMODES_WATER_H
