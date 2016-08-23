//******************************************************************
// Multiplayer AI for Source engine by R_Yell - rebel.y3ll@gmail.com
//******************************************************************

#include "cbase.h"

#ifdef MFS

#include "player.h"
#include "hl2mp_player.h"

#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"

#include "bot_main.h"
#include "bot_combat.h"
#include "bot_navigation.h"

// support for nav mesh
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_area.h"

class CHL2MP_Bot;
void Bot_Think( CHL2MP_Bot *pBot );

ConVar bot_forcefireweapon("bot_forcefireweapon", "", FCVAR_CHEAT, "Force bots with the specified weapon to fire.");
ConVar bot_forceattack2("bot_forceattack2", "0", FCVAR_CHEAT, "When firing, use attack2.");
ConVar bot_forceattackon("bot_forceattackon", "0", FCVAR_CHEAT, "When firing, don't tap fire, hold it down.");
ConVar bot_flipout("bot_flipout", "0", FCVAR_CHEAT, "When on, all bots fire their guns.");
ConVar bot_defend("bot_defend", "0", 0, "Set to a team number, and that team will all keep their combat shields raised.");
//ConVar bot_changeclass("bot_changeclass", "0", FCVAR_CHEAT, "Force all bots to change to the specified class.");
ConVar bot_zombie("bot_zombie", "0", FCVAR_CHEAT, "Brraaaaaiiiins.");
ConVar bot_mimic_yaw_offset("bot_mimic_yaw_offset", "0", FCVAR_CHEAT, "Offsets the bot yaw.");
ConVar bot_attack("bot_attack", "0", FCVAR_CHEAT, "Shoot!");

ConVar bot_sendcmd("bot_sendcmd", "", FCVAR_CHEAT, "Forces bots to send the specified command.");

ConVar bot_crouch("bot_crouch", "0", FCVAR_CHEAT, "Bot crouches");

#ifdef NEXT_BOT
extern ConVar bot_mimic;
#else
ConVar bot_mimic("bot_mimic", "0", FCVAR_CHEAT, "Bot uses usercmd of player by index.");
#endif

void bot_kick_f (const CCommand &args)
{
	int name = atoi(args.Arg(1));
	if ( name )
	{
		engine->ServerCommand(UTIL_VarArgs("kick %s\n", name));
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));

		// Ignore plugin bots
		if (pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT))
		{
			CHL2MP_Bot *pBot = dynamic_cast<CHL2MP_Bot*>(pPlayer);
			if (pBot)
			{
				engine->ClientCommand(pBot->edict(), "disconnect");
			}
		}
	}
}

ConCommand bot_kick("bot_kick", bot_kick_f, "kick a bot", FCVAR_SERVER_CAN_EXECUTE);

static int g_CurBotNumber = 1;

int g_iLastHideSpot = 0;
int g_iLastBotName = 0;

/*CHL2MP_Bot::~CHL2MP_Bot( void )
{
	//g_CurBotNumber--; //FixMe, Crashes the game
}*/

// all bot names
const char *g_ppszRandomBotNames[] =
{
	"Heywood",
	"Coffey",
	"Jackson",
	"Dillon",
	"Lydecker",
	"Carter",
	"Gorman",
	"Powell",
	"Sanchez",
	"Bennings",
	"Bowman",
	"Wychek",
	"Scagnetti",
	"Kurtz",
	"Harris",
	"Palmer",
	"Leyden",
	"Baxter",
	"Thomson",
	"Banks",
	"Good",
	"Hodgson",
	"Hall",
	"Macdonald",
	"Henderson",
	"Williams",
	"Hughes",
	"Hewitt",
	"Jones",
	"Biltcliffe",
	"Donbavand",
	"Fitzsimmons",
};

// Blue bot names
const char *g_ppszRandomBlueBotNames[] =
{
	"Heywood",
	"Jackson",
	"Lydecker",
	"Gorman",
	"Sanchez",
	"Bowman",
	"Scagnetti",
	"Harris",
	"Leyden",
	"Thomson",
	"Good",
	"Hall",
	"Henderson",
	"Hughes",
	"Jones",
	"Donbavand",
};

