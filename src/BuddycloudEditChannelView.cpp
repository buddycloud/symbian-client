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
	else if(aCommand == EAknSoftkeyDone) {
		TInt aItemId = iList->SaveChannelDataL();
		
		AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(aItemId), _L8(""));
	}
	else if(aCommand == EAknSoftkeyCancel) {				
		AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelsViewId));
	}
}

void CBuddycloudEditChannelView::DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& /*aCustomMessage*/) {
	if (!iList) {
		iPrevViewId = aPrevViewId;
		iPrevViewMessageId = aCustomMessageId;
		
		iList = new (ELeave) CBuddycloudEditChannelList;
		iList->ConstructL(ClientRect(), iBuddycloudLogic, aCustomMessageId.iUid);
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
	if (iList) {
		AppUi()->RemoveFromViewStack(*this, iList);
		delete iList;
		iList = NULL;
	}
}

// End of File

