#include <game/conscience/define.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>

#include <engine/shared/config.h>

#include "conscience.h"

CGameControllerConscience::CGameControllerConscience(class CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Conscience Race"; // 'Race' for the blue color gametype
    m_GameFlags = 0;

    m_aNumSpawnPoints[0] = 0;
    m_aNumSpawnPoints[1] = 0;

	mem_zero(m_InWaterTick, sizeof(m_InWaterTick));
}

bool CGameControllerConscience::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;

	switch(Index)
	{
	case ECMapItem::ENTITY_SPAWN_JUSTICE:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	case ECMapItem::ENTITY_SPAWN_MONSTER:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	case ENTITY_ARMOR_1:
		Type = PICKUP_ARMOR;
		break;
	case ENTITY_HEALTH_1:
		Type = PICKUP_HEALTH;
		break;
	case ENTITY_WEAPON_SHOTGUN:
		Type = PICKUP_SHOTGUN;
		break;
	case ENTITY_WEAPON_GRENADE:
		Type = PICKUP_GRENADE;
		break;
	case ENTITY_WEAPON_LASER:
		Type = PICKUP_LASER;
		break;
	case ENTITY_POWERUP_NINJA:
		if(Config()->m_SvPowerups)
			Type = PICKUP_NINJA;
	}

	if(Type != -1)
	{
		new CPickup(&GameServer()->m_World, Type, Pos);
		return true;
	}

	return false;
}

bool CGameControllerConscience::CanSpawn(int Team, vec2 *pOutPos) const
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS || GameServer()->m_World.m_Paused || GameServer()->m_World.m_ResetRequested)
		return false;

	if(!m_aNumSpawnPoints[0])
        return false;
    
    *pOutPos = m_aaSpawnPoints[0][random_int(0, m_aNumSpawnPoints[0])];
    return true;
}

void CGameControllerConscience::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameDataTeam GameDataTeam;

	GameDataTeam.m_TeamscoreBlue = 0;
	GameDataTeam.m_TeamscoreRed = 0;
	
	if(!NetConverter()->SnapNewItemConvert(&GameDataTeam, this, NETOBJTYPE_GAMEDATATEAM, 0, sizeof(CNetObj_GameDataTeam), SnappingClient))
		return;
}

void CGameControllerConscience::Tick()
{
    IGameController::Tick();
}

void CGameControllerConscience::ExtraCharacterTick(class CCharacter *pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	
	if(GameServer()->Collision()->TestBox(pChr->GetPos(), vec2(pChr->GetProximityRadius() / 2, pChr->GetProximityRadius() / 2), ECCollisionFlag::COLFLAG_WATER))
	{
		pChr->Core()->m_Vel.y -= GameServer()->Tuning()->m_Gravity * 1.1f;

		m_InWaterTick[ClientID] ++;

		if(m_InWaterTick[ClientID] < Config()->m_CsBreathholdingTime * Server()->TickSpeed())
		{
			if(m_InWaterTick[ClientID] % Server()->TickSpeed() == 0)
			{
				pChr->TakeDamage(vec2(0.f, 0.1f), vec2(0.f, -0.1f), 0, -1, WEAPON_WORLD);
				GameServer()->CreateDeath(pChr->GetPos(), ClientID);
			}
		}
		else if(m_InWaterTick[ClientID] % (Server()->TickSpeed() / 2) == 0)
		{
			pChr->TakeDamage(vec2(0.f, 0.1f), vec2(0.f, -0.1f), 1, -1, WEAPON_WORLD);
			pChr->SetEmote(EMOTE_PAIN, Server()->Tick() + Server()->TickSpeed() / 2 + 1);
		}
	}
	else if(m_InWaterTick[ClientID])
	{
		GameServer()->CreateDeath(pChr->GetPos(), ClientID);

		if(m_InWaterTick[ClientID] >= Config()->m_CsBreathholdingTime * Server()->TickSpeed())
		{
			GameServer()->SendEmoticon(ClientID, EMOTICON_EYES);
			pChr->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed() / 2 + 1);
		}

		m_InWaterTick[ClientID] = 0;
	}
}