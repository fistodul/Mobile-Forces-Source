#include "cbase.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
//-----------------------------------------------------------------------------
// Rotating health kit. Heals the player when picked up.
//-----------------------------------------------------------------------------
class C_Holdout : public C_BaseAnimating
{
	DECLARE_CLASS( C_Holdout, C_BaseAnimating );
	//DECLARE_CLIENTCLASS();
public:	
	C_Holdout() {
	}
	void Spawn() { }
 
	//void PostDataUpdate(DataUpdateType_t updateType);
	bool ShouldDraw();
};
 
LINK_ENTITY_TO_CLASS( func_holdout, C_Holdout );
 
/*IMPLEMENT_CLIENTCLASS_DT( C_Holdout, DT_Holdout,CHoldout )
	RecvPropBool( RECVINFO(m_bRespawning) ),
END_RECV_TABLE()*/
 
/*void C_Holdout::PostDataUpdate(DataUpdateType_t updateType)
{
	if (m_bRespawning_Cache != m_bRespawning)
	{
		// Appear/disappear
		UpdateVisibility();
		ClientRotAng.y = 0;
		m_bRespawning_Cache = m_bRespawning;
	}
 
	return BaseClass::PostDataUpdate(updateType);
}*/
 
bool C_Holdout::ShouldDraw()
{
	return BaseClass::ShouldDraw();
}