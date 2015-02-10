//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else
	#include "grenade_ar2.h"
	#include "hl2mp_player.h"
	#include "basegrenade_shared.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponTroll C_WeaponTroll
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SMG1_GRENADE_DAMAGE 100.0f
#define SMG1_GRENADE_RADIUS 250.0f

class CWeaponTroll : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS( CWeaponTroll, CHL2MPMachineGun );

	CWeaponTroll();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );

	bool	CanHealPlayer( void );
	bool	HealPlayer( void );
	void	Precache( void );
	void	AddViewKick( void );
	void	SecondaryAttack( void );
	int		m_bDropped;
	int		m_bFired;
	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );

	float	GetFireRate( void ) { return 0.075f; }	// 13.3hz
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();


	

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();

#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );
	void Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary );
#endif //SecobMod__Enable_Fixed_Multiplayer_AI
#endif

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
	
private:
	CWeaponTroll( const CWeaponTroll & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTroll, DT_WeaponTroll )

BEGIN_NETWORK_TABLE( CWeaponTroll, DT_WeaponTroll )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponTroll )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_troll, CWeaponTroll );
PRECACHE_WEAPON_REGISTER(weapon_troll);

#ifndef CLIENT_DLL

acttable_t	CWeaponTroll::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SMG1,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SMG1,				false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_SMG1,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SMG1,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_SMG1,					false },
//SecobMod__Information These are the old act lines, required for our player models animations.
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SMG1,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SMG1,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SMG1,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SMG1,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SMG1,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SMG1,					false },
#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SMG1,					false },

	// HL2
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims


// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims



	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims



	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities



	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
#endif //SecobMod__Enable_Fixed_Multiplayer_AI
	};

IMPLEMENT_ACTTABLE(CWeaponTroll);
#endif //not clientdll.

//=========================================================//
// Constructor												//
//=========================================================//
CWeaponTroll::CWeaponTroll( )
{
	#ifdef Weighted_Weaponry
	Troll_Weight = 0;
	#endif
	m_bDropped = 0; // This is to make sure the Medic drops it one at a time
	m_fMinRange1		= 0;// No minimum range. 
	m_fMaxRange1		= 1400;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponTroll::ItemPostFrame( void )
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer( GetOwner() ); // Get owner of fun
	BaseClass::ItemPostFrame();

	if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (pOwner->m_nButtons & IN_RELOAD)))
   {
      m_bDropped = 0;
   }

}

#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	/*// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime( SINGLE_NPC );

	CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );
	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0 );

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;*/
	if (!m_bDropped) { // If the health vial hasn't been dropped, continue
#ifndef CLIENT_DLL
      CBasePlayer *pPlayer = ToBasePlayer( GetOwner() ); // Grab the wielder
 
      Vector   vecEye = pPlayer->EyePosition(); // Get the eye vector of the wielder
      Vector   vForward, vRight; // Create 2 new vectors
 
      pPlayer->EyeVectors( &vForward, &vRight, NULL );  // Getting direction forward
      Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f; // Finding source of spawning
      trace_t tr; // Gets the trace
 
      UTIL_TraceHull( vecEye, vecSrc, -Vector(6,6,6), Vector(6,6,6),
         pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr ); // Checks if anything in front of it
 
      if ( tr.DidHit() ) // If hit
      {
         vecSrc = tr.endpos; // Drop at location
      }
 
   //   CheckThrowPosition( pPlayer, vecEye, vecSrc );
   //   vForward[0] += 0.1f;
      vForward[2] += 0.1f; // Something else
 
      Vector vecThrow;
      AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow ); // Get Vector for Impulse on Health Vial
      // pPlayer->GetVelocity( &vecThrow, NULL );
      VectorScale( vecThrow, 1000.0f, vecThrow ); // Scale vial down
      // vecThrow += vForward * 1200;
 
      CBaseEntity *pVial = NULL; // Null
      pVial = CBaseEntity::Create( "item_healthvial", pPlayer->Weapon_ShootPosition(), QAngle(80,60,0), pPlayer ); // Creates
      pVial->AddSpawnFlags(SF_NORESPAWN); // Doesn't allow respawning
      if (!pVial) // Has to have to be created to throw it
      {
         DevMsg("unable to create item\n");
      }else // Works
      {
         DevMsg(2, "Starting item throw (Server)\n");
 
         IPhysicsObject *pPhysicsObject = pVial->VPhysicsGetObject(); // Get the physic object
         if ( pPhysicsObject )
         {
            DevMsg(2, "Throwing item (Server)\n");
            pPhysicsObject->SetVelocity( &vecThrow, NULL );// Throw vial
 
         }
      }
      m_bDropped = 1; // Change flag
