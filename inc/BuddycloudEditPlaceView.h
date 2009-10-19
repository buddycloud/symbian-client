/*
============================================================================
 Name        : BuddycloudEditPlaceView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Edit Place View
============================================================================
*/

#ifndef BUDDYCLOUDEDITPLACEVIEW_H
#define BUDDYCLOUDEDITPLACEVIEW_H

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudEditPlaceList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudEditPlaceView
--
----------------------------------------------------------------------------
*/

class CBuddycloudEditPlaceView : public CAknView, MCoeControlObserver {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudEditPlaceView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);
		void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudEditPlaceList* iList;

		CBuddycloudLogic* iBuddycloudLogic;

		TVwsViewId iPrevViewId;
		TUid iPrevViewMessageId;
};

#endif

// End of File
