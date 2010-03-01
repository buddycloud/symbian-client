/*
============================================================================
 Name        : 	ConnectionMonitor.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Monitor when data connections go up & down
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef CONNECTIONMONITOR_H_
#define CONNECTIONMONITOR_H_

#include <e32base.h>
#include <rconnmon.h>

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/
//
//enum TConnMonBearerType {
//    EBearerUnknown            = 0,
//    EBearerCSD                = 1,  // CSD (GSM)
//    EBearerWCDMA              = 2,  // PSD (WCDMA)
//    EBearerLAN                = 3,
//    EBearerCDMA2000           = 4,
//    EBearerGPRS               = 5,    
//    EBearerHSCSD              = 6,  // HSCSD (GSM)
//    EBearerEdgeGPRS           = 7,
//    EBearerWLAN               = 8,
//    EBearerBluetooth          = 9,
//    EBearerVirtual            = 10,
//    EBearerVirtualVPN         = 11,
//    EBearerWcdmaCSD           = 12, // CSD (WCDMA)
//
//    EBearerExternalCSD        = 30, // ext CSD (GSM)
//    EBearerExternalWCDMA      = 31, // ext PSD (WCDMA)
//    EBearerExternalLAN        = 32,
//    EBearerExternalCDMA2000   = 33,
//    EBearerExternalGPRS       = 34,    
//    EBearerExternalHSCSD      = 35, // ext HSCSD (GSM)
//    EBearerExternalEdgeGPRS   = 36,
//    EBearerExternalWLAN       = 37,
//    EBearerExternalBluetooth  = 38,
//    EBearerExternalWcdmaCSD   = 39  // ext CSD (WCDMA)
//};
//
//// Network status
//enum TConnMonNetworkStatus {
//    EConnMonStatusNotAvailable = 0,
//    EConnMonStatusUnattached,
//    EConnMonStatusAttached,
//    EConnMonStatusActive,
//    EConnMonStatusSuspended
//};
//
//// Network registration status. Valid for CSD, GPRS and WCDMA.
//enum TConnMonNetworkRegistration {
//    ENetworkRegistrationNotAvailable = 0,
//    ENetworkRegistrationUnknown,
//    ENetworkRegistrationNoService,
//    ENetworkRegistrationEmergencyOnly,
//    ENetworkRegistrationSearching,
//    ENetworkRegistrationBusy,
//    ENetworkRegistrationHomeNetwork,
//    ENetworkRegistrationDenied,
//    ENetworkRegistrationRoaming
//};

enum TConnectionRequest {
	EConnectionRequestNone, EConnectionRequestIapId, EConnectionRequestBearer, 
	EConnectionRequestStatus, EConnectionRequestRegistration 
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MConnectionMonitorNotification {
	public:
		virtual void ConnectionCreated(TUint aConnectionId) = 0;
		virtual void ConnectionDeleted(TUint aConnectionId) = 0;		
		virtual void ConnectionStatusChanged() = 0;

		virtual void ConnectionMonitorDebug(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CConnection
--
----------------------------------------------------------------------------
*/

class CConnection {
	public:
		TUint iConnectionId;
		TUint iIapId;
		TInt iBearer;
		TInt iNetworkStatus;
		TInt iNetworkRegistration;		
};

/*
----------------------------------------------------------------------------
--
-- CConnectionMonitor
--
----------------------------------------------------------------------------
*/

class CConnectionMonitor : public CActive, MConnectionMonitorObserver {
	public:
		CConnectionMonitor(MConnectionMonitorNotification* aObserver);
		static CConnectionMonitor* NewL(MConnectionMonitorNotification* aObserver);
		~CConnectionMonitor();

	private:
		void ConstructL();
		
	public:
		TInt GetNetworkStatus();	
		TBool ConnectionBearerMobile(TUint aConnectionId);

	public: // From MConnectionMonitorObserver
		void EventL(const CConnMonEventBase& aEvent);
		
	public:	//Implemented functions from CActive
		void DoCancel();
		void RunL();
		
	private:
		MConnectionMonitorNotification* iObserver;
		
		RConnectionMonitor iConnectionMonitor;
		
		RArray<CConnection> iConnections;		
		TInt iNetworkStatus;
		
		// Requests
		TConnectionRequest iConnectionRequest;
		TUint iRequestConnectionId;
		TUint iTUintRequestResult;
		TInt iTIntRequestResult;
};

#endif /*CONNECTIONMONITOR_H_*/
