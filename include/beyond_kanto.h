#ifndef GUARD_BEYOND_KANTO_H
#define GUARD_BEYOND_KANTO_H

#include "global.h"

enum BeyondKantoLeagueTier
{
    BK_TIER_UNRANKED,
    BK_TIER_ROOKIE,
    BK_TIER_BRONZE,
    BK_TIER_SILVER,
    BK_TIER_GOLD,
    BK_TIER_ACE,
    BK_TIER_MASTER,
    BK_TIER_CHAMPION,
};

void BeyondKanto_InitLeagueProfile(void);
void BeyondKanto_RecordTrainerBattleResult(bool8 playerWon);
void BeyondKanto_RecordTournamentWin(void);
void BeyondKanto_RecordChampionDefense(void);

u16 BeyondKanto_GetLeaguePoints(void);
u16 BeyondKanto_GetGlobalRank(void);
u16 BeyondKanto_GetWins(void);
u16 BeyondKanto_GetLosses(void);
u16 BeyondKanto_GetWinStreak(void);
u16 BeyondKanto_GetBestWinStreak(void);
u16 BeyondKanto_GetReputation(void);
u16 BeyondKanto_GetTournamentWins(void);
u16 BeyondKanto_GetChampionDefenses(void);
u8 BeyondKanto_GetTier(void);

#endif // GUARD_BEYOND_KANTO_H
