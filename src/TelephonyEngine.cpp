/*
============================================================================
 Name        : 	TelephonyEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Management of Telephony based operations
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#include <e32property.h>
#include <e32std.h>
#include <TelephonyInternalPSKeys.h>
#include "TelephonyEngine.h"

CTelephonyEngine::CTelephonyEngine(MTelephonyEngineNotification* aObserver) : CActive(EPriorityHigh) {
	iObserver = aObserver;
}

CTelephonyEngine* CTelephonyEngine::NewL(MTelephonyEngineNotification* aObserver) {
	CTelephonyEngine* self = new (ELeave) CTelephonyEngine(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CTelephonyEngine::~CTelephonyEngine() {
	if(iEngineState != ETelephonyIdle) {
		Hangup();
		
		if(iEngineState == ETelephonyHangingUp) {
			User::WaitForRequest(iStatus);
		}
	}
	
	if(iCallerName) 
		delete iCallerName;
	
	if(iTelephony)
		delete iTelephony;
}

void CTelephonyEngine::ConstructL() {
	CActiveScheduler::Add(this);
	
	iTelephony = CTelephony::NewL();
	
	iCallerName = HBufC::NewL(0);
}

void CTelephonyEngine::RequestChangeNotification() {
	if(!IsActive()) {
		CTelephony::TCallStatusV1Pckg aCallStatusV1Pckg(iCallStatus);	
		iTelephony->NotifyChange(iStatus, CTelephony::EVoiceLineStatusChange, aCallStatusV1Pckg);
	    SetActive();	    
	}
}

void CTelephonyEngine::DialNumber(const TDesC& aNumberToCall) {
#ifndef __WINSCW__
	if(!IsActive() && iEngineState == ETelephonyIdle) {
		iEngineState = ETelephonyDialling;
		iObserver->TelephonyStateChanged(iEngineState);		
		iObserver->TelephonyDebug(_L8("CALL  DialNumber"));
		
		iCallParams.iIdRestrict = CTelephony::ESendMyId;
		CTelephony::TCallParamsV1Pckg aCallParamsV1Pckg(iCallParams);		
		iTelephony->DialNewCall(iStatus, aCallParamsV1Pckg, aNumberToCall, iCallId);
	    SetActive();
	}
#endif
}

void CTelephonyEngine::Hangup() {
	Cancel();
	
	if(iEngineState == ETelephonyActive) {
		iEngineState = ETelephonyHangingUp;
		iObserver->TelephonyStateChanged(iEngineState);
		iObserver->TelephonyDebug(_L8("CALL  Hangup"));

		iTelephony->Hangup(iStatus, iCallId);
	    SetActive();
	}
}

void CTelephonyEngine::Hold() {
	if(!OnHold()) {
		Cancel();
		
		// Hold
		iEngineState = ETelephonyHolding;
		iObserver->TelephonyStateChanged(iEngineState);
		iObserver->TelephonyDebug(_L8("CALL  Hold"));

		iTelephony->Hold(iStatus, iCallId);
	    SetActive();	
	}
}

TBool CTelephonyEngine::OnHold() {
	CTelephony::TCallStatusV1Pckg aCallStatusV1Pckg(iCallStatus);	
	iTelephony->GetCallStatus(iCallId, aCallStatusV1Pckg);
	
	return (iCallStatus.iStatus == CTelephony::EStatusHold);
}

void CTelephonyEngine::Resume() {
	if(OnHold()) {
		Cancel();
		
		// Resume
		iEngineState = ETelephonyResuming;
		iObserver->TelephonyStateChanged(iEngineState);
		iObserver->TelephonyDebug(_L8("CALL  Resume"));
		
		iTelephony->Resume(iStatus, iCallId);
	    SetActive();		
	}
}

TTelephonyEngineState CTelephonyEngine::GetTelephonyState() {
	return iEngineState;
}

TBool CTelephonyEngine::IsTelephonyBusy() {
	CTelephony::TCallStatusV1Pckg aCallStatusV1Pckg(iCallStatus);
	
	if(iTelephony->GetLineStatus(CTelephony::EVoiceLine, aCallStatusV1Pckg) == KErrNone) {
		if(iCallStatus.iStatus > CTelephony::EStatusIdle) {
			return true;
		}
	}
	
	return false;
}

TInt CTelephonyEngine::GetCallerId() {
	return iCallerId;
}

TDesC& CTelephonyEngine::GetCallerName() {
	return *iCallerName;
}

void CTelephonyEngine::SetCallerDetailsL(TInt aCallerId, const TDesC& aName) {
	iCallerId = aCallerId;
	
	if(iCallerName)
		delete iCallerName;
	
	iCallerName = aName.AllocL();
}

TTime CTelephonyEngine::GetCallStartTime() {
	return iCallStartTime;
}

TInt CTelephonyEngine::GetCallDuration() {
	TTime aNow;
	aNow.HomeTime();
	
	TTimeIntervalSeconds aInterval;
	aNow.SecondsFrom(iCallStartTime, aInterval);
	
	return aInterval.Int();
}

TBool CTelephonyEngine::LoudspeakerAvailable() {
	TInt aActive = KErrNotFound;
	
	return (RProperty::Get(KTelephonyAudioOutput, KTelephonyAudioOutputPreference, aActive) == KErrNone);
}

TBool CTelephonyEngine::LoudspeakerActive() {
	TInt aActive = KErrNotFound;
	
	RProperty::Get(KTelephonyAudioOutput, KTelephonyAudioOutputPreference, aActive);
	
	return (aActive == EPSPublic);
}

void CTelephonyEngine::SetLoudspeakerActive(TBool aActive) {
	if(aActive) {
		RProperty::Set(KTelephonyAudioOutput, KTelephonyAudioOutputPreference, EPSPublic);
	}
	else {
		RProperty::Set(KTelephonyAudioOutput, KTelephonyAudioOutputPreference, EPSPrivate);
	}
}

void CTelephonyEngine::RunL() {
	TBuf8<256> aText;
	aText.Format(_L8("CALL  Active Complete: %d"), iEngineState);
	iObserver->TelephonyDebug(aText);	
	
	switch(iEngineState) {
		case ETelephonyDialling:
			if(iStatus == KErrNone) {
				iEngineState = ETelephonyActive;
				iCallStartTime.HomeTime();
			}
			else {
				iEngineState = ETelephonyIdle;
				SetCallerDetailsL(0, _L(""));
			}
			break;
		case ETelephonyActive:
			if(iStatus == KErrNone) {
				aText.Format(_L8("CALL  Notification: %d"), iCallStatus.iStatus);
				iObserver->TelephonyDebug(aText);
				
				if(iCallStatus.iStatus == CTelephony::EStatusDisconnecting) {
					iEngineState = ETelephonyHangingUp;
				}
			}
			break;
		case ETelephonyHolding:
		case ETelephonyResuming:
			iEngineState = ETelephonyActive;
			break;
		case ETelephonyHangingUp:
			if(iStatus == KErrNone) {
				iEngineState = ETelephonyIdle;		
				SetCallerDetailsL(0, _L(""));
			}
			else {
				iEngineState = ETelephonyActive;
			}
			break;
		default:;
	}
			
	if(iEngineState != ETelephonyIdle) {
		RequestChangeNotification();
	}
	
	iObserver->TelephonyStateChanged(iEngineState);
}

TInt CTelephonyEngine::RunError(TInt /*aError*/) {
	return KErrNone;
}

void CTelephonyEngine::DoCancel() {
	TBuf8<256> aText;
	aText.Format(_L8("CALL  Cancel: %d"), iEngineState);
	iObserver->TelephonyDebug(aText);
	
	switch(iEngineState) {
		case ETelephonyDialling:
			iTelephony->CancelAsync(CTelephony::EDialNewCallCancel);
			iEngineState = ETelephonyIdle;
			iObserver->TelephonyStateChanged(iEngineState);
			SetCallerDetailsL(0, _L(""));
			break;
		case ETelephonyActive:
			iTelephony->CancelAsync(CTelephony::EVoiceLineStatusChangeCancel);
			break;
		case ETelephonyHolding:
			iTelephony->CancelAsync(CTelephony::EHoldCancel);
			break;
		case ETelephonyResuming:
			iTelephony->CancelAsync(CTelephony::EResumeCancel);
			break;
		default:;
	}
}
