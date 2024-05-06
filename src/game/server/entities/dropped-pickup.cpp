#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include "character.h"
#include "dropped-pickup.h"

CDroppedPickup::CDroppedPickup(CGameWorld *pGameWorld, int Type, vec2 Pos, vec2 Direction) :
    CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, DroppedPickupPhysSize)
{
	m_Type = Type;
    m_Vel = Direction * 8.0f;

	GameWorld()->InsertEntity(this);
}

void CDroppedPickup::Tick()
{
	m_Vel.y += GameWorld()->m_Core.m_Tuning.m_Gravity;

	GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(GetProximityRadius(), GetProximityRadius()), 0.25f, nullptr);

	// Check if a player intersected us
	CCharacter *pChr = (CCharacter *)GameWorld()->ClosestEntity(m_Pos, 20.0f, CGameWorld::ENTTYPE_CHARACTER, 0);
	if(pChr && pChr->IsAlive() && GameServer()->m_pController->PlayerPickable(pChr->GetPlayer()->GetCID()))
	{
		// player picked us up, is someone was hooking us, let them go
		bool Picked = false;
		switch (m_Type)
		{
			case PICKUP_HEALTH:
				if(pChr->IncreaseHealth(1))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
				}
				break;

			case PICKUP_ARMOR:
				if(pChr->IncreaseArmor(1))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case PICKUP_GRENADE:
				if(pChr->GiveWeapon(WEAPON_GRENADE, g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_GRENADE);
				}
				break;
			case PICKUP_SHOTGUN:
				if(pChr->GiveWeapon(WEAPON_SHOTGUN, g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_SHOTGUN);
				}
				break;
			case PICKUP_LASER:
				if(pChr->GiveWeapon(WEAPON_LASER, g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Maxammo))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
					if(pChr->GetPlayer())
						GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), WEAPON_LASER);
				}
				break;

			case PICKUP_NINJA:
				{
					Picked = true;
					// activate ninja on target player
					pChr->GiveNinja();

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}

			default:
				break;
		};

		if(Picked)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

            GameWorld()->DestroyEntity(this);
		}
	}
}

void CDroppedPickup::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup Pickup;

	Pickup.m_X = round_to_int(m_Pos.x);
	Pickup.m_Y = round_to_int(m_Pos.y);
	Pickup.m_Type = m_Type;

	if(!NetConverter()->SnapNewItemConvert(&Pickup, this, NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup), SnappingClient))
		return;
}
