#include "cbase.h"
#ifdef simulated_bullets
#include "util_shared.h"
#include "bullet_manager.h"
#include "shot_manipulator.h"
#include "tier0/vprof.h"

CBulletManager *g_pBulletManager;
CUtlLinkedList<CSimulatedBullet*> g_Bullets;

#ifdef CLIENT_DLL//-------------------------------------------------
#include "engine/ivdebugoverlay.h"
ConVar g_debug_client_bullets("g_debug_client_bullets", "0", FCVAR_CHEAT);
extern void FX_PlayerTracer(Vector& start, Vector& end);
#else//-------------------------------------------------------------
#include "soundent.h"
#include "player_pickup.h"
#include "ilagcompensationmanager.h"
ConVar g_debug_bullets("g_debug_bullets", "0", FCVAR_CHEAT);
#endif//------------------------------------------------------------

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



static void BulletSpeedModifierCallback(ConVar *var, const char *pOldString)
{
	if (var->GetFloat() == 0.0f)
		var->Revert();
}
ConVar sv_bullet_speed_modifier("sv_bullet_speed_modifier", "700.000000", (FCVAR_ARCHIVE | FCVAR_REPLICATED),
	"Density/(This Value) * (Distance Penetrated) = (Change in Speed)",
	BulletSpeedModifierCallback);



static void BulletStopSpeedCallback(ConVar *var, const char *pOldString)
{
	int val = var->GetInt();
	if (val<ONE_HIT_MODE)
		var->Revert();
	else if (BulletManager())
		BulletManager()->UpdateBulletStopSpeed();
}
ConVar sv_bullet_stop_speed("sv_bullet_stop_speed", "40", (FCVAR_ARCHIVE | FCVAR_REPLICATED),
	"Integral speed at which to remove the bullet from the bullet queue\n-1 is butter mode\n-2 is 1 hit mode",
	BulletStopSpeedCallback);



LINK_ENTITY_TO_CLASS(bullet_manager, CBulletManager);



//==================================================
// Purpose:	Constructor
//==================================================
CSimulatedBullet::CSimulatedBullet()
{
	m_vecOrigin.Init();
	m_vecDirShooting.Init();
	m_flInitialBulletSpeed = m_flBulletSpeed = 0;
	m_flEntryDensity = 0.0f;
#ifndef CLIENT_DLL
	m_nFlags = (FireBulletsFlags_t)0;
	m_iDamage = 0;
#endif
	m_iAmmoType = 0;
}


//==================================================
// Purpose:	Constructor
//==================================================
CSimulatedBullet::CSimulatedBullet(CBaseEntity *pAttacker, CBaseEntity *pAdditionalIgnoreEnt,
	int ammoType, Vector &vecDirShooting, Vector &vecOrigin,
	bool bTraceHull
#ifndef CLIENT_DLL
	, int nFlags, CBaseEntity *pCaller, int iDamage
#endif
	)
{
	// Input validation
	Assert(pAttacker);
#ifndef CLIENT_DLL
	Assert(pCaller);
#endif

	// Create a list of entities with which this bullet does not collide.
	m_pIgnoreList = new CTraceFilterSimpleList(COLLISION_GROUP_NONE);
	Assert(m_pIgnoreList);

	// Don't collide with the entity firing the bullet.
	m_pIgnoreList->AddEntityToIgnore(pAttacker);

	// Don't collide with some optionally-specified entity.
	if (pAdditionalIgnoreEnt != NULL)
		m_pIgnoreList->AddEntityToIgnore(pAdditionalIgnoreEnt);

	// Not useful yet... may be useful for effects later
	//m_pTwoEnts = new CTraceFilterSkipTwoEntities(pAttacker,pAdditionalIgnoreEnt,COLLISION_GROUP_NONE);

	// Basic information about the bullet
	m_iAmmoType = ammoType;
	m_flInitialBulletSpeed = m_flBulletSpeed = GetAmmoDef()->GetAmmoOfIndex(ammoType)->bulletSpeed;
	m_vecDirShooting = vecDirShooting;
	m_vecOrigin = vecOrigin;
	m_bTraceHull = bTraceHull;
#ifndef CLIENT_DLL
	m_nFlags = (FireBulletsFlags_t)nFlags;
	m_iDamage = iDamage;
	m_hCaller = pCaller;
#endif
	m_flEntryDensity = 0.0f;
	m_vecTraceRay = vecDirShooting * m_flInitialBulletSpeed;
	m_flRayLength = m_flInitialBulletSpeed;
}



