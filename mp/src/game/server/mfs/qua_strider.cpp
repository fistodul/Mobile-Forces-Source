// ======================================================================
// Purpose: Vamos a intentar crear el Strider en MP scratch. Asi, se. 
// podra exportar a cualquier mod que desee tener un Pilotable Strider.
// Si este Strider funciona pelado en este Mod, significa que funcionará
// en cualquier mod. Por ejemplo, en el HL2DM. 
// Intento: Es crucial mantener las funciones para poder pasarlas al
// helicoptero, o cualquier otro objeto pilotable.
// ======================================================================
 
// didn't take the time to sort out which of these are actually needed
#include "cbase.h"
#ifdef pilotable
#include "vehicle_base.h"
#include "in_buttons.h"
#include "qua_strider.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "studio.h"
#include "physics_bone_follower.h"
#include "world.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "physics_prop_ragdoll.h"
#include "hl2_player.h"
#include "ammodef.h"
#include "npcevent.h"
#include "ai_utils.h"
#include "te_effect_dispatch.h"
#include "effect_dispatch_data.h"
#include "beam_flags.h"
#include "movevars_shared.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "rope.h"
#include "shake.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
LINK_ENTITY_TO_CLASS( qua_strider, QUA_Strider );

// Start of our data description for the class
BEGIN_DATADESC( QUA_Strider )
	
	// Save/restore our active state
	
	DEFINE_FIELD( m_ActualSequence, FIELD_INTEGER),
	DEFINE_FIELD( m_flSequenceTime, FIELD_TIME),
	DEFINE_FIELD( m_BodyTargetBone,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iCaps, FIELD_INTEGER ),
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),

	// This is reconstructed in CreateVPhysics
	// DEFINE_EMBEDDED( m_BoneFollowerManager ),

	
	


	// Links our input name from Hammer to our input member function



END_DATADESC()

// De npc_strider.cpp

//---------------------------------------------------------

Vector QUA_Strider::gm_cullBoxStandMins;
Vector QUA_Strider::gm_cullBoxStandMaxs;
Vector QUA_Strider::gm_cullBoxCrouchMins;
Vector QUA_Strider::gm_cullBoxCrouchMaxs;
float QUA_Strider::gm_strideLength;

int QUA_Strider::gm_BodyHeightPoseParam;
int QUA_Strider::gm_YawControl;
int QUA_Strider::gm_PitchControl;
int QUA_Strider::gm_CannonAttachment;
int QUA_Strider::gm_DriverEyes;

float QUA_Strider::gm_zCannonDist;
float QUA_Strider::gm_zMinigunDist;
Vector QUA_Strider::gm_vLocalRelativePositionCannon;
Vector QUA_Strider::gm_vLocalRelativePositionMinigun;


static const char *pFollowerBoneNames[] =
{
	// head
	"Combine_Strider.Body_Bone",
	// lower legs
	"Combine_Strider.Leg_Left_Bone1",
	"Combine_Strider.Leg_Right_Bone1",
	"Combine_Strider.Leg_Hind_Bone1",
	// upper legs
	"Combine_Strider.Leg_Left_Bone",
	"Combine_Strider.Leg_Right_Bone",
	"Combine_Strider.Leg_Hind_Bone",
};

enum
{
	STRIDER_BODY_FOLLOWER_INDEX = 0,
	STRIDER_LEFT_LEG_FOLLOWER_INDEX,
	STRIDER_RIGHT_LEG_FOLLOWER_INDEX,
	STRIDER_BACK_LEG_FOLLOWER_INDEX,

	STRIDER_LEFT_UPPERLEG_FOLLOWER_INDEX,
	STRIDER_RIGHT_UPPERLEG_FOLLOWER_INDEX,
	STRIDER_BACK_UPPERLEG_FOLLOWER_INDEX,
};

extern void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );


#define STRIDER_DEFAULT_SHOOT_DURATION			2.5 // spend this much time stitching to each target.
#define STRIDER_SHOOT_VARIATION					1.0 // up to 1 second of variance
#define STRIDER_SHOOT_DOWNTIME					1.0 // This much downtime between bursts
#define STRIDER_SUBSEQUENT_TARGET_DURATION		1.5 // Spend this much time stitching to targets chosen by distributed fire.
#define STRIDER_IGNORE_TARGET_DURATION			1.0
#define STRIDER_IGNORE_PLAYER_DURATION			1.5

//Animation events
#define STRIDER_AE_FOOTSTEP_LEFT		1
#define STRIDER_AE_FOOTSTEP_RIGHT		2
#define STRIDER_AE_FOOTSTEP_BACK		3
#define STRIDER_AE_FOOTSTEP_LEFTM		4
#define STRIDER_AE_FOOTSTEP_RIGHTM		5
#define STRIDER_AE_FOOTSTEP_BACKM		6
#define STRIDER_AE_FOOTSTEP_LEFTL		7
#define STRIDER_AE_FOOTSTEP_RIGHTL		8
#define STRIDER_AE_FOOTSTEP_BACKL		9
#define STRIDER_AE_WHOOSH_LEFT			11
#define STRIDER_AE_WHOOSH_RIGHT			12
#define STRIDER_AE_WHOOSH_BACK			13
#define STRIDER_AE_CREAK_LEFT			21
#define STRIDER_AE_CREAK_RIGHT			22
#define STRIDER_AE_CREAK_BACK			23
#define STRIDER_AE_SHOOTCANNON			100
#define STRIDER_AE_CANNONHIT			101
#define STRIDER_AE_SHOOTMINIGUN			105
#define STRIDER_AE_STOMPHITL			110
#define STRIDER_AE_STOMPHITR			111
#define STRIDER_AE_FLICKL				112
#define STRIDER_AE_FLICKR				113

#define STRIDER_AE_DIE					999

// UNDONE: Share properly with the client code!!!
#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3
#define STRIDER_MSG_MACHINEGUN			4 // Añadido por mi para que el cliente gire
#define STOMP_IK_SLOT					11

// can hit anything within this range
#define STRIDER_STOMP_RANGE				260

// Crouch down if trying to shoot an enemy that's this close
#define STRIDER_CROUCH_RANGE			4000.0f

// Stand up again if crouched and engaging an enemy at this distance
#define STRIDER_STAND_RANGE				6000.0f

#define STRIDER_NO_TRACK_NAME			"null"

// Time after which if you haven't seen your enemy you stop facing him
#define STRIDER_TIME_STOP_FACING_ENEMY 3.0 
// Nombre de la entidad

#define MINIGUN_MAX_YAW		90.0f
#define MINIGUN_MIN_YAW		-90.0f
#define MINIGUN_MAX_PITCH	45.0f
#define MINIGUN_MIN_PITCH	-45.0f

#define	ENTITY_MODEL	"models/combine_strider.mdl"
//
IMPLEMENT_SERVERCLASS_ST(QUA_Strider, DT_NPC_QUA_Strider)
	SendPropVector(SENDINFO(m_vecHitPos), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_EyePosition), -1, SPROP_COORD),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 0 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 1 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 2 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 3 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 4 ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO_NETWORKARRAYELEM( m_vecIKTarget, 5 ), -1, SPROP_COORD ),
	SendPropInt		(SENDINFO(m_iHealth), 10 ),
	SendPropInt		(SENDINFO(m_iAmmoCount), 10 ),
	SendPropInt	(SENDINFO(m_iCannonCount), 10),
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropBool(SENDINFO(m_bEnterAnimOn)),
	SendPropBool(SENDINFO(m_bExitAnimOn)),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropVector(SENDINFO(m_vecEyeExitEndpoint), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vecGunCrosshair), -1, SPROP_COORD),
END_SEND_TABLE()

static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs );

int s_QUA_iImpactEffectTexture = -1;

ConVar qua_strider_shake_ropes_radius( "qua_strider_shake_ropes_radius", "1200" );
ConVar qua_strider_shake_ropes_magnitude( "qua_strider_shake_ropes_magnitude", "150" );

