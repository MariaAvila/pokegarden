#include "global.h"
#include "farm_mons.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/farm.h"
#include "constants/layouts.h"
#include "constants/vars.h"
#include "constants/pokemon.h"
#include "constants/trainer_types.h"

// ---------------------------------------------------------------------------
// Habitat type-bonus table
// ---------------------------------------------------------------------------

struct HabitatInfo {
    u16 layoutId;
    u8  types[4]; // up to 4 bonus types, TYPE_MYSTERY = sentinel
};

#define TYPE_NONE 0

static const struct HabitatInfo sHabitatInfo[FARM_HABITATS_COUNT] = {
    [HABITAT_MAIN]   = { LAYOUT_GARDEN_MAP,          { TYPE_NORMAL, TYPE_GRASS, TYPE_NONE, TYPE_NONE } },
    [HABITAT_WATER]  = { LAYOUT_FARM_HABITAT_WATER,  { TYPE_WATER,  TYPE_ICE,   TYPE_NONE, TYPE_NONE } },
    [HABITAT_FIRE]   = { LAYOUT_FARM_HABITAT_FIRE,   { TYPE_FIRE,   TYPE_DRAGON,TYPE_NONE, TYPE_NONE } },
    [HABITAT_FOREST] = { LAYOUT_FARM_HABITAT_FOREST, { TYPE_BUG,    TYPE_POISON,TYPE_FAIRY,TYPE_FLYING} },
    [HABITAT_CAVE]   = { LAYOUT_FARM_HABITAT_CAVE,   { TYPE_ROCK,   TYPE_GROUND,TYPE_STEEL,TYPE_FIGHTING} },
    [HABITAT_MYSTIC] = { LAYOUT_FARM_HABITAT_MYSTIC, { TYPE_PSYCHIC,TYPE_GHOST, TYPE_DARK, TYPE_ELECTRIC} },
};

// ---------------------------------------------------------------------------
// Helper: get a Pokemon pointer from a FarmMon slot
// ---------------------------------------------------------------------------

static struct Pokemon *GetFarmMonPokemon(const struct FarmMon *slot)
{
    if (slot->boxNum == 0)
    {
        if (slot->boxPos >= gPlayerPartyCount)
            return NULL;
        return &gPlayerParty[slot->boxPos];
    }
    else
    {
        struct BoxPokemon *boxMon = GetBoxedMonPtr(slot->boxNum - 1, slot->boxPos);
        return (struct Pokemon *)boxMon; // BoxPokemon is the first member of Pokemon
    }
}

// ---------------------------------------------------------------------------
// Helper: check if mon type matches habitat bonus types
// ---------------------------------------------------------------------------

bool8 FarmMonHasHabitatBonus(u8 slotIndex)
{
    u8 h, t;
    const struct FarmMon *slot;
    const struct HabitatInfo *habitat = NULL;
    struct Pokemon *mon;
    u8 type1, type2;

    if (slotIndex >= FARM_MON_SLOTS)
        return FALSE;

    slot = &gSaveBlock1Ptr->farmMons[slotIndex];
    if (!slot->active)
        return FALSE;

    // Find matching habitat
    for (h = 0; h < FARM_HABITATS_COUNT; h++)
    {
        if (sHabitatInfo[h].layoutId == slot->mapLayoutId)
        {
            habitat = &sHabitatInfo[h];
            break;
        }
    }
    if (habitat == NULL)
        return FALSE;

    mon = GetFarmMonPokemon(slot);
    if (mon == NULL)
        return FALSE;

    {
        u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
        type1 = gSpeciesInfo[species].types[0];
        type2 = gSpeciesInfo[species].types[1];
    }

    for (t = 0; t < 4; t++)
    {
        u8 bonusType = habitat->types[t];
        if (bonusType == TYPE_NONE)
            break;
        if (type1 == bonusType || type2 == bonusType)
            return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------------------
// Spawn a single farm mon's overworld sprite
// ---------------------------------------------------------------------------

static void SpawnFarmMonSprite(u8 slotIndex)
{
    struct FarmMon *slot = &gSaveBlock1Ptr->farmMons[slotIndex];
    struct Pokemon *mon;
    u16 species;
    struct ObjectEventTemplate t;

    mon = GetFarmMonPokemon(slot);
    if (mon == NULL)
        return;

    species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    if (species == SPECIES_NONE)
        return;

    t.localId         = FARM_MON_LOCAL_ID_BASE + slotIndex;
    t.graphicsId      = (OBJ_EVENT_GFX_MON_BASE + species) & OBJ_EVENT_GFX_SPECIES_MASK;
    t.kind            = OBJ_KIND_NORMAL;
    t.x               = (s16)slot->x - MAP_OFFSET;
    t.y               = (s16)slot->y - MAP_OFFSET;
    t.elevation       = 3;
    t.movementType    = MOVEMENT_TYPE_WANDER_AROUND;
    t.movementRangeX  = 3;
    t.movementRangeY  = 3;
    t.unused          = 0;
    t.trainerType     = TRAINER_TYPE_NONE;
    t.trainerRange_berryTreeId = slotIndex; // used to look up slot on interaction
    t.script          = FarmMonInteractScript;
    t.flagId          = 0;
    t.filler          = 0;

    // Write into save template array so TrySpawnObjectEvents handles respawn
    {
        u8 staticCount = gMapHeader.events ? gMapHeader.events->objectEventCount : 0;
        u8 tplSlot = staticCount + TILLED_PLOTS_COUNT + slotIndex;
        if (tplSlot < OBJECT_EVENT_TEMPLATES_COUNT)
            gSaveBlock1Ptr->objectEventTemplates[tplSlot] = t;
    }

    SpawnSpecialObjectEvent(&t);
}

// ---------------------------------------------------------------------------
// RestoreFarmMons - called on every map load, spawns mons for current layout
// ---------------------------------------------------------------------------

void RestoreFarmMons(void)
{
    u8 i;
    u16 currentLayout = gMapHeader.mapLayoutId;

    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        struct FarmMon *slot = &gSaveBlock1Ptr->farmMons[i];
        if (!slot->active)
            continue;
        if (slot->mapLayoutId != currentLayout)
            continue;
        SpawnFarmMonSprite(i);
    }
}

// ---------------------------------------------------------------------------
// GetTotalFarmMonTemplateCount - used to extend TrySpawnObjectEvents loop
// ---------------------------------------------------------------------------

u8 GetTotalFarmMonTemplateCount(void)
{
    u8 i, count = 0;
    u16 currentLayout = gMapHeader.mapLayoutId;
    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        if (gSaveBlock1Ptr->farmMons[i].active &&
            gSaveBlock1Ptr->farmMons[i].mapLayoutId == currentLayout)
            count++;
    }
    return count;
}

