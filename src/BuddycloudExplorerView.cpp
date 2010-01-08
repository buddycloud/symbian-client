/*
============================================================================
 Name        : BuddycloudExplorerView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Explorer View
============================================================================
*/

// INCLUDE FILES
#include <apgtask.h>
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudExplorer.h"
#include "BuddycloudExplorerView.h"
#include "BuddycloudExplorerContainer.h"
#include "ViewReference.h"

// ================= MEMBER FUNCTIONS =======================

void CBuddycloudExplorerView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_EXPLORER_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudExplorerView::~CBuddycloudExplorerView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudExplorerView::Id() const {
	return KExplorerViewId;
}

void CBuddycloudExplorerView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudExplorerView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuExitCommand || aCommand == EMenuHelpCommand || aCommand == EMenuUpdateCommand || 
			aCommand == EMenuAboutCommand || aCommand == EMenuUserSettingsCommand || 
			aCommand == EMenuCommunitySettingsCommand || aCommand == EMenuClearConnectionCommand) {
		
		AppUi()->HandleCommandL(aCommand);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

void CBuddycloudExplorerView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& aCustomMessage) {		
	if (!iContainer) {
		TViewReferenceBuf aViewReference;
		aViewReference.Copy(aCustomMessage);
		
		iContainer = new (ELeave) CBuddycloudExplorerContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aViewReference());
		iContainer->SetMopParent(this);
 		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudExplorerView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

CEikMenuBar* CBuddycloudExplorerView::ViewMenuBar() {
	return MenuBar();
}

CEikStatusPane* CBuddycloudExplorerView::ViewStatusPane() {
	return StatusPane();
}

CEikButtonGroupContainer* CBuddycloudExplorerView::ViewCba() {
	return Cba();
}

#ifdef __SERIES60_40__
CAknToolbar* CBuddycloudExplorerView::ViewToolbar() {
	return Toolbar();
}
#endif

// End of File

