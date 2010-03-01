/*
============================================================================
 Name        : BuddycloudNotificationsSettingsView.cpp
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
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
#include "BuddycloudNotificationsSettingsView.h"
#include "BuddycloudNotificationsSettingsList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudNotificationsSettingsView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_SETTINGS_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudNotificationsSettingsView::~CBuddycloudNotificationsSettingsView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudNotificationsSettingsView::Id() const {
	return KNotificationsSettingsViewId;
}

void CBuddycloudNotificationsSettingsView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {	
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
	}
}

void CBuddycloudNotificationsSettingsView::HandleCommandL(TInt aCommand) {
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

void CBuddycloudNotificationsSettingsView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTIFICATIONSSETTINGS_TITLE);
		aTitlePane->SetTextL(*aTitle);
		CleanupStack::PopAndDestroy(); // aTitle

		iList = new (ELeave) CBuddycloudNotificationsSettingsList(iBuddycloudLogic);
		iList->ConstructL(ClientRect());
		iList->SetMopParent(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_NOTIFICATIONS_SETTINGS_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL();
	}
}

void CBuddycloudNotificationsSettingsView::DoDeactivate() {
	if (iList) {		
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

