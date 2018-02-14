//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:	helicopter Guard
//
//=============================================================================//

#include "cbase.h"
#ifdef pilotable
#include "vehicle_helicopter.h"
#include "vehicle_base.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "ai_hint.h"
#include "ai_memory.h"
#include "ai_moveprobe.h"
#include "npcevent.h"
#include "IEffects.h"
#include "ndebugoverlay.h"
#include "soundent.h"
#include "soundenvelope.h"
#include "ai_squad.h"
#include "ai_network.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_senses.h"
#include "npc_rollermine.h"
#include "ai_blended_movement.h"
#include "physics_prop_ragdoll.h"
#include "iservervehicle.h"
#include "player_pickup.h"
#include "props.h"
#include "npc_antlion.h"
#include "decals.h"
#include "prop_combine_ball.h"
#include "eventqueue.h"
#include "ai_basenpc.h"
#include "ai_motor.h"
#include "globalstate.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "grenade_homer.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "explode.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar	g_debug_Vehiclehelicopter( "g_debug_Vehiclehelicopter", "0" );
ConVar	sk_Vehiclehelicopter_dmg_charge( "sk_Vehiclehelicopter_dmg_charge", "0" );
ConVar	sk_Vehiclehelicopter_dmg_shove( "sk_Vehiclehelicopter_dmg_shove", "0" );
static const char *s_pQUARampSoundContext = "RampSound";
extern ConVar sv_gravity;

extern short g_sModelIndexFireball; // Echh...

#define HELICOPTER_MAX_CHUNKS	3

#define QUABOMB_LIFETIME	1.0f
#define QUABOMB_RAMP_SOUND_TIME 1.0f
static const char *s_pChunkModelName[HELICOPTER_MAX_CHUNKS] = 
{
	"models/gibs/helicopter_brokenpiece_01.mdl",
	"models/gibs/helicopter_brokenpiece_02.mdl",
	"models/gibs/helicopter_brokenpiece_03.mdl",
};

#define HELICOPTER_MAX_GIBS	3
static const char *s_pGibModelName[HELICOPTER_MAX_GIBS] = 
{
	"models/gibs/helicopter_brokenpiece_04_cockpit.mdl",
	"models/gibs/helicopter_brokenpiece_05_tailfan.mdl",
	"models/gibs/helicopter_brokenpiece_06_body.mdl",
};
//==================================================
//
// helicopter Guard
//
//==================================================

#define Vehiclehelicopter_MAX_OBJECTS				128
#define	Vehiclehelicopter_MIN_OBJECT_MASS			8
#define	Vehiclehelicopter_MAX_OBJECT_MASS			750
#define	Vehiclehelicopter_FARTHEST_PHYSICS_OBJECT	350
#define Vehiclehelicopter_OBJECTFINDING_FOV			0.7

//Melee definitions
#define	Vehiclehelicopter_MELEE1_RANGE		156.0f
#define	Vehiclehelicopter_MELEE1_CONE		0.7f

// helicopter summoning
#define Vehiclehelicopter_SUMMON_COUNT		3

// Sight
#define	Vehiclehelicopter_FOV_NORMAL			-0.4f

#define	Vehiclehelicopter_CHARGE_MIN			256
#define	Vehiclehelicopter_CHARGE_MAX			2048

#define HELICOPTER_MAX_UP_PITCH			20
#define HELICOPTER_MAX_DOWN_PITCH		90
#define HELICOPTER_MAX_LEFT_YAW			40
#define HELICOPTER_MAX_RIGHT_YAW		40

#define	HELICOPTER_MSG_MACHINEGUN		1

#define HELI_MODEL "models/combine_Helicopter.mdl"

ConVar	sk_Vehiclehelicopter_health( "sk_Vehiclehelicopter_health", "200" );

//==================================================
// QUA_helicopter::m_DataDesc
//==================================================

BEGIN_DATADESC( QUA_helicopter)
END_DATADESC()


//==================================================
// QUA_helicopter
//==================================================

IMPLEMENT_SERVERCLASS_ST(QUA_helicopter, DT_QUA_helicopter)
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropBool(SENDINFO(m_bEnterAnimOn)),
	SendPropBool(SENDINFO(m_bExitAnimOn)),
	SendPropVector(SENDINFO(m_vecEyeExitEndpoint), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vecGunCrosshair), -1, SPROP_COORD),
	SendPropInt		(SENDINFO(m_iHealth), 10 ),
	SendPropInt		(SENDINFO(m_iAmmoCount), 10 ),
	SendPropInt		(SENDINFO(m_iCannonCount), 10 ),
END_SEND_TABLE()



