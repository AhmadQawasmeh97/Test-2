#include <Windows.h>
#include <cstdint>

#include "Utils.h"
#include "GameData.h"
#include "HookAPI.h"
#include "FUN.h"

const uint16_t FLAME_WINGMAN = 0x0018;
const uint16_t HERO_AVIAN = 0x05E4;
const uint16_t HERO_BURSTINATRIX = 0x05E9;

static uint16_t g_pendingDestroyedIntID = 0;
static uint32_t g_pendingBurnATK = 0;
static uint8_t g_pendingBurnPlayer = 0;
static uint8_t g_pendingBurnSide = 0;
static uint8_t g_pendingBurnZone = 0;
static bool g_pendingBurn = false;

static bool IsWingman(uint16_t raw)
{
    uint16_t id = (uint16_t)(raw & 0x0FFF);

    if (id == FLAME_WINGMAN)
        return true;

    uint32_t cardId = FUN::GetCardID(id);

    return (cardId & 0x0FFF) == FLAME_WINGMAN;
}

static uint32_t GetOriginalATK(uint16_t intId)
{
    intId = (uint16_t)(intId & 0x0FFF);

    if (intId == 0)
        return 0;

    uint32_t atk = FUN::GetMonsterATK(intId);

    if (atk == 0 || atk > 10000)
        return 0;

    return atk;
}

static void ClearPendingBurn()
{
    g_pendingDestroyedIntID = 0;
    g_pendingBurnATK = 0;
    g_pendingBurnPlayer = 0;
    g_pendingBurnSide = 0;
    g_pendingBurnZone = 0;
    g_pendingBurn = false;
}

void __stdcall FlameWingman_OnSentToGrave(
    uint32_t playerIdx,
    uint32_t zoneIdx)
{
    if (!g_pendingBurn)
        return;

    if (playerIdx != g_pendingBurnSide)
        return;

    if (zoneIdx != g_pendingBurnZone)
        return;

    uint32_t damage = g_pendingBurnATK;
    uint8_t targetPlayer = g_pendingBurnPlayer;

    ClearPendingBurn();

    if (damage == 0)
        return;

    FUN::DealEffectDamage(
        (unsigned int)targetPlayer,
        (unsigned int)damage
    );
}

void __stdcall FlameWingman_AfterBattle()
{
    GameData::BattleResult* result =
        GameData::GetBattleResult();

    if (!result)
        return;

    uint16_t intID0 =
        (uint16_t)(result->sides[0].IntID & 0x0FFF);

    uint16_t intID1 =
        (uint16_t)(result->sides[1].IntID & 0x0FFF);

    bool wing0 = IsWingman(intID0);
    bool wing1 = IsWingman(intID1);

    if (!wing0 && !wing1)
        return;

    uint8_t wingIndex =
        wing0 ? 0 : 1;

    uint8_t destroyedIndex =
        wingIndex ^ 1;

    uint16_t destroyedIntID =
        (uint16_t)(
            result->sides[destroyedIndex].IntID & 0x0FFF
            );

    if (destroyedIntID == 0)
        return;

    uint32_t destroyedOriginalATK =
        GetOriginalATK(destroyedIntID);

    if (destroyedOriginalATK == 0)
        return;

    uint8_t wingSide =
        result->GetSide(wingIndex);

    uint8_t destroyedSide =
        result->GetSide(destroyedIndex);

    uint8_t destroyedZone =
        result->GetZone(destroyedIndex);

    if (wingSide == destroyedSide)
        return;

    g_pendingDestroyedIntID =
        destroyedIntID;

    g_pendingBurnATK =
        destroyedOriginalATK;

    g_pendingBurnPlayer =
        wingSide;

    g_pendingBurnSide =
        destroyedSide;

    g_pendingBurnZone =
        destroyedZone;

    g_pendingBurn = true;

    Register_OnSentToGraveTrigger(
        destroyedIntID,
        FlameWingman_OnSentToGrave
    );
}

void Register_FlameWingman()
{
    Register_AfterDamageCalculation(
        FlameWingman_AfterBattle
    );
}