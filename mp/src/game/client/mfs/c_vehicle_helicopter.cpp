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


#define HELICOPTER_MSG_MACHINEGUN				1

extern ConVar default_fov;

class C_QUA_helicopter : public C_BaseAnimating, public IClientVehicle
{
	DECLARE_CLASS( C_QUA_helicopter, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();
	DECLARE_DATADESC();

	C_QUA_helicopter();
	~C_QUA_helicopter();
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual void ReceiveMessage( int classID, bf_read &msg );

public:
	
	virtual void GetVehicleFOV( float &flFOV ) { flFOV = 0.0f; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) {}
	virtual C_BaseCombatCharacter* GetPassenger( int nRole );
	virtual int	GetPassengerRole(C_BaseCombatCharacter *pEnt);
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;
	virtual int GetPrimaryAmmoType() const { return -1; }
	virtual int GetPrimaryAmmoCount() const { return m_iAmmoCount; }
	virtual int GetPrimaryAmmoClip() const  { return m_iCannonCount; }
	virtual bool PrimaryAmmoUsesClips() const { return false; }
	// IClientVehicle overrides.
	void GetVehicleViewPosition(int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL);
	void RestrictView( float *pYawBounds, float *pPitchBounds, float *pRollBounds, QAngle &vecViewAngles );
	virtual int		GetHealth() const { return m_iHealth; };
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
	int GetJoystickResponseCurve() const;

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

	Vector						m_vecGunCrosshair;
	CInterpolatedVar<Vector>	m_iv_vecGunCrosshair;
};
IMPLEMENT_CLIENTCLASS_DT(C_QUA_helicopter, DT_QUA_helicopter, QUA_helicopter)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
	RecvPropInt		(RECVINFO(m_iHealth)),
	RecvPropInt		(RECVINFO(m_iAmmoCount)),
	RecvPropInt		(RECVINFO(m_iCannonCount)),
END_RECV_TABLE()

BEGIN_DATADESC( C_QUA_helicopter )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()

extern ConVar joy_response_move_vehicle;

C_QUA_helicopter::C_QUA_helicopter()
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );
	m_ViewSmoothingData.pVehicle = this;
}

C_QUA_helicopter::~C_QUA_helicopter()
{
}
C_BaseCombatCharacter* C_QUA_helicopter::GetPassenger( int nRole )
{
	if (nRole == VEHICLE_ROLE_DRIVER)
		return m_hPlayer.Get();
	return NULL;
}

//-----------------------------------------------------------------------------
// By default all driveable vehicles use the curve defined by the convar.
//-----------------------------------------------------------------------------
int C_QUA_helicopter::GetJoystickResponseCurve() const
{
	return joy_response_move_vehicle.GetInt();
}

void C_QUA_helicopter::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

int	C_QUA_helicopter::GetPassengerRole( C_BaseCombatCharacter *pEnt )
{
	if (m_hPlayer.Get() == pEnt)
	{
		return VEHICLE_ROLE_DRIVER;
	}
	return -1;
}

void C_QUA_helicopter::DrawHudElements( )
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

void C_QUA_helicopter::ReceiveMessage( int classID, bf_read &msg )
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
	case HELICOPTER_MSG_MACHINEGUN:
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
			Vector vectorspr;
			QAngle no;
			this->GetAttachment(LookupAttachment("Muzzle"),vectorspr,no);
			info.m_vecSrc = vectorspr;
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
			data.m_nEntIndex = entindex();
			DispatchEffect("QUA_StriderMuzzleFlash",data );*/

		}
	}
}


void C_QUA_helicopter::GetVehicleViewPosition(int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV)
{
	VehicleViewSmoothingHLC( m_hPlayer, pAbsOrigin, pAbsAngles, m_bEnterAnimOn, m_bExitAnimOn, &m_vecEyeExitEndpoint, &m_ViewSmoothingData, NULL );
}
void C_QUA_helicopter::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	int eyeAttachmentIndex = LookupAttachment( "Bomb" );
	
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	QAngle no;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, no );
	Vector up,forward,right;
	this->GetVectors(&forward,&right,&up);
	vehicleEyeOrigin+=(forward*218)+(up*-10);
	vehicleEyeAngles=this->GetAbsAngles();
	// Limit the yaw.
	if ( pYawBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.y, vehicleEyeAngles.y );
		flAngleDiff = clamp( flAngleDiff, pYawBounds[0], pYawBounds[1] );
		vecViewAngles.y = vehicleEyeAngles.y + flAngleDiff;
	}

	// Limit the pitch.
	/*if ( pPitchBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.x, vehicleEyeAngles.x );
		flAngleDiff = clamp( flAngleDiff, pPitchBounds[0], pPitchBounds[1] );
		vecViewAngles.x = vehicleEyeAngles.x + flAngleDiff;
	}*/

	// Limit the roll.
	if ( pRollBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.z, vehicleEyeAngles.z );
		flAngleDiff = clamp( flAngleDiff, pRollBounds[0], pRollBounds[1] );
		vecViewAngles.z = vehicleEyeAngles.z + flAngleDiff;
	}
}

void C_QUA_helicopter::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	float flTracerDist;
	Vector vecDir;
	Vector vecEndPos;

	vecDir = tr.endpos - vecTracerSrc;
	
	flTracerDist = VectorNormalize( vecDir );

	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "StriderTracer" );
}
#endif
