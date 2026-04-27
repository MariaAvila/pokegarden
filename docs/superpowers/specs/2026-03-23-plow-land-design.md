# Plow Land - Design Spec

**Date:** 2026-03-23
**Status:** Approved

## Summary

The player uses a hoe (Key Item) to till `MB_SAND` tiles on a dedicated garden submap, dynamically creating berry tree planting spots. Tilled plots persist across map transitions via a new save block struct. Berry growth, watering, and harvesting use the vanilla berry tree system unchanged.

---

## Scope

This spec covers:
- The `ITEM_PLOW_TOOL` use flow
- The `TilledPlot` save struct
- Metatile manipulation on use
- Berry tree object event spawning and restoration on map load
- The GardenMap submap structure

It does not cover:
- Design of the main farm map or other subareas
- Display Pokémon placement
- Berry tree graphics or growth logic (vanilla, unchanged)

---

## Data Model

### New struct added to `SaveBlock1`

```c
#define TILLED_PLOTS_COUNT 32
#define TILLED_PLOT_LOCAL_ID_BASE 2  // localIds 2..15 reserved for berry tree object events

struct TilledPlot {
    s16 x;          // raw map-grid coords (as returned by GetXYCoordsOneStepInFrontOfPlayer)
    s16 y;          // raw map-grid coords
    u16 mapLayoutId;
    u8  berryTreeId;
    bool8 active;
};
// sizeof == 8 bytes; verify with STATIC_ASSERT(sizeof(struct TilledPlot) == 8, ...)
```

Added to `struct SaveBlock1` in `include/global.h`:

```c
struct TilledPlot tilledPlots[TILLED_PLOTS_COUNT];  // 256 bytes
```

### Berry tree slot reservation

- Existing `berryTrees[128]` array in SaveBlock1 is reused
- Slots 96–127 (last 32) are reserved for farm plots
- Slots 0–95 remain available for the rest of the world (verified: world maps currently use only IDs 0–7)
- Reserved slots are left as `gBlankBerryTree` (all zeroes) until the player actually plants a berry; `PlantBerryTree()` is called only by the existing `ObjectEventInteractionPlantBerryTree` handler, not by the hoe

### Initialization

- `SaveBlock1` is fully zeroed by `ClearSav1()` on new game, so `tilledPlots[]` requires no explicit initialization code. No change to `src/new_game.c` needed.

---

## Metatile vs. Metatile Behavior

Two distinct concepts are used in this system:

- **Metatile behavior** (`MB_*`): read-only flags checked at runtime (e.g. `MB_SAND = 0x21`). Used in `CanTill()` to detect tillable ground.
- **Metatile ID**: the tileset-specific index written to the map grid via `MapGridSetMetatileIdAt()`. The tilled-soil tile ID must be defined during GardenMap authoring and added to `include/constants/metatile_labels.h` as `METATILE_GARDEN_TILLED_SOIL`. This tile must have behavior `MB_BERRY_TREE_SOIL (0xA0)` in the tileset so that the vanilla berry interaction logic recognises it.

---

## Hoe Use Flow

### `CanTill(s16 x, s16 y)` (static) in `src/item_use.c`

`x` and `y` are raw map-grid coordinates as returned by `GetXYCoordsOneStepInFrontOfPlayer`.

Returns `TRUE` if all of the following hold:
- `MapGridGetMetatileBehaviorAt(x, y) == MB_SAND`
- No active `tilledPlots[]` entry already exists with matching `(x, y, mapLayoutId)` (prevents double-tilling)
- The number of active plots with `mapLayoutId == gMapHeader.mapLayoutId` is less than 14

Note: `CanTill` does not repeat the `InFarm()` check - its caller `ItemUseOutOfBattle_Hoe` gates on `InFarm()` first. This precondition is documented here explicitly.

### `ItemUseOutOfBattle_Hoe(u8 taskId)` in `src/item_use.c`

1. Call `GetXYCoordsOneStepInFrontOfPlayer(&x, &y)` to get raw map-grid coords
2. If `InFarm()` AND `CanTill(x, y)`:
   - Set `sItemUseOnFieldCB = ItemUseOnFieldCB_Hoe`
   - Call `SetUpItemUseOnFieldCallback(taskId)`
3. Else: `DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem)`

### `ItemUseOnFieldCB_Hoe(u8 taskId)` (static)

Steps are ordered so spawn is attempted before any save state is written, enabling clean rollback on failure:

1. Find the first free slot `i` in `gSaveBlock1Ptr->tilledPlots[]` (`active == FALSE`)
2. Assign `berryTreeId = 96 + i` - a direct 1-to-1 mapping between plot slot and berry tree slot; no secondary scan needed
3. Compute `localId = TILLED_PLOT_LOCAL_ID_BASE + i` (values 2–15; unique per map)
4. Attempt spawn: call `SpawnSpecialObjectEventParameterized(OBJ_EVENT_GFX_BERRY_TREE, MOVEMENT_TYPE_BERRY_TREE_GROWTH, localId, x + MAP_OFFSET, y + MAP_OFFSET, 3)`. Note: the function subtracts `MAP_OFFSET` internally, so pass `x + MAP_OFFSET` (consistent with existing callers in `src/field_specials.c`)
5. If spawn returns `OBJECT_EVENTS_COUNT` (pool full): display error message, `DestroyTask(taskId)`, return - do not write to save data or change the metatile
6. Set `.trainerRange_berryTreeId = berryTreeId` on the returned object event
7. Call `MapGridSetMetatileIdAt(x, y, METATILE_GARDEN_TILLED_SOIL)`, then `DrawWholeMapView()` to update screen
8. Store `x`, `y`, `gMapHeader.mapLayoutId`, `berryTreeId` in slot `i`; set `active = TRUE`
9. `DestroyTask(taskId)`

