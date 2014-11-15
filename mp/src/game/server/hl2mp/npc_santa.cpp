//========= Copyright Valve Corporation, All rights reserved. ============//
// This is a skeleton file for use when creating a new 
// NPC. Copy and rename this file for the new
// NPC and add the copy to the build.
//
// Leave this file in the build until we ship! Allowing 
// this file to be rebuilt with the rest of the game ensures
// that it stays up to date with the rest of the NPC code.
//
// Replace occurances of CNewNPC with the new NPC's
// classname. Don't forget the lower-case occurance in 
// LINK_ENTITY_TO_CLASS()
//
//
// ASSUMPTIONS MADE:
//
// You're making a character based on CAI_BaseNPC. If this 
// is not true, make sure you replace all occurances
// of 'CAI_BaseNPC' in this file with the appropriate 
// parent class.
//
// You're making a human-sized NPC that walks.
//
//=============================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
// Private activities
//=========================================================
int	ACT_CUSTOMACTIVITY = -1;

//=========================================================
// Custom schedules
//=========================================================
enum
{
	TASK_JUMP = LAST_SHARED_TASK,
	TASK_FIND_DODGE_DIRECTION, 
	LAST_MY_NPC_TASK,
	SCHED_MYCUSTOMSCHEDULE = LAST_SHARED_SCHEDULE,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_MYCUSTOMTASK = LAST_SHARED_TASK,
};


//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	COND_MYNPC_HUNGRY/* = BaseClass::NEXT_CONDITION*/,
	COND_MYCUSTOMCONDITION = LAST_SHARED_CONDITION,
};


//=========================================================
//=========================================================
class CSanta : public CAI_BaseNPC
{
	DECLARE_CLASS( CSanta, CAI_BaseNPC );

public:
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );

	DECLARE_DATADESC();

	// This is a dummy field. In order to provide save/restore
	// code in this file, we must have at least one field
	// for the code to operate on. Delete this field when
	// you are ready to do your own save/restore for this
	// character.
	int		m_iDeleteThisField;

	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS( npc_santa, CSanta );
IMPLEMENT_CUSTOM_AI( npc_citizen,CSanta );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CSanta )

	DEFINE_FIELD( m_iDeleteThisField, FIELD_INTEGER ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSanta::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CSanta);

	ADD_CUSTOM_TASK(CSanta,		TASK_MYCUSTOMTASK);

	ADD_CUSTOM_SCHEDULE(CSanta,	SCHED_MYCUSTOMSCHEDULE);

	ADD_CUSTOM_ACTIVITY(CSanta,	ACT_CUSTOMACTIVITY);

	ADD_CUSTOM_CONDITION(CSanta,	COND_MYCUSTOMCONDITION);
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CSanta::Precache( void )
{
	PrecacheModel( "models/Player/slow/santa_claus/slow_fix.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CSanta::Spawn( void )
{
	Precache();

	SetModel( "models/Player/slow/santa_claus/slow_fix.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 20;
	m_flFieldOfView		= 0.5;
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	//CapabilitiesAdd( bits_CAP_NONE );

	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CSanta::Classify( void )
{
	return	CLASS_PLAYER_ALLY;
}
