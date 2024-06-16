/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "eventhandler.h"
#include "gamecontext.h"
#include "player.h"

//////////////////////////////////////////////////
// Event handler
//////////////////////////////////////////////////
CEventHandler::CEventHandler()
{
	m_pGameServer = 0;
	Clear();
}

void CEventHandler::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
}

void *CEventHandler::Create(int Type, int Size, int64 Mask, int Area)
{
	if(m_NumEvents == MAX_EVENTS)
		return 0;
	if(m_CurrentOffset+Size >= MAX_DATASIZE)
		return 0;

	void *p = &m_aData[m_CurrentOffset];
	m_aOffsets[m_NumEvents] = m_CurrentOffset;
	m_aTypes[m_NumEvents] = Type;
	m_aSizes[m_NumEvents] = Size;
	m_aClientMasks[m_NumEvents] = Mask;
	m_aEventArea[m_NumEvents] = Area;
	m_CurrentOffset += Size;
	m_NumEvents++;
	return p;
}

void CEventHandler::Clear()
{
	m_NumEvents = 0;
	m_CurrentOffset = 0;
}

void CEventHandler::Snap(int SnappingClient)
{
	for(int i = 0; i < m_NumEvents; i++)
	{
		if(SnappingClient == -1 || CmaskIsSet(m_aClientMasks[i], SnappingClient))
		{
			CNetEvent_Common *ev = (CNetEvent_Common *)&m_aData[m_aOffsets[i]];
			ivec2 Pos = ivec2(ev->m_X, ev->m_Y);
			
			if(SnappingClient != -1 && GameServer()->m_apPlayers[SnappingClient])
			{
				int SnapMirrorArea = GameServer()->m_apPlayers[SnappingClient]->m_MirrorArea;
				int SelfMirrorArea = m_aEventArea[i];
				if(SnapMirrorArea != SelfMirrorArea)
				{
					if(SnapMirrorArea == -1)
					{
						ev->m_X -= GameServer()->m_MirrorAreaInfos[SelfMirrorArea].m_Go.x;
						ev->m_Y -= GameServer()->m_MirrorAreaInfos[SelfMirrorArea].m_Go.y;
					}
					else
					{
						ev->m_X += GameServer()->m_MirrorAreaInfos[SnapMirrorArea].m_Go.x;
						ev->m_Y += GameServer()->m_MirrorAreaInfos[SnapMirrorArea].m_Go.y;
					}
				}
			}

			if(SnappingClient == -1 || distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, vec2(ev->m_X, ev->m_Y)) < 1500.0f)
			{
				void *d = GameServer()->Server()->SnapNewItem(m_aTypes[i], i, m_aSizes[i]);
				if(d)
					mem_copy(d, &m_aData[m_aOffsets[i]], m_aSizes[i]);
			}
			ev->m_X = Pos.x;
			ev->m_Y = Pos.y;
		}
	}
}