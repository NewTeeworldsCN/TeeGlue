#include <engine/shared/config.h>

#include <generated/server_data.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/cace.h>
#include <game/server/player.h>

#include "dsb.h"
#include "character.h"

CDSB::CDSB(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Direction) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_DSB, Pos)
{
	m_Owner = Owner;

	m_Vel = Direction * 8.0f;

	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CDSB::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CDSB::Tick()
{
    if(GameLayerClipped(m_Pos))
    {
        Reset();
        return;
    }

    m_Pos += m_Vel;
	if(GameServer()->Collision()->CheckPoint(m_Pos))
    {
        Reset();
        return;
    }

	float time = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()*12.0f;
    float Angle = fmodf(time*pi/2, 2.0f*pi);

    float FromShiftedAngle = Angle + 2.0*pi*static_cast<float>(0)/static_cast<float>(2);
    float ToShiftedAngle = Angle + 2.0*pi*static_cast<float>(1)/static_cast<float>(2);
    
    vec2 From = vec2((int)(m_Pos.x + 48.0f * cos(FromShiftedAngle)), (int)(m_Pos.y + 48.0f * sin(FromShiftedAngle)));
    vec2 To = vec2((int)(m_Pos.x + 48.0f * cos(ToShiftedAngle)), (int)(m_Pos.y + 48.0f * sin(ToShiftedAngle)));

    for(CCharacter *pChr = (CCharacter *) GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *) pChr->TypeNext())
    {
        if(pChr->GetPlayer()->GetCID() == m_Owner)
            continue;

        vec2 ClosestPos = closest_point_on_line(From, To, pChr->GetPos());

        float Len = distance(pChr->GetPos(), ClosestPos);
        if(Len < 2.0f + pChr->GetProximityRadius())
        {
            pChr->TakeDamage(vec2(0.f, -1.f), vec2(0.f, 1.f), 1, m_Owner, WEAPON_NINJA);
        }
    }
}

void CDSB::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	float time = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed()*8.0f;
    float Angle = fmodf(time*pi/2, 2.0f*pi);

    float FromShiftedAngle = Angle + 2.0*pi*static_cast<float>(0)/static_cast<float>(2);
    float ToShiftedAngle = Angle + 2.0*pi*static_cast<float>(1)/static_cast<float>(2);

    CNetObj_Laser Laser;

    Laser.m_StartTick = Server()->Tick();

    Laser.m_FromX = (int)(m_Pos.x + 48.0f * cos(FromShiftedAngle));
    Laser.m_FromY = (int)(m_Pos.y + 48.0f * sin(FromShiftedAngle));
    Laser.m_X = (int)(m_Pos.x + 48.0f * cos(ToShiftedAngle));
    Laser.m_Y = (int)(m_Pos.y + 48.0f * sin(ToShiftedAngle));

    if(!NetConverter()->SnapNewItemConvert(&Laser, this, NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser), SnappingClient))
        return;
}
