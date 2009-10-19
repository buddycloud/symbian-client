/*
============================================================================
 Name        : 	ConnectionMonitor.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Monitor when data connections go up & down
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <nifvar.h>
#include "ConnectionMonitor.h"

CConnectionMonitor::CConnectionMonitor(MConnectionMonitorNotification* aObserver) : CActive(EPriorityStandard) {
	iObserver = aObserver;
}

CConnectionMonitor* CConnectionMonitor::NewL(MConnectionMonitorNotification* aObserver) {
	CConnectionMonitor* self = new (ELeave) CConnectionMonitor(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CConnectionMonitor::~CConnectionMonitor() {
	Cancel();

	iConnectionMonitor.CancelNotifications();
	iConnectionMonitor.Close();

	iConnections.Close();
}

void CConnectionMonitor::ConstructL() {
	TBuf8<256> aPrint;
	TInt aResult = KErrNone;
	
	TRAPD(aErr, aResult = iConnectionMonitor.ConnectL());
	
	if(aErr == KErrNone) {
		aPrint.Format(_L8("CM    RConnectionMonitor.ConnectL %d"), aErr);
		iObserver->ConnectionMonitorDebug(aPrint);
		
		if(aResult == KErrNone) {
			iConnectionMonitor.NotifyEventL(*this);
			CActiveScheduler::Add(this);
		}
	}
}

TInt CConnectionMonitor::GetNetworkStatus() {
	return iNetworkStatus;
}

TBool CConnectionMonitor::ConnectionBearerMobile(TUint aConnectionId) {
	for(TInt i = 0; i < iConnections.Count(); i++) {
		if(iConnections[i].iConnectionId == aConnectionId) {
			switch(iConnections[i].iBearer) {
				case EBearerLAN:
				case EBearerWLAN:
				case EBearerBluetooth:
				case EBearerExternalLAN:
				case EBearerExternalWLAN:
				case EBearerExternalBluetooth:
					return false;
					break;
				default:
					return true;
					break;
			}
		}
	}

	return false;
}

void CConnectionMonitor::EventL(const CConnMonEventBase& aEvent) {
	TInt aEventId = aEvent.EventType();
	TUint aConnectionId = aEvent.ConnectionId();

	TBuf8<256> aPrint;

	if(aEventId == EConnMonCreateConnection) { // 1
		aPrint.Format(_L8("CM    EConnMonCreateConnection %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		// New Connection
		CConnection aNewConnection;
		aNewConnection.iConnectionId = aConnectionId;
		iConnections.Append(aNewConnection);

		iObserver->ConnectionCreated(aConnectionId);

		// Cancel existing
		iConnectionRequest = EConnectionRequestNone;
		Cancel();

		// Request Information
		if(!IsActive()) {
			iRequestConnectionId = aConnectionId;
			iConnectionRequest = EConnectionRequestBearer;

			iConnectionMonitor.GetIntAttribute(aConnectionId, 0, KBearer, iTIntRequestResult, iStatus);
			SetActive();
		}
	}
	else if(aEventId == EConnMonDeleteConnection) { // 2
		aPrint.Format(_L8("CM    EConnMonDeleteConnection %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonDeleteConnection* aEventDetail = (CConnMonDeleteConnection*)& aEvent;

		aPrint.Format(_L8("      D/l %d, U/l %d, Auth %d"), aEventDetail->DownlinkData(), aEventDetail->UplinkData(), aEventDetail->AuthoritativeDelete());
		iObserver->ConnectionMonitorDebug(aPrint);

		// Delete Connection
		if(iRequestConnectionId == aConnectionId) {
			iConnectionRequest = EConnectionRequestNone;
			Cancel();
		}

		for(TInt i = 0; i < iConnections.Count(); i++) {
			if(iConnections[i].iConnectionId == aConnectionId) {
				iConnections.Remove(i);

				iObserver->ConnectionDeleted(aConnectionId);
				break;
			}
		}
	}
	else if(aEventId == EConnMonNetworkStatusChange) { // 7
		aPrint.Format(_L8("CM    EConnMonNetworkStatusChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonNetworkStatusChange* aEventDetail = (CConnMonNetworkStatusChange*)& aEvent;

		aPrint.Format(_L8("      Status %d"), aEventDetail->NetworkStatus());
		iObserver->ConnectionMonitorDebug(aPrint);

		// Network Status Change
		iNetworkStatus = aEventDetail->NetworkStatus();
		iObserver->ConnectionStatusChanged();
	}
	else if(aEventId == EConnMonConnectionStatusChange) { // 8
		aPrint.Format(_L8("CM    EConnMonConnectionStatusChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonConnectionStatusChange * aEventDetail = (CConnMonConnectionStatusChange *)& aEvent;

		aPrint.Format(_L8("      SubId %d, Status %d"), aEventDetail->SubConnectionId(), aEventDetail->ConnectionStatus());
		iObserver->ConnectionMonitorDebug(aPrint);

		// Connection Status Change
		for(TInt i = 0; i < iConnections.Count(); i++) {
			if(iConnections[i].iConnectionId == aConnectionId) {
				switch(aEventDetail->ConnectionStatus()) {
					case KConnectionOpen:
					case KLinkLayerClosed:
						iConnections[i].iNetworkStatus = EConnMonStatusUnattached;
						break;
					case KConfigDaemonFinishedRegistration:
						iConnections[i].iNetworkStatus = EConnMonStatusAttached;
						break;
					case KConfigDaemonStartingDeregistration:
					case KDataTransferTemporarilyBlocked:
						iConnections[i].iNetworkStatus = EConnMonStatusSuspended;
						break;
					case KLinkLayerOpen:
						iConnections[i].iNetworkStatus = EConnMonStatusActive;
						break;
					default:
						break;
				}

				iObserver->ConnectionStatusChanged();
			}
		}
	}
	else if(aEventId == EConnMonConnectionActivityChange) { // 9
		aPrint.Format(_L8("CM    EConnMonConnectionActivityChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonConnectionActivityChange * aEventDetail = (CConnMonConnectionActivityChange *)& aEvent;

		aPrint.Format(_L8("      SubId %d, Act %d"), aEventDetail->SubConnectionId(), aEventDetail->ConnectionActivity());
		iObserver->ConnectionMonitorDebug(aPrint);
	}
	else if(aEventId == EConnMonNetworkRegistrationChange) { // 10
		aPrint.Format(_L8("CM    EConnMonNetworkRegistrationChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonNetworkRegistrationChange * aEventDetail = (CConnMonNetworkRegistrationChange *)& aEvent;

		aPrint.Format(_L8("      Status %d"), aEventDetail->RegistrationStatus());
		iObserver->ConnectionMonitorDebug(aPrint);
	}
	else if(aEventId == EConnMonBearerChange) { // 11
		aPrint.Format(_L8("CM    EConnMonBearerChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonBearerChange * aEventDetail = (CConnMonBearerChange *)& aEvent;

		aPrint.Format(_L8("      Bearer %d"), aEventDetail->Bearer());
		iObserver->ConnectionMonitorDebug(aPrint);
	}
	else if(aEventId == EConnMonBearerAvailabilityChange) { // 13
		aPrint.Format(_L8("CM    EConnMonBearerAvailabilityChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonBearerAvailabilityChange * aEventDetail;
		aEventDetail = (CConnMonBearerAvailabilityChange *)& aEvent;
		TBool aAvailability = aEventDetail->Availability();

		aPrint.Format(_L8("      Avail %d"), aAvailability);
		iObserver->ConnectionMonitorDebug(aPrint);
	}
	else if(aEventId == EConnMonIapAvailabilityChange) { // 14
		aPrint.Format(_L8("CM    EConnMonIapAvailabilityChange %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);

		CConnMonIapAvailabilityChange * aEventDetail;
		aEventDetail = (CConnMonIapAvailabilityChange *)& aEvent;
		TConnMonIapInfo aIapAvailability = aEventDetail->IapAvailability();

		aPrint.Format(_L8("      IAPs: "));

		for(TUint i = 0;i < aIapAvailability.iCount; i++) {
			if(aPrint.Length() <= aPrint.MaxLength() - 6) {
				aPrint.AppendFormat(_L8("%d, "), aIapAvailability.iIap[i].iIapId);
			}
		}
		iObserver->ConnectionMonitorDebug(aPrint);
	}
	else if(aEventId == 19) { // 19
		aPrint.Format(_L8("CM    EConnMonPacketDataAvailable %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);
			
		// Data Packet Available
		iNetworkStatus = EConnMonStatusActive;
		iObserver->ConnectionStatusChanged();
	}
	else if(aEventId == 20) { // 20
		aPrint.Format(_L8("CM    EConnMonPacketDataUnavailable %d"), aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);
		
		// Data Packet Unavailable
		iNetworkStatus = EConnMonStatusSuspended;
		iObserver->ConnectionStatusChanged();
	}
	else {
		aPrint.Format(_L8("CM    Event %d: Connection %d"), aEventId, aConnectionId);
		iObserver->ConnectionMonitorDebug(aPrint);
	}
}

void CConnectionMonitor::DoCancel() {
	switch(iConnectionRequest) {
		case EConnectionRequestIapId:
			iConnectionMonitor.CancelAsyncRequest(EConnMonGetUintAttribute);
			break;
		case EConnectionRequestBearer:
		case EConnectionRequestStatus:
		case EConnectionRequestRegistration:
			iConnectionMonitor.CancelAsyncRequest(EConnMonGetIntAttribute);
			break;
		default:
			break;
	}
}

void CConnectionMonitor::RunL() {
	TBuf8<256> aPrint;

	for(TInt i = 0; i < iConnections.Count(); i++) {
		if(iConnections[i].iConnectionId == iRequestConnectionId) {
			switch(iConnectionRequest) {
				case EConnectionRequestIapId:
					// IAP Id
					if(iStatus == KErrNone) {
						iConnections[i].iIapId = iTUintRequestResult;

						aPrint.Format(_L8("NC    IAP %d"), iTUintRequestResult);
						iObserver->ConnectionMonitorDebug(aPrint);
					}

					iConnectionRequest = EConnectionRequestBearer;
					iConnectionMonitor.GetIntAttribute(iRequestConnectionId, 0, KBearer, iTIntRequestResult, iStatus);
					SetActive();
					break;
				case EConnectionRequestBearer:
					// Bearer
					if(iStatus == KErrNone) {
						iConnections[i].iBearer = iTIntRequestResult;

						aPrint.Format(_L8("NC    Bearer %d"), iTIntRequestResult);
						iObserver->ConnectionMonitorDebug(aPrint);
					}

					iConnectionRequest = EConnectionRequestStatus;
					iConnectionMonitor.GetIntAttribute(iRequestConnectionId, 0, KNetworkStatus, iTIntRequestResult, iStatus);
					SetActive();
					break;
				case EConnectionRequestStatus:
					// Network Status
					if(iStatus == KErrNone) {
						iConnections[i].iNetworkStatus = iTIntRequestResult;

						aPrint.Format(_L8("NC    Net Status %d"), iTIntRequestResult);
						iObserver->ConnectionMonitorDebug(aPrint);
					}

					iConnectionRequest = EConnectionRequestRegistration;
					iConnectionMonitor.GetIntAttribute(iRequestConnectionId, 0, KNetworkRegistration, iTIntRequestResult, iStatus);
					SetActive();
					break;
				case EConnectionRequestRegistration:
					// Network Registration
					if(iStatus == KErrNone) {
						iConnections[i].iNetworkRegistration = iTIntRequestResult;

						aPrint.Format(_L8("NC    Net Reg %d"), iTIntRequestResult);
						iObserver->ConnectionMonitorDebug(aPrint);
					}

					iConnectionRequest = EConnectionRequestNone;
					break;
				default:
					break;
			}

			break;
		}
	}
}

// End of File
