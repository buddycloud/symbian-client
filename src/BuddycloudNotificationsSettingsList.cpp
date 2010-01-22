/*
============================================================================
 Name        : BuddycloudNotificationsSettingsList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Settings List
============================================================================
*/

#include <AknListQueryDialog.h>
#include <aknnavide.h>
#include <akntabgrp.h>
#include <akntextsettingpage.h>
#include <barsread.h>
#include <eikspane.h>
#include <f32file.h>
#include <Buddycloud.rsg>
#include "Buddycloud.hrh"
#include "BuddycloudNotificationsSettingsList.h"
#include "BuddycloudConstants.h"

CBuddycloudNotificationsSettingsList::CBuddycloudNotificationsSettingsList(CBuddycloudLogic* aBuddycloudLogic) : CAknSettingItemList() {
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudNotificationsSettingsList::ConstructL(const TRect& aRect) {
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_SETTINGS_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	CAknTabGroup* aTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	aTabGroup->SetActiveTabById(ETabNotifications);

	iNaviPane->PushL(*iNaviDecorator);

	SetRect(aRect);
}

CBuddycloudNotificationsSettingsList::~CBuddycloudNotificationsSettingsList() {
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
}

void CBuddycloudNotificationsSettingsList::EditCurrentItemL() {
	EditItemL(ListBox()->CurrentItemIndex(), false);
}

TKeyResponse CBuddycloudNotificationsSettingsList::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyLeftArrow ) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPreferencesSettingsViewId));

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KBeaconSettingsViewId));

			aResult = EKeyWasConsumed;
		}
	}
	
	if(aResult == EKeyWasNotConsumed) {
		CAknSettingItemList::OfferKeyEventL(aKeyEvent, aType);
	}
	
	return aResult;
}

void CBuddycloudNotificationsSettingsList::EditItemL(TInt aIndex, TBool aCalledFromMenu) {
	CAknSettingItemArray* aItemArray = SettingItemArray();
	TInt aIdentifier = ((*aItemArray)[aIndex])->Identifier();
	
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
	
	((*aItemArray)[aIndex])->StoreL();
	
	if(aIdentifier == ESettingNotificationsPrivateMessageTone) {		
		iPrivateMessageTone = iBuddycloudLogic->GetIntSetting(ESettingItemPrivateMessageTone);
		
		if(iPrivateMessageTone == EToneUserDefined) {
			SelectCustomToneL(iBuddycloudLogic->GetDescSetting(ESettingItemPrivateMessageToneFile));
		}
		
		iBuddycloudLogic->SettingsItemChanged(ESettingItemPrivateMessageTone);
	}
	else if(aIdentifier == ESettingNotificationsChannelPostTone) {		
		iChannelPostTone = iBuddycloudLogic->GetIntSetting(ESettingItemChannelPostTone);
		
		if(iChannelPostTone == EToneUserDefined) {
			SelectCustomToneL(iBuddycloudLogic->GetDescSetting(ESettingItemChannelPostToneFile));
		}
		
		iBuddycloudLogic->SettingsItemChanged(ESettingItemChannelPostTone);
	}
	else if(aIdentifier == ESettingNotificationsDirectReplyTone) {		
		iDirectReplyTone = iBuddycloudLogic->GetIntSetting(ESettingItemDirectReplyTone);
		
		if(iDirectReplyTone == EToneUserDefined) {
			SelectCustomToneL(iBuddycloudLogic->GetDescSetting(ESettingItemDirectReplyToneFile));
		}
		
		iBuddycloudLogic->SettingsItemChanged(ESettingItemDirectReplyTone);
	}
	
	((*aItemArray)[aIndex])->UpdateListBoxTextL();
}