#endif
   } else {
      DevMsg(2, "Already dropped one. Release the button. (Server)\n"); // Let go of button
   }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SMG1:
		{
			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;

			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = pOperator->Weapon_ShootPosition();
			}

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			FireNPCPrimaryAttack( pOperator, vecShootOrigin, vecShootDir );
		}
		break;

		/*//FIXME: Re-enable
		case EVENT_WEAPON_AR2_GRENADE:
		{
		CAI_BaseNPC *npc = pOperator->MyNPCPointer();

		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetShootEnemyDir( vecShootOrigin );

		Vector vecThrow = m_vecTossVelocity;

		CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecShootOrigin, vec3_angle, npc );
		pGrenade->SetAbsVelocity( vecThrow );
		pGrenade->SetLocalAngularVelocity( QAngle( 0, 400, 0 ) );
		pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY ); 
		pGrenade->m_hOwner			= npc;
		pGrenade->m_pMyWeaponAR2	= this;
		pGrenade->SetDamage(sk_npc_dmg_ar2_grenade.GetFloat());

		// FIXME: arrgg ,this is hard coded into the weapon???
		m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.

		m_iClip2--;
		}
		break;
		*/

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif
#endif //SecobMod__Enable_Fixed_Multiplayer_AI

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther("grenade_ar2");
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponTroll::Equip( CBaseCombatCharacter *pOwner )
{
	m_fMaxRange1 = 1400;

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponTroll::GetPrimaryAttackActivity( void )
{
	if ( m_nShotsFired < 2 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nShotsFired < 3 )
		return ACT_VM_RECOIL1;
	
	if ( m_nShotsFired < 4 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponTroll::Reload( void )
{
	bool fRet;
	float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		// Undo whatever the reload process has done to our secondary
		// attack timer. We allow you to interrupt reloading to fire
		// a grenade.
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = fCacheTime;

		WeaponSound( RELOAD );
//		ToHL2MPPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

bool CWeaponTroll::CanHealPlayer( void ) // Checks boolean
{
   if (!m_bFired){ // If it hasn't been fired,
      CHL2MP_Player *pOwner = ToHL2MPPlayer( GetOwner() ); // Get owner of fun
 
      if (!pOwner) // If owner doesn't exist
      {
         return false; // Terminate code and return false
      }
 
      Vector vecSrc, vecAiming; // Create vectors used for secondary fire
 
      // Take the eye position and direction
      vecSrc = pOwner->EyePosition();
 
      QAngle angles = pOwner->GetLocalAngles(); // Get local angles
 
      AngleVectors( angles, &vecAiming );
 
      trace_t tr; // Create a new trace to use
 
      Vector   vecEnd = vecSrc + (vecAiming * 42);
      UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
 
      if (tr.fraction < 1.0)
      {
         // Don't attach to a living creature
         if (tr.m_pEnt)
         {
            CBaseEntity *pEntity = tr.m_pEnt;
            CBaseCombatCharacter *pBCC      = ToBaseCombatCharacter( pEntity );
            if (pBCC)
            {
               if (pBCC->GetHealth()<100) // If player does not have 100 health
                  return true; // Return true to 
            }
         }
         return false; // Else return false
      }
      else
      {
         return false;
      }
   }else {return false;}
}

bool CWeaponTroll::HealPlayer( void ) // Heal player declaration
{
   m_bFired = 1; // Sets it to fired
 
   CHL2MP_Player *pOwner = ToHL2MPPlayer( GetOwner() ); // Get owner of health vial
 
   if (!pOwner) // If Owner is no longer in server
   {
      return false; // Terminate code
   }
 
   Vector vecSrc, vecAiming;
 
   // Take the eye position and direction
   vecSrc = pOwner->EyePosition();
 
   QAngle angles = pOwner->GetLocalAngles();
 
   AngleVectors( angles, &vecAiming );
 
   trace_t tr;
 
   Vector   vecEnd = vecSrc + (vecAiming * 42);
   UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
 
   if (tr.fraction < 1.0)
   {
      // Don't attach to a living creature
      if (tr.m_pEnt)
      {
         CBaseEntity *pEntity = tr.m_pEnt;
         if (pEntity->IsPlayer())
         {
            if (pEntity->GetHealth()<pEntity->GetMaxHealth())
            {
#ifndef CLIENT_DLL
               CBasePlayer *pPlayer = ToBasePlayer(pEntity);
 
               CPASAttenuationFilter filter( pPlayer, "HealthVial.Touch" ); // Filters
               EmitSound( filter, pPlayer->entindex(), "HealthVial.Touch" ); // Play HealthVial.Touch
               pEntity->TakeHealth( 20, DMG_GENERIC ); // Damage 20 health under generic reason
#endif
               return true;
            }
         }
      }
      return false;
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTroll::SecondaryAttack( void )
{
	/*// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//Must have ammo
	if ( ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 ) || ( pPlayer->GetWaterLevel() == 3 ) )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	if( m_bInReload )
		m_bInReload = false;

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound( WPN_DOUBLE );

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecThrow;
	// Don't autoaim on grenade tosses
	AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow );
	VectorScale( vecThrow, 1000.0f, vecThrow );
	
#ifndef CLIENT_DLL
	//Create the grenade
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecSrc, vec3_angle, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );

	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE ); 
	pGrenade->SetThrower( GetOwner() );
	pGrenade->SetDamage( SMG1_GRENADE_DAMAGE );
	pGrenade->SetDamageRadius( SMG1_GRENADE_RADIUS );
#endif

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	//Tony; TODO SECONDARY!
//	ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );


	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
#ifndef CLIENT_DLL
	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	CSoundEnt::InsertSound( SOUND_COMBAT, pPlayer->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer );
#endif
#endif //SecobMod__Enable_Fixed_Multiplayer_AI*/
	if (CanHealPlayer()) // Checks if it can heal
      HealPlayer(); // Run method
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponTroll::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}