//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#ifdef pilotable
#include "vehicle_apc2.h"
#include "ammodef.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "weapon_rpg.h"
#include "in_buttons.h"
#include "globalstate.h"
#include "soundent.h"
#include "ai_basenpc.h"
#include "ndebugoverlay.h"
#include "gib.h"
#include "EntityFlame.h"
#include "smoke_trail.h"
#include "explode.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ROCKET_ATTACK_RANGE_MAX 5500.0f
#define ROCKET_ATTACK_RANGE_MIN 1250.0f

#define MACHINE_GUN_ATTACK_RANGE_MAX 1250.0f
#define MACHINE_GUN_ATTACK_RANGE_MIN 0.0f

#define MACHINE_GUN_MAX_UP_PITCH	30
#define MACHINE_GUN_MAX_DOWN_PITCH	10
#define MACHINE_GUN_MAX_LEFT_YAW	30
#define MACHINE_GUN_MAX_RIGHT_YAW	30

#define MACHINE_GUN_BURST_SIZE		10
#define MACHINE_GUN_BURST_TIME		0.075f
#define MACHINE_GUN_BURST_PAUSE_TIME	0.1f

#define ROCKET_SALVO_SIZE				5
#define ROCKET_DELAY_TIME				1.5
#define ROCKET_MIN_BURST_PAUSE_TIME		3
#define ROCKET_MAX_BURST_PAUSE_TIME		4
#define ROCKET_SPEED					800
#define DEATH_VOLLEY_ROCKET_COUNT		4
#define DEATH_VOLLEY_MIN_FIRE_TIME		0.333
#define DEATH_VOLLEY_MAX_FIRE_TIME		0.166

#define APC_MSG_MACHINEGUN				1

extern short g_sModelIndexFireball; // Echh...


//ConVar sk_apc2_health( "sk_apc2_health", "750" );


#define APC_MAX_CHUNKS	3
static const char *s_pChunkModelName[APC_MAX_CHUNKS] = 
{
	"models/gibs/helicopter_brokenpiece_01.mdl",
	"models/gibs/helicopter_brokenpiece_02.mdl",
	"models/gibs/helicopter_brokenpiece_03.mdl",
};

#define APC_MAX_GIBS	6
static const char *s_pGibModelName[APC_MAX_GIBS] = 
{
	"models/combine_apc_destroyed_gib01.mdl",
	"models/combine_apc_destroyed_gib02.mdl",
	"models/combine_apc_destroyed_gib03.mdl",
	"models/combine_apc_destroyed_gib04.mdl",
	"models/combine_apc_destroyed_gib05.mdl",
	"models/combine_apc_destroyed_gib06.mdl",
};


LINK_ENTITY_TO_CLASS( prop_vehicle_apc, CPropAPC2 );


BEGIN_DATADESC( CPropAPC2 )

	DEFINE_FIELD( m_flDangerSoundTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flHandbrakeTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bInitialHandbrake,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nSmokeTrailCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flMachineGunTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iMachineGunBurstLeft,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nMachineGunMuzzleAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nMachineGunBaseAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_vecBarrelPos,		FIELD_VECTOR ),
	DEFINE_FIELD( m_bInFiringCone,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hLaserDot,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hRocketTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_iRocketSalvoLeft,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flRocketTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nRocketAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nRocketSide,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hSpecificRocketTarget, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_strMissileHint,	FIELD_STRING, "missilehint" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Destroy", InputDestroy ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FireMissileAt", InputFireMissileAt ),
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
	DEFINE_OUTPUT( m_OnFiredMissile, "OnFiredMissile" ),

