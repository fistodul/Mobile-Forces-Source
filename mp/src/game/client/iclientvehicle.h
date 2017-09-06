//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICLIENTVEHICLE_H
#define ICLIENTVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"

class C_BasePlayer;
class Vector;
class QAngle;
class C_BaseEntity;


//-----------------------------------------------------------------------------
// Purpose: All client vehicles must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientVehicle : public IVehicle
{
public:
	// When a player is in a vehicle, here's where the camera will be
	virtual void GetVehicleFOV( float &flFOV ) = 0;

	// Allows the vehicle to restrict view angles, blend, etc.
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) = 0;

	// Hud redraw...
	virtual void DrawHudElements() = 0;

	// Is this predicted?
	virtual bool IsPredicted() const = 0;

	// Get the entity associated with the vehicle.
	virtual C_BaseEntity *GetVehicleEnt() = 0;

	// Allows the vehicle to change the near clip plane
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const = 0;
	
	// Allows vehicles to choose their own curves for players using joysticks
	virtual int GetJoystickResponseCurve() const = 0;

#ifdef HL2_CLIENT_DLL
	// Ammo in the vehicles
	virtual int GetPrimaryAmmoType() const = 0;
	virtual int GetPrimaryAmmoClip() const = 0;
	virtual bool PrimaryAmmoUsesClips() const = 0;
	virtual int GetPrimaryAmmoCount() const = 0;
#endif
};

/*#ifdef pilotable
//==========================================================================================
// VEHICLE VIEW SMOOTHING CODE
//==========================================================================================

// If we enter the linear part of the remap for curve for any degree of freedom, we can lock
// that DOF (stop remapping). This is useful for making flips feel less spastic as we oscillate
// randomly between different parts of the remapping curve.
struct ViewLockData_t
{
	float	flLockInterval;			// The duration to lock the view when we lock it for this degree of freedom.
									// 0 = never lock this degree of freedom.

	bool	bLocked;				// True if this DOF was locked because of the above condition.

	float	flUnlockTime;			// If this DOF is locked, the time when we will unlock it.

	float	flUnlockBlendInterval;	// If this DOF is locked, how long to spend blending out of the locked view when we unlock.
};
#endif*/

#endif // ICLIENTVEHICLE_H
