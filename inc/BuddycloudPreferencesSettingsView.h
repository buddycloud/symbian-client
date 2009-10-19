/*
============================================================================
 Name        : BuddycloudPreferencesSettingsView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Settings View
============================================================================
*/

#ifndef BUDDYCLOUDPREFERENCESSETTINGSVIEW_H
#define BUDDYCLOUDPREFERENCESSETTINGSVIEW_H

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudPreferencesSettingsList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPreferencesSettingsView
--
----------------------------------------------------------------------------
*/

class CBuddycloudPreferencesSettingsView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudPreferencesSettingsView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudPreferencesSettingsList* iList;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
