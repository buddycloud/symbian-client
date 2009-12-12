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
#include "BuddycloudList.h"
#include "GeolocData.h"

const TInt KEditingNewPlaceId = -9999;

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

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

class CBuddycloudPlace : public CBuddycloudListItem {
	public:
		static CBuddycloudPlace* NewL();
		static CBuddycloudPlace* NewLC();
		~CBuddycloudPlace();
		
	protected:
		CBuddycloudPlace();
		void ConstructL();

	public: 
		TBool Shared();
		void SetShared(TBool aPublic);
		
		TInt GetRevision();
		void SetRevision(TInt aRevision);
		
	public:
		CGeolocData* GetGeoloc();
		void CopyGeolocL(CGeolocData* aGeoloc);
		
		void UpdateFromGeolocL();
		
	protected:
		TBool iPublic;
		TInt iRevision;
	
		CGeolocData* iGeoloc;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudExtendedPlace
--
----------------------------------------------------------------------------
*/

class CBuddycloudExtendedPlace : public CBuddycloudPlace {
	public:
		static CBuddycloudExtendedPlace* NewL();
		static CBuddycloudExtendedPlace* NewLC();

	private:
		CBuddycloudExtendedPlace();
	
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

class CBuddycloudPlaceStore : public CBuddycloudListStore {
	public:
		static CBuddycloudPlaceStore* NewL();
		static CBuddycloudPlaceStore* NewLC();
	
	private:
		CBuddycloudPlaceStore();
		
	public: // Editing place
		CBuddycloudListItem* GetEditedPlace();
		void SetEditedPlace(TInt aPlaceId);
		
	public: // From CBuddycloudListStore
		void DeleteItemById(TInt aItemId);

	protected:
		TInt iEditedPlaceId;
};

#endif /*BUDDYCLOUDPLACES_H_*/
