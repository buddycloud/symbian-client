/*
============================================================================
 Name        : BuddycloudChannelInfoContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2010
 Description : Declares Channel Info Container
============================================================================
*/

#include <aknmessagequerydialog.h>
#include <avkon.hrh>
#include <Buddycloud_lang.rsg>
#include "BuddycloudChannelInfoContainer.h"
#include "BuddycloudExplorer.h"
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
	
	SetTitleL(aQuery.iTitle);
	
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
	
	// Description
	if(iChannelDescription) {
		delete iChannelDescription;
	}
	
	for(TInt i = 0; i < iWrappedDescription.Count(); i++) {
		delete iWrappedDescription[i];
	}
	
	iWrappedDescription.Close();

	// Statistics
	for(TInt i = 0; i < iStatistics.Count(); i++) {
		delete iStatistics[i];
	}
	
	iStatistics.Close();
}

void CBuddycloudChannelInfoContainer::AddStatisticL(TInt aTitleResource, TInt aValueResource) {
	HBufC* aValueText = iCoeEnv->AllocReadResourceLC(aValueResource);
	
	AddStatisticL(aTitleResource, *aValueText);
	CleanupStack::PopAndDestroy();
}

void CBuddycloudChannelInfoContainer::AddStatisticL(TInt aTitleResource, TDesC& aValue) {
	HBufC* aTitleText = iCoeEnv->AllocReadResourceLC(aTitleResource);
	
	HBufC* aStatisticText = HBufC::NewLC(aTitleText->Des().Length() + 3 + aValue.Length());
	TPtr pStatisticText(aStatisticText->Des());
	pStatisticText.Append(*aTitleText);
	pStatisticText.Append(_L(" : "));
	pStatisticText.Append(aValue);
	
	iStatistics.Append(aStatisticText);
	CleanupStack::Pop(); // aStatisticText
	
	CleanupStack::PopAndDestroy(); // aTitleText
}
	
