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
		TMessagingViewObject aObject;
		TMessagingViewObjectPckg aObjectPckg(aObject);
		aObjectPckg.Copy(aCustomMessage);
		
		if(aObject.iId.Length() == 0) {
			// No jid referenced, get channel jid as default
			CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
			CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aCustomMessageId.iUid));
					
			if(aItem && aItem->GetItemType() >= EItemRoster) {
				aObject.iFollowerId = aItem->GetItemId();
				aObject.iId = aItem->GetId();
				aObject.iTitle = aItem->GetTitle();
			}
		}
	
		iContainer = new (ELeave) CBuddycloudMessagingContainer(this, iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aObject);
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

