/*
============================================================================
 Name        : BuddycloudExplorerContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Explorer Container
============================================================================
*/

// INCLUDE FILES
#include <aknnavide.h>
#include <aknmessagequerydialog.h>
#include <barsread.h>
#include <eikmenup.h>
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "BrowserLauncher.h"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudExplorerContainer.h"
#include "BuddycloudMessagingContainer.h"
#include "XmlParser.h"
#include "XmppUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudExplorerContainer
--
----------------------------------------------------------------------------
*/

CBuddycloudExplorerContainer::CBuddycloudExplorerContainer(MViewAccessorObserver* aViewAccessor, 
		CBuddycloudLogic* aBuddycloudLogic) : CBuddycloudListComponent(aViewAccessor, aBuddycloudLogic) {
	
	iXmppInterface = aBuddycloudLogic->GetXmppInterface();
}

void CBuddycloudExplorerContainer::ConstructL(const TRect& aRect, TExplorerQuery aQuery) {
	iRect = aRect;
	CreateWindowL();
	
	SetPrevView(TVwsViewId(TUid::Uid(APPUID), KChannelsViewId), TUid::Uid(0));

	// Tabs
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_DEFAULT_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	iTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	iTabGroup->SetActiveTabById(ETabExplorer);
	iTabGroup->SetObserver(this);

	iNaviPane->PushL(*iNaviDecorator);
	
	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	// Strings
	iLocalizedRank = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_CHANNELRANK);
	
	SetRect(iRect);
	ActivateL();
	
	// Initialize explorer
	if(aQuery.iStanza.Length() == 0) {
		// Add default query (nearby objects)
		CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
		
		if(aOwnItem) {			
			// Build stanza
			aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(EExplorerItemPerson, iTextUtilities->UnicodeToUtf8L(aOwnItem->GetId()));

			iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_EXPLORERTAB_HINT);
		}
	}
	
	PushLevelL(aQuery.iTitle, aQuery.iStanza);
}

void CBuddycloudExplorerContainer::SetPrevView(const TVwsViewId& aViewId, TUid aViewMsgId) {
	iPrevViewId = aViewId;
	iPrevViewMsgId = aViewMsgId;
}

CBuddycloudExplorerContainer::~CBuddycloudExplorerContainer() {
	if(iBuddycloudLogic) {
		iXmppInterface->CancelXmppStanzaAcknowledge(this);
	}

	// Tabs
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
	
	//Strings
	if(iLocalizedRank)
		delete iLocalizedRank;
	
	// Levels
	for(TInt i = 0; i < iExplorerLevels.Count(); i++) {
		delete iExplorerLevels[i];
	}
	
	iExplorerLevels.Close();
}

void CBuddycloudExplorerContainer::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationLocationUpdated) {
		if(iBuddycloudLogic->GetState() == ELogicOnline && iExplorerLevels.Count() == 0) {
			// Re-request nearby objects
			CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
			
			if(aOwnItem) {			
				// Build stanza
				TExplorerQuery aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(EExplorerItemPerson, iTextUtilities->UnicodeToUtf8L(aOwnItem->GetId()));
				iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_EXPLORERTAB_HINT);
				
				PushLevelL(aQuery.iTitle, aQuery.iStanza);
			}
		}
	}
	else {
		CBuddycloudListComponent::NotificationEvent(aEvent, aId);
	}
}

void CBuddycloudExplorerContainer::ParseAndSendXmppStanzasL(const TDesC8& aStanza) {
	iQueryResultSize = 0;
	iQueryResultCount = 0;
	
	if(iBuddycloudLogic->GetState() == ELogicOnline) {
		iExplorerState = EExplorerRequesting;
		
		TPtrC8 aSlicedStanza(aStanza);
		TInt aSearchResult = KErrNotFound;
	
		while((aSearchResult = aSlicedStanza.Find(_L8("\r\n"))) != KErrNotFound) {
			aSearchResult += 2;
			iQueryResultSize++;
	
			// Send query
			iXmppInterface->SendAndAcknowledgeXmppStanza(aSlicedStanza.Left(aSearchResult), this, true);		
			aSlicedStanza.Set(aSlicedStanza.Mid(aSearchResult));
		}
		
		if(aSlicedStanza.Length() > 0) {
			iQueryResultSize++;
			iXmppInterface->SendAndAcknowledgeXmppStanza(aSlicedStanza, this, true);		
		}
		
		if(iQueryResultSize == 0) {
			iExplorerState = EExplorerIdle;
		}
	}
}

