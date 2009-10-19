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
	if(iTitle)
		delete iTitle;
	
	if(iDescription)
		delete iDescription;
}

CFollowingItem::CFollowingItem() {
	iItemType = EItem;

	iItemId = 0;
	iIsOwn = false;
	iLastUpdated = TTime();
	iIsFiltered = true;
}

void CFollowingItem::ConstructL() {
	iTitle = HBufC::NewL(0);
	iDescription = HBufC::NewL(0);
}

TInt CFollowingItem::GetItemId() {
	return iItemId;
}

void CFollowingItem::SetItemId(TInt aId) {
	iItemId = aId;
}

TFollowingItemType CFollowingItem::GetItemType() {
	return iItemType;
}

TBool CFollowingItem::GetIsOwn() {
	return iIsOwn;
}

void CFollowingItem::SetIsOwn(TBool aIsOwn) {
	iIsOwn = aIsOwn;
}

TTime CFollowingItem::GetLastUpdated() {
	return iLastUpdated;
}

void CFollowingItem::SetLastUpdated(TTime aTime) {
	iLastUpdated = aTime;
}

TBool CFollowingItem::IsFiltered() {
	return iIsFiltered;
}

void CFollowingItem::SetFiltered(TBool aFilter) {
	iIsFiltered = aFilter;
}

TInt CFollowingItem::GetAvatarId() {
	return iAvatarId;
}

void CFollowingItem::SetAvatarId(TInt aAvatarId) {
	iAvatarId = aAvatarId;
}

TDesC& CFollowingItem::GetTitle() {
	return *iTitle;
}

void CFollowingItem::SetTitleL(const TDesC& aTitle) {
	if(iTitle)
		delete iTitle;

	iTitle = aTitle.AllocL();
}

TDesC& CFollowingItem::GetDescription() {
	return *iDescription;
}

void CFollowingItem::SetDescriptionL(const TDesC& aDescription) {
	if(iDescription)
		delete iDescription;

	iDescription = aDescription.AllocL();
}

/*
----------------------------------------------------------------------------
--
-- CFollowingNoticeItem
--
----------------------------------------------------------------------------
*/

