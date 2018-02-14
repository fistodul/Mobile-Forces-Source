//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#ifdef pilotable
//#include "c_ai_basenpc.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_sparks.h"
#include "c_tracer.h"
#include "clientsideeffects.h"
#include "iefx.h"
#include "dlight.h"
#include "bone_setup.h"
#include "c_rope.h"
#include "fx_line.h"
#include "c_sprite.h"
#include "view.h"
#include "view_scene.h"
#include "materialsystem/imaterialvar.h"
#include "simple_keys.h"
#include "fx_envelope.h"
#include "iclientvehicle.h"
#include "engine/ivdebugoverlay.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "movevars_shared.h"
#include "view.h"
#include "flashlighteffect.h"
#include "c_baseplayer.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "ClientEffectPrecacheSystem.h"
#include <bitbuf.h>
#include "fx_water.h"

#include "c_prop_vehicle.h"
#include "hud.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar default_fov;

#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3
#define STRIDER_MSG_MACHINEGUN			4

#define STOMP_IK_SLOT					11
const int NUM_STRIDER_IK_TARGETS = 6;

const float	STRIDERFX_BIG_SHOT_TIME = 1.25f;
const float STRIDERFX_END_ALL_TIME = 4.0f;

class C_QUA_StriderFX : public C_EnvelopeFX
{
public:
	typedef C_EnvelopeFX	BaseClass;

	C_QUA_StriderFX();
	~C_QUA_StriderFX()
	{
		EffectShutdown();
	}


	void			Update( C_BaseEntity *pOwner, const Vector &targetPos );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( m_worldPosition, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

	virtual void EffectInit( int entityIndex, int attachment )
	{
		m_limitHitTime = 0;
		BaseClass::EffectInit( entityIndex, attachment );
	}
	virtual void EffectShutdown( void )
	{
		m_limitHitTime = 0;
		BaseClass::EffectShutdown();
	}

	virtual int	DrawModel( int flags );
	virtual void LimitTime( float tmax ) 
	{ 
		float dt = tmax - m_t;
		if ( dt < 0 )
		{
			dt = 0;
		}
		m_limitHitTime = gpGlobals->curtime + dt;
		BaseClass::LimitTime( tmax );
	}

	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_beamEndPosition;
	pixelvis_handle_t		m_queryHandleGun;
	pixelvis_handle_t		m_queryHandleBeamEnd;
	float					m_limitHitTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_QUA_Strider : public C_BaseAnimating, public IClientVehicle
{
	DECLARE_CLASS( C_QUA_Strider, C_BaseAnimating);
public:
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();
	DECLARE_DATADESC();


					C_QUA_Strider();
	virtual			~C_QUA_Strider();

	// model specific
	virtual void	ReceiveMessage( int classID, bf_read &msg );
	virtual void	CalculateIKLocks( float currentTime )
	{
		// NOTE: All strider IK is solved on the server, enable this to do it client-side
		// BaseClass::CalculateIKLocks( currentTime );
		if ( m_pIk && m_pIk->m_target.Count() )
		{
			Assert(m_pIk->m_target.Count() > STOMP_IK_SLOT);
			// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
			CIKTarget &target = m_pIk->m_target[STOMP_IK_SLOT];
			target.SetPos( m_vecHitPos );
			// target.latched.pos = m_vecHitPos;

			for ( int i = 0; i < NUM_STRIDER_IK_TARGETS; i++ )
			{
				CIKTarget &target = m_pIk->m_target[i];
				target.SetPos( m_vecIKTarget[i] );
#if 0
				debugoverlay->AddBoxOverlay( m_vecIKTarget[i], Vector( -2, -2, -2 ), Vector( 2, 2, 2), QAngle( 0, 0, 0 ), (int)255*m_pIk->m_target[i].est.latched, 0, 0, 0, 0 );
#endif
			}
		}
	}

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual void ClientThink();

public:

	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	// IClientVehicle overrides.
	void RestrictView( float *pYawBounds, float *pPitchBounds, float *pRollBounds, QAngle &vecViewAngles );
	//void OnEnteredVehicle( C_BasePlayer *pPlayer );
	void GetVehicleViewPosition(int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL);
	/*void VehicleViewSmoothingAPC( CBasePlayer *pPlayer, Vector *pAbsOrigin, QAngle *pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector *vecEyeExitEndpoint, ViewSmoothingData_t *pData, float *pFOV );
	void RemapViewAngles( ViewSmoothingData_t *pData, QAngle &vehicleEyeAngles );
	float ApplyViewLocking( float flAngleRaw, float flAngleClamped, ViewLockData_t &lockData, RemapAngleRange_CurvePart_t eCurvePart );
	*/
	// IClientVehicle overrides.
	//virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual void GetVehicleFOV( float &flFOV ) { flFOV = 0.0f; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	virtual C_BaseCombatCharacter* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BaseCombatCharacter *pEnt );
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;
	virtual int GetPrimaryAmmoType() const { return -1; }
	virtual int GetPrimaryAmmoCount() const { return m_iAmmoCount; }
	virtual int GetPrimaryAmmoClip() const  { return m_iCannonCount; }
	virtual bool PrimaryAmmoUsesClips() const { return false; }
	int GetJoystickResponseCurve() const;

public:

	// C_BaseEntity overrides.
	virtual int		GetHealth() const { return m_iHealth; };
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
private:
	C_QUA_Strider( const C_QUA_Strider & );
	C_QUA_StriderFX	m_cannonFX;
	Vector		m_vecHitPos;
	Vector		m_EyePosition;
	Vector		m_vecIKTarget[NUM_STRIDER_IK_TARGETS];
	CInterpolatedVar< Vector >		m_iv_vecHitPos;
	CInterpolatedVarArray< Vector, NUM_STRIDER_IK_TARGETS >	m_iv_vecIKTarget;
	Vector		m_vecRenderMins;
	Vector		m_vecRenderMaxs;
	
	EHANDLE	m_hRagdoll;
	

	float m_flNextRopeCutTime;
	
	// IClient

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;


	CHandle<C_BasePlayer>	m_hPlayer;
	CHandle<C_BasePlayer>	m_hPrevPlayer;

	bool					m_bEnterAnimOn;
	bool					m_bExitAnimOn;
	Vector					m_vecEyeExitEndpoint;
	int						m_iAmmoCount;
	int						m_iCannonCount;
	bool					m_bMagnetOn;

	Vector					m_vecOldShadowDir;

	ViewSmoothingData_t			m_ViewSmoothingData;

	// Fin IClient

	Vector						m_vecGunCrosshair;
	CInterpolatedVar<Vector>	m_iv_vecGunCrosshair;
};

class C_StriderRagdoll : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_StriderRagdoll, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	
	C_StriderRagdoll();
	~C_StriderRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	void UpdateOnRemove( void );
	virtual void SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);
	
private:
	
