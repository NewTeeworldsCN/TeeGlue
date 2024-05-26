#ifndef GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
#define GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H

#include <game/server/gamemodes/entity.h>

class CPartsPlank : public CModEntity
{
    vec2 m_Direction;
    int m_Length;
public:
	CPartsPlank(CGameControllerWater *pController, vec2 Pos, vec2 Direction, int Length);

    void DoMove();
    void Tick() override;

    int Length() const { return m_Length; }
};

#endif // GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
