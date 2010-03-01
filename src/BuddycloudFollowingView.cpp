/*
============================================================================
 Name        : BuddycloudFollowingView.cpp
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
 Description : Declares Following View
============================================================================
*/

// INCLUDE FILES
#include <apgtask.h>
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudFollowingView.h"
#include "BuddycloudFollowingContainer.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudFollowingView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_FOLLOWING_VIEW);
	
#ifdef __SERIES60_40__
	if(Toolbar()) {
		Toolbar()->SetToolbarObserver(this);
	}
#endif

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudFollowingView::~CBuddycloudFollowingView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack( *this, iContainer );
		delete iContainer;
	}
}

TUid CBuddycloudFollowingView::Id() const {
	return KFollowingViewId;
}

void CBuddycloudFollowingView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);		
	}
}

void CBuddycloudFollowingView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuExitCommand || aCommand == EMenuHelpCommand || aCommand == EMenuUpdateCommand || 
			aCommand == EMenuAboutCommand || aCommand == EMenuUserSettingsCommand || 
			aCommand == EMenuCommunitySettingsCommand || aCommand == EMenuClearConnectionCommand) {
		
		AppUi()->HandleCommandL(aCommand);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}
		
#ifdef __SERIES60_40__
void CBuddycloudFollowingView::OfferToolbarEventL(TInt aCommandId) {
	if(iContainer) {
		iContainer->HandleCommandL(aCommandId);
	}
}
#endif

void CBuddycloudFollowingView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/) {
	if (!iContainer) {
		iContainer = new (ELeave) CBuddycloudFollowingContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aCustomMessageId.iUid);
		iContainer->SetMopParent(this);
		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudFollowingView::DoDeactivate() {
	if(iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

CEikMenuBar* CBuddycloudFollowingView::ViewMenuBar() {
	return MenuBar();
}

CEikStatusPane* CBuddycloudFollowingView::ViewStatusPane() {
	return StatusPane();
}

CEikButtonGroupContainer* CBuddycloudFollowingView::ViewCba() {
	return Cba();
}

#ifdef __SERIES60_40__
CAknToolbar* CBuddycloudFollowingView::ViewToolbar() {
	return Toolbar();
}
#endif

// End of File

