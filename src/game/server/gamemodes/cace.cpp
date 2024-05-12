#include <base/math.h>

#include <engine/server.h>

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>

#include <game/server/gamecontext.h>
#include <game/server/localization.h>
#include <game/server/player.h>

#include <generated/protocol.h>
#include <generated/protocol6.h>

#include "cace.h"
// only Cace Entity
static int GetEntitySnapIDNum(int Type)
{
	switch (Type)
	{
		case WEAPON_BOMB:
		case WEAPON_WAVEBOMB: 
		case WEAPON_TELELASER:
		case WEAPON_HEALBOMB:
			return 5;
	}
	return 1;
}
// in seconds
static int GetEntityRespawnTime(int Type)
{
	switch (Type)
	{
		case WEAPON_BOMB:
		case WEAPON_WAVEBOMB:
			return 12;
		case WEAPON_TELELASER:
			return 20;
	}
	return 15;
}

bool CGameControllerCace::IsPicked(SPickupInfo *pPickupInfo)
{
	return pPickupInfo->m_PickedTick + GetEntityRespawnTime(pPickupInfo->m_Type) * Server()->TickSpeed() > Server()->Tick();
}
// F-DDrace
void CGameControllerCace::SendBroadcastHUD(const char *pMessage, int To)
{
	char aBuf[256] = "";
	for (int i = 0; i < ((Server()->ClientProtocol(To) == NetProtocolType::NETPROTOCOL_SIX) ? 4 : 0); i++)
		str_append(aBuf, "\n", sizeof(aBuf));

	str_append(aBuf, pMessage, sizeof(aBuf));

	if(Server()->ClientProtocol(To) == NetProtocolType::NETPROTOCOL_SIX)
		for(int i = 0; i < 128; i++)
			str_append(aBuf, " ", sizeof(aBuf));

	GameServer()->SendBroadcast(aBuf, To);
}

// TMJ: we need a catch race!!!!!
CGameControllerCace::CGameControllerCace(CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Race Catch";
}

CGameControllerCace::~CGameControllerCace()
{
	for(auto& vPickupPoints : m_vCacePickupPoints)
	{
		for(auto& PickupInfo : vPickupPoints.second)
		{
			for(auto& ID : PickupInfo.m_SnapIDs)
			{
				Server()->SnapFreeID(ID);
			}
		}
	}
}

void CGameControllerCace::Tick()
{
    IGameController::Tick();

	if(IsGamePaused())
		return;

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		m_aCacePlayerActiveItem[i] = -1;

		if(!GameServer()->m_apPlayers[i])
			continue;
		if(!GameServer()->m_apPlayers[i]->GetCharacter())
			continue;

		CCharacter *pChr = GameServer()->m_apPlayers[i]->GetCharacter();
		GetPlayerActiveItem(pChr->ActiveWeapon(), i);

		CaceItemTick(m_aCacePlayerActiveItem[i], i);
	}

	for(auto& vPickupPoints : m_vCacePickupPoints)
	{
		int Type = vPickupPoints.first;
		for(auto& PickupInfo : vPickupPoints.second)
		{
			if(IsPicked(&PickupInfo))
				continue;
			CacePickupTick(Type, &PickupInfo);
		}
	}
}

void CGameControllerCace::ResetGame()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aCacePlayersInventory[i].clear();
		if(GameServer()->m_apPlayers[i])
			GameServer()->m_apPlayers[i]->m_DeadPosActive = false;
	}

	for(auto& vPickupPoints : m_vCacePickupPoints)
	{
		for(auto& PickupInfo : vPickupPoints.second)
		{
			PickupInfo.m_PickedTick = -100 * Server()->TickSpeed();
			PickupInfo.m_Pos = PickupInfo.m_StartPos;
			PickupInfo.m_Type = random_int() % NUM_CASEITEMS;
		}
	}

	IGameController::ResetGame();
}

void CGameControllerCace::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	if(!Server()->ClientIngame(SnappingClient))
		return;
	if(!GameServer()->m_apPlayers[SnappingClient])
		return;

	for(auto& vPickupPoints : m_vCacePickupPoints)
	{
		int Type = vPickupPoints.first;
		for(auto& PickupInfo : vPickupPoints.second)
		{
			if(IsPicked(&PickupInfo))
				continue;
			SnapCacePickup(SnappingClient, Type, &PickupInfo);
		}
	}
}

