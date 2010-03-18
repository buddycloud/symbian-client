/*
============================================================================
 Name        : 	DiscussionManager.h
 Author      : 	Ross Savage
 Copyright   : 	2009 Buddycloud
 Description : 	Definition & management of discussions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <e32base.h>
#include <f32file.h>
#include "AtomEntryData.h"
#include "BuddycloudList.h"
#include "Timer.h"

#ifndef DISCUSSIONMANAGER_H_
#define DISCUSSIONMANAGER_H_

class CDiscussion;

/*
----------------------------------------------------------------------------
--
-- CThreadedEntry
--
----------------------------------------------------------------------------
*/

class MThreadedEntryObserver {
	public:
		virtual void EntryAdded(CAtomEntryData* aEntry) = 0;
		virtual void EntryDeleted(CAtomEntryData* aEntry) = 0;
};

class CThreadedEntry : public CBase {
	public:
		static CThreadedEntry* NewL(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver);
		static CThreadedEntry* NewLC(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver);
		~CThreadedEntry();
		
	private:
		CThreadedEntry(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver);
		
	public: // Entry access
		CAtomEntryData* GetEntry();
	
	public: // Comment functions
		TInt CommentCount();
		
		TBool AddCommentLD(CAtomEntryData* aComment);
		
		TBool DeleteAllComments();
		TBool DeleteCommentByIndex(TInt aIndex);
		TBool DeleteCommentById(const TDesC8& aId);
		
		CAtomEntryData* GetCommentByIndex(TInt aIndex);
		CAtomEntryData* GetCommentById(const TDesC8& aId);
		
	protected:
		MThreadedEntryObserver* iObserver;
		
		// Entry
		CAtomEntryData* iEntry;
		
		// Comments
		RPointerArray<CAtomEntryData> iComments;
};

/*
----------------------------------------------------------------------------
--
-- CDiscussion
--
----------------------------------------------------------------------------
*/

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
		MDiscussionUnreadData() {
			iUnreadEntries = 0;
			iUnreadReplies = 0;
		}
	
	public:
		TInt iUnreadEntries;
		TInt iUnreadReplies;
};

class CDiscussion : public CBase, public MDiscussionUnreadData, MAtomEntryObserver, MThreadedEntryObserver {
	public:
		static CDiscussion* NewL(const TDesC& aDiscussionId);
		static CDiscussion* NewLC(const TDesC& aDiscussionId);
		~CDiscussion();
		
	private:
		CDiscussion();
		void ConstructL(const TDesC& aDiscussionId);
		
	public: // Unread
		TInt GetUnreadEntries();
		TInt GetUnreadReplies();
		void SetUnreadData(TInt aEntries, TInt aReplies = 0);
		
	public: // Observers
		void SetDiscussionReadObserver(MDiscussionReadObserver* aObserver, TInt aItemId);
		void SetDiscussionUpdateObserver(MDiscussionUpdateObserver* aObserver);

	public: // Discussion id
		TDesC& GetDiscussionId();
		void SetDiscussionIdL(const TDesC& aDiscussionId);
		
		TInt GetItemId();
		
	public: // Notify setting
		TBool Notify();
		void SetNotify(TBool aNotify);
		
	public: // Cleaning & compression
		void CacheL();
		void CompressL(TBool aForced = false);
		void CleanL();

	public:
		TInt EntryCount();
	
	public: // Get thread
		CThreadedEntry* GetThreadedEntryByIndex(TInt aIndex);
		CThreadedEntry* GetThreadedEntryById(const TDesC8& aEntryId);
		
	public: // Entry deletion
		TBool DeleteEntryByIndex(TInt aIndex);
		TBool DeleteEntryById(const TDesC8& aEntryId);
		TBool DeleteAllEntries();
		
	public: // Entry addition
		TBool AddEntryOrCommentLD(CAtomEntryData* aEntry, const TDesC8& aEntryReference = KNullDesC8);
		
	private:
		void ParseEntryForLinksL(CAtomEntryData* aEntry);
		
	private: // From MAtomEntryObserver
		void EntryRead(CAtomEntryData* aEntry);
	
	private: // From MThreadedEntryObserver
		void EntryAdded(CAtomEntryData* aEntry);
		void EntryDeleted(CAtomEntryData* aEntry);
			
	private: // File interfaces
		void ReadDiscussionToMemoryL();
		void WriteDiscussionToFileL();
		void DeleteDiscussionFileL();
		
	private:
		TFileName GetFileName(RFs& aSession);
		
	protected:
		// Observers
		MDiscussionReadObserver* iDiscussionReadObserver;
		MDiscussionUpdateObserver* iDiscussionUpdateObserver;
		
		HBufC* iDiscussionId;
		TInt iDiscussionIndexer;
		TInt iItemId;
		
		// Cache handling
		TTime iLastUpdate;
		TBool iDiscussionInMemory;
		
		
		// Flags
		TBool iNotify;
		
		// Entries
		RPointerArray<CThreadedEntry> iEntries;
};

/*
----------------------------------------------------------------------------
--
-- CDiscussionManager
--
----------------------------------------------------------------------------
*/

class CDiscussionManager : public CBase, MTimeoutNotification {
	public:
		static CDiscussionManager* NewL();
		~CDiscussionManager();
	
	private:
		void ConstructL();
		
	public:
		CDiscussion* GetDiscussionL(const TDesC& aId);		
		void DeleteDiscussionL(const TDesC& aId);
		
		void CompressDiscussionL(TBool aForced = false);
		
	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);
		
	private:
		TBool CompressionNeeded();
		
	protected:
		RPointerArray<CDiscussion> iDiscussions;
		
		CCustomTimer* iTimer;
};

#endif /* DISCUSSIONMANAGER_H_ */
