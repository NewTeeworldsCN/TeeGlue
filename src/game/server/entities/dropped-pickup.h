#ifndef GAME_SERVER_ENTITIES_DROPPED_PICKUP_H
#define GAME_SERVER_ENTITIES_DROPPED_PICKUP_H

#include <game/server/entity.h>

const int DroppedPickupPhysSize = 14;

class CDroppedPickup : public CEntity
{
public:
	CDroppedPickup(CGameWorld *pGameWorld, int Type, vec2 Pos, vec2 Direction);

	void Tick() override;
	void Snap(int SnappingClient) override;

private:
	int m_Type;
    vec2 m_Vel;
};

#endif // GAME_SERVER_ENTITIES_DROPPED_PICKUP_H
