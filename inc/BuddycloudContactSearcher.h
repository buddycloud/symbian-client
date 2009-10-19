/*
============================================================================
 Name        : 	BuddycloudContactSearcher.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Search contacts from the database
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDCONTACTSEARCHER_H_
#define BUDDYCLOUDCONTACTSEARCHER_H_

#include <e32base.h>
#include <cntdef.h>
#include <cntdb.h>
#include <cntitem.h>

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MBuddycloudContactSearcherObserver {
	public:
		virtual void FinishedContactSearch() = 0;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudContactSearcher
--
----------------------------------------------------------------------------
*/

class CBuddycloudContactSearcher : public CBase, MIdleFindObserver {
	public:
		CBuddycloudContactSearcher(CContactDatabase* aContactDatabase, MBuddycloudContactSearcherObserver* aObserver);
		static CBuddycloudContactSearcher* NewL(CContactDatabase* aContactDatabase, MBuddycloudContactSearcherObserver* aObserver);
		~CBuddycloudContactSearcher();

	private:
		void ConstructL();

	public:
		void StartSearchL(const TDesC &aText);
		void StopSearch();
		
	public:
		CContactIdArray* SearchResults();

	public: // From MIdleFindObserver
		void IdleFindCallback();

	private: // Variables
		MBuddycloudContactSearcherObserver* iObserver;
		
		CContactDatabase* iContactDatabase;

		CIdleFinder* iIdleFinder;
		CContactItemFieldDef* iItemFieldDef;
};

#endif /* BUDDYCLOUDCONTACTSEARCHER_H_ */
