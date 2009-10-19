/*
============================================================================
 Name        : 	WlanDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Handle data for Wlan resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef WLANDATAHANDLER_H_
#define WLANDATAHANDLER_H_

#include <e32base.h>
#ifdef __SERIES60_3X__
#include <rconnmon.h>
#endif

#include "DataHandler.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TWlanEngineStatus {
	EWlanDisconnected, EWlanDisconnecting, EWlanScanning, EWlanProcessing
};

enum TWlanNotification {
	EWlanNone, EWlanScanFailed, EWlanScanUnavailable, EWlanInUse, EWlanFinished
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MWlanNotification {
	public:
		virtual void WlanData(TDesC8& aMac, TDesC8& aSecurity, TUint aRxLevel) = 0;
		virtual void WlanNotification(TWlanNotification aCode) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CWlanDataHandler : public CDataHandler {
	public:
		CWlanDataHandler(MWlanNotification* aObserver);
		static CWlanDataHandler* NewL(MWlanNotification* aObserver);
		~CWlanDataHandler();
	private:
		void ConstructL();
		
	public:
		TInt AvailableWlanNetworks();
		
	public: // From CDataHandler
		void StartL();
		void Stop();
		TBool IsConnected();
		
	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();

	private:
		MWlanNotification* iObserver;

		TWlanEngineStatus iEngineStatus;
		TInt iWlanNetworkCount;

#ifdef __SERIES60_3X__
		RConnectionMonitor iConnectionMonitor;
		TPckgBuf<TConnMonNetworkNames> iNetworks;
#endif
};

#endif /*WLANDATAHANDLER_H_*/
