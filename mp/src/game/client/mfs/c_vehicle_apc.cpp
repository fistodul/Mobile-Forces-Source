//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#ifdef pilotable
#include "c_prop_vehicle.h"

#include "movevars_shared.h"
#include "view.h"
#include "flashlighteffect.h"
#include "c_baseplayer.h"
#include "c_te_effect_dispatch.h"

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

#include "view_scene.h"
#include "materialsystem/imaterialvar.h"
#include "simple_keys.h"
#include "fx_envelope.h"
#include "iclientvehicle.h"
#include "engine/ivdebugoverlay.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"

#include "view.h"
#include "tier0/vprof.h"
#include "ClientEffectPrecacheSystem.h"
#include <bitbuf.h>
#include "fx_water.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar default_fov;

ConVar r_APCViewBlendTo( "r_APCViewBlendTo", "1", FCVAR_CHEAT );
ConVar r_APCViewBlendToScale( "r_APCViewBlendToScale", "0.03", FCVAR_CHEAT );
ConVar r_APCViewBlendToTime( "r_APCViewBlendToTime", "1.5", FCVAR_CHEAT );
ConVar r_APCFOV( "r_APCFOV", "90", FCVAR_CHEAT );

#define JEEP_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define JEEP_FRAMETIME_MIN		1e-6
#define JEEP_HEADLIGHT_DISTANCE 1000

#define APC_MSG_MACHINEGUN				1

//=============================================================================
//
// Client-side Jeep Class
//
class C_PropAPC : public C_PropVehicleDriveable
{

	DECLARE_CLASS( C_PropAPC, C_PropVehicleDriveable );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_PropAPC();
	~C_PropAPC();

public:
	
	virtual int		GetHealth() const { return m_iHealth; };
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	/*void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );*/

	/*void OnEnteredVehicle( C_BasePlayer *pPlayer );
	*/void Simulate( void );
	void GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles );
	bool IsPredicted() const { return true; }
	virtual void ReceiveMessage( int classID, bf_read &msg );
	virtual int GetPrimaryAmmoCount() const { return m_iAmmoCount; }
	virtual int GetPrimaryAmmoClip() const  { return m_iCannonCount; }
	
private:
	/*void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );*/

private:
	int						m_iAmmoCount;
	int						m_iCannonCount;
	/*Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;
	
	float		m_flViewAngleDeltaTime;

	float		m_flJeepFOV;
	CHeadlightEffect *m_pHeadlight;
	bool		m_bHeadlightIsOn;*/
};

