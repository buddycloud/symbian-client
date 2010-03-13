/*
============================================================================
 Name        : BuddycloudList.cpp
 Author      : Ross Savage
 Copyright   : 2009 Buddycloud
 Description : Generic list class & storage
============================================================================
*/

#include "BuddycloudList.h"


/*
----------------------------------------------------------------------------
--
-- CBuddycloudListItem
--
----------------------------------------------------------------------------
*/
	
CBuddycloudListItem* CBuddycloudListItem::NewL() {
	CBuddycloudListItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudListItem* CBuddycloudListItem::NewLC(){
	CBuddycloudListItem* self = new (ELeave) CBuddycloudListItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CBuddycloudListItem::~CBuddycloudListItem() {
	if(iTitle)
		delete iTitle;
	
	if(iSubTitle)
		delete iSubTitle;
	
	if(iDescription)
		delete iDescription;
}

CBuddycloudListItem::CBuddycloudListItem() {
	iLastUpdated = TTime();
	iFiltered = true;
}

void CBuddycloudListItem::ConstructL() {
	iTitle = HBufC::NewL(0);
	iSubTitle = HBufC::NewL(0);
	iDescription = HBufC::NewL(0);
}

TInt CBuddycloudListItem::GetItemId() {
	return iItemId;
}

void CBuddycloudListItem::SetItemId(TInt aItemId) {
	iItemId = aItemId;
}

TTime CBuddycloudListItem::GetLastUpdated() {
	return iLastUpdated;
}

void CBuddycloudListItem::SetLastUpdated(TTime aTime) {
	iLastUpdated = aTime;
}

TInt CBuddycloudListItem::GetDistance() {
	return iDistance;
}

void CBuddycloudListItem::SetDistance(TInt aDistance) {
	iDistance = aDistance;
}

TBool CBuddycloudListItem::Filtered() {
	return iFiltered;
}

void CBuddycloudListItem::SetFiltered(TBool aFilter) {
	iFiltered = aFilter;
}

TInt CBuddycloudListItem::GetIconId() {
	return iIconId;
}

void CBuddycloudListItem::SetIconId(TInt aIconId) {
	iIconId = aIconId;
}

TDesC& CBuddycloudListItem::GetTitle() {
	return *iTitle;
}

void CBuddycloudListItem::SetTitleL(const TDesC& aTitle) {
	if(iTitle)
		delete iTitle;

	iTitle = aTitle.AllocL();
}

TDesC& CBuddycloudListItem::GetSubTitle() {
	return *iSubTitle;
}

void CBuddycloudListItem::SetSubTitleL(const TDesC& aSubTitle) {
	if(iSubTitle)
		delete iSubTitle;

	iSubTitle = aSubTitle.AllocL();
}

TDesC& CBuddycloudListItem::GetDescription() {
	return *iDescription;
}

void CBuddycloudListItem::SetDescriptionL(const TDesC& aDescription) {
	if(iDescription)
		delete iDescription;

	iDescription = aDescription.AllocL();
}

/*
----------------------------------------------------------------------------
--
-- CBuddycloudListStore
--
----------------------------------------------------------------------------
*/

CBuddycloudListStore* CBuddycloudListStore::NewL() {
	CBuddycloudListStore* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudListStore* CBuddycloudListStore::NewLC() {
	CBuddycloudListStore* self = new (ELeave) CBuddycloudListStore();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CBuddycloudListStore::~CBuddycloudListStore() {
	if(iFilterText) {
		delete iFilterText;
	}
	
	// List
	for(TInt i = 0; i < iItemStore.Count();i++) {
		delete iItemStore[i];
	}

	iItemStore.Close();
}

void CBuddycloudListStore::ConstructL() {
	iFilterText = HBufC::NewL(0);
}

TInt CBuddycloudListStore::Count() {
	return iItemStore.Count();
}

CBuddycloudListItem* CBuddycloudListStore::GetItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		return iItemStore[aIndex];
	}

	return NULL;
}

CBuddycloudListItem* CBuddycloudListStore::GetItemById(TInt aItemId) {
	for(TInt i = 0;i < iItemStore.Count();i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			return iItemStore[i];
		}
	}

	return NULL;
}

TInt CBuddycloudListStore::GetIdByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		return iItemStore[aIndex]->GetItemId();
	}

	return 0;
}

