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
#define	HOLDOUT_SPRITE2	"sprites/redglow1.vmt"

BEGIN_DATADESC( CHoldout )

	DEFINE_FIELD( m_Owner, FIELD_INTEGER ),

	// Function Pointers
	//DEFINE_USEFUNC( HoldoutUse ),
	DEFINE_THINKFUNC( HoldoutThink ),

END_DATADESC()

/*IMPLEMENT_SERVERCLASS_ST( CHoldout, DT_Holdout )
	SendPropFloat( SENDINFO( m_BlueTime ) ),
	SendPropFloat( SENDINFO( m_RedTime ) ),
END_SEND_TABLE()*/

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
if ( m_Owner != NULL )
{
	if ( m_Owner == 2 )
	{
	// Create a blue blinking light to show the owner team
	m_hGlowSprite = CSprite::SpriteCreate( HOLDOUT_SPRITE2, GetAbsOrigin(), false );
	}
	else
	{
	// Create a red blinking light to show the owner team
	m_hGlowSprite = CSprite::SpriteCreate( HOLDOUT_SPRITE, GetAbsOrigin(), false );
	}
	m_hGlowSprite->SetAttachment( this, 0 );
	m_hGlowSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxStrobeFast );
	m_hGlowSprite->SetBrightness( 255, 1.0f );
	m_hGlowSprite->SetScale( 0.2f, 0.5f );
	m_hGlowSprite->TurnOn();
}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CHoldout::HoldoutUse(CBaseEntity *pPlayer, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if ( pPlayer->GetTeamNumber() > 1 ) //Shouldnt be owned by spectators even if they somehow "use"
	{
		if (m_Owner != pPlayer->GetTeamNumber())
		{
		m_Owner = pPlayer->GetTeamNumber();
		if ( m_hGlowSprite != NULL )
		{
		UTIL_Remove( m_hGlowSprite );
		m_hGlowSprite = NULL;
		}
		CreateEffects();
		}
	}
}

void CHoldout::HoldoutThink( void )
{
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
				HL2MPRules()->GoToIntermission();
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
				HL2MPRules()->GoToIntermission();
			}
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
