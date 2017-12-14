//******************************************************************
// Multiplayer AI for Source engine by R_Yell - rebel.y3ll@gmail.com
// Modded by Filip - Yours Trully xD
//******************************************************************

#include "cbase.h"
#define bot_main_cpp

#include "player.h"
#include "hl2mp_player.h"

#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"

#ifndef MFS
#include "team.h"
#endif

#ifdef MFS
ConVar bot_forcefireweapon("bot_forcefireweapon", "", FCVAR_CHEAT, "Force bots with the specified weapon to fire.");
ConVar bot_forceattack2("bot_forceattack2", "0", FCVAR_CHEAT, "When firing, use attack2.");
ConVar bot_forceattackon("bot_forceattackon", "0", FCVAR_CHEAT, "When firing, don't tap fire, hold it down.");
ConVar bot_flipout("bot_flipout", "0", FCVAR_CHEAT, "When on, all bots fire their guns.");
ConVar bot_defend("bot_defend", "0", FCVAR_CHEAT, "Set to a team number, and that team will all keep their combat shields raised.");
//ConVar bot_changeclass("bot_changeclass", "0", FCVAR_CHEAT, "Force all bots to change to the specified class.");
ConVar bot_zombie("bot_zombie", "0", FCVAR_CHEAT, "Brraaaaaiiiins.");
ConVar bot_mimic_yaw_offset("bot_mimic_yaw_offset", "0", FCVAR_CHEAT, "Offsets the bot yaw.");
ConVar bot_attack("bot_attack", "0", FCVAR_CHEAT, "Shoot!");
ConVar bot_randomize("bot_randomize", "0", FCVAR_SERVER_CAN_EXECUTE, "Makes bot's have random profiles");
ConVar bot_sendcmd("bot_sendcmd", "", FCVAR_CHEAT, "Forces bots to send the specified command.");
ConVar bot_crouch("bot_crouch", "0", FCVAR_CHEAT, "Bot crouches");
#ifdef NEXT_BOT
extern ConVar bot_mimic;
#else
ConVar bot_mimic("bot_mimic", "0", FCVAR_CHEAT, "Bot uses usercmd of player by index.");
#endif
#endif

#include "bot_main.h"
#include "bot_combat.h"
#include "bot_navigation.h"

// support for nav mesh
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_area.h"

class CHL2MP_Bot;
void Bot_Think( CHL2MP_Bot *pBot );

#ifndef MFS
// Handler for the "bot" command.
CON_COMMAND_F( bot_add, "Add a bot.", FCVAR_SERVER_CAN_EXECUTE )
{
	if (!TheNavMesh->IsLoaded())
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
		DevMsg("No navigation mesh loaded, Creating one");
		engine->ClientCommand(pPlayer->edict(), "nav_generate");
	}

	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 32 );

	CTeam *pCombine = g_Teams[TEAM_COMBINE];
	CTeam *pRebels = g_Teams[TEAM_REBELS];

	int iTeam;
	if (HL2MPRules()->IsTeamplay() == false)
		iTeam = TEAM_UNASSIGNED;
	else
	{
		if (pCombine == NULL || pRebels == NULL)
		{
			iTeam = random->RandomInt(TEAM_COMBINE, TEAM_REBELS);
		}
		else
		{
			if (pCombine->GetNumPlayers() > pRebels->GetNumPlayers())
			{
				iTeam = TEAM_REBELS;
			}
			else if (pCombine->GetNumPlayers() < pRebels->GetNumPlayers())
			{
				iTeam = TEAM_COMBINE;
			}
			else
			{
				iTeam = random->RandomInt(TEAM_COMBINE, TEAM_REBELS);
			}
		}
	}

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );

	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		BotPutInServer( bFrozen, iTeam );
	}
}
#endif

static int g_CurBotNumber = 0;

int g_iLastHideSpot = 0;
int g_iLastBotName = 0;
int g_iLastBlueBotName = 0;
int g_iLastRedBotName = 0;

void bot_kick_f(const CCommand &args)
{
	if (args[1] != UTIL_VarArgs("all"))
	{
		engine->ServerCommand(UTIL_VarArgs("kick %s\n", atoi(args[1])));
		g_CurBotNumber--;
	}
	else
	{
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
					g_CurBotNumber--;
				}
			}
		}
	}
}