LINK_ENTITY_TO_CLASS( vehicle_helicopter, QUA_helicopter);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QUA_helicopter::QUA_helicopter()
{
	
	//m_bHasGun = true;
	m_pServerVehicle.SetVehicle( this );
	m_vecGunCrosshair.Init();
}
void QUA_helicopter::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( HELI_MODEL );
	m_pServerVehicle.Initialize( STRING(m_vehicleScript) );

	int i;
	for ( i = 0; i < HELICOPTER_MAX_CHUNKS; ++i )
	{
		PrecacheModel( s_pChunkModelName[i] );
	}

	for ( i = 0; i < HELICOPTER_MAX_GIBS; ++i )
	{
		PrecacheModel( s_pGibModelName[i] );
	}
	UTIL_PrecacheOther( "qua_grenade_helicopter" );
	PrecacheScriptSound( "ReallyLoudSpark" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.Ping" );
	PrecacheScriptSound("NPC_AttackHelicopter.ChargeGun");
	/*if ( HasSpawnFlags( SF_HELICOPTER_LOUD_ROTOR_SOUND ) )
	{*/
		PrecacheScriptSound("NPC_AttackHelicopter.RotorsLoud");
	/*}*/
	/*else*/
	/*{*/
		PrecacheScriptSound("NPC_AttackHelicopter.Rotors");
	/*}*/
	PrecacheScriptSound( "NPC_AttackHelicopter.DropMine" );
	PrecacheScriptSound( "NPC_AttackHelicopter.BadlyDamagedAlert" );
	PrecacheScriptSound( "NPC_AttackHelicopter.CrashingAlarm1" );
	PrecacheScriptSound( "NPC_AttackHelicopter.MegabombAlert" );

	PrecacheScriptSound( "NPC_AttackHelicopter.RotorBlast" );
	PrecacheScriptSound( "NPC_AttackHelicopter.FireGun" );
	PrecacheScriptSound( "NPC_AttackHelicopter.Crash" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void QUA_helicopter::Spawn( void )
{
	m_aimYaw = 0;
	m_aimPitch = 0;
	m_bSuppressSound = false;
	m_flSiguienteAtaque=gpGlobals->curtime;
	m_flSiguienteSummon=gpGlobals->curtime;
	Precache();
	SetModel(HELI_MODEL);
	//SetVehicleType( VEHICLE_TYPE_CAR_RAYCAST );
	BaseClass::Spawn();
	SetCollisionGroup(COLLISION_GROUP_VEHICLE);
	m_flSequenceTime=gpGlobals->curtime+4.0f;
	m_flNextShootingTime=gpGlobals->curtime;
	aux=true;
	m_iAmmoType = GetAmmoDef()->Index("StriderMinigun"); 
	m_bInhelicopter=false;
	m_bhelicopterTurbo=false;

	acl=0;
	aclizq=0;
	aclder=0;
	
	
	//SetBlocksLOS( true );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLY );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	m_iCaps = FCAP_IMPULSE_USE;
//	SetBloodColor( BLOOD_COLOR_YELLOW );
	UTIL_SetSize(this,Vector(-220,-40,-80),Vector(170, 40, 40));

	//See if we're supposed to start burrowed
	

	// Do not dissolve
	//AddEFlags( EFL_NO_DISSOLVE );

	// We get a minute of free knowledge about the target
	
	// We need to bloat the absbox to encompass all the hitboxes
	/*Vector absMin = -Vector(100,100,0);
	Vector absMax = Vector(100,100,128);

	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &absMin, &absMax );
	*/InitializeRotorSound();
	m_iHealth=500;
	// Indispensable para que sea vulnerable
	m_takedamage		= DAMAGE_YES;
	m_iAmmoCount=40;
	m_iCannonCount=100;
	m_fReloadTime=gpGlobals->curtime+0.5f;
	m_fCannonCharge=gpGlobals->curtime+0.05f;
	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();
	m_flRespawnTime=gpGlobals->curtime+7.0f;
	m_bSpawn=false;
	SetActivity( ACT_IDLE );
	SetThink(&QUA_helicopter::Think);
	SetNextThink(gpGlobals->curtime);
	Vector maxcull,mincull;
	ExtractBbox(SelectHeaviestSequence(ACT_IDLE),maxcull,mincull);
	//CollisionProp()->SetSurroundingBoundsType( USE_BEST_COLLISION_BOUNDS );
}

void QUA_helicopter::EnterVehicle( CBaseCombatCharacter *pPlayer )
{
	if ( !pPlayer )
		return;

	// Ojo, a ver como hacemos para que nadie pueda quitar
	// el strider a nadie.
	if ( m_hPlayer )
	{
		ExitVehicle(VEHICLE_ROLE_DRIVER);
	}

	//m_hPlayer = pPlayer;
	//m_playerOn.FireOutput( pPlayer, this, 0 );

	m_pServerVehicle.SoundStart();
}
void QUA_helicopter::ExitVehicle( int iRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );
	//m_playerOff.FireOutput( pPlayer, this, 0 );
	//m_bEnterAnimOn = false;

	m_pServerVehicle.SoundShutdown( 1.0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void QUA_helicopter::Activate() {
	BaseClass::Activate();
	m_nGunBaseAttachment = LookupAttachment("gun");
	m_nMachineGunMuzzleAttachment = LookupAttachment( "muzzle" );
	m_nBombAttachment=LookupAttachment("Bomb");
	Vector vecWorldBarrelPos;
	QAngle worldBarrelAngle;
	matrix3x4_t matRefToWorld;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecWorldBarrelPos, worldBarrelAngle );
	GetAttachment(m_nGunBaseAttachment, matRefToWorld );
	VectorITransform( vecWorldBarrelPos, matRefToWorld, m_vecBarrelPos );
}
void QUA_helicopter::Think(void)
{
	if (!m_bSpawn) {
	CBasePlayer	*pPlayer = UTIL_GetLocalPlayer();
	
	/*if ( m_bEngineLocked )
	{
		m_bUnableToFire = true;
		
		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}
	else
	{*/
		// Start this as false and update it again each frame
		//m_bUnableToFire = false;

		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_VEHICLE_CROSSHAIR;
		}
	//}

	SetNextThink( gpGlobals->curtime );

	

	//StudioFrameAdvance();

	if(m_fReloadTime<=gpGlobals->curtime && m_iAmmoCount<40) {
		m_iAmmoCount++;
		m_fReloadTime=gpGlobals->curtime+0.5f;
	}
	if(m_fCannonCharge<=gpGlobals->curtime && m_iCannonCount<100) {
		m_iCannonCount++;
		m_fCannonCharge=gpGlobals->curtime+0.02f;
	}
	if ( m_hPlayer )
	{
		/*engine->Con_NPrintf( 10, " " );
		engine->Con_NPrintf( 11, "PILOTABLE VEHICLE HELICOPTER" );
		engine->Con_NPrintf( 12, " " );
		engine->Con_NPrintf( 13, "Cursors : Movement" );
		engine->Con_NPrintf( 14, "Duck : Down" );
		engine->Con_NPrintf( 15, "Jump : up" );
		engine->Con_NPrintf( 16, "Attack 1 : Machine Gun" );
	*/	Vector vecEyeDir, vecEyePos;
		m_hPlayer->EyePositionAndVectors( &vecEyePos, &vecEyeDir, NULL, NULL );

		// Trace out from the player's eye point.
		Vector	vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH );
		trace_t	trace;
		UTIL_TraceLine( vecEyePos, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );

		// See if we hit something, if so, adjust end position to hit location.
		if ( trace.fraction < 1.0 )
		{
   			vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH * trace.fraction );
		}

		//m_vecLookCrosshair = vecEndPos;
		m_vecGunCrosshair=vecEndPos;
		//AimPrimaryWeapon( vecEndPos );
	}
	PostThink();
	SetNextThink( gpGlobals->curtime);
	}
}
void QUA_helicopter::AimPrimaryWeapon( const Vector &vecWorldTarget ) 
{
	//engine->Con_NPrintf(25,"Llega x:%f,y:%f,z:%f",vecWorldTarget.x,vecWorldTarget.y,vecWorldTarget.z);
	//matrix3x4_t gunMatrix;
	//GetAttachment( m_nGunBaseAttachment, gunMatrix );

	//// transform the enemy into gun space
	//Vector localEnemyPosition;
	//VectorITransform( vecWorldTarget, gunMatrix, localEnemyPosition );

	//// do a look at in gun space (essentially a delta-lookat)
	//QAngle localEnemyAngles;
	//VectorAngles( localEnemyPosition, localEnemyAngles );
	////engine->Con_NPrintf(26,"Angulos x:%f,y:%f,z:%f",localEnemyAngles.x,localEnemyAngles.y,localEnemyAngles.z);
	//
	//localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	//localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	//float targetYaw = m_aimYaw + localEnemyAngles.y;
	//float targetPitch = m_aimPitch + localEnemyAngles.x;
	///*engine->Con_NPrintf(27,"targetYaw: %f",targetYaw);
	//engine->Con_NPrintf(28,"targetPitch: %f",targetPitch);*/
	//float newTargetYaw	= clamp( targetYaw, -HELICOPTER_MAX_LEFT_YAW, HELICOPTER_MAX_RIGHT_YAW );
	//float newTargetPitch = clamp( targetPitch, -90, 90 );

	//targetYaw = newTargetYaw;
	//targetPitch = newTargetPitch;

	//
	//// Exponentially approach the target
	//float yawSpeed = 1;
	//float pitchSpeed = 1;

	//
	//m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	//m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	//
	//SetPoseParameter( "weapon_yaw", m_aimYaw);
	//SetPoseParameter( "weapon_pitch", -m_aimPitch );


	//m_aimPitch = -GetPoseParameter( "weapon_pitch" );
	//m_aimYaw = GetPoseParameter( "weapon_yaw" );
	///*engine->Con_NPrintf(29,"Pitch: %f",m_aimPitch);
	//*/engine->Con_NPrintf(30,"Yaw: %f",m_aimYaw);
}
void QUA_helicopter::InitializeRotorSound( void )
{
	if ( !m_pRotorSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CPASAttenuationFilter filter( this );

		/*if ( HasSpawnFlags( SF_HELICOPTER_LOUD_ROTOR_SOUND ) )
		{
			m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.RotorsLoud" );
		}
		else
		{*/
			m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.Rotors" );
		//}

		m_pRotorBlast = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.RotorBlast" );
		m_pGunFiringSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.FireGun" );
		controller.Play( m_pGunFiringSound, 0.0, 100 );
	}
	else
	{
		Assert(m_pRotorSound);
		Assert(m_pRotorBlast);
		Assert(m_pGunFiringSound);
	}


	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pRotorSound )
	{
		// Get the rotor sound started up.
		controller.Play( m_pRotorSound, 0.0, 100 );
		UpdateRotorWashVolume();
	}

	if ( m_pRotorBlast )
	{
		// Start the blast sound and then immediately drop it to 0 (starting it at 0 wouldn't start it)
		controller.Play( m_pRotorBlast, 1.0, 100 );
		controller.SoundChangeVolume(m_pRotorBlast, 0, 0.0);
	}

	m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
}
void QUA_helicopter::StopLoopingSounds() 
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );
	controller.SoundDestroy( m_pRotorBlast );
	m_pRotorSound = NULL;
	m_pRotorBlast = NULL;
	BaseClass::StopLoopingSounds();
}
void QUA_helicopter::UpdateRotorWashVolume()
{
	if ( !m_pRotorSound )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	float flVolDelta = GetRotorVolume()	- controller.SoundGetVolume( m_pRotorSound );
	if ( flVolDelta )
	{
		// We can change from 0 to 1 in 3 seconds. 
		// Figure out how many seconds flVolDelta will take.
		float flRampTime = fabs( flVolDelta ) * 3.0f; 
		controller.SoundChangeVolume( m_pRotorSound, GetRotorVolume(), flRampTime );
	}
}
float QUA_helicopter::GetRotorVolume( void )
{
	return m_bSuppressSound ? 0.0f : 1.0f;
}
//-----------------------------------------------------------------------------
// Purpose: Funcion que se ejecuta despues de cada pensamiento
//-----------------------------------------------------------------------------
void QUA_helicopter::Morro(bool bAdelante) 
{
	if((acl==0)) {
	
		return;
	}
	QAngle Valido=this->GetLocalAngles();

	;
	if (bAdelante) {
		float inclina=2*(acl/1);
		Valido.x+=inclina;
		if(Valido.x>20) {
			Valido.x=20;
		} 
	} else {
		float inclina=1*(acl/1);
		Valido.x-=inclina;
		if(Valido.x<0) {
			Valido.x=0;
		} 
	}
	this->SetLocalAngles(Valido);
}
void QUA_helicopter::Inclina(void) 
{
QAngle Valido=this->GetLocalAngles();
float fl_lr;
fl_lr=(20*(aclder/1))-(20*(aclizq/1));
Valido.z=fl_lr;
this->SetLocalAngles(Valido);
}
void QUA_helicopter::PostThink(void) {
	if (IsActivityFinished()) {
	}

	StudioFrameAdvance( ); // Esta funcion anima las secuencias.

	// Porque no hara caso de los pose parameters??.
	// En single player tampoco hace caso, pero ya esta claro.

	DispatchAnimEvents(this);
}
void QUA_helicopter::Adelante(void) 
{
	if((acl==0) & (aclup==0) & (acldown==0)) {
		SetAbsVelocity(vec3_origin);
		m_vImpulso=GetAbsVelocity();
		return;
	}
	// Aqui esto no tira, abra que mirarlo bien despues;
		Vector dir;
		float fl_updown;
		fl_updown=(500*(aclup/1))-(500*(acldown/1));
		Vector updown(0,0,fl_updown);
		AngleVectors(this->GetAbsAngles(),&dir);
		dir.z=0;
		if (m_bPressed) {
		//ApplyAbsVelocityImpulse(dir*5.0f+updown);
		SetAbsVelocity((dir*600*(acl/1))+updown);
		m_vImpulso=dir;
		} else {
		SetAbsVelocity((m_vImpulso*600*(acl/1))+updown);
		}
	//SetAbsVelocity(250*(acl/1));
}
void QUA_helicopter::GiraIzquierda(void) 
{
	if(aclizq==0)
		return;

	// Aqui esto no tira, abra que mirarlo bien despues;
		QAngle angles = GetAbsAngles();
		angles.y += 1*(aclizq/1);
		SetAbsAngles( angles );
	/*}*/
	
}
void QUA_helicopter::GiraDerecha(void) 
{
	if(aclder==0)
		return;

	// Aqui esto no tira, abra que mirarlo bien despues;
		QAngle angles = GetAbsAngles();
		angles.y -= 1*(aclder/1);
		SetAbsAngles( angles );
	/*}*/
	
}
//=======================
// Es necesario declarar esto para cuando lo recojan otros NPC's, ya que nuestra velocidad no es correcta para los vehiculos
//======================


