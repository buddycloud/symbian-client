/*
============================================================================
 Name        : 	BtDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Handle data for Bluetooth resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <bt_subscribe.h>
#include <e32property.h>
#include "BtDataHandler.h"

CBtDataHandler::CBtDataHandler(MBtNotification* aObserver) {
	iObserver = aObserver;
}

CBtDataHandler* CBtDataHandler::NewL(MBtNotification* aObserver) {
	CBtDataHandler* self = new (ELeave) CBtDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CBtDataHandler::~CBtDataHandler() {
#ifndef __NO_BT__
	if(iBtSettings) {
		delete iBtSettings;
	}
#endif
	
	if(iLaunchTimer) {
		delete iLaunchTimer;
	}
	
	if(iScanTimer) {
		delete iScanTimer;
	}

	if(iBtMacBuffer) {
		delete iBtMacBuffer;
	}
}

void CBtDataHandler::ConstructL() {
	iEngineStatus = EBtOffline;
	iBtPowerState = true;

#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
	iBtSettings = CBTMCMSettings::NewL(this);
#else
	iBtSettings = CBTEngSettings::NewL(this);
#endif
#endif

	iLaunchTimer = CCustomTimer::NewL(this, KLaunchTimerId);
	iScanTimer = CCustomTimer::NewL(this, KScanTimerId);
	iBtMacBuffer = HBufC8::NewL(64);
	
	CActiveScheduler::Add(this);
}

void CBtDataHandler::SetLaunch(TInt aSeconds) {
	iLaunchTimer->After(aSeconds * KMillisecond);
}

void CBtDataHandler::CollectData() {
	TPtr8 pBtMacBuffer(iBtMacBuffer->Des());
	TBuf8<32> aMacAddress;
	TInt aSearch = KErrNotFound;

	if(pBtMacBuffer.Length() > 0) {
		do {
			// Get next MAC
			aSearch = pBtMacBuffer.Locate(',');

			if(aSearch == KErrNotFound) {
				aMacAddress.Copy(pBtMacBuffer);
				pBtMacBuffer.Zero();
			}
			else {
				aMacAddress.Copy(pBtMacBuffer.Ptr(), aSearch);
				pBtMacBuffer.Delete(0, aSearch+1);
			}

			// Return data
			iObserver->BtData(aMacAddress);
		} while(aSearch != KErrNotFound);
	}

	// Finished
	iObserver->BtNotification(EBtFinished);
}

TInt CBtDataHandler::AvailableBtDevices() {
	return iBtDeviceCount;
}

void CBtDataHandler::TimerExpired(TInt aExpiryId) {
	if(aExpiryId == KLaunchTimerId) {
		iLaunchTimer->After(300 * KMillisecond);

		switch(iEngineStatus) {
			case EBtOffline:
				GetOwnBtAddress();
				break;
			case EBtDisconnected:
				// Power-On Bluetooth
				iEngineStatus = EBtPowerUp;
				iBtConnectionState = false;
	
				// Get Bluetooth settings
#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
				iBtSettings->GetPowerState(iBtPowerState);
				iBtSettings->GetDiscoverabilityMode(iBtDiscoverabilityMode);
				iBtSettings->GetSearchMode(iBtSearchMode);
	
				if( iBtPowerState ) {
					ConfigureBtSettingsL();
				}
				else {
					iBtSettings->SetPowerState(true);
				}
#else
				TBTPowerStateValue aBtPowerState;
				iBtSettings->GetPowerState(aBtPowerState);
				iBtSettings->GetVisibilityMode(iBtDiscoverabilityMode);
				
				iBtPowerState = (aBtPowerState == EBTPowerOn) ? true : false;
				iBtConnectionState = false;
				
				if( iBtPowerState ) {
					ConfigureBtSettingsL();
				}
				else {
					iBtSettings->SetPowerState(EBTPowerOn);
				}
#endif
#endif
	
				// Connect
				if(iBtPowerState) {
					GetOwnBtAddress();
					ConnectL();
				}
	
				// Set Power-Off timeout
				iScanTimer->After(60 * KMillisecond);
				break;
			default:;
		}		
	}
	else {
		switch(iEngineStatus) {
			case EBtDisconnecting:
				PowerDownL();
				break;
			default:
				// Power-Off Bluetooth
				DoCancel();
				break;
		}
	}
}

void CBtDataHandler::StartL() {
	iEngineStatus = EBtDisconnected;
}

void CBtDataHandler::Stop() {
	Cancel();
	PowerDownL();
	iScanTimer->Stop();
	iEngineStatus = EBtOffline;
	iBtDeviceCount = 0;
}

TBool CBtDataHandler::IsConnected() {
	return (iEngineStatus != EBtOffline);
}

void CBtDataHandler::GetOwnBtAddress() {
	// Reset buffer
	TPtr8 pBtMacBuffer(iBtMacBuffer->Des());
	pBtMacBuffer.Zero();

	if(iOwnAddress.Length() == 0) {
		TBuf<32> aOwnAddress;
		TPckgBuf<TBTDevAddr> aBtAddressPckg;
		
		if(RProperty::Get(KUidSystemCategory, KPropertyKeyBluetoothGetLocalDeviceAddress, aBtAddressPckg) == KErrNone) {
			aBtAddressPckg().GetReadable(aOwnAddress, KNullDesC, _L(":"), KNullDesC);
			aOwnAddress.UpperCase();
			
			if(aOwnAddress.Compare(_L("00:00:00:00:00:00")) != 0) {
				iOwnAddress.Copy(aOwnAddress);
			}
		}
	}
	
	// Add own address
	AddToBuffer(iOwnAddress, pBtMacBuffer);
}

void CBtDataHandler::ConfigureBtSettingsL() {
#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
	if(iBtSearchMode != EBTSearchModeGeneral) {
		iBtSettings->SetSearchMode(EBTSearchModeGeneral);
	}

	if(iBtDiscoverabilityMode != EBTDiscModeGeneral) {
		iBtSettings->SetDiscoverabilityModeL(EBTDiscModeGeneral);
	}
#else
	if(iBtDiscoverabilityMode != EBTVisibilityModeGeneral) {
		iBtSettings->SetVisibilityMode(EBTVisibilityModeGeneral);
	}
#endif
#endif
}

void CBtDataHandler::ConnectL() {
	TPtr8 pBtMacBuffer(iBtMacBuffer->Des());
	TInt aResult = KErrInUse;
	
	iBtDeviceCount = 0;

	if(!IsActive() && (aResult = iSocketServ.Connect()) == KErrNone) {
		_LIT(KLinkMan, "BTLinkManager");
		TProtocolName aProtocolName(KLinkMan);

		if((aResult = iSocketServ.FindProtocol(aProtocolName, iProtocolDesc)) == KErrNone) {
			CBluetoothPhysicalLinks* aBtConnections = CBluetoothPhysicalLinks::NewLC(*this, iSocketServ);
			RBTDevAddrArray aBtDeviceArray;
			
			if((aResult = aBtConnections->Enumerate(aBtDeviceArray, 1)) == KErrNone) {
				if(aBtDeviceArray.Count() == 0) {
					if((aResult = iResolver.Open(iSocketServ, iProtocolDesc.iAddrFamily, iProtocolDesc.iProtocol)) == KErrNone) {
						// Create Resolver Inquiry
						TInquirySockAddr aInquiry;
						aInquiry.SetIAC(KGIAC);
						aInquiry.SetAction(KHostResName | KHostResInquiry | KHostResIgnoreCache);
						iResolver.GetByAddress(aInquiry, iNameEntry, iStatus);
		
						iEngineStatus = EBtScanning;
						SetActive();
					}
				}
				else {
					aResult = KErrInUse;
				}
			}
			
			CleanupStack::PopAndDestroy(); // aBtConnections
		}		
	}

	if(aResult != KErrNone) {
		iSocketServ.Close();
		iEngineStatus = EBtPowerUp;
		iObserver->BtNotification(EBtScanFailed);
	}
}

void CBtDataHandler::PowerDownL() {
#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
	if( !iBtPowerState ) {
		iBtSettings->SetPowerState(false);
	}
#else
	if( !iBtPowerState ) {
		iBtSettings->SetPowerState(EBTPowerOff);
	}
#endif
#endif
	
	iEngineStatus = EBtDisconnected;
}

void CBtDataHandler::AddToBuffer(TDesC8& aString, TPtr8& aBuffer) {
	if(aBuffer.Length() + aString.Length() > aBuffer.MaxLength()) {
		iBtMacBuffer = iBtMacBuffer->ReAlloc(aBuffer.MaxLength()+aString.Length());
		aBuffer.Set(iBtMacBuffer->Des());
	}

	aBuffer.Append(aString);
}

void CBtDataHandler::RunL() {
	TPtr8 pBtMacBuffer(iBtMacBuffer->Des());
	TBuf8<32> aMacAddress;

	switch(iEngineStatus) {
		case EBtScanning:
			if(iStatus == KErrNone) {
				// Device found
				TInquirySockAddr aInquiry = TInquirySockAddr::Cast(iNameEntry().iAddr);
				TBTDevAddr aBtAddress = aInquiry.BTAddr();

				// Get MAC address
				for(TInt i = 0; i < aBtAddress.Des().Length(); i++) {
					if(aMacAddress.Length() > 0) {
						aMacAddress.Append(_L8(":"));
					}

					aMacAddress.AppendFormat(_L8("%02X"), aBtAddress[i]);
					
					iBtDeviceCount++;
				}

				if(pBtMacBuffer.Length() > 0) {
					aMacAddress.Insert(0, _L8(","));
				}

				AddToBuffer(aMacAddress, pBtMacBuffer);

				// Get next
				iResolver.Next(iNameEntry, iStatus);
    			SetActive();
			}
			else if(iStatus == KErrHostResNoMoreResults) {
				// No more devices found
				iResolver.Close();
				iSocketServ.Close();
				iEngineStatus = EBtPowerUp;
			}
			else {
				// Error
				iResolver.Close();
				iSocketServ.Close();
				iEngineStatus = EBtPowerUp;
				iObserver->BtNotification(EBtScanUnavailable);
			}
			break;
		case EBtDisconnecting:
			iResolver.Close();
			iSocketServ.Close();
			
			iScanTimer->After(10 * KMillisecond);
			break;
		default:;
	}
}

TInt CBtDataHandler::RunError(TInt /*aError*/) {
	DoCancel();

	return KErrNone;
}

