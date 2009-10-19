/*
============================================================================
 Name        : BuddycloudBeaconSettingsView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Settings View
============================================================================
*/

// INCLUDE FILES
#include <akntitle.h>
#include <aknviewappui.h>
#include <barsread.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudBeaconSettingsView.h"
#include "BuddycloudBeaconSettingsList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudBeaconSettingsView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_SETTINGS_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudBeaconSettingsView::~CBuddycloudBeaconSettingsView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudBeaconSettingsView::Id() const {
	return KBeaconSettingsViewId;
}

void CBuddycloudBeaconSettingsView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {	
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
	}
}

void CBuddycloudBeaconSettingsView::HandleCommandL(TInt aCommand) {
	if(aCommand == EAknSoftkeyDone) {
		AppUi()->ActivateLocalViewL(KFollowingViewId);
	}
	else if(aCommand == EMenuHelpCommand) {
		AppUi()->HandleCommandL(aCommand);
	}
	else if(aCommand == EMenuEditItemCommand) {
		if(iList) {
			iList->EditCurrentItemL();
		}
	}
}

void CBuddycloudBeaconSettingsView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_BEACONSETTINGS_TITLE);
		aTitlePane->SetTextL(*aTitle);
		CleanupStack::PopAndDestroy(); // aTitle

		iList = new (ELeave) CBuddycloudBeaconSettingsList(iBuddycloudLogic);
		iList->ConstructL(ClientRect());
		iList->SetMopParent(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_BEACON_SETTINGS_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL();
	}
}

void CBuddycloudBeaconSettingsView::DoDeactivate() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

