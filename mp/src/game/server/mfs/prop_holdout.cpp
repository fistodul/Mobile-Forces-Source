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

#define	HOLDOUT_SPRITE	"sprites/blueglow1.vmt"
#define	HOLDOUT_SPRITE2	"sprites/redglow1.vmt"

BEGIN_DATADESC( CHoldout )

	DEFINE_FIELD( m_Owner, FIELD_INTEGER ),

	// Function Pointers
	//DEFINE_USEFUNC( HoldoutUse ),
	DEFINE_THINKFUNC( HoldoutThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_holdout, CHoldout );

void CHoldout::Spawn( void )
{
#ifdef MFS
if (HL2MPRules()->IsHoldout() == false)
{
return; //Why tf u no work
}
#endif
	Precache( );
	SetModel( "models/props/holdout.mdl" );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );

	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	//UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetUse( &CHoldout::HoldoutUse );
	SetThink( &CHoldout::HoldoutThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_hGlowSprite = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Start up any effects for us
//-----------------------------------------------------------------------------
void CHoldout::CreateEffects( void )
{
	// Dont overlap
	if ( m_hGlowSprite != NULL )
		return;

	if ( m_Owner == 2 )
	{
		// Create a blue blinking light to show the owner team
		m_hGlowSprite = CSprite::SpriteCreate( HOLDOUT_SPRITE, GetAbsOrigin(), false );
	}
	else if (m_Owner == 3)
	{
		// Create a red blinking light to show the owner team
		m_hGlowSprite = CSprite::SpriteCreate( HOLDOUT_SPRITE2, GetAbsOrigin(), false );
	}
	m_hGlowSprite->SetAttachment( this, 0 );
	m_hGlowSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxStrobeFast );
	m_hGlowSprite->SetBrightness( 255, 1.0f );
	m_hGlowSprite->SetScale( 0.2f, 0.5f );
	m_hGlowSprite->TurnOn();

}

//-----------------------------------------------------------------------------
// Purpose: Get's the player's team number, excluding spectators(1)
// Input  : Idk who tf is the Player here.. pActivator or pCaller
// Output :
//-----------------------------------------------------------------------------
void CHoldout::HoldoutUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator->GetTeamNumber() > 1)
	{
		if (m_Owner != pActivator->GetTeamNumber())
		{
			m_Owner = pActivator->GetTeamNumber();
			if ( m_hGlowSprite != NULL )
			{
				UTIL_Remove( m_hGlowSprite );
				m_hGlowSprite = NULL;
			}
			CreateEffects();
		}
	}
	else if (pCaller->GetTeamNumber() > 1)
	{
		if (m_Owner != pCaller->GetTeamNumber())
		{
			m_Owner = pCaller->GetTeamNumber();
			if (m_hGlowSprite != NULL)
			{
				UTIL_Remove(m_hGlowSprite);
				m_hGlowSprite = NULL;
			}
			CreateEffects();
		}
	}
}

void CHoldout::HoldoutThink( void )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
	if (!pPlayer)
	{
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}

	if (m_Owner == 2)
	{
		if (pPlayer->GetBlueTime() >= 0.1f)
		{
			pPlayer->m_BlueTime = pPlayer->GetBlueTime() - 0.1f;
		}
		else
		{
			HL2MPRules()->GoToIntermission();
		}
	}
	else if (m_Owner == 3)
	{
		if (pPlayer->GetRedTime() >= 0.1f)
		{
			pPlayer->m_RedTime = pPlayer->GetRedTime() - 0.1f;
		}
		else
		{
			HL2MPRules()->GoToIntermission();
		}
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CHoldout::Precache( void )
{
	PrecacheModel("models/props/holdout.mdl");
	PrecacheModel(HOLDOUT_SPRITE);
	PrecacheModel(HOLDOUT_SPRITE2);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CHoldout::CHoldout(void)
{
	m_Owner = NULL;
}

CHoldout::~CHoldout(void)
{
	if ( m_hGlowSprite != NULL )
	{
		UTIL_Remove( m_hGlowSprite );
		m_hGlowSprite = NULL;
	}
}
