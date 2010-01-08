/*
============================================================================
 Name        : BuddycloudEditChannelView.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2009
 Description : Declares Edit Channel View
============================================================================
*/

// INCLUDE FILES
#include <aknviewappui.h>
#include <barsread.h>
#include <avkon.hrh>
#include "Buddycloud.hrh"
#include <Buddycloud_lang.rsg>
#include "BuddycloudEditChannelView.h"
#include "BuddycloudEditChannelList.h"


// ================= MEMBER FUNCTIONS =======================

void CBuddycloudEditChannelView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_EDITCHANNEL_VIEW);

	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudEditChannelView::~CBuddycloudEditChannelView() {
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
	}
}

TUid CBuddycloudEditChannelView::Id() const {
	return KEditChannelViewId;
}

void CBuddycloudEditChannelView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iList) {
		iList->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudEditChannelView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuEditItemCommand) {
		iList->EditCurrentItemL();
	}
	else if(aCommand == EMenuHelpCommand) {
		AppUi()->HandleCommandL(aCommand);
	}
	else if(aCommand == EAknSoftkeyDone || aCommand == EAknSoftkeyCancel) {
		TViewReferenceBuf aViewReference;
		TInt aItemId = 0;
	
		if(aCommand == EAknSoftkeyDone && (aItemId = iList->SaveChannelDataL()) > 0) {
			// Channel created
			aViewReference().iCallbackViewId = iPrevViewId;
			aViewReference().iOldViewData = iPrevViewData;
			aViewReference().iNewViewData.iId = aItemId;
				
			iPrevViewId = KMessagingViewId;
			iPrevViewData.iId = aItemId;
		}
		else {
			aViewReference().iCallbackViewId = KEditChannelViewId;
			aViewReference().iNewViewData = iPrevViewData;
		}
			
		AppUi()->ActivateLocalViewL(iPrevViewId, TUid::Uid(iPrevViewData.iId), aViewReference);
	}
}

void CBuddycloudEditChannelView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& aCustomMessage) {
	if(!iList) {
		TViewReferenceBuf aViewReference;
		aViewReference.Copy(aCustomMessage);
		
		iPrevViewId = aViewReference().iCallbackViewId;
		iPrevViewData = aViewReference().iOldViewData;
		
		iList = new (ELeave) CBuddycloudEditChannelList(iBuddycloudLogic);
		iList->ConstructL(ClientRect(), aViewReference().iNewViewData);
		iList->SetMopParent(this);

		TResourceReader aReader;
		iEikonEnv->CreateResourceReaderLC(aReader, R_EDITCHANNEL_ITEM_LIST);
		iList->ConstructFromResourceL(aReader);
		CleanupStack::PopAndDestroy(); // aReader

		AppUi()->AddToViewStackL(*this, iList);
		iList->ActivateL();
	}
}

void CBuddycloudEditChannelView::DoDeactivate() {
	if(iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

