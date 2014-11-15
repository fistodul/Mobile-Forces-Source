#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	//#include "PHK_PHK.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PHK_TIMER	2.5f //Seconds

#define PHK_PAUSED_NO			0
#define PHK_PAUSED_PRIMARY		1
#define PHK_PAUSED_SECONDARY	2

#define PHK_RADIUS	4.0f // inches

#define PHK_DAMAGE_RADIUS 250.0f

#ifdef CLIENT_DLL
#define CWeaponPHK C_WeaponPHK
#endif

ConVar sk_portable_healthkit_health( "sk_portable_healthkit_health", "100", FCVAR_REPLICATED | FCVAR_CHEAT, "It sets ur Health to this value after u use it xD" );
ConVar sk_max_healthkit( "sk_max_healthkit", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "The max ammo for this PHK :P" );

//-----------------------------------------------------------------------------
// PHKmentation PHKs
//-----------------------------------------------------------------------------
class CWeaponPHK: public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponPHK, CBaseHL2MPCombatWeapon );
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponPHK();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	bool	Reload( void );

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	void	ThrowPHK( CBasePlayer *pPlayer );
	bool	IsPrimed( bool ) { return ( m_AttackPaused != 0 );	}
	
/*#ifdef GAME_DLL
	void IncrementHealth( int nCount )
	{ 
	m_iHealth += nCount;
	if (m_iMaxHealth > 0 && m_iHealth > m_iMaxHealth)
		m_iHealth = m_iMaxHealth;
	}
#endif*/

private:

	void	RollPHK( CBasePlayer *pPlayer );
	void	LobPHK( CBasePlayer *pPlayer );
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool,	m_bRedraw );	//Draw the weapon again after throwing a PHK
	
	CNetworkVar( int,	m_AttackPaused );
	CNetworkVar( bool,	m_fDrawbackFinished );

	CWeaponPHK( const CWeaponPHK & );

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#ifndef CLIENT_DLL

acttable_t	CWeaponPHK::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,					false },

#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true }, 

	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_GRENADE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_GRENADE,					false },
#endif //SecobMod__Enable_Fixed_Multiplayer_AI

};

IMPLEMENT_ACTTABLE(CWeaponPHK);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPHK, DT_WeaponPHK )

BEGIN_NETWORK_TABLE( CWeaponPHK, DT_WeaponPHK )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropBool( RECVINFO( m_fDrawbackFinished ) ),
	RecvPropInt( RECVINFO( m_AttackPaused ) ),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropBool( SENDINFO( m_fDrawbackFinished ) ),
	SendPropInt( SENDINFO( m_AttackPaused ) ),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPHK )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_portablehealthkit, CWeaponPHK );
PRECACHE_WEAPON_REGISTER(weapon_portablehealthkit);

