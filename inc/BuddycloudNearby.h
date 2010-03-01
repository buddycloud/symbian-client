/*
============================================================================
 Name        : 	BuddycloudNearby.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Storage of Buddycloud nearby data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDNEARBY_H_
#define BUDDYCLOUDNEARBY_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- CBuddycloudNearbyPlace
--
----------------------------------------------------------------------------
*/

class CBuddycloudNearbyPlace : public CBase {
	public:
		CBuddycloudNearbyPlace();
		~CBuddycloudNearbyPlace();

	public:
		TInt GetPlaceId();
		void SetPlaceId(TInt aId);

		TDesC& GetPlaceName();
		void SetPlaceNameL(TDesC& aName);

	protected:
		TInt iPlaceId;
		HBufC* iPlaceName;
};

#endif /*BUDDYCLOUDNEARBY_H_*/
