//******************************************************************
// Multiplayer AI for Source engine by R_Yell - rebel.y3ll@gmail.com
//******************************************************************

#include "cbase.h"

#include "player.h"
#include "hl2mp_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"
#include "bot_main.h"


bool AcquireEnemy( CHL2MP_Bot *pBot )
{
	float minDist = FLT_MAX;
	bool Success = false;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));

		if (HL2MPRules()->IsTeamplay() == true)
		{
			//if (pPlayer && pPlayer != NULL && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != pBot->GetTeamNumber())
			if (pPlayer && pPlayer != NULL && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != pBot->GetTeamNumber() && pPlayer != pBot && !pPlayer->IsBot()) // acquiring only human players
			{
				float dist = (pBot->GetLocalOrigin() - pPlayer->GetLocalOrigin()).Length();

				if (dist < minDist)
				{
					if (minDist == FLT_MAX)
					{
						minDist = dist;
					}
					pBot->hEnemy.Set(pPlayer);
					Success = true;
				}
			}
		}
		else
		{
			//if (pPlayer && pPlayer != NULL && pPlayer->IsAlive())
			if (pPlayer && pPlayer != NULL && pPlayer->IsAlive() && pPlayer != pBot && !pPlayer->IsBot()) // acquiring only human players
			{
				float dist = (pBot->GetLocalOrigin() - pPlayer->GetLocalOrigin()).Length();

				if (dist < minDist)
				{
					if (minDist == FLT_MAX)
					{
						minDist = dist;
					}
					pBot->hEnemy.Set(pPlayer);
					Success = true;
				}
			}
		}
	}

	return Success;
}

void BotAttack( CHL2MP_Bot *pBot, CUserCmd &cmd )
{
#ifdef MFS
	if (!pBot->RunMimicCommand(cmd) && !bot_zombie.GetBool())
	{
#endif
		//EXCEPTIONS
		if (!pBot->m_bEnemyOnSights || pBot->m_flNextBotAttack > gpGlobals->curtime)
			return;

		//Make the bot a savage(more than it already is)
		if (!pBot->m_bInRangeToAttack)
		{
			if (pBot->m_flBotToEnemyDist < 80.0f && pBot->FInViewCone(pBot->GetEnemy()))
			{
				if (pBot->CanSprint() == true)
					pBot->StartSprinting();
				else
					return;
			}
			else
				return;
		}

		if ( pBot->IsSprinting() == false )
		{
			CBasePlayer *pPlayer = pBot->GetEnemy();
#ifdef MFS
			if (pPlayer->MaxSpeed() >= pPlayer->GetSprintSpeed()) //Close in on the bastard, he's trying to escape
#else
			if (pPlayer->MaxSpeed() >= 320) //Close in on the bastard, he's trying to escape
#endif
			{
				if (pBot->CanSprint() == true)
					pBot->StartSprinting();
			}
		}

		cmd.buttons |= IN_ATTACK;
		pBot->m_flNextBotAttack = gpGlobals->curtime + 0.75f;
#ifdef MFS
	}
#endif
}