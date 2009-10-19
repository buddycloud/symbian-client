/*
============================================================================
 Name        : 	TelephonyEngine.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Management of Telephony based operations
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#include <e32base.h>
#include <etel3rdparty.h>

#ifndef TELEPHONYENGINE_H_
#define TELEPHONYENGINE_H_

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TTelephonyEngineState {
	ETelephonyIdle, ETelephonyDialling, ETelephonyActive, 
	ETelephonyHangingUp, ETelephonyHolding, ETelephonyResuming
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MTelephonyEngineNotification {
	public:
		virtual void TelephonyStateChanged(TTelephonyEngineState aState) = 0;
		virtual void TelephonyDebug(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CLocationEngine
--
----------------------------------------------------------------------------
*/

class CTelephonyEngine : public CActive {
	public:
		CTelephonyEngine(MTelephonyEngineNotification* aObserver);
		static CTelephonyEngine* NewL(MTelephonyEngineNotification* aObserver);
		~CTelephonyEngine();

	private:
		void ConstructL();

	private:
		void RequestChangeNotification();
		
	public:
		void DialNumber(const TDesC& aNumber);
		void Hangup();
		
	public:
		void Hold();
		TBool OnHold();
		void Resume();
		
	public:
		TTelephonyEngineState GetTelephonyState();
		TBool IsTelephonyBusy();
		
		void SetCallerDetailsL(TInt aCallerId, const TDesC& aName);
		TInt GetCallerId();	
		TDesC& GetCallerName();
		
		TTime GetCallStartTime();
		TInt GetCallDuration();
	
	public: // Loudspeaker
		TBool LoudspeakerAvailable();
		TBool LoudspeakerActive();
		void SetLoudspeakerActive(TBool aActive);

	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();
	
	private:
		MTelephonyEngineNotification* iObserver;
		
		// Engine & Call State
		TTelephonyEngineState iEngineState;
		
		TInt iCallerId;
		HBufC* iCallerName;
		
		TTime iCallStartTime;
		
		// ETel 
		CTelephony* iTelephony;
		CTelephony::TCallStatusV1 iCallStatus;
		CTelephony::TCallParamsV1 iCallParams;
		CTelephony::TIdentityServiceV1 iIdentityService;
		CTelephony::TCallId iCallId;
};

#endif /*TELEPHONYENGINE_H_*/
