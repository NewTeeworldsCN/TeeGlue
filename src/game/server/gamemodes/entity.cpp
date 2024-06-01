#include "entity.h"
#include "flood.h"

CModEntity::CModEntity(CGameControllerFlood *pController, int Objtype, vec2 Pos, int ProximityRadius) :
    CEntity(pController->GameWorld(), CGameWorld::ENTTYPE_MOD, Pos, ProximityRadius),
    m_pController(pController)
{
    m_ModType = Objtype;

    GameWorld()->InsertEntity(this);
}