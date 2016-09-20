//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VEHICLE_APC_H
#define VEHICLE_APC_H

#ifdef pilotable
#ifdef _WIN32
#pragma once
#endif

#include "vehicle_base.h"
#include "smoke_trail.h"
//#include "vehicle_apc.h" //Make it inherrit in the future

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle with weaponry
//-----------------------------------------------------------------------------
class CPropAPC2;

class CAPC2FourWheelServerVehicle : public CFourWheelServerVehicle
{
	typedef CFourWheelServerVehicle BaseClass;
// IServerVehicle
public:
	bool		NPC_HasPrimaryWeapon( void ) { return true; }
	void		NPC_AimPrimaryWeapon( Vector vecTarget );
	bool		NPC_HasSecondaryWeapon( void ) { return true; }
	void		NPC_AimSecondaryWeapon( Vector vecTarget );
	void	GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles );


	// Weaponry
	void		Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	void		Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );
	float		Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	float		Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again
protected:
	CPropAPC2 *GetAPC( void );
};


//-----------------------------------------------------------------------------
// A driveable vehicle with a gun that shoots wherever the driver looks.
//-----------------------------------------------------------------------------
class CPropAPC2 : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropAPC2, CPropVehicleDriveable );
	DECLARE_SERVERCLASS();
public:

	CPropAPC2::CPropAPC2();

	// CBaseEntity
	virtual void Precache( void );
	void	Think( void );
	virtual void Spawn(void);
	virtual void Activate();
	virtual void UpdateOnRemove( void );

	// CPropVehicle
	virtual void	CreateServerVehicle( void );
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual Class_T	ClassifyPassenger(CBaseCombatCharacter *pPassenger, Class_T defaultClassification);
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual float	DamageModifier ( CTakeDamageInfo &info );
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual void	EnterVehicle( CBasePlayer *pPlayer );
//	virtual void	ExitVehicle( int iRole );
	virtual void		SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = false; }
	virtual void		SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = false; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	

	// Weaponry
	const Vector	&GetPrimaryGunOrigin( void );
	void			AimPrimaryWeapon( const Vector &vecForward );
	void			AimSecondaryWeapon( Vector &vecForward );
	void			AimSecondaryWeaponAt( CBaseEntity *pTarget );
	float			PrimaryWeaponFireTime( void ) { return m_flMachineGunTime; }
	float			SecondaryWeaponFireTime( void ) { return m_flRocketTime; }
	float			MaxAttackRange() const;
	bool			IsInPrimaryFiringCone() const { return m_bInFiringCone; }

	// Muzzle flashes
	const char		*GetTracerType( void ) ;
	void			DoImpactEffect( trace_t &tr, int nDamageType );
	void			DoMuzzleFlash( void );

	virtual Vector	EyePosition( );				// position of eyes
	
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	enum
	{
		MAX_SMOKE_TRAILS = 4,
		MAX_EXPLOSIONS = 4,
	};

	// Should we trigger a damage effect?
	bool ShouldTriggerDamageEffect( int nPrevHealth, int nEffectCount ) const;

	// Add a smoke trail since we've taken more damage
	void AddSmokeTrail( const Vector &vecPos );

	// Creates the breakable husk of an attack chopper
	void CreateChopperHusk();

	// Pow!
	void ExplodeAndThrowChunk( const Vector &vecExplosionPos );

	void Event_Killed( const CTakeDamageInfo &info );

	// Purpose: 
	void GetRocketShootPosition( Vector *pPosition );

	void FireMachineGun( void );
	void FireRocket( void );

	// Death volley 
	void FireDying( );

	// Create a corpse 
	void CreateCorpse( );
	void	Materialize( void );
	// Blows da shizzle up
	void InputDestroy( inputdata_t &inputdata );
	void InputFireMissileAt( inputdata_t &inputdata );

	void CreateAPCLaserDot( void );
	Vector InicialSpawn;
	QAngle InicialAngle;
private:
	// Danger sounds made by the APC
	float	m_flDangerSoundTime;
	float m_fReloadTime;
	float m_fCannonCharge;

	// handbrake after the fact to keep vehicles from rolling
	float	m_flHandbrakeTime;
	bool	m_bInitialHandbrake;

	// Damage effects
	int		m_nSmokeTrailCount;

	// Machine gun attacks
	int		m_nMachineGunMuzzleAttachment;
	int		m_nMachineGunBaseAttachment;
	float	m_flMachineGunTime;
	int		m_iMachineGunBurstLeft;
	Vector	m_vecBarrelPos;
	bool	m_bInFiringCone;

	// Rocket attacks
	EHANDLE	m_hLaserDot;
	EHANDLE m_hRocketTarget;
	int		m_iRocketSalvoLeft;
	float	m_flRocketTime;
	int		m_nRocketAttachment;
	int		m_nRocketSide;
	EHANDLE m_hSpecificRocketTarget;
	string_t m_strMissileHint;

	bool m_bSpawn;

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;

	Vector m_vOriginalMins;
	Vector m_vOriginalMaxs;

	COutputEvent m_OnDeath;
	COutputEvent m_OnFiredMissile;
	CNetworkVar( int,	m_iAmmoCount);
	CNetworkVar( int,	m_iCannonCount);
	DECLARE_DATADESC();
};

#endif
#endif // VEHICLE_APC_H
