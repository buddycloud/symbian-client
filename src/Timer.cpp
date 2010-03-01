/*
============================================================================
 Name        : 	Timer.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Custom timer & periodic classes with observers
 History     : 	1.0
 		
		1.0	Initial Development
============================================================================
*/

#include "Timer.h"

/*
----------------------------------------------------------------------------
--
-- CCustomTimer
--
----------------------------------------------------------------------------
*/


CCustomTimer::CCustomTimer(MTimeoutNotification* aObserver, TInt aExpiryId) : CActive(EPriorityHigh) {
	iObserver = aObserver;
	iExpiryId = aExpiryId;
}

CCustomTimer* CCustomTimer::NewL(MTimeoutNotification* aObserver, TInt aExpiryId) {
	CCustomTimer* self = new (ELeave) CCustomTimer(aObserver, aExpiryId);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CCustomTimer::~CCustomTimer() {
	Cancel();
	
	iTimer.Close();
}

void CCustomTimer::ConstructL() {
	iTimer.CreateLocal();
	
	CActiveScheduler::Add(this);	
}

void CCustomTimer::After(TTimeIntervalMicroSeconds32 anInterval) {
	Cancel();
	iTimer.After(iStatus, anInterval);
	iRunning = true;
	SetActive();
}

TBool CCustomTimer::IsRunning() {
	return iRunning;
}

void CCustomTimer::Stop() {
	iTimerState = ETimerCancel;
	Cancel();	
}

void CCustomTimer::RunL() {
	iRunning = false;
	iObserver->TimerExpired(iExpiryId);
}

void CCustomTimer::DoCancel() {
	iTimer.Cancel();
	iRunning = false;
}