	C_StriderRagdoll( const C_StriderRagdoll & ) {}

	void Interp_Copy( VarMapping_t *pDest, C_BaseAnimating *pDestinationEntity, C_BaseAnimating *pSourceEntity, VarMapping_t *pSrc );
	void CreateStriderRagdoll( void );

private:

	EHANDLE	m_hStrider;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar(float , m_fAltura);
};


IMPLEMENT_CLIENTCLASS_DT(C_QUA_Strider, DT_NPC_QUA_Strider, QUA_Strider)
	RecvPropVector(RECVINFO(m_vecHitPos)),
	RecvPropVector(RECVINFO(m_EyePosition)),
	RecvPropVector(RECVINFO(m_vecIKTarget[0])),
	RecvPropVector(RECVINFO(m_vecIKTarget[1])),
	RecvPropVector(RECVINFO(m_vecIKTarget[2])),
	RecvPropVector(RECVINFO(m_vecIKTarget[3])),
	RecvPropVector(RECVINFO(m_vecIKTarget[4])),
	RecvPropVector(RECVINFO(m_vecIKTarget[5])),
	RecvPropInt		(RECVINFO(m_iHealth)),
	RecvPropInt		(RECVINFO(m_iAmmoCount)),
	RecvPropInt		(RECVINFO(m_iCannonCount)),
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_QUA_Strider )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()

extern ConVar joy_response_move_vehicle;

C_QUA_StriderFX::C_QUA_StriderFX()
{
	m_pOwner = NULL;
	m_active = false;
}

//-----------------------------------------------------------------------------
// By default all driveable vehicles use the curve defined by the convar.
//-----------------------------------------------------------------------------
int C_QUA_Strider::GetJoystickResponseCurve() const
{
	return joy_response_move_vehicle.GetInt();
}

void C_QUA_StriderFX::Update( C_BaseEntity *pOwner, const Vector &targetPos )
{
	BaseClass::Update();

	m_pOwner = pOwner;
	
	if ( m_active )
	{
		m_targetPosition = targetPos;
	}
}

// --on gun
// warpy sprite bit
// darkening sprite
// glowy blue flare sprite
// bubble warpy sprite
// after glow sprite

// --on line of sight
// narrow beam
// wide beam

// --on impact point
// sparkly white bits
// sparkly white streaks
// pale blue particle steam

enum 
{
	STRIDERFX_WARP_SCALE = 0,
	STRIDERFX_DARKNESS,
	STRIDERFX_FLARE_COLOR,
	STRIDERFX_FLARE_SIZE,
	STRIDERFX_BUBBLE_SIZE,
	STRIDERFX_BUBBLE_REFRACT,

	STRIDERFX_NARROW_BEAM_COLOR,
	STRIDERFX_NARROW_BEAM_SIZE,
	
	STRIDERFX_WIDE_BEAM_COLOR,
	STRIDERFX_WIDE_BEAM_SIZE,

	STRIDERFX_AFTERGLOW_COLOR,

