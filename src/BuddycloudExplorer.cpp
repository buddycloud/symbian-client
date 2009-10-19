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
	
	_LIT8(KPlaceVisitorsStanza, "<iq to='butler.buddycloud.com' type='get' id='exp_users1'><query xmlns='http://buddycloud.com/protocol/place#visitors' node=''><place><id>http://buddycloud.com/places/%d</id></place></query></iq>");
	aQuery.iStanza.Format(KPlaceVisitorsStanza, aPlaceId);
	aQuery.iStanza.Insert(125, aNode);
	
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
	
	if(iTitle) {
		delete iTitle;
	}
	
	if(iDescription) {
		delete iDescription;
	}
	
	if(iLocation) {
		delete iLocation;
	}
}

void CExplorerResultItem::ConstructL() {
	iId = HBufC::NewL(0);
	iTitle = HBufC::NewL(0);
	iDescription = HBufC::NewL(0);	
	
	iLocation = CBuddycloudBasicPlace::NewL();
}

TExplorerItemType CExplorerResultItem::GetResultType() {
	return iResultType;
}

void CExplorerResultItem::SetResultType(TExplorerItemType aResultType) {
	iResultType = aResultType;
}

TInt CExplorerResultItem::GetAvatarId() {
	return iAvatarId;
}

void CExplorerResultItem::SetAvatarId(TInt aAvatarId) {
	iAvatarId = aAvatarId;
}

TInt CExplorerResultItem::GetOverlayId() {
	return iOverlayId;
}

void CExplorerResultItem::SetOverlayId(TInt aOverlayId) {
	iOverlayId = aOverlayId;
}

TInt CExplorerResultItem::GetDistance() {
	return iDistance;
}

void CExplorerResultItem::SetDistance(TInt aDistance) {
	iDistance = aDistance;
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

TDesC& CExplorerResultItem::GetTitle() {
	return *iTitle;
}

void CExplorerResultItem::SetTitleL(const TDesC& aTitle) {
	if(iTitle) {
		delete iTitle;
	}

	iTitle = aTitle.AllocL();	
}

TDesC& CExplorerResultItem::GetDescription() {
	return *iDescription;
}

void CExplorerResultItem::SetDescriptionL(const TDesC& aDescription) {
	if(iDescription) {
		delete iDescription;
	}

	iDescription = aDescription.AllocL();	
}

void CExplorerResultItem::GenerateDescriptionL() {
	if(iDescription) {
		delete iDescription;
	}
	
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

	// Create description string
	iDescription = HBufC::NewL(iLocation->GetPlaceStreet().Length() + iLocation->GetPlaceArea().Length() + 
			iLocation->GetPlaceCity().Length() + iLocation->GetPlaceRegion().Length() + 
			iLocation->GetPlaceCountry().Length() + 8 + aDistance.Length());
	
	// Generate
	AddDescriptionDelimitedString(iLocation->GetPlaceStreet());
	AddDescriptionDelimitedString(iLocation->GetPlaceArea());
	AddDescriptionDelimitedString(iLocation->GetPlaceCity());
	AddDescriptionDelimitedString(iLocation->GetPlaceRegion());
	AddDescriptionDelimitedString(iLocation->GetPlaceCountry());
	
	if(iDistance > 0) {
		TPtr pDescription(iDescription->Des());
		pDescription.Append(aDistance);
	}
}

void CExplorerResultItem::AddDescriptionDelimitedString(TDesC& aString) {
	TPtr pDescription(iDescription->Des());
	
	if(pDescription.Length() > 0 && aString.Length() > 0) {
		pDescription.Append(_L(", "));
	}
	
	pDescription.Append(aString);
}

CBuddycloudBasicPlace* CExplorerResultItem::GetLocation() {
	return iLocation;
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

// End of File
