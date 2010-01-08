/*
============================================================================
 Name        : BuddycloudChannelInfoContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2010
 Description : Declares Channel Info Container
============================================================================
*/

#include <avkon.hrh>
#include <Buddycloud_lang.rsg>
#include "BuddycloudChannelInfoContainer.h"
#include "Buddycloud.hlp.hrh"
#include "XmppUtilities.h"

CBuddycloudChannelInfoContainer::CBuddycloudChannelInfoContainer(CBuddycloudLogic* aBuddycloudLogic) : 
		CBuddycloudListComponent(NULL, aBuddycloudLogic) {
	
	iXmppInterface = aBuddycloudLogic->GetXmppInterface();
}

void CBuddycloudChannelInfoContainer::ConstructL(const TRect& aRect, TViewReference aQueryReference) {
	iRect = aRect;
	iQueryReference = aQueryReference;
	
	CreateWindowL();

	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	iShowMialog = false;
	
	TViewData aQuery = iQueryReference.iNewViewData;
	aQuery.iData.LowerCase();

	// Set title
	CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aQuery.iData);
	aQuery.iTitle.Copy(aNodeParser->GetNode(1));
	
	SetTitleL(aQuery.iTitle);
	
	iStatistics.Append(aQuery.iTitle.AllocL());
	CleanupStack::PopAndDestroy(); // aNodeParser
	
	// Get channel data
	iChannelItem = static_cast <CFollowingChannelItem*> (iBuddycloudLogic->GetFollowingStore()->GetItemById(iBuddycloudLogic->IsSubscribedTo(iTextUtilities->Utf8ToUnicodeL(aQuery.iData), EItemChannel)));
	
	// Collect metadata
	_LIT8(KDiscoItemsStanza, "<iq to='' type='get' id='metadata1'><query xmlns='http://jabber.org/protocol/disco#info' node=''/></iq>\r\n");
	HBufC8* aDiscoItemsStanza = HBufC8::NewLC(KDiscoItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aQuery.iData.Length());
	TPtr8 pDiscoItemsStanza(aDiscoItemsStanza->Des());
	pDiscoItemsStanza.Append(KDiscoItemsStanza);
	pDiscoItemsStanza.Insert(95, aQuery.iData);
	pDiscoItemsStanza.Insert(8, KBuddycloudPubsubServer);
		
	iXmppInterface->SendAndAcknowledgeXmppStanza(pDiscoItemsStanza, this, true);
	CleanupStack::PopAndDestroy(); // aDiscoItemsStanza
	
	SetRect(iRect);	
	RenderScreen();
	ActivateL();
}

CBuddycloudChannelInfoContainer::~CBuddycloudChannelInfoContainer() {
	if(iBuddycloudLogic) {
		iXmppInterface->CancelXmppStanzaAcknowledge(this);
	}
	
	if(iChannelTitle) {
		delete iChannelTitle;
	}
	
	if(iChannelDescription) {
		delete iChannelDescription;
	}

	for(TInt i = 0; i < iStatistics.Count(); i++) {
		delete iStatistics[i];
	}
	
	iStatistics.Close();
}
	
void CBuddycloudChannelInfoContainer::RenderWrappedText(TInt /*aIndex*/) {
	// Clear
	ClearWrappedText();
	
	if(iDataCollected) {		
		// Wrap
		if(iChannelDescription) {
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, *iChannelDescription, (iRect.Width() - 2 - iLeftBarSpacer - iRightBarSpacer));
		}
	}
	else {
		// Empty list
		HBufC* aEmptyList = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_REQUESTING);		
		iTextUtilities->WrapToArrayL(iWrappedTextArray, i10BoldFont, *aEmptyList, (iRect.Width() - 2 - iLeftBarSpacer - iRightBarSpacer));		
		CleanupStack::PopAndDestroy(); // aEmptyList		
	}
}

TInt CBuddycloudChannelInfoContainer::CalculateItemSize(TInt /*aIndex*/) {
	return 0;
}

