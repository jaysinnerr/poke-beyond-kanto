#include "global.h"
#include "pokemon.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "beyond_kanto_follower.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/pokemon.h"
#include "constants/species.h"

static u8 GetFollowerGraphicsId(u16 species)
{
    switch (species)
    {
    case SPECIES_PIDGEY:      return OBJ_EVENT_GFX_PIDGEY;
    case SPECIES_PIDGEOT:     return OBJ_EVENT_GFX_PIDGEOT;
    case SPECIES_SPEAROW:     return OBJ_EVENT_GFX_SPEAROW;
    case SPECIES_FEAROW:      return OBJ_EVENT_GFX_FEAROW;
    case SPECIES_PIKACHU:     return OBJ_EVENT_GFX_PIKACHU;
    case SPECIES_NIDORAN_F:   return OBJ_EVENT_GFX_NIDORAN_F;
    case SPECIES_NIDORAN_M:   return OBJ_EVENT_GFX_NIDORAN_M;
    case SPECIES_NIDORINO:    return OBJ_EVENT_GFX_NIDORINO;
    case SPECIES_CLEFAIRY:    return OBJ_EVENT_GFX_CLEFAIRY;
    case SPECIES_JIGGLYPUFF:  return OBJ_EVENT_GFX_JIGGLYPUFF;
    case SPECIES_WIGGLYTUFF:  return OBJ_EVENT_GFX_WIGGLYTUFF;
    case SPECIES_MEOWTH:      return OBJ_EVENT_GFX_MEOWTH;
    case SPECIES_PSYDUCK:     return OBJ_EVENT_GFX_PSYDUCK;
    case SPECIES_POLIWRATH:   return OBJ_EVENT_GFX_POLIWRATH;
    case SPECIES_MACHOP:      return OBJ_EVENT_GFX_MACHOP;
    case SPECIES_MACHOKE:     return OBJ_EVENT_GFX_MACHOKE;
    case SPECIES_SLOWPOKE:    return OBJ_EVENT_GFX_SLOWPOKE;
    case SPECIES_SLOWBRO:     return OBJ_EVENT_GFX_SLOWBRO;
    case SPECIES_DODUO:       return OBJ_EVENT_GFX_DODUO;
    case SPECIES_SEEL:        return OBJ_EVENT_GFX_SEEL;
    case SPECIES_VOLTORB:     return OBJ_EVENT_GFX_VOLTORB;
    case SPECIES_CUBONE:      return OBJ_EVENT_GFX_CUBONE;
    case SPECIES_CHANSEY:     return OBJ_EVENT_GFX_CHANSEY;
    case SPECIES_KANGASKHAN:  return OBJ_EVENT_GFX_KANGASKHAN;
    case SPECIES_LAPRAS:      return OBJ_EVENT_GFX_LAPRAS;
    case SPECIES_OMANYTE:     return OBJ_EVENT_GFX_OMANYTE;
    case SPECIES_KABUTO:      return OBJ_EVENT_GFX_KABUTO;
    case SPECIES_SNORLAX:     return OBJ_EVENT_GFX_SNORLAX;
    case SPECIES_ARTICUNO:    return OBJ_EVENT_GFX_ARTICUNO;
    case SPECIES_ZAPDOS:      return OBJ_EVENT_GFX_ZAPDOS;
    case SPECIES_MOLTRES:     return OBJ_EVENT_GFX_MOLTRES;
    case SPECIES_MEWTWO:      return OBJ_EVENT_GFX_MEWTWO;
    case SPECIES_MEW:         return OBJ_EVENT_GFX_MEW;
    case SPECIES_RAIKOU:      return OBJ_EVENT_GFX_RAIKOU;
    case SPECIES_ENTEI:       return OBJ_EVENT_GFX_ENTEI;
    case SPECIES_SUICUNE:     return OBJ_EVENT_GFX_SUICUNE;
    case SPECIES_LUGIA:       return OBJ_EVENT_GFX_LUGIA;
    case SPECIES_HO_OH:       return OBJ_EVENT_GFX_HO_OH;
    case SPECIES_CELEBI:      return OBJ_EVENT_GFX_CELEBI;
    default:                   return NUM_OBJ_EVENT_GFX;
    }
}

