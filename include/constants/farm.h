#ifndef GUARD_CONSTANTS_FARM_H
#define GUARD_CONSTANTS_FARM_H

// Habitat purchase flags (unused flags repurposed)
#define FLAG_HABITAT_WATER_BUILT    0x022
#define FLAG_HABITAT_FIRE_BUILT     0x023
#define FLAG_HABITAT_FOREST_BUILT   0x024
#define FLAG_HABITAT_CAVE_BUILT     0x025
#define FLAG_HABITAT_MYSTIC_BUILT   0x026

// Habitat indices (index into gHabitatInfo table)
#define HABITAT_MAIN    0
#define HABITAT_WATER   1
#define HABITAT_FIRE    2
#define HABITAT_FOREST  3
#define HABITAT_CAVE    4
#define HABITAT_MYSTIC  5

// Hunger constants
#define HUNGER_MAX          63
#define HUNGER_MINUTES_PER_TICK  30  // real minutes between hunger increments (no 240x multiplier for hunger)

#endif // GUARD_CONSTANTS_FARM_H
