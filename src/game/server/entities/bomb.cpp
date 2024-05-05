#include <engine/shared/config.h>

#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/cace.h>
#include <game/server/player.h>

#include "character.h"
#include "bomb.h"
#include "projectile.h"

CBomb::CBomb(CGameWorld *pGameWorld, int Owner, int Type, vec2 Pos, vec2 Direction) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_BOMB, Pos, BombPhysSize)
{
	m_Owner = Owner;

	m_Type = Type;

	m_Vel = Direction * 16.0f;

	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CBomb::Tick()
{
	m_Vel.y += GameWorld()->m_Core.m_Tuning.m_Gravity;

	GameServer()->Collision()->MovePoint(&m_Pos, &m_Vel, 0.5f, nullptr);

	if(Server()->Tick() >= m_StartTick + Config()->m_CaceBombExplodeTime * Server()->TickSpeed())
	{
		if(m_Type == WEAPON_BOMB)
		{
			// explode
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, 20); // enough to kill a player
		}
		else if(m_Type == WEAPON_WAVEBOMB)
		{
			// repulsed
            for(CProjectile *pProj2 = (CProjectile *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PROJECTILE); pProj2; pProj2 = (CProjectile *) pProj2->TypeNext())
            {
				int Tick = Server()->Tick() - pProj2->GetStartTick();
                float Len = distance(pProj2->GetPos(Tick / (float) Server()->TickSpeed()), m_Pos);
                if(Len < 160.0f)
                {
                    pProj2->DoBounce();
                }
            }

            for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
            {
                float Len = distance(pChr->GetPos(), m_Pos);
                if(Len < 160.0f)
                {
					pChr->TakeDamage(normalize(pChr->GetPos() - m_Pos) * (32.0f + 80.f / (161.0f - Len)), 
						-normalize(pChr->GetPos() - m_Pos), 
						10,
						GetOwner(),
						WEAPON_HAMMER);
				}
            }

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
            GameServer()->CreateExplosion(m_Pos, GetOwner(), WEAPON_HAMMER, 5);
		}

		GameWorld()->DestroyEntity(this);
	}
	else if(Server()->Tick() >= m_StartTick + (Config()->m_CaceBombExplodeTime - 1) * Server()->TickSpeed())
	{
		// fast warning
		if(Server()->Tick() % 5 == 0)
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH);
	}
	else if(Server()->Tick() >= m_StartTick + (Config()->m_CaceBombExplodeTime - 2) * Server()->TickSpeed())
	{
		// warning
		if(Server()->Tick() % 10 == 0)
			GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH);
	}
	else if(Server()->Tick() % 50 == 0)
	{
		// usual warning
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH);
	}
}

void CBomb::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup Pickup;

	Pickup.m_X = round_to_int(m_Pos.x);
	Pickup.m_Y = round_to_int(m_Pos.y);
	Pickup.m_Type = PICKUP_HEALTH;

	if(m_Type == ECaceDefine::WEAPON_WAVEBOMB)
		Pickup.m_Type = PICKUP_ARMOR;

	if(!NetConverter()->SnapNewItemConvert(&Pickup, this, NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup), SnappingClient))
		return;
}