void CBuddycloudExplorerContainer::PushLevelL(const TDesC& aTitle, const TDesC8& aStanza) {
	if(aStanza.Length() > 0) {
		if(iExplorerLevels.Count() > 0) {
			TInt aLevel = iExplorerLevels.Count() - 1;
			iExplorerLevels[aLevel]->iSelectedResultItem = iSelectedItem;
		}
		
		if(iExplorerState == EExplorerRequesting) {
			iXmppInterface->CancelXmppStanzaAcknowledge(this);
			iExplorerState = EExplorerIdle;
		}	
		
		// Set level data
		CExplorerQueryLevel* aExplorerLevel = CExplorerQueryLevel::NewLC();
		aExplorerLevel->SetQueryTitleL(aTitle);
		aExplorerLevel->SetQueriedStanzaL(aStanza);
		
		// Push onto stack
		iExplorerLevels.Append(aExplorerLevel);
		CleanupStack::Pop(); // aExplorerLevel
		
		// Set title
		SetTitleL(aTitle);
		iTimer->After(15000000);
	
		// Send query
		ParseAndSendXmppStanzasL(aStanza);
	}
	
	// Update screen
	RenderWrappedText(iSelectedItem);
	RepositionItems(true);
	RenderScreen();
}

void CBuddycloudExplorerContainer::PopLevelL() {
	TInt aLevel = iExplorerLevels.Count() - 1;

	if(aLevel > 0) {
		if(iExplorerState == EExplorerRequesting) {
			iXmppInterface->CancelXmppStanzaAcknowledge(this);
			iExplorerState = EExplorerIdle;
		}
	
		// Pop top level
		delete iExplorerLevels[aLevel];
		iExplorerLevels.Remove(aLevel);
		
		// Set selected item
		aLevel--;
		iSelectedItem = iExplorerLevels[aLevel]->iSelectedResultItem;
		
		// Set title
		SetTitleL(iExplorerLevels[aLevel]->GetQueryTitle());
		iTimer->After(15000000);

		// Update screen
		RenderWrappedText(iSelectedItem);
		RepositionItems(true);
		RenderScreen();
	}
}

void CBuddycloudExplorerContainer::RefreshLevelL() {
	if(iExplorerState == EExplorerIdle) {
		TInt aLevel = iExplorerLevels.Count() - 1;
		
		// Clear previous result data
		iExplorerLevels[aLevel]->ClearResultItems();
		
		// Send query
		ParseAndSendXmppStanzasL(iExplorerLevels[aLevel]->GetQueriedStanza());
		
		// Update screen
		RenderWrappedText(iSelectedItem);
		RepositionItems(true);
		RenderScreen();
	}
}

void CBuddycloudExplorerContainer::RenderWrappedText(TInt aIndex) {
	// Clear
	ClearWrappedText();
	
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0 && 
			aIndex >= 0 && aIndex < iExplorerLevels[aLevel]->iResultItems.Count()) {
			
		// Wrap
		iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, iExplorerLevels[aLevel]->iResultItems[aIndex]->GetDescription(), (iRect.Width() - iSelectedItemIconTextOffset - 5 - iLeftBarSpacer - iRightBarSpacer));
	}
	else {
		// Empty list
		TInt aResourceId = R_LOCALIZED_STRING_NOTE_NORESULTSFOUND;
			
		if(iExplorerState != EExplorerIdle) {
			aResourceId = R_LOCALIZED_STRING_NOTE_REQUESTING;
		}		

		HBufC* aEmptyList = iEikonEnv->AllocReadResourceLC(aResourceId);		
		iTextUtilities->WrapToArrayL(iWrappedTextArray, i10BoldFont, *aEmptyList, (iRect.Width() - 2 - iLeftBarSpacer - iRightBarSpacer));		
		CleanupStack::PopAndDestroy(); // aEmptyList		
	}
}

void CBuddycloudExplorerContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
}