CFollowingNoticeItem* CFollowingNoticeItem::NewL() {
	CFollowingNoticeItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CFollowingNoticeItem* CFollowingNoticeItem::NewLC(){
	CFollowingNoticeItem* self = new (ELeave) CFollowingNoticeItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CFollowingNoticeItem::~CFollowingNoticeItem() {
	if(iJid)
		delete iJid;
}

CFollowingNoticeItem::CFollowingNoticeItem() {
	iItemType = EItemNotice;
	iAvatarId = KIconNotice;
}

void CFollowingNoticeItem::ConstructL() {
	CFollowingItem::ConstructL();

	iJid = HBufC::NewL(0);
}

TDesC& CFollowingNoticeItem::GetJid() {
	return *iJid;
}

void CFollowingNoticeItem::SetJidL(const TDesC& aJid, TJidType aType) {
	if(iJid)
		delete iJid;

	iJid = aJid.AllocL();
	iJidType = aType;
}

TJidType CFollowingNoticeItem::GetJidType() {
	return iJidType;
}

/*
----------------------------------------------------------------------------
--
-- CFollowingContactItem
--
----------------------------------------------------------------------------
*/

CFollowingContactItem* CFollowingContactItem::NewL() {
	CFollowingContactItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CFollowingContactItem* CFollowingContactItem::NewLC(){
	CFollowingContactItem* self = new (ELeave) CFollowingContactItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CFollowingContactItem::CFollowingContactItem() {
	iItemType = EItemContact;
	iAddressbookId = -1;
}

TInt CFollowingContactItem::GetAddressbookId() {
	return iAddressbookId;
}

void CFollowingContactItem::SetAddressbookId(TInt aAddressbookId) {
	iAddressbookId = aAddressbookId;
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

CFollowingChannelItem::~CFollowingChannelItem() {
	if(iChannelJid)
		delete iChannelJid;
}

CFollowingChannelItem::CFollowingChannelItem() {
	iItemType = EItemChannel;
	iAvatarId = KIconChannel;
}

void CFollowingChannelItem::ConstructL() {
	CFollowingContactItem::ConstructL();

	iChannelJid = HBufC::NewL(0);
}

TDesC& CFollowingChannelItem::GetJid(TJidType /*aType*/) {
	return *iChannelJid;
}

void CFollowingChannelItem::SetJidL(const TDesC& aJid, TJidType /*aType*/) {
	if(iChannelJid)
		delete iChannelJid;

	iChannelJid = aJid.AllocL();
}

TPresenceState CFollowingChannelItem::GetPresence(TJidType /*aType*/) {
	return iChannelPresence;
}

void CFollowingChannelItem::SetPresence(TPresenceState aPresence, TJidType /*aType*/) {
	iChannelPresence = aPresence;
}

TBool CFollowingChannelItem::IsExternal() {
	return iExternal;
}

void CFollowingChannelItem::SetExternal(TBool aExternal) {
	iExternal = aExternal;
}

TInt CFollowingChannelItem::GetRank() {
	return iRank;
}

TInt CFollowingChannelItem::GetRankShift() {
	return iRankShift;
}

void CFollowingChannelItem::SetRank(TInt aRank, TInt aShift) {
	if(aShift != 0) {
		iRankShift = aShift;
	}
	else if(iRank != 0 && iRank != aRank) {
		iRankShift = iRank - aRank;
	}
	
	iRank = aRank;
}

void CFollowingChannelItem::MessageRead(TDesC& /*aJid*/, TInt /*aMessageId*/, TInt aUnreadMessages, TInt aUnreadReplies) {
	iChannelUnread = aUnreadMessages;
	iChannelReplies = aUnreadReplies;
}

TInt CFollowingChannelItem::GetUnread(TJidType /*aType*/) {
	return iChannelUnread;
}

TInt CFollowingChannelItem::GetReplies() {
	return iChannelReplies;
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
	if(iRosterJid)
		delete iRosterJid;
	
	if(iPlacePrevious)
		delete iPlacePrevious;
	
	if(iPlaceCurrent)
		delete iPlaceCurrent;
	
	if(iPlaceNext)
		delete iPlaceNext;
}

CFollowingRosterItem::CFollowingRosterItem() {
	iItemType = EItemRoster;
	iAvatarId = KIconPerson;

	iSubscription = ESubscriptionUnknown;
	iPubsubCollected = false;
}

void CFollowingRosterItem::ConstructL() {
	CFollowingChannelItem::ConstructL();
	
	iRosterJid = HBufC::NewL(0);

	iPlacePrevious = CBuddycloudBasicPlace::NewL();
	iPlaceCurrent = CBuddycloudBasicPlace::NewL();
	iPlaceNext = CBuddycloudBasicPlace::NewL();
}

TDesC& CFollowingRosterItem::GetJid(TJidType aType) {
	if(aType == EJidRoster) {
		return *iRosterJid;
	}
	
	return *iChannelJid;
}

void CFollowingRosterItem::SetJidL(TDesC& aJid, TJidType aType) {
	if(aType == EJidRoster) {
		if(iRosterJid)
			delete iRosterJid;

		iRosterJid = aJid.AllocL();
		
		if(GetTitle().Length() == 0) {
			TPtrC aName(aJid);
			TInt aLocate = aName.Locate('@');
			
			if(aLocate != KErrNotFound) {
				aName.Set(aName.Left(aLocate));
			}
			
			SetTitleL(aName);
		}
	}
	else {
		CFollowingChannelItem::SetJidL(aJid, aType);
	}
}

TPresenceState CFollowingRosterItem::GetPresence(TJidType aType) {
	if(aType == EJidChannel) {
		return iChannelPresence;
	}
	
	return iRosterPresence;
}

void CFollowingRosterItem::SetPresence(TPresenceState aPresence, TJidType aType) {
	if(aType == EJidRoster) {	
		iRosterPresence = aPresence;
	}
	else {
		iChannelPresence = aPresence;
	}
}

TSubscriptionType CFollowingRosterItem::GetSubscriptionType() {
	return iSubscription;
}

void CFollowingRosterItem::SetSubscriptionType(TSubscriptionType aSubscription) {
	iSubscription = aSubscription;
}

TBool CFollowingRosterItem::GetPubsubCollected() {
	return iPubsubCollected;
}

void CFollowingRosterItem::SetPubsubCollected(TBool aCollected) {
	iPubsubCollected = aCollected;
}

CBuddycloudBasicPlace* CFollowingRosterItem::GetPlace(TBuddycloudPlaceType aPlaceType) {
	switch(aPlaceType) {
		case EPlacePrevious:
			return iPlacePrevious;
		case EPlaceNext:
			return iPlaceNext;
		default:
			return iPlaceCurrent;			
	}
}

void CFollowingRosterItem::SetPlaceL(TBuddycloudPlaceType aPlaceType, CBuddycloudBasicPlace* aPlace) {
	switch(aPlaceType) {
		case EPlacePrevious:
			if(iPlacePrevious)
				delete iPlacePrevious;

			iPlacePrevious = aPlace;
			break;
		case EPlaceCurrent:
			if(iPlaceCurrent)
				delete iPlaceCurrent;

			iPlaceCurrent = aPlace;
			break;
		case EPlaceNext:
			if(iPlaceNext)
				delete iPlaceNext;

			iPlaceNext = aPlace;
			break;
	}
}

void CFollowingRosterItem::MessageRead(TDesC& aJid, TInt /*aMessageId*/, TInt aUnreadMessages, TInt aUnreadReplies) {
	if(aJid.Compare(*iRosterJid) == 0) {
		iRosterUnread = aUnreadMessages;
	}
	else {
		iChannelUnread = aUnreadMessages;
		iChannelReplies = aUnreadReplies;
	}
}

TInt CFollowingRosterItem::GetUnread(TJidType aType) {
	if(aType == EJidRoster) {
		return iRosterUnread;
	}
	
	return iChannelUnread;
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
	return self;
}

CBuddycloudFollowingStore::~CBuddycloudFollowingStore() {
	for(TInt i = 0; i < iItemStore.Count();i++) {
		if(iItemStore[i]->GetItemType() == EItemChannel) {
			delete (static_cast <CFollowingChannelItem*> (iItemStore[i]));
		}
		else if(iItemStore[i]->GetItemType() == EItemRoster) {
			delete (static_cast <CFollowingRosterItem*> (iItemStore[i]));
		}
		else if(iItemStore[i]->GetItemType() == EItemContact) {
			delete (static_cast <CFollowingContactItem*> (iItemStore[i]));
		}
		else if(iItemStore[i]->GetItemType() == EItemNotice) {
			delete (static_cast <CFollowingNoticeItem*> (iItemStore[i]));
		}
		else {
			delete iItemStore[i];
		}
	}

	iItemStore.Close();
}

TInt CBuddycloudFollowingStore::Count() {
	return iItemStore.Count();
}

CFollowingItem* CBuddycloudFollowingStore::GetItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		return iItemStore[aIndex];
	}

	return NULL;
}

CFollowingItem* CBuddycloudFollowingStore::GetItemById(TInt aItemId) {
	for(TInt i = 0;i < iItemStore.Count();i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			return iItemStore[i];
		}
	}

	return NULL;
}

TInt CBuddycloudFollowingStore::GetIdByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		return iItemStore[aIndex]->GetItemId();
	}

	return 0;
}

