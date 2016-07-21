#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
 
#ifdef CLIENT_DLL
  #include "c_hl2mp_player.h"
#else
	#include "basecombatcharacter.h"
	#include "player.h"
  #include "hl2mp_player.h"
#endif
 
#include "weapon_hl2mpbasehlmpcombatweapon.h"
 
//modify this to alter the default(before its increased) rate of fire
#define ROF 0.080f //RPS, 60 Sec / 800 Rounds = 0.075f
 
//The gun will fire up to this number of bullets while you hold the fire button. 
//If you set it to 1 the gun will be semi auto. If you set it to 3 the gun will fire three round bursts
#define BURST 720;
 
#ifdef CLIENT_DLL
#define CWeaponMiniGun C_WeaponMiniGun
#endif
 
ConVar sk_max_Rifle( "sk_max_Rifle", "720", FCVAR_SERVER_CAN_EXECUTE | FCVAR_ARCHIVE, "How much this troll wep can have max ammo" );

//-----------------------------------------------------------------------------
// CWeaponMiniGun
//-----------------------------------------------------------------------------
 
class CWeaponMiniGun : public CBaseHL2MPCombatWeapon
{
  public:
 
    DECLARE_CLASS( CWeaponMiniGun, CBaseHL2MPCombatWeapon );
    CWeaponMiniGun(void);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
 
	int ROFI;

    void Precache( void );
    void ItemPostFrame( void );
    void ItemPreFrame( void );
    void ItemBusyFrame( void );
    void PrimaryAttack( void );
	void DelayedAttack( void );
    void AddViewKick( void );
    void DryFire( void );
    void GetStance( void );
    bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL ); // Required so that you un-zoom when switching weapons
    Activity GetPrimaryAttackActivity( void );
 
    virtual bool Reload( void );
 
    int GetMinBurst() { return 2; }
	int GetMaxBurst() { return 5; }
    float GetFireRate( void ) { return ROFI; }
 
    //modify this part to control the general accuracy of the gun
 
    /*virtual const Vector& GetBulletSpread( void )
    {
		Vector cone = VECTOR_CONE_1DEGREES;
 
      // if you don't need stance and health dependent accuracy, you can just remove this.   
      if ( m_iStance == E_DUCK )
        { cone = VECTOR_CONE_1DEGREES;}
      if ( m_iStance == E_STAND )
        { cone = VECTOR_CONE_2DEGREES;}
      if ( m_iStance == E_MOVE )
        { cone = VECTOR_CONE_3DEGREES;}
      if ( m_iStance == E_RUN )
        { cone = VECTOR_CONE_4DEGREES;}
      if ( m_iStance == E_INJURED )
        { cone = VECTOR_CONE_3DEGREES;}
      if ( m_iStance == E_JUMP )
        { cone = VECTOR_CONE_4DEGREES;}
      if ( m_iStance == E_DYING )
        { cone = VECTOR_CONE_10DEGREES;}
 
      //This part simlates recoil. Each successive shot will have increased spread.
      if ( m_iBurst != BURST )
      {
        for (int i = m_iBurst; i < BURST; i++)
        {
          cone += VECTOR_CONE_1DEGREES;
        }
      }
 
      //This part is the zoom modifier. If in zoom, lower the bullet spread.
      if (m_bInZoom)
      {
        cone -= VECTOR_CONE_1DEGREES;
      }
 
      //return cone;
    }*/
 
    //void ToggleZoom( void );
    //void CheckZoomToggle( void );
 
    DECLARE_ACTTABLE();
 
  private:
    CNetworkVar( int, m_iBurst );
    //CNetworkVar( bool, m_bInZoom );
    CNetworkVar( float, m_flAttackEnds );
    CNetworkVar( int, m_iStance);
 
  private:
	bool  m_bDelayedAttack;
	float m_flDelayedAttackTime;
    CWeaponMiniGun( const CWeaponMiniGun & );
};
 
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMiniGun, DT_WeaponMiniGun )
 
BEGIN_NETWORK_TABLE( CWeaponMiniGun, DT_WeaponMiniGun )
#ifdef CLIENT_DLL
  RecvPropInt(  RECVINFO( m_iBurst) ),
  //RecvPropBool( RECVINFO( m_bInZoom ) ),
  RecvPropTime( RECVINFO( m_flAttackEnds ) ),
  RecvPropInt(  RECVINFO( m_iStance ) ),
#else
  SendPropInt(  SENDINFO( m_iBurst ) ),
  //SendPropBool( SENDINFO( m_bInZoom ) ),
  SendPropTime( SENDINFO( m_flAttackEnds ) ),
  SendPropInt(  SENDINFO( m_iStance ) ),
#endif
END_NETWORK_TABLE()
 
