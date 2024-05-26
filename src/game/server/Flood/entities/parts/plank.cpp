#include <game/server/Flood/define.h>

#include <game/server/gamecontext.h>

#include <game/collision.h>

#include "plank.h"

CPartsPlank::CPartsPlank(CGameControllerWater *pController, vec2 Pos, vec2 Direction, vec2 Size, b2World* World) :
    CModEntity(pController, EModEntityType::MODENTTYPE_PARTS_PLANK, Pos)
{
    m_Size = Size;
    m_Direction = Direction;

    // the box
 	b2BodyDef BodyDef;
	BodyDef.position = b2Vec2(Pos.x / SCALE, Pos.y / SCALE);
	BodyDef.type = b2_dynamicBody;
	BodyDef.angle = 0;
	m_pBody = GameServer()->m_pB2World->CreateBody(&BodyDef);

	b2PolygonShape Shape;
	Shape.SetAsBox(Size.x / 2 / SCALE, Size.y / 2 / SCALE);
	b2FixtureDef FixtureDef;
	FixtureDef.density = 1.0f;
	FixtureDef.shape = &Shape;
	m_pBody->CreateFixture(&FixtureDef);

    for (int i = 0; i < 4; i++)
        m_aID[i] = Server()->SnapNewID();
    
    GameWorld()->InsertEntity(this);
}

CPartsPlank::~CPartsPlank()
{
    for (int i = 0; i < 4; i++)
        Server()->SnapFreeID(m_aID[i]);

    if (GameServer()->m_pB2World)
		GameServer()->m_pB2World->DestroyBody(m_pBody);

	for (unsigned i=0; i<GameServer()->m_vpB2Bodies.size(); i++)
	{
		if (GameServer()->m_vpB2Bodies[i] == this)
		{
			GameServer()->m_vpB2Bodies.erase(GameServer()->m_vpB2Bodies.begin() + i);
			break;
		}
	}
}

// TODO:
// The move function is running in the Box2D.
// So remove this function if No need.
void CPartsPlank::DoMove()
{
    m_Pos = vec2(GetBody()->GetPosition().x * SCALE, GetBody()->GetPosition().y * SCALE);
}

void CPartsPlank::Tick()
{
    DoMove();
    if (GameLayerClipped(m_Pos))
		GameWorld()->RemoveEntity(this);
}

void CPartsPlank::Snap(int SnappingClient)
{
    vec2 pos(m_pBody->GetPosition().x * SCALE, m_pBody->GetPosition().y * SCALE);
	vec2 vertices[4] = {
		vec2(pos.x - (m_Size.x/2), pos.y - (m_Size.y/2)),
		vec2(pos.x + (m_Size.x/2), pos.y - (m_Size.y/2)),
		vec2(pos.x + (m_Size.x/2), pos.y + (m_Size.y/2)),
		vec2(pos.x - (m_Size.x/2), pos.y + (m_Size.y/2))
	};

    float Angle = GetBody()->GetAngle(); // radians

	for (int i=0; i<4; i++)
		Rotate(&vertices[i], pos.x, pos.y, Angle);

    for (int i = 0; i < 4; i++)
    {
        CNetObj_Laser Obj;
        int I = ((i+1>=4) ? 0 : (i+1));
        Obj.m_X = round_to_int(vertices[I].x);
    	Obj.m_Y = round_to_int(vertices[I].y);
    	Obj.m_FromX = round_to_int(vertices[i].x);
    	Obj.m_FromY = round_to_int(vertices[i].y);
    	Obj.m_StartTick = Server()->Tick();
    
	    if(!NetConverter()->SnapNewItemConvert(&Obj, this, NETOBJTYPE_LASER, m_aID[i], sizeof(CNetObj_Laser), SnappingClient))
	    	return;
    }
}