ConCommand bot_kick("bot_kick", bot_kick_f, "kick a bot", FCVAR_SERVER_CAN_EXECUTE);

CHL2MP_Bot::~CHL2MP_Bot( void )
{
	
}

// all bot names
const char *g_ppszBotNames[] =
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
const char *g_ppszBlueBotNames[] =
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
const char *g_ppszRedBotNames[] =
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

int nHeadsAll = ARRAYSIZE(g_ppszBotNames);
int nHeadsBlue = ARRAYSIZE(g_ppszBlueBotNames);
int nHeadsRed = ARRAYSIZE(g_ppszRedBotNames);

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

#ifdef MFS
#pragma warning( disable : 4706 )
void bot_quota_f(const CCommand &args)
{
	int bot_quota = atoi(args[1]);
	if (g_CurBotNumber < bot_quota)
	{
		for (int i = g_CurBotNumber; i = bot_quota; i++)
		{
			engine->ServerCommand("bot_add");
		}
	}
	else if (g_CurBotNumber > bot_quota)
	{
		for (int i = g_CurBotNumber; i = bot_quota; i--)
		{
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				// Ignore plugin bots
				if (pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT))
				{
					CHL2MP_Bot *pBot = dynamic_cast<CHL2MP_Bot*>(pPlayer);
					if (pBot)
					{
						if (pBot->GetBotNumber() == g_CurBotNumber)
						{
							engine->ServerCommand(UTIL_VarArgs("bot_kick %s\n", pBot->GetPlayerName()));
							break;
						}
					}
				}
			}
		}
	}
}
ConCommand bot_quota("bot_quota", bot_quota_f, "amount of bots", FCVAR_SERVER_CAN_EXECUTE);
#endif

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
		if (nHeadsAll >= 1)
		{
			if (bot_randomize.GetBool() == true)
			{
				botname = g_ppszBotNames[(random->RandomInt(0, nHeadsAll - 1)+ ++g_iLastBotName) % nHeadsAll];
			}
			else
			{
				botname = g_ppszBotNames[g_iLastBotName++];
			}

			for (int i = 0; i < nHeadsAll; i++) // Loop to find the item to delete.
			{
				if (g_ppszBotNames[i] == botname) // If we find the item to delete...
				{
					if (bot_randomize.GetBool() == true)
					{
						g_ppszBotNames[i] = g_ppszBotNames[nHeadsAll - 1]; //We set the current element with the last, cheaper and randomizes a bit
					}
					else
					{
						for (int j = i; j < nHeadsAll - 1; j++) // Iterate through the remaining elements, stopping one before the end.
						{
							g_ppszBotNames[j] = g_ppszBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
						}
					}
					g_ppszBotNames[nHeadsAll - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
					nHeadsAll--; // Reduce the array size by one.
					break; // Exit out of the 'find item to delete' loop.	
				}
			}
		}
		else
		{
			Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber+1);
		}
	}
	else
	{
		if (iTeam == TEAM_COMBINE)
		{
			if (nHeadsBlue >= 1)
			{
				if (bot_randomize.GetBool() == true)
				{
					botname = g_ppszBlueBotNames[(random->RandomInt(0, nHeadsBlue - 1)+ ++g_iLastRedBotName) % nHeadsBlue];
				}
				else
				{
					botname = g_ppszBlueBotNames[g_iLastBotName++];
				}

				for (int i = 0; i < nHeadsBlue; i++) // Loop to find the item to delete.
				{
					if (g_ppszBlueBotNames[i] == botname) // If we find the item to delete...
					{
						if (bot_randomize.GetBool() == true)
						{
							g_ppszBlueBotNames[i] = g_ppszBlueBotNames[nHeadsBlue - 1]; //We set the current element with the last, cheaper and randomizes a bit
						}
						else
						{
							for (int j = i; j < nHeadsBlue - 1; j++) // Iterate through the remaining elements, stopping one before the end.
							{
								g_ppszBlueBotNames[j] = g_ppszBlueBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
							}
						}
						g_ppszBlueBotNames[nHeadsBlue - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
						nHeadsBlue--; // Reduce the array size by one.
						break; // Exit out of the 'find item to delete' loop.	
					}
				}
			}
			else
			{
				Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber+1);
			}
		}
		else
		{
			if (nHeadsRed >= 1)
			{
				if (bot_randomize.GetBool() == true)
				{
					botname = g_ppszRedBotNames[(random->RandomInt(0, nHeadsRed - 1)+ ++g_iLastRedBotName) % nHeadsRed];
				}
				else
				{
					botname = g_ppszRedBotNames[g_iLastRedBotName++];
				}

				for (int i = 0; i < nHeadsRed; i++) // Loop to find the item to delete.
				{
					if (g_ppszRedBotNames[i] == botname) // If we find the item to delete...
					{
						if (bot_randomize.GetBool() == true)
						{
							g_ppszRedBotNames[i] = g_ppszRedBotNames[nHeadsRed - 1]; //We set the current element with the last, cheaper and randomizes a bit
						}
						else
						{
							for (int j = i; j < nHeadsRed - 1; j++) // Iterate through the remaining elements, stopping one before the end.
							{
								g_ppszRedBotNames[j] = g_ppszRedBotNames[j + 1]; // Overwrite the current element with the next. This effectively deletes the item to delete, and moves everything else down one.
							}
						}
						g_ppszRedBotNames[nHeadsRed - 1] = NULL; // Set the last item in the array to null, (or some other appropriate null value). This may not necessarily be needed, but is good practice.
						nHeadsRed--; // Reduce the array size by one.
						break; // Exit out of the 'find item to delete' loop.	
					}
				}
			}
			else
			{
				Q_snprintf(botname2, sizeof(botname2), "Bot%02i", g_CurBotNumber+1);
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

#ifndef MFS
		pPlayer->ChangeTeam( iTeam ); // In MFS we modified Hl2DM's pickdefaultspawnteam to not check for bots
		//pPlayer->RemoveAllItems( true ); //Why
#endif

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
#ifdef MFS
		pPlayer->botnumber = g_CurBotNumber;
#endif
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

#ifndef MFS
		pPlayer->ChangeTeam( iTeam ); // In MFS we modified Hl2DM's pickdefaultspawnteam to not check for bots
		//pPlayer->RemoveAllItems( true ); //Why
#endif

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
#ifdef MFS
		pPlayer->botnumber = g_CurBotNumber;
#endif
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
static void RunPlayerMove(CHL2MP_Player *fakeclient, CUserCmd &cmd, float frametime)
{
	if (!fakeclient)
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase(flTimeBase);

	Q_memset(&cmd, 0, sizeof(cmd));

#ifdef MFS
	//QAngle vecViewAngles;
	unsigned short buttons = 0;

	CHL2MP_Bot *pBot = dynamic_cast<CHL2MP_Bot*>(fakeclient);
	//vecViewAngles = pBot->GetLocalAngles();

	if (pBot->RunMimicCommand(cmd))
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(bot_mimic.GetInt());
		cmd = *pPlayer->GetLastUserCommand();
		cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();
	}
	
	if (bot_zombie.GetBool())
	{
		cmd.random_seed = random->RandomInt(0, 0x7fffffff);
	}

	// Is my team being forced to defend?
	if (bot_defend.GetInt() == fakeclient->GetTeamNumber())
	{
		buttons |= IN_ATTACK2;
	}
	// If bots are being forced to fire a weapon, see if I have it
	else if (bot_forcefireweapon.GetString())
	{
		CBaseCombatWeapon *pWeapon = fakeclient->Weapon_OwnsThisType(bot_forcefireweapon.GetString());
		if (pWeapon)
		{
			// Switch to it if we don't have it out
			CBaseCombatWeapon *pActiveWeapon = fakeclient->GetActiveWeapon();

			// Switch?
			if (pActiveWeapon != pWeapon)
			{
				fakeclient->Weapon_Switch(pWeapon);
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
		fakeclient->ClientCommand(args);

		bot_sendcmd.SetValue("");
	}

	if (bot_crouch.GetInt())
		cmd.buttons |= IN_DUCK;

	if (bot_attack.GetBool())
		cmd.buttons |= IN_ATTACK;

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

	//if (bot_flipout.GetBool())
	//VectorCopy(viewangles, cmd.viewangles);
#endif

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

	RunPlayerMove(pBot, cmd, gpGlobals->frametime);
}