/*
============================================================================
 Name        : BuddycloudPlacesView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Places View
============================================================================
*/

// INCLUDE FILES
#include <apgtask.h>
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudPlacesView.h"
#include "BuddycloudPlacesContainer.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudPlacesView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_PLACES_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudPlacesView::~CBuddycloudPlacesView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudPlacesView::Id() const {
	return KPlacesViewId;
}

void CBuddycloudPlacesView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudPlacesView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuExitCommand || aCommand == EMenuHelpCommand || aCommand == EMenuUpdateCommand || 
			aCommand == EMenuAboutCommand || aCommand == EMenuUserSettingsCommand || 
			aCommand == EMenuCommunitySettingsCommand || aCommand == EMenuClearConnectionCommand) {
		
		AppUi()->HandleCommandL(aCommand);
	}
	else if(aCommand == EAknSoftkeyBack) {
		AppUi()->ActivateLocalViewL(KFollowingViewId);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

void CBuddycloudPlacesView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/) {
	if (!iContainer) {
		iContainer = new (ELeave) CBuddycloudPlacesContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aCustomMessageId.iUid);
		iContainer->SetMopParent(this);
 		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudPlacesView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

CEikMenuBar* CBuddycloudPlacesView::ViewMenuBar() {
	return MenuBar();
}

CEikStatusPane* CBuddycloudPlacesView::ViewStatusPane() {
	return StatusPane();
}

CEikButtonGroupContainer* CBuddycloudPlacesView::ViewCba() {
	return Cba();
}

#ifdef __SERIES60_40__
CAknToolbar* CBuddycloudPlacesView::ViewToolbar() {
	return Toolbar();
}
#endif

// End of File

