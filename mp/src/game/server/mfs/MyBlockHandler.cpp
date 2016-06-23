#include "cbase.h"
#include "gameinterface.h"
#include "MyBlockHandler.h"
 
static short MYBLOCKHANDLER_SAVE_RESTORE_VERSION = 1;
 
class CMyBlockHandlerSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "MyBlockHandler";
	}
 
	//---------------------------------
 
	void Save( ISave *pSave )
	{
		pSave->StartBlock( "MyBlockHandler" );
 
		char*	Data		=	"\n == OMG OH MY GOD, I JUST GOT SAVED TO A SAVE FILE! ==\n\n";
		size_t	DataLength	=	strlen(Data)+1;
 
		pSave->WriteData( (const char*)&DataLength, sizeof(DataLength) );
		pSave->WriteData( (const char*)Data, (int)DataLength );
 
		pSave->EndBlock();
	}
 
	//---------------------------------
 
	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &MYBLOCKHANDLER_SAVE_RESTORE_VERSION );
	}
 
	//---------------------------------
 
	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		// only load if version matches and if we are loading a game, not a transition
		m_fDoLoad = ( ( version == MYBLOCKHANDLER_SAVE_RESTORE_VERSION ) && 
			( ( MapLoad_LoadGame == gpGlobals->eLoadType ) || ( MapLoad_NewGame == gpGlobals->eLoadType )  ) 
		);
	}
 
	//---------------------------------
 
	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( m_fDoLoad )
		{
			pRestore->StartBlock();
 
			size_t	DataLength;
 
			pRestore->ReadData((char*)&DataLength,sizeof(size_t),0);
 
			char*	Data	=	new char[DataLength];
 
			pRestore->ReadData((char*)Data,(int)DataLength,DataLength);
 
			Color ConsoleColor(100,255,100,255);
			ConColorMsg(ConsoleColor,Data);
 
			delete[] Data;
 
			pRestore->EndBlock();
		}
	}
 
private:
	bool m_fDoLoad;
};
 
//-----------------------------------------------------------------------------
 
CMyBlockHandlerSaveRestoreBlockHandler g_MyBlockHandlerSaveRestoreBlockHandler;
 
//-------------------------------------
 
ISaveRestoreBlockHandler *GetMyBlockHandlerSaveRestoreBlockHandler()
{
	return &g_MyBlockHandlerSaveRestoreBlockHandler;
}