#ifndef QUA_STRIDER_H
#define QUA_STRIDER_H

#ifdef pilotable
#include "ai_blended_movement.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_utils.h"
#include "smoke_trail.h"
#include "physics_bone_follower.h"
#include "physics_prop_ragdoll.h"

#ifdef _WIN32
#pragma once
#endif

const int NUM_STRIDER_IK_TARGETS = 6;

class CQUAStriderMinigun;
class QUA_Strider;
class CRagdollProp;
struct QUAStriderMinigunViewcone_t;
struct AI_EnemyInfo_t;



void AdjustStriderNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );

//-----------------------------------------------------------------------------
//
// Strider Minigun
//
//-----------------------------------------------------------------------------

class IQUAMinigunHost
{
public:
	virtual void ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin ) = 0;
	virtual void UpdateMinigunControls( float &yaw, float &pitch ) = 0;
	virtual void GetViewCone( QUAStriderMinigunViewcone_t &cone ) = 0;
	virtual void NewTarget() = 0;
	virtual void OnMinigunStartShooting( CBaseEntity *pTarget ) = 0;
	virtual void OnMinigunStopShooting( CBaseEntity *pTarget ) = 0;
	virtual CBaseAnimating *GetEntity() = 0;
};
//-----------------------------------------------------------------------------
// Purpose: Crane vehicle server
//-----------------------------------------------------------------------------
class CQUAServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	void	GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles );
	Vector GetSmoothedVelocity( void );
	// NPC Driving
	void	NPC_SetDriver( CNPC_VehicleDriver *pDriver );
	void	NPC_DriveVehicle( void );
	void    HandlePassengerEntry( CBasePlayer *pPlayer, bool bAllowEntryOutsideZone );

protected:
	QUA_Strider *GetStrider( void );
};

class QUA_Strider : public CBaseAnimating, public IQUAMinigunHost, public IDrivableVehicle
{
	DECLARE_CLASS( QUA_Strider, CBaseAnimating );
public:
	// TODO: Para que todo marche, hay que definir la serverclass
	// Lo haremos otro dia, pero POR FIN funciona el strider.
	 DECLARE_SERVERCLASS();
	

	// Funciones basicas
	QUA_Strider();
	~QUA_Strider();

	void Spawn( void );
	virtual void Precache( void );
	void Think(void);
	void PostThink(void);
	void Activate(void);

	bool CreateVPhysics();
	
	void InitBoneControllers();

	void SetupGlobalModelData(void);

	void			CalculateIKLocks( float currentTime );

	// Esto hace que se pueda usar el Strider.

	void Use(CBaseEntity *pActivator,CBaseEntity *pCaller,USE_TYPE useType,float value);
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | m_iCaps; }
	int m_iCaps;

	// A ver que pasa.

	Vector GetDriverEyes(void);

	// Conservative collision volumes
	static Vector gm_cullBoxStandMins;
	static Vector gm_cullBoxStandMaxs;
	static Vector gm_cullBoxCrouchMins;
	static Vector gm_cullBoxCrouchMaxs;
	static float gm_strideLength;
	
	// Funciones de animacion


	// Establece la actividad del Strider.
	void SetActivity (Activity NewActivity);
	// Recoge cual es la actividad actual del Strider.
	Activity GetActivity (void) {return m_Activity; }

	bool IsPlayerInStrider(void);
	// Alturas
	// Contained IServerVehicle

	CQUAServerVehicle		m_pServerVehicle;

	float			GetMaxHeightModel() const	{ return 500.0; }
	float			GetMaxHeight() const		{ return 490.0; }
	float			GetMinHeight() const		{ return 200.0; }
	float			GetHeightRange() const  { return GetMaxHeight() - GetMinHeight(); }
	void			SetHeight( float h );
	float			GetHeight()					{ return GetPoseParameter( gm_BodyHeightPoseParam ); }
	void 			SetIdealHeight( float h );	
	
	// Posturas

	bool			IsCrouching() { return m_bCrouching;}
	bool			IsStanding()  { return m_bStanding;}
	bool			IsInCrouchedPosture() {return m_bCrouchPosture;}
	bool			IsInStandPosture() {return m_bStandPosture;}
	
	// STOMPHIT

	Vector CalculateStompHitByPlayer(void);
	void StompHit( int followerBoneIndex );
	
	bool			TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	Vector			CannonPosition();

	void			ResetUseKey( CBasePlayer *pPlayer );

	//---------------------------------
	// Minigun
	//---------------------------------


	void			ShootMinigun( const Vector *pTarget, float aimError, const Vector &vecSpread = vec3_origin );
	void			UpdateMinigunControls( float &yaw, float &pitch ) {}; // ojo
	void			GetViewCone( QUAStriderMinigunViewcone_t &cone ) {}; // ojo
	void			NewTarget() { m_flTargetAcquiredTime = gpGlobals->curtime; }
	void			OnMinigunStartShooting( CBaseEntity *pTarget ) {};
	void			OnMinigunStopShooting( CBaseEntity *pTarget)  {}; // ojo


	void			DoMuzzleFlash();
	void Adelante();
	void GiraIzquierda();
	void GiraDerecha();
	void Dispara();
	Vector DondeApuntaPlayer();

	CBaseAnimating *	GetEntity() { return this; }

	bool			HasPass()	{ return m_PlayerFreePass.HasPass(); }
	void            TakeDamage( const CTakeDamageInfo &inputInfo );
	int	VPhysicsTakeDamage( const CTakeDamageInfo &info );

virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer );
//virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );
//	virtual void		SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = false; }
//	virtual void		SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = false; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	virtual void		PreExitVehicle(CBaseCombatCharacter *pPlayer, int iRole) {}
	virtual void		EnterVehicle(CBaseCombatCharacter *pPlayer);
	virtual void		ExitVehicle( int iRole );
	virtual bool		AllowBlockedExit(CBaseCombatCharacter *pPlayer, int iRole) { return true; }
	virtual bool		AllowMidairExit(CBaseCombatCharacter *pPlayer, int nRole) { return false; }

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
//	
//
	virtual IServerVehicle *GetServerVehicle() { return &m_pServerVehicle; }
	virtual CBaseEntity	*GetVehicleEnt() { return this; }
	// CPropVehicle
	virtual void	CreateServerVehicle( void );
	virtual void	DestroyServerVehicle( void);
	virtual void	DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual Class_T	ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification );
	virtual void	SetVehicleEntryAnim( bool bOn ) { m_bEnterAnimOn = false; }
	virtual void	SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = false; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	
	void	DriveStrider( int iDriverButtons, int iButtonsPressed, float flNPCSteering = 0.0 );
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void DoImpactEffect( trace_t &tr, int nDamageType );
	void KillStrider(void);

	void FireCannonManually(void);
	void ShootCannon(void);
	bool AimCannonAtManual( Vector pTarget, float flInterval );
	void HandleAnimEvent( animevent_t *pEvent );
	void CannonHitThink();
	Vector			LeftFootHit( float eventtime );
	Vector			RightFootHit( float eventtime );
	Vector			BackFootHit( float eventtime );
	//void			StompHit( int followerBoneIndex );
	void			FootFX( const Vector &origin );

	// Spawn

	void	Materialize( void );
	void CreateRagdollEntity( void );

	// Sobre la vida
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual float			DamageModifier ( CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	void			StartSmoking();
	void			StopSmoking( float flDelay = 0.1 );
	bool			IsSmoking() { return m_hSmoke != NULL; }
	Vector CalcDamageForceVector( const CTakeDamageInfo &info );

	bool m_bSpawn;
	float m_flRespawnTime;
	int m_iSpawnTry;
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	Vector m_vecTotalBulletForce;
private:

	CQUAStriderMinigun *m_pMinigun;
	bool force_exit;
	float m_fKillStrider;
	bool m_bMuerte;
	float acl;
	float aclizq;
	float aclder;
	bool m_IsTurning;
	CBaseEntity *elegido; // Elegido para morir bajo las patas de nuestro amigo.

	// Cannon Vectors
	Vector			m_blastHit;
	Vector			m_blastNormal;
	//

	// RagDoll
	CHandle<SmokeTrail> m_hSmoke;

CAI_FreePass m_PlayerFreePass;

	int		m_ActualSequence;
	float	m_flSequenceTime;
	float  m_flNextShootingTime;
	bool		aux;
	bool   m_DisCanon;
	float m_flCannonShot;
	float m_aimYaw;
	float m_aimPitch;
	float			m_idealHeight;
	float			m_HeightVelocity;
	Activity m_Activity;
	bool m_bCrouching;
	bool m_bCrouchPosture;
	bool m_bStanding;
	bool m_bStandPosture;
	bool m_bInStrider;
	bool m_bStompAnim;
	float m_flNextCannonTime;

float			m_flTargetAcquiredTime;

	static float	gm_zCannonDist;
	static float	gm_zMinigunDist;
	static Vector	gm_vLocalRelativePositionCannon;
	static Vector	gm_vLocalRelativePositionMinigun;

	static int		gm_YawControl;
	static int		gm_PitchControl;
	static int		gm_CannonAttachment;
	static int		gm_BodyHeightPoseParam;
	static int		gm_DriverEyes;

	// Los bonefollowers
	CBoneFollowerManager m_BoneFollowerManager;

	int				m_BodyTargetBone;
	bool			m_bDisableBoneFollowers;
	CHandle<CBasePlayer> miPlayer;
	//CHandle<CBaseCombatWeapon> m_hSaveWeapon;;

	CNetworkHandle( CBasePlayer, m_hPlayer );

	// NPC Driving
	CHandle<CNPC_VehicleDriver>		m_hNPCDriver;
	int								m_nNPCButtons;

	// Entering / Exiting
	bool				m_bLocked;
	CNetworkVar( bool,	m_bEnterAnimOn );
	CNetworkVar( bool,	m_bExitAnimOn );
	CNetworkVector(		m_vecEyeExitEndpoint );
	CNetworkVector( m_vecGunCrosshair );
	int				m_miniGunAmmo;
	CNetworkVar( int,	m_iAmmoCount);
	CNetworkVar( int,	m_iCannonCount);

	float m_fReloadTime;
	float m_fCannonCharge;

	CNetworkVector( m_vecHitPos );
	CNetworkVector( m_EyePosition );
	CNetworkArray( Vector, m_vecIKTarget, NUM_STRIDER_IK_TARGETS );
	
	CRandSimTimer	m_PostureAnimationTimer;
	COutputEvent m_OnDeath;
	string_t		m_vehicleScript;

	// Para Respawn

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;

	Vector m_vOriginalMins;
	Vector m_vOriginalMaxs;


	DECLARE_DATADESC();

};

