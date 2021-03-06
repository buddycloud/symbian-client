/*
============================================================================
 Name        : BuddycloudFollowingContainer.h
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
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

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From MBuddycloudLogicStatusObserver
		void JumpToItem(TInt aItemId);
	
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
		
		// Edwin
		CEikEdwin* iEdwin;
		TBool iEdwinVisible;
		TInt iEdwinLength;

		// Images
		TSize iArrow1Size;
		CFbsBitmap* iArrow1Bitmap;
		CFbsBitmap* iArrow1Mask;	
 		
		TSize iArrow2Size;
		CFbsBitmap* iArrow2Bitmap;
		CFbsBitmap* iArrow2Mask;	

		CBuddycloudFollowingStore* iItemStore;
		
		TInt iViewEntryItem;
};

#endif

// End of File