//---------------------------------------------------------
//---------------------------------------------------------
QUA_Strider::QUA_Strider()
{
	m_pMinigun = new CQUAStriderMinigun;
	//m_bHasGun = true;
	m_pServerVehicle.SetVehicle( this );
	m_vecGunCrosshair.Init();

}
QUA_Strider::~QUA_Strider()
{
	delete m_pMinigun;
}
//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void QUA_Strider::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/combine_strider.mdl" );
	m_pServerVehicle.Initialize( STRING(m_vehicleScript) );
	PrecacheScriptSound( "NPC_Strider.Charge" );
	PrecacheScriptSound( "NPC_Strider.RagdollDetach" );
	PrecacheScriptSound( "NPC_Strider.Whoosh" );
	PrecacheScriptSound( "NPC_Strider.Creak" );
	PrecacheScriptSound( "NPC_Strider.Alert" );
	PrecacheScriptSound( "NPC_Strider.Pain" );
	PrecacheScriptSound( "NPC_Strider.Death" );
	PrecacheScriptSound( "NPC_Strider.FireMinigun" );
	PrecacheScriptSound( "NPC_Strider.Shoot" );
	PrecacheScriptSound( "NPC_Strider.OpenHatch" );
	PrecacheScriptSound( "NPC_Strider.Footstep" );
	PrecacheScriptSound( "NPC_Strider.Skewer" );
	PrecacheMaterial( "effects/water_highlight" );
	s_QUA_iImpactEffectTexture = PrecacheModel( "sprites/physbeam.vmt" );
	PrecacheMaterial( "sprites/bluelaser1" );
	PrecacheMaterial( "effects/blueblacklargebeam" );
	PrecacheMaterial( "effects/strider_pinch_dudv" );
	PrecacheMaterial( "effects/blueblackflash" );
	PrecacheMaterial( "effects/strider_bulge_dudv" );
	PrecacheMaterial( "effects/strider_muzzle" );

	//PrecacheModel( ENTITY_MODEL );
	/*m_ServerVehicle.Initialize( STRING(m_vehicleScript) );*/
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void QUA_Strider::Spawn( void )
{
	
	Precache();
	SetModel( ENTITY_MODEL );
//	SetVehicleType( VEHICLE_TYPE_CAR_RAYCAST );
	SetCollisionGroup(COLLISION_GROUP_VEHICLE);
	BaseClass::Spawn();
	m_miniGunAmmo = GetAmmoDef()->Index("StriderMinigun");
	m_pMinigun->Init();
	m_flSequenceTime=gpGlobals->curtime+4.0f;
	m_flNextShootingTime=gpGlobals->curtime;
	m_fReloadTime=gpGlobals->curtime+0.5f;
	m_fCannonCharge=gpGlobals->curtime+0.05f;
	m_bCrouching=false;
	m_bStanding=false;
	m_bCrouchPosture=true;
	m_bStandPosture=false;
	aux=true;
	m_iHealth=500;
	m_iAmmoCount=50; // Balas seguidas que podemos disparar
	m_iCannonCount=100; // Tanto por cien de cañon.
	m_bMuerte=false;
	// Nadie esta en el Strider al spawnear
	m_bInStrider=false;
	force_exit=false;
	acl=0;
	aclizq=0;
	aclder=0;
	// Encendemos IK
	// La encendemos en cliente a ver que pasa
	EnableServerIK();

	//
	
	
	//SetModel( ENTITY_MODEL );
	//AddFlag( FL_FLY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLY );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	m_iCaps = FCAP_IMPULSE_USE;
	SetupGlobalModelData();
	
	//UTIL_SetSize(this, gm_cullBoxStandMins,gm_cullBoxStandMaxs);

	UTIL_SetSize(this,Vector(-38,-38, -328),Vector(60,38,-252));
	// Esto sirve para que al empezar, el Strider se ponga de pie
	m_vOriginalMins=Vector(-38,-38,-328);
	m_vOriginalMaxs=Vector(60,38,-252);
	Vector mins(-16,-16,-16), maxs(16,16,16);
	Vector origin = GetAbsOrigin();
	Vector up;
	GetVectors(NULL,NULL,&up);
	//
	MoveToGround( &origin, this, mins, maxs );
	origin.z+=GetMaxHeight();
	SetAbsOrigin( origin );
	
	// Sacado de NPCinit
	
	// Constantemente llama a IKLock con esto
	InitBoneControllers( );
	CreateVPhysics();

	//SetActivity( ACT_IDLE );

	m_BodyTargetBone = -1;

	// Indispensable para que sea vulnerable
	m_takedamage		= DAMAGE_YES;

	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();
	m_flRespawnTime=gpGlobals->curtime+7.0f;
	m_bSpawn=false;
	SetThink( &QUA_Strider::Think );
	SetNextThink( gpGlobals->curtime);
	//SetNextThink( gpGlobals->curtime);
}
void QUA_Strider::Dispara(void)
{
		if (m_hPlayer) {
			Vector vecEyeDir, vecEyePos;
			m_hPlayer->EyePositionAndVectors(&vecEyePos, &vecEyeDir, NULL, NULL);

			// Trace out from the player's eye point.
			Vector	vecEndPos = vecEyePos + (vecEyeDir * MAX_TRACE_LENGTH);
			trace_t	trace;
			UTIL_TraceLine(vecEyePos, vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace);

			// See if we hit something, if so, adjust end position to hit location.
			if (trace.fraction < 1.0)
			{
				vecEndPos = vecEyePos + (vecEyeDir * MAX_TRACE_LENGTH * trace.fraction);
			}
			Vector DondeApuntaPlayer2 = vecEndPos; //Fucking FixMe
	if (m_iAmmoCount!=0)
		ShootMinigun(&DondeApuntaPlayer2, m_pMinigun->GetAimError(), vec3_origin);
		}
}
Vector QUA_Strider::DondeApuntaPlayer(void) 
{
	//if (m_hPlayer) {
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
	return vecEndPos;
	//}
}
void QUA_Strider::TakeDamage( const CTakeDamageInfo &inputInfo )
{
	OnTakeDamage(inputInfo);
}
int	QUA_Strider::VPhysicsTakeDamage( const CTakeDamageInfo &info ) 
{
	// llamara aqui?
	return 0;
}	
int QUA_Strider::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth == 0 )
		return 0;

	CTakeDamageInfo dmgInfo = info;
	float dim=dmgInfo.GetDamage();
	if ( dmgInfo.GetInflictor() && dmgInfo.GetInflictor()->GetOwnerEntity() == this ) {
		dmgInfo.SetDamage(dim*0.0);

	} else if( (dmgInfo.GetDamageType() & DMG_BULLET) || (dmgInfo.GetDamageType() & DMG_SLASH) ||
		(dmgInfo.GetDamageType() & DMG_BUCKSHOT) || (dmgInfo.GetDamageType() & DMG_CLUB) )
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

void QUA_Strider::StartSmoking( void )
{
	if ( m_hSmoke != NULL )
		return;

	m_hSmoke = SmokeTrail::CreateSmokeTrail();
	
	if ( m_hSmoke )
	{
		m_hSmoke->m_SpawnRate			= 32;
		m_hSmoke->m_ParticleLifetime	= 3.0;
		m_hSmoke->m_StartSize			= 16;
		m_hSmoke->m_EndSize				= 64;
		m_hSmoke->m_SpawnRadius			= 20;
		m_hSmoke->m_MinSpeed			= 8;
		m_hSmoke->m_MaxSpeed			= 64;
		m_hSmoke->m_Opacity 			= 0.3;
		
		m_hSmoke->m_StartColor.Init( 0.25f, 0.25f, 0.25f );
		m_hSmoke->m_EndColor.Init( 0, 0, 0 );
		m_hSmoke->SetLifetime( 500.0f );
		m_hSmoke->FollowEntity( this, "MiniGunBase" );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void QUA_Strider::StopSmoking( float flDelay )
{
	if ( m_hSmoke )
	{
		m_hSmoke->SetLifetime( flDelay );
	}
}
float QUA_Strider::DamageModifier ( CTakeDamageInfo &info ) 
{ 
	CTakeDamageInfo DmgInfo = info;
	// bullets, slashing and headbutts don't hurt us in the apc, neither do rockets
	if( (DmgInfo.GetDamageType() & DMG_BULLET) || (DmgInfo.GetDamageType() & DMG_SLASH) ||
		(DmgInfo.GetDamageType() & DMG_CLUB) || (DmgInfo.GetDamageType() & DMG_BLAST) )
	{
		return (0.1);
	}
	else
	{
		return 0.3; 
	}
}


void QUA_Strider::Event_Killed( const CTakeDamageInfo &info )
{
	//m_lifeState=LIFE_DYING;

	// Calculate death force
	m_vecTotalBulletForce = CalcDamageForceVector( info );

	CBasePlayer *pPlayer = m_hPlayer;
	if ( pPlayer )
		 {
		force_exit=true;
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
	m_BoneFollowerManager.DestroyBoneFollowers();
	//BaseClass::Event_Killed( info );
	
	// Stop our large cannon
	EntityMessageBegin( this, true );
		WRITE_BYTE( STRIDER_MSG_DEAD );
	MessageEnd();
	//DestroyServerVehicle();
	Vector up,forward;
	GetVectors(&forward,NULL,&up);
	if (IsInCrouchedPosture()) {
	ExplosionCreate( EyePosition()+up*-350, GetAbsAngles(), this, 500, 256, (SF_ENVEXPLOSION_NOSPARKS|SF_ENVEXPLOSION_NODLIGHTS|SF_ENVEXPLOSION_NODAMAGE|SF_ENVEXPLOSION_NOSMOKE), false );
	} else {
	ExplosionCreate( EyePosition()+up*-50, GetAbsAngles(), this, 500, 256, (SF_ENVEXPLOSION_NOSPARKS|SF_ENVEXPLOSION_NODLIGHTS|SF_ENVEXPLOSION_NODAMAGE|SF_ENVEXPLOSION_NOSMOKE), false );
	}
	EmitSound( "NPC_Strider.Death" );

	BecomeRagdoll( info, m_vecTotalBulletForce );
    //BecomeRagdollOnClient(forceVector);
	//Dissolve(NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL);
}

Vector QUA_Strider::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	if ( info.GetDamageForce() != vec3_origin || (info.GetDamageType() & /*DMG_NO_PHYSICS_FORCE*/DMG_BLAST) )
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
static ConVar qua_dxlevel( "qua_dxlevel", "0" );
bool QUA_Strider::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector ) 
{ 
	
		/*CRagdollProp *pRagdoll = NULL;
		pRagdoll = assert_cast<CRagdollProp *>( CreateServerRagdoll( this, m_nForceBone, info, COLLISION_GROUP_VEHICLE ) );
		pRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime+2.0f, false, ENTITY_DISSOLVE_NORMAL );
		*/
		
		CreateRagdollEntity();
		m_bMuerte=true;
		m_fKillStrider=gpGlobals->curtime+0.1f;
		//pRagdoll->Dissolve
		//pRagdoll->DisableAutoFade();
		
		// Creamos un nuevo fantastico Strider
		
	
	//AddEffects(EF_NODRAW);
	//AddSolidFlags(FSOLID_NOT_SOLID);
	////UTIL_Remove(this);
	return true; 
}



void KillStriders(void)
{
QUA_Strider *pStrider=NULL;
	while ( ( pStrider = (QUA_Strider*)gEntList.FindEntityByClassname( pStrider, "qua_strider" )) != NULL )
	{
		// Si el strider esta conducido, entonces no se le mata.
			CTakeDamageInfo info( pStrider, pStrider, 10000, DMG_BLAST );
			info.SetDamagePosition( pStrider->WorldSpaceCenter() );
			info.SetDamageForce( Vector( 0, 0, 1 ) );
			pStrider->TakeDamage( info );
	}
}
static ConCommand sv_killstriders("sv_killstriders", KillStriders, "Kill and respawn striders in game",FCVAR_CHEAT);


void QUA_Strider::KillStrider(void) {
	QUA_Strider *pStrider = (QUA_Strider *)CreateEntityByName( "qua_strider" );
		
	if ( pStrider )
	{
		pStrider->m_bSpawn=true;
		pStrider->m_iSpawnTry=0;
		pStrider->m_hRagdoll=m_hRagdoll;
		pStrider->SetThink( &QUA_Strider::Materialize );
		pStrider->SetContextThink( &QUA_Strider::Materialize, gpGlobals->curtime + 5.0f, "RESPAWNING" );
		pStrider->SetNextThink( gpGlobals->curtime + 5.0f );
		pStrider->Teleport( &m_vOriginalSpawnOrigin, &m_vOriginalSpawnAngles, NULL );	
		
		//pStrider->AddEffects(EF_NODRAW);
		//pStrider->AddSolidFlags(FSOLID_NOT_SOLID);
	}
	else
	{
		Warning("Respawn failed to create %s!\n", GetClassname() );
	}
	AddEffects(EF_NODRAW);
	AddSolidFlags(FSOLID_NOT_SOLID);
	this->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_ELECTRICAL );
	//this->Remove();
}
//--------------------------------------------------
// Purpose: Funcion unica del Strider.
//--------------------------------------------------
void QUA_Strider::SetupGlobalModelData() {
	
	// Recoge todos los attachments

	gm_BodyHeightPoseParam = LookupPoseParameter( "body_height" );
	gm_YawControl = LookupPoseParameter( "yaw" );
	gm_PitchControl = LookupPoseParameter( "pitch" );
	gm_CannonAttachment = LookupAttachment( "BigGun" );
	gm_DriverEyes = LookupAttachment("vehicle_driver_eyes");

	// Recoge las colisiones

	ExtractBbox( SelectHeaviestSequence( ACT_WALK ), gm_cullBoxStandMins, gm_cullBoxStandMaxs ); 
	ExtractBbox( SelectHeaviestSequence( ACT_WALK_CROUCH ), gm_cullBoxCrouchMins, gm_cullBoxCrouchMaxs );
	QUA_Strider::gm_strideLength = (gm_cullBoxStandMaxs.x - gm_cullBoxStandMins.x) * 0.5;
	
	CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );
}
bool QUA_Strider::CreateVPhysics()
{
	if ( !m_bDisableBoneFollowers )
	{
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames );
	}
	BaseClass::CreateVPhysics();
	return true;
}
void QUA_Strider::Activate()
{
	BaseClass::Activate();

	const char *pszBodyTargetBone = "combine_strider.neck_bone";

	m_BodyTargetBone = LookupBone( pszBodyTargetBone );

	if ( m_BodyTargetBone == -1 )
	{
		DevMsg( "Couldn't find npc_strider bone %s, which is used as target for others\n", pszBodyTargetBone );
	}

	gm_BodyHeightPoseParam = LookupPoseParameter( "body_height" );
	gm_YawControl = LookupPoseParameter( "yaw" );
	gm_PitchControl = LookupPoseParameter( "pitch" );
	gm_CannonAttachment = LookupAttachment( "BigGun" );
	gm_DriverEyes= LookupAttachment("vehicle_driver_eyes");

	if ( gm_zCannonDist == 0 )
	{
		// Have to create a virgin strider to ensure proper pose
		QUA_Strider *pStrider = (QUA_Strider *)CreateEntityByName( "qua_strider" );
		Assert(pStrider);
		pStrider->m_bDisableBoneFollowers = true; // don't create these since we're just going to destroy him
		pStrider->Spawn();

		pStrider->SetActivity( ACT_WALK );
		pStrider->InvalidateBoneCache();
		gm_zCannonDist = pStrider->CannonPosition().z - pStrider->GetAbsOrigin().z;

		// Currently just using the gun for the vertical component!
		Vector defEyePos;
		pStrider->GetAttachment( "minigunbase", defEyePos );
		gm_zMinigunDist = defEyePos.z - pStrider->GetAbsOrigin().z;

		Vector position;
		pStrider->GetAttachment( "biggun", position );
		VectorITransform( position, pStrider->EntityToWorldTransform(), gm_vLocalRelativePositionCannon );

		pStrider->GetAttachment( "minigun", position );
		VectorITransform( position, pStrider->EntityToWorldTransform(), gm_vLocalRelativePositionMinigun );
		UTIL_Remove( pStrider );
	}
}