	STRIDERFX_WIDE_BEAM_LENGTH,

	STRIDERFX_SPARK_COUNT,
	STRIDERFX_STREAK_COUNT,
	STRIDERFX_STEAM_COUNT,


	// must be last
	STRIDERFX_PARAMETERS,
};

class CQUAStriderFXEnvelope
{
public:
	CQUAStriderFXEnvelope();

	void AddKey( int parameterIndex, const CSimpleKeyInterp &key )
	{
		Assert( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS );

		if ( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS )
		{
			m_parameters[parameterIndex].Insert( key );
		}

	}

	CSimpleKeyList		m_parameters[STRIDERFX_PARAMETERS];
};

// NOTE: Beam widths are half-widths or radii, so this is a beam that represents a cylinder with 2" radius
const float NARROW_BEAM_WIDTH = 2;
const float WIDE_BEAM_WIDTH = 16;
const float FLARE_SIZE = 128;
const float	DARK_SIZE = 64;
const float AFTERGLOW_SIZE = 64;

const float WARP_SIZE = 512;
const float WARP_REFRACT = 0.075f;
const float WARP_BUBBLE_SIZE = 256;
const float WARP_BUBBLE_REFRACT = 1.0f;

CQUAStriderFXEnvelope::CQUAStriderFXEnvelope()
{
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.25, KEY_SPLINE, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0.5, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.5, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 2.0, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0.5 ) );
	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 2.0, KEY_DECELERATE, 2 ) );

	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 1.3, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 2.0, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1.0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.5, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.5, KEY_DECELERATE, 2 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.75, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 2.1, KEY_SPLINE, 0 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 2.1, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.25, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.5, KEY_ACCELERATE, 0 ) );

	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.5, KEY_ACCELERATE, 0.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 2.1, KEY_LINEAR, 0 ) );

	//AddKey( STRIDERFX_SPARK_COUNT,
	//AddKey( STRIDERFX_STREAK_COUNT,
	//AddKey( STRIDERFX_STEAM_COUNT,
}

CQUAStriderFXEnvelope g_StriderCannonEnvelope;

void QUA_ScaleColor( color32 &out, const color32 &in, float scale )
{
	out.r = (byte)(int)((float)in.r * scale);
	out.g = (byte)(int)((float)in.g * scale);
	out.b = (byte)(int)((float)in.b * scale);
	out.a = (byte)(int)((float)in.a * scale);
}

void QUA_DrawSpriteTangentSpace( const Vector &vecOrigin, float flWidth, float flHeight, color32 color )
{
	unsigned char pColor[4] = { color.r, color.g, color.b, color.a };

	// Generate half-widths
	flWidth *= 0.5f;
	flHeight *= 0.5f;

	// Compute direction vectors for the sprite
	Vector fwd, right( 1, 0, 0 ), up( 0, 1, 0 );
	VectorSubtract( CurrentViewOrigin(), vecOrigin, fwd );
	float flDist = VectorNormalize( fwd );
	if (flDist >= 1e-3)
	{
		CrossProduct( CurrentViewUp(), fwd, right );
		flDist = VectorNormalize( right );
		if (flDist >= 1e-3)
		{
			CrossProduct( fwd, right, up );
		}
		else
		{
			// In this case, fwd == g_vecVUp, it's right above or 
			// below us in screen space
			CrossProduct( fwd, CurrentViewRight(), up );
			VectorNormalize( up );
			CrossProduct( up, fwd, right );
		}
	}

	Vector left = -right;
	Vector down = -up;
	Vector back = -fwd;

	CMatRenderContextPtr pRenderContext(materials);

	CMeshBuilder meshBuilder;
	Vector point;
	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}


void QUA_Strider_DrawSprite( const Vector &vecOrigin, float size, const color32 &color )
{
	QUA_DrawSpriteTangentSpace( vecOrigin, size, size, color );
}


void QUA_Strider_DrawLine( const Vector &start, const Vector &end, float width, IMaterial *pMaterial, const color32 &color )
{
	FX_DrawLineFade( start, end, width, pMaterial, color, 8.0f );
}

