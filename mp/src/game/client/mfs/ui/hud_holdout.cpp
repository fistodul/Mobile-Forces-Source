//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The ugliest there is, but it should work
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ihudlcd.h"
#include "hl2mp/hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays the holdout time
//-----------------------------------------------------------------------------
class HudHoldout : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE(HudHoldout, CHudNumericDisplay);
public:

	HudHoldout(const char *pElementName);
	void Init( void );
	void VidInit( void );
	void Reset();

	void SetAmmo(int ammo, bool playAnimation);
	void SetAmmo2(int ammo2, bool playAnimation);

protected:
	virtual void OnThink();
	void UpdateAmmoDisplays(C_BasePlayer *pPlayer);
	
private:
	int	BlueTime;
	int	RedTime;
};

DECLARE_HUDELEMENT(HudHoldout);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
HudHoldout::HudHoldout(const char *pElementName) : BaseClass(NULL, "HudHoldout"), CHudElement(pElementName)
{
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HudHoldout::Init(void)
{
	BlueTime		= -1;
	RedTime = -1;

	wchar_t *tempString = g_pVGuiLocalize->Find("#MFS_Hud_Holdout");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"Holdout");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HudHoldout::VidInit(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void HudHoldout::Reset()
{
	BaseClass::Reset();

	BlueTime = 0;
	RedTime = 0;
}

void HudHoldout::UpdateAmmoDisplays(C_BasePlayer *pPlayer)
{
	CHL2MPRules *pRules = HL2MPRules();
	if (pRules->IsHoldout() == false)
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}
	else
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
	}
#ifdef MFS
	BlueTime = pPlayer->GetBlueTime();
	RedTime = pPlayer->GetRedTime();
#endif
	SetAmmo(BlueTime, true);
	SetAmmo2(RedTime, true);
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void HudHoldout::OnThink()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();//UTIL_PlayerByIndex(1);
	UpdateAmmoDisplays(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void HudHoldout::SetAmmo(int ammo, bool playAnimation)
{
	if (ammo != BlueTime)
	{
		if (ammo == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoEmpty");
		}
		else if (ammo < BlueTime)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreased");
		}

		BlueTime = ammo;
	}

	SetDisplayValue(ammo);
}

//-----------------------------------------------------------------------------
// Purpose: Updates 2nd ammo display
//-----------------------------------------------------------------------------
void HudHoldout::SetAmmo2(int ammo2, bool playAnimation)
{
	if (ammo2 != RedTime)
	{
		if (ammo2 == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Empty");
		}
		else if (ammo2 < RedTime)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Decreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Increased");
		}

		RedTime = ammo2;
	}

	SetSecondaryValue(ammo2);
}
