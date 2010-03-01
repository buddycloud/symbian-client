/*
============================================================================
 Name        : BuddycloudSetupView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Setup View
============================================================================
*/

// INCLUDE FILES
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include "BuddycloudSetupView.h"
#include "BuddycloudSetupContainer.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudSetupView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL();

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudSetupView::~CBuddycloudSetupView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack( *this, iContainer );
		delete iContainer;
	}
}

TUid CBuddycloudSetupView::Id() const {
	return KSetupViewId;
}

void CBuddycloudSetupView::HandleControlEventL(CCoeControl* /*aControl*/, TCoeEvent /*aEventType*/) {
	if(iContainer) {
		AppUi()->ActivateLocalViewL(KExplorerViewId);
	}
}

void CBuddycloudSetupView::DoActivateL(const TVwsViewId& /*aPrevViewId*/,TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/) {
	if (!iContainer) {
		iContainer = new (ELeave) CBuddycloudSetupContainer(iBuddycloudLogic);
		iContainer->ConstructL();
		iContainer->SetMopParent(this);
		iContainer->SetObserver(this);
		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudSetupView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

// End of File

