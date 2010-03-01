/*
============================================================================
 Name        : 	BuddycloudNearby.cpp
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Storage of Buddycloud nearby data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "BuddycloudNearby.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudNearbyPlace
--
----------------------------------------------------------------------------
*/

CBuddycloudNearbyPlace::CBuddycloudNearbyPlace() {
	iPlaceId = 0;

	iPlaceName = HBufC::NewL(0);
}

CBuddycloudNearbyPlace::~CBuddycloudNearbyPlace() {
	if(iPlaceName)
		delete iPlaceName;
}

TInt CBuddycloudNearbyPlace::GetPlaceId() {
	return iPlaceId;
}

void CBuddycloudNearbyPlace::SetPlaceId(TInt aId) {
	iPlaceId = aId;
}

TDesC& CBuddycloudNearbyPlace::GetPlaceName() {
	return *iPlaceName;
}

void CBuddycloudNearbyPlace::SetPlaceNameL(TDesC& aName) {
	if(iPlaceName)
		delete iPlaceName;

	iPlaceName = aName.AllocL();
}

// End of File
