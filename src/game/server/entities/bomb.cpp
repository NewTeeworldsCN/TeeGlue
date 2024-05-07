#include <engine/shared/config.h>

#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/cace.h>
#include <game/server/player.h>

#include "bomb.h"
#include "character.h"
#include "laser.h"
#include "projectile.h"

CBomb::CBomb(CGameWorld *pGameWorld, int Owner, int Type, vec2 Pos, vec2 Direction) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_BOMB, Pos, BombPhysSize)
{
	m_Owner = Owner;

	m_Type = Type;

	m_Vel = Direction * 48.0f;

	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CBomb::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CBomb::Tick()
{
	// get ground state
	const bool Grounded =
		GameServer()->Collision()->CheckPoint(m_Pos.x + GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5)
		|| GameServer()->Collision()->CheckPoint(m_Pos.x - GetProximityRadius() / 2, m_Pos.y + GetProximityRadius() / 2 + 5);

	float Friction = Grounded ? GameWorld()->m_Core.m_Tuning.m_GroundFriction : GameWorld()->m_Core.m_Tuning.m_AirFriction;
	
	if(!Grounded)
		m_Vel.y += GameWorld()->m_Core.m_Tuning.m_Gravity;
	else
		m_Vel.y = 0;

	GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(GetProximityRadius(), GetProximityRadius()), 0.25f, nullptr);

	m_Vel.x *= Friction;

	if(Server()->Tick() >= m_StartTick + Config()->m_CaceBombExplodeTime * Server()->TickSpeed())
	{
		if(m_Type == WEAPON_BOMB)
		{
			// explode
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_HAMMER, 20); // enough to kill a player

			// infclass
			float AngleStep = 2.0f * pi / 12;
			float RandomShift = random_float() * 2.0f * pi;
			for(int i = 0; i < 12; i ++)
			{
				new CLaser(GameWorld(), m_Pos, direction(RandomShift + AngleStep * i), GameServer()->Tuning()->m_LaserReach, m_Owner);
			}
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
			// infclass
			float AngleStep = 2.0f * pi / 6;
			float RandomShift = random_float() * 2.0f * pi;
			for(int i = 0; i < 6; i ++)
			{
				new CLaser(GameWorld(), m_Pos, direction(RandomShift + AngleStep * i), GameServer()->Tuning()->m_LaserReach, m_Owner);
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
