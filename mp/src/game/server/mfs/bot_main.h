//******************************************************************
// Multiplayer AI for Source engine by R_Yell - rebel.y3ll@gmail.com
//******************************************************************

#ifndef BOT_MAIN_H
#define BOT_MAIN_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "player.h"
#include "hl2mp_player.h"

#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "nav_area.h"

// how often bot checks for obstacles (phys props, doors,etc) and deals with them
#define BOT_NEXT_OBSTACLE_CHECK 0.25f

// how often bot checks if got stuck
#define STUCK_SAMPLES 5.0f

// how quick bot turns 
#define SKILL_MIN_YAW_RATE 7.0f
#define SKILL_MAX_YAW_RATE 10.0f

// how fast bot walks
#define SKILL_MIN_WALK_SPEED 150.0f
#define SKILL_MAX_WALK_SPEED 250.0f

// how fast bot runs
#define SKILL_MIN_RUN_SPEED 260.0f
#define SKILL_MAX_RUN_SPEED 320.0f

// how often bot strafes
#define SKILL_MIN_STRAFE 0.0f
#define SKILL_MAX_STRAFE 10.0f // set this to 0 to disable strafe skill at all

//Individual hide spot declerations, allowing for a dynamic list of hide spots
#define SPAWN_POINT_DEATHMATCH "info_player_deathmatch"
#define SPAWN_POINT_BLUE "info_player_combine"
#define SPAWN_POINT_RED "info_player_rebels"
#define SPAWN_POINT_CAPTAIN_BLUE "info_player_captain_blue"
#define SPAWN_POINT_CAPTAIN_RED "info_player_combine_red"

// ladder's top dismount point gets added some artificial distance to allow bot more room to complete the operation
#define LADDER_EXTRA_HEIGHT_VEC Vector(0,0,25)

#ifdef MFS
#ifndef bot_main_cpp
extern ConVar bot_mimic;
#endif
#endif

enum BotGeneralStates_t
{
	BOT_NAVIG_IDLE, // bot just awaits
	BOT_NAVIG_PATH, // assigned a route to somewhere
	BOT_NAVIG_PATH_ENFORCED, // same but cannot be aborted by direct navigation
	BOT_NAVIG_DIRECT, // bot can see enemy and no obstacles in between detected, no need to ask for waypoints (less expensive)
	BOT_NAVIG_UNSTUCK, // bot is headed to a waypoint to get unstuck, cannot be interrupted no matter what
};

enum BotSchedules_t
{
	BOT_SCHED_COMBAT, 
	BOT_SCHED_HIDE,	
	//BOT_SCHED_SCOUT,
};

// these are the basic bot parameters, defined at creation time only
enum BotSkill_t
{
	BOT_SKILL_WALK_SPEED,
	BOT_SKILL_RUN_SPEED,
	BOT_SKILL_YAW_RATE,
	BOT_SKILL_STRAFE,

	BOT_SKILL_MAX_TOKENS,
};

// structure to store waypoints
struct NavAreaData_t
{
	Vector Center;
	NavTraverseType TransientType;
	int AttributeType;
	int Id;
};

// intermediate sized hull used when we want to test something smaller than the full bot hull, and bigger than a simple trace line
static Vector BotTestHull = Vector(5,5,5);

// This is our bot class.
class CHL2MP_Bot : public CHL2MP_Player
{
public:

	~CHL2MP_Bot();

	float			m_flNextStrafeTime;
	float			m_flStrafeSkillRelatedTimer;
	float			m_flSideMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;

	// behaviour - capabilities
	int m_nBotState;
	int m_nBotSchedule;
	float m_flSkill[BOT_SKILL_MAX_TOKENS];

	// navigation
	float m_flNextJump;
	bool m_bIsOnLadder;
	CUtlVector <NavAreaData_t> m_Waypoints;
	float m_flNextPathCheck;
	float m_flDontUseDirectNav;
	float m_flTimeToRecheckStuck;
	float m_flDesiredYaw;
	float m_flNextDealObstacles;
	int m_nIsHiding;
	CUtlVector<int> m_AlreadyCheckedHideSpots;
	float m_flCreateRandomPathCoolDown;
	float m_flNextProximityCheck;
	float m_flBotToEnemyDist;
	float m_flDistTraveled;
	float m_flHeightDifToEnemy;

