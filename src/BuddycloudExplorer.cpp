/*
============================================================================
 Name        : 	BuddycloudExplorer.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Storage types for Buddycloud explorer data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudExplorer.h"
#include "TextUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CExplorerStanzaBuilder
--
----------------------------------------------------------------------------
*/
		
TExplorerQuery CExplorerStanzaBuilder::BuildNearbyXmppStanza(TExplorerItemType aType, const TDesC8& aReferenceId, TInt aOptionsLimit) {
	TExplorerQuery aQuery;			
	
	_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='exp_nearby1'><query xmlns='urn:oslo:nearbyobjects'><reference type='' id=''/><options limit='%d'/></query></iq>\r\n");
	aQuery.iStanza.Format(KNearbyStanza, aOptionsLimit);	
	aQuery.iStanza.Insert(120, aReferenceId);
	
	switch(aType) {
		case EExplorerItemPerson:
			aQuery.iStanza.Insert(114, _L8("person"));
			break;
		case EExplorerItemChannel:
			aQuery.iStanza.Insert(114, _L8("channel"));
			break;
		default:
			aQuery.iStanza.Insert(114, _L8("place"));
			break;
	}
	
	return aQuery;
}
		
TExplorerQuery CExplorerStanzaBuilder::BuildNearbyXmppStanza(TReal aPointLatitude, TReal aPointLongitude, TInt aOptionsLimit) {
	TExplorerQuery aQuery;			

	_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='exp_nearby1'><query xmlns='urn:oslo:nearbyobjects'><point lat='%.6f' lon='%.6f'/><options limit='%d'/></query></iq>\r\n");
	aQuery.iStanza.Format(KNearbyStanza, aPointLatitude, aPointLongitude, aOptionsLimit);
	
	return aQuery;
}

TExplorerQuery CExplorerStanzaBuilder::BuildChannelsXmppStanza(const TDesC8& aSubject, TExplorerChannelRole aRole, TInt aOptionsLimit) {
	TExplorerQuery aQuery;			
	
	aQuery.iStanza.Append(_L8("<iq to='maitred.buddycloud.com' type='get' id='exp_channels1'><query xmlns='http://buddycloud.com/protocol/channels'><subject></subject></query></iq>\r\n"));
	
	if(aOptionsLimit > 0) {
		TBuf8<32> aMaxElement;
		aMaxElement.Format(_L8("<max>%d</max>"), aOptionsLimit);
		aQuery.iStanza.Insert(136, aMaxElement);
	}
	
	switch(aRole) {
		case EChannelProducer:
			aQuery.iStanza.Insert(136, _L8("<role>producer</role>"));
			break;
		case EChannelModerator:
			aQuery.iStanza.Insert(136, _L8("<role>moderator</role>"));
			break;
		case EChannelFollower:
			aQuery.iStanza.Insert(136, _L8("<role>follower</role>"));
			break;
		default:;
	}
	
	aQuery.iStanza.Insert(126, aSubject);
	
	return aQuery;
}	

TExplorerQuery CExplorerStanzaBuilder::BuildPlaceVisitorsXmppStanza(const TDesC8& aNode, TInt aPlaceId) {
	TExplorerQuery aQuery;			
	
	_LIT8(KPlaceVisitorsStanza, "<iq to='butler.buddycloud.com' type='get' id='exp_visitors'><query xmlns='http://buddycloud.com/protocol/place#visitors' node=''><place><id>http://buddycloud.com/places/%d</id></place></query></iq>");
	aQuery.iStanza.Format(KPlaceVisitorsStanza, aPlaceId);
	aQuery.iStanza.Insert(127, aNode);
	
	return aQuery;
}

/*
----------------------------------------------------------------------------
--
-- CExplorerResultItem
--
----------------------------------------------------------------------------
*/