END_DATADESC()
IMPLEMENT_SERVERCLASS_ST( CPropAPC2, DT_PropAPC2 )
//	//SendPropBool( SENDINFO( m_bHeadlightIsOn ) ),
SendPropInt		(SENDINFO(m_iHealth), 10 ),
SendPropInt		(SENDINFO(m_iAmmoCount), 10 ),
SendPropInt	(SENDINFO(m_iCannonCount), 10),
END_SEND_TABLE();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropAPC2::CPropAPC2() 
{
	m_bHasGun = true;
}
void CPropAPC2::Precache( void )
{
	BaseClass::Precache();

	int i;
	for ( i = 0; i < APC_MAX_CHUNKS; ++i )
	{
		PrecacheModel( s_pChunkModelName[i] );
	}

	for ( i = 0; i < APC_MAX_GIBS; ++i )
	{
		PrecacheModel( s_pGibModelName[i] );
	}

	PrecacheScriptSound( "Weapon_AR2.Single" );
	PrecacheScriptSound( "PropAPC.FireRocket" );
	PrecacheScriptSound( "combine.door_lock" );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropAPC2::Spawn( void )
{
	SetVehicleType( VEHICLE_TYPE_CAR_WHEELS );
	BaseClass::Spawn();
	SetBlocksLOS( true );
	m_iHealth = m_iMaxHealth = 500;
	m_iAmmoCount=30; // Balas seguidas que podemos disparar
	m_iCannonCount=100; // Tanto por cien de cañon.
	SetCycle( 0 );
	m_iMachineGunBurstLeft = MACHINE_GUN_BURST_SIZE;
	m_iRocketSalvoLeft = ROCKET_SALVO_SIZE;
	m_fReloadTime=gpGlobals->curtime+0.5f;
	m_fCannonCharge=gpGlobals->curtime+0.05f;
	m_nRocketSide = 0;
	m_lifeState = LIFE_ALIVE;
	m_bInFiringCone = false;

	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;

	// Reset the gun to a default pose.
	SetPoseParameter( "vehicle_weapon_pitch", 0 );
	SetPoseParameter( "vehicle_weapon_yaw", 90 );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();
	m_bSpawn=false;
	CreateAPCLaserDot();
}

//-----------------------------------------------------------------------------
// Purpose: Create a laser
//-----------------------------------------------------------------------------
void CPropAPC2::CreateAPCLaserDot( void )
{
	// Create a laser if we don't have one
	if ( m_hLaserDot == NULL )
	{
		m_hLaserDot = CreateLaserDot( GetAbsOrigin(), this, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::Activate()
{
	BaseClass::Activate();

	m_nRocketAttachment = LookupAttachment( "cannon_muzzle" );
	m_nMachineGunMuzzleAttachment = LookupAttachment( "muzzle" );
	m_nMachineGunBaseAttachment = LookupAttachment( "gun_base" );

	// NOTE: gun_ref must have the same position as gun_base, but rotates with the gun
	int nMachineGunRefAttachment = LookupAttachment( "gun_def" );

	Vector vecWorldBarrelPos;
	QAngle worldBarrelAngle;
	matrix3x4_t matRefToWorld;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecWorldBarrelPos, worldBarrelAngle );
	GetAttachment( nMachineGunRefAttachment, matRefToWorld );
	VectorITransform( vecWorldBarrelPos, matRefToWorld, m_vecBarrelPos );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::UpdateOnRemove( void )
{
	if ( m_hLaserDot )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	m_pServerVehicle = new CAPC2FourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
Class_T	CPropAPC2::ClassifyPassenger(CBaseCombatCharacter *pPassenger, Class_T defaultClassification)
{ 
	return CLASS_PLAYER;	
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPropAPC2::DamageModifier ( CTakeDamageInfo &info ) 
{ 
	CTakeDamageInfo DmgInfo = info;
	// bullets, slashing and headbutts don't hurt us in the apc, neither do rockets
	if( (DmgInfo.GetDamageType() & DMG_BULLET) || (DmgInfo.GetDamageType() & DMG_SLASH) ||
		(DmgInfo.GetDamageType() & DMG_CLUB) )
	{
		return (0.0);
	}
	else
	{
		return 1.0; 
	}
}


//-----------------------------------------------------------------------------
// position of eyes
//-----------------------------------------------------------------------------
Vector CPropAPC2::EyePosition( )
{
	Vector vecEyePosition;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5, 0.5, 1.0 ), &vecEyePosition );
	return vecEyePosition;
}

	
//-----------------------------------------------------------------------------
// Add a smoke trail since we've taken more damage
//-----------------------------------------------------------------------------
void CPropAPC2::AddSmokeTrail( const Vector &vecPos )
{
	// Start this trail out with a bang!
	ExplosionCreate( vecPos, vec3_angle, this, 1000, 500.0f, SF_ENVEXPLOSION_NODAMAGE | 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | 
		SF_ENVEXPLOSION_NOFIREBALLSMOKE, 0 );
	UTIL_ScreenShake( vecPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	if ( m_nSmokeTrailCount == MAX_SMOKE_TRAILS )
		return;

	SmokeTrail *pSmokeTrail =  SmokeTrail::CreateSmokeTrail();
	if( !pSmokeTrail )
		return;

	// See if there's an attachment for this smoke trail
	char buf[32];
	Q_snprintf( buf, 32, "damage%d", m_nSmokeTrailCount );
	int nAttachment = LookupAttachment( buf );

	++m_nSmokeTrailCount;

	pSmokeTrail->m_SpawnRate = 20;
	pSmokeTrail->m_ParticleLifetime = 4.0f;
	pSmokeTrail->m_StartColor.Init( 0.7f, 0.7f, 0.7f );
	pSmokeTrail->m_EndColor.Init( 0.6, 0.6, 0.6 );
	pSmokeTrail->m_StartSize = 15;
	pSmokeTrail->m_EndSize = 50;
	pSmokeTrail->m_SpawnRadius = 15;
	pSmokeTrail->m_Opacity = 0.75f;
	pSmokeTrail->m_MinSpeed = 10;
	pSmokeTrail->m_MaxSpeed = 20;
	pSmokeTrail->m_MinDirectedSpeed	= 100.0f;
	pSmokeTrail->m_MaxDirectedSpeed	= 120.0f;
	pSmokeTrail->SetLifetime( 5 );
	pSmokeTrail->SetParent( this, nAttachment );

	Vector vecForward( 0, 0, 1 );
	QAngle angles;
	VectorAngles( vecForward, angles );

	if ( nAttachment == 0 )
	{
		pSmokeTrail->SetAbsOrigin( vecPos );
		pSmokeTrail->SetAbsAngles( angles );
	}
	else
	{
		pSmokeTrail->SetLocalOrigin( vec3_origin );
		pSmokeTrail->SetLocalAngles( angles );
	}

	pSmokeTrail->SetMoveType( MOVETYPE_NONE );
}


//------------------------------------------------------------------------------
// Pow!
//------------------------------------------------------------------------------
void CPropAPC2::ExplodeAndThrowChunk( const Vector &vecExplosionPos )
{
	ExplosionCreate( vecExplosionPos, vec3_angle, this, 1000, 500.0f, 
		SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS	|
		SF_ENVEXPLOSION_NOSMOKE  | SF_ENVEXPLOSION_NOFIREBALLSMOKE, 0 );
	UTIL_ScreenShake( vecExplosionPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	// Drop a flaming, smoking chunk.
	CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
	pChunk->Spawn( "models/gibs/hgibs.mdl" );
	pChunk->SetBloodColor( DONT_BLEED );

	QAngle vecSpawnAngles;
	vecSpawnAngles.Random( -90, 90 );
	pChunk->SetAbsOrigin( vecExplosionPos );
	pChunk->SetAbsAngles( vecSpawnAngles );

	int nGib = random->RandomInt( 0, APC_MAX_CHUNKS - 1 );
	pChunk->Spawn( s_pChunkModelName[nGib] );
	pChunk->SetOwnerEntity( this );
	pChunk->m_lifeTime = random->RandomFloat( 6.0f, 8.0f );
	pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
	
	// Set the velocity
	if ( pPhysicsObject )
	{
		pPhysicsObject->EnableMotion( true );
		Vector vecVelocity;

		QAngle angles;
		angles.x = random->RandomFloat( -40, 0 );
		angles.y = random->RandomFloat( 0, 360 );
		angles.z = 0.0f;
		AngleVectors( angles, &vecVelocity );
		
		vecVelocity *= random->RandomFloat( 300, 900 );
		vecVelocity += GetAbsVelocity();

		AngularImpulse angImpulse;
		angImpulse = RandomAngularImpulse( -180, 180 );

		pChunk->SetAbsVelocity( vecVelocity );
		pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
		}

	CEntityFlame *pFlame = CEntityFlame::Create( pChunk, false );
	if ( pFlame != NULL )
	{
		pFlame->SetLifetime( pChunk->m_lifeTime );
	}
	pChunk->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
}


//-----------------------------------------------------------------------------
// Should we trigger a damage effect?
//-----------------------------------------------------------------------------
inline bool CPropAPC2::ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const
{
	int nPrevRange = (int)( ((float)nPrevHealth / (float)GetMaxHealth()) * nEffectCount );
	int nRange = (int)( ((float)GetHealth() / (float)GetMaxHealth()) * nEffectCount );
	return ( nRange != nPrevRange );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::Event_Killed( const CTakeDamageInfo &info )
{
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

	Vector vecAbsMins, vecAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );

	Vector vecNormalizedMins, vecNormalizedMaxs;
	CollisionProp()->WorldToNormalizedSpace( vecAbsMins, &vecNormalizedMins );
	CollisionProp()->WorldToNormalizedSpace( vecAbsMaxs, &vecNormalizedMaxs );

	Vector vecAbsPoint;
	CPASFilter filter( GetAbsOrigin() );
	for (int i = 0; i < 3; i++)
	{
		CollisionProp()->RandomPointInBounds( vecNormalizedMins, vecNormalizedMaxs, &vecAbsPoint );
		te->Explosion( filter, random->RandomFloat( 0.0, 1.0 ),	&vecAbsPoint, 
			g_sModelIndexFireball, random->RandomInt( 4, 10 ), 
			random->RandomInt( 8, 15 ), 
			( i < 2 ) ? TE_EXPLFLAG_NODLIGHTS : TE_EXPLFLAG_NOPARTICLES | TE_EXPLFLAG_NOFIREBALLSMOKE | TE_EXPLFLAG_NODLIGHTS,
			100, 0 );
	}

	// TODO: make the gibs spawn in sync with the delayed explosions
	//int nGibs = random->RandomInt( 1, 4 );
	//for ( i = 0; i < nGibs; i++)
	//{
	//	// Throw a flaming, smoking chunk.
	//	CGib *pChunk = CREATE_ENTITY( CGib, "gib" );
	//	pChunk->Spawn( "models/gibs/hgibs.mdl" );
	//	pChunk->SetBloodColor( DONT_BLEED );

	//	QAngle vecSpawnAngles;
	//	vecSpawnAngles.Random( -90, 90 );
	//	pChunk->SetAbsOrigin( vecAbsPoint );
	//	pChunk->SetAbsAngles( vecSpawnAngles );

	//	int nGib = random->RandomInt( 0, APC_MAX_CHUNKS - 1 );
	//	pChunk->Spawn( s_pChunkModelName[nGib] );
	//	pChunk->SetOwnerEntity( this );
	//	pChunk->m_lifeTime = random->RandomFloat( 6.0f, 8.0f );
	//	pChunk->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	//	IPhysicsObject *pPhysicsObject = pChunk->VPhysicsInitNormal( SOLID_VPHYSICS, pChunk->GetSolidFlags(), false );
	//	
	//	// Set the velocity
	//	if ( pPhysicsObject )
	//	{
	//		pPhysicsObject->EnableMotion( true );
	//		Vector vecVelocity;

	//		QAngle angles;
	//		angles.x = random->RandomFloat( -20, 20 );
	//		angles.y = random->RandomFloat( 0, 360 );
	//		angles.z = 0.0f;
	//		AngleVectors( angles, &vecVelocity );
	//		
	//		vecVelocity *= random->RandomFloat( 300, 900 );
	//		vecVelocity += GetAbsVelocity();

	//		AngularImpulse angImpulse;
	//		angImpulse = RandomAngularImpulse( -180, 180 );

	//		pChunk->SetAbsVelocity( vecVelocity );
	//		pPhysicsObject->SetVelocity(&vecVelocity, &angImpulse );
	//	}

	//	CEntityFlame *pFlame = CEntityFlame::Create( pChunk, false );
	//	if ( pFlame != NULL )
	//	{
	//		pFlame->SetLifetime( pChunk->m_lifeTime );
	//	}
	//	pChunk->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	//}

	UTIL_ScreenShake( vecAbsPoint, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );

	//Ignite( 60, false );

	//m_lifeState = LIFE_DYING;

	// Spawn a lesser amount if the player is close
	/*m_iRocketSalvoLeft = DEATH_VOLLEY_ROCKET_COUNT;
	m_flRocketTime = gpGlobals->curtime;*/
	CreateCorpse();
}



//-----------------------------------------------------------------------------
// Purpose: Blows it up!
//-----------------------------------------------------------------------------
void CPropAPC2::InputDestroy( inputdata_t &inputdata )
{
	CTakeDamageInfo info( this, this, m_iHealth, DMG_BLAST );
	info.SetDamagePosition( WorldSpaceCenter() );
	info.SetDamageForce( Vector( 0, 0, 1 ) );
	TakeDamage( info );
}


//-----------------------------------------------------------------------------
// Aim the next rocket at a specific target
//-----------------------------------------------------------------------------
void CPropAPC2::InputFireMissileAt( inputdata_t &inputdata )
{
	string_t strMissileTarget = MAKE_STRING( inputdata.value.String() );
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, strMissileTarget, NULL );
	if ( pTarget == NULL )
	{
		DevWarning( "%s: Could not find target '%s'!\n", GetClassname(), STRING( strMissileTarget ) );
		return;
	}

	m_hSpecificRocketTarget = pTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropAPC2::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth == 0 )
		return 0;

	CTakeDamageInfo dmgInfo = info;
	if ( dmgInfo.GetDamageType() & (DMG_BLAST | DMG_AIRBOAT) )
	{
		int nPrevHealth = GetHealth();

		m_iHealth -= dmgInfo.GetDamage();
		if ( m_iHealth <= 0 )
		{
			m_iHealth = 0;
			Event_Killed( dmgInfo );
			return 0;
		}

		// Chain
//		BaseClass::OnTakeDamage( dmgInfo );

		// Spawn damage effects
		if ( nPrevHealth != GetHealth() )
		{
			if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_SMOKE_TRAILS ) )
			{
				AddSmokeTrail( dmgInfo.GetDamagePosition() );
			}

			/*if ( ShouldTriggerDamageEffect( nPrevHealth, MAX_EXPLOSIONS ) )
			{
				ExplodeAndThrowChunk( dmgInfo.GetDamagePosition() );
			}*/
		}
	}
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
void CPropAPC2::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	if ( m_flDangerSoundTime > gpGlobals->curtime )
		return;

	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir;

	GetVectors( &vecDir, NULL, NULL );

	// Make danger sounds ahead of the APC
	trace_t	tr;
	Vector	vecSpot, vecLeftDir, vecRightDir;

	// lay down sound path
	vecSpot = vecStart + vecDir * 600;
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	// put sounds a bit to left and right but slightly closer to APC to make a "cone" of sound 
	// in front of it
	QAngle leftAngles = vehicleAngles;
	leftAngles[YAW] += 20;
	VehicleAngleVectors( leftAngles, &vecLeftDir, NULL, NULL );
	vecSpot = vecStart + vecLeftDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	QAngle rightAngles = vehicleAngles;
	rightAngles[YAW] -= 20;
	VehicleAngleVectors( rightAngles, &vecRightDir, NULL, NULL );
	vecSpot = vecStart + vecRightDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this);

	m_flDangerSoundTime = gpGlobals->curtime + 0.3;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::Think( void )
{
	if (!m_bSpawn) {
		if(m_fReloadTime<=gpGlobals->curtime && m_iAmmoCount<50) {
		m_iAmmoCount++;
		m_fReloadTime=gpGlobals->curtime+0.5f;
	}
	if(m_fCannonCharge<=gpGlobals->curtime && m_iCannonCount<100) {
		m_iCannonCount++;
		m_fCannonCharge=gpGlobals->curtime+0.03f;
	}
	BaseClass::Think();

	CBasePlayer	*pPlayer = UTIL_GetLocalPlayer();

	if ( m_bEngineLocked )
	{
		m_bUnableToFire = true;
		
		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}
	else
	{
		// Start this as false and update it again each frame
		m_bUnableToFire = false;

		if ( pPlayer != NULL )
		{
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_VEHICLE_CROSSHAIR;
		}
	}

	SetNextThink( gpGlobals->curtime );

	if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
	{
		m_bInitialHandbrake = true;
		m_VehiclePhysics.SetHandbrake( true );
		m_VehiclePhysics.Think();
	}

	StudioFrameAdvance();

	if ( IsSequenceFinished() )
	{
		int iSequence = SelectWeightedSequence( ACT_IDLE );
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			SetCycle( 0 );
			m_flAnimTime = gpGlobals->curtime;
			ResetSequence( iSequence );
			ResetClientsideFrame();
		}
	}
	if ( m_hPlayer && !m_bExitAnimOn && !m_bEnterAnimOn )
	{
		Vector vecEyeDir, vecEyePos;
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
		AimPrimaryWeapon( vecEndPos );
		//GetRocketShootPosition( &vecEndPos );
		if ( m_hLaserDot != NULL )
		{
			Vector	laserPos = trace.endpos;
			m_hLaserDot->SetAbsOrigin(laserPos);
			
			if ( trace.DidHitNonWorldEntity() )
			{
				CBaseEntity *pHit = trace.m_pEnt;

				if ( ( pHit != NULL ) && ( pHit->m_takedamage ) )
				{
					SetLaserDotTarget( m_hLaserDot, pHit );
					EnableLaserDot( m_hLaserDot, pHit != NULL );
					
				}
				else
				{
					SetLaserDotTarget( m_hLaserDot, NULL );
					EnableLaserDot(m_hLaserDot,true);
					
				}
			}
			else
			{
				SetLaserDotTarget( m_hLaserDot, NULL );
				EnableLaserDot(m_hLaserDot,true);
			}
		}
		
	}
	}
}


//-----------------------------------------------------------------------------
// Aims the secondary weapon at a target 
//-----------------------------------------------------------------------------
void CPropAPC2::AimSecondaryWeaponAt( CBaseEntity *pTarget )
{
	m_hRocketTarget = pTarget;

	// Update the rocket target
	CreateAPCLaserDot();

	if ( m_hRocketTarget )
	{
		m_hLaserDot->SetAbsOrigin( m_hRocketTarget->BodyTarget( WorldSpaceCenter(), false ) );
	}
	SetLaserDotTarget( m_hLaserDot, m_hRocketTarget );
	EnableLaserDot( m_hLaserDot, m_hRocketTarget != NULL );
}

void CPropAPC2::AimSecondaryWeapon(Vector &vecWorldTarget )
{
	//m_hRocketTarget = vecWorldTarget;

	// Update the rocket target
	CreateAPCLaserDot();

	if ( m_hRocketTarget )
	{
		m_hLaserDot->SetAbsOrigin( m_hRocketTarget->BodyTarget( WorldSpaceCenter(), false ) );
	}
	SetLaserDotTarget( m_hLaserDot, m_hRocketTarget );
	EnableLaserDot( m_hLaserDot, m_hRocketTarget != NULL );
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	switch( m_lifeState )
	{
	case LIFE_ALIVE:
		{
			int iButtons = ucmd->buttons;
			if ( iButtons & IN_ATTACK )
			{
			if (m_iAmmoCount!=0)
				FireMachineGun();
			}
			else if ( iButtons & IN_ATTACK2 )
			{
				if (m_iCannonCount>=100) {
				FireRocket();
				m_iCannonCount=0;
				}
			}
		}
		break;

	/*case LIFE_DYING:
		FireDying( );
		break;*/

	case LIFE_DEAD:
		return;
	}

	BaseClass::DriveVehicle( flFrameTime, ucmd, iButtonsDown, iButtonsReleased );
}

void CPropAPC2::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	// Si ya hay un jugador, y el que lo usa no es el conductor
	// entonces olvidate de usarme, chavalote.
	if (m_hPlayer && pPlayer!=m_hPlayer) 
		return;

	if (pPlayer->m_lifeState!=LIFE_ALIVE)
		return;
	if (m_lifeState == LIFE_ALIVE ) {
	BaseClass::Use( pActivator, pCaller, useType, value );
	if ( pActivator->IsPlayer() )
	{
		 EmitSound ( "combine.door_lock" );
	}
	}
}

