#include <game/server/gamemodes/define.h>

#include <game/server/gamecontext.h>

#include <game/collision.h>

#include "plank.h"
#include "flood.h"

CPartsPlank::CPartsPlank(CGameControllerFlood *pController, vec2 Pos, vec2 Direction, int Length) :
    CModEntity(pController, EModEntityType::MODENTTYPE_PARTS_PLANK, Pos)
{
    m_Length = Length;
    m_Direction = Direction;
}

void CPartsPlank::DoMove()
{
}

void CPartsPlank::Tick()
{
    DoMove();
}