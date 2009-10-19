/*
============================================================================
 Name        : 	BuddycloudContactStreamer.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Stream contacts from the database to the observer
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudContactStreamer.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudContactStreamer
--
----------------------------------------------------------------------------
*/

CBuddycloudContactStreamer::CBuddycloudContactStreamer(CContactDatabase* aContactDatabase, MBuddycloudContactStreamerObserver* aObserver) : CActive(EPriorityHigh) {
	iContactDatabase = aContactDatabase;
	iObserver = aObserver;

	iContactIndex = 0;
}

CBuddycloudContactStreamer* CBuddycloudContactStreamer::NewL(CContactDatabase* aContactDatabase, MBuddycloudContactStreamerObserver* aObserver) {
	CBuddycloudContactStreamer* self = new (ELeave) CBuddycloudContactStreamer(aContactDatabase, aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CBuddycloudContactStreamer::~CBuddycloudContactStreamer() {
	Cancel();

	if(iTimer) {
		delete iTimer;
	}
}

void CBuddycloudContactStreamer::ConstructL() {
	TRAPD(aErr, iContactDatabase->OpenTablesL());
	
	iTimer = CCustomTimer::NewL(this);
	CActiveScheduler::Add(this);
}

void CBuddycloudContactStreamer::SortAndStartL(CArrayFix<TSortPref> *aSortOrder) {
#ifdef __SERIES60_3X__
	if(!IsActive()) {
		iContactDatabase->SortAsyncL(aSortOrder, iStatus);
		SetActive();
	}
#else
	iContactDatabase->SortL(aSortOrder);
	
	iContactArray = iContactDatabase->SortedItemsL();
	
	iTimer->After(KCycleFrequency);
#endif
}

void CBuddycloudContactStreamer::StartL() {
	iContactArray = iContactDatabase->SortedItemsL();
	
	iTimer->After(KCycleFrequency);
}

void CBuddycloudContactStreamer::ProcessContactsL() {
	TInt aLastContactIndex = iContactIndex;
	TInt aContactId;

	// Read one cycle of contacts
	while(iContactIndex <= aLastContactIndex + KMaxContactsPerCycle && iContactIndex < iContactArray->Count()) {
		if(iObserver) {
			aContactId = (*iContactArray)[iContactIndex];

			iObserver->HandleStreamingContactL(aContactId, iContactDatabase->ReadContactLC(aContactId));

			CleanupStack::PopAndDestroy();
		}

		iContactIndex++;
	}

	// Notify end of cycle
	if(iObserver) {
		iObserver->FinishedStreamingCycle();
	}

	// Wait until next cycle
	if(iContactIndex < iContactArray->Count()) {
		iTimer->After(KCycleFrequency);
	}
}

void CBuddycloudContactStreamer::TimerExpired(TInt /*aExpiryId*/) {
	TRAPD(aErr, ProcessContactsL());
}

void CBuddycloudContactStreamer::RunL() {
	if(iStatus == KErrNone) {
		iContactArray = iContactDatabase->SortedItemsL();

		TRAPD(aErr, ProcessContactsL());
	}
}

TInt CBuddycloudContactStreamer::RunError(TInt /*aError*/) {
	return KErrNone;
}

void CBuddycloudContactStreamer::DoCancel() {
#ifdef __SERIES60_3X__
	iContactDatabase->CancelAsyncSort();
#endif
}