void KillAPCs(void)
{
CPropAPC2 *pAPC=NULL;
	while ( ( pAPC = (CPropAPC2*)gEntList.FindEntityByClassname( pAPC, "prop_vehicle_apc" )) != NULL )
	{
		// Si el strider esta conducido, entonces no se le mata.
			CTakeDamageInfo info( pAPC, pAPC, 10000, DMG_BLAST );
			info.SetDamagePosition( pAPC->WorldSpaceCenter() );
			info.SetDamageForce( Vector( 0, 0, 1 ) );
			pAPC->TakeDamage( info );
	}
}
static ConCommand sv_killapcs("sv_killapcs", KillAPCs, "Kill and respawn APC's in game",FCVAR_CHEAT);


//-----------------------------------------------------------------------------
// Primary gun 
//-----------------------------------------------------------------------------
void CPropAPC2::AimPrimaryWeapon( const Vector &vecWorldTarget ) 
{
	EntityMatrix parentMatrix;
	parentMatrix.InitFromEntity( this, m_nMachineGunBaseAttachment );
	Vector target = parentMatrix.WorldToLocal( vecWorldTarget ); 

	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x*target.x + target.y*target.y;

	// Target is too close!  Can't aim at it
	if ( quadTarget > m_vecBarrelPos.LengthSqr() )
	{
		// We're trying to aim the offset barrel at an arbitrary point.
		// To calculate this, I think of the target as being on a sphere with 
		// it's center at the origin of the gun.
		// The rotation we need is the opposite of the rotation that moves the target 
		// along the surface of that sphere to intersect with the gun's shooting direction
		// To calculate that rotation, we simply calculate the intersection of the ray 
		// coming out of the barrel with the target sphere (that's the new target position)
		// and use atan2() to get angles

		// angles from target pos to center
		float targetToCenterYaw = atan2( target.y, target.x );
		float centerToGunYaw = atan2( m_vecBarrelPos.y, sqrt( quadTarget - (m_vecBarrelPos.y*m_vecBarrelPos.y) ) );

		float targetToCenterPitch = atan2( target.z, sqrt( quadTargetXY ) );
		float centerToGunPitch = atan2( -m_vecBarrelPos.z, sqrt( quadTarget - (m_vecBarrelPos.z*m_vecBarrelPos.z) ) );

		QAngle angles;
		angles.Init( -RAD2DEG(targetToCenterPitch+centerToGunPitch), RAD2DEG( targetToCenterYaw + centerToGunYaw ), 0 );

		SetPoseParameter( "vehicle_weapon_yaw", angles.y );
		SetPoseParameter( "vehicle_weapon_pitch", angles.x );
		StudioFrameAdvance();

		float curPitch = GetPoseParameter( "vehicle_weapon_pitch" );
		float curYaw = GetPoseParameter( "vehicle_weapon_yaw" );
		m_bInFiringCone = (fabs(curPitch - angles.x) < 1e-3) && (fabs(curYaw - angles.y) < 1e-3);
	}
	else
	{
		m_bInFiringCone = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CPropAPC2::GetTracerType( void ) 
{
	return "HelicopterTracer"; 
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CPropAPC2::DoImpactEffect( trace_t &tr, int nDamageType )
{
	UTIL_ImpactTrace( &tr, nDamageType, "HelicopterImpact" );
} 


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::DoMuzzleFlash( void )
{
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = m_nMachineGunMuzzleAttachment;
	data.m_flScale = 1.0f;
	DispatchEffect( "ChopperMuzzleFlash", data );

	BaseClass::DoMuzzleFlash();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::FireMachineGun( void )
{
	if ( m_flMachineGunTime > gpGlobals->curtime )
		return;

	// If we're still firing the salvo, fire quickly
	m_iMachineGunBurstLeft--;
	if ( m_iMachineGunBurstLeft > 0 )
	{
		m_flMachineGunTime = gpGlobals->curtime + MACHINE_GUN_BURST_TIME;
	}
	else
	{
		// Reload the salvo
		m_iMachineGunBurstLeft = MACHINE_GUN_BURST_SIZE;
		m_flMachineGunTime = gpGlobals->curtime + MACHINE_GUN_BURST_PAUSE_TIME;
	}

	Vector vecMachineGunShootPos;
	QAngle vecMachineGunAngles;
	GetAttachment( m_nMachineGunMuzzleAttachment, vecMachineGunShootPos, vecMachineGunAngles );

	Vector vecMachineGunDir;
	AngleVectors( vecMachineGunAngles, &vecMachineGunDir );
	
	// Fire the round
	int	bulletType = GetAmmoDef()->Index("StriderMiniGun");
	FireBulletsInfo_t info;
		info.m_iShots = 1;
		info.m_vecSrc = vecMachineGunShootPos;
		info.m_vecDirShooting = vecMachineGunDir;
		info.m_vecSpread = VECTOR_CONE_8DEGREES;
		info.m_pAttacker =	(CBaseEntity *) m_hPlayer;
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType =  bulletType;
		info.m_flDamage = 30;
		info.m_iPlayerDamage= 30;
		info.m_iTracerFreq = 1;
		FireBullets( info );
		EntityMessageBegin( this, true );
				WRITE_BYTE( APC_MSG_MACHINEGUN );
				WRITE_VEC3COORD(vecMachineGunShootPos);
				WRITE_VEC3COORD(vecMachineGunDir);
				WRITE_VEC3COORD(VECTOR_CONE_8DEGREES);
				WRITE_BYTE( bulletType );
	MessageEnd();
	DoMuzzleFlash();
	m_iAmmoCount--; 
	EmitSound( "Weapon_AR2.Single" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::GetRocketShootPosition( Vector *pPosition )
{
	QAngle vecRocketAngles;
	GetAttachment( m_nRocketAttachment, *pPosition, vecRocketAngles );
}


//-----------------------------------------------------------------------------
// Create a corpse 
//-----------------------------------------------------------------------------
void CPropAPC2::CreateCorpse( )
{
	m_lifeState = LIFE_DEAD;

	for ( int i = 0; i < APC_MAX_GIBS; ++i )
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
		//pGib->Ignite( 60, false );
		pGib->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}

//

	CPropAPC2 *pAPC = (CPropAPC2 *)CreateEntityByName( "prop_vehicle_apc" );
		
	if ( pAPC )
	{
		pAPC->InicialSpawn=m_vOriginalSpawnOrigin;
		pAPC->InicialAngle=m_vOriginalSpawnAngles;
		pAPC->m_bSpawn=true;
		pAPC->SetThink( &CPropAPC2::Materialize );
		pAPC->SetContextThink( &CPropAPC2::Materialize, gpGlobals->curtime + 5.0f, "RESPAWNING" );
		pAPC->SetNextThink( gpGlobals->curtime + 5.0f );
	}
	else
	{
		Warning("Respawn failed to create %s!\n", GetClassname() );
	}

//

	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
	UTIL_Remove( this );
}

void CPropAPC2::Materialize( void )
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
	SetAbsOrigin( InicialSpawn );
	SetAbsAngles( InicialAngle );
	KeyValue( "model", "models/combine_apc.mdl" );
	KeyValue( "solid", "6" );
	KeyValue( "targetname", "elapc" );
	KeyValue( "vehiclescript", "scripts/vehicles/apc.txt" );
	Teleport( &InicialSpawn, &InicialAngle, NULL );
	Spawn();
	Activate();
	SetThink( &CPropAPC2::Think );
	SetNextThink( gpGlobals->curtime);
	
}
//-----------------------------------------------------------------------------
// Death volley 
//-----------------------------------------------------------------------------
void CPropAPC2::FireDying( )
{
	if ( m_flRocketTime > gpGlobals->curtime )
		return;

	Vector vecRocketOrigin;
	GetRocketShootPosition(	&vecRocketOrigin );

	Vector vecDir;
	vecDir.Random( -1.0f, 1.0f );
	if ( vecDir.z < 0.0f )
	{
		vecDir.z *= -1.0f;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, ROCKET_SPEED * random->RandomFloat( 0.75f, 1.25f ), vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *) CAPCMissile::Create( vecRocketOrigin, angles, vecVelocity, this );
	float flDeathTime = random->RandomFloat( 0.3f, 0.5f );
	if ( random->RandomFloat( 0.0f, 1.0f ) < 0.3f )
	{
		pRocket->ExplodeDelay( flDeathTime );
	}
	else
	{
		pRocket->AugerDelay( flDeathTime );
	}

	// Make erratic firing
	/*m_flRocketTime = gpGlobals->curtime + random->RandomFloat( DEATH_VOLLEY_MIN_FIRE_TIME, DEATH_VOLLEY_MAX_FIRE_TIME );
	if ( --m_iRocketSalvoLeft <= 0 )
	{
		CreateCorpse();
	}*/
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC2::FireRocket( void )
{
	if ( m_flRocketTime > gpGlobals->curtime )
		return;

	// If we're still firing the salvo, fire quickly
	m_iRocketSalvoLeft--;
	if ( m_iRocketSalvoLeft > 0 )
	{
		m_flRocketTime = gpGlobals->curtime + ROCKET_DELAY_TIME;
	}
	else
	{
		// Reload the salvo
		m_iRocketSalvoLeft = ROCKET_SALVO_SIZE;
		m_flRocketTime = gpGlobals->curtime + random->RandomFloat( ROCKET_MIN_BURST_PAUSE_TIME, ROCKET_MAX_BURST_PAUSE_TIME );
	}

	Vector vecRocketOrigin;
	GetRocketShootPosition(	&vecRocketOrigin );

	static float s_pSide[] = { 0.966, 0.866, 0.5, -0.5, -0.866, -0.966 };

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	Vector vecDir;
	CrossProduct( Vector( 0, 0, 1 ), forward, vecDir );
	vecDir.z = 1.0f;
	vecDir.x *= s_pSide[m_nRocketSide];
	vecDir.y *= s_pSide[m_nRocketSide];
	if ( ++m_nRocketSide >= 6 )
	{
		m_nRocketSide = 0;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, ROCKET_SPEED, vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *)CAPCMissile::Create( vecRocketOrigin, angles, vecVelocity, this );
	pRocket->IgniteDelay();

	if ( m_hSpecificRocketTarget )
	{
		pRocket->AimAtSpecificTarget( m_hSpecificRocketTarget );
		m_hSpecificRocketTarget = NULL;
	}
	else if ( m_strMissileHint != NULL_STRING )
	{
		pRocket->SetGuidanceHint( STRING( m_strMissileHint ) );
	}

	EmitSound( "PropAPC.FireRocket" );
	m_OnFiredMissile.FireOutput( this, this );
}
									 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPropAPC2::MaxAttackRange() const
{
	return ROCKET_ATTACK_RANGE_MAX;
}

Vector CPropAPC2::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("cannon_muzzle");
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

void CPropAPC2::EnterVehicle( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

//	CheckWater();
	BaseClass::EnterVehicle( pPlayer );

	// Start looking for seagulls to land
	//m_hLastPlayerInVehicle = m_hPlayer;
	//SetContextThink( NULL, 0, g_pJeepThinkContext );
}
//========================================================================================================================================
// APC FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CPropAPC2 *CAPC2FourWheelServerVehicle::GetAPC( void )
{
	return (CPropAPC2*)GetDrivableVehicle();
}

void CAPC2FourWheelServerVehicle::NPC_AimPrimaryWeapon( Vector vecTarget )
{
	CPropAPC2 *pAPC = ((CPropAPC2*)m_pVehicle);
	pAPC->AimPrimaryWeapon( vecTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPC2FourWheelServerVehicle::NPC_AimSecondaryWeapon( Vector vecTarget )
{
	// Add some random noise
//	Vector vecOffset = vecTarget + RandomVector( -128, 128 );
//	((CPropAPC2*)m_pVehicle)->AimSecondaryWeaponAt( vecOffset );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPC2FourWheelServerVehicle::Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = MACHINE_GUN_ATTACK_RANGE_MIN;
	*flMaxRange = MACHINE_GUN_ATTACK_RANGE_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPC2FourWheelServerVehicle::Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = ROCKET_ATTACK_RANGE_MIN;
	*flMaxRange = ROCKET_ATTACK_RANGE_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's primary weapon can fire again
//-----------------------------------------------------------------------------
float CAPC2FourWheelServerVehicle::Weapon_PrimaryCanFireAt( void )
{
	return ((CPropAPC2*)m_pVehicle)->PrimaryWeaponFireTime();
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's secondary weapon can fire again
//-----------------------------------------------------------------------------
float CAPC2FourWheelServerVehicle::Weapon_SecondaryCanFireAt( void )
{
	return ((CPropAPC2*)m_pVehicle)->SecondaryWeaponFireTime();
}

void CAPC2FourWheelServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{


	Assert( nRole == VEHICLE_DRIVER );
	CBaseCombatCharacter *pPlayer = GetPassenger( VEHICLE_ROLE_DRIVER );
	Assert( pPlayer );

	float flPitchFactor=1.0;
	*pAbsAngles = pPlayer->EyeAngles();
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAPC()->GetAttachment( "cannon_muzzle", vehicleEyeOrigin, vehicleEyeAngles );
	Vector up,forward;
	GetAPC()->GetVectors(NULL,&forward,&up);
	vehicleEyeOrigin+=(forward*37)+(up*35);
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
#endif