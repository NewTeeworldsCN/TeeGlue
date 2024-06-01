#ifndef GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
#define GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H

#include <game/server/gamemodes/entity.h>

class CPartsPlank : public CModEntity
{
    vec2 m_Size;
    vec2 m_Vel;
    float m_RotateVel;
    float m_Angle;

    int m_aIDs[4];
public:
	CPartsPlank(CGameControllerFlood *pController, vec2 Pos, vec2 Size, float Angle);
    ~CPartsPlank();

    void DoPhysic();

    void Tick() override;
    void Snap(int SnappingClient) override;

    vec2 Size() const { return m_Size; }
    float Angle() const { return m_Angle; }
};

#endif // GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