BEGIN_PREDICTION_DATA( CWeaponMiniGun )
END_PREDICTION_DATA()
 
LINK_ENTITY_TO_CLASS( weapon_MiniGun, CWeaponMiniGun );
PRECACHE_WEAPON_REGISTER( weapon_MiniGun );
 
acttable_t CWeaponMiniGun::m_acttable[] =
{
  { ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_AR2, false },
  { ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_AR2, false },
  { ACT_MP_RUN, ACT_HL2MP_RUN_AR2, false },
  { ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_AR2, false },
  { ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
  { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
  { ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_AR2, false },
  { ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_AR2, false },
  { ACT_MP_JUMP, ACT_HL2MP_JUMP_AR2, false },
};
 
IMPLEMENT_ACTTABLE( CWeaponMiniGun );
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMiniGun::CWeaponMiniGun( void )
{
  m_iBurst=BURST;
  m_iStance=10;
  m_fMinRange1 = 1;
  m_fMaxRange1 = 1500;
  m_fMinRange2 = 1;
  m_fMaxRange2 = 200;
  m_bFiresUnderwater = false;
  m_bDelayedAttack = false;
  m_flDelayedAttackTime = 0.0f;
  ROFI = ROF;
  #ifdef Weighted_Weaponry
  Minigun_Weight = 5;
  #endif
}
 
//-----------------------------------------------------------------------------
// Purpose: Required for caching the entity during loading
//-----------------------------------------------------------------------------
void CWeaponMiniGun::Precache( void )
{
  BaseClass::Precache();
}
 
//-----------------------------------------------------------------------------
// Purpose: The gun is empty, plays a clicking noise with a dryfire anim
//-----------------------------------------------------------------------------
void CWeaponMiniGun::DryFire( void )
{
  WeaponSound( EMPTY );
  SendWeaponAnim( ACT_VM_DRYFIRE );
  m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}
 
//-----------------------------------------------------------------------------
// Purpose: This happens if you click and hold the primary fire button
//-----------------------------------------------------------------------------
void CWeaponMiniGun::PrimaryAttack( void )
{
  //do we have any bullets left from the current burst cycle? 
  if (m_iBurst!=0) 
  {
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer )
      { return; }
 
    WeaponSound( SINGLE );
    pPlayer->DoMuzzleFlash();
 
    SendWeaponAnim( ACT_VM_PRIMARYATTACK );
    pPlayer->SetAnimation( PLAYER_ATTACK1 );
//    ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
 
    // Each time the player fires the gun, reset the view punch.  
    if ( pOwner )
      { pOwner->ViewPunchReset(); }
 
    BaseClass::PrimaryAttack();
 
    // We fired one shot, decrease the number of bullets available for this burst cycle 
    m_iBurst--;
    m_flNextPrimaryAttack =gpGlobals->curtime + ROFI;
    m_flAttackEnds = gpGlobals->curtime + SequenceDuration();
  }
}

void CWeaponMiniGun::DelayedAttack( void )
{
         if (m_bDelayedAttack && gpGlobals->curtime > m_flDelayedAttackTime)
         {
		PrimaryAttack();
		BaseClass::PrimaryAttack();
		m_bDelayedAttack = false;
         }
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMiniGun::ItemPreFrame( void )
{
  GetStance();
  BaseClass::ItemPreFrame();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMiniGun::ItemBusyFrame( void )
{
  // Allow zoom toggling even when we're reloading
  //CheckZoomToggle();
  BaseClass::ItemBusyFrame();
}
 
//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponMiniGun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( (pOwner->m_afButtonPressed & IN_ATTACK) && !m_bDelayedAttack)
	{
		//Animation Comments
		//SendWeaponAnim( ACT_VM_HAULBACK );
		//m_flDelayedAttackTime = gpGlobals->curtime + SequenceDuration() + 1.0f;
		m_flDelayedAttackTime = gpGlobals->curtime + 1.0f;
		m_bDelayedAttack = true;
	}
	DelayedAttack();

  BaseClass::ItemPostFrame();

  if ( m_bInReload )
    { return; }
 
  if ( pOwner == NULL )
    { return; }
 
  if ( pOwner->m_nButtons & IN_ATTACK )
  {
	//This epic line makes the increasing happen xD
	if ( ROFI > 0.03f )
	{
	ROFI = ROFI - 0.005f;
	}
    if (m_flAttackEnds<gpGlobals->curtime)
	{
	ROFI = ROF;
	{ SendWeaponAnim(ACT_VM_IDLE); }
	}
  }
  else
  {
    //The firing cycle ended. Reset the burst counter to the max value
    m_iBurst=BURST;
    if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
      { DryFire(); }
  }
  //CheckZoomToggle();
  //check the character's current stance for the accuracy calculation
  GetStance();
}
 
//-----------------------------------------------------------------------------
// Purpose: If we have bullets left then play the attack anim otherwise idle
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponMiniGun::GetPrimaryAttackActivity( void )
{
  if (m_iBurst!=0)
    { return ACT_VM_PRIMARYATTACK; }
  else
    { return ACT_VM_IDLE; }
}
 
//-----------------------------------------------------------------------------
// Purpose: The gun is being reloaded 
//-----------------------------------------------------------------------------
bool CWeaponMiniGun::Reload( void )
{
  bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
  if ( fRet )
  {
    WeaponSound( RELOAD );
    //ToHL2MPPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
    //reset the burst counter to the default
    m_iBurst=BURST;
  }
  return fRet;
}
 
//-----------------------------------------------------------------------------
// Purpose: Put away the gun and disable zoom if needed
//----------------------------------------------------------------------------- 
bool CWeaponMiniGun::Holster(CBaseCombatWeapon *pSwitchingTo /* = NULL */)
{
  //if ( m_bInZoom )
   // { ToggleZoom(); }
  return BaseClass::Holster( pSwitchingTo );
}
 
//-----------------------------------------------------------------------------
// Purpose: Calculate the viewkick
//-----------------------------------------------------------------------------
void CWeaponMiniGun::AddViewKick( void )
{
  CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
  if ( pPlayer == NULL )
    { return; }
 
  int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
  RandomSeed( iSeed );
 
  QAngle viewPunch;
 
  viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
  viewPunch.y = random->RandomFloat( -.6f, .6f );
  viewPunch.z = 0.0f;
 
  //Add it to the view punch
  pPlayer->ViewPunch( viewPunch );
}
 
 
//-----------------------------------------------------------------------------
// Purpose: Toggle the zoom by changing the client's FOV
//-----------------------------------------------------------------------------
/*void CWeaponMiniGun::ToggleZoom( void )
{
  CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
  if ( pPlayer == NULL )
    { return; }
 
  #ifndef CLIENT_DLL
    if ( m_bInZoom )
    {
      // Narrowing the Field Of View here is what gives us the zoomed effect
      if ( pPlayer->SetFOV( this, 0, 0.2f ) )
      {
        m_bInZoom = false;
 
        // Send a message to hide the scope
        /* CSingleUserRecipientFilter filter(pPlayer);
        UserMessageBegin(filter, "ShowScope");
        WRITE_BYTE(0);
        MessageEnd();*/
     /* }
    }
    else
    {
      if ( pPlayer->SetFOV( this, 45, 0.1f ) )
      {
        m_bInZoom = true;
 
        // Send a message to Show the scope
        /* CSingleUserRecipientFilter filter(pPlayer);
        UserMessageBegin(filter, "ShowScope");
        WRITE_BYTE(1);
        MessageEnd();*/
    /*  }
    }
  #endif
}
 
//-----------------------------------------------------------------------------
// Purpose: Toggle the zoom if the Sec attack button was pressed
//-----------------------------------------------------------------------------
void CWeaponMiniGun::CheckZoomToggle( void )
{
  CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
  if ( pPlayer && (pPlayer->m_afButtonPressed & IN_ATTACK2))
    { ToggleZoom(); }
}*/
 
//-----------------------------------------------------------------------------
// Purpose: Get the current stance/status of the player
//----------------------------------------------------------------------------- 
void CWeaponMiniGun::GetStance( void )
{
  CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
  if ( pPlayer == NULL )
    { return; }
 
 /* m_iStance= E_STAND;
 
  // movement based stance
  if ( pPlayer->m_nButtons & IN_DUCK)     
    { m_iStance= E_DUCK;}
  if ( pPlayer->m_nButtons & IN_FORWARD)  
    { m_iStance= E_MOVE;}
  if ( pPlayer->m_nButtons & IN_BACK)     
    { m_iStance= E_MOVE;}
  if ( pPlayer->m_nButtons & IN_MOVERIGHT)
    { m_iStance= E_MOVE;}
  if ( pPlayer->m_nButtons & IN_MOVELEFT) 
    { m_iStance= E_MOVE;}
  if ( pPlayer->m_nButtons & IN_RUN)      
    { m_iStance= E_RUN;}
  if ( pPlayer->m_nButtons & IN_SPEED)    
    { m_iStance= E_RUN;}
  if ( pPlayer->m_nButtons & IN_JUMP)     
    { m_iStance= E_JUMP;}
 
  //health based status
  if ( pPlayer->GetHealth()<25)           
    { m_iStance= E_INJURED;}
  if ( pPlayer->GetHealth()<10)           
    { m_iStance= E_DYING;}*/
}