/*
============================================================================
 Name        : 	LocationEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Location Engine for determining the clients location
                through multiple resources
============================================================================
*/

#include <e32std.h>
#include "LocationEngine.h"

CLocationEngine* CLocationEngine::NewL(MLocationEngineNotification* aEngineObserver) {
	CLocationEngine* self = NewLC(aEngineObserver);
	CleanupStack::Pop(self);
	return self;

}

CLocationEngine* CLocationEngine::NewLC(MLocationEngineNotification* aEngineObserver) {
	CLocationEngine* self = new (ELeave) CLocationEngine();
	CleanupStack::PushL(self);
	self->ConstructL(aEngineObserver);
	return self;
}

CLocationEngine::~CLocationEngine() {
	if(iGpsDataHandler)
		delete iGpsDataHandler;

	if(iCellTowerDataHandler)
		delete iCellTowerDataHandler;
	
#ifdef __WINSCW__
	if(iCellTowerSimulator)
		delete iCellTowerSimulator;
#endif

	if(iWlanDataHandler)
		delete iWlanDataHandler;

	if(iSignalStrengthDataHandler)
		delete iSignalStrengthDataHandler;

	if(iTimer)
		delete iTimer;

	if(iLogStanza)
		delete iLogStanza;
	
	if(iLangCode)
		delete iLangCode;
}

CLocationEngine::CLocationEngine() {
	iCellEnabled = false;
	iGpsEnabled = false;
	iWlanEnabled = false;
	
	iGpsLatitude = 0.0;
	iGpsLongitude = 0.0;
	iGpsAccuracy = 0.0;
	
	iPlaceId = 0;
	iPatternQuality = 0;
	iMotionState = EMotionMoving;
}

void CLocationEngine::ConstructL(MLocationEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
	iXmppInterface = NULL;
	iTimeInterface = NULL;
	
	iLangCode = HBufC8::NewL(0);
	iLogStanza = HBufC8::NewL(512);

	iCellTowerDataHandler = CCellTowerDataHandler::NewL(this);
	iSignalStrengthDataHandler = CSignalStrengthDataHandler::NewL(this);
	iWlanDataHandler = CWlanDataHandler::NewL(this);
	iGpsDataHandler = CGpsDataHandler::NewL(this);

	iEngineState = ELocationIdle;
	iTimer = CCustomTimer::NewL(this);

#ifdef __WINSCW__
	iCellTowerSimulator = CCellTowerDataSimulation::NewL(this);
#endif
}

