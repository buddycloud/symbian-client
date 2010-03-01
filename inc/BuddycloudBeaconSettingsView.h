/*
============================================================================
 Name        : BuddycloudBeaconSettingsView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Settings View
============================================================================
*/

#ifndef BUDDYCLOUDBEACONSETTINGSVIEW_H
#define BUDDYCLOUDBEACONSETTINGSVIEW_H

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudBeaconSettingsList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudBeaconSettingsView
--
----------------------------------------------------------------------------
*/

class CBuddycloudBeaconSettingsView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudBeaconSettingsView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudBeaconSettingsList* iList;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
