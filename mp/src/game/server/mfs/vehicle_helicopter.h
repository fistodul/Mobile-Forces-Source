#ifndef VEHICLE_HELICOPTER_H
#define VEHICLE_HELICOPTER_H

#ifdef pilotable
#include "ai_blended_movement.h"
#include "animation.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_utils.h"
#include "smoke_trail.h"
#include "physics_bone_follower.h"
#include "vehicle_base.h"
#include "ai_hint.h"
#include "ai_memory.h"
#include "ai_moveprobe.h"
#include "npcevent.h"
#include "grenade_homer.h"
#include "IEffects.h"
#include "ndebugoverlay.h"
#include "soundent.h"
#include "soundenvelope.h"
#include "ai_squad.h"
#include "ai_network.h"
#include "ai_pathfinder.h"
#include "ai_senses.h"
#include "npc_rollermine.h"
#include "ai_blended_movement.h"
#include "physics_prop_ragdoll.h"
#include "iservervehicle.h"
#include "player_pickup.h"
#include "props.h"
#include "antlion_dust.h"
#include "npc_antlion.h"
#include "decals.h"
#include "prop_combine_ball.h"
#include "eventqueue.h"
#include "ai_basenpc.h"
#include "ai_motor.h"
#include "ai_navigator.h"

#ifdef _WIN32
#pragma once
#endif

class QUA_helicopter;

class CHLCServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	void	GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles );
	Vector	GetSmoothedVelocity( void );
	void	NPC_SetDriver( CNPC_VehicleDriver *pDriver );
	void	NPC_DriveVehicle( void );
	void    HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone );

	//bool GetPassengerExitPoint( int nRole, Vector *pExitPoint, QAngle *pAngles );
protected:
	QUA_helicopter *Gethelicopter( void );
};

class CQUAGrenadeHelicopter : public CBaseGrenade
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CQUAGrenadeHelicopter, CBaseGrenade );

	virtual void Precache( );
	virtual void Spawn( );
	virtual void UpdateOnRemove();
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );
	virtual void PhysicsSimulate( void );
	virtual float GetShakeAmplitude( void ) { return 25.0; }
	virtual float GetShakeRadius( void ) { return 550.0; }
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	
private:
	// Pow!
	void DoExplosion( const Vector &vecOrigin, const Vector &vecVelocity );
	void ExplodeThink();
	void RampSoundThink();
	void ExplodeConcussion( CBaseEntity *pOther );
	void BecomeActive();
	void ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );
	
	bool m_bActivated;
	CSoundPatch	*m_pWarnSound;
};

class QUA_helicopter : public CBaseAnimating, public IDrivableVehicle
{
	DECLARE_CLASS( QUA_helicopter, CBaseAnimating );
public:
	// TODO: Para que todo marche, hay que definir la serverclass
	// Lo haremos otro dia, pero POR FIN funciona el helicopter.
	DECLARE_SERVERCLASS();
	

	// Funciones basicas
	QUA_helicopter();
	/*~QUA_helicopter();*/

	void Spawn( void );
	virtual void Precache( void );
	void Think(void);
	void PostThink(void);
	void Activate(void);

	// De momento lo vamos a hacer basico. Luego
	// examinaremos como hacemos lo demas.
	

	// Esto hace que se pueda usar el helicopter.

	void Use(CBaseEntity *pActivator,CBaseEntity *pCaller,USE_TYPE useType,float value);
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | m_iCaps; }
	int m_iCaps;

	// A ver que pasa.

	Vector GetDriverEyes(void);

	// Conservative collision volumes
	
	// Funciones de animacion


	// Establece la actividad del helicopter.
	void SetActivity (Activity NewActivity);
	// Recoge cual es la actividad actual del helicopter.
	//Activity GetActivity (void) {return m_Activity; }
	bool IsPlayerInhelicopter(void);
	
	
	
	void			ResetUseKey( CBasePlayer *pPlayer );
	void			ResetForwardKey( CBasePlayer *pPlayer );

	//---------------------------------
	// Minigun
	//---------------------------------
	void Adelante();
	void GiraIzquierda();
	void GiraDerecha();
	void Dispara();
	void Morro(bool bAdelante);
	void Inclina();
	Vector DondeApuntaPlayer();
	Vector	GetSmoothedVelocity( void );
	void AimPrimaryWeapon( const Vector &vecWorldTarget );
	void CreateCorpse();
	CBaseAnimating *	GetEntity() { return this; }

	virtual void StopLoopingSounds();
