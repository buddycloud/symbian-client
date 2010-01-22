/*
============================================================================
 Name        : BuddycloudEditPlaceView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Edit Place View
============================================================================
*/

// INCLUDE FILES
#include <aknviewappui.h>
#include <barsread.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudEditPlaceView.h"
#include "BuddycloudEditPlaceList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudEditPlaceView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_EDITPLACE_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudEditPlaceView::~CBuddycloudEditPlaceView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudEditPlaceView::Id() const {
	return KEditPlaceViewId;
}

void CBuddycloudEditPlaceView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iList) {
		iList->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudEditPlaceView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuEditItemCommand) {
		if(iList) {
			iList->EditCurrentItemL();
		}
	}
	else if(aCommand == EMenuHelpCommand) {
		AppUi()->HandleCommandL(aCommand);
	}
	else if(aCommand == EMenuNewSearchCommand) {
		if(iList) {
			iList->SaveL();
		}
		
		iBuddycloudLogic->SendEditedPlaceDetailsL(true);
	}
	else if(aCommand == EAknSoftkeyDone) {
		if(iList) {
			iList->SaveL();
		}
		
		iBuddycloudLogic->SendEditedPlaceDetailsL(false);
		
		AppUi()->ActivateViewL(iPrevViewId, iPrevViewMessageId, KNullDesC8);
	}
	else if(aCommand == EAknSoftkeyCancel) {		
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iBuddycloudLogic->GetPlaceStore()->GetEditedPlace());
		
		if(aPlace) {
			if(aPlace->GetItemId() > 0) {
				// Recollect old place details
				iBuddycloudLogic->GetPlaceDetailsL(aPlace->GetItemId());
			}
			else {
				// Delete editing place
				iBuddycloudLogic->GetPlaceStore()->DeleteItemById(aPlace->GetItemId());
			}
		}
		
		iBuddycloudLogic->GetPlaceStore()->SetEditedPlace(KErrNotFound);
		
		AppUi()->ActivateViewL(iPrevViewId, iPrevViewMessageId, KNullDesC8);
	}
}

void CBuddycloudEditPlaceView::HandleControlEventL(CCoeControl* /*aControl*/, TCoeEvent /*aEventType*/) {
	AppUi()->ActivateViewL(iPrevViewId, iPrevViewMessageId, KNullDesC8);
}

void CBuddycloudEditPlaceView::DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		iPrevViewId = aPrevViewId;
		iPrevViewMessageId = aCustomMessageId;
		
		iList = new (ELeave) CBuddycloudEditPlaceList;
		iList->ConstructL(ClientRect(), iBuddycloudLogic);
		iList->SetMopParent(this);
		iList->SetObserver(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_EDITPLACE_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL();
	}
}

void CBuddycloudEditPlaceView::DoDeactivate() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