void QUA_Strider::InitBoneControllers()
{
	BaseClass::InitBoneControllers( );
	SetHeight( GetMinHeight() );
	SetIdealHeight( GetMinHeight() );
}
void QUA_Strider::CalculateIKLocks( float currentTime )
{

	BaseClass::CalculateIKLocks( currentTime );
	if ( m_pIk && m_pIk->m_target.Count() )
	{
		Assert(m_pIk->m_target.Count() > STOMP_IK_SLOT);
		// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
		CIKTarget &target = m_pIk->m_target[STOMP_IK_SLOT];
		target.SetPos( m_vecHitPos.Get() );
		for ( int i = 0; i < NUM_STRIDER_IK_TARGETS; i++ )
		{
			target = m_pIk->m_target[i];

			if (!target.IsActive())
				continue;

			m_vecIKTarget.Set( i, target.est.pos );
			

#if 0
		 yellow box at target pos - helps debugging
		if (i == 0)
			NDebugOverlay::Line( GetAbsOrigin(), m_vecIKTarget[i], 255, 255, 0, 0, 0.1 );
		 	NDebugOverlay::Box( m_vecIKTarget[i], Vector(-8,-8,-8), Vector(8,8,8), 255, 255, 0, 0, 4.0 );
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Funcion iterativa que se ejecuta cada ciclo de reloj
//-----------------------------------------------------------------------------

void QUA_Strider::Think(void)
{
	if (!m_bMuerte) {
	if (!m_bSpawn) {
	//engine->Con_NPrintf( 23, "Cannon: %i",m_iCannonCount );
	//BaseClass::Think();
	// Tiempo de recarga de la ametralladora
	
	if(m_fReloadTime<=gpGlobals->curtime && m_iAmmoCount<50) {
		m_iAmmoCount++;
		m_fReloadTime=gpGlobals->curtime+0.5f;
	}
	if(m_fCannonCharge<=gpGlobals->curtime && m_iCannonCount<100) {
		m_iCannonCount++;
		m_fCannonCharge=gpGlobals->curtime+0.1f;
	}

	if ( m_hPlayer)
	{
		
	/*engine->Con_NPrintf( 10, " " );
	engine->Con_NPrintf( 11, "PILOTABLE VEHICLE STRIDER" );
	engine->Con_NPrintf( 12, " " );
	engine->Con_NPrintf( 13, "Cursors : Movement" );
	engine->Con_NPrintf( 14, "Attack 1 : Machine Gun" );
	engine->Con_NPrintf( 15, "Attack 2 : Anti Matter Gun" );
	engine->Con_NPrintf( 16, "Jump: Leg Attack" );
	engine->Con_NPrintf( 17, "Duck: Crouch/Stand" );*/
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
		m_vecGunCrosshair = vecEndPos;
		//engine->Con_NPrintf( 26, "TvecGunCross x:%f,y:%f,z:%f", m_vecGunCrosshair.x, vecEndPos.y,vecEndPos.x);
			
		if (m_DisCanon) {
			if (m_flCannonShot<=gpGlobals->curtime) {
				FireCannonManually();
				m_DisCanon=false;
			}
		AimCannonAtManual( vecEndPos, 0.1 );
		}
		
		m_pMinigun->AimAtPoint(this,vecEndPos);
		if (m_bStompAnim) {
			m_vecHitPos=CalculateStompHitByPlayer();
		}
	} else {
		if (acl>0) {
			acl-=0.02;
		} else if (acl<0) {
			acl=0;
		} else {
			acl=0;
		}
	}
	// Bien, funciona, ahora podriamos examinar el tema de girar
	if (!IsCrouching() && !IsStanding() && !m_bStompAnim && !m_DisCanon) {
	Adelante();
	GiraIzquierda();
	GiraDerecha();
	}
	float diferencia; //Esta variable comprobará quien gira
	diferencia=aclizq-aclder;
	if (diferencia!=0) {
		m_IsTurning=true;
		// Bien, aqui comprobamos que esta girando.
		if (diferencia>0) {
			//gira izquierda
			SetPoseParameter("move_yaw",90);
		} else {
			SetPoseParameter("move_yaw",-90);
		}
	} else {
		m_IsTurning=false;
			SetPoseParameter("move_yaw",0);
	}
	if (!IsCrouching() && !IsStanding() && !m_bStompAnim) {
		if (acl!=0) {
		SetActivity(ACT_RUN);
	} else {
		if (!m_IsTurning) {
		SetActivity(ACT_IDLE);
		} else {
		SetActivity(ACT_RUN);
		}
	}
	}
	
		
			PostThink(); // Pasamos a ACT_IDLE
		
	
	//DevMsg("Nos dice que es %b",IsViewModel());
	SetNextThink( gpGlobals->curtime);
	}
	} else {
		//if (m_fKillStrider<=gpGlobals->curtime) {
		KillStrider();
		//}
	}
}

void QUA_Strider::Materialize( void )
{
	//trace_t tr;
	//UTIL_TraceHull( m_vOriginalSpawnOrigin, m_vOriginalSpawnOrigin, Vector(-38,-38,-38),Vector(60,38,38), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	//if ( (tr.startsolid || tr.allsolid) && m_iSpawnTry<3 )
	//{
	//	//Try again in a second.
	//	SetContextThink(&QUA_Strider::Materialize, gpGlobals->curtime + 1.0f, "RESPAWNING" );
	//	SetNextThink( gpGlobals->curtime + 1.0f );
	//	m_iSpawnTry++;
	//	return;
	//}
	if ( m_hRagdoll )
	{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}
	Spawn();
	SetThink( &QUA_Strider::Think );
	SetNextThink( gpGlobals->curtime);
	
}

void QUA_Strider::CannonHitThink()
{
	CBasePlayer *pPlayer=m_hPlayer;
	
	if (pPlayer) {
		//Warning ("Hace caso al player");
		CreateConcussiveBlast( m_blastHit, m_blastNormal, pPlayer->GetBaseEntity(), 2.5 );
	} else {
		CreateConcussiveBlast( m_blastHit, m_blastNormal, this, 2.5 );
	}
		m_DisCanon=false;
		m_iCannonCount=0;
}
bool QUA_Strider::AimCannonAtManual( Vector pTarget, float flInterval )
{
	matrix3x4_t gunMatrix;
	GetAttachment( gm_CannonAttachment, gunMatrix );

	// transform the enemy into gun space
	m_vecHitPos = pTarget;
	Vector localEnemyPosition;
	VectorITransform( pTarget, gunMatrix, localEnemyPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	Vector unitAngles = Vector( localEnemyAngles.x, localEnemyAngles.y, localEnemyAngles.z ); 
	float angleDiff = VectorNormalize(unitAngles);
	const float aimSpeed = 16;

	// Exponentially approach the target
	float yawSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.y);
	float pitchSpeed = fabsf(aimSpeed*flInterval*localEnemyAngles.x);

	yawSpeed = max(yawSpeed,5);
	pitchSpeed = max(pitchSpeed,5);

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( gm_YawControl, m_aimYaw );
	SetPoseParameter( gm_PitchControl, m_aimPitch );

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = GetPoseParameter( gm_PitchControl );
	m_aimYaw = GetPoseParameter( gm_YawControl );

	// UNDONE: Zero out any movement past the limit and go ahead and fire if the strider hit its 
	// target except for clamping.  Need to clamp targets to limits and compare?
	if ( angleDiff < 1 )
		return true;

	return false;
}
//-----------------------------------------------------------------------------
// Purpose: Funcion que se ejecuta despues de cada pensamiento
//-----------------------------------------------------------------------------

void QUA_Strider::PostThink(void) {
	if (IsActivityFinished()) {
		// Si estaba bajando
		if (IsCrouching()) {
			
			m_bStanding=false;
			m_bCrouching=false;
			m_bCrouchPosture=true;
			m_bStandPosture=false;
			SetHeight(GetMinHeight());
			SetIdealHeight(GetMinHeight());
			UTIL_SetSize(this,Vector(-38,-38, -328),Vector(60,38,-252));
			// FIXME: Wait 0.2 miliseconds before set Activity
			SetActivity(ACT_IDLE);
		}
		// Si estaba subiendo
		if (IsStanding()) {
			m_bStanding=false;
			m_bCrouching=false;
			m_bCrouchPosture=false;
			m_bStandPosture=true;
			SetHeight(GetMaxHeight());
			SetIdealHeight(GetMaxHeight());
			UTIL_SetSize(this,Vector(-38,-38, -150),Vector(60,38,38));
			// FIXME: Wait 0.2 miliseconds before set Activity
			SetActivity(ACT_IDLE);
		}
		if (m_bStompAnim) {
			m_bStompAnim=false;
			SetActivity(ACT_IDLE);
		}
	}
	//	if (aux) {
	//		SetActivity(ACT_STAND);
	//		m_bStanding=true;
	//		aux=false;
	//	}
	//}

	StudioFrameAdvance( ); // Esta funcion anima las secuencias.

	// Porque no hara caso de los pose parameters??.
	// En single player tampoco hace caso, pero ya esta claro.

	m_BoneFollowerManager.UpdateBoneFollowers(this); // refresca los bone.
	DispatchAnimEvents(this);
}
static Vector GetAttachmentPositionInSpaceOfBoneSTR( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex )
{
	int attachment = Studio_FindAttachment( pStudioHdr, pAttachmentName );

	Vector localAttach;
	const mstudioattachment_t &pAttachment = pStudioHdr->pAttachment(attachment);
	int iBone = pStudioHdr->GetAttachmentBone( attachment );
	MatrixGetColumn( pAttachment.local, 3, localAttach );

	matrix3x4_t inputToOutputBone;
	Studio_CalcBoneToBoneTransform( pStudioHdr, iBone, outputBoneIndex, inputToOutputBone );
	
	Vector out;
	VectorTransform( localAttach, inputToOutputBone, out );

	return out;
}

void QUA_Strider::StompHit( int followerBoneIndex )
{
	// CUANDO DA UNA OSTIA CON LA PATA...
	// MIRA ESTA FUNCIÓN

	// Que es la que mata definitivamente al bicho.

	
	CStudioHdr *pStudioHdr = GetModelPtr();
	physfollower_t *bone = m_BoneFollowerManager.GetBoneFollower( followerBoneIndex );
	if ( !bone ) {
		m_bStompAnim=false;
		return;
	}


	const char *pBoneNames[] = {"left skewer", "right skewer" };
	int nameIndex = followerBoneIndex == STRIDER_LEFT_LEG_FOLLOWER_INDEX ? 0 : 1;
	Vector localHit = GetAttachmentPositionInSpaceOfBoneSTR( pStudioHdr, pBoneNames[nameIndex], bone->boneIndex );
	IPhysicsObject *pLegPhys = bone->hFollower->VPhysicsGetObject();

	// now transform into the worldspace of the current position of the leg's physics
	matrix3x4_t legToWorld;
	pLegPhys->GetPositionMatrix( &legToWorld );
	Vector hitPosition;
	VectorTransform( localHit, legToWorld, hitPosition );

	
	CAI_BaseNPC *pNPC = elegido ? elegido->MyNPCPointer() : NULL;
	bool bIsValidTarget = pNPC && pNPC->GetModelPtr();
	/*if ( HasSpawnFlags( SF_CAN_STOMP_PLAYER ) )
	{*/
		bIsValidTarget = bIsValidTarget || ( elegido && elegido->IsPlayer() );
	/*}*/

	if ( !bIsValidTarget ) {
		m_bStompAnim=false;
		return;
	}
	Vector delta;
	VectorSubtract( elegido->GetAbsOrigin(), hitPosition, delta );
	
	// Esto debe indicarle que el enemigo se ha escapado...
	if ( delta.LengthSqr() > (STRIDER_STOMP_RANGE * STRIDER_STOMP_RANGE) ) {
		m_bStompAnim=false;
		return;
	}
	// DVS E3 HACK: Assume we stab our victim midway between their eyes and their center.
	Vector vecStabPos = ( elegido->WorldSpaceCenter() + elegido->EyePosition() ) * 0.5f;
	hitPosition = elegido->GetAbsOrigin();

	Vector footPosition;
	QAngle angles;
	GetAttachment( "left foot", footPosition, angles );

	CPASAttenuationFilter filter( this, "NPC_Strider.Skewer" );
	EmitSound( filter, 0, "NPC_Strider.Skewer", &hitPosition );

	CTakeDamageInfo damageInfo( this, this, 500, DMG_CRUSH );
	Vector forward;
	elegido->GetVectors( &forward, NULL, NULL );
	damageInfo.SetDamagePosition( hitPosition );
	damageInfo.SetDamageForce( -50 * 300 * forward );
	elegido->TakeDamage( damageInfo );

	if ( !pNPC || pNPC->IsAlive() ) {
		return;
		m_bStompAnim=false;
	}
	Vector vecBloodDelta = footPosition - vecStabPos;
	vecBloodDelta.z = 0; // effect looks better 
	VectorNormalize( vecBloodDelta );
	UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );
	UTIL_BloodSpray( vecStabPos + vecBloodDelta * 4, vecBloodDelta, BLOOD_COLOR_RED, 11, FX_BLOODSPRAY_DROPS );
	
	m_bStompAnim=false;
}
void QUA_Strider::Adelante(void) 
{
	if((acl==0)) {
		SetAbsVelocity(vec3_origin);
		return;
	}

	// Aqui esto no tira, abra que mirarlo bien despues;
		Vector dir;
		AngleVectors(this->GetAbsAngles(),&dir);
		//dir.z=0;
		// Despues examinaremos como hacer
		// el tema de sube baja
		Vector mins(-16,-16,-16), maxs(16,16,16);
		//engine->Con_NPrintf( 1, "Distancia al suelo: %f", Distancia.z );
		/*if (Distancia.z>492.0) {
			
			dir.z=-3;
		} else if (Distancia.z<488.0) {
			
			dir.z=3;
		} else {
			dir.z=0;
		}*/

		SetAbsVelocity(dir*250*(acl/1));
		Vector Dot=GetAbsOrigin();
		Vector Dat=GetAbsOrigin();
		float diff;
		MoveToGround( &Dot, this, mins, maxs );
		// Deberiamos examinar esto
		diff=Dat.z-Dot.z;
		// Aqui tenemos la diferencia entre los dos. Examinemos
		//engine->Con_NPrintf(25,"Diferencia: %f",diff);
		
		
		// El Strider siempre tiene que intentar llegar a GetMaxHeight().
		
		//Dot.z es el suelo. La diferencia debe de ser de 490 de normal

		// Entonces, si es asi, la diferencia debe ser interpolada
		// Examinamos pues la diferencia
		// ¿ Es 490?
		if (diff>485.0 && diff<495.0) {
			Dot.z+=GetMaxHeight();
		} else if (diff>495.0) {
			Dot.z+=diff-5;
		} else if (diff<485.0 && diff>400.0) {
			Dot.z+=diff+5;
		} else {
			Dot=Dat;
		}
		// Probemos suerte
		// Ahora hacemos un tracehull
		trace_t tr;
		UTIL_TraceHull(Dot,Dot,Vector(-38,-38,-150),Vector(60,38,38),MASK_SOLID,this,COLLISION_GROUP_VEHICLE,&tr);
		if (tr.fraction==1.0) {
			SetAbsOrigin(Dot);
		} 
		//Dot.z+=; // Siempre le dice que sea 490
		//Probemos a ver
}
void QUA_Strider::GiraIzquierda(void) 
{
	if(aclizq==0)
		return;
	// A ver si podemos poner un pose parameter.
	
	// Aqui esto no tira, abra que mirarlo bien despues;
		QAngle angles = GetAbsAngles();
		angles.y += 1*(aclizq/1);
		SetAbsAngles( angles );
	/*}*/
	
}
void QUA_Strider::ShootCannon(void)
{
			m_DisCanon=true;
			m_flCannonShot=gpGlobals->curtime+1.25f;
			//m_pMinigun->StopShootingForSeconds( this, m_pMinigun->GetTarget(), 5 );
			m_aimYaw=0;
			m_aimPitch=0;
			matrix3x4_t gunMatrix;
			GetAttachment( gm_CannonAttachment, gunMatrix );
			Vector vecShootPos;
			MatrixGetColumn( gunMatrix, 3, vecShootPos );
			EntityMessageBegin( this, true );
				WRITE_BYTE( STRIDER_MSG_STREAKS );
				WRITE_VEC3COORD( vecShootPos );
			MessageEnd();
			CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
			EmitSound( filter2, entindex(), "NPC_Strider.Charge" );
}
void QUA_Strider::FireCannonManually() 
{
	
	trace_t tr;
	matrix3x4_t gunMatrix;
	GetAttachment( gm_CannonAttachment, gunMatrix );

	Vector vecShootPos;
	MatrixGetColumn( gunMatrix, 3, vecShootPos );

	Vector vecShootDir;
	// Aqui definiremos la direccion de disparo, que es igual a la de el jugador
	// Seguramente disparará aunque no este alineado el cannon, a ver que puedo hacer antes.

	Vector vecMuzzlePos,vecEndPos;
	CBasePlayer *pPlayer = m_hPlayer;
	Vector	forward;
	pPlayer->EyePositionAndVectors(&vecMuzzlePos,&forward,NULL,NULL);
	vecEndPos= vecMuzzlePos +(forward * MAX_TRACE_LENGTH);
	trace_t	tro;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, MASK_OPAQUE_AND_NPCS , this, COLLISION_GROUP_NONE, &tro );
	m_blastHit = tro.endpos;
	m_blastHit += tro.plane.normal * 16;
	m_blastNormal = tro.plane.normal;
	// tell the client side effect to complete
	EntityMessageBegin( this, true );
		WRITE_BYTE( STRIDER_MSG_BIG_SHOT );
		WRITE_VEC3COORD( tro.endpos );
	MessageEnd();
	CPASAttenuationFilter filter2( this, "NPC_Strider.Shoot" );
	EmitSound( filter2, entindex(), "NPC_Strider.Shoot");
	SetContextThink( &QUA_Strider::CannonHitThink, gpGlobals->curtime + 0.2f, "CANNON_HIT" );
}
void QUA_Strider::GiraDerecha(void) 
{
	if(aclder==0)
		return;

	// Aqui esto no tira, abra que mirarlo bien despues;
		QAngle angles = GetAbsAngles();
		angles.y -= 1*(aclder/1);
		SetAbsAngles( angles );
	/*}*/
	
}
void QUA_Strider::HandleAnimEvent( animevent_t *pEvent )
{
	//engine->Con_NPrintf( 1, "Evento: %i", pEvent->event );	
	Vector footPosition;
	QAngle angles;

	switch( pEvent->event )
	{
	/*case STRIDER_AE_DIE:
		{
			Explode();
		}
		break;*/
	case STRIDER_AE_SHOOTCANNON:
		{
			//FireCannon();// Mi strider dispara esto MANUALMENTE
			FireCannonManually();
		}
		break;
	case STRIDER_AE_CANNONHIT:
		CreateConcussiveBlast( m_blastHit, m_blastNormal, this, 2.5 );
		break;

	//case STRIDER_AE_SHOOTMINIGUN:
	//	ShootMinigun( NULL, 0 );
	//	break;

	case STRIDER_AE_STOMPHITL:
		StompHit( STRIDER_LEFT_LEG_FOLLOWER_INDEX );
		break;
	case STRIDER_AE_STOMPHITR:
		StompHit( STRIDER_RIGHT_LEG_FOLLOWER_INDEX );
		break;
	/*case STRIDER_AE_FLICKL:
	case STRIDER_AE_FLICKR:
		{
			CBaseEntity *pRagdoll = m_hRagdoll;
			if ( pRagdoll )
			{
				CPASAttenuationFilter filter( pRagdoll, "NPC_Strider.RagdollDetach" );
				EmitSound( filter, pRagdoll->entindex(), "NPC_Strider.RagdollDetach" );
				DetachAttachedRagdoll( pRagdoll );
			}
			m_hRagdoll = NULL;
		}
		break;*/

	case STRIDER_AE_FOOTSTEP_LEFT:
	case STRIDER_AE_FOOTSTEP_LEFTM:
	case STRIDER_AE_FOOTSTEP_LEFTL:
		LeftFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_FOOTSTEP_RIGHT:
	case STRIDER_AE_FOOTSTEP_RIGHTM:
	case STRIDER_AE_FOOTSTEP_RIGHTL:
		RightFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_FOOTSTEP_BACK:
	case STRIDER_AE_FOOTSTEP_BACKM:
	case STRIDER_AE_FOOTSTEP_BACKL:
		BackFootHit( pEvent->eventtime );
		break;
	case STRIDER_AE_WHOOSH_LEFT:
		{
			GetAttachment( "left foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_RIGHT:
		{
			GetAttachment( "right foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_WHOOSH_BACK:
		{
			GetAttachment( "back foot", footPosition, angles );

			CPASAttenuationFilter filter( this, "NPC_Strider.Whoosh" );
			EmitSound( filter, 0, "NPC_Strider.Whoosh", &footPosition );
		}
		break;
	case STRIDER_AE_CREAK_LEFT:
	case STRIDER_AE_CREAK_BACK:
	case STRIDER_AE_CREAK_RIGHT:
		{
			EmitSound( "NPC_Strider.Creak" );
		}
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}
Vector QUA_Strider::LeftFootHit( float eventtime )
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "left foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );

	FootFX( footPosition );

	return footPosition;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector QUA_Strider::RightFootHit( float eventtime )
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "right foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );
	FootFX( footPosition );

	return footPosition;
}

void QUA_Strider::FootFX( const Vector &origin )
{
	trace_t tr;
	AI_TraceLine( origin, origin - Vector(0,0,100), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	float yaw = random->RandomInt(0,120);
	for ( int i = 0; i < 3; i++ )
	{
		Vector dir = UTIL_YawToVector( yaw + i*120 ) * 10;
		VectorNormalize( dir );
		dir.z = 0.25;
		VectorNormalize( dir );
		g_pEffects->Dust( tr.endpos, dir, 12, 50 );
	}
	UTIL_ScreenShake( tr.endpos, 4.0, 1.0, 0.5, 1000, SHAKE_START, false );
	
	if ( qua_strider_shake_ropes_radius.GetInt() )
	{
		CRopeKeyframe::ShakeRopes( tr.endpos, qua_strider_shake_ropes_radius.GetFloat(), qua_strider_shake_ropes_magnitude.GetFloat() );
	}

	//
	// My feet are scary things! NOTE: We might want to make danger sounds as the feet move
	// through the air. Then soldiers could run from the feet, which would look cool.
	//
	CSoundEnt::InsertSound( SOUND_DANGER|SOUND_CONTEXT_EXCLUDE_COMBINE, tr.endpos, 512, 1.0f, this );
}
//---------------------------------------------------------
//---------------------------------------------------------
Vector QUA_Strider::BackFootHit( float eventtime )
{
	Vector footPosition;
	QAngle angles;

	GetAttachment( "back foot", footPosition, angles );
	CPASAttenuationFilter filter( this, "NPC_Strider.Footstep" );
	EmitSound( filter, 0, "NPC_Strider.Footstep", &footPosition, eventtime );
	FootFX( footPosition );

	return footPosition;
}

void QUA_Strider::Use(CBaseEntity *pActivator,CBaseEntity *pCaller,USE_TYPE useType,float value) {
	
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
	
	if (IsStanding() || IsCrouching() || m_bStompAnim || m_DisCanon || acl!=0 || aclizq!=0 || aclder!=0)
		return;

	ResetUseKey( pPlayer );

	GetServerVehicle()->HandlePassengerEntry( pPlayer, (value>0) );
	EmitSound( "NPC_Strider.Alert" );
	
	
}
void QUA_Strider::SetActivity(Activity NewActivity) {

	if (m_Activity == NewActivity)
	{
		return;
	} else {
		int sequence;
		if (NewActivity == ACT_MELEE_ATTACK1) {
		sequence=this->LookupSequence("BighitL");
		} else if (NewActivity == ACT_IDLE){
		sequence=1;
		} else {
		sequence = SelectHeaviestSequence( NewActivity );
		}
		//engine->Con_NPrintf( 23, "Secuencia: %i",sequence );
		//int sequence = this->LookupSequence("Idle01");
		if ( sequence != ACTIVITY_NOT_AVAILABLE )
		{
		m_Activity=NewActivity;
		SetSequence( sequence );
		SetActivity(NewActivity);
		SetCycle( 0 );
		ResetSequenceInfo( );
		m_flPlaybackRate = 1.0;

		} else {
			return;
		}
	}
}
void QUA_Strider::SetHeight( float h )
{

	if ( h > GetMaxHeight() )
		h = GetMaxHeight();
	else if ( h < GetMinHeight() )
		h = GetMinHeight();
	SetPoseParameter(LookupPoseParameter("body_height"), h );
}

//---------------------------------------------------------
//---------------------------------------------------------
void QUA_Strider::SetIdealHeight( float h )
{
	if ( h > GetMaxHeight() )
		h = GetMaxHeight();
	else if ( h < GetMinHeight() )
		h = GetMinHeight();

	m_idealHeight = h;
}

bool QUA_Strider::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	// Let normal hitbox code handle rays
	if ( mask & CONTENTS_HITBOX )
	{
		return BaseClass::TestCollision( ray, mask, trace );
	}

	if ( IntersectRayWithBox( ray, WorldAlignMins() + GetAbsOrigin(), WorldAlignMaxs() + GetAbsOrigin(), DIST_EPSILON, &trace ) )
	{
		trace.hitbox = 0;
		trace.hitgroup = HITGROUP_HEAD;
		return true;
	}
	return false;
}

Vector QUA_Strider::CannonPosition()
{
	Vector position;
	// Currently just using the gun for the vertical component!
	GetAttachment( "biggun", position );
	position.x = GetAbsOrigin().x;
	position.y = GetAbsOrigin().y;

	return position;
}

static void MoveToGround( Vector *position, CBaseEntity *ignore, const Vector &mins, const Vector &maxs )
{
	trace_t tr;
	// Find point on floor where enemy would stand at chasePosition
	Vector floor = *position;
	floor.z -= 1024;
	AI_TraceHull( *position, floor, mins, maxs, MASK_OPAQUE, ignore, COLLISION_GROUP_NONE, &tr );
	
	// Hemos quitado el fraction para comprobar, pero hay que ponerlo por si acaso
	
	if ( tr.fraction < 1 )
	{
		position->z = tr.endpos.z;
	}
}
Vector QUA_Strider::GetDriverEyes()
{
	matrix3x4_t gunMatrix;
	GetAttachment( gm_DriverEyes, gunMatrix );

	Vector vecShootPos;
	MatrixGetColumn( gunMatrix, 3, vecShootPos );

	return vecShootPos;
}
bool QUA_Strider::IsPlayerInStrider(void) {
	return m_bInStrider;
}
void QUA_Strider::ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread)
{
	if ( pTarget )
	{
		Vector muzzlePos;
		QAngle muzzleAng;

		GetAttachment( "Minigun", muzzlePos, muzzleAng );
		
		Vector vecShootDir = *pTarget - muzzlePos;
		VectorNormalize( vecShootDir );

		// exactly on target w/tracer

		FireBulletsInfo_t info;
		info.m_iShots = 1;
		info.m_vecSrc = muzzlePos;
		info.m_vecDirShooting = vecShootDir;
		info.m_vecSpread = vecSpread;
		info.m_pAttacker =	(CBaseEntity *) m_hPlayer;
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType =  m_miniGunAmmo;
		info.m_flDamage = 40;
		info.m_iPlayerDamage= 40;
		info.m_iTracerFreq = 1;
		FireBullets( info );
		
		EntityMessageBegin( this, true );
				WRITE_BYTE( STRIDER_MSG_MACHINEGUN );
				WRITE_VEC3COORD(muzzlePos);
				WRITE_VEC3COORD(vecShootDir);
				WRITE_VEC3COORD(vecSpread);
				WRITE_BYTE( m_miniGunAmmo );
		MessageEnd();
//		UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
		//g_pEffects->MuzzleFlash( muzzlePos, muzzleAng, random->RandomFloat( 2.0f, 4.0f ) , MUZZLEFLASH_TYPE_STRIDER );
		DoMuzzleFlash();
		m_iAmmoCount--; // Descontamos ametralladora.
		EmitSound( "NPC_Strider.FireMinigun" );
	}
}

void QUA_Strider::DoMuzzleFlash( void )
{
	BaseClass::DoMuzzleFlash();
	
	CEffectData data;
	Vector muzzlePos;
	QAngle muzzleAng;
	GetAttachment( "Minigun", muzzlePos, muzzleAng );
	data.m_nAttachmentIndex = LookupAttachment( "MiniGun" );
	data.m_nEntIndex = entindex();
	data.m_vOrigin=muzzlePos;
	DispatchEffect( "QUA_StriderMuzzleFlash", data );
}
void QUA_Strider::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void QUA_Strider::DoImpactEffect( trace_t &tr, int nDamageType )
{
	BaseClass::DoImpactEffect( tr, nDamageType );

	// Add a halo
	CBroadcastRecipientFilter filter;
	te->BeamRingPoint( filter, 0.0, 
		tr.endpos,							//origin
		0,									//start radius
		64,									//end radius
		s_QUA_iImpactEffectTexture,				//texture
		0,									//halo index
		0,									//start frame
		0,									//framerate
		0.2,								//life
		10,									//width
		0,									//spread
		0,									//amplitude
		255,								//r
		255,								//g
		255,								//b
		50,									//a
		0,									//speed
		FBEAM_FADEOUT
		);

	g_pEffects->EnergySplash( tr.endpos, tr.plane.normal );
	
	// Punch the effect through?
	if( tr.m_pEnt && !tr.m_pEnt->MyNPCPointer() )
	{
		Vector vecDir = tr.endpos - tr.startpos;
		VectorNormalize( vecDir );

		trace_t retrace;

		Vector vecReTrace = tr.endpos + vecDir * 12;

		if( UTIL_PointContents( vecReTrace ) == CONTENTS_EMPTY )
		{
			AI_TraceLine( vecReTrace, vecReTrace - vecDir * 24, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &retrace );

			BaseClass::DoImpactEffect( retrace, nDamageType );
		}
	}
}
void QUA_Strider::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

// =================================================
// All Driveable Thinks
// ================================================
void QUA_Strider::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	//m_pServerVehicle.SetVehicle( this );
}
void QUA_Strider::DestroyServerVehicle()
{
	/*if ( m_pServerVehicle )
	{
		delete m_pServerVehicle;
		m_pServerVehicle = NULL;
	}*/
}
Class_T	QUA_Strider::ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification )
{ 
	return CLASS_PLAYER;	
}

void QUA_Strider::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
//	BaseClass::DriveVehicle( flFrameTime, ucmd, iButtonsDown, iButtonsReleased );
}
//void QUA_Strider::ItemPostFrame( CBasePlayer *player )
//{
//}
void QUA_Strider::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If the player's entering/exiting the vehicle, prevent movement
	//if ( !m_bEnterAnimOn && !m_bExitAnimOn )
	//{
		DriveStrider( ucmd->buttons, player->m_afButtonPressed );
	//}

	//// Run the crane's movement
	//RunCraneMovement( gpGlobals->frametime );
}
void QUA_Strider::DriveStrider( int iDriverButtons, int iButtonsPressed, float flNPCSteering )
{
 if (!IsCrouching() && !IsStanding() && !m_bStompAnim && !m_DisCanon) {
	if (iDriverButtons & IN_FORWARD) {
		if (acl<1) {
		acl+=0.02;
		}
	} else {
		if(acl>0) {
		acl-=0.02;
		} else {
		acl=0;
		}
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
				Dispara();
				m_flNextShootingTime=gpGlobals->curtime+0.1;
			}
		} 
	if ( iDriverButtons & IN_ATTACK2 )
		{
			if (m_flNextCannonTime<=gpGlobals->curtime && m_iCannonCount==100 && acl==0 && aclizq==0 && aclder==0) {
				ShootCannon();
				m_flNextCannonTime=gpGlobals->curtime+5.0;
			}
		}
	if ( iDriverButtons & IN_DUCK )
		{	
			if (acl==0) {
				if (!IsCrouching() && !IsStanding()) {
					if (IsInCrouchedPosture()) {
					SetActivity(ACT_STAND);
					m_bStanding=true;
					} else {
					SetActivity(ACT_CROUCH);
					m_bCrouching=true;
					}
				}
			}
		}
	if ( iDriverButtons & IN_JUMP )
		{
			if (acl==0 && IsInStandPosture()) {
				m_bStompAnim=true;
				SetActivity(ACT_MELEE_ATTACK1);
				m_vecHitPos = CalculateStompHitByPlayer();
			}
		}
 }
	
	
}

