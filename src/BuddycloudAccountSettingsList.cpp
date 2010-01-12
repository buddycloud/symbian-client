/*
============================================================================
 Name        : BuddycloudAccountSettingsList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Settings List
============================================================================
*/

#include <aknnavide.h>
#include <aknnotewrappers.h> 
#include <akntabgrp.h>
#include <akntextsettingpage.h>
#include <barsread.h>
#include <eikspane.h>
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudAccountSettingsList.h"
#include "BuddycloudConstants.h"

CBuddycloudAccountSettingsList::CBuddycloudAccountSettingsList(CBuddycloudLogic* aBuddycloudLogic) : CAknSettingItemList() {
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudAccountSettingsList::ConstructL(const TRect& aRect) {
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_SETTINGS_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	CAknTabGroup* aTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	aTabGroup->SetActiveTabById(ETabAccount);

	iNaviPane->PushL(*iNaviDecorator);

	SetRect(aRect);
}

CBuddycloudAccountSettingsList::~CBuddycloudAccountSettingsList() {
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
}

void CBuddycloudAccountSettingsList::EditCurrentItemL() {
	EditItemL(ListBox()->CurrentItemIndex(), false);
}

void CBuddycloudAccountSettingsList::ActivateL(TInt aSelectItem) {
	CCoeControl::ActivateL();
	
	ListBox()->SetCurrentItemIndexAndDraw(aSelectItem);
}

void CBuddycloudAccountSettingsList::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KAccountSettings;
}

TKeyResponse CBuddycloudAccountSettingsList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyLeftArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KBeaconSettingsViewId));

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPreferencesSettingsViewId));

			aResult = EKeyWasConsumed;
		}
	}
	
	if(aResult == EKeyWasNotConsumed) {
		CAknSettingItemList::OfferKeyEventL(aKeyEvent, aType);
	}
	
	return aResult;
}

void CBuddycloudAccountSettingsList::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);

		CAknSettingItemArray* aItemArray = SettingItemArray();
		TInt aIdentifier = ((*aItemArray)[ListBox()->CurrentItemIndex()])->Identifier();
		
		if((aIdentifier == ESettingAccountUsername || aIdentifier == ESettingAccountPassword) && 
				iBuddycloudLogic->GetState() != ELogicOffline) {
			
			aMenuPane->SetItemDimmed(EMenuEditItemCommand, true);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuEditItemCommand, false);
		}
	}
}

void CBuddycloudAccountSettingsList::EditItemL(TInt aIndex, TBool aCalledFromMenu) {
	CAknSettingItemArray* aItemArray = SettingItemArray();
	TInt aIdentifier = ((*aItemArray)[aIndex])->Identifier();
	
	if(aIdentifier >= ESettingAccountUsername && iBuddycloudLogic->GetState() != ELogicOffline) {
		HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_OFFLINEREQUIRED);
		CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
		aDialog->ExecuteLD(*aMessage);
		CleanupStack::PopAndDestroy(); // aMessage
	}
	else {
		// Edit item		
		TBuf<64> iCurrentItemData(aItemArray->MdcaPoint(aIndex).Left(64));
		
		CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
		
		StoreSettingsL();
		
		if(iCurrentItemData.Compare(aItemArray->MdcaPoint(aIndex).Left(64)) != 0) {
			// Data changed
			if(aIdentifier == ESettingAccountUsername) {
				iBuddycloudLogic->ValidateUsername();
				
				LoadSettingsL();
				
				if(iCurrentItemData.Compare(iBuddycloudLogic->GetDescSetting(ESettingItemUsername)) != 0) {
					// Username is changed
					iBuddycloudLogic->ResetStoredDataL();
					
					ListBox()->SetCurrentItemIndexAndDraw(ESettingItemServer - 1);
				}
			}
		}
		
		((*aItemArray)[aIndex])->UpdateListBoxTextL();
	}
}

CAknSettingItem* CBuddycloudAccountSettingsList::CreateSettingItemL (TInt aIdentifier) {
	CAknSettingItem* aSettingItem = NULL;

    switch(aIdentifier) {
		case ESettingAccountFullName:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iBuddycloudLogic->GetDescSetting(ESettingItemFullName));
			aSettingItem->SetSettingPageFlags(CAknTextSettingPage::EZeroLengthAllowed | CAknTextSettingPage::EPredictiveTextEntryPermitted);
			break;
		case ESettingAccountUsername:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
			break;
		case ESettingAccountPassword:
			aSettingItem = new (ELeave) CAknPasswordSettingItem(aIdentifier, CAknPasswordSettingItem::EAlpha, iBuddycloudLogic->GetDescSetting(ESettingItemPassword));
			break;
		case ESettingAccountServer:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iBuddycloudLogic->GetDescSetting(ESettingItemServer));
			aSettingItem->SetSettingPageFlags(CAknTextSettingPage::EZeroLengthAllowed | CAknTextSettingPage::EPredictiveTextEntryPermitted);
			break;
		default:
			break;
    }

    return aSettingItem;
}
