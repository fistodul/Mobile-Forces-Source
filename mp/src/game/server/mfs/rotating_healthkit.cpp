#include "cbase.h"
#include "items.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define PICKUP_DECAL "decals/item_base"
#define PICKUP_MODEL "models/items/healthkit.mdl"
#define PICKUP_MIN_HEIGHT 50
int PickupDecalIndex; // set by CRotatingPickup::Precache()
 
#define SF_SUPPRESS_PICKUP_DECAL	0x00000002
 
//-----------------------------------------------------------------------------
// Rotating health kit. Heals the player when picked up.
//-----------------------------------------------------------------------------
class CRotatingPickup : public CItem
{
	DECLARE_CLASS( CRotatingPickup, CItem );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
public:
 
	CRotatingPickup();
	void	Spawn();
	void	Activate();
	void	Precache();
 
	bool	MyTouch( CBasePlayer *pPlayer );
 
	CBaseEntity*	Respawn();
	void			Materialize();
 
	int	m_iHealthToGive;
	float m_fRespawnTime;
	CNetworkVar(bool, m_bRespawning);
 
private:
	Vector MdlTop;
};
 
LINK_ENTITY_TO_CLASS( item_rotating, CRotatingPickup );
 
PRECACHE_REGISTER( item_rotating );
 
BEGIN_DATADESC( CRotatingPickup )
	DEFINE_KEYFIELD( m_iHealthToGive, FIELD_INTEGER, "givehealth"),
	DEFINE_KEYFIELD( m_fRespawnTime, FIELD_FLOAT, "respawntime"),
END_DATADESC()
 
IMPLEMENT_SERVERCLASS_ST( CRotatingPickup, DT_RotatingPickup )
	SendPropBool( SENDINFO( m_bRespawning )),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
END_SEND_TABLE()
 
CRotatingPickup::CRotatingPickup()
{
	if ( m_iHealthToGive <= 0 )
		m_iHealthToGive = 25;
 
	if ( m_fRespawnTime <= 0 )
		m_fRespawnTime = 20;
}
 
void CRotatingPickup::Spawn()
{
	// CItem is designed for Vphys objects, so we need to undo a couple of things its spawn() does
	Vector OriginalLocation = GetAbsOrigin();
		BaseClass::Spawn();
	VPhysicsDestroyObject();
	SetAbsOrigin(OriginalLocation);
 
	UseClientSideAnimation();
	SetModel(PICKUP_MODEL);
 
	SetMoveType(MOVETYPE_NONE);
 
	// Grab the highest point on the model before we change the bounding box
	MdlTop = GetAbsOrigin();
	MdlTop.z += GetModelPtr()->hull_max().z;
 
	SetSolid(SOLID_NONE);
	CollisionProp()->UseTriggerBounds(true,6); // Reign in the volume added to the trigger collision box
	Vector OBBSize = Vector(CollisionProp()->OBBSize().Length() / 2); // need to use length as the model will be rotated at 45 degrees on clients
	SetSize(-OBBSize,OBBSize); // Resize the bounding box
 
	AddEffects(EF_NOSHADOW);	
}
 
void CRotatingPickup::Activate()
{
	BaseClass::Activate();
 
	// Ensure minimum distance above a standable surfare
	trace_t tr;
	UTIL_TraceLine(MdlTop,MdlTop + Vector(0,0,-PICKUP_MIN_HEIGHT),MASK_PLAYERSOLID,this,COLLISION_GROUP_NONE,&tr); // measuring from MdlTop
	if(tr.DidHit())
	{
		if ( !HasSpawnFlags( SF_SUPPRESS_PICKUP_DECAL ) )
			engine->StaticDecal(tr.endpos,PickupDecalIndex,0,0,false); // mark the location of the pickup
		SetAbsOrigin( GetAbsOrigin() + ( Vector(0,0,PICKUP_MIN_HEIGHT*(1-tr.fraction)) ) );
	}
}
 
void CRotatingPickup::Precache()
{
	PrecacheModel( PICKUP_MODEL );
	PrecacheScriptSound( "HealthKit.Touch" );
	PrecacheScriptSound( "AlyxEmp.Charge" );
	PickupDecalIndex = UTIL_PrecacheDecal(PICKUP_DECAL, true );
}
 
// Called from CItem::ItemTouch()
bool CRotatingPickup::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer && pPlayer->GetHealth() < pPlayer->GetMaxHealth() ) 
	{
		pPlayer->TakeHealth( m_iHealthToGive, DMG_GENERIC );
 
		CSingleUserRecipientFilter PlayerFilter( pPlayer );
		PlayerFilter.MakeReliable();
 
		UserMessageBegin( PlayerFilter, "ItemPickup" );
		WRITE_STRING( GetClassname() );
		MessageEnd();
		EmitSound( PlayerFilter, pPlayer->entindex(), "HealthKit.Touch" ); // this should be done by the HUD really
 
		Respawn();
		return true;
	}
 
	return false;
}
 
// Disappear
CBaseEntity* CRotatingPickup::Respawn()
{
	SetTouch(NULL);
	m_bRespawning = true;
 
	SetThink ( &CRotatingPickup::Materialize );
	SetNextThink( gpGlobals->curtime + m_fRespawnTime );
 
	return this;
}
 
// Reappear
void CRotatingPickup::Materialize()
{
	EmitSound("AlyxEmp.Charge");
	m_bRespawning = false;
	SetTouch(&CItem::ItemTouch);
}

CON_COMMAND(create_rotating_healthkit, "You Spin Me Right Round xD") // Spawns the Thing-amagig.
{
	//Same like the InventoryRemoveItem function.
	Vector vecForward;
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	//CBasePlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin()); // FUCK U GetAbsOrigin !!!!
	/*if(!pPlayer)
	{
		Warning("Could not deterime calling player!\n");
		return;
	}*/ // Let everything be able to spawn it XD

	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	CBaseEntity *pEnt = CreateEntityByName( "item_rotating" );
	if ( pEnt )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
		QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);
		pEnt->SetAbsOrigin(vecOrigin);
		pEnt->SetAbsAngles(vecAngles);
		pEnt->Spawn();
		
	}
}