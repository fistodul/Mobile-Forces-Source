//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hit Marker
//
//=============================================================================//

#include "cbase.h"
#ifdef hitmarkers
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_hitmarker.h"
#include "iclientmode.h"
#include "c_baseplayer.h"

// VGUI panel includes
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;
 
// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"
 
DECLARE_HUDELEMENT( CHudHitmarker );
DECLARE_HUD_MESSAGE( CHudHitmarker, ShowHitmarker );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
 
CHudHitmarker::CHudHitmarker( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudHitmarker")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
 
	// Hitmarker will not show when the player is dead
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
 
	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Init()
{
	HOOK_HUD_MESSAGE( CHudHitmarker, ShowHitmarker );

	SetAlpha( 0 );
	m_bHitmarkerShow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Reset()
{
	SetAlpha( 0 );
	m_bHitmarkerShow = false;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings(scheme);
 
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHitmarker::ShouldDraw( void )
{
	return ( m_bHitmarkerShow && CHudElement::ShouldDraw() );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Paint( void )
{
        C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
        if (!pPlayer)
        {
                return;
        }
 
	if (m_bHitmarkerShow)
	{
		int		x,y;

		// Find our screen position to start from
		x = XRES(320);
		y = YRES(240);

		vgui::surface()->DrawSetColor( m_HitmarkerColor );
		vgui::surface()->DrawLine( x - 6, y - 5, x - 11, y - 10 );
		vgui::surface()->DrawLine( x + 5, y - 5, x + 10, y - 10 );
		vgui::surface()->DrawLine( x - 6, y + 5, x - 11, y + 10 );
		vgui::surface()->DrawLine( x + 5, y + 5, x + 10, y + 10 );
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::MsgFunc_ShowHitmarker(bf_read &msg)
{
	m_bHitmarkerShow = msg.ReadByte();

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HitMarkerShow");
}
#endif