#include <game/server/gamecontext.h>

#include "flood.h"

CGameWorld *CGameControllerFlood::GameWorld() { return &GameServer()->m_World; }

CGameControllerFlood::CGameControllerFlood(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Flood   race";

    m_GameFlags = GAMEFLAG_SURVIVAL;
}

void CGameControllerFlood::Tick()
{
    IGameController::Tick();
}