"MainMenu"
{	
	"cvar say ready"
	{
		"text"			"#MFS_Vote_Restart"
		"description"	"#MFS_Vote_Restart"
		"command"		"cvar say ready"
		"priority"		"17"
		"specifics"		"ingame"
	}
	
	" "
	{
		"text"			" "
		"description"	""
		"command"		""
		"priority"		"16"
		"specifics"		"ingame"
	}

	"ResumeGame"
	{
		"text"			"#RG"
		"description"	"#GameUI2_ResumeGameDescription"
		"command"		"cvar gamemenucommand resumegame"
		"priority"		"15"
		"specifics"		"ingame"
	}
	
	"Disconnect"
	{
		"text"			"#DC"
		"description"	"#DC"
		"command"		"cvar gamemenucommand Disconnect"
		"priority"		"14"
		"specifics"		"ingame"
	}
	
	"OpenPlayerListDialog"
	{
		"text"			"#PL"
		"description"	"#PL"
		"command"		"cvar gamemenucommand OpenPlayerListDialog"
		"priority"		"13"
		"specifics"		"ingame"
	}
	" "
	{
		"text"			" "
		"description"	""
		"command"		""
		"priority"		"12"
		"specifics"		"ingame"
	}
	
	"NewGame"
	{
		"text"			"#NG"
		"description"	"#GameUI2_NewGameDescription"
		"command"		"cvar gamemenucommand opennewgamedialog"
		"priority"		"11"
		"specifics"		"mainmenu"
	}
	
	"LoadGame"
	{
		"text"			"#LG"
		"description"	"#GameUI2_LoadGameDescription"
		"command"		"cvar gamemenucommand openloadgamedialog"
		"priority"		"10"
		"specifics"		"shared"
	}
	
	"SaveGame"
	{
		"text"			"#SG"
		"description"	"#GameUI2_SaveGameDescription"
		"command"		"cvar gamemenucommand opensavegamedialog"
		"priority"		"9"
		"specifics"		"ingame"
	}
	
	" "
	{
		"text"			" "
		"description"	""
		"command"		""
		"priority"		"8"
		"specifics"		"shared"
	}
	
	"OpenServerBrowser"
	{
		"text"			"#FS"
		"description"	"#FS"
		"command"		"cvar gamemenucommand OpenServerBrowser"
		"priority"		"7"
		"specifics"		"shared"
	}
	
	"OpenCreateMultiplayerGameDialog"
	{
		"text"			"#CS"
		"description"	"#CS"
		"command"		"cvar gamemenucommand OpenCreateMultiplayerGameDialog"
		"priority"		"6"
		"specifics"		"shared"
	}
	
	" "
	{
		"text"			" "
		"description"	""
		"command"		""
		"priority"		"5"
		"specifics"		"shared"
	}
	
	"Options"
	{
		"text"			"#OT"
		"description"	"#GameUI2_OptionsDescription"
		"command"		"cvar gamemenucommand openoptionsdialog" //gameui2_openoptionsdialog
		"priority"		"4"
		"specifics"		"shared"
	}

	"Quit"
	{
		"text"			"#QT"
		"description"	"#GameUI2_QuitDescription"
		"command"		"cvar gamemenucommand quit"
		"priority"		"3"
		"specifics"		"shared"
	}
	
	"cvar mp3"
	{
		"text"			"#MFS_MP3Player"
		"description"	"#MFS_MP3Player"
		"command"		"cvar mp3"
		"priority"		"2"
		"specifics"		"shared"
	}
	
	"OpenAchievementsDialog"
	{
		"text"			"#AM"
		"description"	"#AM"
		"command"		"cvar gamemenucommand OpenAchievementsDialog"
		"priority"		"1"
		"specifics"		"shared"
	}
}