int	C_QUA_StriderFX::DrawModel( int )
{
	static color32 white = {255,255,255,255};
	Vector params[STRIDERFX_PARAMETERS];
	bool hasParam[STRIDERFX_PARAMETERS];

	if ( !m_active )
		return 1;

	C_BaseEntity *ent = cl_entitylist->GetEnt( m_entityIndex );
	if ( ent )
	{
		QAngle angles;
		ent->GetAttachment( m_attachment, m_worldPosition, angles );
	}

	// This forces time to drive from the main clock instead of being integrated per-draw below
	// that way the effect moves on even when culled for visibility
	if ( m_limitHitTime > 0 && m_tMax > 0 )
	{
		float dt = m_limitHitTime - gpGlobals->curtime;
		if ( dt < 0 )
		{
			dt = 0;
		}
		// if the clock needs to move, update it.
		if ( m_tMax - dt > m_t )
		{
			m_t = m_tMax - dt;
			m_beamEndPosition = m_worldPosition;
		}
	}
	else
	{
		// don't have enough info to derive the time, integrate current frame time
		m_t += gpGlobals->frametime;
		if ( m_tMax > 0 )
		{
			m_t = clamp( m_t, 0, m_tMax );
			m_beamEndPosition = m_worldPosition;
		}
	}
	float t = m_t;

	bool hasAny = false;
	memset( hasParam, 0, sizeof(hasParam) );
	for ( int i = 0; i < STRIDERFX_PARAMETERS; i++ )
	{
		hasParam[i] = g_StriderCannonEnvelope.m_parameters[i].Interp( params[i], t );
		hasAny = hasAny || hasParam[i];
	}

	pixelvis_queryparams_t gunParams;
	gunParams.Init(m_worldPosition, 4.0f);
	float gunFractionVisible = PixelVisibility_FractionVisible( gunParams, &m_queryHandleGun );
	bool gunVisible = gunFractionVisible > 0.0f ? true : false;

	// draw the narrow beam
	if ( hasParam[STRIDERFX_NARROW_BEAM_COLOR] && hasParam[STRIDERFX_NARROW_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "sprites/bluelaser1", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = NARROW_BEAM_WIDTH * params[STRIDERFX_NARROW_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_NARROW_BEAM_COLOR].x;
		QUA_ScaleColor( color, white, bright );

		QUA_Strider_DrawLine( m_beamEndPosition, m_targetPosition, width, pMat, color );
	}

	// draw the wide beam
	if ( hasParam[STRIDERFX_WIDE_BEAM_COLOR] && hasParam[STRIDERFX_WIDE_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblacklargebeam", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = WIDE_BEAM_WIDTH * params[STRIDERFX_WIDE_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_WIDE_BEAM_COLOR].x;
		QUA_ScaleColor( color, white, bright );
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}

		QUA_Strider_DrawLine( wideBeamEnd, m_targetPosition, width, pMat, color );
	}

// after glow sprite
	bool updated = false;
// warpy sprite bit
	if ( hasParam[STRIDERFX_WARP_SCALE] && !hasParam[STRIDERFX_BUBBLE_SIZE] && gunVisible )
	{
		if ( !updated )
		{
			updated = true;
			materials->Flush();
			UpdateRefractTexture();
		}

		IMaterial *pMat = materials->FindMaterial( "effects/strider_pinch_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = WARP_SIZE;
		float refract = params[STRIDERFX_WARP_SCALE].x * WARP_REFRACT * gunFractionVisible;

		CMatRenderContextPtr pRenderContext(materials);

		pRenderContext->Bind(pMat, (IClientRenderable*)this);
		IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
		pVar->SetFloatValue( refract );
		QUA_Strider_DrawSprite( m_worldPosition, size, white );
	}
// darkening sprite
// glowy blue flare sprite
	if ( hasParam[STRIDERFX_FLARE_COLOR] && hasParam[STRIDERFX_FLARE_SIZE] && hasParam[STRIDERFX_DARKNESS] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = FLARE_SIZE * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_FLARE_COLOR].x * gunFractionVisible;
		QUA_ScaleColor( color, white, bright );
		color.a = (int)(255 * params[STRIDERFX_DARKNESS].x);
		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->Bind(pMat, (IClientRenderable*)this);
		QUA_Strider_DrawSprite( m_worldPosition, size, color );
	}
// bubble warpy sprite
	if ( hasParam[STRIDERFX_BUBBLE_SIZE] )
	{
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}
		pixelvis_queryparams_t endParams;
		endParams.Init(wideBeamEnd, 4.0f, 0.001f);
		float endFractionVisible = PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
		bool endVisible = endFractionVisible > 0.0f ? true : false;

		if ( endVisible )
		{
			if ( !updated )
			{
				updated = true;
				materials->Flush();
				UpdateRefractTexture();
			}
			IMaterial *pMat = materials->FindMaterial( "effects/strider_bulge_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
			float refract = endFractionVisible * WARP_BUBBLE_REFRACT * params[STRIDERFX_BUBBLE_REFRACT].x;
			float size = WARP_BUBBLE_SIZE * params[STRIDERFX_BUBBLE_SIZE].x;
			IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
			pVar->SetFloatValue( refract );

			CMatRenderContextPtr pRenderContext(materials);
			pRenderContext->Bind(pMat, (IClientRenderable*)this);
			QUA_Strider_DrawSprite( wideBeamEnd, size, white );
		}
	}
	else
	{
		// call this to have the check ready on the first frame
		pixelvis_queryparams_t endParams;
		endParams.Init(m_beamEndPosition, 4.0f, 0.001f);
		PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
	}
	if ( hasParam[STRIDERFX_AFTERGLOW_COLOR] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = AFTERGLOW_SIZE;// * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_AFTERGLOW_COLOR].x * gunFractionVisible;
		QUA_ScaleColor( color, white, bright );

		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->Bind(pMat, (IClientRenderable*)this);
		QUA_Strider_DrawSprite( m_worldPosition, size, color );

		dlight_t *dl = effects->CL_AllocDlight( m_entityIndex );
		dl->origin = m_worldPosition;
		dl->color.r = 40;
		dl->color.g = 60;
		dl->color.b = 255;
		dl->color.exponent = 5;
		dl->radius = bright * 128;
		dl->die = gpGlobals->curtime + 0.001;
	}

	if ( m_t >= STRIDERFX_END_ALL_TIME && !hasAny )
	{
		EffectShutdown();
	}
	return 1;
}




//-----------------------------------------------------------------------------
// Purpose: Strider class implementation
//-----------------------------------------------------------------------------
C_QUA_Strider::C_QUA_Strider():
m_iv_vecGunCrosshair( "C_QUA_Strider::m_iv_vecGunCrosshair" )
{
	AddVar( &m_vecHitPos, &m_iv_vecHitPos, LATCH_ANIMATION_VAR );

	memset(m_vecIKTarget, 0, sizeof(m_vecIKTarget));
	AddVar( &m_vecIKTarget, &m_iv_vecIKTarget, LATCH_ANIMATION_VAR );

	m_flNextRopeCutTime = 0;
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );
	m_ViewSmoothingData.pVehicle = this;
}

