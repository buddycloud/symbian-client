/*
============================================================================
 Name        : 	BtDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Handle data for Bluetooth resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef BTDATAHANDLER_H_
#define BTDATAHANDLER_H_

#include <e32base.h>
#include <es_sock.h>
#include <bt_sock.h>
#include <blddef.h>

#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
#include <bteng.h>
#else
#include <btengsettings.h>
#endif
#endif

#include "DataHandler.h"
#include "Timer.h"
		
const TInt KMillisecond = 1000000;

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TBtEngineStatus {
	EBtOffline, EBtDisconnected, EBtDisconnecting, EBtPowerUp, EBtScanning, EBtConnected
};

enum TBtNotification {
	EBtNone, EBtScanFailed, EBtScanUnavailable, EBtInUse, EBtFinished
};

const TInt KLaunchTimerId = 0;
const TInt KScanTimerId = 1;

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MBtNotification {
	public:
		virtual void BtData(TDesC8& aMac) = 0;
		virtual void BtNotification(TBtNotification aCode) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CBtDataHandler : public CDataHandler, MTimeoutNotification, MBluetoothPhysicalLinksNotifier
#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
		, MBTMCMSettingsCB
#else
		, MBTEngSettingsObserver
#endif
#endif
{
	public:
		CBtDataHandler(MBtNotification* aObserver);
		static CBtDataHandler* NewL(MBtNotification* aObserver);
		~CBtDataHandler();
	private:
		void ConstructL();

	public:
		void SetLaunch(TInt aSeconds);
		void CollectData();
		TInt AvailableBtDevices();

	public: // From CCustomTimer
		void TimerExpired(TInt aExpiryId);

	public: // From CDataHandler
		void StartL();
		void Stop();
		TBool IsConnected();

	private:
		void GetOwnBtAddress();
		void ConfigureBtSettingsL();
		void ConnectL();
		void PowerDownL();
		void AddToBuffer(TDesC8& aString, TPtr8& aBuffer);

	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();

#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
	public: // From MBTMCMSettingsCB
		void DiscoverabilityModeChangedL(TBTDiscoverabilityMode aMode);
		void PowerModeChangedL(TBool aState);
		void BTAAccessoryChangedL (TBTDevAddr& aAddr);
		void BTAAConnectionStatusChangedL(TBool& aStatus);
#ifdef __SERIES60_31__
		void BTA2dpAccessoryChangedL (TBTDevAddr& aAddr); 
		void BTA2dpConnectionStatusChangedL(TBool& aStatus); 
#endif
		
#else
	public: // From MBTEngSettingsObserver
		void PowerStateChanged(TBTPowerStateValue aState);
		void VisibilityModeChanged(TBTVisibilityMode aState);
#endif
#endif
		
	public: // From MBluetoothPhysicalLinksNotifier
		void HandleCreateConnectionCompleteL(TInt aErr);
		void HandleDisconnectCompleteL(TInt aErr);
		void HandleDisconnectAllCompleteL(TInt aErr);
		
	private:
		MBtNotification* iObserver;

		TBtEngineStatus iEngineStatus;
		TInt iBtDeviceCount;

		CCustomTimer* iLaunchTimer;
		CCustomTimer* iScanTimer;

		// Connection
		RSocketServ iSocketServ;

		// Resolver
		RHostResolver iResolver;
		TProtocolDesc iProtocolDesc;
		TNameEntry iNameEntry;

		TBool iBtPowerState;
		TBool iBtConnectionState;

#ifndef __WINSCW__
#ifndef __3_2_ONWARDS__
		// Settings
		CBTMCMSettings* iBtSettings;

		TBTSearchMode iBtSearchMode;
		TBTDiscoverabilityMode iBtDiscoverabilityMode;
#else
		// Settings
		CBTEngSettings* iBtSettings;
		
		TBTVisibilityMode iBtDiscoverabilityMode;
#endif
#endif

		TBuf8<32> iOwnAddress;
		HBufC8* iBtMacBuffer;
};

#endif /*BTDATAHANDLER_H_*/
