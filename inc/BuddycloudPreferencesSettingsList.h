/*
============================================================================
 Name        : BuddycloudPreferencesSettingsList.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Settings List
============================================================================
*/

#ifndef BUDDYCLOUDPREFERENCESSETTINGSLIST_H
#define BUDDYCLOUDPREFERENCESSETTINGSLIST_H

// INCLUDES
#include <aknnavi.h>
#include <coecntrl.h>
#include <aknsettingitemlist.h>
#include "BuddycloudPreferencesSettingsList.h"
#include "BuddycloudLogic.h"

// CLASS DECLARATION
class CBuddycloudPreferencesSettingsList : public CAknSettingItemList {
	public: // Constructors and destructor
		CBuddycloudPreferencesSettingsList(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect);
		~CBuddycloudPreferencesSettingsList();
		
	public:
		void EditCurrentItemL();
		void SaveL();

	private: // From CCoeControl
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

	public: // From CAknSettingItemList
		void EditItemL(TInt aIndex, TBool aCalledFromMenu);

	private:
		CAknSettingItem* CreateSettingItemL (TInt aSettingId);

	private: // Data	
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
        
		CBuddycloudLogic* iBuddycloudLogic;
		
		TInt iLanguageUsed;
};

#endif

// End of File
