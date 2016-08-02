//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Satchel Charge
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	HOLDOUT_H
#define	HOLDOUT_H

#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"
#include "hl2mp/weapon_slam.h"

class CSoundPatch;
class CSprite;

class CHoldout : public CBaseGrenade
{
public:
	DECLARE_CLASS( CHoldout, CBaseGrenade );
	DECLARE_SERVERCLASS();  // make this entity networkable

	void			Spawn( void );
	void			Precache( void );
	void			HoldoutUse(CBaseEntity *pPlayer, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void			HoldoutThink( void );

	int			m_Owner;
	CNetworkVar( float, m_RedTime );
	CNetworkVar( float, m_BlueTime );

	CHoldout();
	~CHoldout();

	DECLARE_DATADESC();

private:
	void				CreateEffects( void );
	CHandle<CSprite>	m_hGlowSprite;
};

#endif	//HOLDOUT_H
