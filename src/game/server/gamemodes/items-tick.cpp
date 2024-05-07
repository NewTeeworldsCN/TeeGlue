#include <base/math.h>

#include <engine/server.h>

#include <engine/shared/config.h>

#include <game/server/entities/bomb.h>
#include <game/server/entities/character.h>
#include <game/server/entities/dropped-pickup.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>

#include <game/server/gamecontext.h>
#include <game/server/localization.h>
#include <game/server/player.h>

#include <generated/protocol.h>

#include "cace.h"

static const char* GetCaceItemName(int Type)
{
	switch (Type)
	{
		case WEAPON_BOMB: return FormatLocalize("Bomb");
		case WEAPON_WAVEBOMB: return FormatLocalize("Wave Bomb");
		case WEAPON_TELELASER: return FormatLocalize("Tele Laser");
        case WEAPON_HEALBOMB: return FormatLocalize("Heal Bomb");
	}
	return "null";
}

void CGameControllerCace::CacePickupTick(int Type, SPickupInfo *pPickupInfo)
{
	switch (Type)
	{
		case ECaceDefine::WEAPON_BOMB: PickupBombTick(pPickupInfo); break;
        case ECaceDefine::WEAPON_WAVEBOMB: PickupWaveBombTick(pPickupInfo); break;
        case ECaceDefine::WEAPON_TELELASER: PickupTeleLaserTick(pPickupInfo); break;
        case ECaceDefine::WEAPON_HEALBOMB: PickupHealBombTick(pPickupInfo); break;
	}
}

void CGameControllerCace::CaceItemTick(int Type, int ClientID)
{
	switch (Type)
	{
		case ECaceDefine::WEAPON_BOMB: ItemBombTick(ClientID); break;
		case ECaceDefine::WEAPON_WAVEBOMB: ItemWaveBombTick(ClientID); break;
        case ECaceDefine::WEAPON_TELELASER: ItemTeleLaserTick(ClientID); break;
		case ECaceDefine::WEAPON_HEALBOMB: ItemHealBombTick(ClientID); break;
	}
}

void CGameControllerCace::GetPossibleItem(int Weapon, int ClientID, std::vector<int>& vPossibleItems)
{
    vPossibleItems.clear();

	switch (Weapon)
	{
		case WEAPON_HAMMER:
		{
            if(m_aCacePlayersInventory[ClientID].count(ECaceDefine::WEAPON_BOMB) && 
                    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_BOMB])
			    vPossibleItems.push_back(ECaceDefine::WEAPON_BOMB);

            if(m_aCacePlayersInventory[ClientID].count(ECaceDefine::WEAPON_WAVEBOMB) && 
                    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_WAVEBOMB])
			    vPossibleItems.push_back(ECaceDefine::WEAPON_WAVEBOMB);

            if(m_aCacePlayersInventory[ClientID].count(ECaceDefine::WEAPON_HEALBOMB) && 
                    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_HEALBOMB])
			    vPossibleItems.push_back(ECaceDefine::WEAPON_HEALBOMB);
		}
		break;
		case WEAPON_GUN:
		{
		}
		break;
		case WEAPON_SHOTGUN:
		{
		}
		break;
		case WEAPON_GRENADE:
		{
		}
		break;
		case WEAPON_LASER:
		{
            if(m_aCacePlayersInventory[ClientID].count(ECaceDefine::WEAPON_TELELASER) && 
                    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_TELELASER])
			    vPossibleItems.push_back(ECaceDefine::WEAPON_TELELASER);
		}
		break;
		case WEAPON_NINJA:
		{
		}
		break;
	}
}

void CGameControllerCace::SendBroadcastItem(int ClientID)
{
    int Code = Server()->GetClientLanguage(ClientID);

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "> %s x%d <", Localize(Code, GetCaceItemName(m_aCacePlayerActiveItem[ClientID])), m_aCacePlayersInventory[ClientID][m_aCacePlayerActiveItem[ClientID]]);

    SendBroadcastHUD(aBuf, ClientID);
}

void CGameControllerCace::RefreshItem(int ClientID)
{
	std::vector<int> vPossibleItems;
    GetPossibleItem(GameServer()->GetPlayerChar(ClientID)->ActiveWeapon(), ClientID, vPossibleItems);

	if(vPossibleItems.size() == 0)
	{
		m_aaPlayerSwitchCase[ClientID][GameServer()->GetPlayerChar(ClientID)->ActiveWeapon()] = 0;
		m_aCacePlayerActiveItem[ClientID] = -1;

        GameServer()->SendBroadcast("", ClientID);
		return;
	}
	m_aaPlayerSwitchCase[ClientID][GameServer()->GetPlayerChar(ClientID)->ActiveWeapon()] = m_aaPlayerSwitchCase[ClientID][GameServer()->GetPlayerChar(ClientID)->ActiveWeapon()] % vPossibleItems.size();
	m_aCacePlayerActiveItem[ClientID] = vPossibleItems[m_aaPlayerSwitchCase[ClientID][GameServer()->GetPlayerChar(ClientID)->ActiveWeapon()]];

    SendBroadcastItem(ClientID);
}

