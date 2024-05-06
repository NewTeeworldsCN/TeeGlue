#ifndef GAME_SERVER_GAMEMODES_CACE_H
#define GAME_SERVER_GAMEMODES_CACE_H

#include <base/vmath.h>

#include <game/server/gamecontroller.h>

#include <map>
#include <vector>

enum ECaceDefine
{
    // weapons
    WEAPON_BOMB = 0,
    WEAPON_WAVEBOMB,
    WEAPON_TELELASER,
    WEAPON_HEALBOMB,

    NUM_CASEITEMS,
};

class CGameControllerCace : public IGameController
{
    struct SPickupInfo
    {
        vec2 m_Pos;
        vec2 m_StartPos;
        int m_PickedTick;
        int m_Type;
        std::vector<int> m_SnapIDs;
    };

    bool IsPicked(SPickupInfo *pPickupInfo);

    std::map<int, std::vector<SPickupInfo>> m_vCacePickupPoints;
    // int 1 = Type, int 2 = Num
    std::map<int, int> m_aCacePlayersInventory[MAX_CLIENTS];
    // -1 = none
    int m_aCacePlayerActiveItem[MAX_CLIENTS];
    int m_aaPlayerSwitchCase[MAX_CLIENTS][NUM_WEAPONS];

    void GetPossibleItem(int Weapon, int ClientID, std::vector<int>& vPossibleItems);
    void GetPlayerActiveItem(int Weapon, int ClientID);

    void CacePickupTick(int Type, SPickupInfo *pPickupInfo);
    void CaceItemTick(int Type, int ClientID);
    void SnapCacePickup(int SnappingClient, int Type, SPickupInfo *pPickupInfo);

    void SendBroadcastItem(int ClientID);
    void RefreshItem(int ClientID);

    // F-DDrace
    void SendBroadcastHUD(const char *pMessage, int To);

    void OnPlayerSwitchItem(int Weapon, int ClientID);

	static void ComSwitch(IConsole::IResult *pResult, void *pContext);
public:
    CGameControllerCace(class CGameContext *pGameServer);
    ~CGameControllerCace();

    void Tick() override;
    void ResetGame() override;
    void Snap(int SnappingClient) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;

	bool DoWincheckMatch() override;
    bool OnEntity(int Index, vec2 Pos) override;

    void RegisterChatCommands(CCommandManager *pManager) override;

private:
    // pickup tick
    void PickupBombTick(SPickupInfo *pPickupInfo);
    void PickupWaveBombTick(SPickupInfo *pPickupInfo);
    void PickupTeleLaserTick(SPickupInfo *pPickupInfo);
    void PickupHealBombTick(SPickupInfo *pPickupInfo);

    // item tick
    void ItemBombTick(int ClientID);
    void ItemWaveBombTick(int ClientID);
    void ItemTeleLaserTick(int ClientID);
    void ItemHealBombTick(int ClientID);

    // pickup snap
    void PickupBombSnap(int SnappingClient, SPickupInfo *pPickupInfo);
    void PickupWaveBombSnap(int SnappingClient, SPickupInfo *pPickupInfo);
    void PickupTeleLaserSnap(int SnappingClient, SPickupInfo *pPickupInfo);
    void PickupHealBombSnap(int SnappingClient, SPickupInfo *pPickupInfo);
};

#endif // GAME_SERVER_GAMEMODES_CACE_H