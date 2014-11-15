#include "cbase.h"
#ifdef Testing
#include "icvar.h"
//#include <convar.h>
#include "myconvars.h"
#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
//#include "Fmod_manager.h"
#else
#include "hl2mp_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
extern ConVar	player_throwforce;
//extern ConVar	physcannon_mega_enabled;
//extern ConVar	cl_playermodel;
//C_HL2MP_Player *C_pPlayer = dynamic_cast< C_HL2MP_Player* >( m_hPlayer.Get() );
C_HL2MP_Player *C_pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
#else
extern ConVar	sv_infinite_aux_power;
CHL2MP_Player *pPlayer = dynamic_cast<CHL2MP_Player *>( pPlayer );
#endif
ConVar troll( "troll", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "hmm troll power xD" );
ConVar rageburst( "rageburst", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "If set to 1(or more), Unleashes The Rage" );
ConVar wisdom( "wisdom", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "wisdom, truth of all creation... etc ? 12=12th dimension :O" );
ConVar training( "training", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "U know what doesent kill u makes u stronger xD" );
ConVar gamemode( "gamemode", "0", FCVAR_REPLICATED | FCVAR_SERVER_CAN_EXECUTE, "Decides the gamemode, 0=does nothing, 1=singleplayer, 2=coop" );

/*int trola = troll.GetInt();
int rage = rageburst.GetInt();
int wisdoma = wisdom.GetInt();
int train = training.GetInt();
int power = (trola + rage + wisdoma + train);*/

void superpowers( void )
{
if ( gamemode.GetInt() == 10 )
{
return;
}
vaild = 1;
#ifdef CLIENT_DLL
C_pPlayer->cpowerup();
#else
pPlayer->spowerup();
#endif
}

#ifdef CLIENT_DLL
void C_HL2MP_Player::cpowerup( void ) // The client-side powerup function
{
trola = troll.GetInt();
rage = rageburst.GetInt();
wisdoma = wisdom.GetInt();
train = training.GetInt();
power = (trola + rage + wisdoma + train);
//engine->GetLevelName();
/*if ( trola >= 1 )
{
if ( trola >= 5 )
{

}
}*/
if ( rage >= 1 )
{
player_throwforce.SetValue( "99999999998" );
/*if ( rageburst.GetInt() >= 5 )
{
}*/
}
if ( wisdoma >= 1 )
{
if ( wisdoma >= 4 )
{
//physcannon_mega_enabled.SetValue( "1" );
}
/*if ( wisdoma >= 12 )
{
cl_playermodel.SetValue( "models/pro] );
}*/
}
/*if ( power >= 1 )
{
//engine->ClientCmd();
}*/
/*FMODManager()->IsSoundPlaying( "weapons\powerup.mp3" ) != true;
FMODManager()->PlayAmbientSound( "weapons\powerup.mp3", true );*/
}
#endif
#ifdef GAME_DLL
void CHL2MP_Player::spowerup( void ) // The server-side powerup function
{
trola = troll.GetInt();
rage = rageburst.GetInt();
wisdoma = wisdom.GetInt();
train = training.GetInt();
power = (trola + rage + wisdoma + train);
//gpGlobals->mapname;
/*if ( trola >= 1 )
{
if ( trola >= 5 )
{

}
}*/
if ( rage >= 1 )
{
sv_infinite_aux_power.SetValue( "1" );
/*if ( rage >= 5 )
{
}*/
}
if ( wisdoma >= 1 )
{
if ( wisdoma >= 4 )
{
if ( wisdoma >= 12 )
{
#ifdef	GLOWS_ENABLE
pPlayer->AddGlowEffect();
#endif
}
}
}
if ( power >= 1 )
{
//engine->ClientCommand( pPlayer->edict(), "penis" );
//engine->ServerCommand( "penis" );
pPlayer->LevelUp();
}
}
#endif

void desuperpower( void )
{
if ( gamemode.GetInt() == 10 )
{
return;
}
vaild = 0;
#ifdef CLIENT_DLL
C_pPlayer->cpowerdown();
#else
pPlayer->spowerdown();
#endif
}

#ifdef CLIENT_DLL
void C_HL2MP_Player::cpowerdown( void ) // client-side powerdown function xD
{
trola = troll.GetInt();
rage = rageburst.GetInt();
wisdoma = wisdom.GetInt();
train = training.GetInt();
power = (trola + rage + wisdoma + train);
player_throwforce.SetValue( "1000" );
//physcannon_mega_enabled.SetValue( "0" );
/*FMODManager()->IsSoundPlaying( "weapons\powerdown.mp3" ) != true;
FMODManager()->PlayAmbientSound( "weapons\powerdown.mp3", true );*/
}
#endif
#ifdef GAME_DLL
void CHL2MP_Player::spowerdown( void ) // server-side powerdown function xD
{
trola = troll.GetInt();
rage = rageburst.GetInt();
wisdoma = wisdom.GetInt();
train = training.GetInt();
power = (trola + rage + wisdoma + train);
sv_infinite_aux_power.SetValue( "0" );
pPlayer->LevelUp();
#ifdef GLOWS_ENABLE
pPlayer->RemoveGlowEffect();
#endif
}
#endif

#ifdef CLIENT_DLL
static ConCommand powerup( "powerup", superpowers, "Try to power up, DB/DBZ/DBGT STYLE" );
#ifndef Auto-Testing
static ConCommand powerdown( "powerdown", desuperpower, "Compleatly power down, idk why would u wanna do that?" );
#endif
void afk( void )
{
C_pPlayer->afk = 1;
//SetLetUserIdle(REASON_ALTTAB,IDLE_TIMELIMIT1);
materials->ReleaseResources();
}
static ConCommand afkt( "afk", afk, "IM GONNA WIN THIS BATTLE!!!! when i cum back xD... this makes u afk and releases some resources while ur afk ;p" );
void notafk( void )
{
C_pPlayer->afk = 0;
materials->ReacquireResources();
}
static ConCommand notafkt( "notafk", notafk, "IM BACK !!! NOW IM GONNA WIN THIS BATTLE!!!! this re-allocates resoruces since ur not afk anymore ;p" );
/*void cheating( void )
{
engine->ClientCmd( "Say Truuuulllllll" )
ConVar* var = cvar->FindVar("sv_cheats");
if (var)
   var->AddFlags(var->GetFlags() ^ (FCVAR_NOTIFY | FCVAR_REPLICATED));
}
static ConCommand letmecheat( "letmecheat", cheating, "you troll" );
#else
void cheating( void )
{
engine->ServerCommand( "Say Truuuulllllll" )
ConVar* var = cvar->FindVar("sv_cheats");
if (var)
   var->AddFlags(var->GetFlags() ^ (FCVAR_NOTIFY | FCVAR_REPLICATED));
}
static ConCommand letmecheat( "letmecheat", cheating, "you troll" );*/
#endif
#endif