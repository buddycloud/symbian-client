/*
============================================================================
 Name        : BuddycloudFollowingContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Following Container
============================================================================
*/

#ifndef BUDDYCLOUDFOLLOWINGCONTAINER_H
#define BUDDYCLOUDFOLLOWINGCONTAINER_H

// INCLUDES
#include <aknnavi.h>
#include <akntabgrp.h>
#include <akntabobserver.h>
#include <eikedwin.h>
#include "MessagingParticipants.h"
#include "BuddycloudListComponent.h"
#include "BuddycloudFollowing.h"

#ifdef __SERIES60_40__
#include <akntoolbar.h>
#endif

/*
----------------------------------------------------------------------------
--
-- CBuddycloudFollowingContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudFollowingContainer : public CBuddycloudListComponent, MBuddycloudLogicStatusObserver, MAknTabObserver {

	public: // Constructors and destructor
		CBuddycloudFollowingContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TInt aItem);
        ~CBuddycloudFollowingContainer();

	private:
		void PrecacheImagesL();
		void ResizeCachedImagesL();
		
	private:
		void ConfigureEdwinL();
		void DisplayEdwin(TBool aShowEdwin);
		void ConfigureCbaAndMenuL();

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From MBuddycloudLogicStatusObserver
		void JumpToItem(TInt aItemId);

	private:
		TBool IsFilteredItem(TInt aIndex);
	
	private: // From CBuddycloudListComponent
		void RenderWrappedText(TInt aIndex);
		TInt CalculateItemSize(TInt aIndex);
		void RenderListItems();
		void RepositionItems(TBool aSnapToItem);
		void HandleItemSelection(TInt aItemId);

#ifdef __SERIES60_40__
	public:
		void DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar);
#endif
		
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
		
		TInt iNewMessagesOffset;
		
		// Search Field
		CEikEdwin* iSearchEdwin;
		TBool iSearchVisible;
		TInt iSearchLength;

		// Images
		TSize iArrow1Size;
		CFbsBitmap* iArrow1Bitmap;
		CFbsBitmap* iArrow1Mask;	
 		
		TSize iArrow2Size;
		CFbsBitmap* iArrow2Bitmap;
		CFbsBitmap* iArrow2Mask;	

		CBuddycloudFollowingStore* iItemStore;
};

#endif

// End of File
