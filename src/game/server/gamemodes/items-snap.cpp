#include <base/math.h>

#include <engine/server.h>

#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>

#include <game/server/gamecontext.h>
#include <game/server/localization.h>
#include <game/server/player.h>

#include <generated/protocol.h>

#include "cace.h"

void CGameControllerCace::SnapCacePickup(int SnappingClient, int Type, SPickupInfo *pPickupInfo)
{
	if(SnappingClient != -1)
	{
		float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x - pPickupInfo->m_Pos.x;
		float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y - pPickupInfo->m_Pos.y;

		if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
			return;

		if(distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, pPickupInfo->m_Pos) > 1100.0f)
			return;
	}

	switch (Type)
	{
		case ECaceDefine::WEAPON_BOMB: PickupBombSnap(SnappingClient, pPickupInfo); break;
		case ECaceDefine::WEAPON_WAVEBOMB: PickupWaveBombSnap(SnappingClient, pPickupInfo); break;
		case ECaceDefine::WEAPON_TELELASER: PickupTeleLaserSnap(SnappingClient, pPickupInfo); break;
	}
}

void CGameControllerCace::PickupBombSnap(int SnappingClient, SPickupInfo *pPickupInfo)
{
    {
        CNetObj_Pickup Pickup;

        Pickup.m_Type = PICKUP_HEALTH;

        Pickup.m_X = pPickupInfo->m_Pos.x;
        Pickup.m_Y = pPickupInfo->m_Pos.y;

        if(!NetConverter()->SnapNewItemConvert(&Pickup, this, NETOBJTYPE_PICKUP, pPickupInfo->m_SnapIDs[0], sizeof(CNetObj_Pickup), SnappingClient))
            return;
    }

    // infclass
    float time = (Server()->Tick()-m_GameStartTick)/(float)Server()->TickSpeed();
    float Angle = fmodf(time*pi/2, 2.0f*pi);
    for(int i = 0; i < 4; i ++)
    {
        float ShiftedAngle = Angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(4);

        CNetObj_Projectile Proj;

        Proj.m_StartTick = m_GameStartTick;
        Proj.m_Type = WEAPON_HAMMER;

        Proj.m_X = (int)(pPickupInfo->m_Pos.x + 24.0f * cos(ShiftedAngle));
        Proj.m_Y = (int)(pPickupInfo->m_Pos.y + 24.0f * sin(ShiftedAngle));
        Proj.m_VelX = 0;
        Proj.m_VelY = 0;

        if(!NetConverter()->SnapNewItemConvert(&Proj, this, NETOBJTYPE_PROJECTILE, pPickupInfo->m_SnapIDs[1 + i], sizeof(CNetObj_Projectile), SnappingClient))
        return;
    }
}

void CGameControllerCace::PickupWaveBombSnap(int SnappingClient, SPickupInfo *pPickupInfo)
{
    {
        CNetObj_Pickup Pickup;

        Pickup.m_Type = PICKUP_ARMOR;

        Pickup.m_X = pPickupInfo->m_Pos.x;
        Pickup.m_Y = pPickupInfo->m_Pos.y;

        if(!NetConverter()->SnapNewItemConvert(&Pickup, this, NETOBJTYPE_PICKUP, pPickupInfo->m_SnapIDs[0], sizeof(CNetObj_Pickup), SnappingClient))
            return;
    }

    // infclass
    float time = (Server()->Tick()-m_GameStartTick)/(float)Server()->TickSpeed();
    float Angle = fmodf(time*pi/2, 2.0f*pi);
    for(int i = 0; i < 4; i ++)
    {
        float ShiftedAngle = Angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(4);

        CNetObj_Projectile Proj;

        Proj.m_StartTick = m_GameStartTick;
        Proj.m_Type = WEAPON_LASER;

        Proj.m_X = (int)(pPickupInfo->m_Pos.x + 24.0f * cos(ShiftedAngle));
        Proj.m_Y = (int)(pPickupInfo->m_Pos.y + 24.0f * sin(ShiftedAngle));
        Proj.m_VelX = 0;
        Proj.m_VelY = 0;

        if(!NetConverter()->SnapNewItemConvert(&Proj, this, NETOBJTYPE_PROJECTILE, pPickupInfo->m_SnapIDs[1 + i], sizeof(CNetObj_Projectile), SnappingClient))
        return;
    }
}

void CGameControllerCace::PickupTeleLaserSnap(int SnappingClient, SPickupInfo *pPickupInfo)
{
    {
        CNetObj_Projectile Proj;

        Proj.m_StartTick = m_GameStartTick;
        Proj.m_Type = WEAPON_LASER;

        Proj.m_X = pPickupInfo->m_Pos.x;
        Proj.m_Y = pPickupInfo->m_Pos.y;
        Proj.m_VelX = 0;
        Proj.m_VelY = 0;

        if(!NetConverter()->SnapNewItemConvert(&Proj, this, NETOBJTYPE_PROJECTILE, pPickupInfo->m_SnapIDs[0], sizeof(CNetObj_Projectile), SnappingClient))
            return;
    }

    // infclass
    float time = (Server()->Tick()-m_GameStartTick)/(float)Server()->TickSpeed();
    float Angle = fmodf(time*pi/2, 2.0f*pi);
    for(int i = 0; i < 4; i ++)
    {
        float ShiftedAngle = Angle + 2.0*pi*static_cast<float>(i)/static_cast<float>(4);

        CNetObj_Projectile Proj;

        Proj.m_StartTick = m_GameStartTick;
        Proj.m_Type = WEAPON_HAMMER;

        Proj.m_X = (int)(pPickupInfo->m_Pos.x + 24.0f * cos(ShiftedAngle));
        Proj.m_Y = (int)(pPickupInfo->m_Pos.y + 24.0f * sin(ShiftedAngle));
        Proj.m_VelX = 0;
        Proj.m_VelY = 0;

        if(!NetConverter()->SnapNewItemConvert(&Proj, this, NETOBJTYPE_PROJECTILE, pPickupInfo->m_SnapIDs[1 + i], sizeof(CNetObj_Projectile), SnappingClient))
        return;
    }
}