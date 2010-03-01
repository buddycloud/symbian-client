/*
============================================================================
 Name        : 	SignalStrengthDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for Signal Strength Information
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef SIGNALSTRENGTHDATAHANDLER_H_
#define SIGNALSTRENGTHDATAHANDLER_H_

#include <e32base.h>

#ifndef __SERIES60_3X__
#include <etelmm.h>
#else
#include <Etel3rdParty.h>
#endif

#include "DataHandler.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TSignalStrengthEngineStatus {
	ESignalStrengthDisconnected, ESignalStrengthDisconnecting, ESignalStrengthReading
};

enum TSignalStrengthError {
	ESignalStrengthNone, ESignalStrengthUnavailable, ESignalStrengthReadError
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MSignalStrengthNotification {
	public:
		virtual void SignalStrengthData(TInt32 aSignalStrength, TInt8 aSignalBars) = 0;
		virtual void SignalStrengthError(TSignalStrengthError aError) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CSignalStrengthDataHandler : public CDataHandler {
	public:
		CSignalStrengthDataHandler(MSignalStrengthNotification* aObserver);
		static CSignalStrengthDataHandler* NewL(MSignalStrengthNotification* aObserver);
		~CSignalStrengthDataHandler();
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
		MSignalStrengthNotification* iObserver;

		TSignalStrengthEngineStatus iEngineStatus;

#ifndef __SERIES60_3X__
		RTelServer iServer;
		RMobilePhone iMobileNetworkInfo;

		TInt32 iSignalStrength;
		TInt8 iSignalBars;
#else
		CTelephony* iTelephony;
		CTelephony::TSignalStrengthV1 iSignalStrengthData;
#endif

};

#endif /*SIGNALSTRENGTHDATAHANDLER_H_*/
