#include "cbase.h" 
#ifdef CLIENT_DLL
	#define CSpawner C_Spawner 
#endif
class CSpawner : public CBaseAnimating
{
	DECLARE_CLASS( CSpawner, CBaseAnimating );
	//DECLARE_NETWORKCLASS(); 
	//DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	int m_iType;
	char item[16];

public:
	//char old_item[16];
	void Create();
	void Spawn( void );
	void Precache( void );
	void Think();
};

ConVar	mp_type1( "mp_type1", "weapon_knife", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 1 areas" );
ConVar	mp_type2( "mp_type2", "weapon_pistol", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 2 areas" );
ConVar	mp_type3( "mp_type3", "weapon_smg3", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 3 areas" );
ConVar	mp_type4( "mp_type4", "weapon_shotgun", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 4 areas" );
ConVar	mp_type5( "mp_type5", "weapon_rpg", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 5 areas" );
ConVar	mp_type6( "mp_type6", "weapon_physcannon", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 6 areas" );
ConVar	mp_type7( "mp_type7", "weapon_hopwire", FCVAR_NOTIFY|FCVAR_REPLICATED, "Item to create at type 7 areas" );

/*IMPLEMENT_NETWORKCLASS_ALIASED( Spawner, DT_Spawner )
BEGIN_NETWORK_TABLE( CSpawner, DT_Spawner )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CSpawner )
END_PREDICTION_DATA()*/

BEGIN_DATADESC( CSpawner )
	DEFINE_KEYFIELD( m_iType, FIELD_INTEGER, "Type" ),
	DEFINE_KEYFIELD( item, FIELD_STRING, "Item" ),
#ifndef CLIENT_DLL
	DEFINE_THINKFUNC( Think ),
#endif
END_DATADESC()

LINK_ENTITY_TO_CLASS(ent_spawner, CSpawner);
PRECACHE_REGISTER(ent_spawner);

void CSpawner::Precache( void )
{
	PrecacheModel ("models/items/combine_rifle_ammo01.mdl"); // set a different model, something you have
    //Precache entities that will be spawned, to prevent late caching(ingame stutters suck)
    UTIL_PrecacheOther(item);
}

void CSpawner::Spawn()
{
	if (strlen(item) < 2)
	{
		if (m_iType==1)
			item=mp_type1.GetString();
		else if (m_iType==2)
			item=mp_type2.GetString();
		else if (m_iType==3)
			item=mp_type3.GetString();
		else if (m_iType==4)
			item=mp_type4.GetString();
		else if (m_iType==5)
			item=mp_type5.GetString();
		else if (m_iType==6)
			item=mp_type6.GetString();
		else if (m_iType==7)
			item=mp_type7.GetString();
		
		//old_item=item;
	}
	
	Precache( );	
	BaseClass::Spawn( );
	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetThink( &CSpawner::Think );
	SetNextThink( gpGlobals->curtime + 3 );
}

void CSpawner::Think()
{	
	/*if (strlen(item) < 2)
	{
		if (m_iType==1)
			if( old_item!=mp_type1.GetString())
			{
				item=mp_type1.GetString();
				old_item=item;
			}
		else if (m_iType==2)
			if( old_item!=mp_type2.GetString())
			{
				item=mp_type2.GetString();
				old_item=item;
			}
		else if (m_iType==3)
			if( old_item!=mp_type3.GetString())
			{
				item=mp_type3.GetString();
				old_item=item;
			}
		else if (m_iType==4)
			if( old_item!=mp_type4.GetString())
			{
				item=mp_type4.GetString();
				old_item=item;
			}
		else if (m_iType==5)
			if( old_item!=mp_type5.GetString())
			{
				item=mp_type5.GetString();
				old_item=item;
			}
		else if (m_iType==6)
			if( old_item!=mp_type6.GetString())
			{
				item=mp_type6.GetString();
				old_item=item;
			}
		else if (m_iType==7)
			if( old_item!=mp_type7.GetString())
			{
				item=mp_type7.GetString();
				old_item=item;
			}
	}*/
		
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, item);
	if (pEntity)
	{
		float flDist = this->GetAbsOrigin().DistTo(pEntity->GetAbsOrigin());
		if (flDist > 60)
			Create();
	}
	else
	{
		Create();
	}
	
	SetNextThink( gpGlobals->curtime + 3 );
}

void CSpawner::Create()
{
	CBaseEntity::Create( item, GetLocalOrigin(), GetLocalAngles() );
	//if (pEntity->HasFlags
		//pEntity->RemoveSpawnFlags( SF_NORESPAWN );
}