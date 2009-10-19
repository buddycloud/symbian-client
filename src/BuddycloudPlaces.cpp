/*
============================================================================
 Name        : 	BuddycloudPlaces.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Storage of Buddycloud place data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudPlaces.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlace
--
----------------------------------------------------------------------------
*/

CBuddycloudBasicPlace* CBuddycloudBasicPlace::NewL() {
	CBuddycloudBasicPlace* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudBasicPlace* CBuddycloudBasicPlace::NewLC() {
	CBuddycloudBasicPlace* self = new (ELeave) CBuddycloudBasicPlace();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CBuddycloudBasicPlace::~CBuddycloudBasicPlace() {
	if(iPlaceName)
		delete iPlaceName;

	if(iPlaceStreet)
		delete iPlaceStreet;

	if(iPlaceArea)
		delete iPlaceArea;

	if(iPlaceCity)
		delete iPlaceCity;

	if(iPlacePostcode)
		delete iPlacePostcode;

	if(iPlaceRegion)
		delete iPlaceRegion;

	if(iPlaceCountry)
		delete iPlaceCountry;
}

CBuddycloudBasicPlace::CBuddycloudBasicPlace() {
	iPlaceId = 0;
	iPublic = false;
	iRevision = KErrNotFound;

	iLatitude = 0.0;
	iLongitude = 0.0;
}

void CBuddycloudBasicPlace::ConstructL() {
	iPlaceName = HBufC::NewL(0);
	iPlaceStreet = HBufC::NewL(0);
	iPlaceArea = HBufC::NewL(0);
	iPlaceCity = HBufC::NewL(0);
	iPlacePostcode = HBufC::NewL(0);
	iPlaceRegion = HBufC::NewL(0);
	iPlaceCountry = HBufC::NewL(0);
}

TInt CBuddycloudBasicPlace::GetPlaceId() {
	return iPlaceId;
}

void CBuddycloudBasicPlace::SetPlaceId(TInt aPlaceId) {
	iPlaceId = aPlaceId;
}

TBool CBuddycloudBasicPlace::GetShared() {
	return iPublic;
}

void CBuddycloudBasicPlace::SetShared(TBool aPublic) {
	iPublic = aPublic;
}

TInt CBuddycloudBasicPlace::GetRevision() {
	return iRevision;
}

void CBuddycloudBasicPlace::SetRevision(TInt aRevision) {
	iRevision = aRevision;
}

TDesC& CBuddycloudBasicPlace::GetPlaceName() {
	return *iPlaceName;
}

void CBuddycloudBasicPlace::SetPlaceNameL(TDesC& aPlaceName) {
	if(iPlaceName)
		delete iPlaceName;

	iPlaceName = aPlaceName.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlaceStreet() {
	return *iPlaceStreet;
}

void CBuddycloudBasicPlace::SetPlaceStreetL(TDesC& aPlaceStreet) {
	if(iPlaceStreet)
		delete iPlaceStreet;

	iPlaceStreet = aPlaceStreet.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlaceArea() {
	return *iPlaceArea;
}

void CBuddycloudBasicPlace::SetPlaceAreaL(TDesC& aPlaceArea) {
	if(iPlaceArea)
		delete iPlaceArea;

	iPlaceArea = aPlaceArea.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlaceCity() {
	return *iPlaceCity;
}

void CBuddycloudBasicPlace::SetPlaceCityL(TDesC& aPlaceCity) {
	if(iPlaceCity)
		delete iPlaceCity;

	iPlaceCity = aPlaceCity.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlacePostcode() {
	return *iPlacePostcode;
}

void CBuddycloudBasicPlace::SetPlacePostcodeL(TDesC& aPlacePostcode) {
	if(iPlacePostcode)
		delete iPlacePostcode;

	iPlacePostcode = aPlacePostcode.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlaceRegion() {
	return *iPlaceRegion;
}

void CBuddycloudBasicPlace::SetPlaceRegionL(TDesC& aPlaceRegion) {
	if(iPlaceRegion)
		delete iPlaceRegion;

	iPlaceRegion = aPlaceRegion.AllocL();
}

TDesC& CBuddycloudBasicPlace::GetPlaceCountry() {
	return *iPlaceCountry;
}

void CBuddycloudBasicPlace::SetPlaceCountryL(TDesC& aPlaceCountry) {
	if(iPlaceCountry)
		delete iPlaceCountry;

	iPlaceCountry = aPlaceCountry.AllocL();
}

TReal CBuddycloudBasicPlace::GetPlaceLatitude() {
	return iLatitude;
}

void CBuddycloudBasicPlace::SetPlaceLatitude(TReal aLatitude) {
	iLatitude = aLatitude;
}

TReal CBuddycloudBasicPlace::GetPlaceLongitude() {
	return iLongitude;
}

void CBuddycloudBasicPlace::SetPlaceLongitude(TReal aLongitude) {
	iLongitude = aLongitude;
}

void CBuddycloudBasicPlace::CopyDetailsL(CBuddycloudBasicPlace* aPlace) {
	if(aPlace) {
		SetPlaceNameL(aPlace->GetPlaceName());
		SetPlaceStreetL(aPlace->GetPlaceStreet());
		SetPlaceAreaL(aPlace->GetPlaceArea());
		SetPlaceCityL(aPlace->GetPlaceCity());
		SetPlacePostcodeL(aPlace->GetPlacePostcode());
		SetPlaceRegionL(aPlace->GetPlaceRegion());
		SetPlaceCountryL(aPlace->GetPlaceCountry());
		SetPlaceLatitude(aPlace->GetPlaceLatitude());
		SetPlaceLongitude(aPlace->GetPlaceLongitude());
	}
}

/*
----------------------------------------------------------------------------
--
-- CBuddycloudExtendedPlace
--
----------------------------------------------------------------------------
*/

CBuddycloudExtendedPlace* CBuddycloudExtendedPlace::NewL() {
	CBuddycloudExtendedPlace* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudExtendedPlace* CBuddycloudExtendedPlace::NewLC() {
	CBuddycloudExtendedPlace* self = new (ELeave) CBuddycloudExtendedPlace();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CBuddycloudExtendedPlace::~CBuddycloudExtendedPlace() {
	if(iDescription)
		delete iDescription;
	
	if(iWebsite)
		delete iWebsite;
	
	if(iWiki)
		delete iWiki;
}

CBuddycloudExtendedPlace::CBuddycloudExtendedPlace() {
	iPlaceSeen = EPlaceNotSeen;
	iLastSeen.UniversalTime();

	iVisits = 0;
	iVisitSeconds = 0;
	iTotalSeconds = 0;

	iPopulation = 0;
}

void CBuddycloudExtendedPlace::ConstructL() {
	CBuddycloudBasicPlace::ConstructL();
	
	iDescription = HBufC::NewL(0);
	iWebsite = HBufC::NewL(0);
	iWiki = HBufC::NewL(0);
}

TDesC& CBuddycloudExtendedPlace::GetDescription() {
	return *iDescription;
}

void CBuddycloudExtendedPlace::SetDescriptionL(TDesC& aDescription) {
	if(iDescription)
		delete iDescription;

	iDescription = aDescription.AllocL();
}

TDesC& CBuddycloudExtendedPlace::GetWebsite() {
	return *iWebsite;
}

void CBuddycloudExtendedPlace::SetWebsiteL(TDesC& aWebsite) {
	if(iWebsite)
		delete iWebsite;

	iWebsite = aWebsite.AllocL();
}

TDesC& CBuddycloudExtendedPlace::GetWiki() {
	return *iWiki;
}

void CBuddycloudExtendedPlace::SetWikiL(TDesC& aWiki) {
	if(iWiki)
		delete iWiki;

	iWiki = aWiki.AllocL();
}

TBuddycloudPlaceSeen CBuddycloudExtendedPlace::GetPlaceSeen() {
	return iPlaceSeen;
}

void CBuddycloudExtendedPlace::SetPlaceSeen(TBuddycloudPlaceSeen aPlaceSeen) {
	if(iPlaceSeen != EPlaceHere && iPlaceSeen != EPlaceFlux && aPlaceSeen == EPlaceHere) {
		// Transition to Current
		iVisits++;
		iVisitSeconds = 0;

		iLastSeen.UniversalTime();
	}

	iPlaceSeen = aPlaceSeen;
}

TTime CBuddycloudExtendedPlace::GetLastSeen() {
	return iLastSeen;
}

void CBuddycloudExtendedPlace::SetLastSeen(TTime aTime) {
	if(iPlaceSeen == EPlaceHere) {
		// Update Visit Seconds
		TTimeIntervalSeconds iInterval;

		if(aTime.SecondsFrom(iLastSeen, iInterval) == KErrNone) {
			iTotalSeconds += iInterval.Int();
			iVisitSeconds += iInterval.Int();
		}
	}

	iLastSeen = aTime;
}

TInt CBuddycloudExtendedPlace::GetVisits() {
	return iVisits;
}

void CBuddycloudExtendedPlace::SetVisits(TInt aVisits) {
	iVisits = aVisits;
}

TInt CBuddycloudExtendedPlace::GetVisitSeconds() {
	return iVisitSeconds;
}

void CBuddycloudExtendedPlace::SetVisitSeconds(TInt aVisitSeconds) {
	iVisitSeconds = aVisitSeconds;
}

TInt CBuddycloudExtendedPlace::GetTotalSeconds() {
	return iTotalSeconds;
}

void CBuddycloudExtendedPlace::SetTotalSeconds(TInt aTotalSeconds) {
	iTotalSeconds = aTotalSeconds;
}

TInt CBuddycloudExtendedPlace::GetPopulation() {
	return iPopulation;
}

void CBuddycloudExtendedPlace::SetPopulation(TInt aPopulation) {
	iPopulation = aPopulation;
}

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlaceStore
--
----------------------------------------------------------------------------
*/

CBuddycloudPlaceStore* CBuddycloudPlaceStore::NewL() {
	CBuddycloudPlaceStore* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudPlaceStore* CBuddycloudPlaceStore::NewLC() {
	CBuddycloudPlaceStore* self = new (ELeave) CBuddycloudPlaceStore();
	CleanupStack::PushL(self);
	return self;
}

CBuddycloudPlaceStore::~CBuddycloudPlaceStore() {
	for(TInt i = 0;i < iPlaceStore.Count();i++) {
		delete iPlaceStore[i];
	}

	iPlaceStore.Close();
}

CBuddycloudPlaceStore::CBuddycloudPlaceStore() {
	iEditedPlaceId = KErrNotFound;
}

CBuddycloudExtendedPlace* CBuddycloudPlaceStore::GetPlaceByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iPlaceStore.Count()) {
		return iPlaceStore[aIndex];
	}

	return NULL;
}

CBuddycloudExtendedPlace* CBuddycloudPlaceStore::GetPlaceById(TInt aPlaceId) {
	for(TInt i = 0;i < iPlaceStore.Count();i++) {
		if(iPlaceStore[i]->GetPlaceId() == aPlaceId) {
			return iPlaceStore[i];
		}
	}

	return NULL;
}

TInt CBuddycloudPlaceStore::GetIndexById(TInt aPlaceId) {
	for(TInt i = 0;i < iPlaceStore.Count();i++) {
		if(iPlaceStore[i]->GetPlaceId() == aPlaceId) {
			return i;
		}
	}

	return 0;
}

TInt CBuddycloudPlaceStore::CountPlaces() {
	return iPlaceStore.Count();
}

void CBuddycloudPlaceStore::AddPlace(CBuddycloudExtendedPlace* aPlace) {
	iPlaceStore.Append(aPlace);
}

CBuddycloudExtendedPlace* CBuddycloudPlaceStore::GetEditedPlace() {
	return GetPlaceById(iEditedPlaceId);
}

void CBuddycloudPlaceStore::SetEditedPlace(TInt aPlaceId) {
	iEditedPlaceId = aPlaceId;
}

void CBuddycloudPlaceStore::MovePlaceById(TInt aPlaceId, TInt aPosition) {
	if(aPosition <= iPlaceStore.Count()) {
		for(TInt i = 0; i < iPlaceStore.Count(); i++) {
			if(iPlaceStore[i]->GetPlaceId() == aPlaceId) {
				if(i != aPosition) {
					CBuddycloudExtendedPlace* aPlace = iPlaceStore[i];
	
					iPlaceStore.Remove(i);
					iPlaceStore.Insert(aPlace, aPosition);
				}
				
				break;
			}
		}
	}
}

void CBuddycloudPlaceStore::RemovePlaceByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iPlaceStore.Count()) {
		iPlaceStore.Remove(aIndex);
	}
}

void CBuddycloudPlaceStore::RemovePlaceById(TInt aPlaceId) {
	for(TInt i = 0; i < iPlaceStore.Count(); i++) {
		if(iPlaceStore[i]->GetPlaceId() == aPlaceId) {
			iPlaceStore.Remove(i);
			
			break;
		}
	}
}

void CBuddycloudPlaceStore::DeletePlaceByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iPlaceStore.Count()) {
		delete iPlaceStore[aIndex];
		iPlaceStore.Remove(aIndex);
	}
}

void CBuddycloudPlaceStore::DeletePlaceById(TInt aPlaceId) {
	if(iEditedPlaceId == aPlaceId) {
		iEditedPlaceId = KErrNotFound;
	}
	
	for(TInt i = 0; i < iPlaceStore.Count(); i++) {
		if(iPlaceStore[i]->GetPlaceId() == aPlaceId) {
			delete iPlaceStore[i];
			iPlaceStore.Remove(i);
			
			break;
		}
	}
}