void CGameControllerCace::OnPlayerSwitchItem(int Weapon, int ClientID)
{
	std::vector<int> vPossibleItems;
    GetPossibleItem(Weapon, ClientID, vPossibleItems);

	if(vPossibleItems.size() == 0)
	{
		m_aaPlayerSwitchCase[ClientID][Weapon] = 0;
		m_aCacePlayerActiveItem[ClientID] = -1;
		return;
	}
	m_aaPlayerSwitchCase[ClientID][Weapon] = m_aaPlayerSwitchCase[ClientID][Weapon] % vPossibleItems.size();
	m_aCacePlayerActiveItem[ClientID] = vPossibleItems[m_aaPlayerSwitchCase[ClientID][Weapon]];

    SendBroadcastItem(ClientID);
}

void CGameControllerCace::GetPlayerActiveItem(int Weapon, int ClientID)
{
	std::vector<int> vPossibleItems;
    GetPossibleItem(Weapon, ClientID, vPossibleItems);

	if(vPossibleItems.size() == 0)
	{
		m_aCacePlayerActiveItem[ClientID] = -1;
		return;
	}
	m_aaPlayerSwitchCase[ClientID][Weapon] = m_aaPlayerSwitchCase[ClientID][Weapon] % vPossibleItems.size();
	m_aCacePlayerActiveItem[ClientID] = vPossibleItems[m_aaPlayerSwitchCase[ClientID][Weapon]];

    if(Server()->Tick() % 25 == 0)
    {
        SendBroadcastItem(ClientID);
    }
}

// Pickup
void CGameControllerCace::PickupBombTick(SPickupInfo *pPickupInfo)
{
    for(CProjectile *pProj = (CProjectile *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj; pProj = (CProjectile *) pProj->TypeNext())
    {
        int Tick = Server()->Tick() - pProj->GetStartTick();
        float Len = distance(pProj->GetPos(Tick / (float) Server()->TickSpeed()), pPickupInfo->m_Pos);
        if(Len < 24.0f)
        {
            vec2 Vel = normalize(pProj->GetPos(Tick / (float) Server()->TickSpeed()) - 
                        pProj->GetPos((Tick - 1) / (float) Server()->TickSpeed()))
                        * 8.0f;
            GameServer()->Collision()->MovePoint(&pPickupInfo->m_Pos, &Vel, 1.0f, nullptr);

            pProj->DoBounce();
        }
    }

    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(!pChr->IsAlive())
            continue;

        float Len = distance(pChr->GetPos(), pPickupInfo->m_Pos);
        if(Len < pChr->GetProximityRadius() + 24.0f)
        {
            if(pChr->IsHooked())
            {
                pPickupInfo->m_PickedTick = Server()->Tick();
                GameServer()->CreateSound(pPickupInfo->m_Pos, SOUND_PICKUP_ARMOR);
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("You picked a bomb!"));
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("Use hammer to throw it!"));

                if(!m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()].count(ECaceDefine::WEAPON_BOMB))
                    m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_BOMB] = 0;
                m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_BOMB] ++;

                RefreshItem(pChr->GetPlayer()->GetCID());
                // no more
                return;
            }
            else if(Server()->Tick() % 25 == 0)
            {
                GameServer()->SendBroadcastLocalize(FormatLocalize("Use hook to pick this"), pChr->GetPlayer()->GetCID());
            } 
        }
    }
}