//==================================================
// Purpose:	Deconstructor
//==================================================
CSimulatedBullet::~CSimulatedBullet()
{
	delete m_pIgnoreList;
	delete m_pTwoEnts;
}
#ifdef CLIENT_DLL
bool FX_AffectRagdolls(Vector vecOrigin, Vector vecStart, int iDamageType);


//==================================================
// Purpose:	Simulates a bullet through a ray of its bullet speed
//==================================================
bool C_SimulatedBullet::SimulateBullet(void)
{
	VPROF("C_SimulatedBullet::SimulateBullet");
#else
bool CSimulatedBullet::SimulateBullet(float flLagCompensation/*=-1 (seconds)*/)
{
	VPROF("CSimulatedBullet::SimulateBullet");
#endif

	if (!IsFinite(m_flBulletSpeed))
		return false;//prevent a weird crash

	trace_t trace;
	Vector vecTraceStart(m_vecOrigin);

	Vector vecTraceRay;

	if (m_flRayLength == m_flBulletSpeed)
	{
		vecTraceRay = m_vecTraceRay;
	}
	else
	{
		vecTraceRay = m_vecTraceRay = m_vecDirShooting * m_flBulletSpeed;
		m_flRayLength = m_flBulletSpeed;
	}
#ifdef GAME_DLL
	if (flLagCompensation != -1)
	{
		vecTraceRay *= flLagCompensation * 100;
	}
#endif

	m_vecOrigin += vecTraceRay;
	bool bInWater = (UTIL_PointContents(m_vecOrigin)&MASK_SPLITAREAPORTAL) == 1;
	if (!IsInWorld())
	{
		return false;
	}
#ifdef CLIENT_DLL
	FX_PlayerTracer(vecTraceStart, m_vecOrigin);
#endif
	if (m_bWasInWater != bInWater)
	{
#ifdef CLIENT_DLL
		//TODO: water impact effect
		//CBaseEntity::HandleShotImpactingWater
#endif //CLIENT_DLL
	}
	if (bInWater)
	{
#ifdef CLIENT_DLL
		//TODO: 1 bubble clientside
#endif //CLIENT_DLL
	}

#ifdef CLIENT_DLL
	if (g_debug_client_bullets.GetBool())
	{
		debugoverlay->AddLineOverlay(vecTraceStart, m_vecOrigin, 255, 0, 0, true, 10.0f);
	}
#else
	if (g_debug_bullets.GetBool())
	{
		NDebugOverlay::Line(vecTraceStart, m_vecOrigin, 255, 255, 255, true, 10.0f);
	}
#endif
	m_bWasInWater = bInWater;
	bool bulletSpeedCheck;
	do
	{
		bulletSpeedCheck = false;
		if (m_bTraceHull)
			UTIL_TraceHull(vecTraceStart, m_vecOrigin, Vector(-3, -3, -3), Vector(3, 3, 3), MASK_SHOT, m_pIgnoreList, &trace);
		else
			UTIL_TraceLine(vecTraceStart, m_vecOrigin, MASK_SHOT, m_pIgnoreList, &trace);
		if (!(trace.surface.flags&SURF_SKY))
		{
			if (trace.allsolid)//in solid
			{
				if (!AllSolid(trace))
					return false;
				bulletSpeedCheck = true;
			}
			else if (trace.startsolid)//exit solid
			{
				if (!StartSolid(trace, vecTraceRay))
					return false;
				bulletSpeedCheck = true;
			}
			else if (trace.fraction != 1.0f)//hit solid
			{
				if (!EndSolid(trace))
					return false;
				bulletSpeedCheck = true;
			}
			else
			{
				//don't do a bullet speed check for not touching anything
				break;
			}
		}
		if (bulletSpeedCheck)
		{
			if (m_flBulletSpeed <= BulletManager()->BulletStopSpeed())
			{
				return false;
			}
		}
		vecTraceStart = trace.endpos + m_vecDirShooting;
		if (trace.fraction == 0.0f)
			return false;
	} while (trace.fraction != 1.0f);
	return true;
}


//==================================================
// Purpose:	Simulates when a solid is exited
//==================================================
bool CSimulatedBullet::StartSolid(trace_t &ptr, Vector &vecTraceRay)
{
#ifdef CLIENT_DLL
	//TODO: penetration surface impact stuff
#endif //CLIENT_DLL
	Vector vecExitPosition = ptr.fractionleftsolid * vecTraceRay + ptr.startpos;
	float flPenetrationDistance = m_vecEntryPosition.DistTo(vecExitPosition);
	switch (BulletManager()->BulletStopSpeed())
	{
	case BUTTER_MODE:
	{
		//Do nothing to bullet speed
		break;
	}
	case ONE_HIT_MODE:
	{
		return false;
	}
	default:
	{
		m_flBulletSpeed -= flPenetrationDistance * m_flEntryDensity / sv_bullet_speed_modifier.GetFloat();
		break;
	}
	}
	return true;
}


//==================================================
// Purpose:	Simulates when a solid is being passed through
//==================================================
bool CSimulatedBullet::AllSolid(trace_t &ptr)
{
	switch (BulletManager()->BulletStopSpeed())
	{
	case BUTTER_MODE:
	{
		//Do nothing to bullet speed
		break;
	}
	case ONE_HIT_MODE:
	{
		return false;
	}
	default:
	{
		m_flBulletSpeed -= m_flBulletSpeed * m_flEntryDensity / sv_bullet_speed_modifier.GetFloat();
		break;
	}
	}
	return true;
}


//==================================================
// Purpose:	Simulate when a surface is hit
//==================================================
bool CSimulatedBullet::EndSolid(trace_t &ptr)
{
	m_vecEntryPosition = ptr.endpos;
#ifndef CLIENT_DLL
	int soundEntChannel = (m_nFlags&FIRE_BULLETS_TEMPORARY_DANGER_SOUND) ? SOUNDENT_CHANNEL_BULLET_IMPACT : SOUNDENT_CHANNEL_UNSPECIFIED;

	CSoundEnt::InsertSound(SOUND_BULLET_IMPACT, m_vecEntryPosition, 200, 0.5, m_hCaller, soundEntChannel);
#endif
	if (FStrEq(ptr.surface.name, "tools/toolsblockbullets"))
	{
		return false;
	}
#ifdef CLIENT_DLL
	int iDamageType = GetAmmoDef()->DamageType(m_iAmmoType);
	FX_AffectRagdolls(ptr.startpos, ptr.endpos, iDamageType);
	UTIL_ImpactTrace(&ptr, iDamageType);
#endif //CLIENT_DLL
	m_flEntryDensity = physprops->GetSurfaceData(ptr.surface.surfaceProps)->physics.density;
	if (ptr.DidHitNonWorldEntity())
	{
		if (ptr.m_pEnt != m_hLastHit)
		{
			if (m_pIgnoreList->ShouldHitEntity(ptr.m_pEnt, MASK_SHOT))
			{
				DevMsg("Hit: %s\n", ptr.m_pEnt->GetClassname());
#ifndef CLIENT_DLL
				EntityImpact(ptr);
#endif
			}
		}
		else
			m_pIgnoreList->AddEntityToIgnore(ptr.m_pEnt);
		m_hLastHit = ptr.m_pEnt;
	}
	if (BulletManager()->BulletStopSpeed() == ONE_HIT_MODE)
	{
		return false;
	}
	return true;
}
#ifndef CLIENT_DLL



//==================================================
// Purpose:	Entity impact procedure
//==================================================
void CSimulatedBullet::EntityImpact(trace_t &ptr)
{
	//TODO: entity impact stuff
	/*CTakeDamageInfo dmgInfo( this, pAttacker, flActualDamage, nActualDamageType );
	CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, vecDir, tr.endpos );
	dmgInfo.ScaleDamageForce( info.m_flDamageForceScale );
	dmgInfo.SetAmmoType( info.m_iAmmoType );
	tr.m_pEnt->DispatchTraceAttack( dmgInfo, vecDir, &tr );*/
	if (GetAmmoDef()->Flags(m_iAmmoType) & AMMO_FORCE_DROP_IF_CARRIED)
	{
		// Make sure if the player is holding this, he drops it
		Pickup_ForcePlayerToDropThisObject(ptr.m_pEnt);
	}
}


//==================================================
// Purpose:	Simulates all bullets every centisecond
//==================================================
void CBulletManager::Think(void)
#else
void CBulletManager::ClientThink(void)
#endif
{
	unsigned short iNext = 0;
	for (unsigned short i = g_Bullets.Head(); i != g_Bullets.InvalidIndex(); i = iNext)
	{
		iNext = g_Bullets.Next(i);
		if (!g_Bullets[i]->SimulateBullet())
			RemoveBullet(i);
	}
	if (g_Bullets.Head() != g_Bullets.InvalidIndex())
	{
#ifdef CLIENT_DLL
		SetNextClientThink(gpGlobals->curtime + 0.01f);
#else
		SetNextThink(gpGlobals->curtime + 0.01f);
#endif
	}
}


//==================================================
// Purpose:	Called by sv_bullet_stop_speed callback to keep track of resources
//==================================================
void CBulletManager::UpdateBulletStopSpeed(void)
{
	m_iBulletStopSpeed = sv_bullet_stop_speed.GetInt();
}


//==================================================
// Purpose:	Add bullet to linked list
//			Handle lag compensation with "prebullet simulation"
// Output:	Bullet's index (-1 for error)
//==================================================
#ifdef CLIENT_DLL
int CBulletManager::AddBullet(CSimulatedBullet *pBullet)
#else
int CBulletManager::AddBullet(CSimulatedBullet *pBullet, float flLatency/*=0.0f*/)
#endif
{
	if (pBullet->AmmoIndex() == -1)
	{
		DevMsg("ERROR: Undefined ammo type!\n");
		return -1;
	}
	unsigned short index = g_Bullets.AddToTail(pBullet);
#ifdef CLIENT_DLL
	DevMsg("Client Bullet Created (%i)\n", index);
	if (g_Bullets.Count() == 1)
	{
		SetNextClientThink(gpGlobals->curtime + 0.01f);
	}
#else
	DevMsg("Bullet Created (%i) LagCompensation %f\n", index, flLatency);
	if (flLatency != 0.0f)
		g_Bullets[index]->SimulateBullet(flLatency);
	if (g_Bullets.Count() == 1)
	{
		SetNextThink(gpGlobals->curtime + 0.01f);
	}
#endif
	return index;
}


//==================================================
// Purpose:	Remove the bullet at index from the linked list
// Output:	Next index
//==================================================
unsigned short CBulletManager::RemoveBullet(int index)
{
	unsigned short retVal = g_Bullets.Next(index);
#ifdef CLIENT_DLL
	DevMsg("Client ");
#endif
	DevMsg("Bullet Removed (%i)\n", index);
	g_Bullets.Remove(index);
	if (g_Bullets.Count() == 0)
	{
#ifdef CLIENT_DLL
		SetNextClientThink(TICK_NEVER_THINK);
#else
		SetNextThink(TICK_NEVER_THINK);
#endif
	}
	return retVal;
}
#endif