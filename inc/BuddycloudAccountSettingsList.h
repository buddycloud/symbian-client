/*
============================================================================
 Name        : BuddycloudAccountSettingsList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Settings List
============================================================================
*/

#ifndef BUDDYCLOUDACCOUNTSETTINGSLIST_H
#define BUDDYCLOUDACCOUNTSETTINGSLIST_H

// INCLUDES
#include <aknnavi.h>
#include <coecntrl.h>
#include <aknsettingitemlist.h>
#include "BuddycloudAccountSettingsList.h"
#include "BuddycloudLogic.h"

// CLASS DECLARATION
class CBuddycloudAccountSettingsList : public CAknSettingItemList {
	public: // Constructors and destructor
		CBuddycloudAccountSettingsList(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect);
		~CBuddycloudAccountSettingsList();
		
	public:
		void ActivateL(TInt aSelectItem);
		void EditCurrentItemL();
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;

	private: // From CCoeControl
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

	public: // From CAknSettingItemList
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void EditItemL(TInt aIndex, TBool aCalledFromMenu);

	private:
		CAknSettingItem* CreateSettingItemL (TInt aSettingId);

	private: // Data	
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
        
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
