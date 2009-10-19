/*
============================================================================
 Name        : BuddycloudPreferencesSettingsView.cpp
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
#include "BuddycloudPreferencesSettingsView.h"
#include "BuddycloudPreferencesSettingsList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudPreferencesSettingsView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_SETTINGS_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudPreferencesSettingsView::~CBuddycloudPreferencesSettingsView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudPreferencesSettingsView::Id() const {
	return KPreferencesSettingsViewId;
}

void CBuddycloudPreferencesSettingsView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {	
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
	}
}

void CBuddycloudPreferencesSettingsView::HandleCommandL(TInt aCommand) {
	if(aCommand == EAknSoftkeyDone) {
		if(iList) {
			iList->SaveL();
		}

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

void CBuddycloudPreferencesSettingsView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_PREFERENCESSETTINGS_TITLE);
		aTitlePane->SetTextL(*aTitle);
		CleanupStack::PopAndDestroy(); // aTitle

		iList = new (ELeave) CBuddycloudPreferencesSettingsList(iBuddycloudLogic);
		iList->ConstructL(ClientRect());
		iList->SetMopParent(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_PREFERENCES_SETTINGS_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL();
	}
}

void CBuddycloudPreferencesSettingsView::DoDeactivate() {
	if (iList) {		
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

