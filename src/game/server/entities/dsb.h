#ifndef GAME_SERVER_ENTITIES_DSB_H
#define GAME_SERVER_ENTITIES_DSB_H

#include <game/server/entity.h>

class CDSB : public CEntity
{
public:
	CDSB(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Direction);

	void Tick() override;
	void Reset() override;
	void Snap(int SnappingClient) override;

	int GetStartTick() const { return m_StartTick; }
	int GetOwner() const { return m_Owner; }

private:
	vec2 m_Vel;

	int m_Owner;
	int m_StartTick;
};

#endif
