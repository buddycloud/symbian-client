/*
============================================================================
 Name        : BuddycloudEditChannelList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Edit Channel List
============================================================================
*/

#include <aknnotewrappers.h>
#include <akntitle.h>
#include <akntextsettingpage.h>
#include <eikspane.h>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudConstants.h"
#include "BuddycloudEditChannelList.h"
#include "TextUtilities.h"
#include "XmlParser.h"

CBuddycloudEditChannelList::CBuddycloudEditChannelList() : CAknSettingItemList() {
	iChannelItem = NULL;
	iTitleResourceId = R_LOCALIZED_STRING_NEWCHANNEL_TITLE;
}

void CBuddycloudEditChannelList::ConstructL(const TRect& aRect, CBuddycloudLogic* aBuddycloudLogic, TInt aItemId) {
	iBuddycloudLogic = aBuddycloudLogic;
	iXmppInterface = iBuddycloudLogic->GetXmppInterface();
	
	iChannelItem = static_cast <CFollowingChannelItem*> (iBuddycloudLogic->GetFollowingStore()->GetItemById(aItemId));

	if(iChannelItem && iChannelItem->GetId().Length() > 0) {
		iChannelSaveAllowed = true;
	}
	else if(iChannelItem == NULL) {
		iChannelItem = CFollowingChannelItem::NewL();
	}	

	if(iChannelItem->GetItemId() > 0) {		
		iTitleResourceId = R_LOCALIZED_STRING_EDITCHANNEL_TITLE;
	}

	LoadChannelDataL();
	SetTitleL(iTitleResourceId);
	SetRect(aRect);
}

CBuddycloudEditChannelList::~CBuddycloudEditChannelList() {
	iXmppInterface->CancelXmppStanzaAcknowledge(this);
	
	if(iChannelItem && iChannelItem->GetItemId() <= 0) {
		delete iChannelItem;
	}
}

void CBuddycloudEditChannelList::SetTitleL(TInt aResourceId) {
	CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
	HBufC* aTitle = iEikonEnv->AllocReadResourceLC(aResourceId);
	aTitlePane->SetTextL(*aTitle);
	CleanupStack::PopAndDestroy(); // aTitle
}

void CBuddycloudEditChannelList::ValidateChannelId() {
	iId.LowerCase();
	
	for(TInt i = iId.Length() - 1; i >= 0; i--) {
		if(iId[i] == 32) {
			iId[i] = 95;
		}
		else if(iId[i] <= 47 || (iId[i] >= 57 && iId[i] <= 64) ||
				(iId[i] >= 91 && iId[i] <= 96) || iId[i] >= 123) {
			
			iId.Delete(i, 1);
		}
	}
}

void CBuddycloudEditChannelList::EditCurrentItemL() {
	EditItemL(ListBox()->CurrentItemIndex(), false);
}

void CBuddycloudEditChannelList::LoadChannelDataL() {
	iTitle.Copy(iChannelItem->GetTitle().Left(iTitle.MaxLength()));
	iId.Copy(iChannelItem->GetId().Left(iId.MaxLength()));
	iDescription.Copy(iChannelItem->GetDescription().Left(iDescription.MaxLength()));
	iAccess = iChannelItem->GetAccessModel();
}

TInt CBuddycloudEditChannelList::SaveChannelDataL() {
	StoreSettingsL();
	
	if(iChannelSaveAllowed) {
		iChannelItem->SetTitleL(iTitle);
		iChannelItem->SetIdL(iId);
		iChannelItem->SetDescriptionL(iDescription);
		iChannelItem->SetAccessModel((TXmppPubsubAccessModel)iAccess);
		
		if(iChannelItem->GetItemId() <= 0) {
			// Create new channel			
			return iBuddycloudLogic->CreateChannelL(iChannelItem);
		}
		else {
			iBuddycloudLogic->PublishChannelMetadataL(iChannelItem->GetItemId());
		}
		
		return iChannelItem->GetItemId();
	}
	
	return 0;
}

void CBuddycloudEditChannelList::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_SETTINGS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditItemCommand, false);
		aMenuPane->SetItemDimmed(EAknSoftkeyDone, false);
		
		if(!iChannelSaveAllowed) {
			aMenuPane->SetItemDimmed(EAknSoftkeyDone, true);
		}
	}
}

void CBuddycloudEditChannelList::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KChannelEditing;
}