CWeaponPHK::CWeaponPHK( void ) :
	CBaseHL2MPCombatWeapon()
{
	#ifdef Weighted_Weaponry
	Phk_Weight = 1;
	#endif
	m_bRedraw = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPHK::Precache( void )
{
	BaseClass::Precache();

	//PrecacheScriptSound( "WeaponPHK.Use" );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponPHK::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewPHK = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_fDrawbackFinished = true;
			break;

		case EVENT_WEAPON_THROW:
			ThrowPHK( pOwner );
			DecrementAmmo( pOwner );
			fThrewPHK = true;
			break;

		case EVENT_WEAPON_THROW2:
			RollPHK( pOwner );
			DecrementAmmo( pOwner );
			fThrewPHK = true;
			break;

		case EVENT_WEAPON_THROW3:
			LobPHK( pOwner );
			DecrementAmmo( pOwner );
			fThrewPHK = true;
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewPHK )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
/*#ifdef SecobMod__Enable_Fixed_Multiplayer_AI		
		// Make a sound designed to scare snipers back into their holes!
		CBaseCombatCharacter *pOwner = GetOwner();

		if( pOwner )
		{
			Vector vecSrc = pOwner->Weapon_ShootPosition();
			Vector	vecDir;

			AngleVectors( pOwner->EyeAngles(), &vecDir );

			trace_t tr;

			UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr );

			CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, tr.endpos, 384, 0.2, pOwner );
		}
#endif //SecobMod__Enable_Fixed_Multiplayer_AI*/
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPHK::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPHK::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPHK::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPHK::SecondaryAttack( void )
{
	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the PHK attack to pause.
	m_AttackPaused = PHK_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPHK::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
	{ 
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;

	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the PHK attack to pause.
	m_AttackPaused = PHK_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the PHK.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponPHK::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPHK::ItemPostFrame( void )
{
	if( m_fDrawbackFinished )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

		if (pOwner)
		{
			switch( m_AttackPaused )
			{
			case PHK_PAUSED_PRIMARY:
				if( !(pOwner->m_nButtons & IN_ATTACK) )
				{
					SendWeaponAnim( ACT_VM_THROW );
					m_fDrawbackFinished = false;
				}
				break;

			case PHK_PAUSED_SECONDARY:
				if( !(pOwner->m_nButtons & IN_ATTACK2) )
				{
					//See if we're ducking
					if ( pOwner->m_nButtons & IN_DUCK )
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_HAULBACK );
					}

					m_fDrawbackFinished = false;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponPHK::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(PHK_RADIUS+2,PHK_RADIUS+2,PHK_RADIUS+2), Vector(PHK_RADIUS+2,PHK_RADIUS+2,PHK_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

void DropPrimedPHKPHK( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pPHK )
{
	CWeaponPHK *pWeaponPHK = dynamic_cast<CWeaponPHK*>( pPHK );

	if ( pWeaponPHK )
	{
		pWeaponPHK->ThrowPHK( pPlayer );
		pWeaponPHK->DecrementAmmo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponPHK::ThrowPHK( CBasePlayer *pPlayer )
{
if ( m_iHealth == 100 )
return;
#ifndef CLIENT_DLL
	/*Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 1200;
	CBasePHK *pPHK = PHKPHK_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, PHK_TIMER, false );

	if ( pPHK )
	{
		if ( pPlayer && pPlayer->m_lifeState != LIFE_ALIVE )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pPHK->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
			}
		}
		
		pPHK->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pPHK->SetDamageRadius( PHK_DAMAGE_RADIUS );
	}*/
#endif
	/*#ifdef GAME_DLL
	SetHealthValue( sk_portable_healthkit_health.GetInt() )
	#endif*/
	//pPlayer->SetHealth( sk_portable_healthkit_health.GetInt());
	#ifdef GAME_DLL
	//IncrementHealth ( sk_portable_healthkit_health.GetInt());
	if ( pPlayer && pPlayer->GetHealth() < pPlayer->GetMaxHealth() ) 
	{
	pPlayer->TakeHealth( sk_portable_healthkit_health.GetInt(), DMG_GENERIC );
	}
	#endif
	EmitSound( "HealthVial.Touch" ); // Play HealthVial.Touch

	m_bRedraw = true;

	WeaponSound( SINGLE );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponPHK::LobPHK( CBasePlayer *pPlayer )
{
if ( m_iHealth == 100 )
return;
#ifndef CLIENT_DLL
	/*Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 350 + Vector( 0, 0, 50 );
	CBasePHK *pPHK = PHKPHK_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0), pPlayer, PHK_TIMER, false );

	if ( pPHK )
	{
		pPHK->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pPHK->SetDamageRadius( PHK_DAMAGE_RADIUS );
	}*/
#endif
	/*#ifdef GAME_DLL
	SetHealthValue( sk_portable_healthkit_health.GetInt() )
	#endif*/
	//pPlayer->SetHealth( sk_portable_healthkit_health.GetInt());
	#ifdef GAME_DLL
	//IncrementHealth ( sk_portable_healthkit_health.GetInt());
	if ( pPlayer && pPlayer->GetHealth() < pPlayer->GetMaxHealth() ) 
	{
	pPlayer->TakeHealth( sk_portable_healthkit_health.GetInt(), DMG_GENERIC );
	}
	#endif
	EmitSound( "HealthVial.Touch" ); // Play HealthVial.Touch

	WeaponSound( WPN_DOUBLE );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponPHK::RollPHK( CBasePlayer *pPlayer )
{
if ( m_iHealth == 100 )
return;
#ifndef CLIENT_DLL
	// BUGBUG: Hardcoded PHK width of 4 - better not change the model :)
	/*Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += PHK_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll PHK along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
	CBasePHK *pPHK = PHKPHK_Create( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, PHK_TIMER, false );

	if ( pPHK )
	{
		pPHK->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pPHK->SetDamageRadius( PHK_DAMAGE_RADIUS );
	}*/
#endif
	/*#ifdef GAME_DLL
	SetHealthValue( sk_portable_healthkit_health.GetInt() )
	#endif*/
	#ifdef GAME_DLL
	//IncrementHealth ( sk_portable_healthkit_health.GetInt());
	if ( pPlayer && pPlayer->GetHealth() < pPlayer->GetMaxHealth() ) 
	{
	pPlayer->TakeHealth( sk_portable_healthkit_health.GetInt(), DMG_GENERIC );
	}
	#endif
	//pPlayer->SetHealth( sk_portable_healthkit_health.GetInt());
	//pEntity->TakeHealth( 20, DMG_GENERIC ); // Damage 20 health under generic reason
	EmitSound( "HealthVial.Touch" ); // Play HealthVial.Touch

	WeaponSound( SPECIAL1 );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

