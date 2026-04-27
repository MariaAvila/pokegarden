#ifndef GUARD_FARM_MONS_H
#define GUARD_FARM_MONS_H

void RestoreFarmMons(void);
u8   GetTotalFarmMonTemplateCount(void);
u8   PlaceFarmMon(u8 boxNum, u8 boxPos, u8 x, u8 y);
void RecallFarmMon(u8 slotIndex);
u8   GetFarmMonSlotForObjectEvent(u8 objectEventId);
u16  GetFarmMonHunger(void);
void FeedFarmMon(void);
void RecallSelectedFarmMon(void);
u16  PlacePartyMonOnFarm(void);
u16  IsPartyMonOnFarm(void);
bool8 FarmMonHasHabitatBonus(u8 slotIndex);
void UpdateFarmMonHabitatBonuses(s32 minutes);
void UpdateFarmMonHunger(s32 minutes);

#endif // GUARD_FARM_MONS_H
