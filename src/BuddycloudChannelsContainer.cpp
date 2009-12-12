/*
============================================================================
 Name        : BuddycloudChannelsContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Channels Container
============================================================================
*/

// INCLUDE FILES
#include <aknnavide.h>
#include <barsread.h>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hlp.hrh"
#include "BuddycloudChannelsContainer.h"
#include "BuddycloudExplorer.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudChannelsContainer
--
----------------------------------------------------------------------------
*/

CBuddycloudChannelsContainer::CBuddycloudChannelsContainer(MViewAccessorObserver* aViewAccessor, 
		CBuddycloudLogic* aBuddycloudLogic) : CBuddycloudExplorerContainer(aViewAccessor, aBuddycloudLogic) {
	
	iXmppInterface = aBuddycloudLogic->GetXmppInterface();
}

void CBuddycloudChannelsContainer::ConstructL(const TRect& aRect) {
	iRect = aRect;
	CreateWindowL();
	
	SetPrevView(TVwsViewId(TUid::Uid(APPUID), KPlacesViewId), TUid::Uid(0));

	// Tabs
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_DEFAULT_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	iTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	iTabGroup->SetActiveTabById(ETabChannels);
	iTabGroup->SetObserver(this);

	iNaviPane->PushL(*iNaviDecorator);
	
	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	// Strings
	iLocalizedRank = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_CHANNELRANK);
	
	// Set title
	HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_CHANNELSTAB_HINT);
	SetTitleL(*aTitle);
	iTimer->After(15000000);
	
	// Set level data
	CExplorerQueryLevel* aExplorerLevel = CExplorerQueryLevel::NewLC();
	aExplorerLevel->SetQueryTitleL(*aTitle);
	
	// Push onto stack
	iExplorerLevels.Append(aExplorerLevel);
	CleanupStack::Pop(); // aExplorerLevel
	
	CleanupStack::PopAndDestroy(); // aTitle
	
	// Initialize explorer
    CreateDirectoryItemL(_L("featured"), R_LOCALIZED_STRING_LIST_FEATUREDCHANNELS);
//    CreateDirectoryItemL(_L("suggestions"), R_LOCALIZED_STRING_LIST_CHANNELSYOUMIGHTLIKE);
    CreateDirectoryItemL(_L("social"), R_LOCALIZED_STRING_LIST_FOLLOWERSFAVOURITES);
    CreateDirectoryItemL(_L("popular"), R_LOCALIZED_STRING_LIST_MOSTPOPULARCHANNELS);
    CreateDirectoryItemL(_L("local"), R_LOCALIZED_STRING_LIST_CHANNELSNEARTOYOU);
	
	SetRect(iRect);
	RenderScreen();
	ActivateL();
}

void CBuddycloudChannelsContainer::CreateDirectoryItemL(const TDesC& aId, TInt aTitleResource) {
	CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
	aResultItem->SetResultType(EExplorerItemDirectory);
	aResultItem->SetIconId(KIconChannel);
	aResultItem->SetOverlayId(KOverlayDirectory);
	aResultItem->SetIdL(aId);
	
	HBufC* aTitle = iEikonEnv->AllocReadResourceLC(aTitleResource);
	aResultItem->SetTitleL(*aTitle);	
	CleanupStack::PopAndDestroy(); // aTitle
	
	// Add item
	iExplorerLevels[0]->iResultItems.Append(aResultItem);
	CleanupStack::Pop(); // aResultItem
}

void CBuddycloudChannelsContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KChannelsTab;
}

TKeyResponse CBuddycloudChannelsContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKeyDown) {
		if(aKeyEvent.iScanCode == EStdKeyLeftArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPlacesViewId), TUid::Uid(0), _L8(""));
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iScanCode == EStdKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), _L8(""));
			aResult = EKeyWasConsumed;
		}
	}
	
	if(aResult == EKeyWasNotConsumed) {
		aResult = CBuddycloudExplorerContainer::OfferKeyEventL(aKeyEvent, aType);
	}

	return aResult;
}

// End of File
