/*
============================================================================
 Name        : BuddycloudCommunitiesView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Communities View
============================================================================
*/

#ifndef BUDDYCLOUDCOMMUNITIESVIEW_H_
#define BUDDYCLOUDCOMMUNITIESVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudCommunitiesContainer;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudCommunitiesView
--
----------------------------------------------------------------------------
*/

class CBuddycloudCommunitiesView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudCommunitiesView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudCommunitiesContainer* iContainer;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif /*BUDDYCLOUDCOMMUNITIESVIEW_H_*/

// End of File
