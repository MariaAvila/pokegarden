GardenMap_MapScripts::
	.byte 0

@ ============================================================
@ Farm Mon interaction - triggered when player presses A on
@ a roaming farm mon (set as FarmMonInteractScript in C code)
@ ============================================================

FarmMonInteractScript::
	lockall
	faceplayer
	specialvar VAR_RESULT, GetFarmMonHunger
	compare VAR_RESULT, 0
	goto_if_eq FarmMon_EventScript_NotHungry

@ Mon is hungry
FarmMon_EventScript_Hungry:
	msgbox FarmMon_Text_Hungry, MSGBOX_DEFAULT
	specialvar VAR_RESULT, PlayerHasBerries
	compare VAR_RESULT, TRUE
	goto_if_eq FarmMon_EventScript_OfferBerry
	msgbox FarmMon_Text_NoBerries, MSGBOX_DEFAULT
	goto FarmMon_EventScript_AskRecall

FarmMon_EventScript_NotHungry:
	msgbox FarmMon_Text_Happy, MSGBOX_DEFAULT
	goto FarmMon_EventScript_AskRecall

FarmMon_EventScript_OfferBerry:
	msgbox FarmMon_Text_GiveBerry, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq FarmMon_EventScript_ChooseBerry
	msgbox FarmMon_Text_Declined, MSGBOX_DEFAULT
	goto FarmMon_EventScript_AskRecall

FarmMon_EventScript_ChooseBerry:
	fadescreen FADE_TO_BLACK
	closemessage
	special Bag_ChooseBerry
	waitstate
	compare VAR_ITEM_ID, 0
	goto_if_eq FarmMon_EventScript_AskRecall
	removeitem VAR_ITEM_ID
	special FeedFarmMon
	msgbox FarmMon_Text_Ate, MSGBOX_DEFAULT
	goto FarmMon_EventScript_AskRecall

FarmMon_EventScript_AskRecall:
	msgbox FarmMon_Text_Recall, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq FarmMon_EventScript_Recall
	goto FarmMon_EventScript_End

FarmMon_EventScript_Recall:
	special RecallSelectedFarmMon
	msgbox FarmMon_Text_Recalled, MSGBOX_DEFAULT
	goto FarmMon_EventScript_End

FarmMon_EventScript_End:
	releaseall
	end

@ ============================================================
@ Release Post - lets the player place a party mon on the farm
@ ============================================================

GardenMap_EventScript_ReleasePost::
	lockall
	msgbox GardenMap_Text_ReleasePost_Intro, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	compare VAR_0x8004, PARTY_NOTHING_CHOSEN
	goto_if_eq GardenMap_EventScript_ReleasePost_Cancel
	copyvar VAR_0x8009, VAR_0x8004
	@ Check if this mon is already on the farm
	specialvar VAR_RESULT, IsPartyMonOnFarm
	compare VAR_RESULT, TRUE
	goto_if_eq GardenMap_EventScript_ReleasePost_AlreadyOut
	@ Place the mon
	specialvar VAR_RESULT, PlacePartyMonOnFarm
	compare VAR_RESULT, 0xFF
	goto_if_eq GardenMap_EventScript_ReleasePost_Full
	msgbox GardenMap_Text_ReleasePost_Released, MSGBOX_DEFAULT
	goto GardenMap_EventScript_ReleasePost_End

GardenMap_EventScript_ReleasePost_AlreadyOut:
	msgbox GardenMap_Text_ReleasePost_AlreadyOut, MSGBOX_DEFAULT
	goto GardenMap_EventScript_ReleasePost_End

GardenMap_EventScript_ReleasePost_Full:
	msgbox GardenMap_Text_ReleasePost_Full, MSGBOX_DEFAULT
	goto GardenMap_EventScript_ReleasePost_End

GardenMap_EventScript_ReleasePost_Cancel:
	msgbox GardenMap_Text_ReleasePost_Cancel, MSGBOX_DEFAULT

GardenMap_EventScript_ReleasePost_End:
	releaseall
	end

@ ============================================================
@ Carpenter NPC - sells habitat expansions
@ ============================================================

GardenMap_EventScript_Carpenter::
	lock
	faceplayer
	msgbox GardenMap_Text_Carpenter_Intro, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_Menu

GardenMap_EventScript_Carpenter_Menu:
	@ Water Habitat
	goto_if_set FLAG_HABITAT_WATER_BUILT, GardenMap_EventScript_Carpenter_CheckFire
	msgbox GardenMap_Text_Carpenter_OfferWater, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq GardenMap_EventScript_Carpenter_BuyWater

