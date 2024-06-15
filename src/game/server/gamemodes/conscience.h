#ifndef GAME_SERVER_GAMEMODES_CONSCIENCE_H
#define GAME_SERVER_GAMEMODES_CONSCIENCE_H

#include <game/server/gamecontroller.h>

class CGameControllerConscience : public IGameController
{
    vec2 m_aaSpawnPoints[2][64];
    int m_aNumSpawnPoints[2];

    int m_InWaterTick[MAX_CLIENTS];
public:
	CGameControllerConscience(class CGameContext *pGameServer);

    // override
    bool OnEntity(int Index, vec2 Pos) override;
    bool CanSpawn(int Team, vec2 *pOutPos) const override;

    void Snap(int SnappingClient) override;
    void Tick() override;

    void ExtraCharacterTick(class CCharacter *pChr) override;
};

#endif // GAME_SERVER_GAMEMODES_CONSCIENCE_H
