/*
============================================================================
 Name        : BuddycloudChannelInfoView.cpp
 Author      : Ross Savage
 Copyright   : 2010 Buddycloud
 Description : Declares Channel Info View
============================================================================
*/

// INCLUDE FILES
#include <aknviewappui.h>
#include <avkon.hrh>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"
#include "BuddycloudChannelInfoView.h"
#include "BuddycloudChannelInfoContainer.h"

// ================= MEMBER FUNCTIONS =======================

void CBuddycloudChannelInfoView::ConstructL(CBuddycloudLogic* aBuddycloudLogic) {
	BaseConstructL(R_CHANNELINFO_VIEW);
	
	iBuddycloudLogic = aBuddycloudLogic;
}

CBuddycloudChannelInfoView::~CBuddycloudChannelInfoView() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
	}
}

TUid CBuddycloudChannelInfoView::Id() const {
	return KChannelInfoViewId;
}

void CBuddycloudChannelInfoView::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(iContainer) {
		iContainer->DynInitMenuPaneL(aResourceId, aMenuPane);
	}
}

void CBuddycloudChannelInfoView::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuHelpCommand) {		
		AppUi()->HandleCommandL(aCommand);
	}
	else if(aCommand == EAknSoftkeyBack) {
		TViewReferenceBuf aViewReference;
		aViewReference().iNewViewData = iPrevViewData;
		
		AppUi()->ActivateLocalViewL(iPrevViewId, TUid::Uid(iPrevViewData.iId), aViewReference);
	}
	else if(iContainer) {
		iContainer->HandleCommandL(aCommand);
	}
}

void CBuddycloudChannelInfoView::DoActivateL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& aCustomMessage) {
	if (!iContainer) {
		TViewReferenceBuf aViewReference;
		aViewReference.Copy(aCustomMessage);
		
		if(aViewReference().iCallbackRequested) {
			iPrevViewId = aViewReference().iCallbackViewId;
			iPrevViewData = aViewReference().iOldViewData;
		}
		
		iContainer = new (ELeave) CBuddycloudChannelInfoContainer(iBuddycloudLogic);
		iContainer->ConstructL(ClientRect(), aViewReference());
		iContainer->SetMopParent(this);
 		AppUi()->AddToViewStackL(*this, iContainer);
	}
}

void CBuddycloudChannelInfoView::DoDeactivate() {
	if (iContainer) {
		AppUi()->RemoveFromViewStack(*this, iContainer);
		delete iContainer;
		iContainer = NULL;
	}
}

// End of File