void CBtDataHandler::DoCancel() {
	switch(iEngineStatus) {
		case EBtScanning:
			iEngineStatus = EBtDisconnecting;
			iResolver.Cancel();
			break;
		case EBtPowerUp:
			PowerDownL();
			break;
		default:;
	}
}

#ifndef __NO_BT__
#ifndef __3_2_ONWARDS__
void CBtDataHandler::DiscoverabilityModeChangedL(TBTDiscoverabilityMode /*aMode*/) {
}

void CBtDataHandler::PowerModeChangedL(TBool aState) {
	if(aState) {
		if(iEngineStatus == EBtPowerUp) {
			ConfigureBtSettingsL();
			GetOwnBtAddress();
			ConnectL();
		}
	}
}

void CBtDataHandler::BTAAccessoryChangedL(TBTDevAddr& /*aAddr*/) {
}

void CBtDataHandler::BTAAConnectionStatusChangedL(TBool& /*aStatus*/) {
}

#ifdef __SERIES60_31__
void CBtDataHandler::BTA2dpAccessoryChangedL (TBTDevAddr& /*aAddr*/) {
}

void CBtDataHandler::BTA2dpConnectionStatusChangedL(TBool& /*aStatus*/) {
}
#endif

#else
void CBtDataHandler::PowerStateChanged(TBTPowerStateValue aState) {
	if(aState == EBTPowerOn) {
		if(iEngineStatus == EBtPowerUp) {
			ConfigureBtSettingsL();
			GetOwnBtAddress();
			ConnectL();
		}
	}
}

void CBtDataHandler::VisibilityModeChanged(TBTVisibilityMode /*aState*/) {
}
#endif
#endif

void CBtDataHandler::HandleCreateConnectionCompleteL(TInt /*aErr*/) {
}

void CBtDataHandler::HandleDisconnectCompleteL(TInt /*aErr*/) {	
}

void CBtDataHandler::HandleDisconnectAllCompleteL(TInt /*aErr*/) {
}

// End of File

