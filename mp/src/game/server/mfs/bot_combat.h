//******************************************************************
// Multiplayer AI for Source engine by R_Yell - rebel.y3ll@gmail.com
//******************************************************************

#include "cbase.h"

#ifdef MFS
#include "player.h"
#include "hl2mp_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"
#include "bot_main.h"


bool AcquireEnemy( CSDKBot *pBot )
{
	float minDist = FLT_MAX;
	bool Success = false;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));

		if (HL2MPRules()->IsTeamplay() == true)
		{
			if (pPlayer && pPlayer != NULL && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != pBot->GetTeamNumber())
			{
				float dist = (pBot->GetLocalOrigin() - pPlayer->GetLocalOrigin()).Length();

				if (dist < minDist)
				{
					minDist = dist;
					pBot->hEnemy.Set(pPlayer);
					Success = true;
				}
			}
		}
		else
		{
			if (pPlayer && pPlayer != NULL && pPlayer->IsAlive() && pPlayer != pBot && !pPlayer->IsBot()) // acquiring only human players
			{
				float dist = (pBot->GetLocalOrigin() - pPlayer->GetLocalOrigin()).Length();

				if (dist < minDist)
				{
					minDist = dist;
					pBot->hEnemy.Set(pPlayer);
					Success = true;
				}
			}
		}
	}

	return Success;
}

void BotAttack( CSDKBot *pBot, CUserCmd &cmd )
{

	// EXCEPTIONS
	if( !pBot->m_bEnemyOnSights || !pBot->m_bInRangeToAttack || pBot->m_flNextBotAttack > gpGlobals->curtime )
		return;

	// 
	cmd.buttons |= IN_ATTACK;
	pBot->m_flNextBotAttack = gpGlobals->curtime + 0.75f;

}
#endif