TInt CBuddycloudExplorerContainer::CalculateItemSize(TInt aIndex) {
	TInt aLevel = iExplorerLevels.Count() - 1;
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;
	
	CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[aIndex];

	if(aIndex == iSelectedItem) {
		aMinimumSize = iItemIconSize + 4;
		
		aItemSize += i13BoldFont->HeightInPixels();
		aItemSize += i13BoldFont->FontMaxDescent();
		
		if(aResultItem->GetDescription().Length() > 0) {
			aItemSize += (iWrappedTextArray.Count() * i10ItalicFont->HeightInPixels());
		}
		
		if(aResultItem->GetRank() > 0) {
			if(aItemSize < (iItemIconSize + 2)) {
				aItemSize = (iItemIconSize + 2);
			}

			aItemSize += i10NormalFont->HeightInPixels();
		}
	}
	else {
		aMinimumSize = (iItemIconSize / 2) + 2;
#ifdef __SERIES60_40__
		aMinimumSize += (i10BoldFont->DescentInPixels() * 2);
		aItemSize += (i10BoldFont->DescentInPixels() * 2);
#endif

		aItemSize += i10BoldFont->HeightInPixels();
	}

	aItemSize += i10NormalFont->FontMaxDescent() + 2;

	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudExplorerContainer::RenderListItems() {
	TBuf<256> aBuf;
	TInt aDrawPos = -iScrollbarHandlePosition;
	TInt aLevel = iExplorerLevels.Count() - 1;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
		TInt aItemSize = 0;
		
#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iExplorerLevels[aLevel]->iResultItems.Count(); i++) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[i];

			// Calculate item size
			aItemSize = CalculateItemSize(i);

			// Test to start the page drawing
			if(aDrawPos + aItemSize > 0) {
				TInt aItemDrawPos = aDrawPos;
				TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aResultItem->GetTitle()));
				
#ifdef __SERIES60_40__
				iListItems.Append(TListItem(i, TRect(0, aItemDrawPos, (Rect().Width() - iRightBarSpacer), (aItemDrawPos + aItemSize))));
#endif
				
				if(i == iSelectedItem) {
					RenderItemFrame(TRect(iLeftBarSpacer + 1, aItemDrawPos, (Rect().Width() - iRightBarSpacer), (aItemDrawPos + aItemSize)));
					
					iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 3, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 3), iRect.Height()));
					iBufferGc->SetPenColor(iColourTextSelected);

					// Avatar
					iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
					iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(aResultItem->GetIconId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aResultItem->GetIconId(), true, iIconMidmapSize), true);

					// Overlay
					if(aResultItem->GetOverlayId() != 0) {
						iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(aResultItem->GetOverlayId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aResultItem->GetOverlayId(), true, iIconMidmapSize), true);
					}
					
					iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

					// Title
					if(i13BoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - iRightBarSpacer)) < aDirectionalText.Length()) {
						iBufferGc->UseFont(i10BoldFont);
					}
					else {
						iBufferGc->UseFont(i13BoldFont);
					}

					aItemDrawPos += i13BoldFont->HeightInPixels();
					iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
					aItemDrawPos += i13BoldFont->FontMaxDescent();
					iBufferGc->DiscardFont();

					// Description
					if(aResultItem->GetDescription().Length() > 0) {
						iBufferGc->UseFont(i10ItalicFont);
						
						for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
							aItemDrawPos += i10ItalicFont->HeightInPixels();
							iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
						}
						
						iBufferGc->DiscardFont();
					}
					
					// Rank
					if(aResultItem->GetRank() > 0) {
						if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
							aItemDrawPos = (aDrawPos + iItemIconSize + 2);
						}						

						iBufferGc->UseFont(i10NormalFont);
						aItemDrawPos += i10NormalFont->HeightInPixels();
						aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedRank));
						aBuf.AppendFormat(_L(": %d"), aResultItem->GetRank());
						iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
						iBufferGc->DiscardFont();
					}
				}
				else {
					iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 0, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 1), iRect.Height()));
					iBufferGc->SetPenColor(iColourText);
#ifdef __SERIES60_40__
					aItemDrawPos += i10BoldFont->DescentInPixels();
