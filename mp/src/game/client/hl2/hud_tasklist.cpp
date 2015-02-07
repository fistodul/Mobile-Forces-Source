 //========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
 //
 // Purpose: Task List element
 //
 //=============================================================================
 
 #include "cbase.h"
 #include "hudelement.h"
 #include "hud_macros.h"
 #include "iclientmode.h"
 #include "view.h"
 
 using namespace vgui;
 
 #include <vgui_controls/Panel.h>
 #include <vgui_controls/Frame.h>
 #include <vgui/IScheme.h>
 #include <vgui/ISurface.h>
 #include <vgui/ILocalize.h>
 
 using namespace vgui;
 
 #include "c_baseplayer.h"
 #include "hud_tasklist.h"
 
 DECLARE_HUDELEMENT( CHudTaskList );
 DECLARE_HUD_MESSAGE( CHudTaskList, TaskList );
 
 // ===================================================================
 
 
 CHudTaskList::CHudTaskList( const char *pElementName ) : CHudElement( pElementName ), vgui::Panel( NULL, "HudTaskList" )
 {
 	DevMsg (2, "CHudTaskList::CHudTaskList - constructor sent %s\n", pElementName);
 	scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
 	SetScheme(scheme);	// Here we load up our scheme and set this element to use it. Using a different scheme than ClientScheme doesn't work right off the bat anyways, so... :)
 	vgui::Panel *pParent = g_pClientMode->GetViewport();
 	SetParent( pParent );	// Our parent is the screen itself.
 	SetHiddenBits( 0 );	// Never Hidden. You could make it so that it gets hidden when health is, with HIDEHUD_HEALTH, or any other combination of HIDEHUD flags.
 	SetBgColor(Color( 0,0,0,100 ));
 	SetPaintBackgroundEnabled( true );
 	SetPaintBackgroundType (2); // Rounded corner box
 } 
 
 
 void CHudTaskList::Init( void )
 {
 	HOOK_HUD_MESSAGE( CHudTaskList, TaskList );
 	DevMsg (2, "CHudTaskList::Init\n" );
 	Reset();
 }
 
 void CHudTaskList::Reset( void )
 {
 	DevMsg (2, "CHudTaskList::Reset (clearing out list)\n" );
 	for (int i=0; i<TASKLIST_MAX_TASKS; i++) 
 	{
 		m_pText[i][0] = '\0';
 		m_iPriority[i] = TASKLIST_TASK_INACTIVE;
 	}
 }
 
 void CHudTaskList::VidInit( void )
 {
 	DevMsg (2, "CHudTaskList::VidInit\n" );
 	Reset();
 }
 
 //-----------------------------------------------------------------------------
 // Purpose: 
 //-----------------------------------------------------------------------------
 void CHudTaskList::ApplySchemeSettings( vgui::IScheme *pScheme )
 {
 	DevMsg (2, "CHudTaskList::ApplySchemeSettings\n" );
 
 	m_hSmallFont = pScheme->GetFont( "HudHintTextSmall", true );
 	m_hLargeFont = pScheme->GetFont( "HudHintTextLarge", true );
 
 	BaseClass::ApplySchemeSettings( pScheme );
 }
 
 
 
 	/*
 	virtual int GetFontTall(HFont font) = 0;
 	virtual int GetFontAscent(HFont font, wchar_t wch) = 0;
 	virtual bool IsFontAdditive(HFont font) = 0;
 	virtual void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) = 0;
 	virtual int GetCharacterWidth(HFont font, int ch) = 0;
 	virtual void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) = 0;
 	*/
 
 //-----------------------------------------------------------------------------
 // Purpose: 
 //-----------------------------------------------------------------------------
 void CHudTaskList::Paint( void )
 {
 	// KLUDGE!  DISABLE TEMPORARILY
 	// return;
 
 	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
 	if ( !pPlayer )
 		return;
 
 	int x, y;
 	int textSizeWide, textSizeTall;
 	int iShown = 0; // number of lines shown
 	wchar_t unicode[256]; // scratch space for text to print
 
 	// --- Set up default font and get character height for line spacing
 	vgui::surface()->DrawSetTextFont( m_hLargeFont );
 	int fontTall = vgui::surface()->GetFontTall( m_hLargeFont );
 
 	// --- Don't actually draw the task list at first, but instead 
 	// --- calculate the total width & height of the text.
 	swprintf(unicode, L"Active Tasks");
 	vgui::surface()->GetTextSize (m_hLargeFont, unicode, textSizeWide, textSizeTall);
 	iShown++;
 
 	int border = 20;
 	int maxWidth = textSizeWide;
 	int maxHeight;
 	for (int i=0; i<TASKLIST_MAX_TASKS; i++) 
 	{
 		if (m_iPriority[i] != TASKLIST_TASK_INACTIVE)
 		{
 			// --- Calculate coordinates of text (right justify)
 			swprintf(unicode, L"%s", m_pText[i]);
 			vgui::surface()->GetTextSize (m_hLargeFont, unicode, textSizeWide, textSizeTall);
 			maxWidth = (textSizeWide > maxWidth) ? textSizeWide : maxWidth;
 			iShown++;
 		}
 	}
 
 	// If no text to show except the title, show nothing
 	if (iShown == 1) {
 		SetPaintBackgroundEnabled( false );
 		return;
 	}
 
 	maxHeight = iShown * fontTall;
 
 	SetPos( ScreenWidth() - border - maxWidth, border);
 	SetSize( maxWidth + border, maxHeight );
 	SetPaintBackgroundType (2); // Rounded corner box
 	SetPaintBackgroundEnabled( true );
 
 	// ----------------------------------------------------------
 	// --- Prep text for a title on the task list.  It actually
 	// --- will get drawn in the for loop below on the first pass.
 	// ----------------------------------------------------------
   	vgui::surface()->DrawSetTextColor( 255, 170, 0, 220 ); 
 	swprintf(unicode, L"Active Tasks");
 
 	// ----------------------------------------------------------
 	// --- Get text width and right justify
 	// ----------------------------------------------------------
 	vgui::surface()->GetTextSize (m_hLargeFont, unicode, textSizeWide, textSizeTall);
 
 	// ----------------------------------------------------------
 	// --- Calculate coordinates of title text
 	// ----------------------------------------------------------
 	iShown = 0;
 	x = border / 2; // ScreenWidth() - border - textSizeWide;
 	y = iShown * fontTall; // border + iShown * fontTall;
 
 	// ----------------------------------------------------------
 	// --- Draw all tasks that aren't inactive.
 	// ----------------------------------------------------------
 	for (int i=0; i<TASKLIST_MAX_TASKS; i++) 
 	{
 		if (m_iPriority[i] != TASKLIST_TASK_INACTIVE)
 		{
 			// ----------------------------------------------------------
 			// --- If this is the first task item, show task list title
 			// ----------------------------------------------------------
 			if (iShown == 0) {
 				iShown++;
 				vgui::surface()->DrawSetTextPos(x, y);
 				vgui::surface()->DrawPrintText( unicode, wcslen(unicode) );
 			}
 
 			// ----------------------------------------------------------
 			// --- Calculate coordinates of text (right justify)
 			// ----------------------------------------------------------
 			swprintf(unicode, L"%s", m_pText[i]);
 			vgui::surface()->GetTextSize (m_hLargeFont, unicode, textSizeWide, textSizeTall);
 			x = border / 2; // ScreenWidth() - border - textSizeWide;
 			y = iShown * fontTall; // border + iShown * fontTall;
 
 			iShown++;
 			vgui::surface()->DrawSetTextPos(x, y);
 
 			int lr=255, lg=255, lb=255;
 			// DevMsg (2, "CHudTaskList::Paint task %d, priority %d:  %s\n", i, m_iPriority[i], m_pText[i][0] );
 
 			// ----------------------------------------------------------
 			// --- Set text color based on priority 
 			// ----------------------------------------------------------
 			switch (m_iPriority[i])
 			{
 			case TASKLIST_TASK_COMPLETE :
 			  	// vgui::surface()->DrawSetTextColor( 100, 255, 0, brightness ); 
 				lr = 0;
 				lg = 255;
 				lb = 0;
 				break;
 
 			case TASKLIST_TASK_LOWPRIORITY : 
 			case TASKLIST_TASK_MEDPRIORITY : 
 			  	// vgui::surface()->DrawSetTextColor( 255, 220, 0, 255 ); 
 				lr = 255;
 				lg = 255; // 220;
 				lb = 0;
 				break;
 
 			case TASKLIST_TASK_HIGHPRIORITY : 
 			  	// vgui::surface()->DrawSetTextColor( 255, 0, 0, 255 ); 
 				lr = 255;
 				lg = 0;
 				lb = 0;
 				break;
 
 			default :
 				// ----------------------------------------------------------
 				// --- We should never get here!
 				// ----------------------------------------------------------
 				DevMsg ("hud_tasklistdisplay:  Task List item %d (%s) with unknown priority %d!\n", i, m_pText[i], m_iPriority[i]);
 				break;
 			}
 
 			// ----------------------------------------------------------
 			// --- Calculate text flash color and fade for completed tasks
 			// ----------------------------------------------------------
 			float curtime = gpGlobals->curtime;
 			if ( curtime >= m_flStartTime[i] && curtime < m_flStartTime[i] + m_flDuration[i] )
 			{
 				// ----------------------------------------------------------
 				// --- FLASH TEXT
 				// ----------------------------------------------------------
 				float frac1 = ( curtime - m_flStartTime[i] ) / m_flDuration[i];
 				float frac = frac1;
 
 				frac *= TASKLINE_NUM_FLASHES;
 				frac *= 2 * M_PI;
 
 				frac = cos( frac );
 
 				frac = clamp( frac, 0.0f, 1.0f );
 
 				frac *= (1.0f-frac1);
 
 				int r = lr, g = lg, b = lb;
 
 				r = r + ( 255 - r ) * frac;
 				g = g + ( 255 - g ) * frac;
 				b = b + ( 255 - b ) * frac;
 
 				int alpha = 63 + 192 * (1.0f - frac1 );
 				alpha = clamp( alpha, 0, 255 );
 
 			  	vgui::surface()->DrawSetTextColor( r, g, b, 255 ); 
 				// InsertColorChange( Color( r, g, b, 255 ) );
 			}
 			else if ( m_iPriority[i] == TASKLIST_TASK_COMPLETE &&
 				      curtime <= m_flStartTime[i] + m_flDuration[i] + TASKLINE_FADE_TIME && 
 					  curtime > m_flStartTime[i] + m_flDuration[i] )
 			{
 				// ----------------------------------------------------------
 				// --- FADE TEXT
 				// ----------------------------------------------------------
 				float frac = ( m_flStartTime[i] + m_flDuration[i] + TASKLINE_FADE_TIME - curtime ) / TASKLINE_FADE_TIME;
 
 				int alpha = frac * 255;
 				alpha = clamp( alpha, 0, 255 );
 
 			  	vgui::surface()->DrawSetTextColor( lr * frac, lg * frac, lb * frac, alpha ); 
 			}
 			else
 			{
 				// ----------------------------------------------------------
 				// --- NORMAL TEXT
 				// ----------------------------------------------------------
 				vgui::surface()->DrawSetTextColor( lr, lg, lb, 255 ); 
 			}
 
 			// ----------------------------------------------------------
 			// --- Draw the text
 			// ----------------------------------------------------------
 			vgui::surface()->DrawPrintText( unicode, wcslen(unicode) ); // print text
 
 			// ----------------------------------------------------------
 			// --- Remove fully faded out completed tasks
 			// ----------------------------------------------------------
 			if (m_iPriority[i] == TASKLIST_TASK_COMPLETE && gpGlobals->curtime > m_flDuration[i]+m_flStartTime[i]+TASKLINE_FADE_TIME)
 			{
 				m_iPriority[i] = TASKLIST_TASK_INACTIVE;
 			}
 		}
 	}
 	BaseClass::Paint();
 }
 
 // ----------------------------------------------------------
 // ----------------------------------------------------------
 void CHudTaskList::MsgFunc_TaskList( bf_read &msg )
 {
 	char szString[256];
 	int task_index;
 	int task_priority;
 
 	task_index = msg.ReadByte();
 	task_priority = msg.ReadByte();
 	msg.ReadString( szString, sizeof(szString) );
 
 	DevMsg (2, "CHudTaskList::MsgFunc_TaskList - got message for task %d, priority %d, string %s\n", task_index, task_priority, szString );
 
 	// ----------------------------------------------------------
 	// --- Convert it to localize friendly unicode
 	// ----------------------------------------------------------
 	g_pVGuiLocalize->ConvertANSIToUnicode( szString, m_pText[task_index], sizeof(m_pText[task_index]) );
 
 	// ----------------------------------------------------------
 	// --- Setup a time tracker for tasks just added or updated
 	// ----------------------------------------------------------
 	if ( m_iPriority[task_index] != task_priority )
 	{
 		m_flStartTime[task_index] = gpGlobals->curtime;
 		m_flDuration[task_index] = TASKLINE_FLASH_TIME;
 	}
 	m_iPriority[task_index] = task_priority;
 }