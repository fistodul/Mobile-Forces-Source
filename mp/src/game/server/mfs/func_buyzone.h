#ifndef	BUYZONE_H
#define	BUYZONE_H

#ifdef _WIN32
#pragma once
#endif

class CBuyZone : public CBaseEntity
{
public:
	DECLARE_CLASS(CBuyZone, CBaseEntity);

	int	TeamNum;
	void	BuyZoneTouch(CBaseEntity *pOther);
	bool	isinbuyzone;

	CBuyZone();

	DECLARE_DATADESC();
};

#endif