CBaseEntity *QUA_Strider::GetDriver( void ) 
{ 
	return m_hPlayer; 
}

Vector QUA_Strider::CalculateStompHitByPlayer(void)
{
Vector skewerPosition, footPosition;
	QAngle angles;
	GetAttachment( "left skewer", skewerPosition, angles );
	GetAttachment( "left foot", footPosition, angles );
	Vector vecStabPos =DondeApuntaPlayer();
	
	// Aqui ya calculamos directamente quien es el Elegido

	Vector vecMuzzlePos,vecEndPos;
	// Solo devuelve al jugador local. Cuando sea multiplayer, habra que cambiarlo
	CBasePlayer *pPlayer = m_hPlayer;
	// Recoge la posicion de los ojos del jugador
	// Que nos diga que es el vector Anchor
	Vector	pforward;
	pPlayer->EyePositionAndVectors(&vecMuzzlePos,&pforward,NULL,NULL);
	vecEndPos= vecMuzzlePos +(pforward * MAX_TRACE_LENGTH);
	trace_t	tr;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, MASK_OPAQUE_AND_NPCS , this, COLLISION_GROUP_NONE, &tr );
	
	
	elegido = CBaseEntity::Instance(tr.GetEntityIndex());  // Bien... esto es lo que caga to.
	

	return vecStabPos - skewerPosition + footPosition;
	}