#endif									
					// Avatar
					iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);				
					iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(aResultItem->GetIconId(), false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(aResultItem->GetIconId(), true, (iIconMidmapSize + 1)), true);
					
					// Overlay
					if(aResultItem->GetOverlayId() != 0) {
						iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(aResultItem->GetOverlayId(), false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(aResultItem->GetOverlayId(), true, (iIconMidmapSize + 1)), true);
					}
					
					iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

					// Name
					iBufferGc->UseFont(i10BoldFont);
					aItemDrawPos += i10BoldFont->HeightInPixels();
					iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
					iBufferGc->DiscardFont();
				}

				iBufferGc->CancelClippingRect();
			}
			
			aDrawPos += aItemSize + 1;

			// Finish page drawing
			if(aDrawPos > iRect.Height()) {
				break;
			}
		}
	}
	else {
		// Empty results
		aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * i10BoldFont->HeightInPixels()) / 2));
		
		iBufferGc->UseFont(i10BoldFont);
		iBufferGc->SetPenColor(iColourText);
		
		for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
			aDrawPos += i10ItalicFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2) - (i10BoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2))), aDrawPos));
		}
		
		iBufferGc->DiscardFont();

		iSelectedItem = KErrNotFound;
	}
}

void CBuddycloudExplorerContainer::RepositionItems(TBool aSnapToItem) {
	iTotalListSize = 0;
	
	// Enable snapping again
	if(aSnapToItem) {
		iSnapToItem = aSnapToItem;
		iScrollbarHandlePosition = 0;
	}
	
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
		TInt aItemSize;

		// Check if current item exists
		if(iSelectedItem >= iExplorerLevels[aLevel]->iResultItems.Count()) {
			iSelectedItem = KErrNotFound;
		}
		
		for(TInt i = 0; i < iExplorerLevels[aLevel]->iResultItems.Count(); i++) {
			if(iSelectedItem == KErrNotFound) {
				iSelectedItem = i;
				RenderWrappedText(iSelectedItem);
			}
			
			aItemSize = CalculateItemSize(i);
			
			if(aSnapToItem && i == iSelectedItem) {
				if(iTotalListSize + (aItemSize / 2) > (iRect.Height() / 2)) {
					iScrollbarHandlePosition = (iTotalListSize + (aItemSize / 2)) - (iRect.Height() / 2);
				}
			}
			
			// Increment total list size
			iTotalListSize += (aItemSize + 1);
		}
	}
	else {
		iSelectedItem = KErrNotFound;
	}
	
	CBuddycloudListComponent::RepositionItems(aSnapToItem);
}

void CBuddycloudExplorerContainer::HandleItemSelection(TInt aItemId) {
	TInt aLevel = iExplorerLevels.Count() - 1;

	if(iSelectedItem != aItemId) {
		// New item selected
		iSelectedItem = aItemId;
		
		// Update screen
		RenderWrappedText(iSelectedItem);		
		RepositionItems(true);	
		RenderScreen();
	}
	else {
		// Trigger item event		
		if(iExplorerLevels[aLevel]->iResultItems.Count()) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];

			if(aResultItem->GetResultType() == EExplorerItemDirectory || aResultItem->GetResultType() == EExplorerItemLink) {
				// Trigger open action
				HandleCommandL(EMenuSelectCommand);
			}
			else {
				// Show context menu
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_EXPLORER_ITEM_MENUBAR);
				iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_EXPLORER_OPTIONS_MENUBAR);
			}
		}
	}
}

void CBuddycloudExplorerContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	TInt aLevel = iExplorerLevels.Count() - 1;

	if(aResourceId == R_EXPLORER_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuConnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuRefreshCommand, true);
		aMenuPane->SetItemDimmed(EMenuSelectCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditChannelCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, true);
		aMenuPane->SetItemDimmed(EMenuDisconnectCommand, true);
		
		if(iBuddycloudLogic->GetState() == ELogicOnline) {			
			aMenuPane->SetItemDimmed(EMenuDisconnectCommand, false);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuConnectCommand, false);
		}
		
		if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];

			if(aResultItem->GetResultType() == EExplorerItemDirectory || aResultItem->GetResultType() == EExplorerItemLink) {
				aMenuPane->SetItemDimmed(EMenuSelectCommand, false);
			}
			else if(aResultItem->GetResultType() > EExplorerItemDirectory) {
				aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aResultItem->GetTitle().Left(32));
				aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
				aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, false);
				
				if(iExplorerState == EExplorerIdle) {
					aMenuPane->SetItemDimmed(EMenuRefreshCommand, false);
				}	
			}
			
			if(aResultItem->GetResultType() == EExplorerItemDirectory || aResultItem->GetResultType() == EExplorerItemChannel) {
				aMenuPane->SetItemDimmed(EMenuEditChannelCommand, false);
			}
		}
	}
	else if(aResourceId == R_EXPLORER_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuGetPlaceInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuGetPersonInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuUnfollowCommand, true);
		
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemPlace) {
				aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, false);
				
				if(aResultItem->GetDistance() < 1000) {
					// Allow set current if less than 1km
					aMenuPane->SetItemDimmed(EMenuSetAsPlaceCommand, false);
				}
				
				if(aResultItem->GetItemId() > 0 && 
						iBuddycloudLogic->GetPlaceStore()->GetItemById(aResultItem->GetItemId()) == NULL) {
					
					aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, false);
				}
			}
			else if(aResultItem->GetResultType() == EExplorerItemPerson || aResultItem->GetResultType() == EExplorerItemChannel) {
				TInt aFollowerId = iBuddycloudLogic->IsSubscribedTo(aResultItem->GetId(), EItemRoster|EItemChannel);
				
				if(aFollowerId > 0) {
					CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
					CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aFollowerId));
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						if(aItem->GetItemType() == EItemRoster) {
							aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
						}
						
						if(aItem->GetId().Length() > 0) {
							aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
						}
					}
					
					aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
				}
				else {
					aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
				}
			}
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
		
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemChannel) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
			}
			else if(aResultItem->GetResultType() == EExplorerItemPerson) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
				//aMenuPane->SetItemDimmed(EMenuSeePlacesCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
			}
			else if(aResultItem->GetResultType() == EExplorerItemPlace) {
				aMenuPane->SetItemDimmed(EMenuSeeBeenHereCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeGoingHereCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
			}			
		}
	}
}