void CBuddycloudEditChannelList::EditItemL(TInt aIndex, TBool aCalledFromMenu) {
	CAknSettingItemArray* aItemArray = SettingItemArray();
	TInt aIdentifier = ((*aItemArray)[aIndex])->Identifier();

	// Set title
	switch(aIdentifier) {
		case EEditChannelItemTitle:
			SetTitleL(R_LOCALIZED_STRING_EDITCHANNEL_TITLE_TITLE);
			break;
		case EEditChannelItemId:
			SetTitleL(R_LOCALIZED_STRING_EDITCHANNEL_ID_TITLE);
			break;
		case EEditChannelItemDescription:
			SetTitleL(R_LOCALIZED_STRING_EDITCHANNEL_DESCRIPTION_TITLE);
			break;
		case EEditChannelItemAccess:
			SetTitleL(R_LOCALIZED_STRING_EDITCHANNEL_ACCESS_TITLE);
			break;
		default:;
	}
	
	CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);

	// Reset title
	SetTitleL(iTitleResourceId);
	
	StoreSettingsL();
	
	if(iChannelItem->GetItemId() <= 0 && ((aIdentifier == EEditChannelItemTitle && iId.Length() == 0) || aIdentifier == EEditChannelItemId)) {
		// Check for availablility
		iChannelSaveAllowed = false;
		
		if(aIdentifier == EEditChannelItemTitle) {
			iId.Copy(iTitle);
		}
		
		ValidateChannelId();
		
		LoadSettingsL();
		
		if(iId.Length() > 0) {
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			TPtrC8 aEncId = aTextUtilities->UnicodeToUtf8L(iId);
			
			_LIT8(KDiscoItemsStanza, "<iq to='' type='get' id='discoinfo'><query xmlns='http://jabber.org/protocol/disco#info' node='/channel/'/></iq>\r\n");
			HBufC8* aDiscoItemsStanza = HBufC8::NewLC(KDiscoItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length());
			TPtr8 pDiscoItemsStanza(aDiscoItemsStanza->Des());
			pDiscoItemsStanza.Append(KDiscoItemsStanza);
			pDiscoItemsStanza.Insert(104, aEncId);
			pDiscoItemsStanza.Insert(8, KBuddycloudPubsubServer);
			
			iXmppInterface->SendAndAcknowledgeXmppStanza(pDiscoItemsStanza, this, true);
			CleanupStack::PopAndDestroy(2); // aDiscoItemsStanza, aTextUtilities
		}
		
		ListBox()->SetCurrentItemIndexAndDraw(EEditChannelItemId - 1);
	}
	
	((*aItemArray)[aIndex])->UpdateListBoxTextL();	
}

CAknSettingItem* CBuddycloudEditChannelList::CreateSettingItemL (TInt aIdentifier) {
	CAknSettingItem* aSettingItem = NULL;

    switch(aIdentifier) {
		case EEditChannelItemTitle:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iTitle);
			aSettingItem->SetSettingPageFlags(CAknTextSettingPage::EPredictiveTextEntryPermitted);
			break;
		case EEditChannelItemId:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iId);
			aSettingItem->SetSettingPageFlags(CAknTextSettingPage::EPredictiveTextEntryPermitted);
			
			if(iChannelItem->GetItemId() > 0) {
				aSettingItem->SetHidden(true);
			}
			break;
		case EEditChannelItemDescription:
			aSettingItem = new (ELeave) CAknTextSettingItem(aIdentifier, iDescription);
			aSettingItem->SetSettingPageFlags(CAknTextSettingPage::EZeroLengthAllowed | CAknTextSettingPage::EPredictiveTextEntryPermitted);
			break;
		case EEditChannelItemAccess:
			aSettingItem = new (ELeave) CAknBinaryPopupSettingItem(aIdentifier, iAccess);
			
			if(iChannelItem->GetItemType() != EItemChannel) {
				aSettingItem->SetHidden(true);
			}
			break;
		default:
			break;
    }

    return aSettingItem;
}

void CBuddycloudEditChannelList::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	if(aXmlParser->MoveToElement(_L8("item-not-found"))) {
		iChannelSaveAllowed = true;		
	}
	else {
		HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CHANNELEXISTS);
		CAknErrorNote* aDialog = new (ELeave) CAknErrorNote(true);		
		aDialog->ExecuteLD(*aMessage);
		CleanupStack::PopAndDestroy(); // aMessage
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}