void CGameControllerCace::PickupWaveBombTick(SPickupInfo *pPickupInfo)
{
    for(CProjectile *pProj = (CProjectile *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj; pProj = (CProjectile *) pProj->TypeNext())
    {
        int Tick = Server()->Tick() - pProj->GetStartTick();
        float Len = distance(pProj->GetPos(Tick / (float) Server()->TickSpeed()), pPickupInfo->m_Pos);
        if(Len < 24.0f)
        {
            // repulsed
            for(CProjectile *pProj2 = (CProjectile *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj2; pProj2 = (CProjectile *) pProj2->TypeNext())
            {
                Tick = Server()->Tick() - pProj2->GetStartTick();
                float Len = distance(pProj2->GetPos(Tick / (float) Server()->TickSpeed()), pPickupInfo->m_Pos);
                if(Len < 160.0f)
                {
                    pProj2->DoBounce();
                }
            }

            GameServer()->CreateExplosion(pPickupInfo->m_Pos, pProj->GetOwner(), WEAPON_HAMMER, 2);

            for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
            {
                float Len = distance(pChr->GetPos(), pPickupInfo->m_Pos);
                if(Len < 160.0f)
                {
					pChr->TakeDamage(normalize(pChr->GetPos() - pPickupInfo->m_Pos) * (16.0f + 80.f / (161.0f - Len)), 
						-normalize(pChr->GetPos() - pPickupInfo->m_Pos), 
						0,
						pProj->GetOwner(),
						WEAPON_HAMMER);
                }
            }
            // no more
            break;
        }
    }

    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(!pChr->IsAlive())
            continue;

        float Len = distance(pChr->GetPos(), pPickupInfo->m_Pos);
        if(Len < pChr->GetProximityRadius() + 24.0f)
        {
            if(pChr->IsHooked())
            {
                pPickupInfo->m_PickedTick = Server()->Tick();
                GameServer()->CreateSound(pPickupInfo->m_Pos, SOUND_PICKUP_ARMOR);
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("Wow!! You picked a wave bomb!"));
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("Use hammer to throw it!"));

                if(!m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()].count(ECaceDefine::WEAPON_WAVEBOMB))
                    m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_WAVEBOMB] = 0;
                m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_WAVEBOMB] ++;

                RefreshItem(pChr->GetPlayer()->GetCID());
                // no more
                return;
            }
            else if(Server()->Tick() % 25 == 0)
            {
                GameServer()->SendBroadcastLocalize(FormatLocalize("Use hook to pick this"), pChr->GetPlayer()->GetCID());
            } 
        }
    }
}

void CGameControllerCace::PickupTeleLaserTick(SPickupInfo *pPickupInfo)
{
    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(!pChr->IsAlive())
            continue;

        float Len = distance(pChr->GetPos(), pPickupInfo->m_Pos);
        if(Len < pChr->GetProximityRadius() + 24.0f)
        {
            int Num = random_int() % 4 + 2; // 2 ~ 5
            pPickupInfo->m_PickedTick = Server()->Tick();
            GameServer()->CreateSound(pPickupInfo->m_Pos, SOUND_LASER_FIRE);
            GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                FormatLocalize("You picked tele laser x%d!!"), Num);

            if(!m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()].count(ECaceDefine::WEAPON_TELELASER))
                m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_TELELASER] = 0;
            m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_TELELASER] += Num;

            pChr->WeaponStat(WEAPON_LASER)->m_Ammo += Num;

            RefreshItem(pChr->GetPlayer()->GetCID());
            // no more
            return;
        }
    }
}

void CGameControllerCace::PickupHealBombTick(SPickupInfo *pPickupInfo)
{
    for(CProjectile *pProj = (CProjectile *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj; pProj = (CProjectile *) pProj->TypeNext())
    {
        int Tick = Server()->Tick() - pProj->GetStartTick();
        float Len = distance(pProj->GetPos(Tick / (float) Server()->TickSpeed()), pPickupInfo->m_Pos);
        if(Len < 24.0f)
        {
            vec2 Direction = normalize(pProj->GetPos((Tick - 1) / (float) Server()->TickSpeed()) -
                        pProj->GetPos(Tick / (float) Server()->TickSpeed()));

            // random_int() % 2, 0: PICKUP_HEALTH, 1: PICKUP_ARMOR
            new CDroppedPickup(&GameServer()->m_World, random_int() % 2, pPickupInfo->m_Pos, Direction);
        }
    }

    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(!pChr->IsAlive())
            continue;

        float Len = distance(pChr->GetPos(), pPickupInfo->m_Pos);
        if(Len < pChr->GetProximityRadius() + 24.0f)
        {
            if(pChr->IsHooked())
            {
                pPickupInfo->m_PickedTick = Server()->Tick();
                GameServer()->CreateSound(pPickupInfo->m_Pos, SOUND_PICKUP_HEALTH);
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("You picked a heal bomb!"));
                GameServer()->SendChatLocalize(-1, CHAT_ALL, pChr->GetPlayer()->GetCID(), 
                    FormatLocalize("Use hammer to use it!"));

                if(!m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()].count(ECaceDefine::WEAPON_HEALBOMB))
                    m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_HEALBOMB] = 0;
                m_aCacePlayersInventory[pChr->GetPlayer()->GetCID()][ECaceDefine::WEAPON_HEALBOMB] ++;

                RefreshItem(pChr->GetPlayer()->GetCID());
                // no more
                return;
            }
            else if(Server()->Tick() % 25 == 0)
            {
                GameServer()->SendBroadcastLocalize(FormatLocalize("Use hook to pick this"), pChr->GetPlayer()->GetCID());
            }
        }
    }
}
// Items
void CGameControllerCace::ItemBombTick(int ClientID)
{
    CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
    if(!pChr)
    {
        return;
    }

    if(pChr->ActiveWeapon() != WEAPON_HAMMER || !pChr->IsFired())
    {
        return;
    }

	vec2 Direction = normalize(vec2(pChr->LatestInput()->m_TargetX, pChr->LatestInput()->m_TargetY));
	vec2 ProjStartPos = pChr->GetPos() + Direction * pChr->GetProximityRadius() * 0.75f;

    new CBomb(&GameServer()->m_World, ClientID, ECaceDefine::WEAPON_BOMB, ProjStartPos, Direction);

    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_BOMB] --;

    RefreshItem(ClientID);
}

