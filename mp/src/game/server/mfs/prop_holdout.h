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

	void			Spawn( void );
	void			Precache( void );
	void			HoldoutUse( CBasePlayer *pPlayer );
	void			HoldoutThink( void );

	int			m_Owner;
	float			m_RedTime;
	float			m_BlueTime;

public:
	CHoldout();
	~CHoldout();

	DECLARE_DATADESC();

private:
	void				CreateEffects( void );
	CHandle<CSprite>	m_hGlowSprite;
};

#endif	//HOLDOUT_H
