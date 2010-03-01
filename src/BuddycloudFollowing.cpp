/*
============================================================================
 Name        : 	BuddycloudFollowing.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Storage of Buddycloud following based data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudFollowing.h"
#include "AvatarConstants.h"

/*
----------------------------------------------------------------------------
--
-- CFollowingItem
--
----------------------------------------------------------------------------
*/

CFollowingItem* CFollowingItem::NewL() {
	CFollowingItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CFollowingItem* CFollowingItem::NewLC(){
	CFollowingItem* self = new (ELeave) CFollowingItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CFollowingItem::~CFollowingItem() {
	if(iId)
		delete iId;
}

CFollowingItem::CFollowingItem() {
	iItemType = EItemNotice;
	iIconId = KIconNotice;
}

void CFollowingItem::ConstructL() {
	CBuddycloudListItem::ConstructL();

	iId = HBufC::NewL(0);
}

TFollowingItemType CFollowingItem::GetItemType() {
	return iItemType;
}

void CFollowingItem::SetItemType(TFollowingItemType aItemType) {
	iItemType = aItemType;
}

TDesC& CFollowingItem::GetId(TIdType /*aType*/) {
	return *iId;
}

void CFollowingItem::SetIdL(const TDesC& aId, TIdType aType) {
	if(iId)
		delete iId;

	iId = aId.AllocL();
	iIdType = aType;
}

TIdType CFollowingItem::GetIdType() {
	return iIdType;
}

/*
----------------------------------------------------------------------------
--
-- CFollowingChannelItem
--
----------------------------------------------------------------------------
*/

CFollowingChannelItem* CFollowingChannelItem::NewL() {
	CFollowingChannelItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CFollowingChannelItem* CFollowingChannelItem::NewLC(){
	CFollowingChannelItem* self = new (ELeave) CFollowingChannelItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CFollowingChannelItem::CFollowingChannelItem() {
	iItemType = EItemChannel;
	iIconId = KIconChannel;
}

TXmppPubsubSubscription CFollowingChannelItem::GetPubsubSubscription() {
	return iPubsubSubscription;
}

void CFollowingChannelItem::SetPubsubSubscription(TXmppPubsubSubscription aPubsubSubscription) {
	iPubsubSubscription = aPubsubSubscription;
}

TXmppPubsubAffiliation CFollowingChannelItem::GetPubsubAffiliation() {
	return iPubsubAffiliation;
}

void CFollowingChannelItem::SetPubsubAffiliation(TXmppPubsubAffiliation aPubsubAffiliation) {
	iPubsubAffiliation = aPubsubAffiliation;
}

void CFollowingChannelItem::SetUnreadData(MDiscussionUnreadData* aUnreadData, TIdType /*aType*/) {
	iChannelUnreadData = aUnreadData;
}

TInt CFollowingChannelItem::GetUnread(TIdType /*aType*/) {
	if(iChannelUnreadData) {
		return iChannelUnreadData->iUnreadEntries;
	}
	
	return 0;
}

TInt CFollowingChannelItem::GetReplies() {
	if(iChannelUnreadData) {
		return iChannelUnreadData->iUnreadReplies;
	}
	
	return 0;
}

/*
----------------------------------------------------------------------------
--
-- CFollowingRosterItem
--
----------------------------------------------------------------------------
*/

CFollowingRosterItem* CFollowingRosterItem::NewL() {
	CFollowingRosterItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CFollowingRosterItem* CFollowingRosterItem::NewLC(){
	CFollowingRosterItem* self = new (ELeave) CFollowingRosterItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CFollowingRosterItem::~CFollowingRosterItem() {
	if(iJid)
		delete iJid;
	
	for(TInt i = 0; i < iGeolocs.Count(); i++) {
		delete iGeolocs[i];
	}

	iGeolocs.Close();
}

CFollowingRosterItem::CFollowingRosterItem() {
	iItemType = EItemRoster;
	iIconId = KIconPerson;

	iSubscription = EPresenceSubscriptionUnknown;
}

void CFollowingRosterItem::ConstructL() {
	CFollowingChannelItem::ConstructL();
	
	iJid = HBufC::NewL(0);
	
	for(TInt i = 0; i < EGeolocItemBroad; i++) {
		iGeolocs.Append(NULL);
	}
}

TBool CFollowingRosterItem::OwnItem() {
	return iOwnItem;
}

void CFollowingRosterItem::SetOwnItem(TBool aIsOwn) {
	iOwnItem = aIsOwn;
}

TDesC& CFollowingRosterItem::GetId(TIdType aType) {
	if(aType == EIdRoster) {
		return *iJid;
	}
	
	return *iId;
}

void CFollowingRosterItem::SetIdL(TDesC& aId, TIdType aType) {
	if(aType == EIdRoster) {
		if(iJid)
			delete iJid;

		iJid = aId.AllocL();
	}
	else {
		CFollowingChannelItem::SetIdL(aId, aType);
	}
}

TPresenceState CFollowingRosterItem::GetPresence() {
	return iPresence;
}

void CFollowingRosterItem::SetPresence(TPresenceState aPresence) {
	iPresence = aPresence;
}

TPresenceSubscription CFollowingRosterItem::GetSubscription() {
	return iSubscription;
}

void CFollowingRosterItem::SetSubscription(TPresenceSubscription aSubscription) {
	iSubscription = aSubscription;
}

CGeolocData* CFollowingRosterItem::GetGeolocItem(TGeolocItemType aGeolocItem) {
	if(iGeolocs[aGeolocItem] == NULL) {
		iGeolocs[aGeolocItem] = CGeolocData::NewL();
	}
	
	return iGeolocs[aGeolocItem];
}

void CFollowingRosterItem::SetGeolocItemL(TGeolocItemType aGeolocItem, CGeolocData* aGeoloc) {
	if(iGeolocs[aGeolocItem]) {
		delete iGeolocs[aGeolocItem];
	}
	
	iGeolocs[aGeolocItem] = aGeoloc;
}

void CFollowingRosterItem::SetUnreadData(MDiscussionUnreadData* aUnreadData, TIdType aType) {
	if(aType == EIdRoster) {
		iRosterUnreadData = aUnreadData;
	}
	else {
		iChannelUnreadData = aUnreadData;
	}
}

TInt CFollowingRosterItem::GetUnread(TIdType aType) {
	if(aType == EIdRoster && iRosterUnreadData) {
		return iRosterUnreadData->iUnreadEntries;
	}
	else if(iChannelUnreadData) {
		return iChannelUnreadData->iUnreadEntries;
	}
	
	return 0;
}

/*
----------------------------------------------------------------------------
--
-- CBuddycloudFollowingStore
--
----------------------------------------------------------------------------
*/

CBuddycloudFollowingStore* CBuddycloudFollowingStore::NewL() {
	CBuddycloudFollowingStore* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudFollowingStore* CBuddycloudFollowingStore::NewLC() {
	CBuddycloudFollowingStore* self = new (ELeave) CBuddycloudFollowingStore();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

void CBuddycloudFollowingStore::FilterItemL(TInt aIndex) {
	CBuddycloudListStore::FilterItemL(aIndex);
	
	CFollowingItem* aFollowingItem = static_cast <CFollowingItem*> (iItemStore[aIndex]);
	TPtrC aFilterText(iFilterText->Des());
		
	if(aFollowingItem && !aFollowingItem->Filtered()) {
		if(aFilterText.Length() <= aFollowingItem->GetId().Length() && 
				aFollowingItem->GetId().FindF(aFilterText) != KErrNotFound) {
			
			// Id match
			aFollowingItem->SetFiltered(true);
		}
		
		if(!aFollowingItem->Filtered() && aFollowingItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aFollowingItem);
			TPtrC aCurrentPlace(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText));
			TPtrC aFuturePlace(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText));
			
			if((aFilterText.Length() <= aCurrentPlace.Length() && aCurrentPlace.FindF(aFilterText) != KErrNotFound) || 
					(aFilterText.Length() <= aFuturePlace.Length() && aFuturePlace.FindF(aFilterText) != KErrNotFound)) {
				
				// Current or future place match
				aFollowingItem->SetFiltered(true);
			}
		}
	}	
}