### Bug fix

- `ITEM_PLOW_TOOL.fieldUseFunc` currently points to `ItemUseOutOfBattle_Rod` - change to `ItemUseOutOfBattle_Hoe`

---

## Persistence (Map Load)

### `RestoreTilledPlots()` in `src/item_use.c`

Called on map transition into GardenMap (hooked in `src/overworld.c`). Runs inside `LoadMapFromWarp`, before tilesets are uploaded to VRAM. Do NOT call `DrawWholeMapView()` here - the engine calls it later in the load sequence after tilesets are in VRAM, which will render all restored metatiles correctly.

```
for each tilledPlots[i]:
    if not active → skip
    if mapLayoutId != gMapHeader.mapLayoutId → skip
    localId = TILLED_PLOT_LOCAL_ID_BASE + i
    // Always restore the metatile so the tile stays tilled even if the object event can't spawn
    MapGridSetMetatileIdAt(x, y, METATILE_GARDEN_TILLED_SOIL)
    result = SpawnSpecialObjectEventParameterized(
                 OBJ_EVENT_GFX_BERRY_TREE, MOVEMENT_TYPE_BERRY_TREE_GROWTH,
                 localId, x + MAP_OFFSET, y + MAP_OFFSET, 3)
                 // elevation 3 = standard overworld walking layer (matches vanilla berry tree spawns)
    if result == OBJECT_EVENTS_COUNT → continue  // pool full: tile is restored but not interactable; skip silently
    set .trainerRange_berryTreeId = berryTreeId on the spawned event

// No DrawWholeMapView() call - the engine handles this after tilesets are in VRAM
```

### Hook point

- Hooked into the overworld map transition callback in `src/overworld.c` (same pattern as existing berry tree restoration in `src/berry.c`)

---

## Map Structure

### GardenMap

- Dedicated submap, entered via warp from the main farm
- Tileset includes tiles with `MB_SAND` behavior as tillable ground and a tilled-soil tile (`METATILE_GARDEN_TILLED_SOIL`) with `MB_BERRY_TREE_SOIL` behavior
- **GardenMap must contain zero static object events in its map data.** The player and follower Pokémon are spawned dynamically by the engine; all localIds are reserved for dynamic use.
- **Object event budget:** player (1) + follower Pokémon (1) + up to 14 berry tree spots = 16 total (`OBJECT_EVENTS_COUNT`). `CanTill()` caps active plots per map at 14.
- No display Pokémon or additional NPCs on this map - kept sparse by design

### LocalId reservation

localIds 0 and 1 are used by the player and follower. localIds 2–15 are reserved for tilled plots via `TILLED_PLOT_LOCAL_ID_BASE`. This must be documented in a comment in the map's `scripts.inc` to prevent future authors from accidentally assigning static object events.

### Berry harvest and plot lifecycle

When a player harvests all berries from a tree, the vanilla `ObjectEventInteractionRemoveBerryTree` handler despawns the object event. The `tilledPlots[]` entry remains `active = TRUE`. On the next map load, `RestoreTilledPlots` will restore the tilled-soil metatile and attempt to respawn the object event pointing to the now-empty `BerryTree` slot. The `MOVEMENT_TYPE_BERRY_TREE_GROWTH` handler safely handles an empty slot by displaying bare soil and accepting a new berry from the player. This is the intended lifecycle: **tilled soil persists until explicitly untilled (future feature); the player can replant immediately after harvest.**

### `TILLED_PLOTS_COUNT` vs. per-map cap

The save array holds 32 entries to support multiple garden subareas (e.g. 2 × 14 = 28 active plots across two garden maps, with 4 spare). Each individual garden submap is capped at 14 active plots by `CanTill()`.

### Main farm map

- Holds display Pokémon, NPCs, building warps
- No tilled plots - unaffected by this system

### Scalability

- `TilledPlot.mapLayoutId` supports multiple garden subareas in the future
- Each additional garden submap gets its own 14-plot budget within the 16 object event limit

---

## Files Changed

| File | Change |
|---|---|
| `include/global.h` | Add `TilledPlot` struct + `tilledPlots[]` to `SaveBlock1`; add `STATIC_ASSERT` for struct size |
| `src/data/items.h` | Fix `fieldUseFunc` to point to `ItemUseOutOfBattle_Hoe` |
| `src/item_use.c` | Implement `CanTill()`, `ItemUseOutOfBattle_Hoe()`, `ItemUseOnFieldCB_Hoe()`, `RestoreTilledPlots()` |
| `include/item_use.h` | `ItemUseOutOfBattle_Hoe` already declared (no change) |
| `src/overworld.c` | Hook `RestoreTilledPlots()` into map load |
| `include/constants/metatile_labels.h` | Add `METATILE_GARDEN_TILLED_SOIL` once GardenMap tileset is authored |
| `data/maps/GardenMap/` | New map with sand + tilled-soil tiles, zero static object events (PoryMap) |
| `data/layouts/` | New layout entry for GardenMap |
| `include/constants/layouts.h` | New `LAYOUT_GARDEN_MAP` constant |

---

## Out of Scope / Future Work

- Proper item icon for `ITEM_PLOW_TOOL` (currently uses OldRod placeholder)
- Untilling (removing a plot)
- Fertilizer or soil quality modifiers
- Multiple garden subareas beyond the first
- New game start point (currently warps to TestMap - should be changed to real start once farm is built)
- `InFarm()` in `src/item_use.c` currently checks `LAYOUT_TEST_MAP`; must be updated to `LAYOUT_GARDEN_MAP` once the map is authored