C_QUA_Strider::~C_QUA_Strider()
{
}

void C_QUA_Strider::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
	case STRIDER_MSG_STREAKS:
		{
			Vector	pos;
			msg.ReadBitVec3Coord( pos );
			m_cannonFX.SetRenderOrigin( pos );
			m_cannonFX.EffectInit( entindex(), LookupAttachment( "BigGun" ) );
			m_cannonFX.LimitTime( STRIDERFX_BIG_SHOT_TIME );
		}
		break;

	case STRIDER_MSG_BIG_SHOT:
		{
			Vector tmp;
			msg.ReadBitVec3Coord( tmp );
			m_cannonFX.SetTime( STRIDERFX_BIG_SHOT_TIME );
			m_cannonFX.LimitTime( STRIDERFX_END_ALL_TIME );
		}
		break;

	case STRIDER_MSG_DEAD:
		{
			m_cannonFX.EffectShutdown();
		}
		break;
	case STRIDER_MSG_MACHINEGUN:
		{
			// Necesario en MP, porque si no no se ven las balas.
			Vector muz,dir,spr;
			msg.ReadBitVec3Coord( muz );
			msg.ReadBitVec3Coord( dir );
			msg.ReadBitVec3Coord( spr );
			int ammo = msg.ReadByte();
			//Warning("x: %f,x: %f,x: %f,ammo: %i",muz.x,dir.x,spr.x,ammo);
			FireBulletsInfo_t info;
			info.m_iShots = 1;
			info.m_vecSrc = muz;
			info.m_vecDirShooting = dir;
			info.m_vecSpread = spr;
			info.m_pAttacker =	(C_BaseEntity *) m_hPlayer;
			info.m_flDistance = MAX_TRACE_LENGTH;
			info.m_iAmmoType =  ammo;
			info.m_flDamage = 75;
			info.m_iPlayerDamage = 150;
			info.m_iTracerFreq = 1;
			FireBullets(info);
			CEffectData data;
			data.m_nAttachmentIndex = LookupAttachment( "MiniGun" );
			data.m_hEntity = entindex();
			DispatchEffect("QUA_StriderMuzzleFlash",data );

		}
	}
}

void C_QUA_Strider::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;

	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
}

void C_QUA_Strider::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// We need to have our render bounds defined or shadow creation won't work correctly
		ClientThink();
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );

	m_cannonFX.Update( this, m_vecHitPos );
}

//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_QUA_Strider::ClientThink()
{
	int i;
	Vector vecMins, vecMaxs;
	Vector vecAbsMins, vecAbsMaxs;
	matrix3x4_t worldToStrider, hitboxToStrider;
	Vector vecBoxMins, vecBoxMaxs;
	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	mstudiohitboxset_t *set;
	CBoneCache *pCache = NULL;

	// The reason why this is here, as opposed to in SetObjectCollisionBox,
	// is because of IK. The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the Relink phase.

	studiohdr_t *pStudioHdr = GetModel()?modelinfo->GetStudiomodel( GetModel() ):NULL;
	if (!pStudioHdr)
		goto doneWithComputation;

	set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		goto doneWithComputation;

	CStudioHdr *hdr = GetModelPtr();
	pCache = GetBoneCache( hdr );

	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones );

//	// Compute a box in world space that surrounds this entity
	m_vecRenderMins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_vecRenderMaxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
//
	MatrixInvert( EntityToWorldTransform(), worldToStrider );