// Red bot names
const char *g_ppszRandomRedBotNames[] =
{
	"Coffey",
	"Dillon",
	"Carter",
	"Powell",
	"Bennings",
	"Wychek",
	"Kurtz",
	"Palmer",
	"Baxter",
	"Banks",
	"Hodgson",
	"Macdonald",
	"Williams",
	"Hewitt",
	"Biltcliffe",
	"Fitzsimmons",
};

LINK_ENTITY_TO_CLASS( bot, CHL2MP_Bot );

class CBotManager
{
public:
	static CBasePlayer* ClientPutInServerOverride_Bot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CHL2MP_Bot *pPlayer = static_cast<CHL2MP_Bot *>( CreateEntityByName( "bot" ) );
		if ( pPlayer )
		{
			pPlayer->SetPlayerName( playername );
		}

		return pPlayer;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer(bool  bFrozen, int iTeam)
{
	const char *botname = NULL;
	char botname2[64];
	if (HL2MPRules()->IsTeamplay() == false)
	{
		int nHeads = ARRAYSIZE(g_ppszRandomBotNames);
		if (nHeads >= 1)
		{
			g_iLastBotName = (g_iLastBotName + 1) % nHeads;
			botname = g_ppszRandomBotNames[g_iLastBotName];

			for (int i = 0; i < nHeads; i++) // Loop to find the item to delete.
			{
				if (g_ppszRandomBotNames[i] == botname) // If we find the item to delete...
				{
					for (int j = i; j < nHeads - 1; j++) // Iterate through the remaining elements, stopping one before the end.
					{
						g_ppszRandomBotNames[j] = g_ppszRandomBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
					}
					g_ppszRandomBotNames[nHeads - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
					nHeads--; // Reduce the array size by one.
					break; // Exit out of the 'find item to delete' loop.	
				}
			}
		}
		else
		{
			Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber);
		}
	}
	else
	{
		if (iTeam == TEAM_COMBINE)
		{
			int nHeads = ARRAYSIZE(g_ppszRandomBlueBotNames);
			if (nHeads >= 1)
			{
				g_iLastBotName = (g_iLastBotName + 1) % nHeads;
				botname = g_ppszRandomBlueBotNames[g_iLastBotName];

				for (int i = 0; i < nHeads; i++) // Loop to find the item to delete.
				{
					if (g_ppszRandomBlueBotNames[i] == botname) // If we find the item to delete...
					{
						for (int j = i; j < nHeads - 1; j++) // Iterate through the remaining elements, stopping one before the end.
						{
							g_ppszRandomBlueBotNames[j] = g_ppszRandomBlueBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
						}
						g_ppszRandomBlueBotNames[nHeads - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
						nHeads--; // Reduce the array size by one.
						break; // Exit out of the 'find item to delete' loop.	
					}
				}
			}
			else
			{
				Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber);
			}
		}
		else
		{
			int nHeads = ARRAYSIZE(g_ppszRandomRedBotNames);
			if (nHeads >= 1)
			{
				g_iLastBotName = (g_iLastBotName + 1) % nHeads;
				botname = g_ppszRandomRedBotNames[g_iLastBotName];

				for (int i = 0; i < nHeads; i++) // Loop to find the item to delete.
				{
					if (g_ppszRandomRedBotNames[i] == botname) // If we find the item to delete...
					{
						for (int j = i; j < nHeads - 1; j++) // Iterate through the remaining elements, stopping one before the end.
						{
							g_ppszRandomRedBotNames[j] = g_ppszRandomRedBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
						}
						g_ppszRandomRedBotNames[nHeads - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
						nHeads--; // Reduce the array size by one.
						break; // Exit out of the 'find item to delete' loop.	
					}
				}
			}
			else
			{
				Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber);
			}
		}
	}

	// This trick lets us create a CHL2MP_Bot for this client instead of the CHL2MP_Player
	// that we would normally get when ClientPutInServer is called.
	ClientPutInServerOverride(&CBotManager::ClientPutInServerOverride_Bot);

	if (botname != NULL)
	{
		edict_t *pEdict = engine->CreateFakeClient(botname);
		ClientPutInServerOverride(NULL);

		if (!pEdict)
		{
			Msg("Failed to create Bot (no edict available)\n");
			return NULL;
		}

		/*int pkvWeapon_1_Value;
		int Weapon_1_PriClip_Value;
		int Weapon_1_SecClip_Value;*/

		// Allocate a player entity for the bot, and call spawn
		CHL2MP_Bot *pPlayer = ((CHL2MP_Bot*)CBaseEntity::Instance(pEdict));

		pPlayer->ClearFlags();
		pPlayer->AddFlag(FL_CLIENT | FL_FAKECLIENT);

		if (bFrozen)
			pPlayer->AddEFlags(EFL_BOT_FROZEN);

		//pPlayer->ChangeTeam( iTeam ); // Modified Hl2DM's pickdefaultspawnteam to not check for bots
		//pPlayer->RemoveAllItems( true ); //Why

		// Spawn() doesn't work with MP template codebase, even if this line is part of default bot template...
		//pPlayer->Spawn();

		CCommand args;
		args.Tokenize("joingame");
		pPlayer->ClientCommand(args);

		// set bot skills
		pPlayer->m_flSkill[BOT_SKILL_YAW_RATE] = random->RandomFloat(SKILL_MIN_YAW_RATE, SKILL_MAX_YAW_RATE);
		pPlayer->m_flSkill[BOT_SKILL_WALK_SPEED] = random->RandomFloat(SKILL_MIN_WALK_SPEED, SKILL_MAX_WALK_SPEED);
		pPlayer->m_flSkill[BOT_SKILL_RUN_SPEED] = random->RandomFloat(SKILL_MIN_RUN_SPEED, SKILL_MAX_RUN_SPEED);
		pPlayer->m_flSkill[BOT_SKILL_STRAFE] = random->RandomFloat(SKILL_MIN_STRAFE, SKILL_MAX_STRAFE);

		//pPlayer->CBasePlayer::SetNormSpeed( BOT_SKILL_WALK_SPEED );
		//pPlayer->CBasePlayer::SetSprintSpeed( BOT_SKILL_RUN_SPEED );

		/*if ( pkvWeapon_1_Value != 0 )
		{
		pPlayer->GiveNamedItem( (pkvWeapon_1_Value) );
		pPlayer->CBasePlayer::GiveAmmo( Weapon_1_PriClip_Value,	pkvWeapon_1_PriClipAmmo_Value );
		pPlayer->CBasePlayer::GiveAmmo( Weapon_1_SecClip_Value,	pkvWeapon_1_SecClipAmmo_Value );
		}
		else
		{
		//This is gonna be really fun
		}*/

		g_CurBotNumber++;

		return pPlayer;
	}
	else
	{
		edict_t *pEdict = engine->CreateFakeClient(botname2);
		ClientPutInServerOverride(NULL);

		if (!pEdict)
		{
			Msg("Failed to create Bot (no edict available)\n");
			return NULL;
		}

		/*int pkvWeapon_1_Value;
		int Weapon_1_PriClip_Value;
		int Weapon_1_SecClip_Value;*/

		// Allocate a player entity for the bot, and call spawn
		CHL2MP_Bot *pPlayer = ((CHL2MP_Bot*)CBaseEntity::Instance(pEdict));

		pPlayer->ClearFlags();
		pPlayer->AddFlag(FL_CLIENT | FL_FAKECLIENT);

		if (bFrozen)
			pPlayer->AddEFlags(EFL_BOT_FROZEN);

		//pPlayer->ChangeTeam( iTeam ); // Modified Hl2DM's pickdefaultspawnteam to not check for bots
		//pPlayer->RemoveAllItems( true ); //Why

		// Spawn() doesn't work with MP template codebase, even if this line is part of default bot template...
		//pPlayer->Spawn();

		CCommand args;
		args.Tokenize("joingame");
		pPlayer->ClientCommand(args);

		// set bot skills
		pPlayer->m_flSkill[BOT_SKILL_YAW_RATE] = random->RandomFloat(SKILL_MIN_YAW_RATE, SKILL_MAX_YAW_RATE);
		pPlayer->m_flSkill[BOT_SKILL_WALK_SPEED] = random->RandomFloat(SKILL_MIN_WALK_SPEED, SKILL_MAX_WALK_SPEED);
		pPlayer->m_flSkill[BOT_SKILL_RUN_SPEED] = random->RandomFloat(SKILL_MIN_RUN_SPEED, SKILL_MAX_RUN_SPEED);
		pPlayer->m_flSkill[BOT_SKILL_STRAFE] = random->RandomFloat(SKILL_MIN_STRAFE, SKILL_MAX_STRAFE);

		//pPlayer->CBasePlayer::SetNormSpeed( BOT_SKILL_WALK_SPEED );
		//pPlayer->CBasePlayer::SetSprintSpeed( BOT_SKILL_RUN_SPEED );

		/*if ( pkvWeapon_1_Value != 0 )
		{
		pPlayer->GiveNamedItem( (pkvWeapon_1_Value) );
		pPlayer->CBasePlayer::GiveAmmo( Weapon_1_PriClip_Value,	pkvWeapon_1_PriClipAmmo_Value );
		pPlayer->CBasePlayer::GiveAmmo( Weapon_1_SecClip_Value,	pkvWeapon_1_SecClipAmmo_Value );
		}
		else
		{
		//This is gonna be really fun
		}*/

		g_CurBotNumber++;

		return pPlayer;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{	
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );

		// Ignore plugin bots
		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT)/* && !pPlayer->IsEFlagSet( EFL_PLUGIN_BASED_BOT )*/ ) //FixMe
		{
			CHL2MP_Bot *pBot = dynamic_cast< CHL2MP_Bot* >( pPlayer );
			if ( pBot )
			{
				Bot_Think( (CHL2MP_Bot *)pPlayer );
			}
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			m_flSideMove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove(CHL2MP_Player *fakeclient, CUserCmd &cmd, const QAngle& viewangles, unsigned short buttons, float frametime)
{
	if ( !fakeclient )
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	Q_memset(&cmd, 0, sizeof(cmd));

	//if (bot_flipout.GetBool())
		//VectorCopy(viewangles, cmd.viewangles);

	if (bot_crouch.GetInt())
		cmd.buttons |= IN_DUCK;

	if (bot_attack.GetBool())
		cmd.buttons |= IN_ATTACK;

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}

void Bot_HandleRespawn( CHL2MP_Bot *pBot, CUserCmd &cmd )
{		
	// Try hitting my buttons occasionally
	if ( random->RandomInt( 0, 100 ) > 80 )
	{
		// Respawn the bot
		if ( random->RandomInt( 0, 1 ) == 0 )
		{
			cmd.buttons |= IN_JUMP;
		}
		else
		{
			cmd.buttons = 0;
		}
	}	
}

// here bot updates important info that is used multiple times along the thinking process
void BotInfoGathering( CHL2MP_Bot *pBot )
{	
	pBot->m_flBotToEnemyDist = (pBot->GetLocalOrigin() - pBot->GetEnemy()->GetLocalOrigin()).Length();

	trace_t tr;
	UTIL_TraceHull(  pBot->EyePosition(), pBot->GetEnemy()->EyePosition() - Vector(0,0,20), -BotTestHull, BotTestHull, MASK_SHOT, pBot, COLLISION_GROUP_NONE, &tr );
	
	if( tr.m_pEnt == pBot->GetEnemy() ) // vision line between both
		pBot->m_bEnemyOnSights = true;
	else
		pBot->m_bEnemyOnSights = false;

	pBot->m_bInRangeToAttack = (pBot->m_flBotToEnemyDist < pBot->m_flMinRangeAttack) && pBot->FInViewCone( pBot->GetEnemy() ); 

	pBot->m_flDistTraveled += fabs(pBot->GetLocalVelocity().Length()); // this is used for stuck checking, 

	pBot->m_flHeightDifToEnemy = pBot->GetLocalOrigin().z - pBot->GetEnemy()->GetLocalOrigin().z;
}
	
void CHL2MP_Bot::Update(int mode)
{
	CBaseEntity *pSpot = NULL;
	const char *OldHideSpot = pHideSpot;

	if (mode == 0) // in any other cases we're calling this with a mode pre-determined
	{
		if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_DEATHMATCH)) == NULL)
		{
			if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_BLUE)) == NULL)
			{
				if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_RED)) == NULL)
					mode = 7;// This map sucks
				else
					// This map is racist
					mode = 8;
			}
			else if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_RED)) == NULL)
			{
				if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_BLUE)) == NULL)
					mode = 7;// This map sucks
				else
					// This map is racist
					mode = 9;
			}
			if (mode < 7)
			{
				if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_CAPTAIN_BLUE)) == NULL)
				{
					if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_CAPTAIN_RED)) == NULL)
						mode = 3;
					else
						mode = 4;

				}
				else if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_CAPTAIN_RED)) == NULL)
				{
					if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_CAPTAIN_BLUE)) == NULL)
						mode = 3;
					else
						mode = 5;

				}
				if (mode < 3)
				{
					mode = 2;
				}
			}
		}

		if (mode < 2)
		{
			if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_BLUE)) == NULL && (pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_RED)) == NULL)
				mode = 6;
			else
				mode = 1;
		}
	}

	if (mode == 1)
	{
		int troll = random->RandomInt(1, 5);

		if (troll == 2)
			pHideSpot = SPAWN_POINT_DEATHMATCH;
		else if (troll == 2)
			pHideSpot = SPAWN_POINT_RED;
		else if (troll == 3)
			pHideSpot = SPAWN_POINT_BLUE;
		else if (troll == 4)
			pHideSpot = SPAWN_POINT_CAPTAIN_BLUE;
		else if (troll == 5)
			pHideSpot = SPAWN_POINT_CAPTAIN_RED;

		if (pHideSpot == OldHideSpot)
			Update(1);
	}
	else if (mode == 2) //Highly unlikely, cuz every MFS mapper should include deathmatch spawns
	{
		int troll = random->RandomInt(2, 5);

		if (troll == 2)
			pHideSpot = SPAWN_POINT_RED;
		else if (troll == 3)
			pHideSpot = SPAWN_POINT_BLUE;
		else if (troll == 4)
			pHideSpot = SPAWN_POINT_CAPTAIN_BLUE;
		else if (troll == 5)
			pHideSpot = SPAWN_POINT_CAPTAIN_RED;

		if (pHideSpot == OldHideSpot)
			Update(2);
	}
	else if (mode == 3)
	{
		int troll = random->RandomInt(2, 3);

		if ( troll == 2 )
			pHideSpot = SPAWN_POINT_RED;
		else if ( troll == 3 )
			pHideSpot = SPAWN_POINT_BLUE;

		if (pHideSpot == OldHideSpot)
			Update(3);
	}
	else if (mode == 4) // i also dunno why would this ever happen
	{
		int troll = random->RandomInt(2, 4);

		if (troll == 2)
			pHideSpot = SPAWN_POINT_RED;
		else if (troll == 3)
			pHideSpot = SPAWN_POINT_BLUE;
		else if (troll == 4)
			pHideSpot = SPAWN_POINT_RED;

		if (pHideSpot == OldHideSpot)
			Update(4);
	}
	else if (mode == 5) // or this
	{
		int troll = random->RandomInt(2, 4);

		if (troll == 2)
			pHideSpot = SPAWN_POINT_RED;
		else if (troll == 3)
			pHideSpot = SPAWN_POINT_BLUE;
		else if (troll == 4)
			pHideSpot = SPAWN_POINT_BLUE;

		if (pHideSpot == OldHideSpot)
			Update(5);
	}
	else if (mode == 6)
	{
		pHideSpot = SPAWN_POINT_DEATHMATCH;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(6);*/
	}
	else if (mode == 7) // This actually happends
	{
		pHideSpot = "info_player_start";

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(7);*/
	}
	else if (mode == 8)
	{
		pHideSpot = SPAWN_POINT_RED;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(8);*/
	}
	else if (mode == 9)
	{
		pHideSpot = SPAWN_POINT_BLUE;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(9);*/
	}

	UpdateTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Run this Bot's AI for one tick.
