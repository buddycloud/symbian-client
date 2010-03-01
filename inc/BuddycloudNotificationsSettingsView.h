/*
============================================================================
 Name        : BuddycloudNotificationsSettingsView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Settings View
============================================================================
*/

#ifndef BUDDYCLOUDNOTIFICATIONSSETTINGSVIEW_H_
#define BUDDYCLOUDNOTIFICATIONSSETTINGSVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudNotificationsSettingsList;

/*
----------------------------------------------------------------------------
--
-- CBuddycloudNotificationsSettingsView
--
----------------------------------------------------------------------------
*/

class CBuddycloudNotificationsSettingsView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudNotificationsSettingsView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudNotificationsSettingsList* iList;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
