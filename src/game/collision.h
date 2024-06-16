/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

#include <map>
#include <vector>

class CCollision
{
	struct CTile *m_pTiles;
	class CTeleTile *m_pTele;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	bool IsTile(int x, int y, int Flag=COLFLAG_SOLID) const;
	int GetTile(int x, int y) const;

	std::map<int, std::vector<vec2>> m_TeleIns;
	std::map<int, std::vector<vec2>> m_TeleOuts;
public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_WATER=8,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y, int Flag=COLFLAG_SOLID) const { return IsTile(round_to_int(x), round_to_int(y), Flag); }
	bool CheckPoint(vec2 Pos, int Flag=COLFLAG_SOLID) const { return CheckPoint(Pos.x, Pos.y, Flag); }
	int GetCollisionAt(float x, float y) const { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath=0) const;
	bool TestBox(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID) const;

	const std::vector<vec2> &TeleIns(int Number) { return m_TeleIns[Number]; }
	const std::vector<vec2> &TeleOuts(int Number) { return m_TeleOuts[Number]; }

	int GetTileport(int x, int y) const;
	int GetTileportOut(int x, int y) const;
	int GetTileport(vec2 Pos) const { return GetTileport(Pos.x, Pos.y); }
	int GetTileportOut(vec2 Pos) const { return GetTileportOut(Pos.x, Pos.y); }

	class CTeleTile *TeleLayer() { return m_pTele; }
};

#endif