//
	for ( i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		ConcatTransforms( worldToStrider, *hitboxbones[pbox->bone], hitboxToStrider );

		TransformAABB( hitboxToStrider, pbox->bbmin, pbox->bbmax, vecBoxMins, vecBoxMaxs );
		VectorMin( m_vecRenderMins, vecBoxMins, m_vecRenderMins );
		VectorMax( m_vecRenderMaxs, vecBoxMaxs, m_vecRenderMaxs );
	}
//	// Deberiamos poner aqui lo de la vista??
//
//	// UNDONE: Disabled this until we can get closer to a final map and tune
//#if 0
//	// Cut ropes.
//	if ( gpGlobals->curtime >= m_flNextRopeCutTime )
//	{
//		// Blow the bbox out a little.
//		Vector vExtendedMins = vecMins - Vector( 50, 50, 50 );
//		Vector vExtendedMaxs = vecMaxs + Vector( 50, 50, 50 );
//
//		C_RopeKeyframe *ropes[512];
//		int nRopes = C_RopeKeyframe::GetRopesIntersectingAABB( ropes, ARRAYSIZE( ropes ), GetAbsOrigin() + vExtendedMins, GetAbsOrigin() + vExtendedMaxs );
//		for ( int i=0; i < nRopes; i++ )
//		{
//			C_RopeKeyframe *pRope = ropes[i];
//
//			if ( pRope->GetEndEntity() )
//			{
//				Vector vPos;
//				if ( pRope->GetEndPointPos( 1, vPos ) )
//				{
//					// Detach the endpoint.
//					pRope->SetEndEntity( NULL );
//					
//					// Make some spark effect here..
//					g_pEffects->Sparks( vPos );
//				}				
//			}
//		}
//
//		m_flNextRopeCutTime = gpGlobals->curtime + 0.5;
//	}
//#endif

doneWithComputation:	
	// True argument because the origin may have stayed the same, but the size is expected to always change
	g_pClientShadowMgr->AddToDirtyShadowList( this, true );
}


//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_QUA_Strider::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins = m_vecRenderMins;
	theMaxs = m_vecRenderMaxs;
}