static u16 GetLeadFollowerSpecies(void)
{
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES);

        if (species != SPECIES_NONE && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
            return species;
    }

    return SPECIES_NONE;
}

static u8 TurnLeft(u8 direction)
{
    switch (direction)
    {
    case DIR_NORTH: return DIR_WEST;
    case DIR_SOUTH: return DIR_EAST;
    case DIR_WEST:  return DIR_SOUTH;
    case DIR_EAST:  return DIR_NORTH;
    default:        return DIR_SOUTH;
    }
}

static u8 TurnRight(u8 direction)
{
    switch (direction)
    {
    case DIR_NORTH: return DIR_EAST;
    case DIR_SOUTH: return DIR_WEST;
    case DIR_WEST:  return DIR_NORTH;
    case DIR_EAST:  return DIR_SOUTH;
    default:        return DIR_SOUTH;
    }
}

static bool8 TryGetSafeFollowerSpawnCoords(s16 *x, s16 *y, u8 playerDirection)
{
    struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];
    u8 directions[4];
    s16 playerX;
    s16 playerY;
    u8 i;

    directions[0] = GetOppositeDirection(playerDirection);
    directions[1] = TurnLeft(playerDirection);
    directions[2] = TurnRight(playerDirection);
    directions[3] = playerDirection;

    PlayerGetDestCoords(&playerX, &playerY);

    for (i = 0; i < ARRAY_COUNT(directions); i++)
    {
        s16 candidateX = playerX;
        s16 candidateY = playerY;

        MoveCoords(directions[i], &candidateX, &candidateY);
        if (GetCollisionAtCoords(player, candidateX, candidateY, directions[i]) == COLLISION_NONE)
        {
            *x = candidateX;
            *y = candidateY;
            return TRUE;
        }
    }

    return FALSE;
}

void BeyondKantoFollower_Remove(void)
{
    u8 objectEventId;

    if (!TryGetObjectEventIdByLocalIdAndMap(
            LOCALID_BK_FOLLOWER,
            gSaveBlock1Ptr->location.mapNum,
            gSaveBlock1Ptr->location.mapGroup,
            &objectEventId))
    {
        RemoveObjectEventByLocalIdAndMap(
            LOCALID_BK_FOLLOWER,
            gSaveBlock1Ptr->location.mapNum,
            gSaveBlock1Ptr->location.mapGroup);
    }
}

void BeyondKantoFollower_OnMapLoad(void)
{
    u8 graphicsId;
    u8 objectEventId;
    u16 species;
    s16 x;
    s16 y;
    u8 direction;

    if (!TryGetObjectEventIdByLocalIdAndMap(
            LOCALID_BK_FOLLOWER,
            gSaveBlock1Ptr->location.mapNum,
            gSaveBlock1Ptr->location.mapGroup,
            &objectEventId))
        return;

    species = GetLeadFollowerSpecies();
    if (species == SPECIES_NONE)
        return;

    graphicsId = GetFollowerGraphicsId(species);
    if (graphicsId >= NUM_OBJ_EVENT_GFX)
        return;

    direction = GetPlayerFacingDirection();
    if (!TryGetSafeFollowerSpawnCoords(&x, &y, direction))
        return;

    objectEventId = SpawnSpecialObjectEventParameterized(
        graphicsId,
        MOVEMENT_TYPE_COPY_PLAYER,
        LOCALID_BK_FOLLOWER,
        x,
        y,
        PlayerGetElevation());

    if (objectEventId < OBJECT_EVENTS_COUNT)
        ObjectEventTurn(&gObjectEvents[objectEventId], direction);
}