void CBuddycloudChannelInfoContainer::RenderListItems() {
	TInt aDrawPos = -iScrollbarHandlePosition;
	
	iBufferGc->SetPenColor(iColourText);

	if(iDataCollected) {
		if(iChannelTitle) { 
			// Icon
			TInt aIconId = KIconChannel;
			
			if(iChannelItem) {
				aIconId = iChannelItem->GetIconId();
			}
			
			iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
			iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aDrawPos + 2)), iAvatarRepository->GetImage(aIconId, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aIconId, true, iIconMidmapSize), true);
			iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	
			// Title
			TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(*iChannelTitle));
			
			if(i13BoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - iRightBarSpacer)) < aDirectionalText.Length()) {
				iBufferGc->UseFont(i10BoldFont);
			}
			else {
				iBufferGc->UseFont(i13BoldFont);
			}
	
			aDrawPos += i13BoldFont->HeightInPixels();
			iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aDrawPos));
			aDrawPos += ((iItemIconSize + 2) - i13BoldFont->HeightInPixels());
			iBufferGc->DiscardFont();
		}

		// Description
		iBufferGc->UseFont(i10ItalicFont);
		
		for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
			aDrawPos += i10ItalicFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(iLeftBarSpacer + 5, aDrawPos));
		}
		
		iBufferGc->DiscardFont();	
		
		// Statistics
		if(iStatistics.Count() > 0) {
			aDrawPos += i10ItalicFont->FontMaxAscent();
			iBufferGc->SetPenStyle(CGraphicsContext::EDashedPen);
			iBufferGc->DrawLine(TPoint(iLeftBarSpacer + 4, aDrawPos), TPoint((iRect.Width() - iRightBarSpacer - 5), aDrawPos));		
			
			aDrawPos += i10ItalicFont->FontMaxDescent();
			iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);		
			iBufferGc->UseFont(i10NormalFont);

			for(TInt i = 0; i < iStatistics.Count(); i++) {
				aDrawPos += i10NormalFont->HeightInPixels();
				iBufferGc->DrawText(iStatistics[i]->Des(), TPoint(iLeftBarSpacer + 5, aDrawPos));
			}
			
			iBufferGc->DiscardFont();	
		}
	}
	else {
		// Empty results
		aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * i10BoldFont->HeightInPixels()) / 2));
		
		iBufferGc->UseFont(i10BoldFont);
		
		for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
			aDrawPos += i10ItalicFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2) - (i10BoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2))), aDrawPos));
		}
		
		iBufferGc->DiscardFont();
	}	
}

void CBuddycloudChannelInfoContainer::RepositionItems(TBool aSnapToItem) {
	iTotalListSize = 0;
	
	// Enable snapping again
	if(aSnapToItem) {
		iSnapToItem = aSnapToItem;
		iScrollbarHandlePosition = 0;
	}
	
	// Calculate page size
	iTotalListSize += iItemIconSize + 2;	
	iTotalListSize += (i10ItalicFont->HeightInPixels() * iWrappedTextArray.Count());		
	
	if(iStatistics.Count() > 0) {
		iTotalListSize += (i10ItalicFont->FontMaxAscent() + i10ItalicFont->FontMaxDescent());
		iTotalListSize += (i10NormalFont->HeightInPixels() * iStatistics.Count());
	}
	
	CBuddycloudListComponent::RepositionItems(aSnapToItem);
}

void CBuddycloudChannelInfoContainer::HandleItemSelection(TInt /*aItemId*/) {
	CBuddycloudListComponent::RepositionItems(iSnapToItem);
	
	RenderScreen();
}

void CBuddycloudChannelInfoContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_CHANNELINFO_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditChannelCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeleteCommand, true);
		
		if(iDataCollected) {
			if(iChannelItem) {
				if((iChannelItem->GetItemType() == EItemChannel && iChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator) ||
						(iChannelItem->GetItemType() == EItemRoster && iChannelItem->GetPubsubAffiliation() == EPubsubAffiliationOwner)) {
					
					aMenuPane->SetItemDimmed(EMenuEditChannelCommand, false);
				}

				if(iChannelItem->GetPubsubAffiliation() == EPubsubAffiliationOwner && 
						iChannelItem->GetItemType() == EItemChannel) {
					
					aMenuPane->SetItemDimmed(EMenuDeleteCommand, false);
				}		
			}
			else {
				aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
			}
			
			aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, false);
		}
	}
	else if(aResourceId == R_EXPLORER_OPTIONS_EXPLORE_MENU) {
		aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeePlacesCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeBeenHereCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeGoingHereCommand, true);
		
		if(iDataCollected) {
			aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
		}
	}
}

void CBuddycloudChannelInfoContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuFollowCommand) {
		CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(iQueryReference.iNewViewData.iData);
		TPtrC aId(iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)));
		
		if(aNodeParser->GetNode(0).Compare(_L8("user")) == 0) {
			iBuddycloudLogic->FollowContactL(aId);
		}
		else if(aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
			TInt aItemId = iBuddycloudLogic->FollowChannelL(aId);

			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(aItemId), _L8(""));					
		}
		
		CleanupStack::PopAndDestroy(); // aNodeParser
	}
	else if(aCommand == EMenuEditChannelCommand) {
		TViewReferenceBuf aViewReference;	
		aViewReference().iCallbackViewId = KChannelInfoViewId;
		aViewReference().iOldViewData = iQueryReference.iNewViewData;
		aViewReference().iNewViewData.iId = iChannelItem->GetItemId();

		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditChannelViewId), TUid::Uid(0), aViewReference);					
	}
	else if(aCommand == EMenuDeleteCommand) {
		iBuddycloudLogic->UnfollowChannelL(iChannelItem->GetItemId());
		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId));					
	}
	else if(aCommand == EMenuSeeFollowersCommand) {
		TViewReferenceBuf aViewReference;	
		aViewReference().iCallbackRequested = true;
		aViewReference().iCallbackViewId = KChannelInfoViewId;
		aViewReference().iOldViewData = iQueryReference.iNewViewData;
		
		// Query
		TViewData aQuery;
		aQuery.iData.Append(_L8("<iq to='' type='get' id='exp_followers'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''/></pubsub></iq>"));
		aQuery.iData.Insert(116, iQueryReference.iNewViewData.iData);
		aQuery.iData.Insert(8, KBuddycloudPubsubServer);
		
		iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_FOLLOWEDBY);
		aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, iQueryReference.iNewViewData.iTitle);

		aViewReference().iNewViewData = aQuery;
		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), aViewReference);					
	}
}

void CBuddycloudChannelInfoContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KChannelEditing;
}

void CBuddycloudChannelInfoContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
}

TKeyResponse CBuddycloudChannelInfoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			iScrollbarHandlePosition -= (iRect.Height() / 2);
			HandleItemSelection(iSelectedItem);
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			iScrollbarHandlePosition += (iRect.Height() / 2);
			HandleItemSelection(iSelectedItem);
		}
	}

	return EKeyWasConsumed;
}

void CBuddycloudChannelInfoContainer::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& /*aId*/) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	iDataCollected = true;
	
	if(aAttributeType.Compare(_L8("result")) == 0) {		
		while(aXmlParser->MoveToElement(_L8("field"))) {
			TPtrC8 aAttributeVar(aXmlParser->GetStringAttribute(_L8("var")));
			
			if(aXmlParser->MoveToElement(_L8("value"))) {
				TPtrC aEncDataValue(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));							
				
				if(aAttributeVar.Compare(_L8("pubsub#title")) == 0) {
					iChannelTitle = aEncDataValue.AllocL();
				}
				else if(aAttributeVar.Compare(_L8("pubsub#description")) == 0) {
					iChannelDescription = aEncDataValue.AllocL();
				}
			}
		}
	}
	else if(aAttributeType.Compare(_L8("error")) == 0) {
		if(aXmlParser->MoveToElement(_L8("item-not-found"))) {
			CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(iQueryReference.iNewViewData.iData);
			
			TViewReferenceBuf aViewReference;	
			aViewReference().iCallbackViewId = iQueryReference.iCallbackViewId;
			aViewReference().iOldViewData = iQueryReference.iOldViewData;
			aViewReference().iNewViewData.iData = aNodeParser->GetNode(1);
	
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditChannelViewId), TUid::Uid(0), aViewReference);					
			
			CleanupStack::PopAndDestroy(); // aNodeParser
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(true);
	RenderScreen();
}
