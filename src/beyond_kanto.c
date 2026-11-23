#include "global.h"
#include "beyond_kanto.h"
#include "event_data.h"
#include "constants/vars.h"

#define BK_INITIAL_LEAGUE_POINTS 1000
#define BK_INITIAL_GLOBAL_RANK   10000
#define BK_MAX_LEAGUE_POINTS     9999
#define BK_MAX_REPUTATION        9999

static u16 ClampAddVar(u16 varId, u16 amount, u16 maximum)
{
    u32 value = VarGet(varId) + amount;

    if (value > maximum)
        value = maximum;

    VarSet(varId, value);
    return value;
}

static void RecalculateGlobalRank(void)
{
    u16 points = VarGet(VAR_BK_LEAGUE_POINTS);
    u32 progress;
    u16 rank;

    if (points <= BK_INITIAL_LEAGUE_POINTS)
    {
        rank = BK_INITIAL_GLOBAL_RANK;
    }
    else
    {
        progress = (u32)(points - BK_INITIAL_LEAGUE_POINTS) * (BK_INITIAL_GLOBAL_RANK - 1);
        progress /= (BK_MAX_LEAGUE_POINTS - BK_INITIAL_LEAGUE_POINTS);
        rank = BK_INITIAL_GLOBAL_RANK - progress;

        if (rank < 1)
            rank = 1;
    }

    VarSet(VAR_BK_GLOBAL_RANK, rank);
}

void BeyondKanto_InitLeagueProfile(void)
{
    if (VarGet(VAR_BK_INITIALIZED))
        return;

    VarSet(VAR_BK_LEAGUE_POINTS, BK_INITIAL_LEAGUE_POINTS);
    VarSet(VAR_BK_GLOBAL_RANK, BK_INITIAL_GLOBAL_RANK);
    VarSet(VAR_BK_WINS, 0);
    VarSet(VAR_BK_LOSSES, 0);
    VarSet(VAR_BK_WIN_STREAK, 0);
    VarSet(VAR_BK_BEST_WIN_STREAK, 0);
    VarSet(VAR_BK_REPUTATION, 0);
    VarSet(VAR_BK_CHAMPION_DEFENSES, 0);
    VarSet(VAR_BK_TOURNAMENT_WINS, 0);
    VarSet(VAR_BK_RIVAL_KAI_STAGE, 0);
    VarSet(VAR_BK_RIVAL_MAYA_STAGE, 0);
    VarSet(VAR_BK_RIVAL_DAMON_STAGE, 0);
    VarSet(VAR_BK_INITIALIZED, 1);
}

void BeyondKanto_RecordTrainerBattleResult(bool8 playerWon)
{
    u16 points;
    u16 streak;

    BeyondKanto_InitLeagueProfile();

    if (playerWon)
    {
        ClampAddVar(VAR_BK_WINS, 1, 9999);
        points = VarGet(VAR_BK_LEAGUE_POINTS);
        if (points <= BK_MAX_LEAGUE_POINTS - 25)
            VarSet(VAR_BK_LEAGUE_POINTS, points + 25);
        else
            VarSet(VAR_BK_LEAGUE_POINTS, BK_MAX_LEAGUE_POINTS);

        streak = ClampAddVar(VAR_BK_WIN_STREAK, 1, 9999);
        if (streak > VarGet(VAR_BK_BEST_WIN_STREAK))
            VarSet(VAR_BK_BEST_WIN_STREAK, streak);

        ClampAddVar(VAR_BK_REPUTATION, 3, BK_MAX_REPUTATION);
    }
    else
    {
        ClampAddVar(VAR_BK_LOSSES, 1, 9999);
        points = VarGet(VAR_BK_LEAGUE_POINTS);
        if (points > 1010)
            VarSet(VAR_BK_LEAGUE_POINTS, points - 10);
        else
            VarSet(VAR_BK_LEAGUE_POINTS, BK_INITIAL_LEAGUE_POINTS);

        VarSet(VAR_BK_WIN_STREAK, 0);
    }

    RecalculateGlobalRank();
}

void BeyondKanto_RecordTournamentWin(void)
{
    BeyondKanto_InitLeagueProfile();
    ClampAddVar(VAR_BK_TOURNAMENT_WINS, 1, 9999);
    ClampAddVar(VAR_BK_LEAGUE_POINTS, 100, BK_MAX_LEAGUE_POINTS);
    ClampAddVar(VAR_BK_REPUTATION, 25, BK_MAX_REPUTATION);
    RecalculateGlobalRank();
}

void BeyondKanto_RecordChampionDefense(void)
{
    BeyondKanto_InitLeagueProfile();
    ClampAddVar(VAR_BK_CHAMPION_DEFENSES, 1, 9999);
    ClampAddVar(VAR_BK_REPUTATION, 50, BK_MAX_REPUTATION);
    VarSet(VAR_BK_LEAGUE_POINTS, BK_MAX_LEAGUE_POINTS);
    VarSet(VAR_BK_GLOBAL_RANK, 1);
}

u16 BeyondKanto_GetLeaguePoints(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_LEAGUE_POINTS);
}

u16 BeyondKanto_GetGlobalRank(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_GLOBAL_RANK);
}

u16 BeyondKanto_GetWins(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_WINS);
}

u16 BeyondKanto_GetLosses(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_LOSSES);
}

u16 BeyondKanto_GetWinStreak(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_WIN_STREAK);
}

u16 BeyondKanto_GetBestWinStreak(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_BEST_WIN_STREAK);
}

u16 BeyondKanto_GetReputation(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_REPUTATION);
}

u16 BeyondKanto_GetTournamentWins(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_TOURNAMENT_WINS);
}

u16 BeyondKanto_GetChampionDefenses(void)
{
    BeyondKanto_InitLeagueProfile();
    return VarGet(VAR_BK_CHAMPION_DEFENSES);
}

u16 BeyondKanto_GetTier(void)
{
    u16 points = BeyondKanto_GetLeaguePoints();

    if (VarGet(VAR_BK_CHAMPION_DEFENSES) != 0)
        return BK_TIER_CHAMPION;
    if (points >= 3500)
        return BK_TIER_MASTER;
    if (points >= 2800)
        return BK_TIER_ACE;
    if (points >= 2200)
        return BK_TIER_GOLD;
    if (points >= 1750)
        return BK_TIER_SILVER;
    if (points >= 1350)
        return BK_TIER_BRONZE;
    if (points >= BK_INITIAL_LEAGUE_POINTS)
        return BK_TIER_ROOKIE;

    return BK_TIER_UNRANKED;
}

u16 BeyondKanto_SpecialRecordTournamentWin(void)
{
    BeyondKanto_RecordTournamentWin();
    return 0;
}

u16 BeyondKanto_SpecialRecordChampionDefense(void)
{
    BeyondKanto_RecordChampionDefense();
    return 0;
}