GardenMap_EventScript_Carpenter_CheckFire:
	@ Fire Habitat
	goto_if_set FLAG_HABITAT_FIRE_BUILT, GardenMap_EventScript_Carpenter_CheckForest
	msgbox GardenMap_Text_Carpenter_OfferFire, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq GardenMap_EventScript_Carpenter_BuyFire

GardenMap_EventScript_Carpenter_CheckForest:
	@ Forest Habitat
	goto_if_set FLAG_HABITAT_FOREST_BUILT, GardenMap_EventScript_Carpenter_CheckCave
	msgbox GardenMap_Text_Carpenter_OfferForest, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq GardenMap_EventScript_Carpenter_BuyForest

GardenMap_EventScript_Carpenter_CheckCave:
	@ Cave Habitat
	goto_if_set FLAG_HABITAT_CAVE_BUILT, GardenMap_EventScript_Carpenter_CheckMystic
	msgbox GardenMap_Text_Carpenter_OfferCave, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq GardenMap_EventScript_Carpenter_BuyCave

GardenMap_EventScript_Carpenter_CheckMystic:
	@ Mystic Habitat
	goto_if_set FLAG_HABITAT_MYSTIC_BUILT, GardenMap_EventScript_Carpenter_AllBuilt
	msgbox GardenMap_Text_Carpenter_OfferMystic, MSGBOX_YESNO
	compare VAR_RESULT, YES
	goto_if_eq GardenMap_EventScript_Carpenter_BuyMystic
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_AllBuilt:
	msgbox GardenMap_Text_Carpenter_AllBuilt, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

@ --- Purchase handlers ---

GardenMap_EventScript_Carpenter_BuyWater:
	showmoneybox 0, 0
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, GardenMap_EventScript_Carpenter_NoMoney
	removemoney 3000
	hidemoneybox
	setflag FLAG_HABITAT_WATER_BUILT
	msgbox GardenMap_Text_Carpenter_Built, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_BuyFire:
	showmoneybox 0, 0
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, GardenMap_EventScript_Carpenter_NoMoney
	removemoney 3000
	hidemoneybox
	setflag FLAG_HABITAT_FIRE_BUILT
	msgbox GardenMap_Text_Carpenter_Built, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_BuyForest:
	showmoneybox 0, 0
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, GardenMap_EventScript_Carpenter_NoMoney
	removemoney 3000
	hidemoneybox
	setflag FLAG_HABITAT_FOREST_BUILT
	msgbox GardenMap_Text_Carpenter_Built, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_BuyCave:
	showmoneybox 0, 0
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, GardenMap_EventScript_Carpenter_NoMoney
	removemoney 3000
	hidemoneybox
	setflag FLAG_HABITAT_CAVE_BUILT
	msgbox GardenMap_Text_Carpenter_Built, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_BuyMystic:
	showmoneybox 0, 0
	checkmoney 3000
	goto_if_eq VAR_RESULT, FALSE, GardenMap_EventScript_Carpenter_NoMoney
	removemoney 3000
	hidemoneybox
	setflag FLAG_HABITAT_MYSTIC_BUILT
	msgbox GardenMap_Text_Carpenter_Built, MSGBOX_DEFAULT
	goto GardenMap_EventScript_Carpenter_End

GardenMap_EventScript_Carpenter_NoMoney:
	hidemoneybox
	msgbox GardenMap_Text_Carpenter_NoMoney, MSGBOX_DEFAULT

GardenMap_EventScript_Carpenter_End:
	release
	end

@ ============================================================
@ Habitat warp gates - block entry if habitat not purchased
@ ============================================================

GardenMap_EventScript_WarpWater::
	goto_if_set FLAG_HABITAT_WATER_BUILT, GardenMap_EventScript_WarpWater_Enter
	msgbox GardenMap_Text_HabitatLocked, MSGBOX_DEFAULT
	end
GardenMap_EventScript_WarpWater_Enter:
	warp MAP_FARM_HABITAT_WATER, 255, 7, 12
	waitstate
	end

GardenMap_EventScript_WarpFire::
	goto_if_set FLAG_HABITAT_FIRE_BUILT, GardenMap_EventScript_WarpFire_Enter
	msgbox GardenMap_Text_HabitatLocked, MSGBOX_DEFAULT
	end
