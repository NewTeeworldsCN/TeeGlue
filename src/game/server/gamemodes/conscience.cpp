#include <game/server/gamecontext.h>

#include "conscience.h"

CGameControllerConscience::CGameControllerConscience(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Conscience Race"; // 'Race' for the blue color gametype
    m_GameFlags = 0;
}