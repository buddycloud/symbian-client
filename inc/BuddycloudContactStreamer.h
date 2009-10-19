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

#ifndef BUDDYCLOUDCONTACTSTREAMER_H_
#define BUDDYCLOUDCONTACTSTREAMER_H_

#include <e32base.h>
#include <cntdef.h>
#include <cntdb.h>
#include <cntitem.h>
#include "Timer.h"

const TInt KMaxContactsPerCycle = 50;
const TInt KCycleFrequency      = 50000; // 100 msec

typedef CContactDatabase::TSortPref TSortPref;

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MBuddycloudContactStreamerObserver {
	public:
		virtual void HandleStreamingContactL(TInt aContactId, CContactItem* aContact) = 0;
		virtual void FinishedStreamingCycle() = 0;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudContactStreamer
--
----------------------------------------------------------------------------
*/

class CBuddycloudContactStreamer : public CActive, MTimeoutNotification {
	public:
		CBuddycloudContactStreamer(CContactDatabase* aContactDatabase, MBuddycloudContactStreamerObserver* aObserver);
		static CBuddycloudContactStreamer* NewL(CContactDatabase* aContactDatabase, MBuddycloudContactStreamerObserver* aObserver);
		~CBuddycloudContactStreamer();

	private:
		void ConstructL();

	public:
		void SortAndStartL(CArrayFix<TSortPref> *aSortOrder);
		void StartL();
	
	private:
		void ProcessContactsL();

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);
		
	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();

	private: // Variables
		MBuddycloudContactStreamerObserver* iObserver;
		
		CContactDatabase* iContactDatabase;
		const CContactIdArray* iContactArray;

		CCustomTimer* iTimer;

		TInt iContactIndex;
};

#endif /*BUDDYCLOUDCONTACTSTREAMER_H_*/