void CBuddycloudExplorerContainer::HandleCommandL(TInt aCommand) {
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(aCommand == EMenuConnectCommand) {
		iBuddycloudLogic->ConnectL();
	}
	else if(aCommand == EMenuDisconnectCommand) {
		iBuddycloudLogic->Disconnect();
	}
	else if(aCommand == EMenuRefreshCommand) {
		RefreshLevelL();
	}
	else if(aCommand == EMenuSelectCommand) {
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemDirectory) {
				TExplorerQuery aQuery;		

				if(aResultItem->GetId().Compare(_L("local")) == 0) {
					CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
					
					if(aOwnItem) {
						aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(EExplorerItemPerson, iTextUtilities->UnicodeToUtf8L(aOwnItem->GetId()), 20);
						aQuery.iStanza.Insert((aQuery.iStanza.Length() - 15), _L8("<request var='channel'/>"));
					}
				}
				else {
					aQuery = CExplorerStanzaBuilder::BuildChannelsXmppStanza(iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()), EChannelAll, 20);
				}
				
				if(aQuery.iStanza.Length() > 0) {
					// Add language code
					TPtrC8 aLangCode = iTextUtilities->UnicodeToUtf8L(*iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE));
					aQuery.iStanza.Insert(3, _L8(" xml:lang=''"));
					aQuery.iStanza.Insert(14, aLangCode);
					CleanupStack::PopAndDestroy(); // aLangCode
					
					PushLevelL(aResultItem->GetTitle(), aQuery.iStanza);
				}
			}
			else if(aResultItem->GetResultType() == EExplorerItemLink) {
				CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
				aLauncher->LaunchBrowserWithLinkL(aResultItem->GetId());
				CleanupStack::PopAndDestroy(); // aLauncher
			}
		}
	}
	else if(aCommand == EMenuEditChannelCommand) {
		// Create new channel
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditChannelViewId));			
	}
	else if(aCommand == EMenuSetAsPlaceCommand || aCommand == EMenuSetAsNextPlaceCommand || aCommand == EMenuBookmarkPlaceCommand) {
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemPlace) {
				if(aCommand == EMenuSetAsPlaceCommand) {
					iBuddycloudLogic->SetCurrentPlaceL(aResultItem->GetItemId());
				}
				else if(aCommand == EMenuSetAsNextPlaceCommand) {
					iBuddycloudLogic->SetNextPlaceL(aResultItem->GetTitle(), aResultItem->GetItemId());
				}
				else if(aCommand == EMenuBookmarkPlaceCommand) {
					iBuddycloudLogic->EditMyPlacesL(aResultItem->GetItemId(), true);
				}
			}
		}
	}
	else if(aCommand == EMenuPrivateMessagesCommand || aCommand == EMenuChannelMessagesCommand || 
			aCommand == EMenuFollowCommand || aCommand == EMenuUnfollowCommand) {
		
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemPerson || aResultItem->GetResultType() == EExplorerItemChannel) {
				if(aCommand == EMenuFollowCommand) {
					if(aResultItem->GetResultType() == EExplorerItemPerson) {
						iBuddycloudLogic->FollowContactL(aResultItem->GetId());
					}
					else if(aResultItem->GetResultType() == EExplorerItemChannel) {
						// TODO: Follow explorer channel						
//						TInt aFollowingId = iBuddycloudLogic->FollowTopicChannelL(aResultItem->GetId(), aResultItem->GetTitle(), aResultItem->GetDescription());
//								
//						TMessagingViewObject aObject;			
//						aObject.iFollowerId = aFollowingId;
//						aObject.iTitle.Append(aResultItem->GetTitle());
//						aObject.iId.Append(aResultItem->GetId());
//						
//						TMessagingViewObjectPckg aObjectPckg(aObject);		
//						iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(aFollowingId), aObjectPckg);			
					}
				}
				else {
					TInt aFollowingId = iBuddycloudLogic->IsSubscribedTo(aResultItem->GetId(), EItemRoster|EItemChannel);
					
					if(aFollowingId > 0) {
						if(aCommand == EMenuUnfollowCommand) {
							CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
	
							if(aDialog->ExecuteLD(R_DELETE_DIALOG) != 0) {						
								iBuddycloudLogic->UnfollowItemL(aFollowingId);
							}
						}
						else {
							CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
							CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aFollowingId));
							
							if(aItem && aItem->GetItemType() >= EItemRoster) {														
								iXmppInterface->CancelXmppStanzaAcknowledge(this);
								
								TMessagingViewObject aObject;			
								aObject.iFollowerId = aFollowingId;
								aObject.iTitle.Append(aItem->GetTitle());
								aObject.iId.Append(aItem->GetId());
								
								if(aCommand == EMenuPrivateMessagesCommand) {
									CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
									aObject.iId.Append(aRosterItem->GetId());
								}
								
								TMessagingViewObjectPckg aObjectPckg(aObject);		
								iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(aFollowingId), aObjectPckg);			
							}
						}
					}
				}
			}
		}
	}
	else if(aCommand == EMenuSeeFollowingCommand || aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeProducingCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		
		if(aResultItem->GetResultType() == EExplorerItemPerson || aResultItem->GetResultType() == EExplorerItemChannel) {			
			TPtrC8 aEncId = iTextUtilities->UnicodeToUtf8L(aResultItem->GetId());			
			TExplorerQuery aQuery;
		
			if(aCommand == EMenuSeeFollowersCommand) {
				aQuery.iStanza.Append(_L8("<iq to='' type='get' id='exp_followers'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''/></pubsub></iq>"));
				
				if(aResultItem->GetResultType() == EExplorerItemChannel) {
					aQuery.iStanza.Insert(116, aEncId);
				}
				else if(aResultItem->GetResultType() == EExplorerItemPerson) {
					aQuery.iStanza.Insert(116, _L8("/user//channel"));
					aQuery.iStanza.Insert(122, aEncId);
				}
				
				aQuery.iStanza.Insert(8, KBuddycloudPubsubServer);
				
				iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_FOLLOWEDBY);
				aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, aResultItem->GetTitle().Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 7)));	
			}
			else {
				if(aCommand == EMenuSeeProducingCommand) {
					aQuery = CExplorerStanzaBuilder::BuildChannelsXmppStanza(aEncId, EChannelProducer);
					iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_ISPRODUCING);
				}
				else {
					aQuery = CExplorerStanzaBuilder::BuildChannelsXmppStanza(aEncId, EChannelAll);
					iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_ISFOLLOWING);
				}
				
				aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$USER")), 5, aResultItem->GetTitle().Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 5)));	
			}
			
			if(aQuery.iStanza.Length() > 0) {
				PushLevelL(aQuery.iTitle, aQuery.iStanza);
			}
		}
	}
	else if(aCommand == EMenuSeeNearbyCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		
		// Build stanza
		TExplorerQuery aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(aResultItem->GetResultType(), iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));

		iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO);
		aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, aResultItem->GetTitle().Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 7)));	
		
		PushLevelL(aQuery.iTitle, aQuery.iStanza);			
	}
	else if(aCommand == EMenuSeeBeenHereCommand || aCommand == EMenuSeeGoingHereCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		TExplorerQuery aQuery;
		
		if(aCommand == EMenuSeeBeenHereCommand) {
			aQuery = CExplorerStanzaBuilder::BuildPlaceVisitorsXmppStanza(_L8("past"), aResultItem->GetItemId());
			iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_WHOSBEENTO);
		}
		else {
			aQuery = CExplorerStanzaBuilder::BuildPlaceVisitorsXmppStanza(_L8("next"), aResultItem->GetItemId());
			iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_WHOSGOINGTO);
		}
		
		aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$PLACE")), 6, aResultItem->GetTitle().Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 6)));	
		
		PushLevelL(aQuery.iTitle, aQuery.iStanza);			
	}
	else if(aCommand == EAknSoftkeyBack) {
		if(aLevel > 0) {
			PopLevelL();
		}
		else {
			iXmppInterface->CancelXmppStanzaAcknowledge(this);
			iCoeEnv->AppUi()->ActivateViewL(iPrevViewId, iPrevViewMsgId, _L8(""));
		}
	}
}

void CBuddycloudExplorerContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KExplorerTab;
}

TKeyResponse CBuddycloudExplorerContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(iExplorerLevels.Count() > 0) {
			if(aKeyEvent.iCode == EKeyUpArrow) {
				if(iSelectedItem > 0) {
					HandleItemSelection(iSelectedItem - 1);
				}
	
				aResult = EKeyWasConsumed;
			}
			else if(aKeyEvent.iCode == EKeyDownArrow) {
				TInt aLevel = iExplorerLevels.Count() - 1;
				
				if(iSelectedItem < iExplorerLevels[aLevel]->iResultItems.Count() - 1) {
					HandleItemSelection(iSelectedItem + 1);
				}
	
				aResult = EKeyWasConsumed;
			}
			else if(aKeyEvent.iCode == 63557) { // Center Dpad
				HandleItemSelection(iSelectedItem);
				aResult = EKeyWasConsumed;
			}
		}
	}
	else if(aType == EEventKeyDown) {
		if(aKeyEvent.iScanCode == EStdKeyLeftArrow) {
			iXmppInterface->CancelXmppStanzaAcknowledge(this);
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelsViewId), TUid::Uid(0), _L8(""));
			aResult = EKeyWasConsumed;
		}
	}

	return aResult;
}

void CBuddycloudExplorerContainer::TabChangedL(TInt aIndex) {
	iXmppInterface->CancelXmppStanzaAcknowledge(this);
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), _L8(""));
}