// ---------------------------------------------------------------------------
// PlaceFarmMon - place a party or box mon onto the current habitat map
// Returns slot index, or 0xFF on failure.
// ---------------------------------------------------------------------------

u8 PlaceFarmMon(u8 boxNum, u8 boxPos, u8 x, u8 y)
{
    u8 i;
    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        if (!gSaveBlock1Ptr->farmMons[i].active)
            break;
    }
    if (i == FARM_MON_SLOTS)
        return 0xFF; // no free slot

    gSaveBlock1Ptr->farmMons[i].boxNum      = boxNum;
    gSaveBlock1Ptr->farmMons[i].boxPos      = boxPos;
    gSaveBlock1Ptr->farmMons[i].x = x;
    gSaveBlock1Ptr->farmMons[i].y = y;
    gSaveBlock1Ptr->farmMons[i].mapLayoutId = gMapHeader.mapLayoutId;
    gSaveBlock1Ptr->farmMons[i].active      = TRUE;

    SpawnFarmMonSprite(i);
    return i;
}

// ---------------------------------------------------------------------------
// RecallFarmMon - remove a farm mon from the roster (mon stays in party/box)
// ---------------------------------------------------------------------------

void RecallFarmMon(u8 slotIndex)
{
    u8 i;
    u8 staticCount;
    u8 tplSlot;

    if (slotIndex >= FARM_MON_SLOTS)
        return;

    gSaveBlock1Ptr->farmMons[slotIndex].active = FALSE;

    // Clear the template slot so TrySpawnObjectEvents won't respawn it
    staticCount = gMapHeader.events ? gMapHeader.events->objectEventCount : 0;
    tplSlot = staticCount + TILLED_PLOTS_COUNT + slotIndex;
    if (tplSlot < OBJECT_EVENT_TEMPLATES_COUNT)
        memset(&gSaveBlock1Ptr->objectEventTemplates[tplSlot], 0,
               sizeof(struct ObjectEventTemplate));

    // Remove the live object event using the public API
    RemoveObjectEventByLocalIdAndMap(
        FARM_MON_LOCAL_ID_BASE + slotIndex,
        gSaveBlock1Ptr->location.mapNum,
        gSaveBlock1Ptr->location.mapGroup
    );
}

// ---------------------------------------------------------------------------
// GetFarmMonSlotForObjectEvent - used by interaction script
// ---------------------------------------------------------------------------

u8 GetFarmMonSlotForObjectEvent(u8 objectEventId)
{
    return gObjectEvents[objectEventId].trainerRange_berryTreeId;
}

// ---------------------------------------------------------------------------
// Script-callable specials (use gSelectedObjectEvent)
// ---------------------------------------------------------------------------

// Returns hunger value of the farm mon the player is facing
u16 GetFarmMonHunger(void)
{
    u8 slotIndex = GetFarmMonSlotForObjectEvent(gSelectedObjectEvent);
    struct Pokemon *mon;
    if (slotIndex >= FARM_MON_SLOTS)
        return 0;
    mon = GetFarmMonPokemon(&gSaveBlock1Ptr->farmMons[slotIndex]);
    if (mon == NULL)
        return 0;
    return GetMonData(mon, MON_DATA_HUNGER, NULL);
}

