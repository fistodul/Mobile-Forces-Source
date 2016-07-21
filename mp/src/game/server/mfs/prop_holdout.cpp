//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "Sprite.h"
#include "game.h"
#include "team.h"
#include "prop_holdout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	HOLDOUT_SPRITE	"sprites/redglow1.vmt"

BEGIN_DATADESC( CHoldout )

	DEFINE_FIELD( m_Owner, FIELD_INTEGER ),

	// Function Pointers
//	DEFINE_FUNCTION( HoldoutTouch ),
	DEFINE_THINKFUNC( HoldoutThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_holdout, CHoldout );

void CHoldout::Spawn( void )
{
	if ( HL2MPRules()->IsHoldout() == false )
	return;
	Precache( );
	SetModel( "models/Alyx.mdl" );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );

	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetTouch( &CHoldout::HoldoutTouch );
	SetThink( &CHoldout::HoldoutThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_hGlowSprite = NULL;
	CreateEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Start up any effects for us
//-----------------------------------------------------------------------------
void CHoldout::CreateEffects( void )
{
	// Only do this once
	if ( m_hGlowSprite != NULL )
		return;

	// Create a blinking light to show we're an active SLAM
	m_hGlowSprite = CSprite::SpriteCreate( HOLDOUT_SPRITE, GetAbsOrigin(), false );
	m_hGlowSprite->SetAttachment( this, 0 );
	m_hGlowSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxStrobeFast );
	m_hGlowSprite->SetBrightness( 255, 1.0f );
	m_hGlowSprite->SetScale( 0.2f, 0.5f );
	m_hGlowSprite->TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CHoldout::HoldoutTouch(CBaseEntity *pOther)
{
	CBasePlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
	if (pPlayer)
	{
		if (m_Owner != pPlayer->GetTeamNumber())
			m_Owner = pPlayer->GetTeamNumber();
	}
}

void CHoldout::HoldoutThink( void )
{
	CHL2MPRules *pRules = HL2MPRules();
	if (m_Owner != NULL)
	{
		if (m_Owner == 2)
		{
			if (m_BlueTime < hold_time.GetInt())
			{
				m_BlueTime = m_BlueTime + 0.1f;
			}
			else
			{
				pRules->GoToIntermission();
			}
		}
		else
		{
			if (m_RedTime < hold_time.GetInt())
			{
				m_RedTime = m_RedTime + 0.1f;
			}
			else
			{
				pRules->GoToIntermission();
			}
		}
	}
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CHoldout::Precache( void )
{
	PrecacheModel("models/Alyx.mdl");
	PrecacheModel(HOLDOUT_SPRITE);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CHoldout::CHoldout(void)
{
	m_Owner = NULL;
	m_RedTime = 0;
	m_BlueTime = 0;
}

CHoldout::~CHoldout(void)
{
	if ( m_hGlowSprite != NULL )
	{
		UTIL_Remove( m_hGlowSprite );
		m_hGlowSprite = NULL;
	}
}
