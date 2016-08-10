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
	count = clamp( count, 1, 16 );

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		BotPutInServer( bFrozen );
	}
}

void bot_kick_f (const CCommand &args)
{
	int name = atoi(args[1]);
	if (name < 1 )
	//if ( args.Arg(1) == "" )
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));

			// Ignore plugin bots
			if (pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT)/* && !pPlayer->IsEFlagSet( EFL_PLUGIN_BASED_BOT )*/) //FixMe
			{
				CHL2MP_Bot *pBot = dynamic_cast<CHL2MP_Bot*>(pPlayer);
				if (pBot)
				{
					engine->ClientCommand(pBot->edict(), "disconnect");
				}
			}
		}
		return;
	}

	engine->ServerCommand(UTIL_VarArgs("kick %s\n", name));
}

ConCommand bot_kick("bot_kick", bot_kick_f, "kick a bot", FCVAR_SERVER_CAN_EXECUTE);

static int g_CurBotNumber = 1;
static int g_CurBlueBotNumber = 1;
static int g_CurRedBotNumber = 1;

int g_iLastHideSpot = 0;

CHL2MP_Bot::~CHL2MP_Bot( void )
{
	/*if (HL2MPRules()->IsTeamplay() == false)
		g_CurBotNumber = 1;
	else
	{
			g_CurBlueBotNumber = 1;
			g_CurRedBotNumber = 1;
	}*/
}

// hide spot references for bots
const char *g_ppszRandomHideSpots[] =
{
	"info_player_deathmatch"
	"info_player_combine"
	"info_player_rebels"
	"info_player_captain_blue"
	"info_player_captain_red"
};

