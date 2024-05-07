#ifndef GAME_SERVER_ENTITIES_BOMB_H
#define GAME_SERVER_ENTITIES_BOMB_H

#include <game/server/entity.h>

const int BombPhysSize = 24;

class CBomb : public CEntity
{
public:
	CBomb(CGameWorld *pGameWorld, int Owner, int Type, vec2 Pos, vec2 Direction);

	void Tick() override;
	void Reset() override;
	void Snap(int SnappingClient) override;

	int GetStartTick() const { return m_StartTick; }
	int GetOwner() const { return m_Owner; }

private:
	vec2 m_Vel;

	int m_Type;
	int m_Owner;
	int m_StartTick;
};

#endif