TInt CBuddycloudListStore::GetIndexById(TInt aItemId) {
	for(TInt i = 0;i < iItemStore.Count();i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			return i;
		}
	}

	return 0;
}

void CBuddycloudListStore::MoveItemByIndex(TInt aIndex, TInt aPosition) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		CBuddycloudListItem* aItem = iItemStore[aIndex];

		iItemStore.Remove(aIndex);
		iItemStore.Insert(aItem, aPosition);
	}
}

void CBuddycloudListStore::MoveItemById(TInt aItemId, TInt aPosition) {
	if(aPosition <= iItemStore.Count()) {
		for(TInt i = 0;i < iItemStore.Count();i++) {
			if(iItemStore[i]->GetItemId() == aItemId) {
				CBuddycloudListItem* aItem = iItemStore[i];

				iItemStore.Remove(i);
				iItemStore.Insert(aItem, aPosition);
				
				break;
			}
		}
	}
}

TInt CBuddycloudListStore::AddItem(CBuddycloudListItem* aItem) {
	iItemStore.Append(aItem);
	
	return (iItemStore.Count() - 1);
}

void CBuddycloudListStore::InsertItem(TInt aIndex, CBuddycloudListItem* aItem) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		iItemStore.Insert(aItem, aIndex);
	}
	else {
		iItemStore.Append(aItem);
	}
}

void CBuddycloudListStore::BubbleItem(TInt aIndex, TBubble aDirection) {	
	if((aDirection == EBubbleUp && aIndex > 0) || (aDirection == EBubbleDown && aIndex < iItemStore.Count())) {
		CBuddycloudListItem* aItem = iItemStore[aIndex];
		
		iItemStore.Remove(aIndex);
		iItemStore.Insert(aItem, (aIndex + aDirection));
	}
}

void CBuddycloudListStore::RemoveItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		iItemStore.Remove(aIndex);
	}
}

void CBuddycloudListStore::RemoveItemById(TInt aItemId) {
	for(TInt i = 0; i < iItemStore.Count(); i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			iItemStore.Remove(i);
			
			break;
		}
	}
}

void CBuddycloudListStore::RemoveAll() {
	iItemStore.Reset();
}

void CBuddycloudListStore::DeleteItemByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iItemStore.Count()) {
		delete iItemStore[aIndex];
		iItemStore.Remove(aIndex);
	}
}

void CBuddycloudListStore::DeleteItemById(TInt aItemId) {
	for(TInt i = 0; i < iItemStore.Count(); i++) {
		if(iItemStore[i]->GetItemId() == aItemId) {
			delete iItemStore[i];
			iItemStore.Remove(i);
			
			break;
		}
	}
}

TDesC& CBuddycloudListStore::GetFilterText() {
	return *iFilterText;
}

TBool CBuddycloudListStore::SetFilterTextL(const TDesC& aFilterText) {
	TPtrC aOldFilterText(iFilterText->Des());

	if(aOldFilterText.Compare(aFilterText) != 0) {
		// Set filter text
		if(iFilterText) {
			delete iFilterText;
		}
		
		iFilterText = aFilterText.AllocL();
		
		// Filter
		FilterL();
		
		return true;
	}
	
	return false;
}

void CBuddycloudListStore::FilterItemL(TInt aIndex) {	
	CBuddycloudListItem* aItem = iItemStore[aIndex];
	TPtrC aFilterText(iFilterText->Des());
	
	if(aItem && ((aFilterText.Length() <= aItem->GetTitle().Length() && 
				aItem->GetTitle().FindF(aFilterText) != KErrNotFound) || 
			(aFilterText.Length() <= aItem->GetDescription().Length() && 
				aItem->GetDescription().FindF(aFilterText) != KErrNotFound))) {
			
		// Title or description match
		aItem->SetFiltered(true);
	}
}

void CBuddycloudListStore::FilterL() {
	TPtrC aFilterText(iFilterText->Des());
	
	if(aFilterText.Length() == 0) {
		// Reset filter
		for(TInt i = 0; i < iItemStore.Count(); i++) {
			iItemStore[i]->SetFiltered(true);
		}
	}
	else {
		// Filter items
		for(TInt i = 0; i < iItemStore.Count(); i++) {
			iItemStore[i]->SetFiltered(false);

			FilterItemL(i);
		}
	}
}
