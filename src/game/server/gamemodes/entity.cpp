#include "entity.h"
#include "water.h"

CModEntity::CModEntity(CGameControllerWater *pController, int Objtype, vec2 Pos, int ProximityRadius = 0) :
    CEntity(pController->GameWorld(), CGameWorld::ENTTYPE_MOD, Pos, ProximityRadius),
    m_pController(pController)
{
    m_ModType = Objtype;
}