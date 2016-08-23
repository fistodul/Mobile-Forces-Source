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

CBuyZone::CBuyZone(void)
{
	SetTouch(&CBuyZone::BuyZoneTouch);
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