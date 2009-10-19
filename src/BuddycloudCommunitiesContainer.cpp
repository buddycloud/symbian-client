/*
============================================================================
 Name        : BuddycloudCommunitiesContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Communities Container
============================================================================
*/

// INCLUDES
#include <akniconarray.h>
#include <aknmessagequerydialog.h> 
#include <aknquerydialog.h>
#include <Buddycloud_lang.rsg>
#include <barsread.h>
#include <eikclbd.h>
#include "BrowserLauncher.h"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudCommunitiesContainer.h"


CBuddycloudCommunitiesContainer::CBuddycloudCommunitiesContainer(CBuddycloudLogic* aBuddycloudLogic) {
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudCommunitiesContainer::ConstructL(const TRect& aRect) {
	CreateWindowL();
	CreateListItemsL();
	
	SetRect(aRect);
	ActivateL();
}

CBuddycloudCommunitiesContainer::~CBuddycloudCommunitiesContainer() {
	if(iList)
		delete iList;
}

void CBuddycloudCommunitiesContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {	
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
		
		if(iList->CurrentItemIndex() == ECommunityTwitter) {
			if(iBuddycloudLogic->GetDescSetting(ESettingItemTwitterUsername).Length() > 0) {
				aMenuPane->SetItemDimmed(EMenuDeleteCommand, false);
			}
		}
	}
}

TInt FacebookLaunchCallback(TAny* /*aAny*/) {
	CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
	aLauncher->LaunchBrowserWithLinkL(_L("http://apps.facebook.com/buddycloud"));
	CleanupStack::PopAndDestroy(); // aLauncher

	return EFalse;
}

void CBuddycloudCommunitiesContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuEditItemCommand) {
		HandleItemSelectionL(iList->CurrentItemIndex());
	}
	else if(aCommand == EMenuDeleteCommand) {
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();

		if(aDialog->ExecuteLD(R_DELETE_DIALOG) != 0) {
			iBuddycloudLogic->GetDescSetting(ESettingItemTwitterUsername).Zero();
			iBuddycloudLogic->GetDescSetting(ESettingItemTwitterPassword).Zero();
			
			iBuddycloudLogic->SendCommunityCredentials(ECommunityTwitter);
		}
	}
}

void CBuddycloudCommunitiesContainer::CreateListItemsL(){	
	iList = new (ELeave) CAknSingleLargeStyleListBox();
	iList->SetContainerWindowL(*this);

	// Text array
	TResourceReader aReader;
	iEikonEnv->CreateResourceReaderLC(aReader, R_COMMUNITIES_LISTBOX);
	iList->ConstructFromResourceL(aReader);
	CleanupStack::PopAndDestroy(); // aReader
	
	iList->CreateScrollBarFrameL(ETrue);
    iList->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	iList->Model()->SetOwnershipType(ELbmOwnsItemArray);
	
	// Icons
    CAknIconArray* aIcons = new (ELeave) CAknIconArray(7);
    CleanupStack::PushL(aIcons); 
    aIcons->ConstructFromResourceL(R_ICONS_LIST);
    CleanupStack::Pop();  // aListBox
    iList->ItemDrawer()->ColumnData()->SetIconArray(aIcons);    
}

void CBuddycloudCommunitiesContainer::HandleItemSelectionL(TInt aIndex) {
	if(aIndex == ECommunityFacebook) {
		CAknMessageQueryDialog* aMessageDialog = new (ELeave) CAknMessageQueryDialog();
		aMessageDialog->PrepareLC(R_EXIT_DIALOG);
		
		HBufC* aFacebookText = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_SETTINGS_FACEBOOK_TEXT);
		aMessageDialog->SetHeaderTextL(_L("Facebook"));
		aMessageDialog->SetMessageTextL(*aFacebookText);
		CleanupStack::PopAndDestroy(); // aFacebookText

		TCallBack aCallback(FacebookLaunchCallback);
		aMessageDialog->SetLink(aCallback);
		aMessageDialog->SetLinkTextL(_L("http://apps.facebook.com/buddycloud"));
		
		if(aMessageDialog->RunLD() != 0) {		
			aCallback.CallBack();
		}
	}
	else if(aIndex == ECommunityTwitter) {
		CAknMultiLineDataQueryDialog* aDialog = 
					CAknMultiLineDataQueryDialog::NewL(iBuddycloudLogic->GetDescSetting(ESettingItemTwitterUsername), 
					iBuddycloudLogic->GetDescSetting(ESettingItemTwitterPassword));

		if(aDialog->ExecuteLD(R_CREDENTIALS_DIALOG) != 0) {
			iBuddycloudLogic->SendCommunityCredentials(ECommunityTwitter);
		}
	}
}

void CBuddycloudCommunitiesContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KShareOnline;

	if(iList->CurrentItemIndex() == ECommunityTwitter) {
		aContext.iContext = KCommunitiesTwitter;
	}
}

CCoeControl* CBuddycloudCommunitiesContainer::ComponentControl(TInt /*aIndex*/) const {
	return iList;
}
 
TInt CBuddycloudCommunitiesContainer::CountComponentControls() const {	
	if(iList) {
		return 1;
	}
	
	return 0;
}

void CBuddycloudCommunitiesContainer::SizeChanged() {
	if(iList) {
		iList->SetRect(Rect());
	}
}

void CBuddycloudCommunitiesContainer::HandleResourceChange(TInt aType) {
	if(aType == KEikDynamicLayoutVariantSwitch) {
		TRect aRect;
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, aRect);
		SetRect(aRect);
	}
	
	if(iList) {
		iList->HandleResourceChange(aType);
	}
}

TKeyResponse CBuddycloudCommunitiesContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aEventCode) {
	TKeyResponse aResponse = EKeyWasNotConsumed;
	
	if(aEventCode == EEventKey) {
		if(aKeyEvent.iCode == 63557) { // Center Dpad
			aResponse = EKeyWasConsumed;
			HandleCommandL(EMenuEditItemCommand);
		}
	}
	
	if(aResponse == EKeyWasNotConsumed) {
		if(iList) {
			aResponse = iList->OfferKeyEventL(aKeyEvent, aEventCode);
		}
	}
	
	return aResponse;
}

// End of File

