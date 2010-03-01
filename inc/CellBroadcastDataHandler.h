/*
============================================================================
 Name        : 	CellTowerDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for Cell Tower Identification
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef CELLBROADCASTDATAHANDLER_H_
#define CELLBROADCASTDATAHANDLER_H_

#include <e32base.h>
#include <etelmm.h>
#include "DataHandler.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TCellBroadcastEngineStatus {
	ECellBroadcastDisconnected, ECellBroadcastDisconnecting,
	ECellBroadcastConnected, ECellBroadcastSetFilter, 
	ECellBroadcastSetLanguage, ECellBroadcastReading
};

enum TCellBroadcastError {
	ECellBroadcastNone, ECellBroadcastUnavailable, ECellBroadcastReadError, ECellBroadcastInUse
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MCellBroadcastNotification {
	public:
		virtual void CellBroadcastData(TDesC8& aCellBroadcastMessage) = 0;
		virtual void CellBroadcastError(TCellBroadcastError aError) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CCellBroadcastDataHandler : public CDataHandler {
	public:
		CCellBroadcastDataHandler(MCellBroadcastNotification* aObserver);
		static CCellBroadcastDataHandler* NewL(MCellBroadcastNotification* aObserver);
		~CCellBroadcastDataHandler();
	private:
		void ConstructL();

	public: // From CDataHandler
		void StartL();
		void Stop();
		TBool IsConnected();

	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();

	private:
		void ProcessData();

	private:
		MCellBroadcastNotification* iObserver;
		TCellBroadcastEngineStatus iEngineStatus;

		RTelServer iServer;
		RMobilePhone iMobileNetworkInfo;

		RMobileBroadcastMessaging iBroadcastMessageInfo;
		RMobileBroadcastMessaging::TGsmBroadcastMessageData iGsmBroadcastMessage;
		RMobileBroadcastMessaging::TCdmaBroadcastMessageData iCdmaBroadcastMessage;

		RMobileBroadcastMessaging::TMobileBroadcastCapsV1 iCaps;

		RMobileBroadcastMessaging::TMobileBroadcastAttributesV1 iBroadcastAttributes;
		RMobileBroadcastMessaging::TMobileBroadcastAttributesV1Pckg iBroadcastAttributesPckg;
};

#endif /*CELLBROADCASTDATAHANDLER_H_*/