// HAY UN FALLO AQUI. DEBUGGEAR Y EXAMINAR QUE PASA, PERO NO DEBERIA FALLAR COMO EN EL STRIDER
void QUA_helicopter::Use(CBaseEntity *pActivator,CBaseEntity *pCaller,USE_TYPE useType,float value) {
	
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	// Si ya hay un jugador, y el que lo usa no es el conductor
	// entonces olvidate de usarme, chavalote.
	if (m_hPlayer && pPlayer!=m_hPlayer) 
		return;

	if (pPlayer->m_lifeState!=LIFE_ALIVE)
		return;
	// Si se esta agachando, esta subiendo, esta haciendo lo de la pata
	// o esta disparando el cañon, entonces ni sale ni entra.
	
	
	ResetUseKey( pPlayer );

	GetServerVehicle()->HandlePassengerEntry( pPlayer, (value>0) );
	//EmitSound( "NPC_Strider.Alert" );
	
}
	// Throw an event indicating that the player entered the vehicle.

//-----------------------------------------------------------------------------
// Purpose: Funcion que establece la actividad del Strider Quartz.
//-----------------------------------------------------------------------------

void QUA_helicopter::SetActivity(Activity NewActivity) {
	int sequence;
	if (m_Activity == NewActivity)
	{
		return;
	} else {
			sequence = SelectHeaviestSequence( NewActivity );
		
	
		//int sequence = this->LookupSequence("Idle01");
		if ( sequence != ACTIVITY_NOT_AVAILABLE )
		{
		m_Activity=NewActivity;
		SetSequence( sequence );
		SetActivity(NewActivity);
		SetCycle( 0 );
		ResetSequenceInfo( );
		if(NewActivity==ACT_MELEE_ATTACK1) {
		SetPlaybackRate(2.0f);
		} else {
		SetPlaybackRate(1.0f);
		}
		//m_Activity=NewActivity;

		} else {
			return;
		}
	}

}