CHLCServerVehicle		m_pServerVehicle;

	virtual IServerVehicle *GetServerVehicle() { return &m_pServerVehicle; }
	virtual CBaseEntity	*GetVehicleEnt() { return this; }
	

	// CPropVehicle
	virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );

	virtual void	CreateServerVehicle( void );
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	//virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual Class_T	ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification );
	//virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	//virtual float	DamageModifier ( CTakeDamageInfo &info );
	virtual void	SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = false; }
	virtual void	SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = false; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	
	virtual void		PreExitVehicle( CBaseCombatCharacter *pPlayer, int iRole ) {}
	virtual void		EnterVehicle( CBaseCombatCharacter *pPlayer );
	virtual void		ExitVehicle( int iRole );
	virtual bool		AllowBlockedExit( CBaseCombatCharacter *pPlayer, int iRole ) { return true; }
	virtual bool		AllowMidairExit( CBaseCombatCharacter *pPlayer, int nRole ) { return false; }

	string_t		GetVehicleScriptName() { return m_vehicleScript; }
	// Passengers do not directly receive damage from blasts or radiation damage
	bool PassengerShouldReceiveDamage(CTakeDamageInfo &info)
	{
		if (GetServerVehicle() && GetServerVehicle()->IsPassengerExiting())
			return false;

		if (info.GetDamageType() & DMG_VEHICLE)
			return true;

		return (info.GetDamageType() & (DMG_RADIATION | DMG_BLAST)) == 0;
	}

	void			Drivehelicopter( int iDriverButtons, int iButtonsPressed, float flNPCSteering = 0.0 );
	
	
	virtual Vector	EyePosition( );				// position of eyes
	void InitializeRotorSound( void );
	void	UpdateRotorWashVolume(void);
	float GetRotorVolume( void );
	virtual const char *GetTracerType( void );

	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	virtual void DoMuzzleFlash( void );
	void	Materialize( void );
	// Impact Shock

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	Vector CalcDamageForceVector( const CTakeDamageInfo &info );
	void CreateBomb( bool bCheckForFairness = true, Vector *pVecVelocity = NULL, bool bMegaBomb = false );
	// CBaseAnimating
	void HandleAnimEvent( animevent_t *pEvent );
private:
	
		CNetworkHandle( CBasePlayer, m_hPlayer );

	// NPC Driving
	CHandle<CNPC_VehicleDriver>		m_hNPCDriver;
	int								m_nNPCButtons;
	bool m_bPressed;
	Vector m_vImpulso;
	// Entering / Exiting
	bool				m_bLocked;
	CNetworkVar( bool,	m_bEnterAnimOn );
	CNetworkVar( bool,	m_bExitAnimOn );
	CNetworkVector(		m_vecEyeExitEndpoint );
	CNetworkVector( m_vecGunCrosshair );
	CNetworkVar( int,	m_iAmmoCount);
	CNetworkVar( int,	m_iCannonCount);
	float m_fReloadTime;
	float m_fCannonCharge;
	bool m_bSpawn;
	float m_flRespawnTime;
	Vector m_vecTotalBulletForce;
	float			m_aimYaw;
	float			m_aimPitch;

	float acl;
	float aclizq;
	float aclder;
	float aclup;
	float acldown;

	int		m_ActualSequence;
	float	m_flSequenceTime;
	float  m_flNextShootingTime;
	float m_flSiguienteSummon;
	int    m_nGunBaseAttachment;
	int		m_nGunTipAttachment;
	int			m_iAmmoType;
	bool		aux;
	float			m_idealHeight;
	float			m_HeightVelocity;
	float			m_flWaitAttack;
	int		m_nMachineGunMuzzleAttachment;
	int     m_nBombAttachment;
	Activity m_Activity;
	Activity m_GActivity;
	bool m_bCrouching;
	bool m_bCrouchPosture;
	bool m_bStanding;
	bool m_bStandPosture;
	bool m_bInhelicopter;
	bool m_Ataca;
	bool m_bhelicopterTurbo;
	bool m_Lanza;

	Vector	m_vecBarrelPos;

	// Los bonefollowers
	
	int				m_nFlinchActivity;

	bool			m_bStopped;
	bool			m_bIsBurrowed;
	bool			m_bBarkEnabled;
	float			m_flNextSummonTime;
	int				m_iNumLivehelicopters;
							
	float			m_flSearchNoiseTime;
	float			m_flAngerNoiseTime;
	float			m_flBreathTime;
	float			m_flChargeTime;
	float			m_flPhysicsCheckTime;
	float			m_flNextHeavyFlinchTime;
	float			m_flNextRoarTime;
	int				m_iChargeMisses;
	bool			m_bDecidedNotToStop;
	float			m_flSiguienteAtaque;				
	Vector			m_vecPhysicsTargetStartPos;
	Vector			m_vecPhysicsHitPosition;
	
	CSoundPatch	*m_pGunFiringSound;

	COutputEvent	m_OnSummon;

	CSoundPatch		*m_pGrowlHighSound;
	CSoundPatch		*m_pGrowlLowSound;
	CSoundPatch		*m_pGrowlIdleSound;
	CSoundPatch		*m_pBreathSound;
	CSoundPatch		*m_pConfusedSound;
protected:
	CSoundPatch		*m_pRotorSound;				// Rotor loop played when the player can see the helicopter
	CSoundPatch		*m_pRotorBlast;	
int				m_iSoundState;
bool			m_bSuppressSound;
COutputEvent m_OnDeath;
string_t		m_vehicleScript;

	// Para Respawn

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;

	Vector m_vOriginalMins;
	Vector m_vOriginalMaxs;
	DECLARE_DATADESC();
};

#endif
#endif // VEHICLE_HELICOPTER_H