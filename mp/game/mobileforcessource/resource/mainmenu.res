"MainMenu"
{	
	"say ready"
	{
		"text"			"#MFS_Vote_Restart"
		"description"	"#MFS_Vote_Restart"
		"command"		"cvar say ready"
		"priority"		"12"
		"specifics"		"ingamemp"
	}
	
	" "
	{
		"text"			" "
		"description"	""
		"command"		""
		"priority"		"11"
		"specifics"		"ingamemp"
	}

	"ResumeGame"
	{
		"text"			"#RG"
		"description"	"#GameUI2_ResumeGameDescription"
		"command"		"cvar gamemenucommand resumegame"
		"priority"		"10"
		"specifics"		"ingame"
	}
	
	"Disconnect"
	{
		"text"			"#DC"
		"description"	"#DC"
		"command"		"cvar gamemenucommand Disconnect"
		"priority"		"9"
		"specifics"		"ingame"
	}
	
	"OpenPlayerListDialog"
	{
		"text"			"#PL"
		"description"	"#PL"
		"command"		"cvar gamemenucommand OpenPlayerListDialog"
		"priority"		"8"
		"specifics"		"ingamemp"
	}
	
	"Singleplayer"
	{
		"text"			"#MFS_SP"
		"description"	""
		"command"		"cvar gameui2_openspdialog"
		"priority"		"7"
		"specifics"		"mainmenu"
	}
	
	"Multiplayer"
	{
		"text"			"#MFS_MP"
		"description"	""
		"command"		"cvar gameui2_openmpdialog"
		"priority"		"6"
		"specifics"		"mainmenu"
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
	
	"Mp3"
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
		"specifics"		"mainmenu"
	}
}