//---------------------------------------------------------
//---------------------------------------------------------





bool QUA_helicopter::IsPlayerInhelicopter(void) {
	return m_bInhelicopter;
}




void QUA_helicopter::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}
void QUA_helicopter::ResetForwardKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_FORWARD;
}
// =================================================
// All Driveable Thinks
// ================================================
void QUA_helicopter::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	/*m_pServerVehicle = new CHLCServerVehicle();
	m_pServerVehicle->SetVehicle( this );*/
}
Class_T	QUA_helicopter::ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification )
{ 
	return CLASS_PLAYER;	
}

void QUA_helicopter::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
			//int iButtons = ucmd->buttons;
			//if ( iButtons & IN_ATTACK )
			//{
			//	DevMsg("DISPARO!\n");
			//	//FireMachineGun();
			//}
			/*else if ( iButtons & IN_ATTACK2 )
			{
				FireRocket();
			}*/
		

	//BaseClass::DriveVehicle( flFrameTime, ucmd, iButtonsDown, iButtonsReleased );
}
//void QUA_helicopter::ItemPostFrame( CBasePlayer *player )
//{
//}

CBaseEntity *QUA_helicopter::GetDriver( void ) 
{ 
	return m_hPlayer; 
}
void QUA_helicopter::ItemPostFrame( CBasePlayer *player )
{
}
bool QUA_helicopter::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	/*if (IsCrouching() && IsStanding() && m_bStompAnim )
		return false;
	*/return true;
}
bool QUA_helicopter::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or rotating
	// Adrian: Check also if I'm currently jumping in or out.
	if (acl!=0 || aclizq!=0 || aclder!=0 || aclup!=0 || acldown!=0)
		return false;
	return true;
}
void QUA_helicopter::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If the player's entering/exiting the vehicle, prevent movement
	//if ( !m_bEnterAnimOn && !m_bExitAnimOn )
	//{
		Drivehelicopter( ucmd->buttons, player->m_afButtonPressed );
		ResetForwardKey(player);
	//}

	//// Run the crane's movement
	//RunCraneMovement( gpGlobals->frametime );
}
void QUA_helicopter::Drivehelicopter( int iDriverButtons, int iButtonsPressed, float flNPCSteering )
{
	if (iDriverButtons & IN_JUMP) {
		if (aclup<1) {
		aclup+=0.02;
		}
	} else {
		if(aclup>0) {
		aclup-=0.02;
		} else {
		aclup=0;
		}
	}
	if (iDriverButtons & IN_DUCK) {
		if (acldown<1) {
		acldown+=0.02;
		}
	} else {
		if(acldown>0) {
		acldown-=0.02;
		} else {
		acldown=0;
		}
	}
	if (iDriverButtons & IN_ATTACK) {
		if (m_flSiguienteAtaque<=gpGlobals->curtime) {
		m_flSiguienteAtaque=gpGlobals->curtime+2.0f;
		if (!m_Ataca) {
			// Solo para la actividad
			m_Ataca=true;	
		}
		m_Lanza=true;
		m_flWaitAttack=gpGlobals->curtime+0.3f;
		}
	}
	if (iDriverButtons & IN_ATTACK2) {
		if (m_iCannonCount>=100) {
		CreateBomb();
		m_iCannonCount=0;
		}
	}
	
 if (iDriverButtons & IN_FORWARD) {
		if (acl<1) {
		acl+=0.02;
		}
		Morro(true);
		m_bPressed=true;
	} else {
		if(acl>0) {
		acl-=0.005;
		} else {
		acl=0;
		}
		Morro(false);
		m_bPressed=false;
	}
	if (iDriverButtons & IN_MOVELEFT) {
		if (aclizq<1) {
		aclizq+=0.02;
		}
	} else {
		if(aclizq>0) {
		aclizq-=0.02;
		} else {
		aclizq=0;
		}
	}
	if (iDriverButtons & IN_MOVERIGHT) {
		if (aclder<1) {
		aclder+=0.02;
		}
	} else {
		if(aclder>0) {
		aclder-=0.02;
		} else {
		aclder=0;
		}
	}
	if ( iDriverButtons & IN_ATTACK )
		{
			if (m_flNextShootingTime<=gpGlobals->curtime) {
				if (m_iAmmoCount>0) {
				Dispara();
				m_flNextShootingTime=gpGlobals->curtime+0.1;
				}
			}
		} 
	
		
	
	Adelante();
	GiraIzquierda();
	GiraDerecha();
	Inclina();
}

