/*
============================================================================
 Name        : BuddycloudNotificationsSettingsList.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Settings List
============================================================================
*/

#ifndef BUDDYCLOUDNOTIFICATIONSSETTINGSLIST_H_
#define BUDDYCLOUDNOTIFICATIONSSETTINGSLIST_H_

// INCLUDES
#include <aknnavi.h>
#include <coecntrl.h>
#include <aknsettingitemlist.h>
#include "BuddycloudNotificationsSettingsList.h"
#include "BuddycloudLogic.h"

// CLASS DECLARATION
class CBuddycloudNotificationsSettingsList : public CAknSettingItemList {
	public: // Constructors and destructor
		CBuddycloudNotificationsSettingsList(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect);
		~CBuddycloudNotificationsSettingsList();
		
	public:
		void EditCurrentItemL();
		
	private: // From CCoeControl
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

	public: // From CAknSettingItemList
		void EditItemL(TInt aIndex, TBool aCalledFromMenu);
		
	private:
		void ScanDirectoryL(const TDesC& aDirectory, CDesCArray* aFileArray, RPointerArray<HBufC>& aDirArray);
		void SelectCustomToneL(TDes& aFile);

	private:
		CAknSettingItem* CreateSettingItemL (TInt aSettingId);

	private: // Data	
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
        
		CBuddycloudLogic* iBuddycloudLogic;
		
		TInt iPrivateMessageTone;
		TInt iChannelPostTone;
		TInt iDirectReplyTone;
};

#endif

// End of File
