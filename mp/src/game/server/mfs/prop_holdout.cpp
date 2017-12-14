//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
//#include "player.h"
//#include "soundenvelope.h"
//#include "engine/IEngineSound.h"
//#include "game.h"
//#include "team.h"
#include "Sprite.h"
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
return;
}
#endif
	Precache( );
	SetModel( "models/objectives/holdout/holdout.mdl" );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );

	SetCollisionGroup(COLLISION_GROUP_INTERACTIVE);

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
// Input  : The player here is.. pActivator
// Output :
//-----------------------------------------------------------------------------
void CHoldout::HoldoutUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// if it's not a player, ignore
	if (!pActivator || !pActivator->IsPlayer())
		return;

	m_hActivator = pActivator;
	CBasePlayer *pPlayer = (CBasePlayer *)m_hActivator.Get();

	if (pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
	{
		if (m_Owner != pPlayer->GetTeamNumber())
		{
			m_Owner = pPlayer->GetTeamNumber();
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
#ifdef MFS
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer)
		{
			SetNextThink(gpGlobals->curtime + 0.1f);
			return;
		}

		if (m_Owner == TEAM_COMBINE)
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
		else if (m_Owner == TEAM_REBELS)
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
	}
	SetNextThink(gpGlobals->curtime + 0.1f);
#endif
}

void CHoldout::Precache( void )
{
	PrecacheModel("models/objectives/holdout/holdout.mdl");
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