	// combat
	EHANDLE hEnemy;
	float m_flTimeToRecheckEnemy;
	bool m_bEnemyOnSights;	
	bool m_bInRangeToAttack;
	float m_flNextBotAttack;
	float m_flMinRangeAttack;

	bool HasEnemy() { return (hEnemy.Get() != NULL && hEnemy.Get()->IsAlive()); }
	bool RecheckEnemy() { return m_flTimeToRecheckEnemy < gpGlobals->curtime; }

	CBasePlayer *GetEnemy() { return dynamic_cast<CBasePlayer *>(hEnemy.Get()); }

	float UpdateTime;

#ifndef linux
	char *pHideSpot = SPAWN_POINT_DEATHMATCH; //Create an entity "info_hide_spot" and use that by default or use the nav mesh
//#else
	//#define pHideSpot "SPAWN_POINT_DEATHMATCH"
#endif

	void Spawn()
	{
		CHL2MP_Player::Spawn(); // Player model is set there

		hEnemy.Set(NULL);
		ResetNavigationParams();
		m_AlreadyCheckedHideSpots.RemoveAll();
		m_flNextDealObstacles = 0;
		m_flCreateRandomPathCoolDown = 0;
		m_flNextProximityCheck = 0;
		m_flDistTraveled = 0; // distance this bot has traveled recently, since last stuck check
		m_flMinRangeAttack = 60.0f;
		m_bInRangeToAttack = false;
		m_flNextAttack = gpGlobals->curtime;
		m_flNextStrafeTime = 0;
		m_flStrafeSkillRelatedTimer = 0;

		UpdateTime = gpGlobals->curtime;
	}

	void ResetNavigationParams()
	{
		m_Waypoints.RemoveAll();
		m_flNextJump = 0;
		m_bIsOnLadder = false;
		m_flNextPathCheck = 0;
		m_flDontUseDirectNav = 0;
	}

	void AddWaypoint( Vector center, NavTraverseType transient, int attribute, int id, bool AddToTail = false )
	{  
		NavAreaData_t data;
		VectorCopy( center, data.Center );
		data.TransientType = transient;
		data.AttributeType = attribute;
		data.Id = id;		

		if( AddToTail )
			m_Waypoints.AddToTail(data); 
		else
			m_Waypoints.AddToHead(data); 	
	}

	void ResetWaypoints( void ) { m_Waypoints.RemoveAll(); }

	bool ShouldUpdate() 	
	{
#ifndef linux
		CBaseEntity *pSpot = NULL;

		if ((pSpot = gEntList.FindEntityByClassname(pSpot, pHideSpot)) == NULL)
			return true;
#endif

		if ( gpGlobals->curtime - UpdateTime > 3600 )
			return true; 

			return false;
	}

	void Update(int mode)
	{
#ifndef linux
			CBaseEntity *pSpot = NULL;
			char *OldHideSpot = pHideSpot;

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

				if (troll == 2)
					pHideSpot = SPAWN_POINT_RED;
				else if (troll == 3)
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
					pHideSpot = SPAWN_POINT_CAPTAIN_RED;

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
					pHideSpot = SPAWN_POINT_CAPTAIN_BLUE;

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
#endif

			UpdateTime = gpGlobals->curtime;
	}

#ifdef MFS
	bool RunMimicCommand(CUserCmd& cmd)
	{
		if (bot_mimic.GetInt() <= 0)
			return false;

		if (bot_mimic.GetInt() > gpGlobals->maxClients)
			return false;


		CBasePlayer *pPlayer = UTIL_PlayerByIndex(bot_mimic.GetInt());
		if (!pPlayer)
			return false;

		if (!pPlayer->GetLastUserCommand())
			return false;

		return true;
	}
#endif

};


CBasePlayer *BotPutInServer(bool bFrozen, int iTeam);

bool CreatePath( CHL2MP_Bot *pBot, CBasePlayer *pPlayer, Vector OptionalOrg = vec3_origin );

void Bot_RunAll();

#endif // BOT_MAIN_H
