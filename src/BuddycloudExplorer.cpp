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

#include <eikenv.h>
#include "AvatarConstants.h"
#include "BuddycloudExplorer.h"
#include "TextUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CExplorerStanzaBuilder
--
----------------------------------------------------------------------------
*/
		
void CExplorerStanzaBuilder::BuildButlerXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aReferenceJid, TInt aOptionsLimit) {
	_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='urn:oslo:nearbyobjects'><reference type='person' id='$JID'/><options limit='%d'/></query></iq>\r\n");
	aString.Format(KNearbyStanza, EXmppIdGetNearbyObjects, aStampId, aOptionsLimit);	
	aString.Replace(aString.Find(_L8("$JID")), 4, aReferenceJid);
}
		
void CExplorerStanzaBuilder::BuildButlerXmppStanza(TDes8& aString, TInt aStampId, TReal aPointLatitude, TReal aPointLongitude, TInt aOptionsLimit) {
	_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='urn:oslo:nearbyobjects'><point lat='%.6f' lon='%.6f'/><options limit='%d'/></query></iq>\r\n");
	aString.Format(KNearbyStanza, EXmppIdGetNearbyObjects, aStampId, aPointLatitude, aPointLongitude, aOptionsLimit);
}

void CExplorerStanzaBuilder::BuildBroadcasterXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aNodeId) {
	_LIT8(KBroadcasterStanza, "<iq to='' type='get' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''/></pubsub></iq>\r\n");
	aString.Format(KBroadcasterStanza, EXmppIdGetNodeAffiliations, aStampId);
	aString.Insert(aString.Length() - 119, KBuddycloudPubsubServer);
	aString.Insert(aString.Length() - 19, aNodeId);
}	

void CExplorerStanzaBuilder::BuildMaitredXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aItemId,  const TDesC8& aItemVar) {
	_LIT8(KMaitredStanza, "<iq to='maitred.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/channels'><items id='' var=''/></query></iq>\r\n");
	aString.AppendFormat(KMaitredStanza, EXmppIdGetMaitredList, aStampId);
	aString.Insert(aString.Length() - 25, aItemId);
	aString.Insert(aString.Length() - 18, aItemVar);
}

void CExplorerStanzaBuilder::BuildTitleFromResource(TDes& aString, TInt aResourceId, const TDesC& aReplaceData, const TDesC& aWithData) {
	if(aResourceId != KErrNotFound) {
		CEikonEnv::Static()->ReadResource(aString, aResourceId);		
		TInt aLocate = aString.FindF(aReplaceData);
		
		if(aLocate != KErrNotFound) {
			aString.Replace(aLocate, aReplaceData.Length(), aWithData.Left(aString.MaxLength() - aString.Length() + aReplaceData.Length()));
		}
	}
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
	
	for(TInt i = 0; i < iStatistics.Count(); i++) {
		delete iStatistics[i];
	}
	
	iStatistics.Close();
}

CExplorerResultItem::CExplorerResultItem() {
	iResultType = EExplorerItemPerson;
	iIconId = KIconPerson;
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

TDesC& CExplorerResultItem::GetId() {
	return *iId;
}

void CExplorerResultItem::SetIdL(const TDesC& aId) {
	if(iId) {
		delete iId;
	}

	iId = aId.AllocL();	
}

TInt CExplorerResultItem::StatisticCount() {
	return iStatistics.Count();
}

TDesC& CExplorerResultItem::GetStatistic(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iStatistics.Count()) {
		return *iStatistics[aIndex];
	}
	
	return iNullString;
}

void CExplorerResultItem::AddStatisticL(const TDesC& aStatistic) {
	iStatistics.Append(aStatistic.AllocL());
}

TXmppPubsubAffiliation CExplorerResultItem::GetChannelAffiliation() {
	return iAffiliation;
}

void CExplorerResultItem::SetChannelAffiliation(TXmppPubsubAffiliation aAffiliation) {
	iAffiliation = aAffiliation;
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
		TInt aLocate = iGeoloc->GetString(EGeolocUri).LocateReverse('/');
		
		if(aLocate != KErrNotFound) {
			TLex aLex(iGeoloc->GetString(EGeolocUri).Mid(aLocate + 1));
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
	CleanupStack::PopAndDestroy(2); // aDescription, aTextUtilities	
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
	iQueriedChannel = NULL;
	
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

CFollowingChannelItem* CExplorerQueryLevel::GetQueriedChannel() {
	return iQueriedChannel;
}

void CExplorerQueryLevel::SetQueriedChannel(CFollowingChannelItem* aChannelItem) {
	iQueriedChannel = aChannelItem;
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
