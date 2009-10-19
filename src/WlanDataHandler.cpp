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

// INCLUDE FILES
#ifdef __SERIES60_3X__
#include <e32property.h>
#include <TelephonyInternalPSKeys.h>
#include <wlanmgmtclient.h>
#include <wlanmgmtcommon.h>
#endif

#include "WlanDataHandler.h"

CWlanDataHandler::CWlanDataHandler(MWlanNotification* aObserver) {
	iObserver = aObserver;
}

CWlanDataHandler* CWlanDataHandler::NewL(MWlanNotification* aObserver) {
	CWlanDataHandler* self = new (ELeave) CWlanDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CWlanDataHandler::~CWlanDataHandler() {
}

void CWlanDataHandler::ConstructL() {
	iEngineStatus = EWlanDisconnected;

	CActiveScheduler::Add(this);
}

TInt CWlanDataHandler::AvailableWlanNetworks() {
	return iWlanNetworkCount;
}

void CWlanDataHandler::StartL() {
#ifndef __SERIES60_3X__
	iObserver->WlanNotification(EWlanScanUnavailable);
#else
	if(!IsActive() && iEngineStatus == EWlanDisconnected) {
		// Get line status
		TInt aCallType = EPSTelephonyCallTypeNone;		
		RProperty::Get(KPSUidTelephonyCallHandling, KTelephonyCallType, aCallType);
		
		if(aCallType != EPSTelephonyCallTypeData && aCallType != EPSTelephonyCallTypeVoIP) {
			if(iConnectionMonitor.ConnectL() == KErrNone) {
				iConnectionMonitor.GetPckgAttribute(EBearerIdWLAN, 0, KNetworkNames, iNetworks, iStatus);
	
				iEngineStatus = EWlanScanning;
				iWlanNetworkCount = 0;
				SetActive();
			}
			else {
				iObserver->WlanNotification(EWlanScanUnavailable);
			}
		}
		else {
			iObserver->WlanNotification(EWlanScanUnavailable);
		}
	}
#endif
}

void CWlanDataHandler::Stop() {
#ifdef __SERIES60_3X__
	DoCancel();
#endif
	
	iWlanNetworkCount = 0;
}

TBool CWlanDataHandler::IsConnected() {
	return (iEngineStatus != EWlanDisconnected);
}

void CWlanDataHandler::RunL() {
#ifdef __SERIES60_3X__
	TWlanBssid aBssid;
	TBuf8<32> aMacAddress;
	TBuf8<10> aSecurity;

	switch(iEngineStatus) {
		case EWlanScanning:
			TInt aResult = iStatus.Int();
			if(iStatus == KErrNone) {
				iEngineStatus = EWlanProcessing;

				CWlanScanInfo* iWlanScanInfo=CWlanScanInfo::NewL();
				CleanupStack::PushL(iWlanScanInfo);

				CWlanMgmtClient* iWlanClient=CWlanMgmtClient::NewL();
				CleanupStack::PushL(iWlanClient);

				aResult = iWlanClient->GetScanResults(*iWlanScanInfo);

				if(aResult == KErrNone) {
					for(iWlanScanInfo->First(); !iWlanScanInfo->IsDone(); iWlanScanInfo->Next()) {
						// Add MAC address spacer
						aMacAddress.Zero();

						// Get MAC address
						iWlanScanInfo->Bssid(aBssid);

						for(TInt i = 0; i < aBssid.Length(); i++) {
							if(aMacAddress.Length() > 0) {
								aMacAddress.Append(_L8(":"));
							}

							aMacAddress.AppendFormat(_L8("%02X"), aBssid[i]);
						}

						// Get Security Mode
						switch(iWlanScanInfo->SecurityMode()) {
							case EWlanConnectionSecurityOpen:
								aSecurity.Format(_L8("open"));
								break;
							case EWlanConnectionSecurityWep:
								aSecurity.Format(_L8("wep"));
								break;
							case EWlanConnectionSecurity802d1x:
								aSecurity.Format(_L8("ieee802_1"));
								break;
							case EWlanConnectionSecurityWpa:
								aSecurity.Format(_L8("wpa"));
								break;
							case EWlanConnectionSecurityWpaPsk:
								aSecurity.Format(_L8("wpa_psk"));
								break;
						}

						// Return data
						iWlanNetworkCount++;
						
						iObserver->WlanData(aMacAddress, aSecurity, iWlanScanInfo->RXLevel());
					}
				}

				CleanupStack::PopAndDestroy(2);

				iObserver->WlanNotification(EWlanFinished);

				iConnectionMonitor.Close();
				iEngineStatus = EWlanDisconnected;
			}
			else if(iStatus == KErrNotSupported) {
				iConnectionMonitor.Close();
				iEngineStatus = EWlanDisconnected;
				iObserver->WlanNotification(EWlanScanUnavailable);
			}
			else {
				iConnectionMonitor.Close();
				iEngineStatus = EWlanDisconnected;
				iObserver->WlanNotification(EWlanScanFailed);
			}
			break;
		case EWlanDisconnecting:
			//User::InfoPrint(_L("WIFI Disconnected"));

			iConnectionMonitor.Close();
			iEngineStatus = EWlanDisconnected;
			break;
		default:;
	}
#endif
}

TInt CWlanDataHandler::RunError(TInt /*aError*/) {
	Stop();

	return KErrNone;
}

void CWlanDataHandler::DoCancel() {
#ifdef __SERIES60_3X__
	switch(iEngineStatus) {
		case EWlanScanning:
			iEngineStatus = EWlanDisconnecting;
			iConnectionMonitor.CancelAsyncRequest(EConnMonGetPckgAttribute);
			break;
		default:
			iConnectionMonitor.Close();
			iEngineStatus = EWlanDisconnected;
			break;
	}
#endif
}

// End of File