void QUA_Strider::ItemPostFrame( CBasePlayer *player )
{
}
bool QUA_Strider::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if (IsCrouching() || IsStanding() || m_bStompAnim || m_DisCanon )
		return false;
	return true;
}
bool QUA_Strider::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or rotating
	// Adrian: Check also if I'm currently jumping in or out.
	if ((IsCrouching() || IsStanding() || m_bStompAnim || m_DisCanon || acl!=0 || aclizq!=0 || aclder!=0) && !force_exit )
		return false;
	return true;
}
void QUA_Strider::EnterVehicle(CBaseCombatCharacter *pPlayer)
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
void QUA_Strider::ExitVehicle( int iRole )
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
//========================================================================================================================================
// QUA_STRIDER SERVER VEHICLE
//========================================================================================================================================

QUA_Strider *CQUAServerVehicle::GetStrider( void )
{
	return (QUA_Strider*)GetDrivableVehicle();
}
Vector CQUAServerVehicle::GetSmoothedVelocity( void )
{
	Vector velocity=GetStrider()->GetAbsVelocity();
	return velocity;
}
void CQUAServerVehicle::NPC_SetDriver( CNPC_VehicleDriver *pDriver ) {
	// nada que hacer aqui
}
void CQUAServerVehicle::NPC_DriveVehicle( void ) {
	// nada que hacer aqui
}
//-----------------------------------------------------------------------------
// Purpose: TODO: Review what happens here
//-----------------------------------------------------------------------------
void CQUAServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	//FixMe, wtf?
	#ifndef DEBUG
	Assert( nRole == VEHICLE_DRIVER );
	#endif
	CBasePlayer *pPlayer = ToBasePlayer( GetDrivableVehicle()->GetDriver() );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.

	float flPitchFactor = 1.0;
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle no;
	QAngle vehicleEyeAngles;
	GetStrider()->GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, no);
	vehicleEyeAngles=GetStrider()->GetAbsAngles();
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );
	Vector up,forward;
	GetStrider()->GetVectors(&forward,NULL,&up);
	vehicleEyeOrigin+=(forward*120)+(up*-10);
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

