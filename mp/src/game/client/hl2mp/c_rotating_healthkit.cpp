#include "cbase.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define ITEM_ROTATION_RATE	( 360.0f / 4.0f )
#define PICKUP_MIN_HEIGHT 50
 
//-----------------------------------------------------------------------------
// Rotating health kit. Heals the player when picked up.
//-----------------------------------------------------------------------------
class C_RotatingPickup : public C_BaseAnimating
{
	DECLARE_CLASS( C_RotatingPickup, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
public:	
	C_RotatingPickup() {
		ClientRotAng = QAngle(45,0,0);
		m_bRespawning = m_bRespawning_Cache = false;
	}
	void Spawn() { ClientThink(); }
 
	bool IsRespawning();
	void ClientThink();
	void PostDataUpdate(DataUpdateType_t updateType);
	bool ShouldDraw();
 
	bool	m_bRespawning;
	bool	m_bRespawning_Cache;
 
private:
	QAngle		ClientRotAng; // m_angRotation is stomped sometimes (CItem returning the ent to spawn position?)
};
 
LINK_ENTITY_TO_CLASS( item_rotating, C_RotatingPickup );
 
IMPLEMENT_CLIENTCLASS_DT( C_RotatingPickup, DT_RotatingPickup,CRotatingPickup )
	RecvPropBool( RECVINFO(m_bRespawning) ),
END_RECV_TABLE()
 
inline bool C_RotatingPickup::IsRespawning()
{
	return m_bRespawning;
}
 
void C_RotatingPickup::ClientThink()
{
	if (IsAbsQueriesValid())
	{
		// Rotate
		ClientRotAng.y += ITEM_ROTATION_RATE * gpGlobals->frametime;
		if ( ClientRotAng.y >= 360 )
			ClientRotAng.y -= 360;
 
		SetAbsAngles( ClientRotAng );
	}
 
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}
 
void C_RotatingPickup::PostDataUpdate(DataUpdateType_t updateType)
{
	if (m_bRespawning_Cache != m_bRespawning)
	{
		// Appear/disappear
		UpdateVisibility();
		ClientRotAng.y = 0;
		m_bRespawning_Cache = m_bRespawning;
	}
 
	return BaseClass::PostDataUpdate(updateType);
}
 
bool C_RotatingPickup::ShouldDraw()
{
	return !IsRespawning() && BaseClass::ShouldDraw();
}