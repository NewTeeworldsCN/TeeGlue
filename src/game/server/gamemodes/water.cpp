#include <game/server/gamecontext.h>

#include "water.h"

CGameWorld *CGameControllerWater::GameWorld() { return &GameServer()->m_World; }

CGameControllerWater::CGameControllerWater(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "D-Water";

    m_GameFlags = GAMEFLAG_SURVIVAL;
}

void CGameControllerWater::Tick()
{
    IGameController::Tick();
}