void QUA_helicopter::CreateBomb( bool bCheckForFairness, Vector *pVecVelocity, bool bMegaBomb )
{

	Vector vTipPos;
	QAngle vTipAng;
	GetAttachment( m_nBombAttachment, vTipPos, vTipAng );

	
	// Compute velocity
	Vector vecActualVelocity;
	
	Vector vecAcross;
	vecActualVelocity = GetAbsVelocity();
	CrossProduct( vecActualVelocity, Vector( 0, 0, 1 ), vecAcross );
	VectorNormalize( vecAcross );
	vecAcross *= random->RandomFloat( 10.0f, 30.0f );
	vecAcross *= random->RandomFloat( 0.0f, 1.0f ) < 0.5f ? 1.0f : -1.0f;

		// Blat out z component of velocity if it's moving upward....
	if ( vecActualVelocity.z > 0 )
	{
			vecActualVelocity.z = 0.0f;
	}

		vecActualVelocity += vecAcross;
	
	

	
	EmitSound( "NPC_AttackHelicopter.DropMine" );
	Vector up;
	GetVectors(NULL,&up,NULL);
	CQUAGrenadeHelicopter *pGrenade = static_cast<CQUAGrenadeHelicopter*>(CreateEntityByName( "qua_grenade_helicopter" ));
	pGrenade->SetAbsOrigin( vTipPos+up*-150 );
	if (m_hPlayer) {
	pGrenade->SetOwnerEntity( (CBaseEntity *) m_hPlayer );
	} else {
	pGrenade->SetOwnerEntity( this );
	}
	pGrenade->SetAbsVelocity( vecActualVelocity );
	pGrenade->Spawn();
}
Vector QUA_helicopter::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("SpotLight");
	GetAttachment( eyeAttachmentIndex, matrix );
	MatrixGetColumn( matrix, 3, shotPos );

	if ( bNoisy )
	{
		shotPos[0] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[1] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[2] += random->RandomFloat( -8.0f, 8.0f );
	}

	return shotPos;
}

void QUA_helicopter::Dispara(void)
{
	if (m_flWaitAttack<=gpGlobals->curtime) {
	Vector vecMachineGunShootPos;
	QAngle vecMachineGunAngles;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecMachineGunShootPos, vecMachineGunAngles );

	Vector vecMachineGunDir = DondeApuntaPlayer() - vecMachineGunShootPos;
	VectorNormalize(vecMachineGunDir);
	//AngleVectors( vecMachineGunAngles, &vecMachineGunDir );
	
	// Fire the round
	FireBulletsInfo_t info;
		info.m_iShots = 1;
		info.m_vecSrc = vecMachineGunShootPos;
		info.m_vecDirShooting = vecMachineGunDir;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_pAttacker =	(CBaseEntity *) m_hPlayer;
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType =  m_iAmmoType;
		info.m_flDamage = 60;
		info.m_iPlayerDamage= 60;
		info.m_iTracerFreq = 1;
		FireBullets( info );
	EntityMessageBegin( this, true );
				WRITE_BYTE( HELICOPTER_MSG_MACHINEGUN );
				WRITE_VEC3COORD(vecMachineGunShootPos);
				WRITE_VEC3COORD(vecMachineGunDir);
				WRITE_VEC3COORD(VECTOR_CONE_PRECALCULATED);
				WRITE_BYTE( m_iAmmoType );
	MessageEnd();
	DoMuzzleFlash();
	m_iAmmoCount--; // Descontamos ametralladora.
	EmitSound( "Weapon_AR2.Single" );
	}
}

void KillHelicopters(void)
{
QUA_helicopter *pHelicopter=NULL;
	while ( ( pHelicopter = (QUA_helicopter*)gEntList.FindEntityByClassname( pHelicopter, "vehicle_helicopter" )) != NULL )
	{
		// Si el strider esta conducido, entonces no se le mata.
			CTakeDamageInfo info( pHelicopter, pHelicopter, 10000, DMG_BLAST );
			info.SetDamagePosition( pHelicopter->WorldSpaceCenter() );
			info.SetDamageForce( Vector( 0, 0, 1 ) );
			pHelicopter->TakeDamage( info );
	}
}
static ConCommand sv_killhelicopters("sv_killhelicopters", KillHelicopters, "Kill and respawn helicopters in game",FCVAR_CHEAT);

