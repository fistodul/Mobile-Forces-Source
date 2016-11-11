#include "cbase.h"
#include "func_buyzone.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CBuyZone)

DEFINE_FIELD(TeamNum, FIELD_INTEGER),

// Function Pointers
DEFINE_FUNCTION(BuyZoneTouch),

END_DATADESC()

LINK_ENTITY_TO_CLASS(func_buyzone, CBuyZone);

CUtlVector< CBuyZone * >	CBuyZone::s_BuyZone;
CBuyZone::CBuyZone(void)
{
	SetTouch(&CBuyZone::BuyZoneTouch);
	s_BuyZone.AddToTail(this);
}

CBuyZone::~CBuyZone()
{
	s_BuyZone.FindAndRemove(this);
}

int CBuyZone::GetBuyZoneCount()
{
	return s_BuyZone.Count();
}

CBuyZone *CBuyZone::GetBuyZone(int index)
{
	if (index < 0 || index >= s_BuyZone.Count())
		return NULL;

	return s_BuyZone[index];
}

void CBuyZone::Spawn()
{
	BaseClass::Spawn();

	// Entity is symbolid
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetCollisionGroup(COLLISION_GROUP_NONE);

	//AddFlag( FL_WORLDBRUSH );
	SetModelName(NULL_STRING);

	// Make entity invisible
	AddEffects(EF_NODRAW);
	// No model but should still network
	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
}

void CBuyZone::BuyZoneTouch(CBaseEntity *pOther)
{
	CBaseEntity *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	if (pOther == pPlayer)
	{
		if (TeamNum != 0)
		{
			if (pPlayer->GetTeamNumber() == TeamNum)
				isinbuyzone = true;
		}
		else
			isinbuyzone = true;
	}
}
