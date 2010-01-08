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
#include "BuddycloudList.h"
#include "DiscussionManager.h"
#include "GeolocData.h"
#include "XmppConstants.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TFollowingItemType {
	EItemNotice = 2, 
	EItemRoster = 4, 
	EItemChannel = 8,
	// Private: do not use
	EItemAll = 14
};

enum TIdType {
	EIdRoster, EIdChannel
};

enum TPresenceState {
	EPresenceOffline, EPresenceOnline
};

enum TNoticeResponse {
	ENoticeAccept, ENoticeAcceptAndFollow, ENoticeDecline
};

enum TGeolocItemType {
	EGeolocItemPrevious, EGeolocItemCurrent, EGeolocItemFuture, EGeolocItemBroad
};

/*
----------------------------------------------------------------------------
--
-- CFollowingItem
--
----------------------------------------------------------------------------
*/

class CFollowingItem : public CBuddycloudListItem {
	public:
		static CFollowingItem* NewL();
		static CFollowingItem* NewLC();
		~CFollowingItem();
		
	protected:
		CFollowingItem();
		void ConstructL();

	public:
		TFollowingItemType GetItemType();
		void SetItemType(TFollowingItemType aItemType);

		TDesC& GetId(TIdType aType = EIdChannel);
		void SetIdL(const TDesC& aId, TIdType aType = EIdChannel);
		
		TIdType GetIdType();

	protected:
		TFollowingItemType iItemType;

		HBufC* iId;
		TIdType iIdType;
};

/*
----------------------------------------------------------------------------
--
-- CFollowingChannelItem
--
----------------------------------------------------------------------------
*/

class CFollowingChannelItem : public CFollowingItem {
	public:
		static CFollowingChannelItem* NewL();
		static CFollowingChannelItem* NewLC();
	
	protected:
		CFollowingChannelItem();

// New		
	public:
		TXmppPubsubAccessModel GetAccessModel();
		void SetAccessModel(TXmppPubsubAccessModel aAccessModel);
		
		TXmppPubsubSubscription GetPubsubSubscription();
		void SetPubsubSubscription(TXmppPubsubSubscription aPubsubSubscription);
		
		TXmppPubsubAffiliation GetPubsubAffiliation();
		void SetPubsubAffiliation(TXmppPubsubAffiliation aPubsubAffiliation);
		
	public:
		void SetUnreadData(MDiscussionUnreadData* aUnreadData, TIdType aType = EIdChannel);
		
	public:
		TInt GetUnread(TIdType aType = EIdChannel);
		TInt GetReplies();
		
	protected:
		MDiscussionUnreadData* iChannelUnreadData;
		
		TXmppPubsubAccessModel iAccessModel;
		TXmppPubsubSubscription iPubsubSubscription;
		TXmppPubsubAffiliation iPubsubAffiliation;
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
		TBool OwnItem();
		void SetOwnItem(TBool aIsOwn);

	public:
		TDesC& GetId(TIdType aType = EIdRoster);
		void SetIdL(TDesC& aId, TIdType aType = EIdRoster);
				
	public:
		TPresenceState GetPresence();
		void SetPresence(TPresenceState aPresence);

		TPresenceSubscription GetSubscription();
		void SetSubscription(TPresenceSubscription aSubscription);

	public:
		CGeolocData* GetGeolocItem(TGeolocItemType aGeolocItem);
		void SetGeolocItemL(TGeolocItemType aGeolocItem, CGeolocData* aGeoloc);
		
	public:
		void SetUnreadData(MDiscussionUnreadData* aUnreadData, TIdType aType = EIdRoster);
		
	public:
		TInt GetUnread(TIdType aType = EIdRoster);

	protected:
		MDiscussionUnreadData* iRosterUnreadData;
		
		HBufC* iJid;

		TBool iOwnItem;
		
		TPresenceSubscription iSubscription;
		TPresenceState iPresence;

		RPointerArray<CGeolocData> iGeolocs;
};

#endif /*BUDDYCLOUDFOLLOWING_H_*/
