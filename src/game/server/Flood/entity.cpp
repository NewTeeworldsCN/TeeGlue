#include "entity.h"
#include <game/server/gamemodes/water.h>

CModEntity::CModEntity(CGameControllerWater *pController, int Objtype, vec2 Pos, int ProximityRadius) :
    CEntity(pController->GameWorld(), CGameWorld::ENTTYPE_MOD, Pos, ProximityRadius),
    m_pController(pController)
{
    m_ModType = Objtype;
}