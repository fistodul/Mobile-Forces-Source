#pragma once

#include "igameui2.h"

#include "cdll_int.h"
#include "engine/IEngineSound.h"
#include "ienginevgui.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ivrenderview.h"
#include "view_shared.h"
#include "GameUI/IGameUI.h"

class IVEngineClient;
class IEngineSound;
class IEngineVGui;
class ISoundEmitterSystemBase;
class IVRenderView;
class IGameUI;

class CGameUI2 : public IGameUI2
{
public:
	virtual void		Initialize(CreateInterfaceFn appFactory);
	virtual void		Shutdown();

	virtual void		OnInitialize();
	virtual void		OnShutdown();
	virtual void		OnUpdate();
	virtual void		OnLevelInitializePreEntity();
	virtual void		OnLevelInitializePostEntity();
	virtual void		OnLevelShutdown();

	virtual bool		IsInLevel();
	virtual bool		IsInBackgroundLevel();
	virtual bool		IsInMultiplayer();
	virtual bool		IsInLoading();

	virtual Vector2D	GetViewport();
	virtual float		GetTime();
	virtual vgui::VPANEL GetRootPanel();
	virtual	vgui::VPANEL GetVPanel();
	virtual CViewSetup	GetView();
	virtual VPlane*		GetFrustum();
	virtual ITexture*	GetMaskTexture();
	virtual wchar_t*	GetLocalizedString(const char* text);

	virtual void		SetView(const CViewSetup& view);
	virtual void		SetFrustum(VPlane* frustum);
	virtual void		SetMaskTexture(ITexture* maskTexture);

private:
	CViewSetup			m_pView;
	VPlane*				m_pFrustum;
	ITexture*			m_pMaskTexture;
};

extern CGameUI2& GameUI2();
extern IVEngineClient* engine;
extern IEngineSound* enginesound;
extern IEngineVGui* enginevgui;
extern ISoundEmitterSystemBase* soundemitterbase;