void CBuddycloudNotificationsSettingsList::ScanDirectoryL(const TDesC& aDirectory, CDesCArray* aFileArray, RPointerArray<HBufC>& aDirArray) {
	CDir* aCurrentFolder = NULL;
	CDirScan* aScanner = CDirScan::NewLC(iEikonEnv->FsSession());
	aScanner->SetScanDataL(aDirectory, KEntryAttNormal, ESortByName|EAscending, CDirScan::EScanDownTree); 
	 
	do {
		TRAPD(aErr, aScanner->NextL(aCurrentFolder));
		
		if(!aErr && aCurrentFolder) {
			CleanupStack::PushL(aCurrentFolder);
			
			for(TInt i = 0; i < aCurrentFolder->Count(); i++) {
				TEntry aCurrentEntry = (*aCurrentFolder)[i];
				
				if(!aCurrentEntry.IsDir()) {
					// Add detail
					aFileArray->AppendL(aCurrentEntry.iName);
					aDirArray.Append(aScanner->FullPath().AllocL());
				} 
			}
			
			CleanupStack::PopAndDestroy(); // aCurrentFolder
		}
	} while(aCurrentFolder);
	
	CleanupStack::PopAndDestroy(); // aScanner
}

void CBuddycloudNotificationsSettingsList::SelectCustomToneL(TDes& aFile) {
	// Initialize arrays
	CDesCArray* aFileArray = new (ELeave) CDesCArrayFlat(5);
	CleanupStack::PushL(aFileArray);
	
	RPointerArray<HBufC> aDirArray;
	
	// Scan directories
	ScanDirectoryL(_L("C:\\Data\\Sounds\\"), aFileArray, aDirArray);
	ScanDirectoryL(_L("E:\\Sounds\\"), aFileArray, aDirArray);
	
	if(aFileArray->Count() > 0) {
		// Show list dialog
		TInt aDetailIndex = 0;

		CAknListQueryDialog* aDialog = new (ELeave) CAknListQueryDialog(&aDetailIndex);
		aDialog->PrepareLC(R_SELECTTONE_QUERY);
		aDialog->SetItemTextArray(aFileArray);
		aDialog->SetOwnershipType(ELbmDoesNotOwnItemArray);
		
		if(aDialog->RunLD()) {
			aFile.Zero();
			aFile.Append(*aDirArray[aDetailIndex]);
			aFile.Append(aFileArray->MdcaPoint(aDetailIndex));
		}
	}
	
	for(TInt i = 0; i < aDirArray.Count(); i++) {
		delete aDirArray[i];
	}
	
	aDirArray.Close();
	
	CleanupStack::PopAndDestroy(); // aFileArray
}

CAknSettingItem* CBuddycloudNotificationsSettingsList::CreateSettingItemL (TInt aIdentifier) {
	CAknSettingItem* aSettingItem = NULL;

    switch(aIdentifier) {
		case ESettingNotificationsNotifyChannelsModerating:
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemNotifyChannelsModerating));
			break;
		case ESettingNotificationsNotifyChannelsFollowing:
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemNotifyChannelsFollowing));
			break;
		case ESettingNotificationsNotifyReplyTo:
			aSettingItem = new (ELeave) CAknBinaryPopupSettingItem(aIdentifier, iBuddycloudLogic->GetBoolSetting(ESettingItemNotifyReplyTo));
			break;
		case ESettingNotificationsPrivateMessageTone:
			iPrivateMessageTone = iBuddycloudLogic->GetIntSetting(ESettingItemPrivateMessageTone);
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemPrivateMessageTone));
			break;
		case ESettingNotificationsChannelPostTone:
			iChannelPostTone = iBuddycloudLogic->GetIntSetting(ESettingItemChannelPostTone);
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemChannelPostTone));
			break;
		case ESettingNotificationsDirectReplyTone:
			iDirectReplyTone = iBuddycloudLogic->GetIntSetting(ESettingItemDirectReplyTone);
			aSettingItem = new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iBuddycloudLogic->GetIntSetting(ESettingItemDirectReplyTone));
			break;
		default:
			break;
    }

    return aSettingItem;
}
