#include <game/server/gamemodes/define.h>
#include <game/server/gamemodes/flood.h>

#include <game/server/gamecontext.h>

#include <game/collision.h>

#include "plank.h"

CPartsPlank::CPartsPlank(CGameControllerFlood *pController, vec2 Pos, vec2 Size, float Angle) :
    CModEntity(pController, EModEntityType::MODENTTYPE_PARTS_PLANK, Pos)
{
    m_Size = Size;
    m_Angle = Angle;
	m_Vel = vec2(0.f, 0.f);
	m_RotateVel = 0.f;

	for(int i = 0; i < 4; i++)
		m_aIDs[i] = Server()->SnapNewID();
}

CPartsPlank::~CPartsPlank()
{
	for(int i = 0; i < 4; i++)
		Server()->SnapFreeID(m_aIDs[i]);
}

void CPartsPlank::DoPhysic()
{
	// physic
	m_Vel.y += GameServer()->Tuning()->m_Gravity;

	// Move
	GameServer()->Collision()->MoveBoxAngle(&m_Pos, &m_Vel, &m_Angle, m_Size, 0.5f);
}

void CPartsPlank::Tick()
{
    DoPhysic();
}

void CPartsPlank::Snap(int SnappingClient)
{
	vec2 Vertices[4] = {
		vec2(m_Pos.x - (m_Size.x / 2), m_Pos.y - (m_Size.y / 2)),
		vec2(m_Pos.x + (m_Size.x / 2), m_Pos.y - (m_Size.y / 2)),
		vec2(m_Pos.x + (m_Size.x / 2), m_Pos.y + (m_Size.y / 2)),
		vec2(m_Pos.x - (m_Size.x / 2), m_Pos.y + (m_Size.y / 2))
	};

	for(int i = 0; i < 4; i++)
		Rotate(&Vertices[i], m_Pos.x, m_Pos.y, m_Angle);

    for(int i = 0; i < 4; i++)
    {
        int j = ((i + 1 >= 4) ? 0 : (i + 1));

        CNetObj_Laser Obj;
        Obj.m_X = round_to_int(Vertices[j].x);
    	Obj.m_Y = round_to_int(Vertices[j].y);
    	Obj.m_FromX = round_to_int(Vertices[i].x);
    	Obj.m_FromY = round_to_int(Vertices[i].y);
    	Obj.m_StartTick = Server()->Tick();
    
	    if(!NetConverter()->SnapNewItemConvert(&Obj, this, NETOBJTYPE_LASER, m_aIDs[i], sizeof(CNetObj_Laser), SnappingClient))
	    	return;
    }
}