/*
============================================================================
 Name        : BuddycloudCommunitiesView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Communities View
============================================================================
*/

// INCLUDE FILES
#include <akntitle.h>
#include <aknviewappui.h>
#include <barsread.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BrowserLauncher.h"
#include "BuddycloudCommunitiesView.h"
#include "BuddycloudCommunitiesContainer.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudCommunitiesView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_SETTINGS_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudCommunitiesView::~CBuddycloudCommunitiesView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudCommunitiesView::Id() const {
	return KCommunitiesViewId;
}

void CBuddycloudCommunitiesView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudCommunitiesView::HandleCommandL(TInt aCommand) {
	if(aCommand == EAknSoftkeyDone) {
		AppUi()->ActivateLocalViewL(KFollowingViewId);
	}
	else if(aCommand == EMenuHelpCommand) {
		AppUi()->HandleCommandL(aCommand);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

void CBuddycloudCommunitiesView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iContainer) {
		CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(StatusPane()->ControlL(TUid::Uid( EEikStatusPaneUidTitle)));
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_COMMUNITIES_TITLE);
		aTitlePane->SetTextL(*aTitle);
		CleanupStack::PopAndDestroy(); // aTitle
		
		iContainer = new (ELeave) CBuddycloudCommunitiesContainer(iBuddycloudLogic);
		iContainer->ConstructL(ClientRect());
		iContainer->SetMopParent(this);
 		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudCommunitiesView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

// End of File