void CBuddycloudChannelInfoContainer::RenderWrappedText(TInt /*aIndex*/) {
	// Clear
	ClearWrappedText();
	
	if(iCollectionState == EChannelInfoCollected) {	
		// Wrap texts		
		if(iChannelTitle) {
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i13BoldFont, *iChannelTitle, (iRect.Width() - 5 - iLeftBarSpacer - iRightBarSpacer));
		}
		
		if(iChannelDescription) {
			for(TInt i = 0; i < iWrappedDescription.Count(); i++) {
				delete iWrappedDescription[i];
			}
			
			iWrappedDescription.Reset();
			
			iTextUtilities->WrapToArrayL(iWrappedDescription, i10ItalicFont, *iChannelDescription, (iRect.Width() - 5 - iLeftBarSpacer - iRightBarSpacer));
		}
	}
	else {
		TInt aResourceId = R_LOCALIZED_STRING_NOTE_REQUESTING;
		
		if(iCollectionState == EChannelInfoFailed) {
			aResourceId = R_LOCALIZED_STRING_NOTE_NORESULTSFOUND;
		}
		
		// Empty list
		HBufC* aEmptyList = iEikonEnv->AllocReadResourceLC(aResourceId);		
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

	if(iCollectionState == EChannelInfoCollected) {
		// Title
		iBufferGc->UseFont(i13BoldFont);
		
		for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
			aDrawPos += i13BoldFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(iLeftBarSpacer + 5, aDrawPos));
		}
		
		aDrawPos += i13BoldFont->FontMaxDescent();
		iBufferGc->DiscardFont();	
		
		// Description
		iBufferGc->UseFont(i10ItalicFont);
		
		for(TInt i = 0; i < iWrappedDescription.Count(); i++) {
			aDrawPos += i10ItalicFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedDescription[i]->Des(), TPoint(iLeftBarSpacer + 5, aDrawPos));
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
				aDrawPos += i10NormalFont->FontMaxHeight();
				iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(iStatistics[i]->Des()), TPoint(iLeftBarSpacer + 5, aDrawPos));
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
	iTotalListSize += (i13BoldFont->HeightInPixels() * iWrappedTextArray.Count()) + i13BoldFont->FontMaxDescent();		
	iTotalListSize += (i10ItalicFont->HeightInPixels() * iWrappedDescription.Count());		
	
	if(iStatistics.Count() > 0) {
		iTotalListSize += (i10ItalicFont->FontMaxAscent() + i10ItalicFont->FontMaxDescent());
		iTotalListSize += (i10NormalFont->FontMaxHeight() * iStatistics.Count());
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
		
		if(iCollectionState == EChannelInfoCollected) {
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
		aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeperator, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeModeratingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, true);
		
		if(iCollectionState == EChannelInfoCollected) {
			aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
			aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, false);
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
		HBufC* aMessageText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_DELETECHANNEL_TEXT);		
		
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
		aDialog->PrepareLC(R_DELETE_DIALOG);	
		aDialog->SetMessageTextL(*aMessageText);
		
		if(aDialog->RunLD() != 0) {
			iBuddycloudLogic->UnfollowChannelL(iChannelItem->GetItemId());

			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId));	
		}
		
		CleanupStack::PopAndDestroy(); // aMessageText
	}
	else if(aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeModeratorsCommand) {
		TInt aResourceId = KErrNotFound;
		
		TViewReferenceBuf aViewReference;
		aViewReference().iCallbackRequested = true;
		aViewReference().iCallbackViewId = KChannelInfoViewId;
		aViewReference().iOldViewData = iQueryReference.iNewViewData;
		
		if(aCommand == EMenuSeeModeratorsCommand) {
			CExplorerStanzaBuilder::BuildMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), iQueryReference.iNewViewData.iData, _L8("owner"));
			CExplorerStanzaBuilder::BuildMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), iQueryReference.iNewViewData.iData, _L8("moderator"));
			aResourceId = R_LOCALIZED_STRING_TITLE_MODERATEDBY;
		}
		else {
			CExplorerStanzaBuilder::BuildMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), iQueryReference.iNewViewData.iData, _L8("publisher"));
			CExplorerStanzaBuilder::BuildMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), iQueryReference.iNewViewData.iData, _L8("member"));
			aResourceId = R_LOCALIZED_STRING_TITLE_FOLLOWEDBY;
		}
		
		CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, aResourceId, _L("$OBJECT"), iQueryReference.iNewViewData.iTitle);
		
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
	
	if(aAttributeType.Compare(_L8("result")) == 0) {	
		iCollectionState = EChannelInfoCollected;
		
		if(aXmlParser->MoveToElement(_L8("query"))) {
			CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aXmlParser->GetStringAttribute(_L8("node")));
			
			if(aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
				TPtrC aEncId(iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)));
				
				HBufC* aIdText = HBufC::NewLC(aEncId.Length() + 1);
				TPtr pIdText(aIdText->Des());
				pIdText.Append('#');
				pIdText.Append(aEncId);
				
				SetTitleL(pIdText);
				
				iStatistics.Append(aIdText);
				CleanupStack::Pop(); // aIdText
			}
			
			CleanupStack::PopAndDestroy(); // aNodeParser
				
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
					else if(aAttributeVar.Compare(_L8("pubsub#access_model")) == 0) {
						TXmppPubsubAccessModel aAccess = CXmppEnumerationConverter::PubsubAccessModel(aXmlParser->GetStringData());
						TInt aValueResourceId = R_LOCALIZED_STRING_CHANNELACCESS_OPEN;
						
						if(aAccess == EPubsubAccessWhitelist) {
							aValueResourceId = R_LOCALIZED_STRING_CHANNELACCESS_WHITELIST;
						}
						
						AddStatisticL(R_LOCALIZED_STRING_CHANNELACCESS, aValueResourceId);
					}
					else if(aAttributeVar.Compare(_L8("pubsub#creation_date")) == 0) {
						TTime aCreationDate = CTimeUtilities::DecodeL(aXmlParser->GetStringData());	
						TTime aNow = iBuddycloudLogic->TimeStamp();
						TBuf<10> aStatisticText;
						
						aStatisticText.Format(_L("%d"), aNow.DaysFrom(aCreationDate).Int());		
						
						AddStatisticL(R_LOCALIZED_STRING_NOTE_CHANNELSINCE, aStatisticText);
					}
					else if(aAttributeVar.Compare(_L8("pubsub#channel_rank")) == 0) {
						AddStatisticL(R_LOCALIZED_STRING_NOTE_CHANNELRANK, aEncDataValue);
					}
				}
			}
		}
	}
	else if(aAttributeType.Compare(_L8("error")) == 0) {
		iCollectionState = EChannelInfoFailed;
		
		if(aXmlParser->MoveToElement(_L8("item-not-found"))) {
			CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(iQueryReference.iNewViewData.iData);
			
			if(aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
				TViewReferenceBuf aViewReference;	
				aViewReference().iCallbackViewId = iQueryReference.iCallbackViewId;
				aViewReference().iOldViewData = iQueryReference.iOldViewData;
				aViewReference().iNewViewData.iData.Copy(aNodeParser->GetNode(1).Left(aViewReference().iNewViewData.iData.MaxLength()));
		
				iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditChannelViewId), TUid::Uid(0), aViewReference);					
			}
			
			CleanupStack::PopAndDestroy(); // aNodeParser
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(true);
	RenderScreen();
}
