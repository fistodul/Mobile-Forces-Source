//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "c_prop_vehicle.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "movevars_shared.h"
#include "iviewrender.h"
#include "vgui/ISurface.h"
#include "client_virtualreality.h"
#include "../hud_crosshair.h"
#include "sourcevr/isourcevirtualreality.h"
// NVNT haptic utils
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int ScreenTransform( const Vector& point, Vector& screen );

extern ConVar default_fov;
extern ConVar joy_response_move_vehicle;

IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleDriveable, DT_PropVehicleDriveable, CPropVehicleDriveable)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_nRPM ) ),
	RecvPropFloat( RECVINFO( m_flThrottle ) ),
	RecvPropInt( RECVINFO( m_nBoostTimeLeft ) ),
	RecvPropInt( RECVINFO( m_nHasBoost ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledWeapons ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledVehicle ) ),
	RecvPropInt( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropInt( RECVINFO( m_bExitAnimOn ) ),
	RecvPropInt( RECVINFO( m_bUnableToFire ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropBool( RECVINFO( m_bHasGun ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropVehicleDriveable )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()

ConVar r_VehicleViewClamp( "r_VehicleViewClamp", "1", FCVAR_CHEAT );

#define ROLL_CURVE_ZERO		20		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	90		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between


									
/*#ifdef pilotable
// remaps an angular variable to a 3 band function:
// 0 <= t < start :		f(t) = 0
// start <= t <= end :	f(t) = end * spline(( t-start) / (end-start) )  // s curve between clamped and linear
// end < t :			f(t) = t
float RemapAngleRange( float startInterval, float endInterval, float value, RemapAngleRange_CurvePart_t *peCurvePart )
{
	// Fixup the roll
	value = AngleNormalize( value );
	float absAngle = fabs(value);
	// beneath cutoff?
	if ( absAngle < startInterval )
	{
		if ( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Zero;
		}
		value = 0;
	}
	// in spline range?
	else if ( absAngle <= endInterval )
	{
		float newAngle = SimpleSpline( (absAngle - startInterval) / (endInterval-startInterval) ) * endInterval;
		// grab the sign from the initial value
		if ( value < 0 )
		{
			newAngle *= -1;
		}
		if ( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Spline;
		}
		value = newAngle;
	}
	// else leave it alone, in linear range
	else if ( peCurvePart )
	{
		*peCurvePart = RemapAngleRange_CurvePart_Linear;
	}
	return value;
}
#endif*/

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::C_PropVehicleDriveable() :
	m_iv_vecGunCrosshair( "C_PropVehicleDriveable::m_iv_vecGunCrosshair" )

{
	m_hPrevPlayer = NULL;

	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.bClampEyeAngles = true;
#ifdef SecobMod__HIGH_PING_VEHICLE_FIX
	m_ViewSmoothingData.bDampenEyePosition = false;
#else
	m_ViewSmoothingData.bDampenEyePosition = true;
#endif

	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;
	#ifdef pilotable
	m_ViewSmoothingData.bBlendAnglesAPC = true;
	#endif

	m_ViewSmoothingData.flFOV = m_flFOV = default_fov.GetFloat();

	AddVar( &m_vecGunCrosshair, &m_iv_vecGunCrosshair, LATCH_SIMULATION_VAR );
}

//-----------------------------------------------------------------------------
// Purpose: De-constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::~C_PropVehicleDriveable()
{
}


//-----------------------------------------------------------------------------
// By default all driveable vehicles use the curve defined by the convar.
//-----------------------------------------------------------------------------
int C_PropVehicleDriveable::GetJoystickResponseCurve() const
{
	return joy_response_move_vehicle.GetInt();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehicleDriveable::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleDriveable::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_hPlayer && !m_hPrevPlayer )
	{
		OnEnteredVehicle( m_hPlayer );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		g_ClientVirtualReality.AlignTorsoAndViewToWeapon();
	}
	else if ( !m_hPlayer && m_hPrevPlayer )
	{
		// NVNT notify haptics system of navigation exit
		OnExitedVehicle( m_hPrevPlayer );
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		m_ViewSmoothingData.bWasRunningAnim = false;
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

//-----------------------------------------------------------------------------
// Should this object cast render-to-texture shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_PropVehicleDriveable::ShadowCastType()
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	// Always use render-to-texture. We'll always the dirty bits in our think function
	return SHADOWS_RENDER_TO_TEXTURE;
}


//-----------------------------------------------------------------------------
// Mark the shadow as dirty while the vehicle is being driven
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::ClientThink( void )
{
	// The vehicle is always dirty owing to pose parameters while it's being driven.
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetShadowHandle() );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
{
	// MFS
	if (m_hPlayer->IsLocalPlayer())
	{
		SharedVehicleViewSmoothing(m_hPlayer,
			pAbsOrigin, pAbsAngles,
			m_bEnterAnimOn, m_bExitAnimOn,
			m_vecEyeExitEndpoint,
			&m_ViewSmoothingData,
			pFOV);
	}
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Simply used to return intensity value based upon current timer passed in
//-----------------------------------------------------------------------------
int GetFlashColorIntensity( int LowIntensity, int HighIntensity, bool Dimming, int Increment, int Timer )
{
	if ( Dimming ) 
		return ( HighIntensity - Timer * Increment );
	else
		return ( LowIntensity + Timer * Increment );
}

#define	TRIANGULATED_CROSSHAIR 1

void C_PropVehicleDriveable::DrawHudElements( )
{
	CHudTexture *pIcon;
	int iIconX, iIconY;

	if (m_bHasGun)
	{
		// draw crosshairs for vehicle gun
		pIcon = gHUD.GetIcon( "gunhair" );

		if ( pIcon != NULL )
		{
			float x, y;

			if( UseVR() )
			{
				C_BasePlayer *pPlayer = (C_BasePlayer *)GetPassenger( VEHICLE_ROLE_DRIVER );
				Vector vecStart, vecDirection;
				pPlayer->EyePositionAndVectors( &vecStart, &vecDirection, NULL, NULL );
				Vector vecEnd = vecStart + vecDirection * MAX_TRACE_LENGTH;

				trace_t tr;
				UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

				Vector screen;
				screen.Init();
				ScreenTransform(tr.endpos, screen);

				int vx, vy, vw, vh;
				vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

				float screenWidth = vw;
				float screenHeight = vh;

				x = 0.5f * ( 1.0f + screen[0] ) * screenWidth + 0.5f;
				y = 0.5f * ( 1.0f - screen[1] ) * screenHeight + 0.5f;
			}
			else
			{
				Vector screen;

				x = ScreenWidth()/2;
				y = ScreenHeight()/2;

#if TRIANGULATED_CROSSHAIR
				ScreenTransform( m_vecGunCrosshair, screen );
				x += 0.5 * screen[0] * ScreenWidth() + 0.5;
				y -= 0.5 * screen[1] * ScreenHeight() + 0.5;
#endif
			}


			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
			
			Color	clr = ( m_bUnableToFire ) ? gHUD.m_clrCaution : gHUD.m_clrNormal;
			pIcon->DrawSelf( x, y, clr );
		}

		if ( m_nScannerDisabledWeapons )
		{
			// Draw icons for scanners "weps disabled"  
			pIcon = gHUD.GetIcon( "dmg_bio" );
			if ( pIcon )
			{
				iIconY = 467 - pIcon->Height() / 2;
				iIconX = 385;
				if ( !m_bScannerWepIcon )
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
					m_bScannerWepIcon = true;
					m_iScannerWepFlashTimer = 0;
					m_bScannerWepDim = true;
				}
				else
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerWepDim, 10, m_iScannerWepFlashTimer), 255 ) );
					m_iScannerWepFlashTimer++;
					m_iScannerWepFlashTimer %= 20;
					if(!m_iScannerWepFlashTimer)
						m_bScannerWepDim ^= 1;
				}
			}
		}
	}

	if ( m_nScannerDisabledVehicle )
	{
		// Draw icons for scanners "vehicle disabled"  
		pIcon = gHUD.GetIcon( "dmg_bio" );
		if ( pIcon )
		{
			iIconY = 467 - pIcon->Height() / 2;
			iIconX = 410;
			if ( !m_bScannerVehicleIcon )
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
				m_bScannerVehicleIcon = true;
				m_iScannerVehicleFlashTimer = 0;
				m_bScannerVehicleDim = true;
			}
			else
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerVehicleDim, 10, m_iScannerVehicleFlashTimer), 255 ) );
				m_iScannerVehicleFlashTimer++;
				m_iScannerVehicleFlashTimer %= 20;
				if(!m_iScannerVehicleFlashTimer)
					m_bScannerVehicleDim ^= 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_VehicleViewClamp.GetInt() )
	{
		float pitchBounds[2] = { -85.0f, 25.0f };
		RestrictView( NULL, pitchBounds, NULL, pCmd->viewangles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnEnteredVehicle( C_BaseCombatCharacter *pPassenger )
{
#if defined( WIN32 ) && !defined( _X360 )
	// NVNT notify haptics system of navigation change
	HapticsEnteredVehicle(this,pPassenger);
#endif
}

/*#ifdef pilotable
//=============================================================================================
// VEHICLE VIEW SMOOTHING. See iclientvehicle.h for details.
//=============================================================================================
//-----------------------------------------------------------------------------
// Purpose: For a given degree of freedom, blends between the raw and clamped
//			view depending on this vehicle's preferences. When vehicles wreck
//			catastrophically, it's often better to lock the view for a little
//			while until things settle down than to keep trying to clamp/flatten
//			the view artificially because we can never really catch up with
//			the chaotic flipping.
//-----------------------------------------------------------------------------
float ApplyViewLocking( float flAngleRaw, float flAngleClamped, ViewLockData_t &lockData, RemapAngleRange_CurvePart_t eCurvePart )
{
	// If we're set up to never lock this degree of freedom, return the clamped value.
	if ( lockData.flLockInterval == 0 )
		return flAngleClamped;
	float flAngleOut = flAngleClamped;
	// Lock the view if we're in the linear part of the curve, and keep it locked
	// until some duration after we return to the flat (zero) part of the curve.
	if ( ( eCurvePart == RemapAngleRange_CurvePart_Linear ) ||
		 ( lockData.bLocked && ( eCurvePart == RemapAngleRange_CurvePart_Spline ) ) )
	{
		//Msg( "LOCKED\n" );
		lockData.bLocked = true;
		lockData.flUnlockTime = gpGlobals->curtime + lockData.flLockInterval;
		flAngleOut = flAngleRaw;
	}
	else
	{
		if ( ( lockData.bLocked ) && ( gpGlobals->curtime > lockData.flUnlockTime ) )
		{
			lockData.bLocked = false;
			if ( lockData.flUnlockBlendInterval > 0 )
			{
				lockData.flUnlockTime = gpGlobals->curtime;
			}
			else
			{
				lockData.flUnlockTime = 0;
			}
		}
		if ( !lockData.bLocked )
		{
			if ( lockData.flUnlockTime != 0 )
			{
				// Blend out from the locked raw view (no remapping) to a remapped view.
				float flBlend = RemapValClamped( gpGlobals->curtime - lockData.flUnlockTime, 0, lockData.flUnlockBlendInterval, 0, 1 );
				//Msg( "BLEND %f\n", flBlend );
				flAngleOut = Lerp( flBlend, flAngleRaw, flAngleClamped );
				if ( flBlend >= 1.0f )
				{
					lockData.flUnlockTime = 0;
				}
			}
			else
			{
				// Not blending out from a locked view to a remapped view.
				//Msg( "CLAMPED\n" );
				flAngleOut = flAngleClamped;
			}
		}
		else
		{
			//Msg( "STILL LOCKED\n" );
			flAngleOut = flAngleRaw;
		}
	}
	return flAngleOut;
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pData - 
//			vehicleEyeAngles - 
//-----------------------------------------------------------------------------
void RemapViewAngles( ViewSmoothingData_t *pData, QAngle &vehicleEyeAngles )
{
	QAngle vecEyeAnglesRemapped;
	
	// Clamp pitch.
	RemapAngleRange_CurvePart_t ePitchCurvePart;
	vecEyeAnglesRemapped.x = RemapAngleRange( pData->flPitchCurveZero, pData->flPitchCurveLinear, vehicleEyeAngles.x, &ePitchCurvePart );
	vehicleEyeAngles.z = vecEyeAnglesRemapped.z = AngleNormalize( vehicleEyeAngles.z );
	// Blend out the roll dampening as our pitch approaches 90 degrees, to avoid gimbal lock problems.
	float flBlendRoll = 1.0;
	if ( fabs( vehicleEyeAngles.x ) > 60 )
	{
		flBlendRoll = RemapValClamped( fabs( vecEyeAnglesRemapped.x ), 60, 80, 1, 0);
	}
	RemapAngleRange_CurvePart_t eRollCurvePart;
	float flRollDamped = RemapAngleRange( pData->flRollCurveZero, pData->flRollCurveLinear, vecEyeAnglesRemapped.z, &eRollCurvePart );
	vecEyeAnglesRemapped.z = Lerp( flBlendRoll, vecEyeAnglesRemapped.z, flRollDamped );
	//Msg("PITCH ");
	vehicleEyeAngles.x = ApplyViewLocking( vehicleEyeAngles.x, vecEyeAnglesRemapped.x, pData->pitchLockData, ePitchCurvePart );
	//Msg("ROLL ");
	vehicleEyeAngles.z = ApplyViewLocking( vehicleEyeAngles.z, vecEyeAnglesRemapped.z, pData->rollLockData, eRollCurvePart );
}
#endif*/

// NVNT - added function
void C_PropVehicleDriveable::OnExitedVehicle( C_BaseCombatCharacter *pPassenger )
{
#if defined( WIN32 ) && !defined( _X360 )
	// NVNT notify haptics system of navigation exit
	HapticsExitedVehicle(this,pPassenger);
#endif
}

