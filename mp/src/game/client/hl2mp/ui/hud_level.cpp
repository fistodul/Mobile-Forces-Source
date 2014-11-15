//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// hud_level.cpp
//
// implementation of CHudLevel class
//
#include "cbase.h"
#ifdef Testing
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_hl2mp_player.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define INIT_BAT	-1
extern int power;

//-----------------------------------------------------------------------------
// Purpose: Displays some thing
//-----------------------------------------------------------------------------
class CHudLevel : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudLevel, CHudNumericDisplay );

public:
	CHudLevel( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );
	bool ShouldDraw();

/*private:
	int		m_iBat;	
	int		m_iNewBat;*/
};

DECLARE_HUDELEMENT( CHudLevel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudLevel::CHudLevel( const char *pElementName ) : BaseClass(NULL, "HudLevel"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::Init( void )
{
	Reset();
	/*m_iBat		= INIT_BAT;
	m_iNewBat   = 0;*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::Reset( void )
{
	wchar_t *tempString = g_pVGuiLocalize->Find("#Hud_Level");

	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"Power-Level");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::VidInit( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudLevel::ShouldDraw( void )
{
	//bool bNeedsDraw = ( m_iBat != m_iNewBat ) || ( GetAlpha() > 0 );
	bool bNeedsDraw = ( power != 1 ) || ( GetAlpha() > 0 );

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLevel::OnThink( void )
{
	/*if ( m_iBat == m_iNewBat )
		return;

	if ( !m_iNewBat )
	{
	 	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerZero");
	}
	else if ( m_iNewBat < m_iBat )
	{
		// battery power has decreased, so play the damaged animation
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitDamageTaken");

		// play an extra animation if we're super low
		if ( m_iNewBat < 20 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitArmorLow");
		}
	}
	else
	{
		// battery power has increased (if we had no previous armor, or if we just loaded the game, don't use alert state)
		if ( m_iBat == INIT_BAT || m_iBat == 0 || m_iNewBat >= 20)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedAbove20");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedBelow20");
		}
	}

	m_iBat = m_iNewBat;*/

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ( pPlayer )
		SetDisplayValue( power );
}
#endif