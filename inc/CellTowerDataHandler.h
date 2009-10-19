/*
============================================================================
 Name        : 	CellTowerDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Handle data for Cell Tower Identification
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef CELLTOWERDATAHANDLER_H_
#define CELLTOWERDATAHANDLER_H_

#include <e32base.h>

#ifndef __SERIES60_3X__
#include <etelmm.h>
#else
#include <Etel3rdParty.h>
#endif

#include "DataHandler.h"
#include "Timer.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TCellTowerEngineStatus {
	ECellTowerDisconnected, ECellTowerDisconnecting, ECellTowerReading
};

enum TCellTowerError {
	ECellTowerNone, ECellTowerUnavailable, ECellTowerReadError
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MCellTowerNotification {
	public:
		virtual void CellTowerData(TBuf<4> aCountryCode, TBuf<8> aNetworkIdentity, TUint aLocationAreaCode, TUint aCellId) = 0;
		virtual void CellTowerError(TCellTowerError aError) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CCellTowerDataHandler : public CDataHandler, MTimeoutNotification {
	public:
		CCellTowerDataHandler(MCellTowerNotification* aObserver);
		static CCellTowerDataHandler* NewL(MCellTowerNotification* aObserver);
		~CCellTowerDataHandler();

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

	public: // From CCustomTimer
		void TimerExpired(TInt aExpiryId);

	private:
		MCellTowerNotification* iObserver;

		TCellTowerEngineStatus iEngineStatus;

		CCustomTimer* iTimer;

#ifndef __SERIES60_3X__
		RTelServer iServer;
		RMobilePhone iMobileNetworkInfo;

		RMobilePhone::TMobilePhoneNetworkInfoV1 iNetwork;
		RMobilePhone::TMobilePhoneLocationAreaV1 iArea;
#else
		CTelephony* iTelephony;
		CTelephony::TNetworkInfoV1 iCellTowerData;
#endif

};

#endif /*CELLTOWERDATAHANDLER_H_*/