IMPLEMENT_CLIENTCLASS_DT( C_PropAPC, DT_PropAPC, CPropAPC )
	RecvPropInt		(RECVINFO(m_iHealth)),
	RecvPropInt		(RECVINFO(m_iAmmoCount)),
	RecvPropInt		(RECVINFO(m_iCannonCount)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropAPC::C_PropAPC()
{
	//m_vecEyeSpeed.Init();
	//m_flViewAngleDeltaTime = 0.0f;
	////m_pHeadlight = NULL;
	m_ViewSmoothingData.flFOV = r_APCFOV.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
C_PropAPC::~C_PropAPC()
{
	/*if ( m_pHeadlight )
	{
		delete m_pHeadlight;
	}*/
}

void C_PropAPC::Simulate( void )
{
	//DevMsg("SIMULADO");
	BaseClass::Simulate();
}
void C_PropAPC::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;
	
	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
}
//-----------------------------------------------------------------------------
// Purpose: Blend view angles.
//-----------------------------------------------------------------------------
//void C_PropAPC::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
//{
//	if ( r_APCViewBlendTo.GetInt() )
//	{
//		// Check to see if the mouse has been touched in a bit or that we are not throttling.
//		if ( ( pCmd->mousedx != 0 || pCmd->mousedy != 0 ) || ( fabsf( m_flThrottle ) < 0.01f ) )
//		{
//			m_flViewAngleDeltaTime = 0.0f;
//		}
//		else
//		{
//			m_flViewAngleDeltaTime += gpGlobals->frametime;
//		}
//
//		if ( m_flViewAngleDeltaTime > r_APCViewBlendToTime.GetFloat() )
//		{
//			// Blend the view angles.
//			int eyeAttachmentIndex = LookupAttachment( "cannon_muzzle" );
//			Vector vehicleEyeOrigin;
//			QAngle vehicleEyeAngles;
//			GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
//			
//			QAngle outAngles;
//			InterpolateAngles( pCmd->viewangles, vehicleEyeAngles, outAngles, r_APCViewBlendToScale.GetFloat() );
//			pCmd->viewangles = outAngles;
//		}
//	}
//
//	BaseClass::UpdateViewAngles( pLocalPlayer, pCmd );
//}
//
////-----------------------------------------------------------------------------
//// Purpose:
////-----------------------------------------------------------------------------
//void C_PropAPC::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
//{
//#ifdef HL2_DLL
//	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
//	float flFrameTime = gpGlobals->frametime;
//
//	if ( flFrameTime < JEEP_FRAMETIME_MIN )
//	{
//		vecVehicleEyePos = m_vecLastEyePos;
//		DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, 0.0f );
//		return;
//	}
//
//	// Keep static the sideways motion.
//	// Dampen forward/backward motion.
//	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );
//
//	// Blend up/down motion.
//	DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );
//#endif
//}
//
//
////-----------------------------------------------------------------------------
//// Use the controller as follows:
//// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
////-----------------------------------------------------------------------------
//void C_PropAPC::ComputePDControllerCoefficients( float *pCoefficientsOut,
//												  float flFrequency, float flDampening,
//												  float flDeltaTime )
//{
//	float flKs = 9.0f * flFrequency * flFrequency;
//	float flKd = 4.5f * flFrequency * flDampening;
//
//	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );
//
//	pCoefficientsOut[0] = flKs * flScale;
//	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
//}
// 
////-----------------------------------------------------------------------------
//// Purpose:
////-----------------------------------------------------------------------------
//void C_PropAPC::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
//{
//	Vector vecPredEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;
//	Vector vecPredEyeSpeed = m_vecEyeSpeed;
//    Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
//	m_vecLastEyeTarget = vecVehicleEyePos;
//	if (vecVehicleEyeSpeed.Length() == 0.0)
//		return;
//
//	// Calculate the delta between the predicted eye position and speed and the current eye position and speed.
//	Vector vecDeltaSpeed = vecVehicleEyeSpeed - vecPredEyeSpeed;
//	Vector vecDeltaPos = vecVehicleEyePos - vecPredEyePos;
//
//	// Forward vector.
//	Vector vecForward;
//	AngleVectors( vecVehicleEyeAngles, &vecForward );
//
//	float flDeltaLength = vecDeltaPos.Length();
//	if ( flDeltaLength > JEEP_DELTA_LENGTH_MAX )
//	{
//		// Clamp.
//		float flDelta = flDeltaLength - JEEP_DELTA_LENGTH_MAX;
//		if ( flDelta > 40.0f )
//		{
//			// This part is a bit of a hack to get rid of large deltas (at level load, etc.).
//			m_vecLastEyePos = vecVehicleEyePos;
//			m_vecEyeSpeed = vecVehicleEyeSpeed;
//		}
//		else
//		{
//			// Position clamp.
//			float flRatio = JEEP_DELTA_LENGTH_MAX / flDeltaLength;
//			vecDeltaPos *= flRatio;
//			Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
//			vecVehicleEyePos -= vecForwardOffset;
//			m_vecLastEyePos = vecVehicleEyePos;
//
//			// Speed clamp.
//			vecDeltaSpeed *= flRatio;
//			float flCoefficients[2];
//			ComputePDControllerCoefficients( flCoefficients, r_APCViewDampenFreq.GetFloat(), r_APCViewDampenDamp.GetFloat(), flFrameTime );
//			m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
//		}
//	}
//	else
//	{
//		// Generate an updated (dampening) speed for use in next frames position prediction.
//		float flCoefficients[2];
//		ComputePDControllerCoefficients( flCoefficients, r_APCViewDampenFreq.GetFloat(), r_APCViewDampenDamp.GetFloat(), flFrameTime );
//		m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
//		
//		// Save off data for next frame.
//		m_vecLastEyePos = vecPredEyePos;
//		
//		// Move eye forward/backward.
//		Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
//		vecVehicleEyePos -= vecForwardOffset;
//	}
//}
//
////-----------------------------------------------------------------------------
//// Purpose:
////-----------------------------------------------------------------------------
//void C_PropAPC::DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
//{
//	// Get up vector.
//	Vector vecUp;
//	AngleVectors( vecVehicleEyeAngles, NULL, NULL, &vecUp );
//	vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
//	vecVehicleEyePos.z += r_APCViewZHeight.GetFloat() * vecUp.z;
//
//	// NOTE: Should probably use some damped equation here.
//}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void C_PropAPC::OnEnteredVehicle( C_BasePlayer *pPlayer )
//{
////	int eyeAttachmentIndex = LookupAttachment( "cannon_muzzle" );
////	Vector vehicleEyeOrigin;
////	QAngle vehicleEyeAngles;
////	GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
////
////	m_vecLastEyeTarget = vehicleEyeOrigin;
////	m_vecLastEyePos = vehicleEyeOrigin;
////	m_vecEyeSpeed = vec3_origin;
////	m_bEnterAnimOn=false;
//}
void C_PropAPC::ReceiveMessage( int classID, bf_read &msg )
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
	case APC_MSG_MACHINEGUN:
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
			info.m_iTracerFreq = 1;
			FireBullets(info);
			CEffectData data;
			data.m_hEntity = entindex();
			data.m_nAttachmentIndex = LookupAttachment( "muzzle" );
			//data.m_flScale = 1.0f;
			DispatchEffect( "QUA_APCMuzzleFlash", data );
		/*	CEffectData data;
			data.m_nAttachmentIndex = LookupAttachment( "MiniGun" );
			data.m_hEntity = entindex();
			DispatchEffect("QUA_StriderMuzzleFlash",data );*/

		}
	}
}

