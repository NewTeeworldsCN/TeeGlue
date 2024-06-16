/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

CGameControllerDM::CGameControllerDM(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "OceanDM";
}

void CGameControllerDM::Tick()
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