TInt CBuddycloudFollowingStore::GetIndexById(TInt aItemId) {
	for(TInt i = 0;i < iItemStore.Count();i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			return i;
		}
	}

	return 0;
}

void CBuddycloudFollowingStore::MoveItemByIndex(TInt aIndex, TInt aPosition) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		CFollowingItem* aItem = iItemStore[aIndex];

		iItemStore.Remove(aIndex);
		iItemStore.Insert(aItem, aPosition);
	}
}

void CBuddycloudFollowingStore::MoveItemById(TInt aItemId, TInt aPosition) {
	if(aPosition <= iItemStore.Count()) {
		for(TInt i = 0;i < iItemStore.Count();i++) {
			if(iItemStore[i]->GetItemId() == aItemId) {
				CFollowingItem* aItem = iItemStore[i];

				iItemStore.Remove(i);
				iItemStore.Insert(aItem, aPosition);
				
				break;
			}
		}
	}
}

void CBuddycloudFollowingStore::AddItem(CFollowingItem* aItem) {
	iItemStore.Append(aItem);
}

void CBuddycloudFollowingStore::InsertItem(TInt aIndex, CFollowingItem* aItem) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		iItemStore.Insert(aItem, aIndex);
	}
	else {
		iItemStore.Append(aItem);
	}
}

void CBuddycloudFollowingStore::RemoveItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		iItemStore.Remove(aIndex);
	}
}

void CBuddycloudFollowingStore::RemoveItemById(TInt aItemId) {
	for(TInt i = 0; i < iItemStore.Count(); i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			iItemStore.Remove(i);
			
			break;
		}
	}
}

void CBuddycloudFollowingStore::RemoveAll() {
	iItemStore.Reset();
}

void CBuddycloudFollowingStore::DeleteItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		delete iItemStore[aIndex];
		iItemStore.Remove(aIndex);
	}
}

void CBuddycloudFollowingStore::DeleteItemById(TInt aItemId) {
	for(TInt i = 0; i < iItemStore.Count(); i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			delete iItemStore[i];
			iItemStore.Remove(i);
			
			break;
		}
	}
}
