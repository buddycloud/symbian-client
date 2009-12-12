/*
============================================================================
 Name        : BuddycloudAccountSettingsView.cpp
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
#include "BuddycloudAccountSettingsView.h"
#include "BuddycloudAccountSettingsList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudAccountSettingsView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_SETTINGS_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudAccountSettingsView::~CBuddycloudAccountSettingsView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudAccountSettingsView::Id() const {
	return KAccountSettingsViewId;
}

void CBuddycloudAccountSettingsView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iList) {
		iList->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudAccountSettingsView::HandleCommandL(TInt aCommand) {
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

void CBuddycloudAccountSettingsView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(StatusPane()->ControlL(TUid::Uid( EEikStatusPaneUidTitle)));
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_ACCOUNTSETTINGS_TITLE);
		aTitlePane->SetTextL(*aTitle);
		CleanupStack::PopAndDestroy(); // aTitle

		iList = new (ELeave) CBuddycloudAccountSettingsList(iBuddycloudLogic);
		iList->ConstructL(ClientRect());
		iList->SetMopParent(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_ACCOUNT_SETTINGS_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL(aCustomMessageId.iUid);
	}
}

void CBuddycloudAccountSettingsView::DoDeactivate() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

