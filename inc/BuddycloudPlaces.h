/*
============================================================================
 Name        : 	BuddycloudPlaces.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Storage of Buddycloud place data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDPLACES_H_
#define BUDDYCLOUDPLACES_H_

#include <e32base.h>

const TInt KEditingNewPlaceId = -9999;

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TBuddycloudPlaceType {
	EPlacePrevious, EPlaceCurrent, EPlaceNext
};

enum TBuddycloudPlaceSeen {
	EPlaceNotSeen, EPlaceHere, EPlaceRecent, EPlaceFlux
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlace
--
----------------------------------------------------------------------------
*/

class CBuddycloudBasicPlace : public CBase {
	public:
		static CBuddycloudBasicPlace* NewL();
		static CBuddycloudBasicPlace* NewLC();
		~CBuddycloudBasicPlace();
		
	protected:
		CBuddycloudBasicPlace();
		void ConstructL();

	public: 
		TInt GetPlaceId();
		void SetPlaceId(TInt aPlaceId);

		TBool GetShared();
		void SetShared(TBool aPublic);
		
		TInt GetRevision();
		void SetRevision(TInt aRevision);

	public: // Descriptive Values
		TDesC& GetPlaceName();
		void SetPlaceNameL(TDesC& aPlaceName);

		TDesC& GetPlaceStreet();
		void SetPlaceStreetL(TDesC& aPlaceStreet);

		TDesC& GetPlaceArea();
		void SetPlaceAreaL(TDesC& aPlaceArea);

		TDesC& GetPlaceCity();
		void SetPlaceCityL(TDesC& aPlaceCity);

		TDesC& GetPlacePostcode();
		void SetPlacePostcodeL(TDesC& aPlacePostcode);

		TDesC& GetPlaceRegion();
		void SetPlaceRegionL(TDesC& aPlaceRegion);

		TDesC& GetPlaceCountry();
		void SetPlaceCountryL(TDesC& aPlaceCountry);

	public: // Position
		TReal GetPlaceLatitude();
		void SetPlaceLatitude(TReal aLatitude);

		TReal GetPlaceLongitude();
		void SetPlaceLongitude(TReal aLongitude);
		
	public: // Copy
		void CopyDetailsL(CBuddycloudBasicPlace* aPlace);
	
	protected:
		TInt iPlaceId;
		TBool iPublic;
		TInt iRevision;
		
		HBufC* iPlaceName;
		HBufC* iPlaceStreet;
		HBufC* iPlaceArea;
		HBufC* iPlaceCity;
		HBufC* iPlacePostcode;
		HBufC* iPlaceRegion;
		HBufC* iPlaceCountry;

		TReal iLatitude;
		TReal iLongitude;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudExtendedPlace
--
----------------------------------------------------------------------------
*/

class CBuddycloudExtendedPlace : public CBuddycloudBasicPlace {
	public:
		static CBuddycloudExtendedPlace* NewL();
		static CBuddycloudExtendedPlace* NewLC();
		~CBuddycloudExtendedPlace();

	private:
		CBuddycloudExtendedPlace();
		void ConstructL();

	public:
		TDesC& GetDescription();
		void SetDescriptionL(TDesC& aDescription);
		
		TDesC& GetWebsite();
		void SetWebsiteL(TDesC& aWebsite);
		
		TDesC& GetWiki();
		void SetWikiL(TDesC& aWiki);
	
	public:
		TBuddycloudPlaceSeen GetPlaceSeen();
		void SetPlaceSeen(TBuddycloudPlaceSeen aPlaceSeen);

	public: // Time
		TTime GetLastSeen();
		void SetLastSeen(TTime aTime);

		TInt GetVisits();
		void SetVisits(TInt aVisits);

		TInt GetVisitSeconds();
		void SetVisitSeconds(TInt aVisitSeconds);

		TInt GetTotalSeconds();
		void SetTotalSeconds(TInt aTotalSeconds);
		
	public: // Int switches		
		TInt GetPopulation();
		void SetPopulation(TInt aPopulation);

	protected:
		HBufC* iDescription;
		HBufC* iWebsite;
		HBufC* iWiki;
		
		TBuddycloudPlaceSeen iPlaceSeen;
		TTime iLastSeen;

		TInt iVisits;
		TInt iVisitSeconds;
		TInt iTotalSeconds;

		TInt iPopulation;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlaceStore
--
----------------------------------------------------------------------------
*/

class CBuddycloudPlaceStore : public CBase {
	public:
		static CBuddycloudPlaceStore* NewL();
		static CBuddycloudPlaceStore* NewLC();
		~CBuddycloudPlaceStore();

	private:
		CBuddycloudPlaceStore();

	public: // Get places & indexes
		CBuddycloudExtendedPlace* GetPlaceByIndex(TInt aIndex);
		CBuddycloudExtendedPlace* GetPlaceById(TInt aPlaceId);
		TInt GetIndexById(TInt aPlaceId);

	public:
		TInt CountPlaces();
		void AddPlace(CBuddycloudExtendedPlace* aPlace);
		
	public: // Editing place
		CBuddycloudExtendedPlace* GetEditedPlace();
		void SetEditedPlace(TInt aPlaceId);

	public:
		void MovePlaceById(TInt aPlaceId, TInt aPosition);
		
	public: // Removing (not deleted)
		void RemovePlaceByIndex(TInt aIndex);
		void RemovePlaceById(TInt aPlaceId);

	public: // Deleting
		void DeletePlaceByIndex(TInt aIndex);
		void DeletePlaceById(TInt aPlaceId);

	protected:
		RPointerArray<CBuddycloudExtendedPlace> iPlaceStore;
		
		TInt iEditedPlaceId;
};

#endif /*BUDDYCLOUDPLACES_H_*/
