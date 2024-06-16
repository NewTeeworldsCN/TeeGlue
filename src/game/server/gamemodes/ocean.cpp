#include <engine/shared/config.h>

#include <game/server/gamecontext.h>

#include "ocean.h"

CGameControllerOcean::CGameControllerOcean(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Ocean  Race"; // Race for color gametype
    m_GameFlags = 0;
}

bool CGameControllerOcean::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2)
		return false;

    if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
        return false;

    return true; // this is a PvE mod.
}