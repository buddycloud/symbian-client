/*
============================================================================
 Name        : BuddycloudPreferencesSettingsList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
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
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;

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
