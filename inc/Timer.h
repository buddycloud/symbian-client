/*
============================================================================
 Name        : 	Timer.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Custom timer & periodic classes with observers
 History     : 	1.0
 		
		1.0	Initial Development
============================================================================
*/

#ifndef TIMER_H_
#define TIMER_H_

#include <e32base.h>


/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TTimerState {
	ETimerIdle, ETimerRestart, ETimerActive, ETimerCancel
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MTimeoutNotification {
	public:
		virtual void TimerExpired(TInt aExpiryId) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CCustomTimer
--
----------------------------------------------------------------------------
*/

class CCustomTimer : public CActive {
	public:
		CCustomTimer(MTimeoutNotification* aObserver, TInt aExpiryId);
		static CCustomTimer* NewL(MTimeoutNotification* aObserver, TInt aExpiryId = 0);
		~CCustomTimer();
		
	private:
		void ConstructL();
		
	public:
		void After(TTimeIntervalMicroSeconds32 anInterval);
		TBool IsRunning();
		void Stop();

	public: // From CActive
    	void RunL();
		void DoCancel();	

	private:
		MTimeoutNotification* iObserver;
		TInt iExpiryId;
		TBool iRunning;
		
		RTimer iTimer;
		TTimerState iTimerState;
		
		TTimeIntervalMicroSeconds32 iInterval;
};

#endif /*TIMER_H_*/
