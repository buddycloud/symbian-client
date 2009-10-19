/*
============================================================================
 Name        : 	BuddycloudContactSearcher.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Search contacts from the database
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudContactSearcher.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudContactSearcher
--
----------------------------------------------------------------------------
*/

CBuddycloudContactSearcher::CBuddycloudContactSearcher(CContactDatabase* aContactDatabase, MBuddycloudContactSearcherObserver* aObserver) {
	iContactDatabase = aContactDatabase;
	iObserver = aObserver;
}

CBuddycloudContactSearcher* CBuddycloudContactSearcher::NewL(CContactDatabase* aContactDatabase, MBuddycloudContactSearcherObserver* aObserver) {
	CBuddycloudContactSearcher* self = new (ELeave) CBuddycloudContactSearcher(aContactDatabase, aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CBuddycloudContactSearcher::~CBuddycloudContactSearcher() {
	StopSearch();
	
	if(iItemFieldDef) {
		delete iItemFieldDef;
	}
}

void CBuddycloudContactSearcher::ConstructL() {
	iItemFieldDef = new CContactItemFieldDef();
	iItemFieldDef->AppendL(KUidContactFieldFamilyName);
	iItemFieldDef->AppendL(KUidContactFieldGivenName);
}

void CBuddycloudContactSearcher::StartSearchL(const TDesC &aText) {
	StopSearch();
	
	iIdleFinder = iContactDatabase->FindAsyncL(aText, iItemFieldDef, this);
}

void CBuddycloudContactSearcher::StopSearch() {
	if(iIdleFinder) {
		iIdleFinder->Cancel();
		
		delete iIdleFinder;
		iIdleFinder = NULL;
	}
}

CContactIdArray* CBuddycloudContactSearcher::SearchResults() {
	if(iIdleFinder) {
		return iIdleFinder->TakeContactIds();
	}
	
	return NULL;
}

void CBuddycloudContactSearcher::IdleFindCallback() {
	if(iIdleFinder->IsComplete() && iIdleFinder->Error() == KErrNone && iObserver) {
		iObserver->FinishedContactSearch();
	}
}