//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
//void C_QUA_Strider::DrawHudElements( )
//{
//}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Strider muzzle flashes
//-----------------------------------------------------------------------------
void QUA_MuzzleFlash_Strider(ClientEntityHandle_t hEntity, int attachmentIndex)
{
	VPROF_BUDGET( "QUA_MuzzleFlash_Strider", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	matrix3x4_t	matAttachment;
	// If the client hasn't seen this entity yet, bail.
	if (!FX_GetAttachmentTransform(hEntity, attachmentIndex, matAttachment))
		return;

	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create("QUA_MuzzleFlash_Strider", hEntity, attachmentIndex);

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 3.0f, 4.0f );

	float burstSpeed = random->RandomFloat( 400.0f, 600.0f );

#define	FRONT_LENGTH 12

	// Front flash
	for ( int i = 1; i < FRONT_LENGTH; i++ )
	{
		offset = (forward * (i*2.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.1f;

		pParticle->m_vecVelocity = forward * burstSpeed;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255.0f;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * (FRONT_LENGTH-(i))/(FRONT_LENGTH*0.75f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
	
	Vector right(0,1,0), up(0,0,1);
	Vector dir = right - up;

#define	SIDE_LENGTH	8

	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Diagonal flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	dir = right + up;
	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Diagonal flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (-dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * -burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	dir = up;
	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Top flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/strider_muzzle" ), vec3_origin );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= random->RandomFloat( 0.3f, 0.4f );

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 0;

	pParticle->m_uchStartSize	= flScale * random->RandomFloat( 12.0f, 16.0f );
	pParticle->m_uchEndSize		= 0.0f;
	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= 0.0f;

	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );

	int entityIndex = ClientEntityList().HandleToEntIndex(hEntity);
	if (entityIndex >= 0)
	{
		dlight_t *el = effects->CL_AllocElight(LIGHT_INDEX_MUZZLEFLASH + entityIndex);

		el->origin = origin;

		el->color.r = 64;
		el->color.g = 128;
		el->color.b = 255;
		el->color.exponent = 5;

		el->radius = random->RandomInt(100, 150);
		el->decay = el->radius / 0.05f;
		el->die = gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void QUA_StriderMuzzleFlashCallback( const CEffectData &data )
{
	QUA_MuzzleFlash_Strider(data.m_hEntity, data.m_nAttachmentIndex);
}

DECLARE_CLIENT_EFFECT( "QUA_StriderMuzzleFlash", QUA_StriderMuzzleFlashCallback );

#define	BLOOD_MIN_SPEED	64.0f*4.0f
#define BLOOD_MAX_SPEED 256.0f*4.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//-----------------------------------------------------------------------------
void QUA_StriderBlood( const Vector &origin, const Vector &normal, float scale )
{
	VPROF_BUDGET( "QUA_StriderBlood", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	Vector	offset;
	
	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );

	Vector	color[3];
	color[0] = color[1] = color[2] = Vector( 1, 1, 1 );

	float	colorRamp;

	int i;
	float	flScale = scale / 8.0f;

	PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial("effects/slime1");

	float	length = 0.1f;
	Vector	vForward, vRight, vUp;
	Vector	offDir;

	TrailParticle	*tParticle;

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( origin );
	sparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );

	//Dump out drops
	for ( i = 0; i < 64; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= 1.0f;

		offDir = normal + RandomVector( -1.0f, 1.0f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( BLOOD_MIN_SPEED * flScale * 2.0f, BLOOD_MAX_SPEED * flScale * 2.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 8.0f, 32.0f ) * flScale;

		tParticle->m_flWidth		= random->RandomFloat( 12.0f, 16.0f ) * flScale;
		tParticle->m_flLength		= random->RandomFloat( length*0.5f, length ) * flScale;

		colorRamp = random->RandomFloat( 0.5f, 2.0f );

		int randomColor = random->RandomInt( 0, 2 );

		tParticle->m_color.r = min( 1.0f, color[randomColor].x * colorRamp ) * 255;
		tParticle->m_color.g = min( 1.0f, color[randomColor].y * colorRamp ) * 255;
		tParticle->m_color.b = min( 1.0f, color[randomColor].z * colorRamp ) * 255;
		tParticle->m_color.a = 255;
	}
}

//void C_QUA_Strider::OnEnteredVehicle( C_BasePlayer *pPlayer )
//{
//	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
//	Vector vehicleEyeOrigin;
//	QAngle vehicleEyeAngles;
//	GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

//	Vector up,forward;
//	this->GetVectors(&forward,NULL,&up);
//	vehicleEyeOrigin+=(forward*120)+(up*-10);

//	m_vecLastEyeTarget = vehicleEyeOrigin;
//	m_vecLastEyePos = vehicleEyeOrigin;
//	m_vecEyeSpeed = vec3_origin;
//}

void C_QUA_Strider::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	
		float pitchBounds[2] = { -65.0f, 65.0f };
		RestrictView( pitchBounds,NULL, NULL, pCmd->viewangles );
	
}
void C_QUA_Strider::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	// Cambiado. Para que el cliente vea.
	//int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	// Encontrado el bug de los Bones... ahora debemos mirar que hacemos.
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	//GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
	Vector up,forward;
	vehicleEyeOrigin=this->GetAbsOrigin();
	vehicleEyeAngles=this->GetAbsAngles();
	this->GetVectors(&forward,NULL,&up);
	vehicleEyeOrigin+=(forward*120)+(up*-10);

	// NO limitamos el yaw en el Strider, porque si no no vemos nada.
	// Limit the yaw.
	if ( pYawBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.y, vehicleEyeAngles.y );
		flAngleDiff = clamp( flAngleDiff, pYawBounds[0], pYawBounds[1] );
		vecViewAngles.y = vehicleEyeAngles.y + flAngleDiff;
	}

	// Limit the pitch.
	if ( pPitchBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.x, vehicleEyeAngles.x );
		flAngleDiff = clamp( flAngleDiff, pPitchBounds[0], pPitchBounds[1] );
		vecViewAngles.x = vehicleEyeAngles.x + flAngleDiff;
	}

	// Limit the roll.
	if ( pRollBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.z, vehicleEyeAngles.z );
		flAngleDiff = clamp( flAngleDiff, pRollBounds[0], pRollBounds[1] );
		vecViewAngles.z = vehicleEyeAngles.z + flAngleDiff;
	}
}
void C_QUA_Strider::DrawHudElements( )
{
	CHudTexture *pIcon;
	//int iIconX, iIconY;


		// draw crosshairs for vehicle gun
		pIcon = gHUD.GetIcon( "gunhair" );

		if ( pIcon != NULL )
		{
			float x, y;
			Vector screen;

			x = ScreenWidth()/2;
			y = ScreenHeight()/2;

		
			ScreenTransform( m_vecGunCrosshair, screen );
			x += 0.5 * screen[0] * ScreenWidth() + 0.5;
			y -= 0.5 * screen[1] * ScreenHeight() + 0.5;

			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
			bool unable;
			unable=false;
			Color	clr = ( unable ) ? gHUD.m_clrCaution : gHUD.m_clrNormal;
			pIcon->DrawSelf( x, y, clr );
		}

	// Aqui pondremos el crosshair
}


void C_QUA_Strider::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}
int	C_QUA_Strider::GetPassengerRole( C_BaseCombatCharacter *pEnt )
{
	if (m_hPlayer.Get() == pEnt)
	{
		return VEHICLE_ROLE_DRIVER;
	}
	return -1;
}

C_BaseCombatCharacter* C_QUA_Strider::GetPassenger( int nRole )
{
	if (nRole == VEHICLE_ROLE_DRIVER)
		return m_hPlayer.Get();
	return NULL;
}