void CQUAServerVehicle::HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone )
{
	// Check to see if this vehicle can be controlled or if it's locked
	if ( GetDrivableVehicle()->CanEnterVehicle(pPlayer) )
	{
		pPlayer->GetInVehicle( this, VEHICLE_ROLE_DRIVER);
	}
}

// ------------------------------------------
// Strider Minigun
// ------------------------

BEGIN_DATADESC_NO_BASE( CQUAStriderMinigun )

	DEFINE_FIELD( m_enable,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_minigunState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nextBulletTime,		FIELD_TIME ),
	DEFINE_FIELD( m_burstTime,			FIELD_TIME ),
	DEFINE_FIELD( m_nextTwitchTime,		FIELD_TIME ),
	DEFINE_FIELD( m_randomState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bWarnedAI,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_shootDuration,		FIELD_TIME ),
	DEFINE_FIELD( m_vecAnchor,			FIELD_VECTOR ),
	DEFINE_FIELD( m_bOverrideEnemy,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecLastTargetPos,	FIELD_VECTOR ),
	DEFINE_FIELD( m_iOnTargetShots,		FIELD_INTEGER ),

	// Silence, Classcheck!
	// DEFINE_FIELD( m_yaw, StriderMinigunAnimController_t ),
	// DEFINE_FIELD( m_pitch, StriderMinigunAnimController_t ),
	
	DEFINE_FIELD( m_yaw.current,		FIELD_FLOAT ),
	DEFINE_FIELD( m_yaw.target,			FIELD_FLOAT ),
	DEFINE_FIELD( m_yaw.rate,			FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.current,		FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.target,		FIELD_FLOAT ),
	DEFINE_FIELD( m_pitch.rate,			FIELD_FLOAT ),