void QUA_MuzzleFlash_APC(ClientEntityHandle_t hEntity, int attachmentIndex)
{
	VPROF_BUDGET( "QUA_MuzzleFlash_APC", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	matrix3x4_t	matAttachment;
	// If the client hasn't seen this entity yet, bail.
	if (!FX_GetAttachmentTransform(hEntity, attachmentIndex, matAttachment))
		return;
	
	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create("MuzzleFlash", hEntity, attachmentIndex);

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 2.5f, 4.5f );

	// Flash
	for ( int i = 1; i < 7; i++ )
	{
		offset = (forward * (i*2.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random->RandomFloat( 0.05f, 0.1f );

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * (10-(i))/7) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
	
	// Grab the origin out of the transform for the attachment
	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );	

	
	int entityIndex = ClientEntityList().HandleToEntIndex(hEntity);
	if (entityIndex >= 0)
	{
		dlight_t *el = effects->CL_AllocElight(LIGHT_INDEX_MUZZLEFLASH + entityIndex);

		el->origin = origin;

		el->color.r = 255;
		el->color.g = 192;
		el->color.b = 64;
		el->color.exponent = 6;

		el->radius = random->RandomInt(128, 256);
		el->decay = el->radius / 0.05f;
		el->die = gpGlobals->curtime + 0.1f;
	}
}

void C_PropAPC::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{	
	VehicleViewSmoothingAPC( m_hPlayer, pAbsOrigin, pAbsAngles, m_bEnterAnimOn, m_bExitAnimOn, &m_vecEyeExitEndpoint, &m_ViewSmoothingData, NULL );
}

void QUA_APCMuzzleFlashCallback( const CEffectData &data )
{
	QUA_MuzzleFlash_APC(data.m_hEntity, data.m_nAttachmentIndex);
}

DECLARE_CLIENT_EFFECT( "QUA_APCMuzzleFlash", QUA_APCMuzzleFlashCallback );
#endif