void C_QUA_Strider::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV )
{
#ifndef pilotable2
	SharedVehicleViewSmoothing(m_hPlayer, pAbsOrigin, pAbsAngles, m_bEnterAnimOn, m_bExitAnimOn, m_vecEyeExitEndpoint, &m_ViewSmoothingData, NULL);
#else
	//Crash lol
	VehicleViewSmoothingSTR( m_hPlayer, pAbsOrigin, pAbsAngles, m_bEnterAnimOn, m_bExitAnimOn, &m_vecEyeExitEndpoint, &m_ViewSmoothingData, NULL );
#endif
}

//STRIDERRAGDOLL


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_StriderRagdoll, DT_StriderRagdoll, CStriderRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hStrider ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	RecvPropFloat( RECVINFO( m_fAltura ) ),
END_RECV_TABLE()



C_StriderRagdoll::C_StriderRagdoll()
{

}

C_StriderRagdoll::~C_StriderRagdoll()
{
	PhysCleanupFrictionSounds( this );

	if ( m_hStrider )
	{
		m_hStrider->CreateModelInstance();
	}
}

void C_StriderRagdoll::Interp_Copy( VarMapping_t *pDest, C_BaseAnimating *pDestinationEntity, C_BaseAnimating *pSourceEntity, VarMapping_t *pSrc )
{
	if ( !pDest || !pSrc )
		return;

	if ( pDest->m_Entries.Count() != pSrc->m_Entries.Count() )
	{
		return;
	}

	int c = pDest->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		pDest->m_Entries[ i ].watcher->Copy( pSrc->m_Entries[i].watcher );
	}

	VarMapping_t *varMap = GetVarMapping();

	Interp_Copy(varMap, pDestinationEntity, pSourceEntity, varMap);
}

void C_StriderRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght
				
		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		// Blood spray!
//		FX_CS_BloodSpray( hitpos, dir, 10 );
	}
}


void C_StriderRagdoll::CreateStriderRagdoll( void )
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_QUA_Strider *pStrider = dynamic_cast< C_QUA_Strider* >( m_hStrider.Get());
	
	if ( pStrider)
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pStrider->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		/*bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{*/
			Interp_Copy( varMap, this, pStrider, pStrider->C_BaseAnimating::GetVarMapping() );

			SetAbsAngles( pStrider->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pStrider->m_flAnimTime;
			SetSequence( pStrider->GetSequence() );
			m_flPlaybackRate = pStrider->GetPlaybackRate();
			SetPoseParameter(LookupPoseParameter("body_height"),200.0f);
		/*}
		else
		{*/
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
		//	SetAbsOrigin( m_vecRagdollOrigin );
		//	
		//	SetAbsAngles( pPlayer->GetRenderAngles() );

		//	SetAbsVelocity( m_vecRagdollVelocity );

		//	int iSeq = pPlayer->GetSequence();
		//	if ( iSeq == -1 )
		//	{
		//		Assert( false );	// missing walk_lower?
		//		iSeq = 0;
		//	}
		//	
		//	SetSequence( iSeq );	// walk_lower, basic pose
		//	SetCycle( 0.0 );

		//	Interp_Reset( varMap );
		//}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );
		SetSequence(LookupSequence("idle01"));
		SetPoseParameter(LookupPoseParameter("body_height"),m_fAltura);
	
		Interp_Reset( GetVarMapping() );
		
	}

	SetModelIndex( m_nModelIndex );
	//engine->Con_NPrintf(27,"Le volvemos a poner de altura: %f",m_fAltura);		
	SetPoseParameter(LookupPoseParameter("body_height"),200.0f);
	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	BecomeRagdollOnClient( /*false*/ ); //FixMe No? xd
}


void C_StriderRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateStriderRagdoll();
	
		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

		if( pPhysicsObject )
		{
			AngularImpulse aVelocity(0,0,0);

			Vector vecExaggeratedVelocity = 3 * m_vecRagdollVelocity;

			pPhysicsObject->AddVelocity( &vecExaggeratedVelocity, &aVelocity );
		}
	}
}

IRagdoll* C_StriderRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_StriderRagdoll::UpdateOnRemove( void )
{
	VPhysicsSetObject( NULL );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_StriderRagdoll::SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights)
{
	BaseClass::SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);

	//static float destweight[128];
	//static bool bIsInited = false;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	/*if (hdr->numflexdesc > 0)
	{
		if (!bIsInited)
		{
			int i;
			for (i = 0; i < 128; i++)
			{
				destweight[i] = 0.0f;
			}
			bIsInited = true;
		}
		//modelrender->SetFlexWeights( hdr->numflexdesc, destweight );
	}*/

	if (m_iEyeAttachment > 0)
	{
		matrix3x4_t attToWorld;
		if (GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget(hdr, GetBody(), tmp);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void QUA_StriderBloodCallback( const CEffectData &data )
{
	QUA_StriderBlood( data.m_vOrigin, data.m_vNormal, data.m_flScale );
}

DECLARE_CLIENT_EFFECT( "QUA_StriderBlood", QUA_StriderBloodCallback );
#endif
