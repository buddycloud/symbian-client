/*
============================================================================
 Name        : 	DataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for a Resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "DataHandler.h"

CDataHandler::CDataHandler() : CActive(EPriorityStandard) {
}

void CDataHandler::SetWaiting(TBool aWaiting) {
	iWaiting = aWaiting;
}

TBool CDataHandler::IsWaiting() {
	return iWaiting;
}

// End of File
