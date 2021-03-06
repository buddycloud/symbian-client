/*
============================================================================
 Name        : 	LocationEngine.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Location Engine for determining the clients location
 		through multiple resources
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef LOCATIONENGINE_H_
#define LOCATIONENGINE_H_

#include <e32base.h>
#include "CellTowerDataHandler.h"
#include "GpsDataHandler.h"
#include "LocationInterfaces.h"
#include "SignalStrengthDataHandler.h"
#include "Timer.h"
#include "TimeUtilities.h"
#include "WlanDataHandler.h"
#include "XmppInterfaces.h"
#include "XmlParser.h"

#ifdef __WINSCW__
#include "CellTowerDataSimulation.h"
#endif

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TLocationEngineState {
	ELocationShutdown, ELocationIdle, ELocationWaitForTimeout, ELocationWaitForResources
};

enum TLocationError {
	ELocationErrorNone, ELocationErrorNoXmppInterface, ELocationErrorBadQueryResult
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CLocationEngine : public CBase, MLocationEngineDataInterface, MXmppStanzaObserver, MTimeoutNotification, 
							MGpsNotification, MCellTowerNotification, MWlanNotification, MSignalStrengthNotification {
	public:
		static CLocationEngine* NewL(MLocationEngineNotification* aEngineObserver);
		static CLocationEngine* NewLC(MLocationEngineNotification* aEngineObserver);
		~CLocationEngine();
		
	private:
		CLocationEngine();
		void ConstructL(MLocationEngineNotification* aEngineObserver);

	public:
		void SetEngineObserver(MLocationEngineNotification* aEngineObserver);
		void SetXmppWriteInterface(MXmppWriteInterface* aXmppInterface);
		void SetTimeInterface(MTimeInterface* aTimeInterface);

	public: // Setup & Config
		void SetLanguageCodeL(TDesC8& aLangCode);
		
		void SetCellActive(TBool aActive);
		void SetGpsActive(TBool aActive);
		void SetWlanActive(TBool aActive);

		void TriggerEngine();
		
	public: // Stop & shutdown
		void StopEngine();
		
		void PrepareShutdown();
		
	public: // From MLocationEngineDataInterface
		TBool CellDataAvailable();
		TBool GpsDataAvailable();
		TBool WlanDataAvailable();
		
	public: // From MLocationEngineDataInterface
		void GetGpsPosition(TReal& aLatitude, TReal& aLongitude);
		
	public: // From MLocationEngineDataInterface
		TLocationMotionState GetLastMotionState();
		TInt GetLastPatternQuality();
		TInt GetLastPlaceId();

	private: 
		TFormattedTimeDesc GetCurrentTime();
	
	private: // Analysis & Transmission
		void InsertCellTowerDataL();
		void InsertSignalStrengthL(TInt32 aSignalStrength);
		void InsertIntoPayload(TInt aPosition, const TDesC8& aString, TPtr8& aLogStanza);
		void BuildLocationPayload();

	public:	// From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	public: // Implemented functions from MCellTowerNotification
		void CellTowerData(TBuf<4> aCountryCode, TBuf<8> aNetworkIdentity, TUint aLocationAreaCode, TUint aCellId);
		void CellTowerError(TCellTowerError aError);

	public: // Implemented functions from MGpsNotification
		void GpsData(TReal aLatitude, TReal aLongitude, TReal aAccuracy);
		void GpsError(TGpsError aError);

	public: // Implemented functions from MWlanNotification
		void WlanData(TDesC8& aMac, TDesC8& aSecurity, TUint aRxLevel);
		void WlanNotification(TWlanNotification aCode);

	public: // Implemented functions from MSignalStrengthNotification
		void SignalStrengthData(TInt32 aSignalStrength, TInt8 aSignalBars);
		void SignalStrengthError(TSignalStrengthError aError);
	
	public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);

	private:
		static const TInt KIdleTimeout     = 180000000;
		static const TInt KResourceTimeout = 40000000;
		static const TInt KShutdownTimeout = 300000;

	private:
		MLocationEngineNotification* iEngineObserver;
		MXmppWriteInterface* iXmppInterface;
		MTimeInterface* iTimeInterface;

		TLocationEngineState iEngineState;

		CCustomTimer* iTimer;
		
		HBufC8* iLangCode;
		HBufC8* iLogStanza;

		// Cell tower resource
		CCellTowerDataHandler* iCellTowerDataHandler;

		class TCellTowerData {
			public:
				TBuf8<4> CountryCode;
				TBuf8<8> NetworkIdentity;
				TUint LocationAreaCode;
				TUint CellId;
		};

		TCellTowerData iLastCellTower;
		
		// Cell Tower Simulation
#ifdef __WINSCW__
		CCellTowerDataSimulation* iCellTowerSimulator;
#endif
		
		// Gps resource
		CGpsDataHandler* iGpsDataHandler;
		
		TReal iGpsLatitude;
		TReal iGpsLongitude;
		TReal iGpsAccuracy;
		
		// Other resources
		CWlanDataHandler* iWlanDataHandler;
		CSignalStrengthDataHandler* iSignalStrengthDataHandler;

		TBool iCellEnabled;
		TBool iGpsEnabled;
		TBool iWlanEnabled;
		
		// Location server results
		TLocationMotionState iMotionState;
		TInt iPatternQuality;
		TInt iPlaceId;
};

#endif /*LOCATIONENGINE_H_*/