END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::Init()
{
	m_enable = true;
	m_nextTwitchTime = gpGlobals->curtime;
	m_randomState = 0;
	m_yaw.current = m_yaw.target = 0;
	m_pitch.current = m_pitch.target = 0;
	m_yaw.rate = 360;
	m_pitch.rate = 180;

	SetState( MINIGUN_OFF );
	m_burstTime = gpGlobals->curtime;
	m_nextBulletTime = FLT_MAX;
	m_vecAnchor = vec3_invalid;
	m_shootDuration = STRIDER_DEFAULT_SHOOT_DURATION;
	m_bOverrideEnemy = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CQUAStriderMinigun::ShouldFindTarget( IQUAMinigunHost *pHost )
{
	ASSERT( pHost != NULL );

	if( !GetTarget() )
	{
		// No target. Find one.
		return true;
	}

	if( m_bOverrideEnemy )
	{
		return false;
	}

	if( pHost->GetEntity()->GetEnemy() )
	{
		return GetTarget() != pHost->GetEntity()->GetEnemy();
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
float CQUAStriderMinigun::GetAimError()
{
	return fabs(m_yaw.target-m_yaw.current) + fabs(m_pitch.target-m_pitch.current);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::AimAtPoint( IQUAMinigunHost *pHost, const Vector &vecPoint, bool bSnap )
{
	matrix3x4_t gunMatrix;
	CBaseAnimating *pOwner = pHost->GetEntity();

	int mingunAttachment = pOwner->LookupAttachment( "minigunbase" );
	pOwner->GetAttachment( mingunAttachment, gunMatrix );

	Vector forward, pos;
	MatrixGetColumn( gunMatrix, 0, forward );
	MatrixGetColumn( gunMatrix, 3, pos );

	// transform the point into gun space
	Vector localPointPosition;
	VectorITransform( vecPoint, gunMatrix, localPointPosition );
	
	// do a look at in gun space (essentially a delta-lookat)
	QAngle localPointAngles;
	VectorAngles( localPointPosition, localPointAngles );

	// convert to +/- 180 degrees
	float pdiff, ydiff;
	pdiff = UTIL_AngleDiff( localPointAngles.x, 0 );
	ydiff = UTIL_AngleDiff( localPointAngles.y, 0 );

	m_pitch.target += 0.5 * pdiff;
	m_yaw.target -= 0.5 * ydiff;

	m_pitch.target = max( MINIGUN_MIN_PITCH, m_pitch.target );
	m_pitch.target = min( MINIGUN_MAX_PITCH, m_pitch.target );
	m_yaw.target = max( MINIGUN_MIN_YAW, m_yaw.target );
	m_yaw.target = min( MINIGUN_MAX_YAW, m_yaw.target );

	pOwner->SetPoseParameter( "miniGunYaw", m_yaw.target );
	pOwner->SetPoseParameter( "miniGunPitch", m_pitch.target );

	/*Vector	vecMuzzle, vecMuzzleDir;
	QAngle	vecMuzzleAng;

	pOwner->GetAttachment( "MiniGun", vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );

	trace_t	tr;
	UTIL_TraceLine( vecMuzzle, vecMuzzle + (vecMuzzleDir * MAX_TRACE_LENGTH), MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );*/

		// see if we hit something, if so, adjust endPos to hit location
	/*if ( tr.fraction < 1.0 )
	{
		pOwner->m_vecGunCrosshair = vecMuzzle + ( vecMuzzleDir * MAX_TRACE_LENGTH * tr.fraction );
	}*/
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::AimAtTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, bool bSnap )
{
	if ( pTarget && !(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
	{
		Vector vecTargetPos = pTarget->BodyTarget( pHost->GetEntity()->EyePosition() );
		AimAtPoint( pHost, vecTargetPos, bSnap );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::ShootAtTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float shootTime )
{
	if ( !pTarget && !(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) )
		return;

	variant_t emptyVariant;
	pTarget->AcceptInput( "InputTargeted", pHost->GetEntity(), pHost->GetEntity(), emptyVariant, 0 );
	Enable( NULL, true );
	if ( shootTime <= 0 )
	{
		shootTime = random->RandomFloat( 4, 8 );
	}
	SetTarget( pHost, pTarget, true );

	StartShooting( pHost, pTarget, shootTime );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::StartShooting( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float duration )
{
	bool bHasSetAnchor = false;

	SetState( MINIGUN_SHOOTING );
	
	m_nextBulletTime = gpGlobals->curtime;
	m_burstTime = gpGlobals->curtime + duration;

	m_shootDuration = duration;

	// don't twitch while shooting
	m_nextTwitchTime = FLT_MAX;

	if( pTarget->IsPlayer() )
	{
		// Don't shoot a player in the back if they aren't looking. 
		// Give them a chance to see they're being fired at.

		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(pTarget);

		if( !pPlayer->FInViewCone( pHost->GetEntity() ) )
		{
			// Player doesn't see me. Try to start shooting so that they can see
			// the bullet impacts.
			m_vecAnchor = pPlayer->EyePosition();

			Vector vecPlayerLook;
			Vector vecToPlayer;

			// Check in 2D.
			vecPlayerLook = pPlayer->EyeDirection3D();
			vecPlayerLook.z = 0.0;
			VectorNormalize( vecPlayerLook );

			vecToPlayer = pPlayer->EyePosition() - pHost->GetEntity()->EyePosition();
			vecToPlayer.z = 0.0;
			VectorNormalize( vecToPlayer );

			float flDot = DotProduct( vecToPlayer, vecPlayerLook );

			if( flDot < 0.95 )
			{
				// If the player is looking sufficiently to a side, start 30 feet out in that direction.
				m_vecAnchor += pPlayer->EyeDirection3D() * 320;
				bHasSetAnchor = true;
			}
			else
			{
				// Start over their head, cause firing the direction they are looking will drill them.
				// in the back!
				m_vecAnchor += Vector( 0, 0, random->RandomFloat( 160, 240 ) );

				// Move it to one side of the other randomly, just to get it off center.
				Vector right;
				pPlayer->GetVectors( NULL, &right, NULL );
				m_vecAnchor += right * random->RandomFloat( -100, 100 );
				bHasSetAnchor = true;
			}
		}
	}

	if( !bHasSetAnchor )
	{
		m_vecAnchor = pTarget->WorldSpaceCenter();

		Vector right;
		pTarget->GetVectors( NULL, &right, NULL );

		// Start 5 or 10 feet off target.
		Vector offset = right * random->RandomFloat( 60, 120 );
		
		// Flip a coin to decide left or right.
		if( random->RandomInt( 0, 1 ) == 0 )
		{
			offset *= -1;
		}

		m_vecAnchor += offset;

		// Start below them, too.
		m_vecAnchor.z -= random->RandomFloat( 80, 200 );
	}

	pHost->OnMinigunStartShooting( pTarget );
}

//---------------------------------------------------------
// Fixes up the math for stitching.
//---------------------------------------------------------
void CQUAStriderMinigun::ExtendShooting( float timeExtend )
{
	m_burstTime = gpGlobals->curtime + timeExtend;
	m_shootDuration = timeExtend;
	m_bWarnedAI = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::SetShootDuration( float duration )
{
}

//---------------------------------------------------------
// Is the gun turned as far as it can go?
//---------------------------------------------------------
bool CQUAStriderMinigun::IsPegged( int dir )
{
	bool up, down, left, right, any;

	up = down = left = right = any = false;

	if( m_yaw.current >= 89.0 )
		any = right = true;

	if( m_yaw.current <= -89.0 )
		any = left = true;

	if( m_pitch.current >= 44.0 )
		any = down = true;

	if( m_pitch.current <= -44.0 )
		any = up = true;

	switch( dir )
	{
	case MINIGUN_PEGGED_UP:
		return up;

	case MINIGUN_PEGGED_DOWN:
		return down;

	case MINIGUN_PEGGED_LEFT:
		return left;

	case MINIGUN_PEGGED_RIGHT:
		return right;

	default:
		return (any && !up);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::StopShootingForSeconds( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float duration )
{
	if ( IsShooting() )
	{
		SetState( MINIGUN_OFF );
	}

	m_burstTime = gpGlobals->curtime + duration;
	m_nextBulletTime = FLT_MAX;

	ClearOnTarget();
	m_nextTwitchTime = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	pHost->OnMinigunStopShooting( pTarget );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::SetState( int newState )
{
	m_minigunState = newState;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::SetTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, bool bOverrideEnemy )
{
	m_hTarget = pTarget;

	if( pTarget )
	{
		// New target, we haven't scared this guy yet!
		m_bWarnedAI = false;

		if( m_vecAnchor == vec3_invalid )
		{
			Vector right;
			pHost->GetEntity()->GetVectors( NULL, &right, NULL );

			m_vecAnchor = pTarget->GetAbsOrigin() - Vector( 0, 0, 256 );
			m_vecAnchor += right * random->RandomFloat( -60.0f, 60.0f );
		}
	}

	ClearOnTarget();

	m_bOverrideEnemy = bOverrideEnemy;
}

//---------------------------------------------------------
// The strider minigun can track and fire at targets in a fairly
// large arc. It looks unnatural for a Strider to acquire a target
// off to one side and begin firing at it, so we don't let the
// minigun BEGIN shooting at a target unless the target is fairly
// well in front of the Strider. Once firing, the gun is allowed
// to track the target anywhere for the duration of that burst of
// minigun fire. This is tuned by eye. (sjb)
//---------------------------------------------------------
bool CQUAStriderMinigun::CanStartShooting( IQUAMinigunHost *pHost, CBaseEntity *pTargetEnt )
{
	//if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
	//	return false;

	//if( !pTargetEnt )
	//	return false;
	//
	//if( gpGlobals->curtime < m_burstTime )
	//	return false;

	//QUA_Strider *pStrider = dynamic_cast<QUA_Strider *>(pHost->GetEntity() );
	//if ( pTargetEnt->IsPlayer() && pStrider->HasPass() )
	//	return false;

	//if( !m_bOverrideEnemy )
	//{
	//	if( pTargetEnt != pHost->GetEntity()->GetEnemy() )
	//	{
	//		return false;
	//	}

	//	// If the strider can't see the enemy, this may be because the enemy is
	//	// hiding behind something breakable. If the weapon has LOS, fire away.
	//	//if( !pHost->GetEntity()->HasCondition( COND_SEE_ENEMY )  )
	//	//{
	//	//	Assert( pStrider != NULL );

	//	//	/*if( !pStrider->WeaponLOSCondition( pStrider->GetAdjustedOrigin(), pTargetEnt->WorldSpaceCenter(), false ) )
	//	//	{
	//	//		return false;
	//	//	}*/
	//	//}
	//}
	//
	//Vector los = ( pTargetEnt->WorldSpaceCenter() - pHost->GetEntity()->EyePosition() );

	//// Following code stolen from FVisible. This check done in 2d intentionally.
	//los.z = 0;
	//VectorNormalize( los );

	//Vector facingDir = pHost->GetEntity()->EyeDirection2D( );
	//float flDot = DotProduct( los, facingDir );

	//// Too far to a side.
	//if ( flDot <= .707 )
	//	return false;

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::Enable( IQUAMinigunHost *pHost, bool enable )
{
	m_enable = enable;
	if ( !m_enable )
	{
		m_yaw.current = m_yaw.target = 0;
		m_pitch.current = m_pitch.target = 0;
		if ( pHost )
		{
			// KENB pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CQUAStriderMinigun::Think( IQUAMinigunHost *pHost, float dt )
{
	//if ( !m_enable )
	//	return;

	//if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
	//	return;

	//if( ShouldFindTarget( pHost ) )
	//{
	//	CBaseEntity *pOldTarget = GetTarget();

	//	// Take my host's enemy.
	//	SetTarget( pHost, pHost->GetEntity()->GetEnemy() );
	//	
	//	if( IsShooting() )
	//	{
	//		// Changing targets hot! 
	//		if( pOldTarget )
	//		{
	//			m_vecAnchor = pOldTarget->WorldSpaceCenter();
	//		}

	//		ExtendShooting( STRIDER_SUBSEQUENT_TARGET_DURATION + random->RandomFloat( 0, 0.5 ) );
	//	}
	//	
	//	pHost->NewTarget();
	//}

	//if ( !GetTarget() && m_nextTwitchTime <= gpGlobals->curtime )
	//{
	//	// invert one and randomize the other.
	//	// This has the effect of making the gun cross the field of
	//	// view more often - like he's looking around
	//	m_randomState = !m_randomState;
	//	if ( m_randomState )
	//	{
	//		m_yaw.Random( MINIGUN_MIN_YAW, MINIGUN_MAX_YAW, 360, 720 );
	//		m_pitch.target = -m_pitch.target;
	//	}
	//	else
	//	{
	//		m_pitch.Random( MINIGUN_MIN_PITCH, MINIGUN_MAX_PITCH, 270, 360 );
	//		m_yaw.target = -m_yaw.target;
	//	}
	//	m_nextTwitchTime = gpGlobals->curtime + random->RandomFloat( 0.3, 2 );
	//}

	//CBaseEntity *pTargetEnt = m_hTarget.Get();

	//if ( pTargetEnt )
	//{
	//	pHost->GetEntity()->InvalidateBoneCache();
	//	AimAtTarget( pHost, pTargetEnt );
	//}

	// Update the minigun's pose parameters using approaching. 
	/*m_yaw.Update( dt );
	m_pitch.Update( dt );

	pHost->UpdateMinigunControls( m_yaw.current, m_pitch.current );*/

	//// Start or stop shooting.
	//if( IsShooting() )
	//{
	//	if( gpGlobals->curtime > m_burstTime || !pTargetEnt )
	//	{
	//		// Time to stop firing.
	//		if( m_bOverrideEnemy )
	//		{
	//			// Get rid of this target.
	//			SetTarget( pHost, NULL );
	//		}

	//		StopShootingForSeconds( pHost, pTargetEnt, STRIDER_SHOOT_DOWNTIME );
	//	}
	//}
	//else
	//{
	//	if( CanStartShooting( pHost, pTargetEnt ) )
	//	{
	//		StartShooting( pHost, pTargetEnt, STRIDER_DEFAULT_SHOOT_DURATION + random->RandomFloat( 0, STRIDER_SHOOT_VARIATION ) );
	//	}
	//}

	//// Fire the next bullet!
	//if ( m_nextBulletTime <= gpGlobals->curtime && !IsPegged() )
	//{
	//	if( pTargetEnt && pTargetEnt == pHost->GetEntity()->GetEnemy() )
	//	{
	//		// Shooting at the Strider's enemy. Strafe to target!
	//		float flRemainingShootTime = m_burstTime - gpGlobals->curtime;

	//		// Skim a little time off of the factor, leave .5 seconds of on-target
	//		// time. This guarantees that the minigun will strike the target a few times.
	//		float flFactor = (flRemainingShootTime - 0.5) / m_shootDuration;

	//		flFactor = max( 0.0f, flFactor );

	//		Vector vecTarget = pTargetEnt->BodyTarget( assert_cast<QUA_Strider *>(pHost->GetEntity())->GetAdjustedOrigin());

	//		Vector vecLine = m_vecAnchor - vecTarget;
	//		
	//		float flDist = VectorNormalize( vecLine );

	//		vecTarget += vecLine * flDist * flFactor;

	//		if( flFactor == 0.0 )
	//		{
	//			m_vecAnchor = vecTarget;
	//			RecordShotOnTarget();
	//		}

	//		if ( GetTarget() )
	//		{
	//			pHost->ShootMinigun( &vecTarget, GetAimError(), vec3_origin );

	//			if( flFactor <= 0.5 && !m_bWarnedAI )
	//			{
	//				m_bWarnedAI = true;

	//				CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_REACT_TO_SOURCE, pTargetEnt->EarPosition() + Vector( 0, 0, 1 ), 120, max( 1.0, flRemainingShootTime ), pHost->GetEntity() );
	//			}
	//		}
	//	}
	//	else
	//	{
	//		const Vector *pTargetPoint = pTargetEnt ? &pTargetEnt->GetAbsOrigin() : NULL;
	//		pHost->ShootMinigun( pTargetPoint, GetAimError() );
	//	}

	//	m_nextBulletTime = gpGlobals->curtime + 0.2;
	//}
}

class CStriderRagdoll : public CBaseAnimating
{
public:
	DECLARE_CLASS( CStriderRagdoll, CBaseAnimating );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	CNetworkHandle( CBaseEntity, m_hStrider );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar(float , m_fAltura);
};

LINK_ENTITY_TO_CLASS( Strider_ragdoll, CStriderRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CStriderRagdoll, DT_StriderRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hStrider ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) ),
	SendPropFloat ( SENDINFO(m_fAltura)),
END_SEND_TABLE()

void QUA_Strider::CreateRagdollEntity( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// If we already have a ragdoll, don't make another one.
	CStriderRagdoll *pRagdoll = dynamic_cast< CStriderRagdoll* >( m_hRagdoll.Get() );
	
	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CStriderRagdoll* >( CreateEntityByName( "Strider_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hStrider = this->GetBaseEntity();
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecTotalBulletForce;
		pRagdoll->m_fAltura = this->GetHeight();
		//engine->Con_NPrintf(25,"Altura %f",GetHeight());
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
		pRagdoll->SetPoseParameter(LookupPoseParameter("body_height"),this->GetHeight() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}
bool QUA_Strider::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}
#endif
