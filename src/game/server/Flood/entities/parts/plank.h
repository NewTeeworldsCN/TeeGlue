#ifndef GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
#define GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H

#include <box2d/box2d.h>
#include <game/server/Flood/entity.h>

class CPartsPlank : public CModEntity
{
    vec2 m_Direction;
    vec2 m_Size;
public:
	CPartsPlank(CGameControllerWater *pController, vec2 Pos, vec2 Direction, vec2 Size, b2World* World);
    ~CPartsPlank();

    void DoMove();
    void Tick() override;
    void Snap(int SnappingClient) override;

    vec2 Size() const { return m_Size; }

    b2Body* GetBody() { return m_pBody; }

private: // TODO: Move to CBox2DEntity
	b2Body* m_pBody;

    int m_aID[4];
};

#endif // GAME_SERVER_GAMEMODES_ENTITIES_PLANK_H
