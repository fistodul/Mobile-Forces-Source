#include "ammodef.h"
#define BUTTER_MODE -1
#define ONE_HIT_MODE -2

#ifdef simulated_bullets

#ifdef CLIENT_DLL//-----------------------
class C_BulletManager;
extern C_BulletManager *g_pBulletManager;
#define CBulletManager C_BulletManager
#define CSimulatedBullet C_SimulatedBullet
#else//-----------------------------------
class CBulletManager;
extern CBulletManager *g_pBulletManager;
#endif//----------------------------------

inline CBulletManager *BulletManager()
{
	return g_pBulletManager;
}

extern ConVar g_debug_bullets;
class CSimulatedBullet
{
public:
	CSimulatedBullet();
	CSimulatedBullet(CBaseEntity *pAttacker, CBaseEntity *pAdditionalIgnoreEnt,
		int ammoType, Vector &vecDirShooting, Vector &vecOrigin,
		bool bTraceHull
#ifndef CLIENT_DLL
		, int nFlags, CBaseEntity *pCaller, int iDamage
#endif
		);
	~CSimulatedBullet();
	inline float BulletSpeedRatio(void)
	{
		return m_flBulletSpeed / m_flInitialBulletSpeed;
	}
	inline bool IsInWorld(void) const
	{
		if (m_vecOrigin.x >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.y >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.z >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.x <= MIN_COORD_INTEGER) return false;
		if (m_vecOrigin.y <= MIN_COORD_INTEGER) return false;
		if (m_vecOrigin.z <= MIN_COORD_INTEGER) return false;
		return true;
	}
	bool StartSolid(trace_t &ptr, Vector &vecNewRay);
	bool AllSolid(trace_t &ptr);
	bool EndSolid(trace_t &ptr);
#ifdef CLIENT_DLL
	bool SimulateBullet(void);
#else
	void EntityImpact(trace_t &ptr);
	bool SimulateBullet(float flLagCompensation = -1);
#endif
	inline int AmmoIndex(void) const
	{
		return m_iAmmoType;
	}
private:
	bool m_bTraceHull;
	bool m_bWasInWater;

	CTraceFilterSkipTwoEntities *m_pTwoEnts;
	CTraceFilterSimpleList *m_pIgnoreList;//already hit
#ifndef CLIENT_DLL
	CUtlVector<CBaseEntity *> m_CompensationConsiderations;//Couldn't resist
#endif

	EHANDLE m_hCaller;
	EHANDLE	m_hLastHit;


	float m_flBulletSpeed;
	float m_flEntryDensity;
	float m_flInitialBulletSpeed;
	float m_flRayLength;

	int m_iAmmoType;
#ifndef CLIENT_DLL
	int m_iDamage;
	FireBulletsFlags_t m_nFlags;
#endif

	Vector m_vecDirShooting;
	Vector m_vecOrigin;
	Vector m_vecEntryPosition;
	Vector m_vecTraceRay;
};
extern CUtlLinkedList<CSimulatedBullet*> g_Bullets;
class CBulletManager : public CBaseEntity
{
	DECLARE_CLASS(CBulletManager, CBaseEntity);
public:
	~CBulletManager()
	{
		g_Bullets.PurgeAndDeleteElements();
	}
#ifdef CLIENT_DLL
	void ClientThink(void);
	int AddBullet(CSimulatedBullet *pBullet);
#else
	void Think(void);
	int AddBullet(CSimulatedBullet *pBullet, float flLatency = 0.0f);
#endif
	unsigned short RemoveBullet(int index);
	void UpdateBulletStopSpeed(void);
	int BulletStopSpeed(void)
	{
		return m_iBulletStopSpeed;
	}
private:
	int m_iBulletStopSpeed;
};
#endif