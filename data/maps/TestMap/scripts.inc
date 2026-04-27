TestMap_MapScripts::
	map_script MAP_SCRIPT_ON_LOAD, TestMap_OnLoad
	.byte 0

TestMap_OnLoad:
	goto_if_set FLAG_SYS_CLOCK_SET, TestMap_OnLoad_End
	initclock 12, 0
TestMap_OnLoad_End:
	end
