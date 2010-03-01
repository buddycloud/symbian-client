/*
============================================================================
 Name        : BuddycloudAccountSettingsView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Settings View
============================================================================
*/

#ifndef BUDDYCLOUDACCOUNTSETTINGSVIEW_H
#define BUDDYCLOUDACCOUNTSETTINGSVIEW_H

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudAccountSettingsList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudAccountSettingsView
--
----------------------------------------------------------------------------
*/

class CBuddycloudAccountSettingsView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudAccountSettingsView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudAccountSettingsList* iList;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
