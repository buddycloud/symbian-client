/*
============================================================================
 Name        : BuddycloudPreferencesSettingsList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Settings List
============================================================================
*/

#include <aknnavide.h>
#include <akntabgrp.h>
#include <akntextsettingpage.h>
#include <barsread.h>
#include <eikspane.h>
#include <Buddycloud.rsg>
#include "Buddycloud.hrh"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudPreferencesSettingsList.h"
#include "BuddycloudConstants.h"

CBuddycloudPreferencesSettingsList::CBuddycloudPreferencesSettingsList(CBuddycloudLogic* aBuddycloudLogic) : CAknSettingItemList() {
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudPreferencesSettingsList::ConstructL(const TRect& aRect) {
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_SETTINGS_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	CAknTabGroup* aTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	aTabGroup->SetActiveTabById(ETabPreferences);

	iNaviPane->PushL(*iNaviDecorator);

	SetRect(aRect);
}

CBuddycloudPreferencesSettingsList::~CBuddycloudPreferencesSettingsList() {
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
}

void CBuddycloudPreferencesSettingsList::EditCurrentItemL() {
	EditItemL(ListBox()->CurrentItemIndex(), false);
}

void CBuddycloudPreferencesSettingsList::SaveL() {
	if(iLanguageUsed != iBuddycloudLogic->GetIntSetting(ESettingItemLanguage)) {
		iBuddycloudLogic->SettingsItemChanged(ESettingItemLanguage);
	}
}

TKeyResponse CBuddycloudPreferencesSettingsList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyLeftArrow ) {
			SaveL();
			
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KAccountSettingsViewId));

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyRightArrow) {
			SaveL();

			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KNotificationsSettingsViewId));

			aResult = EKeyWasConsumed;
		}
	}
	
	if(aResult == EKeyWasNotConsumed) {
		CAknSettingItemList::OfferKeyEventL(aKeyEvent, aType);
	}
	
	return aResult;
}

void CBuddycloudPreferencesSettingsList::EditItemL(TInt aIndex, TBool aCalledFromMenu) {
	CAknSettingItemArray* aItemArray = SettingItemArray();
	TInt aIdentifier = ((*aItemArray)[aIndex])->Identifier();
	
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
	
	((*aItemArray)[aIndex])->StoreL();
	((*aItemArray)[aIndex])->UpdateListBoxTextL();
}

CAknSettingItem* CBuddycloudPreferencesSettingsList::CreateSettingItemL (TInt aIdentifier) {
	CAknSettingItem* aSettingItem = NULL;

    switch(aIdentifier) {
		case ESettingPreferencesLanguage:
			iLanguageUsed = iBuddycloudLogic->GetIntSetting(ESettingItemLanguage);
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemLanguage));
			break;
		case ESettingPreferencesBlocking:
			aSettingItem = new (ELeave) CAknBinaryPopupSettingItem(aIdentifier, iBuddycloudLogic->GetBoolSetting(ESettingItemMessageBlocking));
			break;
		case ESettingPreferencesAccessPoint:
			aSettingItem = new (ELeave) CAknBinaryPopupSettingItem(aIdentifier, iBuddycloudLogic->GetBoolSetting(ESettingItemAccessPoint));
			break;
		case ESettingPreferencesAutoStart:
			aSettingItem = new (ELeave) CAknBinaryPopupSettingItem(aIdentifier, iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart));
			break;
		default:
			break;
    }

    return aSettingItem;
}