enum QUAStriderMinigunPeg_t
{
	MINIGUN_PEGGED_DONT_CARE = 0,
	MINIGUN_PEGGED_UP,
	MINIGUN_PEGGED_DOWN,
	MINIGUN_PEGGED_LEFT,
	MINIGUN_PEGGED_RIGHT,
};

//---------------------------------------------------------

struct QUAStriderMinigunViewcone_t
{
	Vector origin;
	Vector axis;
	float cosAngle;
	float length;
};

//---------------------------------------------------------

struct QUAStriderMinigunAnimController_t
{
	float	current;
	float	target;
	float	rate;

	void Update( float dt, bool approach = true )
	{
		if( approach )
		{
			current = Approach( target, current, rate * dt );
		}
		else
		{
			current = target;
		}
	}

	void Random( float minTarget, float maxTarget, float minRate, float maxRate )
	{
		target = random->RandomFloat( minTarget, maxTarget );
		rate = random->RandomFloat( minRate, maxRate );
	}
};

//---------------------------------------------------------

class CQUAStriderMinigun
{
public:
	DECLARE_DATADESC();

	void		Init();
	void		SetTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, bool bOverrideEnemy = false );
	CBaseEntity *GetTarget()		{ return m_hTarget.Get(); }
	void		Think( IQUAMinigunHost *pHost, float dt );
	void		SetState( int newState );
	bool		ShouldFindTarget( IQUAMinigunHost *pHost );
	void 		AimAtPoint( IQUAMinigunHost *pHost, const Vector &vecPoint, bool bSnap = false );
	void 		AimAtTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, bool bSnap = false );
	void 		ShootAtTarget( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float shootTime );
	void 		StartShooting( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float duration );
	void 		ExtendShooting( float timeExtend );
	void 		SetShootDuration( float duration );
	void 		StopShootingForSeconds( IQUAMinigunHost *pHost, CBaseEntity *pTarget, float duration );
	bool 		IsPegged( int dir = MINIGUN_PEGGED_DONT_CARE );
	bool 		CanStartShooting( IQUAMinigunHost *pHost, CBaseEntity *pTargetEnt );
	float 		GetBurstTimeRemaining() { return m_burstTime - gpGlobals->curtime; }

	void 		RecordShotOnTarget()			{ m_iOnTargetShots++; }
	void 		ClearOnTarget()					{ m_iOnTargetShots = 0; }
	bool		IsOnTarget( int numShots = 0 )	{ return ( numShots == 0 ) ? (m_iOnTargetShots > 0) : (m_iOnTargetShots >= numShots); }

	void		Enable( IQUAMinigunHost *pHost, bool enable );
	float		GetAimError();
	
	enum minigunstates_t
	{
		MINIGUN_OFF = 0,
		MINIGUN_SHOOTING = 1,
	};

	int			GetState()		{ return m_minigunState; }
	bool		IsShooting()	{ return GetState() == MINIGUN_SHOOTING; }

private:
	bool		m_enable;
	int			m_minigunState;
	float		m_nextBulletTime;	// Minigun is shooting, when can I fire my next bullet?
	float		m_burstTime;		// If firing, how long till done? If not, how long till I can?
	float		m_nextTwitchTime;
	int			m_randomState;
	EHANDLE		m_hTarget;
	QUAStriderMinigunAnimController_t m_yaw;
	QUAStriderMinigunAnimController_t m_pitch;
	bool		m_bWarnedAI;
	float		m_shootDuration;
	Vector		m_vecAnchor;		// A burst starts here and goes to the target's orgin.
	bool		m_bOverrideEnemy;	// The minigun wants something other than the Strider's enemy as a target right now.
	Vector		m_vecLastTargetPos;	// Last place minigun saw the target.
	int			m_iOnTargetShots;
};

#endif
#endif // QUA_STRIDER_H