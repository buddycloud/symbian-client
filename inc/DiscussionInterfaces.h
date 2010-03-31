/*
============================================================================
 Name        : 	DiscussionInterfaces.h
 Author      : 	Ross Savage
 Copyright   : 	2010 Buddycloud
 Description : 	Discussion interfaces for data and observer access
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef DISCUSSIONINTERFACES_H_
#define DISCUSSIONINTERFACES_H_

#include "AtomEntryData.h"

class MThreadedEntryObserver {
	public:
		virtual void EntryAdded(CAtomEntryData* aEntry) = 0;
		virtual void EntryDeleted(CAtomEntryData* aEntry) = 0;
};

class MDiscussionReadObserver {
	public:
		virtual void DiscussionRead(TDesC& aDiscussionId, TInt aItemId) = 0;		
#ifdef _DEBUG
		virtual void DiscussionDebug(const TDesC8& aMessage) = 0;
#endif
};

class MDiscussionUpdateObserver {
	public:
		virtual void EntryAdded(CAtomEntryData* aAtomEntry) = 0;
		virtual void EntryDeleted(CAtomEntryData* aAtomEntry) = 0;
};

class MDiscussionUnreadData {	
	public:
		TInt iEntries;
		TInt iReplies;
};

#endif /* DISCUSSIONINTERFACES_H_ */
