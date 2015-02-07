 #ifndef HUD_TASKLIST_H
 #define HUD_TASKLIST_H
 
 #define TASKLIST_MAX_TASKS 4
 #define TASKLIST_TASK_INACTIVE 0
 #define TASKLIST_TASK_COMPLETE 1
 #define TASKLIST_TASK_LOWPRIORITY 2
 #define TASKLIST_TASK_MEDPRIORITY 3
 #define TASKLIST_TASK_HIGHPRIORITY 4
 
 #define TASKLINE_NUM_FLASHES 8.0f
 #define TASKLINE_FLASH_TIME 5.0f
 #define TASKLINE_FADE_TIME 1.0f
 
 class CHudTaskList : public CHudElement , public vgui::Panel
 {
 DECLARE_CLASS_SIMPLE( CHudTaskList, vgui::Panel );
 
 public:
 	CHudTaskList( const char *pElementName );	//The constructor.
 	void Init( void );	//This gets called when the element is created.
 	void VidInit( void );	//Same as above, this may only gets called when you load up the map for the first time.
 	void Reset();	//This gets called when you reset the HUD.
 	void Paint( void );	//This gets called every frame, to display the element on the screen!
 	// void OnThink(void);	//This gets called also almost every frame, so we can update things often.
 	void MsgFunc_TaskList( bf_read &msg );
 
 private:
 	// CHudTexture *m_pIcon;		// Icon texture reference
 	wchar_t		m_pText[TASKLIST_MAX_TASKS][256];	// Unicode text buffer
 	int			m_iPriority[TASKLIST_MAX_TASKS];		// 0=inactive, 1=complete, 2=low, 3=medium, 4=high
 	float		m_flStartTime[TASKLIST_MAX_TASKS];	// When the message was recevied
 	float		m_flDuration[TASKLIST_MAX_TASKS];	// Duration of the message
 	vgui::HFont m_hSmallFont, m_hLargeFont;
 
 protected:
 	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
 
 private:
  	// vgui::Label *m_pNameLabel;	//The vgui label that will hold our name.
  	vgui::HScheme scheme; 	 	//The Scheme object to hold our scheme info.
 };
 #endif