void CGameControllerCace::OnCharacterSpawn(class CCharacter *pChr)
{
	m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()].clear();
	m_aCacePlayerActiveItem[pChr->GetPlayer()->GetCID()] = -1;

	for(int i = 0; i < NUM_WEAPONS; i ++)
		m_aaPlayerSwitchCase[pChr->GetPlayer()->GetCID()][i] = 0;

	IGameController::OnCharacterSpawn(pChr);

	// give start equipment
	pChr->IncreaseArmor(10);
	pChr->GiveWeapon(WEAPON_SHOTGUN, 3);
	pChr->GiveWeapon(WEAPON_GRENADE, 5);
	pChr->GiveWeapon(WEAPON_LASER, 0);

	GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), FormatLocalize("Type /s or /switch to switch your item!"));
}

void CGameControllerCace::OnPlayerConnect(CPlayer *pPlayer)
{
	int ClientID = pPlayer->GetCID();
	pPlayer->Respawn();

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), pPlayer->GetTeam());
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// update game info
	UpdateGameInfo(ClientID);

	if(IsGameRunning())
	{
		GameServer()->SendChatLocalize(-1, CHAT_ALL, ClientID, FormatLocalize("Race was already start! Hurry up! Be the first finisher!\nIf someone around you! Kill him!\nDo not let others kill you!"));
		GameServer()->SendBroadcastLocalize(FormatLocalize("Race was already start! Hurry up! Be the first finisher!\nIf someone around you! Kill him!\nDo not let others kill you!"), ClientID);
	}
}

bool CGameControllerCace::DoWincheckMatch()
{
	int PlayerCount = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
			(!GameServer()->m_apPlayers[i]->m_RespawnDisabled ||
			(GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
			++PlayerCount;
	}

	if(PlayerCount == 0 || (m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
	{
		GameServer()->SendChatLocalize(-1, CHAT_ALL, -1, FormatLocalize("Game aborted"));
		EndMatch();
		return true;
	}

    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(!pChr->IsAlive())
            continue;
		if(GameServer()->Collision()->CheckPoint(pChr->GetPos(), CCollision::COLFLAG_FINISH))
		{
			CNetMsg_Sv_RaceFinish Msg;

			Msg.m_ClientID = pChr->GetPlayer()->GetCID();
			Msg.m_Diff = 0;
			Msg.m_RecordPersonal = 0;
			Msg.m_RecordServer = 0;
			Msg.m_Time = (float)(Server()->Tick() - m_GameStartTick) / ((float)Server()->TickSpeed()) * 1000;

			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

			EndMatch();
			return true;
		}
	}

	return false;
}

bool CGameControllerCace::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int CaceType = -1;

	switch(Index)
	{
	case ENTITY_SPAWN:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	case ENTITY_SPAWN_RED:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	case ENTITY_SPAWN_BLUE:
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
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
	case ENTITY_RANDOM_CACE:
		CaceType = random_int() % NUM_CASEITEMS;
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
	if(CaceType != -1)
	{
		SPickupInfo PickupInfo;
		PickupInfo.m_Pos = Pos;
		PickupInfo.m_StartPos = Pos;
		PickupInfo.m_Type = CaceType;
		PickupInfo.m_PickedTick = -100 * Server()->TickSpeed();
		for(int i = 0; i < GetEntitySnapIDNum(CaceType); i ++)
			PickupInfo.m_SnapIDs.push_back(Server()->SnapNewID());
		m_vCacePickupPoints[CaceType].push_back(PickupInfo);

		return true;
	}

	return false;
}

void CGameControllerCace::ComSwitch(IConsole::IResult *pResult, void *pContext)
{
	CCommandManager::SCommandContext *pComContext = (CCommandManager::SCommandContext *)pContext;
	CGameControllerCace *pSelf = (CGameControllerCace *) pComContext->m_pContext;
	int ClientID = pComContext->m_ClientID;

	if(!pSelf->GameServer()->GetPlayerChar(ClientID))
		return;

	pSelf->m_aaPlayerSwitchCase[ClientID][pSelf->GameServer()->GetPlayerChar(ClientID)->ActiveWeapon()] ++;

	pSelf->OnPlayerSwitchItem(pSelf->GameServer()->GetPlayerChar(ClientID)->ActiveWeapon(), ClientID);
}

void CGameControllerCace::RegisterChatCommands(CCommandManager *pManager)
{
	pManager->AddCommand("s", "Switch your item", "", ComSwitch, this);
	pManager->AddCommand("switch", "Switch your item", "", ComSwitch, this);
}