Vector QUA_helicopter::DondeApuntaPlayer(void) 
{
	//if (m_hPlayer) {
	Vector vecEyeDir, vecEyePos;
		m_hPlayer->EyePositionAndVectors( &vecEyePos, &vecEyeDir, NULL, NULL );
		
		// Trace out from the player's eye point.
		Vector	vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH );
		trace_t	trace;
		UTIL_TraceLine( vecEyePos, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );

		// See if we hit something, if so, adjust end position to hit location.
		/*if ( trace.fraction < 1.0 )
		{
   			vecEndPos = vecEyePos + ( vecEyeDir * MAX_TRACE_LENGTH * trace.fraction );
		}*/
	return trace.endpos;
	//}
}
int QUA_helicopter::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth == 0 )
		return 0;

	CTakeDamageInfo dmgInfo = info;
	float dim=dmgInfo.GetDamage();
	if ( dmgInfo.GetInflictor() && dmgInfo.GetInflictor()->GetOwnerEntity() == this ) {
		dmgInfo.SetDamage(dim*0.0);

	} else if( (dmgInfo.GetDamageType() & DMG_BULLET) || (dmgInfo.GetDamageType() & DMG_SLASH) ||
		(dmgInfo.GetDamageType() & DMG_CLUB) )
	{
		
		dmgInfo.SetDamage( dim*0.1 );
	} else {
		dmgInfo.SetDamage( dim*1.2 );
	}
		//int nPrevHealth = GetHealth();

	m_iHealth -= dmgInfo.GetDamage();
	/*if( !IsSmoking() && m_iHealth <= 500 / 2 )
			{
				StartSmoking();
			}*/
	if ( m_iHealth <= 0 )
		{
			m_iHealth = 0;
			Event_Killed( dmgInfo );
			return 0;
		}
		
	return 1;
}
void QUA_helicopter::Event_Killed( const CTakeDamageInfo &info )
{
	//m_lifeState=LIFE_DYING;

	// Calculate death force
	m_vecTotalBulletForce = CalcDamageForceVector( info );

	CBasePlayer *pPlayer = m_hPlayer;
	if ( pPlayer )
		 {
		pPlayer->LeaveVehicle(); // Force exit vehicle
		CBaseEntity *pAPC=this->GetBaseEntity();
		CTakeDamageInfo playerinfo;
			if (info.GetAttacker()==pAPC && info.GetInflictor()==pAPC) {
				playerinfo.SetAttacker(pPlayer);
				playerinfo.SetInflictor(pPlayer);
				playerinfo.SetDamage(10000);
				playerinfo.SetDamageType(DMG_BLAST);
			} else {
				playerinfo.SetAttacker(info.GetAttacker());
				playerinfo.SetInflictor(info.GetInflictor());
				playerinfo.SetDamage(10000);
				playerinfo.SetDamageType(DMG_BLAST);
			}
		playerinfo.SetDamagePosition( pPlayer->WorldSpaceCenter() );
		playerinfo.SetDamageForce( Vector(0,0,-1) );
		pPlayer->TakeDamage( playerinfo );
		m_hPlayer = NULL;
		 }
	m_OnDeath.FireOutput( info.GetAttacker(), this );
	//StopSmoking();
	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );

	Vector vecNormalizedMins, vecNormalizedMaxs;
	CollisionProp()->WorldToNormalizedSpace( vecAbsMins, &vecNormalizedMins );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMaxs, &vecNormalizedMaxs );

	Vector vecAbsPoint;
	CPASFilter filter( GetAbsOrigin() );
	for (int i = 0; i < 5; i++)
	{
		CollisionProp()->RandomPointInBounds( vecNormalizedMins, vecNormalizedMaxs, &vecAbsPoint );
		te->Explosion( filter, random->RandomFloat( 0.0, 1.0 ),	&vecAbsPoint, 
			g_sModelIndexFireball, random->RandomInt( 4, 10 ), 
			random->RandomInt( 8, 15 ), 
			( i < 2 ) ? TE_EXPLFLAG_NODLIGHTS : TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NODLIGHTS,
			100, 0 );
	}
	// Aqui destruiremos todo
	StopLoopingSounds();
	BecomeRagdoll( info, m_vecTotalBulletForce );
	UTIL_ScreenShake( vecAbsPoint, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	CreateCorpse();
	
	//BecomeRagdoll( info, m_vecTotalBulletForce );
    //BecomeRagdollOnClient(m_vecTotalBulletForce);
	//Dissolve(NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL);
}
Vector QUA_helicopter::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	if (info.GetDamageForce() != vec3_origin || (info.GetDamageType() & /*DMG_NO_PHYSICS_FORCE*/DMG_BLAST))
	{
		//if( info.GetDamageType() & DMG_BLAST )
		//{
			// Fudge blast forces a little bit, so that each
			// victim gets a slightly different trajectory. 
			// This simulates features that usually vary from
			// person-to-person variables such as bodyweight,
			// which are all indentical for characters using the same model.
			float scale = random->RandomFloat( 0.85, 1.15 );
			Vector force = info.GetDamageForce();
			force.x *= scale;
			force.y *= scale;
			// Try to always exaggerate the upward force because we've got pretty harsh gravity
			force.z *= (force.z > 0) ? 1.15 : scale;
			return force;
		//}

		return info.GetDamageForce();
	}

	CBaseEntity *pForce = info.GetInflictor();
	if ( !pForce )
	{
		pForce = info.GetAttacker();
	}

	if ( pForce )
	{
		// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
		float forceScale = info.GetDamage() * 75 * 4;

		Vector forceVector;
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			// exaggerate the force from explosions a little (37.5%)
			forceVector = (GetLocalOrigin() + Vector(0, 0, WorldAlignSize().z) ) - pForce->GetLocalOrigin();
			VectorNormalize(forceVector);
			forceVector *= 1.375f;
		}
		else
		{
			// taking damage from self?  Take a little random force, but still try to collapse on the spot.
			if ( this == pForce )
			{
				forceVector.x = random->RandomFloat( -1.0f, 1.0f );
				forceVector.y = random->RandomFloat( -1.0f, 1.0f );
				forceVector.z = 0.0;
				forceScale = random->RandomFloat( 1000.0f, 2000.0f );
			}
			else
			{
				// UNDONE: Collision forces are baked in to CTakeDamageInfo now
				// UNDONE: Is this MOVETYPE_VPHYSICS code still necessary?
				if ( pForce->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					// killed by a physics object
					IPhysicsObject *pPhysics = VPhysicsGetObject();
					if ( !pPhysics )
					{
						pPhysics = pForce->VPhysicsGetObject();
					}
					pPhysics->GetVelocity( &forceVector, NULL );
					forceScale = pPhysics->GetMass();
				}
				else
				{
					forceVector = GetLocalOrigin() - pForce->GetLocalOrigin();
					VectorNormalize(forceVector);
				}
			}
		}
		return forceVector * forceScale;
	}
	return vec3_origin;
}
void QUA_helicopter::CreateCorpse( )
{
	m_lifeState = LIFE_DEAD;

	for ( int i = 0; i < HELICOPTER_MAX_GIBS; ++i )
	{
		CPhysicsProp *pGib = assert_cast<CPhysicsProp*>(CreateEntityByName( "prop_physics_multiplayer" ));
		pGib->SetAbsOrigin( GetAbsOrigin() );
		pGib->SetAbsAngles( GetAbsAngles() );
		pGib->SetAbsVelocity( GetAbsVelocity() );
		pGib->SetModel( s_pGibModelName[i] );
		pGib->Spawn();
		pGib->SetMoveType( MOVETYPE_VPHYSICS );

		float flMass = pGib->GetMass();
		/*if ( flMass < 200 )
		{*/
			Vector vecVelocity;
			pGib->GetMassCenter( &vecVelocity );
			vecVelocity -= WorldSpaceCenter();
			vecVelocity.z = fabs(vecVelocity.z);
			VectorNormalize( vecVelocity );

			// Apply a force that would make a 100kg mass travel 150 - 300 m/s
			float flRandomVel = random->RandomFloat( 150, 300 );
			vecVelocity *= (100 * flRandomVel) / flMass;
			vecVelocity.z += 100.0f;
			AngularImpulse angImpulse = RandomAngularImpulse( -500, 500 );
			
			IPhysicsObject *pObj = pGib->VPhysicsGetObject();
			if ( pObj != NULL )
			{
				pObj->AddVelocity( &vecVelocity, &angImpulse );
			}
			pGib->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		/*}*/
		pGib->Ignite( 60, false );
		pGib->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}

	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	UTIL_RemoveImmediate( this );
}
bool QUA_helicopter::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector ) 
{ 
	
		/*CRagdollProp *pRagdoll = NULL;
		pRagdoll = assert_cast<CRagdollProp *>( CreateServerRagdoll( this, m_nForceBone, info, COLLISION_GROUP_VEHICLE ) );
		pRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime+2.0f, false, ENTITY_DISSOLVE_NORMAL );
		*/
		
		//CreateRagdollEntity();
		
		//pRagdoll->Dissolve
		//pRagdoll->DisableAutoFade();
		
		// Creamos un nuevo fantastico Strider
		
	QUA_helicopter *pHeli = (QUA_helicopter *)CreateEntityByName( "vehicle_helicopter" );
		
	if ( pHeli )
	{
		pHeli->m_bSpawn=true;
		//pHeli->m_iSpawnTry=0;
		//pHeli->m_hRagdoll=m_hRagdoll;
		pHeli->SetThink( &QUA_helicopter::Materialize );
		pHeli->SetContextThink( &QUA_helicopter::Materialize, gpGlobals->curtime + 5.0f, "RESPAWNING" );
		pHeli->SetNextThink( gpGlobals->curtime + 5.0f );
		pHeli->Teleport( &m_vOriginalSpawnOrigin, &m_vOriginalSpawnAngles, NULL );	
		
		//pStrider->AddEffects(EF_NODRAW);
		//pStrider->AddSolidFlags(FSOLID_NOT_SOLID);
	}
	else
	{
		Warning("Respawn failed to create %s!\n", GetClassname() );
	}

	//this->Dissolve( NULL, gpGlobals->curtime+2.0f, false, ENTITY_DISSOLVE_NORMAL );
	return true; 
}
void QUA_helicopter::Materialize( void )
{
	//trace_t tr;
	//UTIL_TraceHull( m_vOriginalSpawnOrigin, m_vOriginalSpawnOrigin, Vector(-38,-38,-38),Vector(38,38,38), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	//if ( (tr.startsolid || tr.allsolid) && m_iSpawnTry<3 )
	//{
	//	//Try again in a second.
	//	SetContextThink(&QUA_Strider::Materialize, gpGlobals->curtime + 1.0f, "RESPAWNING" );
	//	SetNextThink( gpGlobals->curtime + 1.0f );
	//	m_iSpawnTry++;
	//	return;
	//}
	Spawn();
	SetThink( &QUA_helicopter::Think );
	SetNextThink( gpGlobals->curtime);
	
}
void QUA_helicopter::HandleAnimEvent( animevent_t *pEvent )
{
	//engine->Con_NPrintf( 6, "Evento: %f",pEvent->event);
}
Vector QUA_helicopter::EyePosition( )
{
	Vector vecEyePosition;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5, 0.5, 1.0 ), &vecEyePosition );
	return vecEyePosition;
}