//Probably really rare case as every MFS mapper should include deathmatch spawns
const char *g_ppszRandomHideSpotsTeams[] =
{
	"info_player_combine"
	"info_player_rebels"
	"info_player_captain_blue"
	"info_player_captain_red"
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
CBasePlayer *BotPutInServer( bool  bFrozen )
{
	char botname[ 64 ];
	/*int pkvWeapon_1_Value;
	int Weapon_1_PriClip_Value;
	int Weapon_1_SecClip_Value;*/
	if (HL2MPRules()->IsTeamplay() == false)
	{
		if (g_CurBotNumber == 1)
		{
			Q_snprintf(botname, sizeof(botname), "Heywood");
		}
		else if (g_CurBotNumber == 2)
		{
			Q_snprintf(botname, sizeof(botname), "Coffey");
		}
		else if (g_CurBotNumber == 3)
		{
			Q_snprintf(botname, sizeof(botname), "Jackson");
		}
		else if (g_CurBotNumber == 4)
		{
			Q_snprintf(botname, sizeof(botname), "Dillon");
		}
		else if (g_CurBotNumber == 5)
		{
			Q_snprintf(botname, sizeof(botname), "Lydecker");
		}
		else if (g_CurBotNumber == 6)
		{
			Q_snprintf(botname, sizeof(botname), "Carter");
		}
		else if (g_CurBotNumber == 7)
		{
			Q_snprintf(botname, sizeof(botname), "Gorman");
		}
		else if (g_CurBotNumber == 8)
		{
			Q_snprintf(botname, sizeof(botname), "Powell");
		}
		else if (g_CurBotNumber == 9)
		{
			Q_snprintf(botname, sizeof(botname), "Sanchez");
		}
		else if (g_CurBotNumber == 10)
		{
			Q_snprintf(botname, sizeof(botname), "Bennings");
		}
		else if (g_CurBotNumber == 11)
		{
			Q_snprintf(botname, sizeof(botname), "Bowman");
		}
		else if (g_CurBotNumber == 12)
		{
			Q_snprintf(botname, sizeof(botname), "Wychek");
		}
		else if (g_CurBotNumber == 13)
		{
			Q_snprintf(botname, sizeof(botname), "Scagnetti");
		}
		else if (g_CurBotNumber == 14)
		{
			Q_snprintf(botname, sizeof(botname), "Kurtz");
		}
		else if (g_CurBotNumber == 15)
		{
			Q_snprintf(botname, sizeof(botname), "Harris");
		}
		else if (g_CurBotNumber == 16)
		{
			Q_snprintf(botname, sizeof(botname), "Palmer");
		}
		else if (g_CurBotNumber == 17)
		{
			Q_snprintf(botname, sizeof(botname), "Leyden");
		}
		else if (g_CurBotNumber == 18)
		{
			Q_snprintf(botname, sizeof(botname), "Baxter");
		}
		else if (g_CurBotNumber == 19)
		{
			Q_snprintf(botname, sizeof(botname), "Thomson");
		}
		else if (g_CurBotNumber == 20)
		{
			Q_snprintf(botname, sizeof(botname), "Banks");
		}
		else if (g_CurBotNumber == 21)
		{
			Q_snprintf(botname, sizeof(botname), "Good");
		}
		else if (g_CurBotNumber == 22)
		{
			Q_snprintf(botname, sizeof(botname), "Hodgson");
		}
		else if (g_CurBotNumber == 23)
		{
			Q_snprintf(botname, sizeof(botname), "Hall");
		}
		else if (g_CurBotNumber == 24)
		{
			Q_snprintf(botname, sizeof(botname), "Macdonald");
		}
		else if (g_CurBotNumber == 25)
		{
			Q_snprintf(botname, sizeof(botname), "Henderson");
		}
		else if (g_CurBotNumber == 26)
		{
			Q_snprintf(botname, sizeof(botname), "Williams");
		}
		else if (g_CurBotNumber == 27)
		{
			Q_snprintf(botname, sizeof(botname), "Hughes");
		}
		else if (g_CurBotNumber == 28)
		{
			Q_snprintf(botname, sizeof(botname), "Hewitt");
		}
		else if (g_CurBotNumber == 29)
		{
			Q_snprintf(botname, sizeof(botname), "Jones");
		}
		else if (g_CurBotNumber == 30)
		{
			Q_snprintf(botname, sizeof(botname), "Biltcliffe");
		}
		else if (g_CurBotNumber == 31)
		{
			Q_snprintf(botname, sizeof(botname), "Donbavand");
		}
		else if (g_CurBotNumber == 32)
		{
			Q_snprintf(botname, sizeof(botname), "Fitzsimmons");
		}
		else
		{
			Q_snprintf(botname, sizeof(botname), "Bot%02i", g_CurBotNumber);
		}
	}
	else
	{
		/*CBasePlayer *pPlayer = NULL; // so Basically a "whatever the fuck u will be" pointer?
		if ( pPlayer->GetTeamNumber() == 2)
		{
			if (g_CurBlueBotNumber == 1)
			{
				Q_snprintf(botname, sizeof(botname), "Heywood");
			}
			else if (g_CurBlueBotNumber == 2)
			{
				Q_snprintf(botname, sizeof(botname), "Jackson");
			}
			else if (g_CurBlueBotNumber == 3)
			{
				Q_snprintf(botname, sizeof(botname), "Lydecker");
			}
			else if (g_CurBlueBotNumber == 4)
			{
				Q_snprintf(botname, sizeof(botname), "Gorman");
			}
			else if (g_CurBlueBotNumber == 5)
			{
				Q_snprintf(botname, sizeof(botname), "Sanchez");
			}
			else if (g_CurBlueBotNumber == 6)
			{
				Q_snprintf(botname, sizeof(botname), "Bowman");
			}
			else if (g_CurBlueBotNumber == 7)
			{
				Q_snprintf(botname, sizeof(botname), "Scagnetti");
			}
			else if (g_CurBlueBotNumber == 8)
			{
				Q_snprintf(botname, sizeof(botname), "Harris");
			}
			else if (g_CurBlueBotNumber == 9)
			{
				Q_snprintf(botname, sizeof(botname), "Leyden");
			}
			else if (g_CurBlueBotNumber == 10)
			{
				Q_snprintf(botname, sizeof(botname), "Thomson");
			}
			else if (g_CurBlueBotNumber == 11)
			{
				Q_snprintf(botname, sizeof(botname), "Good");
			}
			else if (g_CurBlueBotNumber == 12)
			{
				Q_snprintf(botname, sizeof(botname), "Hall");
			}
			else if (g_CurBlueBotNumber == 13)
			{
				Q_snprintf(botname, sizeof(botname), "Henderson");
			}
			else if (g_CurBlueBotNumber == 14)
			{
				Q_snprintf(botname, sizeof(botname), "Hughes");
			}
			else if (g_CurBlueBotNumber == 15)
			{
				Q_snprintf(botname, sizeof(botname), "Jones");
			}
			else if (g_CurBlueBotNumber == 16)
			{
				Q_snprintf(botname, sizeof(botname), "Donbavand");
			}
			else
			{
				Q_snprintf(botname, sizeof(botname), "Bot%02i", g_CurBlueBotNumber);
			}
		}
		else
		{
			if (g_CurRedBotNumber == 1)
			{
				Q_snprintf(botname, sizeof(botname), "Coffey");
			}
			else if (g_CurRedBotNumber == 2)
			{
				Q_snprintf(botname, sizeof(botname), "Dillon");
			}
			else if (g_CurRedBotNumber == 3)
			{
				Q_snprintf(botname, sizeof(botname), "Carter");
			}
			else if (g_CurRedBotNumber == 4)
			{
				Q_snprintf(botname, sizeof(botname), "Powell");
			}
			else if (g_CurRedBotNumber == 5)
			{
				Q_snprintf(botname, sizeof(botname), "Bennings");
			}
			else if (g_CurRedBotNumber == 6)
			{
				Q_snprintf(botname, sizeof(botname), "Wychek");
			}
			else if (g_CurRedBotNumber == 7)
			{
				Q_snprintf(botname, sizeof(botname), "Kurtz");
			}
			else if (g_CurRedBotNumber == 8)
			{
				Q_snprintf(botname, sizeof(botname), "Palmer");
			}
			else if (g_CurRedBotNumber == 9)
			{
				Q_snprintf(botname, sizeof(botname), "Baxter");
			}
			else if (g_CurRedBotNumber == 10)
			{
				Q_snprintf(botname, sizeof(botname), "Banks");
			}
			else if (g_CurRedBotNumber == 11)
			{
				Q_snprintf(botname, sizeof(botname), "Hodgson");
			}
			else if (g_CurRedBotNumber == 12)
			{
				Q_snprintf(botname, sizeof(botname), "Macdonald");
			}
			else if (g_CurRedBotNumber == 13)
			{
				Q_snprintf(botname, sizeof(botname), "Williams");
			}
			else if (g_CurRedBotNumber == 14)
			{
				Q_snprintf(botname, sizeof(botname), "Hewitt");
			}
			else if (g_CurRedBotNumber == 15)
			{
				Q_snprintf(botname, sizeof(botname), "Biltcliffe");
			}
			else if (g_CurRedBotNumber == 16)
			{
				Q_snprintf(botname, sizeof(botname), "Fitzsimmons");
			}
			else
			{*/
				Q_snprintf(botname, sizeof(botname), "Bot%02i", g_CurRedBotNumber);
			//}
		//}
	}
	
	// This trick lets us create a CHL2MP_Bot for this client instead of the CHL2MP_Player
	// that we would normally get when ClientPutInServer is called.
	ClientPutInServerOverride( &CBotManager::ClientPutInServerOverride_Bot );
	edict_t *pEdict = engine->CreateFakeClient( botname );
	ClientPutInServerOverride( NULL );

	if (!pEdict)
	{
		Msg( "Failed to create Bot (no edict available)\n");
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	CHL2MP_Bot *pPlayer = ((CHL2MP_Bot*)CBaseEntity::Instance( pEdict ));

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	//pPlayer->ChangeTeam( 0 ); // Is handled by hl2dm's spawn
	pPlayer->RemoveAllItems( true ); //Why

	// Spawn() doesn't work with MP template codebase, even if this line is part of default bot template...
	//pPlayer->Spawn();

	CCommand args;
	args.Tokenize( "joingame" );
	pPlayer->ClientCommand( args );

	// set bot skills
	pPlayer->m_flSkill[BOT_SKILL_YAW_RATE] = random->RandomFloat(SKILL_MIN_YAW_RATE, SKILL_MAX_YAW_RATE);
	pPlayer->m_flSkill[BOT_SKILL_WALK_SPEED] = random->RandomFloat(SKILL_MIN_WALK_SPEED, SKILL_MAX_WALK_SPEED);
	pPlayer->m_flSkill[BOT_SKILL_RUN_SPEED] = random->RandomFloat(SKILL_MIN_RUN_SPEED, SKILL_MAX_RUN_SPEED);
	pPlayer->m_flSkill[BOT_SKILL_STRAFE] = random->RandomFloat( SKILL_MIN_STRAFE, SKILL_MAX_STRAFE);

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

	if (HL2MPRules()->IsTeamplay() == false)
		g_CurBotNumber++;
	else
	{
		if (pPlayer->GetTeamNumber() == 2 )
		g_CurBlueBotNumber++;
		else
		g_CurRedBotNumber++;
	}

	return pPlayer;
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
static void RunPlayerMove( CHL2MP_Player *fakeclient, CUserCmd &cmd, float frametime )
{
	if ( !fakeclient )
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

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
					mode = 6;// This map sucks
				else
					// This map is racist
					mode = 4;
			}
			else if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_RED)) == NULL)
			{
				if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_BLUE)) == NULL)
					mode = 6;// This map sucks
				else
					// This map is racist
					mode = 5;
			}
			if (mode < 4)
				mode = 2;
		}

		if (mode < 2)
		{
			if ((pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_BLUE)) == NULL && (pSpot = gEntList.FindEntityByClassname(pSpot, SPAWN_POINT_RED)) == NULL)
				mode = 3;
			else
				mode = 1;
		}
	}

	if (mode == 1)
	{
		int nHeads = ARRAYSIZE(g_ppszRandomHideSpots);

		g_iLastHideSpot = (g_iLastHideSpot + 1) % nHeads;
		pHideSpot = g_ppszRandomHideSpots[g_iLastHideSpot];

		if (pHideSpot == OldHideSpot)
			Update(1);
	}
	else if (mode == 2)
	{
		int nHeads = ARRAYSIZE(g_ppszRandomHideSpotsTeams);

		g_iLastHideSpot = (g_iLastHideSpot + 1) % nHeads;
		pHideSpot = g_ppszRandomHideSpotsTeams[g_iLastHideSpot];

		if (pHideSpot == OldHideSpot)
			Update(2);
	}
	else if (mode == 3)
	{
		pHideSpot = SPAWN_POINT_DEATHMATCH;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(3);*/
	}
	else if (mode == 4)
	{
		pHideSpot = SPAWN_POINT_BLUE;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(4);*/
	}
	else if (mode == 5)
	{
		pHideSpot = SPAWN_POINT_RED;

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(5);*/
	}
	else if (mode == 6) // This actually happends
	{
		pHideSpot = "info_player_start";

		/*if (pHideSpot == OldHideSpot) // Not really much to do about it currently
		Update(6);*/
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

	RunPlayerMove( pBot, cmd, gpGlobals->frametime );	

}
#endif