//-----------------------------------------------------------------------------
void Bot_Think( CHL2MP_Bot *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );

	if ( pBot->IsEFlagSet(EFL_BOT_FROZEN) )
		return;

	if (pBot->ShouldUpdate() == true)
		pBot->Update(0);

	//QAngle vecViewAngles;
	unsigned short buttons = 0;

	//vecViewAngles = pBot->GetLocalAngles();

	CUserCmd cmd;
	Q_memset( &cmd, 0, sizeof( cmd ) );	

	if ( !pBot->IsAlive() )
	{
		Bot_HandleRespawn( pBot, cmd );
	}
	else
	{
		trace_t tr_front;
		Vector Forward;
		AngleVectors(pBot->GetLocalAngles(), &Forward);
		UTIL_TraceHull( pBot->GetLocalOrigin()+Vector(0,0,5), pBot->GetLocalOrigin() + Vector(0,0,5) + (Forward * 50), pBot->GetPlayerMins(), pBot->GetPlayerMaxs(), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &tr_front );

		// Is my team being forced to defend?
		if (bot_defend.GetInt() == pBot->GetTeamNumber())
		{
			buttons |= IN_ATTACK2;
		}
		// If bots are being forced to fire a weapon, see if I have it
		else if (bot_forcefireweapon.GetString())
		{
			CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType(bot_forcefireweapon.GetString());
			if (pWeapon)
			{
				// Switch to it if we don't have it out
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

				// Switch?
				if (pActiveWeapon != pWeapon)
				{
					pBot->Weapon_Switch(pWeapon);
				}
				else
				{
					// Start firing
					// Some weapons require releases, so randomise firing
					if (bot_forceattackon.GetBool() || (RandomFloat(0.0, 1.0) > 0.5))
					{
						buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
					}
				}
			}
		}

		if (bot_flipout.GetInt())
		{
			if (bot_forceattackon.GetBool() || (RandomFloat(0.0, 1.0) > 0.5))
			{
				buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
			}
		}

		if (strlen(bot_sendcmd.GetString()) > 0)
		{
			//send the cmd from this bot
			CCommand args;
			args.Tokenize(bot_sendcmd.GetString());
			pBot->ClientCommand(args);

			bot_sendcmd.SetValue("");
		}

		// enemy acquisition
		if( !pBot->GetEnemy() || pBot->RecheckEnemy() || !pBot->GetEnemy()->IsAlive() )
		{
			if( pBot->GetEnemy() && !pBot->GetEnemy()->IsAlive() )
				pBot->ResetNavigationParams();

			if( !AcquireEnemy( pBot ) )
				return;
	
			pBot->m_flTimeToRecheckEnemy = gpGlobals->curtime + 1.0f;
		}

		// assume we have an enemy from now on

		BotInfoGathering( pBot );

		BotAttack( pBot, cmd );

		if( pBot->m_flTimeToRecheckStuck < gpGlobals->curtime )
			CheckStuck( pBot, cmd );
		
		if( pBot->m_flNextDealObstacles < gpGlobals->curtime )
			DealWithObstacles( pBot, tr_front.m_pEnt, cmd );	

		BotNavigation( pBot, cmd );

		CheckNavMeshAttrib(pBot, &tr_front, cmd);		
	}

	// debug waypoint related position
	/*for( int i=0; i<pBot->m_Waypoints.Count(); i++ )
	{
		NDebugOverlay::Cross3DOriented( pBot->m_Waypoints[i].Center, QAngle(0,0,0), 5*i+1, 200, 0, 0, false, -1 );
	}*/

	/*if (bot_flipout.GetInt() >= 2)
	{

		QAngle angOffset = RandomAngle(-1, 1);
		QAngle LastAngles = (pBot->GetLocalAngles() + angOffset);

		for (int i = 0; i < 2; i++)
		{
			if (fabs(LastAngles[i] - pBot->GetLocalAngles[i]) > 15.0f)
			{
				if (LastAngles[i] > pBot->GetLocalAngles[i])
				{
					LastAngles[i] = pBot->GetLocalAngles[i] + 15;
				}
				else
				{
					LastAngles[i] = pBot->GetLocalAngles[i] - 15;
				}
			}
		}

		LastAngles[2] = 0;

		pBot->SetLocalAngles(LastAngles);
	}*/

	RunPlayerMove(pBot, cmd, pBot->GetLocalAngles(), buttons, gpGlobals->frametime);
}
#endif
