#include "cbase.h"
 
void CC_TestCloak( void )
{
    CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
 
    if ( pPlayer->GetCloakStatus() == 0 )
    {
        pPlayer->SetCloakStatus( 3 );
        Msg( "Cloaking...\n" );
    }
    else if ( pPlayer->GetCloakStatus() == 2 )
    {
        pPlayer->SetCloakStatus( 1 );
        Msg( "Uncloaking...\n" );
    }
    else
        return;
}
 
static ConCommand testcloak( "testcloak", CC_TestCloak );