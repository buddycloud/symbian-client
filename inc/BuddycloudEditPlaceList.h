/*
============================================================================
 Name        : BuddycloudEditPlaceList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Edit Place List
============================================================================
*/

#ifndef BUDDYCLOUDEDITPLACELIST_H
#define BUDDYCLOUDEDITPLACELIST_H

// INCLUDES
#include <coecntrl.h>
#include <aknsettingitemlist.h>
#include "BuddycloudEditPlaceList.h"
#include "BuddycloudLogic.h"

// CLASS DECLARATION
class CBuddycloudEditPlaceList : public CAknSettingItemList, MBuddycloudLogicNotificationObserver {
	public: // Constructors and destructor
		CBuddycloudEditPlaceList();
		void ConstructL(const TRect& aRect, CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudEditPlaceList();
		
	private:
		void SetTitleL(TInt aResourceId);
		void GetEditedPlace();

	public:
		void EditCurrentItemL();
		void LoadL();
		void SaveL();

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From CAknSettingItemList
		void EditItemL(TInt aIndex, TBool aCalledFromMenu);

	private:
		CAknSettingItem* CreateSettingItemL (TInt aSettingId);

	private: // Data
		CBuddycloudLogic* iBuddycloudLogic;

		CBuddycloudExtendedPlace* iEditingPlace;
		
		TInt iTitleResourceId;
		
		TBuf<256> iName;
		TBuf<256> iStreet;
		TBuf<256> iArea;
		TBuf<256> iCity;
		TBuf<256> iPostcode;
		TBuf<256> iRegion;
		TBuf<256> iCountry;
		TBool iShared;
};

#endif

// End of File