// ================================
//
// Esto es necesario porque nosotros estamos HACKENDO el sistema de vehiculo, por lo que cualquier bicho querra saber
// Cual es su velocidad para predecir donde ubicarse
//
// ================================0
Vector QUA_helicopter::GetSmoothedVelocity( void )
{
	Vector velocity=this->GetAbsVelocity();
	return velocity;
}
const char *QUA_helicopter::GetTracerType( void ) 
{
	return "HelicopterTracer"; 
}
void QUA_helicopter::DoImpactEffect( trace_t &tr, int nDamageType )
{
	UTIL_ImpactTrace( &tr, nDamageType, "HelicopterImpact" );
} 
void QUA_helicopter::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;

	data.m_nAttachmentIndex = LookupAttachment( "muzzle" );
	data.m_nEntIndex = entindex();
	DispatchEffect( "ChopperMuzzleFlash", data );
}
QUA_helicopter *CHLCServerVehicle::Gethelicopter( void )
{
	return (QUA_helicopter*)GetDrivableVehicle();
}
// ================================
//
// Esto es necesario porque nosotros estamos HACKENDO el sistema de vehiculo, por lo que cualquier bicho querra saber
// Cual es su velocidad para predecir donde ubicarse
//
// ================================0
Vector CHLCServerVehicle::GetSmoothedVelocity( void )
{
	Vector velocity=Gethelicopter()->GetAbsVelocity();
	return velocity;
}
void CHLCServerVehicle::NPC_SetDriver( CNPC_VehicleDriver *pDriver ) {
	// nada que hacer aqui
}
void CHLCServerVehicle::NPC_DriveVehicle( void ) {
	// nada que hacer aqui
}
void CHLCServerVehicle::HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone )
{
	// Check to see if this vehicle can be controlled or if it's locked
	if ( GetDrivableVehicle()->CanEnterVehicle(pPlayer) )
	{
		pPlayer->GetInVehicle( this, VEHICLE_ROLE_DRIVER);
	}
}
//bool CHLCServerVehicle::GetPassengerExitPoint( int nRole, Vector *pExitPoint, QAngle *pAngles )
//{
//	Assert( nRole == VEHICLE_DRIVER ); 
//
//	// First, see if we've got an attachment point
//	
//		Vector vehicleExitOrigin;
//		QAngle vehicleExitAngles;
//		Gethelicopter()->GetAttachment( "Bomb", vehicleExitOrigin, NULL );
//		Vector up,forward,right;
//		Gethelicopter()->GetVectors(&forward,&right,&up);
//		vehicleExitOrigin+=(forward*-50)+(right*-150)+(up*-50);
//		vehicleExitAngles=Gethelicopter()->GetAbsAngles();
//		*pAngles = vehicleExitAngles;
//		*pExitPoint = vehicleExitOrigin;
//		return true;
//			
//		
//	
//}
//-----------------------------------------------------------------------------
// Purpose: TODO: Revisar bien que pasa aqui 
//-----------------------------------------------------------------------------
void CHLCServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CBaseCombatCharacter *pPlayer = GetPassenger( VEHICLE_ROLE_DRIVER );
	Assert( pPlayer );

	float flPitchFactor=1.0;
	*pAbsAngles = pPlayer->EyeAngles();
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	Gethelicopter()->GetAttachment( "Bomb", vehicleEyeOrigin, NULL );
	vehicleEyeAngles=Gethelicopter()->GetAbsAngles();
	Vector up,forward,right;
	Gethelicopter()->GetVectors(&forward,&right,&up);
	vehicleEyeOrigin+=(forward*215)+(up*-15);
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

