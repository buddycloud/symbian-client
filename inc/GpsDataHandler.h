/*
============================================================================
 Name        : 	GpsDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for Gps resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef GPSDATAHANDLER_H_
#define GPSDATAHANDLER_H_

#include <e32base.h>

#ifndef __SERIES60_3X__
#include <es_sock.h>
#include <bt_sock.h>
#include <e32math.h>
#else
#include <lbs.h>
#include <lbscriteria.h>
#include <lbsposition.h>
#include <lbscommon.h>
#endif

#include "DataHandler.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TGpsEngineStatus {
	EGpsDisconnected, EGpsDisconnecting, EGpsResolving,
	EGpsConnecting, EGpsConnected, EGpsReading
};

enum TGpsError {
	EGpsNone, EGpsConnectionFailed, EGpsDeviceUnavailable, EGpsInUse
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MGpsNotification {
	public:
		virtual void GpsData(TReal aLatitude, TReal aLongitude, TReal aAccuracy) = 0;
		virtual void GpsError(TGpsError aError) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CGpsDataHandler : public CDataHandler {
	public:
		CGpsDataHandler(MGpsNotification* aObserver);
		static CGpsDataHandler* NewL(MGpsNotification* aObserver);
		~CGpsDataHandler();
		
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
		void ResolveAndConnectL();

#ifndef __SERIES60_3X__
		TReal ParseCoordinateData();
		void ProcessData();
#endif

	private:
		MGpsNotification* iObserver;

		TGpsEngineStatus iEngineStatus;
		TBool iGpsWarmedUp;

#ifndef __SERIES60_3X__
		// Connection
		RSocketServ iSocketServ;
		RSocket iSocket;
		TBTSockAddr iBTAddr;

		// Resolver
		RHostResolver iResolver;
		TProtocolDesc iProtocolDesc;
		TNameEntry iNameEntry;
		TBool iGpsDeviceResolved;
			
		// Data
		TBuf8<1> iChar;
		HBufC8* iBuffer;
#else
		RPositionServer iPositionServ;
		RPositioner iPositioner;

		TPositionInfo iPositionInfo;
#endif
};

#endif /*GPSDATAHANDLER_H_*/
