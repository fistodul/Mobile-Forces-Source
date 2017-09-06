#ifndef	BUYZONE_H
#define	BUYZONE_H

#ifdef _WIN32
#pragma once
#endif

class CBuyZone : public CBaseEntity
{
public:
	DECLARE_CLASS(CBuyZone, CBaseEntity);
	DECLARE_DATADESC();

	CBuyZone();
	~CBuyZone();

	virtual void Spawn();

	int	TeamNum;
	static CBuyZone *GetBuyZone(int index);
	static CUtlVector< CBuyZone * >	s_BuyZone;
	static int GetBuyZoneCount();
	void	BuyZoneTouch(CBaseEntity *pOther);
	bool	isinbuyzone;

};

#endif