GardenMap_EventScript_WarpFire_Enter:
	warp MAP_FARM_HABITAT_FIRE, 255, 7, 12
	waitstate
	end

GardenMap_EventScript_WarpForest::
	goto_if_set FLAG_HABITAT_FOREST_BUILT, GardenMap_EventScript_WarpForest_Enter
	msgbox GardenMap_Text_HabitatLocked, MSGBOX_DEFAULT
	end
GardenMap_EventScript_WarpForest_Enter:
	warp MAP_FARM_HABITAT_FOREST, 255, 7, 12
	waitstate
	end

GardenMap_EventScript_WarpCave::
	goto_if_set FLAG_HABITAT_CAVE_BUILT, GardenMap_EventScript_WarpCave_Enter
	msgbox GardenMap_Text_HabitatLocked, MSGBOX_DEFAULT
	end
GardenMap_EventScript_WarpCave_Enter:
	warp MAP_FARM_HABITAT_CAVE, 255, 7, 12
	waitstate
	end

GardenMap_EventScript_WarpMystic::
	goto_if_set FLAG_HABITAT_MYSTIC_BUILT, GardenMap_EventScript_WarpMystic_Enter
	msgbox GardenMap_Text_HabitatLocked, MSGBOX_DEFAULT
	end
GardenMap_EventScript_WarpMystic_Enter:
	warp MAP_FARM_HABITAT_MYSTIC, 255, 7, 12
	waitstate
	end

@ ============================================================
@ Strings
@ ============================================================

@ Farm Mon strings
FarmMon_Text_Hungry:
	.string "This Pokémon looks hungry!\nMaybe give it a berry?$"

FarmMon_Text_Happy:
	.string "This Pokémon looks happy\nand well-fed!$"

FarmMon_Text_NoBerries:
	.string "You don't have any berries\nto offer...$"

FarmMon_Text_GiveBerry:
	.string "Do you want to give this\nPokémon a berry?$"

FarmMon_Text_Declined:
	.string "The Pokémon watches you\nhopefully...$"

FarmMon_Text_Ate:
	.string "The Pokémon ate the berry\nhappily!$"

FarmMon_Text_Recall:
	.string "Do you want to call this\nPokémon back?$"

FarmMon_Text_Recalled:
	.string "The Pokémon returned\nto your care!$"

@ Release Post strings
GardenMap_Text_ReleasePost_Intro:
	.string "Which Pokémon do you want\nto let roam the farm?$"

GardenMap_Text_ReleasePost_Released:
	.string "Your Pokémon is now roaming\nthe farm freely!$"

GardenMap_Text_ReleasePost_AlreadyOut:
	.string "That Pokémon is already\nroaming the farm!$"

GardenMap_Text_ReleasePost_Full:
	.string "The farm is full!\nRecall a Pokémon first.$"

GardenMap_Text_ReleasePost_Cancel:
	.string "Come back when you want\nto release a Pokémon!$"

@ Carpenter strings
GardenMap_Text_Carpenter_Intro:
	.string "Howdy! I can build new habitats\nfor your Pokémon. Take a look!$"

GardenMap_Text_Carpenter_OfferWater:
	.string "A Water & Ice Habitat!\nPerfect for Water and Ice types.\nBuild it for ¥3000?$"

GardenMap_Text_Carpenter_OfferFire:
	.string "A Fire & Dragon Habitat!\nA heated haven for Fire and\nDragon types. Build for ¥3000?$"

GardenMap_Text_Carpenter_OfferForest:
	.string "A Forest Habitat!\nIdeal for Bug, Poison, Fairy\nand Flying types. Build for ¥3000?$"

GardenMap_Text_Carpenter_OfferCave:
	.string "A Cave Habitat!\nRugged home for Rock, Ground,\nSteel and Fighting types. ¥3000?$"

GardenMap_Text_Carpenter_OfferMystic:
	.string "A Mystic Habitat!\nA mysterious place for Psychic,\nGhost, Dark and Electric. ¥3000?$"

GardenMap_Text_Carpenter_AllBuilt:
	.string "You've built all the habitats!\nYour farm is truly magnificent.$"

GardenMap_Text_Carpenter_Built:
	.string "Consider it done! Your new\nhabitat is ready to use.$"

GardenMap_Text_Carpenter_NoMoney:
	.string "Hmm, you don't have enough\nmoney for that right now.$"

@ Warp strings
GardenMap_Text_HabitatLocked:
	.string "This habitat hasn't been built yet.\nTalk to the Carpenter!$"
