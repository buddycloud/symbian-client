/*
============================================================================
 Name        : 	BuddycloudFollowing.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Storage of Buddycloud following based data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDFOLLOWING_H_
#define BUDDYCLOUDFOLLOWING_H_

#include <e32base.h>
#include "BuddycloudPlaces.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TFollowingItemType {
	EItem = 0, 
	EItemNotice = 2, 
	EItemContact = 4, 
	EItemRoster = 8, 
	EItemChannel = 16,
	// Private: do not use
	EItemAll = 30
};

enum TJidType {
	EJidRoster, EJidChannel
};

enum TSubscriptionType {
	ESubscriptionUnknown, ESubscriptionRemove, ESubscriptionNone, 
	ESubscriptionFrom, ESubscriptionTo, ESubscriptionBoth
};

enum TPresenceState {
	EPresenceOffline, EPresenceSent, EPresenceOnline
};

enum TNoticeResponse {
	ENoticeAccept, ENoticeAcceptAndFollow, ENoticeDecline
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MMessageReadObserver {
	public:
		virtual void MessageRead(TDesC& aJid, TInt aMessageId, TInt aUnreadMessages, TInt aUnreadReplies = 0) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingItem
--
----------------------------------------------------------------------------
*/

class CFollowingItem : public CBase {
	public:
		static CFollowingItem* NewL();
		static CFollowingItem* NewLC();
		~CFollowingItem();
	
	protected:
		CFollowingItem();
		void ConstructL();

	public:
		TInt GetItemId();
		void SetItemId(TInt aId);

		TFollowingItemType GetItemType();

		TBool GetIsOwn();
		void SetIsOwn(TBool aIsOwn);

		TTime GetLastUpdated();
		void SetLastUpdated(TTime aTime);

		TBool IsFiltered();
		void SetFiltered(TBool aFilter);
		
	public:
		TInt GetAvatarId();
		void SetAvatarId(TInt aAvatarId);

		TDesC& GetTitle();
		void SetTitleL(const TDesC& aTitle);
		
		TDesC& GetDescription();
		void SetDescriptionL(const TDesC& aDescription);

	protected:
		TFollowingItemType iItemType;
		TInt iItemId;
		TBool iIsOwn;
		TTime iLastUpdated;
		TBool iIsFiltered;
		
		TInt iAvatarId;
		HBufC* iTitle;
		HBufC* iDescription;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingNoticeItem
--
----------------------------------------------------------------------------
*/

class CFollowingNoticeItem : public CFollowingItem {
	public:
		static CFollowingNoticeItem* NewL();
		static CFollowingNoticeItem* NewLC();
		~CFollowingNoticeItem();
		
	protected:
		CFollowingNoticeItem();
		void ConstructL();

	public:
		TDesC& GetJid();
		void SetJidL(const TDesC& aJid, TJidType aType);
		
		TJidType GetJidType();

	protected:
		HBufC* iJid;
		TJidType iJidType;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingContactItem
--
----------------------------------------------------------------------------
*/

class CFollowingContactItem : public CFollowingItem {
	public:
		static CFollowingContactItem* NewL();
		static CFollowingContactItem* NewLC();
		
	protected:
		CFollowingContactItem();

	public:
		TInt GetAddressbookId();
		void SetAddressbookId(TInt aAddressbookId);

	protected:
		TInt iAddressbookId;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingChannelItem
--
----------------------------------------------------------------------------
*/

class CFollowingChannelItem : public CFollowingContactItem, public MMessageReadObserver {
	public:
		static CFollowingChannelItem* NewL();
		static CFollowingChannelItem* NewLC();
		~CFollowingChannelItem();
	
	protected:
		CFollowingChannelItem();
		void ConstructL();
		
	public:
		TDesC& GetJid(TJidType aType = EJidChannel);
		void SetJidL(const TDesC& aJid, TJidType aType = EJidChannel);
		
		TPresenceState GetPresence(TJidType aType = EJidChannel);
		void SetPresence(TPresenceState aPresence, TJidType aType = EJidChannel);
		
	public:
		TBool IsExternal();
		void SetExternal(TBool aExternal);
		
	public:
		TInt GetRank();
		TInt GetRankShift();
		void SetRank(TInt aRank, TInt aShift = 0);
		
	public: // From MMessageReadObserver
		void MessageRead(TDesC& aJid, TInt aMessageId, TInt aUnreadMessages, TInt aUnreadReplies = 0);
		
	public:
		TInt GetUnread(TJidType aType = EJidChannel);
		TInt GetReplies();
		
	protected:
		HBufC* iChannelJid;
		TInt iChannelUnread;	
		TInt iChannelReplies;	
		
		TPresenceState iChannelPresence;
		
		TBool iExternal;
		
		TInt iRank;
		TInt iRankShift;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingRosterItem
--
----------------------------------------------------------------------------
*/

class CFollowingRosterItem : public CFollowingChannelItem {
	public:
		static CFollowingRosterItem* NewL();
		static CFollowingRosterItem* NewLC();
		~CFollowingRosterItem();
	
	protected:
		CFollowingRosterItem();
		void ConstructL();

	public:
		TDesC& GetJid(TJidType aType = EJidRoster);
		void SetJidL(TDesC& aJid, TJidType aType = EJidRoster);

		TPresenceState GetPresence(TJidType aType = EJidRoster);
		void SetPresence(TPresenceState aPresence, TJidType aType = EJidRoster);

		TSubscriptionType GetSubscriptionType();
		void SetSubscriptionType(TSubscriptionType aSubscription);

	public:
		TBool GetPubsubCollected();
		void SetPubsubCollected(TBool aCollected);

		CBuddycloudBasicPlace* GetPlace(TBuddycloudPlaceType aPlaceType);
		void SetPlaceL(TBuddycloudPlaceType aPlaceType, CBuddycloudBasicPlace* aPlace);
		
	public: // From MMessageReadObserver
		void MessageRead(TDesC& aJid, TInt aMessageId, TInt aUnreadMessages, TInt aUnreadReplies = 0);
		
	public:
		TInt GetUnread(TJidType aType = EJidRoster);

	protected:
		HBufC* iRosterJid;
		TInt iRosterUnread;

		TSubscriptionType iSubscription;
		TPresenceState iRosterPresence;

		TBool iPubsubCollected;

		CBuddycloudBasicPlace* iPlacePrevious;
		CBuddycloudBasicPlace* iPlaceCurrent;
		CBuddycloudBasicPlace* iPlaceNext;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudFollowingStore
--
----------------------------------------------------------------------------
*/

class CBuddycloudFollowingStore : public CBase {
	public:
		static CBuddycloudFollowingStore* NewL();
		static CBuddycloudFollowingStore* NewLC();
		~CBuddycloudFollowingStore();

	public:
		TInt Count();

		CFollowingItem* GetItemByIndex(TInt aIndex);
		CFollowingItem* GetItemById(TInt aItemId);
		
		TInt GetIdByIndex(TInt aIndex);
		TInt GetIndexById(TInt aItemId);

		void MoveItemByIndex(TInt aIndex, TInt aPosition);
		void MoveItemById(TInt aItemId, TInt aPosition);

		void AddItem(CFollowingItem* aItem);
		void InsertItem(TInt aIndex, CFollowingItem* aItem);

		void RemoveItemByIndex(TInt aIndex);
		void RemoveItemById(TInt aItemId);
		void RemoveAll();

		void DeleteItemByIndex(TInt aIndex);
		void DeleteItemById(TInt aItemId);

	protected:
		RPointerArray<CFollowingItem> iItemStore;
};

#endif /*BUDDYCLOUDFOLLOWING_H_*/
