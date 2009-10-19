/*
============================================================================
 Name        : BuddycloudChannelsView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Channels View
============================================================================
*/

// INCLUDE FILES
#include <apgtask.h>
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"
#include "BuddycloudChannelsView.h"
#include "BuddycloudChannelsContainer.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudChannelsView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_EXPLORER_VIEW);
	
	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudChannelsView::~CBuddycloudChannelsView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudChannelsView::Id() const {
	return KChannelsViewId;
}

void CBuddycloudChannelsView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudChannelsView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuExitCommand || aCommand == EMenuHelpCommand || aCommand == EMenuUpdateCommand || 
			aCommand == EMenuAboutCommand || aCommand == EMenuUserSettingsCommand || 
			aCommand == EMenuCommunitySettingsCommand || aCommand == EMenuClearConnectionCommand) {
		
		AppUi()->HandleCommandL(aCommand);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

void CBuddycloudChannelsView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iContainer) {
		iContainer = new (ELeave) CBuddycloudChannelsContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect());
		iContainer->SetMopParent(this);
 		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudChannelsView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

CEikMenuBar* CBuddycloudChannelsView::ViewMenuBar() {
	return MenuBar();
}

CEikStatusPane* CBuddycloudChannelsView::ViewStatusPane() {
	return StatusPane();
}

CEikButtonGroupContainer* CBuddycloudChannelsView::ViewCba() {
	return Cba();
}

#ifdef __SERIES60_40__
CAknToolbar* CBuddycloudChannelsView::ViewToolbar() {
	return Toolbar();
}
#endif

// End of File