//#ifdef HL2_DLL
//	// View dampening.
//	if ( r_VehicleViewDampen.GetInt() )
//	{
//		GetAPC()->DampenEyePosition( vehicleEyeOrigin, vehicleEyeAngles );
//	}
//#endif

	// Compute the relative rotation between the unperterbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Now perterb the attachment point
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO * flPitchFactor, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO * flPitchFactor, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perterbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}

LINK_ENTITY_TO_CLASS( qua_grenade_helicopter, CQUAGrenadeHelicopter );

BEGIN_DATADESC( CQUAGrenadeHelicopter )

	DEFINE_FIELD( m_bActivated,			FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_pWarnSound ),

	DEFINE_THINKFUNC( ExplodeThink ),
	DEFINE_THINKFUNC( RampSoundThink ),
	DEFINE_ENTITYFUNC( ExplodeConcussion ),

END_DATADESC()


//------------------------------------------------------------------------------
// Precache
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::Precache( void )
{
	BaseClass::Precache( );
	PrecacheModel( "models/combine_helicopter/helicopter_bomb01.mdl" );

	PrecacheScriptSound( "ReallyLoudSpark" );
	PrecacheScriptSound( "NPC_AttackHelicopterGrenade.Ping" );
}


//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::Spawn( void )
{
	Precache();

	// point sized, solid, bouncing
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetModel( "models/combine_helicopter/helicopter_bomb01.mdl" );

	
		SetSolid( SOLID_BBOX );
		SetCollisionBounds( Vector( -12.5, -12.5, -12.5 ), Vector( 12.5, 12.5, 12.5 ) );
		VPhysicsInitShadow( false, false );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		SetElasticity( 0.5f );
		AddEffects( EF_NOSHADOW );
	

	// We're always being dropped beneath the helicopter; need to not
	// be affected by the rotor wash
	AddEFlags( EFL_NO_ROTORWASH_PUSH );

	// contact grenades arc lower
	SetGravity( UTIL_ScaleForGravity( 400 ) );	// use a lower gravity for grenades to make them easier to see
	QAngle angles;
	VectorAngles(GetAbsVelocity(), angles );
	SetLocalAngles( angles );
	
	SetThink( NULL );
	
	// Tumble in air
	QAngle vecAngVel( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVel );
	
	// Explode on contact
	SetTouch( &CQUAGrenadeHelicopter::ExplodeConcussion );

	m_bActivated = false;
	m_pWarnSound = NULL;

	m_flDamage = 75.0;

	// Allow player to blow this puppy up in the air
	m_takedamage = DAMAGE_YES;

	g_pNotify->AddEntity( this, this );
}


//------------------------------------------------------------------------------
// On Remve
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::UpdateOnRemove()
{
	if( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pWarnSound );
	}
	g_pNotify->ClearEntity( this );
	BaseClass::UpdateOnRemove();
}


//------------------------------------------------------------------------------
// Activate!
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::BecomeActive()
{
	if ( m_bActivated )
		return;

	if ( IsMarkedForDeletion() )
		return;

	m_bActivated = true;

	
	SetThink( &CQUAGrenadeHelicopter::ExplodeThink );
	SetNextThink( gpGlobals->curtime + QUABOMB_LIFETIME );

	
		SetContextThink( &CQUAGrenadeHelicopter::RampSoundThink, gpGlobals->curtime + QUABOMB_LIFETIME - QUABOMB_RAMP_SOUND_TIME, s_pQUARampSoundContext );

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		CReliableBroadcastRecipientFilter filter;
		m_pWarnSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopterGrenade.Ping" );
		controller.Play( m_pWarnSound, 1.0, PITCH_NORM );
	
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::RampSoundThink( )
{
	if ( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pWarnSound, 140, QUABOMB_RAMP_SOUND_TIME );
	}
	SetContextThink( NULL, gpGlobals->curtime, s_pQUARampSoundContext );
}


//------------------------------------------------------------------------------
// Entity events... these are events targetted to a particular entity
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	BaseClass::OnEntityEvent( event, pEventData );

	if ( event == ENTITY_EVENT_WATER_TOUCH )
	{
		BecomeActive();
	}
}


//------------------------------------------------------------------------------
// If we hit water, then stop
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::PhysicsSimulate( void )
{
	Vector vecPrevPosition = GetAbsOrigin();
	
	BaseClass::PhysicsSimulate();

	if (!m_bActivated && (GetMoveType() != MOVETYPE_VPHYSICS))
	{
		if ( GetWaterLevel() > 1 )
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			BecomeActive();
		}

		// Stuck condition, can happen pretty often
		if ( vecPrevPosition == GetAbsOrigin() )
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			BecomeActive();
		}
	}
}


//------------------------------------------------------------------------------
// If we hit something, start the timer
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );
	BecomeActive();
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
int CQUAGrenadeHelicopter::OnTakeDamage( const CTakeDamageInfo &info )
{
	// We don't take blast damage
	if ( info.GetDamageType() & DMG_BLAST )
		return 0;

	return BaseClass::OnTakeDamage( info );
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::DoExplosion( const Vector &vecOrigin, const Vector &vecVelocity )
{
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), this, 75.0, 
		275.0, (SF_ENVEXPLOSION_NOSPARKS|SF_ENVEXPLOSION_NODLIGHTS|SF_ENVEXPLOSION_NODECAL|SF_ENVEXPLOSION_NOFIREBALL|SF_ENVEXPLOSION_NOPARTICLES), 
		55000.0 );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}

	CEffectData data;

	// If we're under water do a water explosion
	if ( GetWaterLevel() != 0 && (GetWaterType() & CONTENTS_WATER) )
	{
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = 128;
		data.m_flScale = 128;
		data.m_fFlags = 0;
		DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		// Otherwise do a normal explosion
		data.m_vOrigin = GetAbsOrigin();
		DispatchEffect( "HelicopterMegaBomb", data );
	}

	UTIL_Remove( this );
}


//------------------------------------------------------------------------------
// I think I Pow!
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::ExplodeThink(void)
{
	Vector vecVelocity;
	GetVelocity( &vecVelocity, NULL );
	DoExplosion( GetAbsOrigin(), vecVelocity );
}


//------------------------------------------------------------------------------
// I think I Pow!
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	ResolveFlyCollisionBounce( trace, vecVelocity, 0.1f );
}


//------------------------------------------------------------------------------
// Contact grenade, explode when it touches something
//------------------------------------------------------------------------------
void CQUAGrenadeHelicopter::ExplodeConcussion( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;

	if ( pOther->IsWorld() )
		return;

	Vector vecVelocity;
	GetVelocity( &vecVelocity, NULL );
	DoExplosion( GetAbsOrigin(), vecVelocity );
}
#endif