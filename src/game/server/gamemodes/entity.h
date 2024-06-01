#ifndef GAME_SERVER_GAMEMODES_ENTITY_H
#define GAME_SERVER_GAMEMODES_ENTITY_H

#include <game/server/entity.h>

class CModEntity : public CEntity
{
    class CGameControllerFlood *m_pController;

    int m_ModType;
public:
	CModEntity(CGameControllerFlood *pController, int Objtype, vec2 Pos, int ProximityRadius = 0);

	class CGameControllerFlood *Controller() { return m_pController; }
    int ModType() const { return m_ModType; }
};

#endif // GAME_SERVER_GAMEMODES_ENTITY_H
