/*
============================================================================
 Name        : BuddycloudPlacesContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Places Container
============================================================================
*/

#ifndef BUDDYCLOUDPLACESCONTAINER_H_
#define BUDDYCLOUDPLACESCONTAINER_H_

// INCLUDES
#include <aknnavi.h>
#include <akntabgrp.h>
#include <akntabobserver.h>
#include "BuddycloudListComponent.h"
#include "BuddycloudPlaces.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlacesContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudPlacesContainer : public CBuddycloudListComponent, MAknTabObserver {
	public: // Constructors and destructor
		CBuddycloudPlacesContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TInt aPlaceId);
        ~CBuddycloudPlacesContainer();
		
	private:
		void ConfigureEdwinL();
		void DisplayEdwin(TBool aShowEdwin);

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	private: // From CBuddycloudListComponent
		void RenderWrappedText(TInt aIndex);
		TInt CalculateItemSize(TInt aIndex);
		void RenderListItems();
		void RepositionItems(TBool aSnapToItem);
		void HandleItemSelection(TInt aItemId);

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;
		
	private: // From CCoeControl
		void HandleResourceChange(TInt aType);
		void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
 		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
  		
	public: // From MAknTabObserver
		void TabChangedL(TInt aIndex);

	private: // Variables
		// Tabs
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
		CAknTabGroup* iTabGroup;
		
		// Strings
		HBufC* iLocalizedPlaceLastSeen;
		HBufC* iLocalizedPlaceVisits;
		HBufC* iLocalizedPlacePopulation;
		HBufC* iLocalizedLearningPlace;
		HBufC* iLocalizedUpdatingPlace;
		
		// Edwin
		CEikEdwin* iEdwin;
		TBool iEdwinVisible;
		TInt iEdwinLength;

		CBuddycloudPlaceStore* iPlaceStore;
};

#endif /*BUDDYCLOUDPLACESCONTAINER_H_*/