void CLocationEngine::SetEngineObserver(MLocationEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

void CLocationEngine::SetXmppWriteInterface(MXmppWriteInterface* aXmppInterface) {
	iXmppInterface = aXmppInterface;
}

void CLocationEngine::SetTimeInterface(MTimeInterface* aTimeInterface) {
	iTimeInterface = aTimeInterface;
}

void CLocationEngine::SetLanguageCodeL(TDesC8& aLangCode) {
	if(iLangCode)
		delete iLangCode;
	
	iLangCode = aLangCode.AllocL();
}

void CLocationEngine::SetCellActive(TBool aActive) {
	if(iCellEnabled != aActive) {
		iCellEnabled = aActive;
	
		if(iEngineState > ELocationIdle) {
			if( !iCellEnabled ) {
				iCellTowerDataHandler->SetWaiting(false);
				iCellTowerDataHandler->Stop();
				
				iSignalStrengthDataHandler->SetWaiting(false);
				iSignalStrengthDataHandler->Stop();
			}
			else {
#ifndef __WINSCW__
				iLastCellTower.CellId = 0;
				iLastCellTower.LocationAreaCode = 0;
	
				iCellTowerDataHandler->StartL();
#else
				iCellTowerSimulator->StartL();
#endif
			}
		}
	}
}

void CLocationEngine::SetGpsActive(TBool aActive) {
	if(iGpsEnabled != aActive) {
		iGpsEnabled = aActive;
	
		if(iEngineState > ELocationIdle) {
			if( !iGpsEnabled ) {
				iGpsDataHandler->SetWaiting(false);
				iGpsDataHandler->Stop();
				
				iGpsLatitude = 0.0;
				iGpsLongitude = 0.0;
				iGpsAccuracy = 0.0;
			}
			else if(iGpsEnabled) {
				TriggerEngine();
			}
		}
	}
}

void CLocationEngine::SetWlanActive(TBool aActive) {
	if(iWlanEnabled != aActive) {
		iWlanEnabled = aActive;
	
		if(iEngineState > ELocationIdle) {
			if( !iWlanEnabled ) {
				iWlanDataHandler->SetWaiting(false);
				iWlanDataHandler->Stop();
			}
			else if(iWlanEnabled) {
				TriggerEngine();
			}
		}
	}
}

void CLocationEngine::TriggerEngine() {
	if(iCellEnabled && iEngineState == ELocationIdle) {
#ifndef __WINSCW__
		iLastCellTower.CellId = 0;
		iLastCellTower.LocationAreaCode = 0;
	
		iCellTowerDataHandler->StartL();
#else
		iCellTowerSimulator->StartL();
#endif
	}
	else if(iEngineState > ELocationShutdown) {
		iEngineState = ELocationWaitForTimeout;
		TimerExpired(0);
	}
}

void CLocationEngine::StopEngine() {
	if(iEngineState > ELocationShutdown) {
		iTimer->Stop();
		iEngineState = ELocationIdle;
		
		iCellTowerDataHandler->SetWaiting(false);	
		iSignalStrengthDataHandler->SetWaiting(false);
		iGpsDataHandler->SetWaiting(false);
		iWlanDataHandler->SetWaiting(false);
		
		iCellTowerDataHandler->Stop();
		iSignalStrengthDataHandler->Stop();
		iGpsDataHandler->Stop();
		iWlanDataHandler->Stop();
		
		iGpsLatitude = 0.0;
		iGpsLongitude = 0.0;
		iGpsAccuracy = 0.0;
	}
}

void CLocationEngine::PrepareShutdown() {
	if(iEngineState > ELocationShutdown) {
		StopEngine();
		
		iEngineState = ELocationShutdown;
		iTimer->After(KShutdownTimeout);
	}
	else {
		iEngineObserver->LocationShutdownComplete();
	}
}

TBool CLocationEngine::CellDataAvailable() {
	return (iLastCellTower.CellId != 0 && iLastCellTower.LocationAreaCode != 0);
}

TBool CLocationEngine::GpsDataAvailable() {
	return (iGpsLatitude != 0.0 || iGpsLatitude != 0.0);
}

TBool CLocationEngine::WlanDataAvailable() {
	return iWlanDataHandler->AvailableWlanNetworks();
}

void CLocationEngine::GetGpsPosition(TReal& aLatitude, TReal& aLongitude) {
	aLatitude = iGpsLatitude;
	aLongitude = iGpsLongitude;
}

TLocationMotionState CLocationEngine::GetLastMotionState() {
	return iMotionState;
}

TInt CLocationEngine::GetLastPatternQuality() {
	return iPatternQuality;
}

TInt CLocationEngine::GetLastPlaceId() {
	return iPlaceId;
}

TFormattedTimeDesc CLocationEngine::GetCurrentTime() {
	CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
	TFormattedTimeDesc aResult;
	TTime aTime;

	if(iTimeInterface) {
		aTime = iTimeInterface->GetTime();
	}
	else {
		aTime.UniversalTime();
	}
	
	aTimeUtilities->EncodeL(aTime, aResult);
	CleanupStack::PopAndDestroy(); // aTimeUtilities
	
	return aResult;
}

void CLocationEngine::InsertCellTowerDataL() {	
	_LIT8(KCellData, "<reference><id>::%d:%d</id><type>cell</type><signalstrength>$SIG</signalstrength></reference>");
	HBufC8* aCellData = HBufC8::NewLC(KCellData().Length() + iLastCellTower.NetworkIdentity.Length() + iLastCellTower.CountryCode.Length() + 16);
	TPtr8 pCellData(aCellData->Des());
	pCellData.Format(KCellData, iLastCellTower.LocationAreaCode, iLastCellTower.CellId);
	pCellData.Insert(16, iLastCellTower.NetworkIdentity);
	pCellData.Insert(15, iLastCellTower.CountryCode);

	TPtr8 pLogStanza(iLogStanza->Des());
	InsertIntoPayload(pLogStanza.Length() - 23, pCellData, pLogStanza);
	CleanupStack::PopAndDestroy();
}

void CLocationEngine::InsertSignalStrengthL(TInt32 aSignalStrength) {
	TPtr8 pLogStanza(iLogStanza->Des());
	TInt aResult = pLogStanza.Find(_L8("$SIG"));
	
	if(aResult != KErrNotFound) {
		TBuf8<8> aSignal;
		aSignal.Format(_L8("%d"), aSignalStrength);
		pLogStanza.Replace(aResult, 4, aSignal);
	}
}

void CLocationEngine::InsertIntoPayload(TInt aPosition, const TDesC8& aString, TPtr8& aLogStanza) {
	if(aLogStanza.Length() + aString.Length() > aLogStanza.MaxLength()) {
		iLogStanza = iLogStanza->ReAlloc(aLogStanza.MaxLength() + aString.Length() + 32);
		aLogStanza.Set(iLogStanza->Des());
	}

	aLogStanza.Insert(aPosition, aString);
}

void CLocationEngine::BuildLocationPayload() {
	if(!iGpsDataHandler->IsWaiting() && !iWlanDataHandler->IsWaiting() && !iSignalStrengthDataHandler->IsWaiting()) {
		TPtr8 pLogStanza(iLogStanza->Des());
		InsertIntoPayload(128 + 20, GetCurrentTime(), pLogStanza);
		InsertIntoPayload(52, *iLangCode, pLogStanza);
	
		if(iXmppInterface) {
			iXmppInterface->SendAndAcknowledgeXmppStanza(pLogStanza, this, false);
		}

		iEngineState = ELocationWaitForTimeout;
		iTimer->After(KIdleTimeout);
	}
}

void CLocationEngine::TimerExpired(TInt /*aExpiryId*/) {	
	switch(iEngineState) {
		case ELocationShutdown:
			if(iCellTowerDataHandler->IsConnected() || iGpsDataHandler->IsConnected() ||
					iWlanDataHandler->IsConnected() || iSignalStrengthDataHandler->IsConnected()) {

				iTimer->After(KShutdownTimeout);
			}
			else {
				iEngineObserver->LocationShutdownComplete();
			}
			break;
		case ELocationWaitForTimeout:
			if(iCellEnabled || iWlanEnabled || iGpsEnabled) {
				iEngineState = ELocationWaitForResources;
				iTimer->After(KResourceTimeout);

				_LIT8(KLogStanza, "<iq to='butler.buddycloud.com' type='get' xml:lang='' id='location1'><locationquery xmlns='urn:xmpp:locationquery:0' clientver='s60/1.0'><timestamp></timestamp><publish>true</publish></locationquery></iq>\r\n");
				TPtr8 pLogStanza(iLogStanza->Des());
				pLogStanza.Copy(KLogStanza);
				
				// Set resources as busy
				if(iWlanEnabled) {
					iWlanDataHandler->SetWaiting(true);
				}
				
				if(iGpsEnabled) {
					iGpsDataHandler->SetWaiting(true);
				}
				
				// Start resources
				if(iCellEnabled) {
					if(iLastCellTower.LocationAreaCode != 0 && iLastCellTower.CellId != 0) {
						InsertCellTowerDataL();
						
						iSignalStrengthDataHandler->SetWaiting(true);
						iSignalStrengthDataHandler->StartL();
					}
				}
				
				if(iWlanEnabled) {
					iWlanDataHandler->StartL();
				}
				
				if(iGpsEnabled) {
					// Test if automatic gps handling is possible
					if((iCellEnabled || iWlanEnabled) && iMotionState == EMotionStationary) {
						// Pattern stable so no need to run gps
						iGpsDataHandler->SetWaiting(false);
						
						// Stop gps resource handler
						iGpsDataHandler->Stop();
					}
					else {
						// Start gps resource handler
						iGpsDataHandler->StartL();
					}
				}
			}
			else {
				iEngineState = ELocationWaitForTimeout;
				iTimer->After(KIdleTimeout);
			}
			break;
		case ELocationWaitForResources:
			if(iSignalStrengthDataHandler->IsWaiting() || iWlanDataHandler->IsWaiting() || iGpsDataHandler->IsWaiting()) {
				if(iSignalStrengthDataHandler->IsWaiting()) {
					InsertSignalStrengthL(0);
					iSignalStrengthDataHandler->SetWaiting(false);
				}

				iWlanDataHandler->SetWaiting(false);
				iGpsDataHandler->SetWaiting(false);

				iSignalStrengthDataHandler->Stop();
				iWlanDataHandler->Stop();

				BuildLocationPayload();
			}
			else {
				iEngineState = ELocationWaitForTimeout;
				iTimer->After(KIdleTimeout);
			}
			break;
		default:;
	}
}

void CLocationEngine::CellTowerData(TBuf<4> aCountryCode, TBuf<8> aNetworkIdentity, TUint aLocationAreaCode, TUint aCellId) {
	if(aLocationAreaCode != 0 && aCellId != 0) {
		if(aLocationAreaCode != iLastCellTower.LocationAreaCode || aCellId != iLastCellTower.CellId) {
			iTimer->Stop();

			if(iEngineState == ELocationWaitForResources) {
				// Still waiting for previous cells secondary resources to complete
				// Send last cell to location server incomplete
				TimerExpired(0);
			}
			
			// Store latest cell tower data
			iLastCellTower.CellId = aCellId;
			iLastCellTower.LocationAreaCode = aLocationAreaCode;
			iLastCellTower.NetworkIdentity.Copy(aNetworkIdentity);
			iLastCellTower.CountryCode.Copy(aCountryCode);
			
			iEngineState = ELocationWaitForTimeout;
			TimerExpired(0);
		}
	}
	else if(iEngineState == ELocationIdle){
		iEngineState = ELocationWaitForTimeout;
		TimerExpired(0);
	}
}

void CLocationEngine::CellTowerError(TCellTowerError /*aError*/) {
	if(iEngineState == ELocationIdle) {
		iEngineState = ELocationWaitForTimeout;
		TimerExpired(0);
	}
}

void CLocationEngine::GpsData(TReal aLatitude, TReal aLongitude, TReal aAccuracy) {
	iGpsLatitude = aLatitude;
	iGpsLongitude = aLongitude;
	iGpsAccuracy = aAccuracy;
	
	if(iGpsDataHandler->IsWaiting()) {
		iGpsDataHandler->SetWaiting(false);

		_LIT8(KGpsData, "<lat>%.6f</lat><lon>%.6f</lon><accuracy>%.2f</accuracy>");
		HBufC8* aGpsData = HBufC8::NewLC(KGpsData().Length() + 32);
		TPtr8 pGpsData(aGpsData->Des());
		pGpsData.Format(KGpsData, aLatitude, aLongitude, aAccuracy);
	
		TPtr8 pLogStanza(iLogStanza->Des());
		InsertIntoPayload(140 + 20, pGpsData, pLogStanza);
		CleanupStack::PopAndDestroy();
	
		BuildLocationPayload();
	}
}

void CLocationEngine::GpsError(TGpsError /*aError*/) {
	iGpsLatitude = 0.0;
	iGpsLongitude = 0.0;
	iGpsAccuracy = 0.0;

	if(iGpsDataHandler->IsWaiting()) {
		iGpsDataHandler->SetWaiting(false);

		BuildLocationPayload();
	}
}

void CLocationEngine::WlanData(TDesC8& aMac, TDesC8& aSecurity, TUint aRxLevel) {
	if(iWlanDataHandler->IsWaiting()) {
		_LIT8(KWlanData, "<reference><id></id><type>wifi</type><enc></enc><signalstrength>%u</signalstrength></reference>");
		HBufC8* aWlanData = HBufC8::NewLC(KWlanData().Length() + aMac.Length() + aSecurity.Length() + 8);
		TPtr8 pWlanData(aWlanData->Des());
		pWlanData.Format(KWlanData, aRxLevel);
		pWlanData.Insert(42, aSecurity);
		pWlanData.Insert(15, aMac);

		TPtr8 pLogStanza(iLogStanza->Des());
		InsertIntoPayload(pLogStanza.Length() - 23, pWlanData, pLogStanza);
		CleanupStack::PopAndDestroy();
	}
}

void CLocationEngine::WlanNotification(TWlanNotification /*aCode*/) {
	if(iWlanDataHandler->IsWaiting()) {
		iWlanDataHandler->SetWaiting(false);
		
		BuildLocationPayload();
	}
}

void CLocationEngine::SignalStrengthData(TInt32 aSignalStrength, TInt8 /*aSignalBars*/) {
	if(iSignalStrengthDataHandler->IsWaiting()) {
		iSignalStrengthDataHandler->SetWaiting(false);

		InsertSignalStrengthL(aSignalStrength);
		BuildLocationPayload();
	}
}

void CLocationEngine::SignalStrengthError(TSignalStrengthError /*aError*/) {
	SignalStrengthData(0, 0);
}

void CLocationEngine::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& /*aId*/) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aXmlParser->MoveToElement(_L8("location"))) {		
		if(aAttributeType.Compare(_L8("result")) == 0) {			
			// Beacon log result
			iPatternQuality = aXmlParser->GetIntAttribute(_L8("cellpatternquality"));
			iPlaceId = aXmlParser->GetIntAttribute(_L8("placeid"));
			TPtrC8 aAttributeState = aXmlParser->GetStringAttribute(_L8("state"));
			
			if(aAttributeState.Compare(_L8("stationary")) == 0) {
				iMotionState = EMotionStationary;
			}
			else if(aAttributeState.Compare(_L8("restless")) == 0) {
				iMotionState = EMotionRestless;
			}
			else {
				iMotionState = EMotionMoving;
			}
			
			iEngineObserver->HandleLocationServerResult(iMotionState, iPatternQuality, iPlaceId);
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

// End of File

