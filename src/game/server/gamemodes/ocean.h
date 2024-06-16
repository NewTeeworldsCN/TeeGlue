#ifndef GAME_SERVER_GAMEMODES_OCEAN_H
#define GAME_SERVER_GAMEMODES_OCEAN_H

#include <game/server/gamecontroller.h>

#include <map>

class CGameControllerOcean : public IGameController
{
public:
	CGameControllerOcean(class CGameContext *pGameServer);

    // override
    bool IsFriendlyFire(int ClientID1, int ClientID2) const override;

    void Tick();
};

#endif // GAME_SERVER_GAMEMODES_OCEAN_H
