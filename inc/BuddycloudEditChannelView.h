/*
============================================================================
 Name        : BuddycloudEditChannelView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2009
 Description : Declares Edit Channel View
============================================================================
*/

#ifndef BUDDYCLOUDEDITCHANNELVIEW_H_
#define BUDDYCLOUDEDITCHANNELVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudEditChannelList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudEditChannelView
--
----------------------------------------------------------------------------
*/

class CBuddycloudEditChannelView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudEditChannelView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudEditChannelList* iList;

		CBuddycloudLogic* iBuddycloudLogic;

		TVwsViewId iPrevViewId;
		TUid iPrevViewMessageId;
};

#endif

// End of File