void CBuddycloudExplorerContainer::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aAttributeType.Compare(_L8("result")) == 0) {
		if(aId.Compare(_L8("exp_nearby1")) == 0) {
			// Handle new nearby result
			while(aXmlParser->MoveToElement(_L8("item"))) {
				TPtrC pAttributeId = iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("id")));
				TPtrC8 pAttributeVar = aXmlParser->GetStringAttribute(_L8("var"));
				
				CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
				aResultItem->SetIdL(pAttributeId);
				
				// Set result type & avatar id
				if(pAttributeVar.Compare(_L8("person")) == 0) {
					aResultItem->SetResultType(EExplorerItemPerson);
					aResultItem->SetIconId(KIconPerson);
					
					HBufC* aJid = pAttributeId.AllocLC();
					TPtr pJid(aJid->Des());
					
					if(!iBuddycloudLogic->IsSubscribedTo(pJid, EItemRoster)) {
						// Sensor jid
						for(TInt i = (pJid.Locate('@') - 1), x = (i - 2); i >= 0 && i > x; i--) {
							pJid[i] = 46;
						}
					}
								
					aResultItem->SetTitleL(pJid);
					CleanupStack::PopAndDestroy(); // aJid
				}
				else if(pAttributeVar.Compare(_L8("channel")) == 0) {
					aResultItem->SetResultType(EExplorerItemChannel);
					aResultItem->SetIconId(KIconChannel);
				}
				else if(pAttributeVar.Compare(_L8("place")) == 0) {
					aResultItem->SetResultType(EExplorerItemPlace);
					aResultItem->SetIconId(KIconPlace);
					
					// Get place id
					TInt aIntValue = pAttributeId.LocateReverse('/');
					
					if(aIntValue != KErrNotFound) {
						// Convert place id
						TLex aLexValue(pAttributeId.Mid(aIntValue + 1));
						
						if(aLexValue.Val(aIntValue) == KErrNone) {
							aResultItem->SetItemId(aIntValue);
						}
					}
				}
				else if(pAttributeVar.Compare(_L8("link")) == 0 || 
						pAttributeVar.Compare(_L8("wiki")) == 0 || 
						pAttributeVar.Compare(_L8("advert")) == 0) {
					
					aResultItem->SetResultType(EExplorerItemLink);
					aResultItem->SetIconId(KIconLink);
				}
				
				CXmlParser* aItemXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
				
				if(aResultItem->GetTitle().Length() == 0 && aItemXmlParser->MoveToElement(_L8("name"))) {
					aResultItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData()));
				}
				
				if(aItemXmlParser->MoveToElement(_L8("description"))) {
					aResultItem->SetDescriptionL(iTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData()));
				}
				
				if(aItemXmlParser->MoveToElement(_L8("distance"))) {
					aResultItem->SetDistance(aItemXmlParser->GetIntData());
				}
				
				if(aItemXmlParser->MoveToElement(_L8("geoloc"))) {
					CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
					
					aResultItem->SetGeolocL(aGeolocParser->XmlToGeolocLC(aItemXmlParser->GetStringData()));
					aResultItem->UpdateFromGeolocL();
					CleanupStack::Pop(); // aGeoloc
					
					CleanupStack::PopAndDestroy(); // aGeolocParser
				}
				
				CleanupStack::PopAndDestroy(); // aItemXmlParser
				
				// Append sorted item
				iExplorerLevels[aLevel]->AppendSortedItem(aResultItem, ESortByDistance);
				CleanupStack::Pop(); // aResultItem
			}
		}
		else if(aId.Compare(_L8("exp_followers")) == 0) {
			// Handle place/channel users result
			while(aXmlParser->MoveToElement(_L8("affiliation"))) {
				CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
				aResultItem->SetResultType(EExplorerItemPerson);
				aResultItem->SetIconId(KIconPerson);
				aResultItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
				
				HBufC* aTitle = aResultItem->GetId().AllocLC();
				TPtr pTitle(aTitle->Des());
				
				if(!iBuddycloudLogic->IsSubscribedTo(pTitle, EItemRoster)) {
					// Sensor jid						
					for(TInt i = (pTitle.Locate('@') - 1), x = (i - 2); i >= 0 && i > x; i--) {
						pTitle[i] = 46;
					}
				}
							
				aResultItem->SetTitleL(pTitle);
				CleanupStack::PopAndDestroy(); // aTitle
				
				// Add item
				iExplorerLevels[aLevel]->AppendSortedItem(aResultItem);
				CleanupStack::Pop(); // aResultItem
			}
		}
		else if(aId.Compare(_L8("exp_channels1")) == 0) {
			// Handle channels result
			while(aXmlParser->MoveToElement(_L8("channel"))) {
				CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
				aResultItem->SetResultType(EExplorerItemChannel);
				aResultItem->SetIconId(KIconChannel);
				
				CXmlParser* aChannelXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
				
				do {
					TPtrC8 aElementName = aChannelXmlParser->GetElementName();
					TPtrC aElementData = iTextUtilities->Utf8ToUnicodeL(aChannelXmlParser->GetStringData());
					
					if(aElementName.Compare(_L8("jid")) == 0) {					
						aResultItem->SetIdL(aElementData);
					}
					else if(aElementName.Compare(_L8("title")) == 0) {					
						aResultItem->SetTitleL(aElementData);
					}
					else if(aElementName.Compare(_L8("description")) == 0) {					
						aResultItem->SetDescriptionL(aElementData);
					}
					else if(aElementName.Compare(_L8("rank")) == 0) {					
						aResultItem->SetRank(aChannelXmlParser->GetIntData());
					}
				} while(aChannelXmlParser->MoveToNextElement());
				
				CleanupStack::PopAndDestroy(); // aChannelXmlParser
				
				// Add item
				iExplorerLevels[aLevel]->AppendSortedItem(aResultItem);
				CleanupStack::Pop(); // aResultItem
			}		
		}
	}
		
	CleanupStack::PopAndDestroy(); // aXmlParser
	
	iQueryResultCount++;
		
	if(iQueryResultCount == iQueryResultSize) {
		iExplorerState = EExplorerIdle;
	}	
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(false);
	RenderScreen();
}

// End of File
