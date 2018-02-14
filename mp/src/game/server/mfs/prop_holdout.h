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

//#include "basegrenade_shared.h"
#include "hl2mp/weapon_slam.h"

//class CSoundPatch;
//class CSprite;

class CHoldout : public CBaseToggle
{
public:
	DECLARE_CLASS(CHoldout, CBaseToggle);

	void			Spawn( void );
	void			Precache( void );
	void			HoldoutUse(CBaseEntity *pPlayer, CBaseEntity *pCaller, USE_TYPE useType, float value);
	/*virtual */void			HoldoutThink(void);

	int			m_Owner;

	CHoldout();
	~CHoldout();

	DECLARE_DATADESC();

private:
	void				CreateEffects( void );
	CHandle<CSprite>	m_hGlowSprite;
};

#endif	//HOLDOUT_H