CExplorerResultItem* CExplorerResultItem::NewL() {
	CExplorerResultItem* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CExplorerResultItem* CExplorerResultItem::NewLC() {
	CExplorerResultItem* self = new (ELeave) CExplorerResultItem();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CExplorerResultItem::~CExplorerResultItem() {
	if(iId) {
		delete iId;
	}
	
	if(iGeoloc) {
		delete iGeoloc;
	}
}

void CExplorerResultItem::ConstructL() {
	CBuddycloudListItem::ConstructL();
	
	iId = HBufC::NewL(0);
	iGeoloc = CGeolocData::NewL();
}

TExplorerItemType CExplorerResultItem::GetResultType() {
	return iResultType;
}

void CExplorerResultItem::SetResultType(TExplorerItemType aResultType) {
	iResultType = aResultType;
}

TInt CExplorerResultItem::GetOverlayId() {
	return iOverlayId;
}

void CExplorerResultItem::SetOverlayId(TInt aOverlayId) {
	iOverlayId = aOverlayId;
}

TInt CExplorerResultItem::GetRank() {
	return iRank;
}

void CExplorerResultItem::SetRank(TInt aRank) {
	iRank = aRank;
}

TDesC& CExplorerResultItem::GetId() {
	return *iId;
}

void CExplorerResultItem::SetIdL(const TDesC& aId) {
	if(iId) {
		delete iId;
	}

	iId = aId.AllocL();	
}

CGeolocData* CExplorerResultItem::GetGeoloc() {
	return iGeoloc;
}

void CExplorerResultItem::SetGeolocL(CGeolocData* aGeoloc) {
	// Set geoloc
	if(iGeoloc) {
		delete iGeoloc;
	}
	
	iGeoloc = aGeoloc;
	
	// Set id
	if(iGeoloc->GetString(EGeolocUri).Length() > 0) {
		TInt aResult = iGeoloc->GetString(EGeolocUri).LocateReverse('/');
		
		if(aResult != KErrNotFound) {
			TLex aLex(iGeoloc->GetString(EGeolocUri).Mid(aResult + 1));
			aLex.Val(iItemId);
		}
	}
}

void CExplorerResultItem::UpdateFromGeolocL() {
	// Format distance
	TBuf<32> aDistance;
	
	if(iDistance < 999) {
		_LIT(KDistance, " (%dm)");
		aDistance.Format(KDistance, iDistance);
	}
	else {
		_LIT(KDistance, " (%.2fkm)");
		aDistance.Format(KDistance, (TReal(iDistance) / 1000.0));
	}
	// Set description
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	
	HBufC* aDescription = HBufC::NewLC(iGeoloc->GetString(EGeolocStreet).Length() + iGeoloc->GetString(EGeolocArea).Length() + 
			iGeoloc->GetString(EGeolocLocality).Length() + iGeoloc->GetString(EGeolocPostalcode).Length() + 
			iGeoloc->GetString(EGeolocRegion).Length() + iGeoloc->GetString(EGeolocCountry).Length() + 9 + aDistance.Length());
	TPtr pDescription(aDescription->Des());
	
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocStreet), _L(""));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocArea), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocLocality), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocPostalcode), _L(" "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocRegion), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocCountry), _L(", "));
	
	if(iDistance > 0) {
		pDescription.Append(aDistance);
	}
	
	SetDescriptionL(pDescription);
	CleanupStack::PopAndDestroy(2); // CExplorerResultItem, aTextUtilities	
}

/*
----------------------------------------------------------------------------
--
-- CExplorerQueryLevel
--
----------------------------------------------------------------------------
*/

CExplorerQueryLevel* CExplorerQueryLevel::NewL() {
	CExplorerQueryLevel* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CExplorerQueryLevel* CExplorerQueryLevel::NewLC() {
	CExplorerQueryLevel* self = new (ELeave) CExplorerQueryLevel();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CExplorerQueryLevel::~CExplorerQueryLevel() {
	if(iQueryTitle) {
		delete iQueryTitle;
	}
	
	if(iQueriedStanza) {
		delete iQueriedStanza;
	}
	
	ClearResultItems();
	iResultItems.Close();
}

void CExplorerQueryLevel::ConstructL() {
	iSelectedResultItem = 0;
	
	iQueryTitle = HBufC::NewL(0);
	iQueriedStanza = HBufC8::NewL(0);	
}

TDesC& CExplorerQueryLevel::GetQueryTitle() {
	return *iQueryTitle;
}

void CExplorerQueryLevel::SetQueryTitleL(const TDesC& aTitle) {
	if(iQueryTitle) {
		delete iQueryTitle;
	}

	iQueryTitle = aTitle.AllocL();	
}

TDesC8& CExplorerQueryLevel::GetQueriedStanza() {
	return *iQueriedStanza;
}

void CExplorerQueryLevel::SetQueriedStanzaL(const TDesC8& aStanza) {
	if(iQueriedStanza) {
		delete iQueriedStanza;
	}

	iQueriedStanza = aStanza.AllocL();		
}

void CExplorerQueryLevel::ClearResultItems() {
	for(TInt i = 0; i < iResultItems.Count(); i++) {
		delete iResultItems[i];
	}
	
	iResultItems.Reset();
}

void CExplorerQueryLevel::AppendSortedItem(CExplorerResultItem* aResultItem, TSortByType aSort) {
	if(aSort == ESortByDistance) {
		for(TInt i = 0; i < iResultItems.Count(); i++) {
			if(aResultItem->GetDistance() < iResultItems[i]->GetDistance()) {
				iResultItems.Insert(aResultItem, i);
				
				return;
			}
		}		
	}
	
	iResultItems.Append(aResultItem);
}

// End of File
