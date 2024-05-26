/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, vec2(round_to_int(Pos.x), round_to_int(Pos.y)))
{
	m_Type = Type;
	m_Direction.x = round_to_int(Dir.x*100.0f) / 100.0f;
	m_Direction.y = round_to_int(Dir.y*100.0f) / 100.0f;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_OwnerTeam = GameServer()->m_apPlayers[Owner]->GetTeam();
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;

	// box2d
	b2BodyDef BodyDef;
    BodyDef.position = b2Vec2(m_Pos.x / 30.f, (m_Pos.y / 30.f) + 2);
    BodyDef.type = b2_dynamicBody;
    m_b2Body = GameServer()->m_pB2World->CreateBody(&BodyDef);

    b2CircleShape Shape;
    Shape.m_radius = 30 / 2 / 30.f;
    b2FixtureDef FixtureDef;
    FixtureDef.density = 1.f;
    FixtureDef.shape = &Shape;
    m_b2Body->CreateFixture(&FixtureDef);

    // dummy body
    b2BodyDef dBodyDef;
    m_DummyBody = GameServer()->m_pB2World->CreateBody(&dBodyDef);

    b2MouseJointDef def;
    def.bodyA = m_DummyBody;
    def.bodyB = m_b2Body;
    def.target = BodyDef.position;
    def.maxForce = 100000;
    def.damping = 0;
    def.stiffness = 100000;
    def.collideConnected = true;

    m_TeeJoint = (b2MouseJoint *)GameServer()->m_pB2World->CreateJoint(&def);
    m_b2Body->SetAwake(true);

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameWorld()->DestroyEntity(this);
	if (m_b2Body)
        GameServer()->m_pB2World->DestroyBody(m_b2Body);
    if (m_DummyBody)
        GameServer()->m_pB2World->DestroyBody(m_DummyBody);
    m_b2Body = 0;
    m_DummyBody = 0;
}

void CProjectile::LoseOwner()
{
	if(m_OwnerTeam == TEAM_BLUE)
		m_Owner = PLAYER_TEAM_BLUE;
	else
		m_Owner = PLAYER_TEAM_RED;
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameWorld()->IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	m_TeeJoint->SetTarget(b2Vec2(CurPos.x / SCALE, CurPos.y / SCALE));
	BodyCollideQuery queryCallback;
	b2Vec2 b2Pos(CurPos.x / 30.f, CurPos.y / 30.f);
	queryCallback.Body = 0;
	queryCallback.findPos = b2Pos;
	b2AABB aabb;
	aabb.lowerBound = b2Vec2(b2Pos.x - 0.001, b2Pos.y - 0.001);
	aabb.upperBound = b2Vec2(b2Pos.x + 0.001, b2Pos.y + 0.001);
	GameServer()->m_pB2World->QueryAABB(&queryCallback, aabb);

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
			GameServer()->CreateSound(CurPos, m_SoundImpact);

		if(m_Explosive)
			GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, m_Damage);

		else if(TargetChr)
			TargetChr->TakeDamage(m_Direction * maximum(0.001f, m_Force), m_Direction*-1, m_Damage, m_Owner, m_Weapon);

		GameWorld()->DestroyEntity(this);
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = round_to_int(m_Pos.x);
	pProj->m_Y = round_to_int(m_Pos.y);
	pProj->m_VelX = round_to_int(m_Direction.x*100.0f);
	pProj->m_VelY = round_to_int(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile Proj;
	FillInfo(&Proj);
	if(!NetConverter()->SnapNewItemConvert(&Proj, this, NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile), SnappingClient))
		return;

	CNetObj_Laser pB2Body;
    pB2Body.m_FromX = pB2Body.m_X = m_b2Body->GetPosition().x * 30.f;
    pB2Body.m_FromY = pB2Body.m_Y = m_b2Body->GetPosition().y * 30.f;
    pB2Body.m_StartTick = Server()->Tick();

	if(!NetConverter()->SnapNewItemConvert(&pB2Body, this, NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser), SnappingClient))
		return;
}