// Feeds the farm mon the player is facing (decrements hunger)
void FeedFarmMon(void)
{
    u8 slotIndex = GetFarmMonSlotForObjectEvent(gSelectedObjectEvent);
    struct Pokemon *mon;
    u16 hunger;
    if (slotIndex >= FARM_MON_SLOTS)
        return;
    mon = GetFarmMonPokemon(&gSaveBlock1Ptr->farmMons[slotIndex]);
    if (mon == NULL)
        return;
    hunger = GetMonData(mon, MON_DATA_HUNGER, NULL);
    if (hunger > 0)
    {
        hunger--;
        SetMonData(mon, MON_DATA_HUNGER, &hunger);
    }
}

// Recalls the farm mon the player is facing back to party/box
void RecallSelectedFarmMon(void)
{
    u8 slotIndex = GetFarmMonSlotForObjectEvent(gSelectedObjectEvent);
    RecallFarmMon(slotIndex);
}

// ---------------------------------------------------------------------------
// Script-callable placement specials
// ---------------------------------------------------------------------------

// Places a party mon on the current farm map.
// Script sets VAR_0x8009 = party slot index before calling.
// Returns 0xFF on failure (no free slot or invalid slot).
u16 PlacePartyMonOnFarm(void)
{
    u8 partySlot = (u8)VarGet(VAR_0x8009);
    s16 sx, sy;

    if (partySlot >= gPlayerPartyCount)
        return 0xFF;

    GetXYCoordsOneStepInFrontOfPlayer(&sx, &sy);
    return PlaceFarmMon(0, partySlot, (u8)sx, (u8)sy);
}

// Returns TRUE if the party mon in VAR_0x8009 is already on a farm.
u16 IsPartyMonOnFarm(void)
{
    u8 partySlot = (u8)VarGet(VAR_0x8009);
    u8 i;
    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        struct FarmMon *slot = &gSaveBlock1Ptr->farmMons[i];
        if (slot->active && slot->boxNum == 0 && slot->boxPos == partySlot)
            return TRUE;
    }
    return FALSE;
}

// ---------------------------------------------------------------------------
// Habitat time bonuses - called from clock.c each minute tick
// ---------------------------------------------------------------------------

void UpdateFarmMonHabitatBonuses(s32 minutes)
{
    u8 i;
    // One friendship tick and XP drip per 60 real minutes per matching mon
    // (minutes here is real minutes, no 240x multiplier)
    s32 friendshipTicks = minutes / 60;
    s32 expTicks        = minutes / 60;

    if (friendshipTicks == 0 && expTicks == 0)
        return;

    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        struct Pokemon *mon;
        u8 friendship;
        u32 exp, nextLvlExp;
        u8 level;

        if (!gSaveBlock1Ptr->farmMons[i].active)
            continue;
        if (!FarmMonHasHabitatBonus(i))
            continue;

        mon = GetFarmMonPokemon(&gSaveBlock1Ptr->farmMons[i]);
        if (mon == NULL)
            continue;

        // +1 friendship per hour in matching habitat
        if (friendshipTicks > 0)
        {
            friendship = GetMonData(mon, MON_DATA_FRIENDSHIP, NULL);
            if (friendship < MAX_FRIENDSHIP)
            {
                friendship += (u8)friendshipTicks;
                if (friendship > MAX_FRIENDSHIP)
                    friendship = MAX_FRIENDSHIP;
                SetMonData(mon, MON_DATA_FRIENDSHIP, &friendship);
            }
        }

        // Small XP drip: +expTicks XP per hour (modest, farming is slow)
        if (expTicks > 0)
        {
            level = GetMonData(mon, MON_DATA_LEVEL, NULL);
            if (level < MAX_LEVEL)
            {
                u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
                exp = GetMonData(mon, MON_DATA_EXP, NULL);
                exp += (u32)expTicks;
                // Cap at next level threshold to avoid silent level-ups off-map
                nextLvlExp = gExperienceTables[gSpeciesInfo[species].growthRate][level + 1];
                if (exp >= nextLvlExp)
                    exp = nextLvlExp - 1;
                SetMonData(mon, MON_DATA_EXP, &exp);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// UpdateFarmMonHunger - called from clock.c each real minute
// ---------------------------------------------------------------------------

void UpdateFarmMonHunger(s32 minutes)
{
    u8 i;
    s32 hungerTicks = minutes / HUNGER_MINUTES_PER_TICK;

    if (hungerTicks == 0)
        return;

    for (i = 0; i < FARM_MON_SLOTS; i++)
    {
        struct Pokemon *mon;
        u16 hunger;

        if (!gSaveBlock1Ptr->farmMons[i].active)
            continue;

        mon = GetFarmMonPokemon(&gSaveBlock1Ptr->farmMons[i]);
        if (mon == NULL)
            continue;

        hunger = GetMonData(mon, MON_DATA_HUNGER, NULL);
        if (hunger < HUNGER_MAX)
        {
            hunger += (u16)hungerTicks;
            if (hunger > HUNGER_MAX)
                hunger = HUNGER_MAX;
            SetMonData(mon, MON_DATA_HUNGER, &hunger);
        }
    }
}
