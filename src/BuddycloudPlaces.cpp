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
#include "TextUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlace
--
----------------------------------------------------------------------------
*/

CBuddycloudPlace* CBuddycloudPlace::NewL() {
	CBuddycloudPlace* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CBuddycloudPlace* CBuddycloudPlace::NewLC() {
	CBuddycloudPlace* self = new (ELeave) CBuddycloudPlace();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CBuddycloudPlace::~CBuddycloudPlace() {
	if(iGeoloc)
		delete iGeoloc;
}

CBuddycloudPlace::CBuddycloudPlace() {
	iPublic = false;
	iRevision = KErrNotFound;
}

void CBuddycloudPlace::ConstructL() {
	CBuddycloudListItem::ConstructL();
	
	iGeoloc = CGeolocData::NewL();
}

TBool CBuddycloudPlace::Shared() {
	return iPublic;
}

void CBuddycloudPlace::SetShared(TBool aPublic) {
	iPublic = aPublic;
}

TInt CBuddycloudPlace::GetRevision() {
	return iRevision;
}

void CBuddycloudPlace::SetRevision(TInt aRevision) {
	iRevision = aRevision;
}

CGeolocData* CBuddycloudPlace::GetGeoloc() {
	return iGeoloc;
}

void CBuddycloudPlace::CopyGeolocL(CGeolocData* aGeoloc) {
	// Copy geoloc
	for(TInt i = 0; i < KMaxStringItems; i++) {
		iGeoloc->SetStringL((TGeolocString)i, aGeoloc->GetString((TGeolocString)i));
	}
	
	for(TInt i = 0; i < KMaxRealItems; i++) {
		iGeoloc->SetRealL((TGeolocReal)i, aGeoloc->GetReal((TGeolocReal)i));
	}
	
	// Set id
	if(iGeoloc->GetString(EGeolocUri).Length() > 0) {
		TInt aResult = iGeoloc->GetString(EGeolocUri).LocateReverse('/');
		
		if(aResult != KErrNotFound) {
			TLex aLex(iGeoloc->GetString(EGeolocUri).Mid(aResult + 1));
			aLex.Val(iItemId);
		}
	}
}

void CBuddycloudPlace::UpdateFromGeolocL() {
	// Set title
	SetTitleL(iGeoloc->GetString(EGeolocText));
	
	// Set description
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	
	HBufC* aDescription = HBufC::NewLC(iGeoloc->GetString(EGeolocStreet).Length() + iGeoloc->GetString(EGeolocArea).Length() + 
			iGeoloc->GetString(EGeolocLocality).Length() + iGeoloc->GetString(EGeolocPostalcode).Length() + 
			iGeoloc->GetString(EGeolocRegion).Length() + iGeoloc->GetString(EGeolocCountry).Length() + 9);
	TPtr pDescription(aDescription->Des());
	
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocStreet), KNullDesC);
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocArea), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocLocality), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocPostalcode), _L(" "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocRegion), _L(", "));
	aTextUtilities->AppendToString(pDescription, iGeoloc->GetString(EGeolocCountry), _L(", "));
	
	SetDescriptionL(pDescription);
	CleanupStack::PopAndDestroy(2); // aDescription, aTextUtilities	
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

CBuddycloudExtendedPlace::CBuddycloudExtendedPlace() : CBuddycloudPlace() {	
	iPlaceSeen = EPlaceNotSeen;
	iLastSeen.UniversalTime();

	iVisits = 0;
	iVisitSeconds = 0;
	iTotalSeconds = 0;

	iPopulation = 0;
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
	self->ConstructL();
	return self;
}

CBuddycloudPlaceStore::CBuddycloudPlaceStore() : CBuddycloudListStore() {
	iEditedPlaceId = KErrNotFound;
}

CBuddycloudListItem* CBuddycloudPlaceStore::GetEditedPlace() {
	return GetItemById(iEditedPlaceId);
}

void CBuddycloudPlaceStore::SetEditedPlace(TInt aPlaceId) {
	iEditedPlaceId = aPlaceId;
}

void CBuddycloudPlaceStore::DeleteItemById(TInt aItemId) {
	if(iEditedPlaceId == aItemId) {
		iEditedPlaceId = KErrNotFound;
	}
	
	CBuddycloudListStore::DeleteItemById(aItemId);
}
