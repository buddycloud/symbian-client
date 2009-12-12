/*
============================================================================
 Name        : 	GeolocData.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Geoloc data store
 History     : 	1.0
============================================================================
*/

#ifndef GEOLOCDATA_H_
#define GEOLOCDATA_H_

// INCLUDES
#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- Constants & Enumerations
--
----------------------------------------------------------------------------
*/

const TInt KMaxStringItems = 9;
const TInt KMaxRealItems = 3;

enum TGeolocString {
	EGeolocArea, EGeolocCountry, EGeolocDescription, EGeolocLocality, 
	EGeolocPostalcode, EGeolocRegion, EGeolocStreet, EGeolocText, EGeolocUri
};

enum TGeolocReal {
	EGeolocAccuracy, EGeolocLatitude, EGeolocLongitude
};

/*
----------------------------------------------------------------------------
--
-- CGeolocData
--
----------------------------------------------------------------------------
*/

class CGeolocData : public CBase {
	public:
		static CGeolocData* NewL();
		static CGeolocData* NewLC();
		~CGeolocData();

	private:
		void ConstructL();
		
	public:
		TDesC& GetString(TGeolocString aId);
		void SetStringL(TGeolocString aId, const TDesC& aString);
		
		TReal GetReal(TGeolocReal aId);
		void SetRealL(TGeolocReal aId, TReal aReal);

	protected:
		RPointerArray<HBufC> iStringData;
		RArray<TReal> iRealData;
		
		TPtrC iNullString;
};

#endif /* GEOLOCDATA_H_ */