void CGameControllerCace::ItemWaveBombTick(int ClientID)
{
    CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
    if(!pChr)
    {
        return;
    }

    if(pChr->ActiveWeapon() != WEAPON_HAMMER || !pChr->IsFired())
    {
        return;
    }

	vec2 Direction = normalize(vec2(pChr->LatestInput()->m_TargetX, pChr->LatestInput()->m_TargetY));
	vec2 ProjStartPos = pChr->GetPos() + Direction * pChr->GetProximityRadius() * 0.75f;

    new CBomb(&GameServer()->m_World, ClientID, ECaceDefine::WEAPON_WAVEBOMB, ProjStartPos, Direction);

    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_WAVEBOMB] --;

    RefreshItem(ClientID);
}

void CGameControllerCace::ItemTeleLaserTick(int ClientID)
{
    CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
    if(!pChr)
    {
        return;
    }

    if(pChr->ActiveWeapon() != WEAPON_LASER || !pChr->IsFired())
    {
        return;
    }

    vec2 Direction = normalize(vec2(pChr->LatestInput()->m_TargetX, pChr->LatestInput()->m_TargetY));
    vec2 Vel = Direction * (pChr->GetProximityRadius() * 0.75f + GameServer()->Tuning()->m_LaserReach);
    vec2 TempPos = pChr->Core()->m_Pos;

    CCharacter *pHit = nullptr;
    if((pHit = GameServer()->m_World.IntersectCharacter(TempPos, TempPos + Vel, 0.f, TempPos, pChr)))
    {
        TempPos = pChr->Core()->m_Pos;
        pChr->Core()->m_Pos = pHit->Core()->m_Pos;
        pHit->Core()->m_Pos = TempPos;

        GameServer()->CreateDeath(pHit->Core()->m_Pos, ClientID);
    }
    else 
    {
        TempPos = pChr->Core()->m_Pos;
        if(!GameServer()->Collision()->IntersectLine(TempPos, TempPos + Vel, nullptr, &TempPos))
        {
            pChr->Core()->m_Pos = TempPos;
        }
    }

    GameServer()->CreateDeath(pChr->Core()->m_Pos, ClientID);

    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_TELELASER] --;

    RefreshItem(ClientID);
}

void CGameControllerCace::ItemHealBombTick(int ClientID)
{
    CCharacter *pChr = GameServer()->GetPlayerChar(ClientID);
    if(!pChr)
    {
        return;
    }

    if(pChr->ActiveWeapon() != WEAPON_HAMMER || !pChr->IsFired())
    {
        return;
    }
    // infclass
    float Angle = 2.0f*pi;
    int Num = random_int() % 3 + 3;
    for(int i = 0; i < Num; i ++)
    {
        float ShiftedAngle = Angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(Num);

        vec2 ToPos = vec2(pChr->GetPos().x + 24.0f * cos(ShiftedAngle), pChr->GetPos().y + 24.0f * sin(ShiftedAngle));
        vec2 Direction = normalize(ToPos - pChr->GetPos());

        // random_int() % 2, 0: PICKUP_HEALTH, 1: PICKUP_ARMOR
        new CDroppedPickup(&GameServer()->m_World, random_int() % 2, pChr->GetPos(), Direction);
    }

    m_aCacePlayersInventory[ClientID][ECaceDefine::WEAPON_HEALBOMB] --;

    RefreshItem(ClientID);
}