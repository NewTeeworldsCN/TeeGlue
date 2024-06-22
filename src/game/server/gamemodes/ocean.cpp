#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

#include "ocean.h"

CGameControllerOcean::CGameControllerOcean(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Ocean Race"; // Race
    m_GameFlags = 0;
}

bool CGameControllerOcean::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2)
		return false;

    if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
        return false;

    return true; // this is a Race mod.
}

void CGameControllerOcean::Tick()
{
    for(int i = 0; i < MAX_CLIENTS; i ++)
    {
        if(!GameServer()->GetPlayerChar(i))
            continue;
        
        CCharacter *pChr = GameServer()->GetPlayerChar(i);

        if(GameServer()->Collision()->TestBox(pChr->GetPos(), vec2(pChr->GetProximityRadius(), pChr->GetProximityRadius()), CCollision::TILEFLAG_CANNON))
        {
            pChr->SetWeapon(WEAPON_HAMMER);
            pChr->SetQueuedWeapon(WEAPON_HAMMER);
            if(pChr->IsAttacked())
            {
                vec2 TargetPos = pChr->GetPos() + vec2(pChr->Core()->m_Input.m_TargetX, pChr->Core()->m_Input.m_TargetY);
                GameServer()->CreateExplosion(TargetPos, i, WEAPON_GRENADE, 12, -1);
                GameServer()->CreateSound(TargetPos, SOUND_GRENADE_EXPLODE);
                GameServer()->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
                pChr->SetReloadTimer(1.5f * Server()->TickSpeed()); // in tick
            }
        }
    }
}