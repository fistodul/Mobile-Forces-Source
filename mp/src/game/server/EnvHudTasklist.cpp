//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
 //
 // Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
 //
 // $NoKeywords: $
 //=============================================================================//
 
 #include "cbase.h"
 #include "engine/IEngineSound.h"
 #include "baseentity.h"
 #include "entityoutput.h"
 #include "recipientfilter.h"
 
 // memdbgon must be the last include file in a .cpp file!!!
 #include "tier0/memdbgon.h"
 
 #define TASKLIST_MAX_TASKS 4
 #define TASKLIST_TASK_INACTIVE 0
 #define TASKLIST_TASK_COMPLETE 1
 #define TASKLIST_TASK_LOWPRIORITY 2
 #define TASKLIST_TASK_MEDPRIORITY 3
 #define TASKLIST_TASK_HIGHPRIORITY 4
 
 //-----------------------------------------------------------------------------
 // Purpose: 
 //-----------------------------------------------------------------------------
 class CEnvHudTasklist : public CPointEntity
 {
 public:
 	DECLARE_CLASS( CEnvHudTasklist, CPointEntity );
 
 	void	Spawn( void );
 	void	Precache( void );
 
 private:
 
 	void InputShowHudTasklist( inputdata_t &inputdata );
 	void InputHideHudTasklist( inputdata_t &inputdata );
 
 	void InputTask1Message( inputdata_t &inputdata );
 	void InputTask2Message( inputdata_t &inputdata );
 	void InputTask3Message( inputdata_t &inputdata );
 	void InputTask4Message( inputdata_t &inputdata );
 
 	void InputTask1Urgency( inputdata_t &inputdata );
 	void InputTask2Urgency( inputdata_t &inputdata );
 	void InputTask3Urgency( inputdata_t &inputdata );
 	void InputTask4Urgency( inputdata_t &inputdata );
 
 	void SendTaskData (int index);
 
 	string_t m_iszTaskmsg[TASKLIST_MAX_TASKS];
 	int m_iUrgency[TASKLIST_MAX_TASKS]; // 0=complete, 1=low, 2=medium, 3=high
 
 	DECLARE_DATADESC();
 };
 
 LINK_ENTITY_TO_CLASS( env_hudtasklist, CEnvHudTasklist );
 
 BEGIN_DATADESC( CEnvHudTasklist )
 
 	DEFINE_KEYFIELD( m_iszTaskmsg[0], FIELD_STRING, "task1message" ),
 	DEFINE_KEYFIELD( m_iUrgency[0], FIELD_INTEGER,  "task1urgency" ),
 
 	DEFINE_KEYFIELD( m_iszTaskmsg[1], FIELD_STRING, "task2message" ),
 	DEFINE_KEYFIELD( m_iUrgency[1], FIELD_INTEGER,  "task2urgency" ),
 
 	DEFINE_KEYFIELD( m_iszTaskmsg[2], FIELD_STRING, "task3message" ),
 	DEFINE_KEYFIELD( m_iUrgency[2], FIELD_INTEGER,  "task3urgency"),
 
 	DEFINE_KEYFIELD( m_iszTaskmsg[3], FIELD_STRING, "task4message" ),
 	DEFINE_KEYFIELD( m_iUrgency[3], FIELD_INTEGER,  "task4urgency" ),
 
 	// Show/hide entire task list
 	DEFINE_INPUTFUNC( FIELD_VOID, "ShowHudTasklist", InputShowHudTasklist ),
 	DEFINE_INPUTFUNC( FIELD_VOID, "HideHudTasklist", InputHideHudTasklist ),
 
 	// Set individual task list strings
 	DEFINE_INPUTFUNC( FIELD_STRING, "Task1Message", InputTask1Message ),
 	DEFINE_INPUTFUNC( FIELD_STRING, "Task2Message", InputTask2Message ),
 	DEFINE_INPUTFUNC( FIELD_STRING, "Task3Message", InputTask3Message ),
 	DEFINE_INPUTFUNC( FIELD_STRING, "Task4Message", InputTask4Message ),
 
 	// Set individual task urgency values 
 	DEFINE_INPUTFUNC( FIELD_INTEGER, "Task1Urgency", InputTask1Urgency ),
 	DEFINE_INPUTFUNC( FIELD_INTEGER, "Task2Urgency", InputTask2Urgency ),
 	DEFINE_INPUTFUNC( FIELD_INTEGER, "Task3Urgency", InputTask3Urgency ),
 	DEFINE_INPUTFUNC( FIELD_INTEGER, "Task4Urgency", InputTask4Urgency ),
 
 END_DATADESC()
 
 
 
 //-----------------------------------------------------------------------------
 // Purpose: Spawn the new ent
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::Spawn( void )
 {
 	Precache();
 	SetSolid( SOLID_NONE );
 	SetMoveType( MOVETYPE_NONE );
 }
 
 
 //-----------------------------------------------------------------------------
 // Purpose: 
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::Precache( void )
 {
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for showing the task list
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputShowHudTasklist( inputdata_t &inputdata )
 {
 	for (int i=0; i<TASKLIST_MAX_TASKS; i++) {
 		SendTaskData (i);
 	}
 }
 
 //-----------------------------------------------------------------------------
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputHideHudTasklist( inputdata_t &inputdata )
 {
 }
 
 //-----------------------------------------------------------------------------
 // Send a task data message to the client.  This gets caught by the task HUD
 // display element and shown.
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::SendTaskData (int index)
 {
 	CBaseEntity *pPlayer = NULL;
 
 	pPlayer = UTIL_GetLocalPlayer();
 
 	if ( pPlayer )
 	{
 		if ( !pPlayer->IsNetClient() )
 		{
 			return;
 		}
 
 		CSingleUserRecipientFilter user( (CBasePlayer *)pPlayer );
 		user.MakeReliable();
 
 		UserMessageBegin( user, "TaskList" );
 			WRITE_BYTE( index );
 			WRITE_BYTE ( m_iUrgency[index] );
 			WRITE_STRING( STRING (m_iszTaskmsg[index]) );
 		MessageEnd();
 		DevMsg (2, "Sent msg %d, %d, %s\n", index, m_iUrgency[index], m_iszTaskmsg[index]);
 	}
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 1 urgency
 // DEFINE_INPUTFUNC( FIELD_STRING, "Task1Urgency", InputTask1Urgency ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask1Urgency ( inputdata_t &inputdata )
 {
 	m_iUrgency[0] = inputdata.value.Int();
 	SendTaskData (0);
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 2 urgency
 // DEFINE_INPUTFUNC( FIELD_STRING, "Task2Urgency", InputTask2Urgency ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask2Urgency ( inputdata_t &inputdata )
 {
 	DevMsg (2, "Got req. to set task2 urgency to %d\n", inputdata.value.Int());
 	m_iUrgency[1] = inputdata.value.Int();
 	SendTaskData (1);
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 3 urgency
 // DEFINE_INPUTFUNC( FIELD_STRING, "Task3Urgency", InputTask3Urgency ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask3Urgency ( inputdata_t &inputdata )
 {
 	m_iUrgency[2] = inputdata.value.Int();
 	SendTaskData (2);
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 4 urgency
 // DEFINE_INPUTFUNC( FIELD_STRING, "Task4Urgency", InputTask4Urgency ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask4Urgency ( inputdata_t &inputdata )
 {
 	m_iUrgency[3] = inputdata.value.Int();
 	SendTaskData (3);
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 1 text
 // DEFINE_INPUTFUNC( FIELD_STRING, "TaskMessage1", InputTaskMessage1 ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask1Message( inputdata_t &inputdata )
 {
 	m_iszTaskmsg[0] = MAKE_STRING( inputdata.value.String() );
 	SendTaskData (0);
 }
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 2 text
 // DEFINE_INPUTFUNC( FIELD_STRING, "TaskMessage2", InputTaskMessage2 ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask2Message( inputdata_t &inputdata )
 {
 	m_iszTaskmsg[1] = MAKE_STRING( inputdata.value.String() );
 	SendTaskData (1);
 }
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 3 text
 // DEFINE_INPUTFUNC( FIELD_STRING, "TaskMessage3", InputTaskMessage3 ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask3Message( inputdata_t &inputdata )
 {
 	m_iszTaskmsg[2] = MAKE_STRING( inputdata.value.String() );
 	SendTaskData (2);
 }
 //-----------------------------------------------------------------------------
 // Purpose: Input handler for setting task 4 text
 // DEFINE_INPUTFUNC( FIELD_STRING, "TaskMessage4", InputTaskMessage4 ),
 //-----------------------------------------------------------------------------
 void CEnvHudTasklist::InputTask4Message( inputdata_t &inputdata )
 {
 	m_iszTaskmsg[3] = MAKE_STRING( inputdata.value.String() );
 	SendTaskData (3);
 }