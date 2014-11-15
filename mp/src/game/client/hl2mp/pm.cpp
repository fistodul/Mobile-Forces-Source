#include "cbase.h"
#include "usermessages.h"
#include "utlbuffer.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
CON_COMMAND_F(testum, "SayText", FCVAR_CLIENTCMD_CAN_EXECUTE) {
	CUtlBuffer msg_data;
	msg_data.PutChar(0);
	msg_data.PutString("BAM, SayText");
	msg_data.PutChar(1);
	usermessages->DispatchUserMessage(usermessages->LookupUserMessage("SayText"), bf_read(msg_data.Base(), msg_data.TellPut()));
}