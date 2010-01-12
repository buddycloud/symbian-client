/*
============================================================================
 Name        : BuddycloudMessagingView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Messaging View
============================================================================
*/

// INCLUDE FILES
#include <aknviewappui.h>
#include <eikmenub.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudMessagingView.h"
#include "BuddycloudMessagingContainer.h"
#include "ViewReference.h"

// ================= MEMBER FUNCTIONS =======================

void CBuddycloudMessagingView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_MESSAGING_VIEW);
	
#ifdef __SERIES60_40__
	if(Toolbar()) {
		Toolbar()->SetToolbarObserver(this);
	}
#endif

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudMessagingView::~CBuddycloudMessagingView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudMessagingView::Id() const {
	return KMessagingViewId;
}

void CBuddycloudMessagingView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudMessagingView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuHelpCommand) {
		AppUi()->HandleCommandL(aCommand);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingView::OfferToolbarEventL(TInt aCommandId) {
	if(iContainer) {
		iContainer->HandleCommandL(aCommandId);
	}
}
#endif

void CBuddycloudMessagingView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid aCustomMessageId, const TDesC8& aCustomMessage) {
	if (!iContainer) {
		TViewReferenceBuf aViewReference;
		aViewReference.Copy(aCustomMessage);
		
		TViewData aQuery = aViewReference().iNewViewData;
		
		if(aQuery.iData.Length() == 0) {
			// Nothing referenced, get channel node as default
			CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
			CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aCustomMessageId.iUid));
					
			if(aItem && aItem->GetItemType() >= EItemRoster) {
				aQuery.iId = aItem->GetItemId();
				aQuery.iData.Copy(aItem->GetId());
				aQuery.iTitle.Copy(aItem->GetTitle());
			}
		}
	
		iContainer = new (ELeave) CBuddycloudMessagingContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aQuery);
		iContainer->SetMopParent(this);
		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudMessagingView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

CEikMenuBar* CBuddycloudMessagingView::ViewMenuBar() {
	return MenuBar();
}

CEikStatusPane* CBuddycloudMessagingView::ViewStatusPane() {
	return StatusPane();
}

CEikButtonGroupContainer* CBuddycloudMessagingView::ViewCba() {
	return Cba();
}

#ifdef __SERIES60_40__
CAknToolbar* CBuddycloudMessagingView::ViewToolbar() {
	return Toolbar();
}
#endif

// End of File

