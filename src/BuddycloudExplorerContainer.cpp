/*
============================================================================
 Name        : BuddycloudExplorerContainer.cpp
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
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
#include "BuddycloudExplorerContainer.h"
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

void CBuddycloudExplorerContainer::ConstructL(const TRect& aRect, TViewReference aQueryReference) {
	iRect = aRect;
	iQueryReference = aQueryReference;
	
	CreateWindowL();

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
	
	SetRect(iRect);
	ActivateL();		
	
#ifdef __SERIES60_40__
	DynInitToolbarL(R_EXPLORER_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
	
	// Initialize explorer
	TViewData aQuery = iQueryReference.iNewViewData;
	
	if(!iQueryReference.iCallbackRequested) {
		iQueryReference.iCallbackViewId = KPlacesViewId;
		iQueryReference.iOldViewData.iId = 0;
	}
		
	if(aQuery.iData.Length() == 0) {
		// Query channels directories
		iCoeEnv->ReadResource(aQuery.iTitle, R_LOCALIZED_STRING_EXPLORERTAB_HINT);
		
		_LIT8(KMaitredDirectories, "<iq to='maitred.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/channels'><items var='directory'/></query></iq>\r\n");
		aQuery.iData.Format(KMaitredDirectories, EXmppIdGetDirectories, iBuddycloudLogic->GetNewIdStamp());

		// Add language code
		HBufC* aLangCode = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE);
		TPtrC8 aEncLangCode(iTextUtilities->UnicodeToUtf8L(*aLangCode));
		
		CExplorerStanzaBuilder::InsertXmlLanguageIntoStanza(aQuery.iData, aEncLangCode);
		CleanupStack::PopAndDestroy(); // aLangCode
	}

	PushLevelL(aQuery.iTitle, aQuery.iData);
}

CBuddycloudExplorerContainer::~CBuddycloudExplorerContainer() {
	if(iBuddycloudLogic) {
		iXmppInterface->CancelXmppStanzaAcknowledge(this);
	}

	// Tabs
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
	
	// Levels
	for(TInt i = 0; i < iExplorerLevels.Count(); i++) {
		delete iExplorerLevels[i];
	}
	
	iExplorerLevels.Close();
}

void CBuddycloudExplorerContainer::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationLocationUpdated) {
		TInt aLevel = iExplorerLevels.Count() - 1;

		if(iExplorerState < EExplorerRequesting && 
				iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() == 0) {			
			
			// Re-request explorer level results
			RefreshLevelL();
		}		
	}
#ifdef __SERIES60_40__
	else if(aEvent == ENotificationConnectivityChanged) {
		DynInitToolbarL(R_EXPLORER_TOOLBAR, iViewAccessor->ViewToolbar());
		
	}
#endif		
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
			iXmppInterface->SendAndAcknowledgeXmppStanza(aSlicedStanza.Left(aSearchResult), this, true, EXmppPriorityHigh);		
			aSlicedStanza.Set(aSlicedStanza.Mid(aSearchResult));
		}
		
		if(aSlicedStanza.Length() > 0) {
			iQueryResultSize++;
			iXmppInterface->SendAndAcknowledgeXmppStanza(aSlicedStanza, this, true, EXmppPriorityHigh);		
		}
		
		if(iQueryResultSize == 0) {
			iExplorerState = EExplorerNoItems;
		}
	}
}

void CBuddycloudExplorerContainer::PushLevelL(const TDesC& aTitle, const TDesC8& aStanza) {
	if(aStanza.Length() > 0) {
		iTimer->Stop();
		
		if(iExplorerLevels.Count() > 0) {
			TInt aLevel = iExplorerLevels.Count() - 1;
			iExplorerLevels[aLevel]->iSelectedResultItem = iSelectedItem;
			
			// Compress stored data
			iExplorerLevels[aLevel]->iResultItems.Compress();
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
		
		// Compress heap
		User::Heap().Compress();
		
		// Set selected item
		aLevel--;
		iSelectedItem = iExplorerLevels[aLevel]->iSelectedResultItem;
		
		// Set title
		SetTitleL(iExplorerLevels[aLevel]->GetQueryTitle());

		// Update screen
		RenderWrappedText(iSelectedItem);
		RepositionItems(true);
		RenderScreen();
	}
}

void CBuddycloudExplorerContainer::RefreshLevelL() {
	TInt aLevel = iExplorerLevels.Count() - 1;

	if(iExplorerState < EExplorerRequesting && aLevel >= 0) {
		if(iExplorerLevels[aLevel]->GetQueriedStanza().Length() > 0) {
			// Clear previous result data
			iExplorerLevels[aLevel]->ClearResultItems();
	        
	        // Compress heap
	        User::Heap().Compress();
			
			// Send query
			ParseAndSendXmppStanzasL(iExplorerLevels[aLevel]->GetQueriedStanza());
			
			// Update screen
			RenderWrappedText(iSelectedItem);
			RepositionItems(true);
			RenderScreen();
		}
	}
}

void CBuddycloudExplorerContainer::RequestMoreResultsL() {
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
		// Reset rsm allowed
		iExplorerLevels[aLevel]->SetRsmAllowed(false);
		
		TInt aLocate = iExplorerLevels[aLevel]->GetQueriedStanza().Find(_L8("</set>"));
		
		if(aLocate != KErrNotFound) {		
			TInt aLastItem = iExplorerLevels[aLevel]->iResultItems.Count() - 1;
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[aLastItem];
			TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));
			
			// Build query
			TBuf8<32> aRsmIndexElement;
			aRsmIndexElement.Format(_L8("<index>%d</index>"), iExplorerLevels[aLevel]->iResultItems.Count());
			
			_LIT8(KRsmAfterElement, "<after></after>");
			HBufC8* aNextResultsStanza = HBufC8::NewLC(iExplorerLevels[aLevel]->GetQueriedStanza().Length() + aRsmIndexElement.Length() + KRsmAfterElement().Length() + aEncId.Length());
			TPtr8 pNextResultsStanza(aNextResultsStanza->Des());
			pNextResultsStanza.Append(iExplorerLevels[aLevel]->GetQueriedStanza());
			pNextResultsStanza.Insert(aLocate, aRsmIndexElement);
			pNextResultsStanza.Insert(aLocate, KRsmAfterElement);
			pNextResultsStanza.Insert(aLocate + 7, aEncId);
			
			// Send query
			iQueryResultSize++;
			iExplorerState = EExplorerRequesting;
			iXmppInterface->SendAndAcknowledgeXmppStanza(pNextResultsStanza, this, true, EXmppPriorityHigh);		
			
			CleanupStack::PopAndDestroy(); // aNextResultsStanza
		}
	}	
}

void CBuddycloudExplorerContainer::RenderWrappedText(TInt aIndex) {
	// Clear
	ClearWrappedText();
	
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() > 0 && 
			aIndex >= 0 && aIndex < iExplorerLevels[aLevel]->iResultItems.Count()) {
			
		// Wrap
		iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryItalicFont, iExplorerLevels[aLevel]->iResultItems[aIndex]->GetDescription(), (iRect.Width() - iLeftBarSpacer - iRightBarSpacer - 10));
	}
	else {
		// Empty list
		if(iExplorerState > EExplorerIdle) {
			TInt aResourceId = R_LOCALIZED_STRING_NOTE_NORESULTSFOUND;
				
			if(iExplorerState == EExplorerRequesting) {
				aResourceId = R_LOCALIZED_STRING_NOTE_REQUESTING;
			}		
	
			HBufC* aEmptyList = iEikonEnv->AllocReadResourceLC(aResourceId);		
			iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryBoldFont, *aEmptyList, (iRect.Width() - 2 - iLeftBarSpacer - iRightBarSpacer));		
			CleanupStack::PopAndDestroy(); // aEmptyList
		}
	}
}

void CBuddycloudExplorerContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	if(!iLayoutMirrored) {
		iLeftBarSpacer += (iItemIconSize / 8);
	}
	else {
		iRightBarSpacer += (iItemIconSize / 8);
	}

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
		
		// Title
		aItemSize += iPrimaryBoldFont->HeightInPixels();
		aItemSize += iPrimarySmallFont->DescentInPixels();
		
		// Sub-title
		if(aResultItem->GetSubTitle().Length() > 0) {
			aItemSize += iPrimarySmallFont->HeightInPixels();
			aItemSize += iPrimarySmallFont->DescentInPixels();
		}
		
		// Description
		if(aResultItem->GetDescription().Length() > 0) {
			if(aItemSize < (iItemIconSize + 2)) {
				aItemSize = (iItemIconSize + 2);
			}
			
			aItemSize += (iWrappedTextArray.Count() * iSecondaryItalicFont->HeightInPixels());
		}
		
		// Statistics
		if(aResultItem->StatisticCount() > 0) {
			if(aItemSize < (iItemIconSize + 2)) {
				aItemSize = (iItemIconSize + 2);
			}

			aItemSize += iSecondaryItalicFont->FontMaxDescent();
			aItemSize += (aResultItem->StatisticCount() * iSecondaryFont->HeightInPixels());
		}
	}
	else {
		aMinimumSize = (iItemIconSize / 2) + 2;
#ifdef __SERIES60_40__
		aMinimumSize += (iSecondaryBoldFont->DescentInPixels() * 2);
		aItemSize += (iSecondaryBoldFont->DescentInPixels() * 2);
#endif

		aItemSize += iSecondaryBoldFont->HeightInPixels();
	}

	aItemSize += iSecondaryFont->FontMaxDescent() + 2;

	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudExplorerContainer::RenderListItems() {
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

				// Affiliation
				if(aResultItem->GetChannelAffiliation() >= EPubsubAffiliationPublisher) {
					if(aResultItem->GetChannelAffiliation() == EPubsubAffiliationOwner) {
						// Owner
						iBufferGc->SetBrushColor(TRgb(237, 88, 47));
						iBufferGc->SetPenColor(TRgb(237, 88, 47, 125));			
					}
					else if(aResultItem->GetChannelAffiliation() == EPubsubAffiliationModerator) {
						// Moderator
						iBufferGc->SetBrushColor(TRgb(246, 170, 44));
						iBufferGc->SetPenColor(TRgb(246, 170, 44, 125));	
					}
					else {
						// Publisher
						iBufferGc->SetBrushColor(TRgb(175, 175, 175));
						iBufferGc->SetPenColor(TRgb(175, 175, 175, 125));					
					}
					
					TRect aAffiliationBox(TRect(2, aItemDrawPos, iLeftBarSpacer - 1, (aItemDrawPos + aItemSize + 1)));
					
					if(iLayoutMirrored) {
						aAffiliationBox = TRect(iRightBarSpacer + 2, aItemDrawPos, iRect.Width() - 1, (aItemDrawPos + aItemSize + 1));
					}				
								
					iBufferGc->SetPenStyle(CGraphicsContext::ENullPen);
					iBufferGc->DrawRect(aAffiliationBox);
					iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
					iBufferGc->DrawLine(TPoint(aAffiliationBox.iTl.iX - 1, aAffiliationBox.iTl.iY), TPoint(aAffiliationBox.iTl.iX - 1, aAffiliationBox.iBr.iY));
					iBufferGc->DrawLine(TPoint(aAffiliationBox.iBr.iX, aAffiliationBox.iTl.iY), aAffiliationBox.iBr);
				}
				
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
					if(iPrimaryBoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - iRightBarSpacer)) < aDirectionalText.Length()) {
						iBufferGc->UseFont(iSecondaryBoldFont);
					}
					else {
						iBufferGc->UseFont(iPrimaryBoldFont);
					}

					aItemDrawPos += iPrimaryBoldFont->HeightInPixels();
					iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
					aItemDrawPos += iPrimarySmallFont->DescentInPixels();
					iBufferGc->DiscardFont();
					
					if(aResultItem->GetSubTitle().Length() > 0) {
						iBufferGc->UseFont(iSecondaryFont);
						aItemDrawPos += iPrimarySmallFont->HeightInPixels();
						iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aResultItem->GetSubTitle()), TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
						aItemDrawPos += iPrimarySmallFont->DescentInPixels();
						iBufferGc->DiscardFont();
					}
				
					// Move to under icon
					if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
						aItemDrawPos = (aDrawPos + iItemIconSize + 2);
					}

					// Description
					if(aResultItem->GetDescription().Length() > 0) {
						iBufferGc->UseFont(iSecondaryItalicFont);
						
						for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
							aItemDrawPos += iSecondaryItalicFont->HeightInPixels();
							iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + 5, aItemDrawPos));
						}
						
						iBufferGc->DiscardFont();
					}
					
					// Statistics
					if(aResultItem->StatisticCount() > 0) {		
						aItemDrawPos += iSecondaryItalicFont->FontMaxDescent();

						if(aResultItem->GetDescription().Length() > 0) {
							iBufferGc->SetPenColor(iColourTextSelectedTrans);
							iBufferGc->SetPenStyle(CGraphicsContext::EDashedPen);
							
							iBufferGc->DrawLine(TPoint(iLeftBarSpacer + 5, aItemDrawPos), TPoint((iRect.Width() - iRightBarSpacer - 5), aItemDrawPos));		
							
							iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
							iBufferGc->SetPenColor(iColourTextSelected);
						}
						
						iBufferGc->UseFont(iSecondaryFont);

						for(TInt i = 0; i < aResultItem->StatisticCount(); i++) {
							aItemDrawPos += iSecondaryFont->HeightInPixels();
							iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aResultItem->GetStatistic(i)), TPoint(iLeftBarSpacer + 5, aItemDrawPos));							
						}
						
						iBufferGc->DiscardFont();
					}
				}
				else {
					iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 0, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 1), iRect.Height()));
					iBufferGc->SetPenColor(iColourText);
#ifdef __SERIES60_40__
					aItemDrawPos += iSecondaryBoldFont->DescentInPixels();
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
					iBufferGc->UseFont(iSecondaryBoldFont);
					aItemDrawPos += iSecondaryBoldFont->HeightInPixels();
					iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
					iBufferGc->DiscardFont();
				}

				iBufferGc->CancelClippingRect();
			}
			
			aDrawPos += (aItemSize + 1);

			// Finish page drawing
			if(aDrawPos > iRect.Height()) {
				break;
			}
		}
		
		// Check if rsm call is needed
		if(iExplorerState == EExplorerIdle && iExplorerLevels[aLevel]->RsmAllowed() && 
				(iScrollbarHandlePosition > (iTotalListSize - (iRect.Height() * 3)))) {
				
			// Collect next set of results
			RequestMoreResultsL();
		}
	}
	else {
		// Empty results
		aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * iSecondaryBoldFont->HeightInPixels()) / 2));
		
		iBufferGc->UseFont(iSecondaryBoldFont);
		iBufferGc->SetPenColor(iColourText);
		
		for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
			aDrawPos += iSecondaryItalicFont->HeightInPixels();
			iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2) - (iSecondaryBoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2))), aDrawPos));
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

#ifdef __SERIES60_40__
void CBuddycloudExplorerContainer::DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar) {
	if(aResourceId == R_EXPLORER_TOOLBAR) {
		aToolbar->SetItemDimmed(EMenuEditChannelCommand, true, true);
		
		if(iBuddycloudLogic->GetState() == ELogicOnline) {
			aToolbar->SetItemDimmed(EMenuEditChannelCommand, false, true);
		}
	}	
}
#endif

void CBuddycloudExplorerContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	TInt aLevel = iExplorerLevels.Count() - 1;

	if(aResourceId == R_EXPLORER_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuConnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuRefreshCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditChannelCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuDisconnectCommand, true);
		
		if(iBuddycloudLogic->GetState() == ELogicOnline) {			
			aMenuPane->SetItemDimmed(EMenuDisconnectCommand, false);
		
			if(iExplorerLevels.Count() > 0) {
				if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
					CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
					
					if(aResultItem->GetResultType() >= EExplorerItemPlace && aResultItem->GetResultType() <= EExplorerItemChannel) {
						aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aResultItem->GetTitle().Left(32));
						aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
					}
					
					if(aResultItem->GetResultType() == EExplorerItemDirectory || aResultItem->GetResultType() == EExplorerItemChannel) {
						aMenuPane->SetItemDimmed(EMenuEditChannelCommand, false);
					}
				}
				
				if(iExplorerState < EExplorerRequesting && iExplorerLevels[aLevel]->GetQueriedStanza().Length() > 0) {
					aMenuPane->SetItemDimmed(EMenuRefreshCommand, false);
				}	
			}
		}
		else {
			aMenuPane->SetItemDimmed(EMenuConnectCommand, false);
		}
	}
	else if(aResourceId == R_EXPLORER_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeModeratingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, true);
		aMenuPane->SetItemDimmed(EMenuChangePermissionCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuUnfollowCommand, true);
		
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemPlace) {
				aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
				
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
				
				if(aResultItem->GetResultType() == EExplorerItemPerson) {
					CFollowingChannelItem* aChannelItem = iExplorerLevels[aLevel]->GetQueriedChannel();
					
					if(aChannelItem && aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
							aResultItem->GetChannelAffiliation() < aChannelItem->GetPubsubAffiliation()) {
						
						// Allow channel moderation
						aMenuPane->SetItemDimmed(EMenuChangePermissionCommand, false);
					}
					
					aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
					aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, false);
					aMenuPane->SetItemDimmed(EMenuSeeModeratingCommand, false);
					aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, false);
					aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
				}
				else {
					aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
					aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, false);
				}
				
				if(aFollowerId > 0) {
					CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
					CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aFollowerId));
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						if(aItem->GetItemType() == EItemRoster) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
							
							if(!aRosterItem->OwnItem()) {
								aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
							}
							
							aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
						}
						else {
							CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
							
							if(aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationOwner) {
								aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
							}
						}
						
						if(aItem->GetId().Length() > 0) {
							aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
						}
					}
				}
				else {
					aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
					aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
				}
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
				TPtrC8 aEncUsername(iTextUtilities->UnicodeToUtf8L(iBuddycloudLogic->GetDescSetting(ESettingItemUsername)));
				
				TViewData aQuery;	

				if(aResultItem->GetId().Compare(_L("nearbyobjects")) == 0) {
					CExplorerStanzaBuilder::FormatButlerXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncUsername);
				}
				else {
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()), _L8("directory"));
					CExplorerStanzaBuilder::InsertResultSetIntoStanza(aQuery.iData, _L8("30"));
				}
				
				if(aQuery.iData.Length() > 0) {
					// Add language code
					HBufC* aLangCode = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE);
					TPtrC8 aEncLangCode(iTextUtilities->UnicodeToUtf8L(*aLangCode));
					
					CExplorerStanzaBuilder::InsertXmlLanguageIntoStanza(aQuery.iData, aEncLangCode);
					CleanupStack::PopAndDestroy(); // aLangCode
					
					// Push
					PushLevelL(aResultItem->GetTitle(), aQuery.iData);
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
		TViewReferenceBuf aViewReference;	
		aViewReference().iCallbackViewId = KExplorerViewId;
		aViewReference().iOldViewData = iQueryReference.iNewViewData;

		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditChannelViewId), TUid::Uid(0), aViewReference);					
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
	else if(aCommand == EMenuChannelInfoCommand) {
		if(iExplorerLevels[aLevel]->iResultItems.Count() > 0) {
			CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
			
			if(aResultItem->GetResultType() == EExplorerItemPerson || aResultItem->GetResultType() == EExplorerItemChannel) {
				TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));	
				
				TViewReferenceBuf aViewReference;	
				aViewReference().iCallbackRequested = true;
				aViewReference().iCallbackViewId = KExplorerViewId;
				aViewReference().iOldViewData.iId = iSelectedItem;
				aViewReference().iOldViewData.iTitle = iExplorerLevels[aLevel]->GetQueryTitle();
				aViewReference().iOldViewData.iData = iExplorerLevels[aLevel]->GetQueriedStanza();
				aViewReference().iNewViewData.iTitle.Copy(aResultItem->GetTitle());
				aViewReference().iNewViewData.iData.Copy(aEncId);
				
				if(aResultItem->GetResultType() == EExplorerItemPerson) {
					_LIT8(KUserNode, "/user//channel");
					
					HBufC8* aEncNodeId = HBufC8::NewLC(KUserNode().Length() + aEncId.Length());
					TPtr8 pEncNodeId(aEncNodeId->Des());
					pEncNodeId.Append(KUserNode);
					pEncNodeId.Insert(6, aEncId);
					
					aViewReference().iNewViewData.iData.Copy(pEncNodeId);
					
					CleanupStack::PopAndDestroy(); // aEncNodeId
				}
	
				iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelInfoViewId), TUid::Uid(0), aViewReference);					
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
						// Follow channel						
						TInt aFollowingId = iBuddycloudLogic->FollowChannelL(aResultItem->GetId());
								
						TViewReferenceBuf aViewReference;	
						aViewReference().iNewViewData.iId = aFollowingId;
						aViewReference().iNewViewData.iTitle.Copy(aResultItem->GetTitle());
						aViewReference().iNewViewData.iData.Copy(aResultItem->GetId());

						iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(aFollowingId), aViewReference);					
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
							CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
							CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aFollowingId));
							
							if(aItem && aItem->GetItemType() >= EItemRoster) {														
								iXmppInterface->CancelXmppStanzaAcknowledge(this);
								
								TViewReferenceBuf aViewReference;	
								aViewReference().iNewViewData.iId = aFollowingId;
								aViewReference().iNewViewData.iTitle.Copy(aItem->GetTitle());
								aViewReference().iNewViewData.iData.Copy(aItem->GetId());
								
								if(aCommand == EMenuPrivateMessagesCommand) {
									CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
									aViewReference().iNewViewData.iData.Copy(aRosterItem->GetId());
								}

								iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(aFollowingId), aViewReference);					
							}
						}
					}
				}
			}
		}
	}
	else if(aCommand == EMenuChangePermissionCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		CFollowingChannelItem* aChannelItem = iExplorerLevels[aLevel]->GetQueriedChannel();
		
		if(aChannelItem && aResultItem->GetResultType() == EExplorerItemPerson) {
			aResultItem->SetChannelAffiliation(iBuddycloudLogic->ShowAffiliationDialogL(aChannelItem->GetId(), aResultItem->GetId(), aResultItem->GetChannelAffiliation()));		
			aResultItem->SetOverlayId(aResultItem->GetChannelAffiliation() != EPubsubAffiliationOutcast ? 0 : KOverlayLocked);
			
			RenderScreen();
		}
	}
	else if(aCommand == EMenuSeeFollowingCommand || aCommand == EMenuSeeModeratingCommand || aCommand == EMenuSeeProducingCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		
		if(aResultItem->GetResultType() == EExplorerItemPerson) {
			TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));			
			TInt aResourceId = KErrNotFound;
			TViewData aQuery;
			
			if(aCommand == EMenuSeeProducingCommand) {
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("owner"));
				aResourceId = R_LOCALIZED_STRING_TITLE_ISPRODUCING;
			}
			else if(aCommand == EMenuSeeModeratingCommand) {
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("moderator"));
				aResourceId = R_LOCALIZED_STRING_TITLE_ISMODERATING;
			}
			else {
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("publisher"));
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("member"));
				aResourceId = R_LOCALIZED_STRING_TITLE_ISFOLLOWING;
			}
			
			if(aQuery.iData.Length() > 0) {
				CExplorerStanzaBuilder::BuildTitleFromResource(aQuery.iTitle, aResourceId, _L("$USER"), aResultItem->GetTitle());
				
				PushLevelL(aQuery.iTitle, aQuery.iData);
			}
		}
	}
	else if(aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeModeratorsCommand) {		
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));	
		TInt aResourceId = KErrNotFound;
		TViewData aQuery;
		
		if(aResultItem->GetResultType() == EExplorerItemPerson) {
			_LIT8(KUserNode, "/user//channel");
			
			HBufC8* aEncNodeId = HBufC8::NewLC(KUserNode().Length() + aEncId.Length());
			TPtr8 pEncNodeId(aEncNodeId->Des());
			pEncNodeId.Append(KUserNode);
			pEncNodeId.Insert(6, aEncId);
			
			if(aCommand == EMenuSeeModeratorsCommand) {
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), pEncNodeId, _L8("owner"));
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), pEncNodeId, _L8("moderator"));
				aResourceId = R_LOCALIZED_STRING_TITLE_MODERATEDBY;
			}
			else {
				CExplorerStanzaBuilder::FormatBroadcasterXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), pEncNodeId);
                CExplorerStanzaBuilder::InsertResultSetIntoStanza(aQuery.iData, _L8("50"));
				aResourceId = R_LOCALIZED_STRING_TITLE_FOLLOWEDBY;
			}	
			
			CleanupStack::PopAndDestroy(); // aEncNodeId
		}
		else if(aResultItem->GetResultType() == EExplorerItemChannel) {			
			if(aCommand == EMenuSeeModeratorsCommand) {
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("owner"));
				CExplorerStanzaBuilder::AppendMaitredXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("moderator"));
				aResourceId = R_LOCALIZED_STRING_TITLE_MODERATEDBY;
			}
			else {
				CExplorerStanzaBuilder::FormatBroadcasterXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId);
	            CExplorerStanzaBuilder::InsertResultSetIntoStanza(aQuery.iData, _L8("50"));
				aResourceId = R_LOCALIZED_STRING_TITLE_FOLLOWEDBY;
			}	
		}
		
		if(aQuery.iData.Length() > 0) {
			CExplorerStanzaBuilder::BuildTitleFromResource(aQuery.iTitle, aResourceId, _L("$OBJECT"), aResultItem->GetTitle());
			
			PushLevelL(aQuery.iTitle, aQuery.iData);
		}
	}
	else if(aCommand == EMenuSeeNearbyCommand) {
		CExplorerResultItem* aResultItem = iExplorerLevels[aLevel]->iResultItems[iSelectedItem];
		TViewData aQuery;
		
		// Build stanza
		if(aResultItem->GetResultType() == EExplorerItemPerson) {
			CExplorerStanzaBuilder::FormatButlerXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), iTextUtilities->UnicodeToUtf8L(aResultItem->GetId()));
		}
		else if(aResultItem->GetResultType() == EExplorerItemPlace) {
			CExplorerStanzaBuilder::FormatButlerXmppStanza(aQuery.iData, iBuddycloudLogic->GetNewIdStamp(), aResultItem->GetGeoloc()->GetReal(EGeolocLongitude), aResultItem->GetGeoloc()->GetReal(EGeolocLatitude));
		}
		
		if(aQuery.iData.Length() > 0) {
			CExplorerStanzaBuilder::BuildTitleFromResource(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO, _L("$OBJECT"), aResultItem->GetTitle());
			
			PushLevelL(aQuery.iTitle, aQuery.iData);
		}
	}
	else if(aCommand == EAknSoftkeyBack) {
		if(aLevel > 0) {
			PopLevelL();
		}
		else {
			iXmppInterface->CancelXmppStanzaAcknowledge(this);
			
			TViewReferenceBuf aViewReference;
			aViewReference().iCallbackViewId = KExplorerViewId;
			aViewReference().iOldViewData = iQueryReference.iNewViewData;
			
			if(iQueryReference.iCallbackRequested) {
				aViewReference().iNewViewData = iQueryReference.iOldViewData;
			}
			
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), iQueryReference.iCallbackViewId), TUid::Uid(iQueryReference.iOldViewData.iId), aViewReference);
		}
	}
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
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPlacesViewId), TUid::Uid(0), KNullDesC8);
			aResult = EKeyWasConsumed;
		}
	}

	return aResult;
}

void CBuddycloudExplorerContainer::TabChangedL(TInt aIndex) {
	iXmppInterface->CancelXmppStanzaAcknowledge(this);
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), KNullDesC8);
}

void CBuddycloudExplorerContainer::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
	TInt aLevel = iExplorerLevels.Count() - 1;
	
	if(aId.Locate(':') != KErrNotFound) {
		TLex8 aIdLex(aId.Left(aId.Locate(':')));
		TInt aIdEnum = KErrNotFound;
		
		if(aIdLex.Val(aIdEnum) != KErrNotFound) {
			CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
			TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
			
			if(aIdEnum == EXmppIdGetDirectories) {
				// Add nearby
				CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
				aResultItem->SetResultType(EExplorerItemDirectory);
				aResultItem->SetIconId(KIconChannel);
				aResultItem->SetOverlayId(KOverlayDirectory);
				aResultItem->SetIdL(_L("nearbyobjects"));
				
				HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_LIST_NEARBYTOYOU);
				aResultItem->SetTitleL(*aTitle);	
				CleanupStack::PopAndDestroy(); // aTitle

				iExplorerLevels[0]->iResultItems.Append(aResultItem);
				CleanupStack::Pop(); // aResultItem
			}
			
			if(aAttributeType.Compare(_L8("result")) == 0) {
				CFollowingChannelItem* aChannelItem = NULL;
				
				if(aIdEnum == EXmppIdGetDirectories) {
					// Handle directory result
					while(aXmlParser->MoveToElement(_L8("item"))) {
						CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
						aResultItem->SetResultType(EExplorerItemDirectory);
						aResultItem->SetIconId(KIconChannel);
						aResultItem->SetOverlayId(KOverlayDirectory);

						CXmlParser* aItemXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
						
						do {
							TPtrC8 aElementName = aItemXmlParser->GetElementName();
							TPtrC aElementData = iTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData());
							
							if(aElementName.Compare(_L8("id")) == 0) {					
								aResultItem->SetIdL(aElementData);
								
								// Localise directory title
								TInt aResourceId = KErrNotFound;
								
								if(aElementData.Compare(_L("featured")) == 0) {
									aResourceId = R_LOCALIZED_STRING_LIST_FEATUREDCHANNELS;
								}
								else if(aElementData.Compare(_L("social")) == 0) {
									aResourceId = R_LOCALIZED_STRING_LIST_FOLLOWERSFAVOURITES;
								}
								else if(aElementData.Compare(_L("popular")) == 0) {
									aResourceId = R_LOCALIZED_STRING_LIST_MOSTPOPULARCHANNELS;
								}
								
								if(aResourceId != KErrNotFound) {
									HBufC* aTitle = iEikonEnv->AllocReadResourceLC(aResourceId);
									aResultItem->SetTitleL(*aTitle);	
									CleanupStack::PopAndDestroy(); // aTitle
								}
								else {
									aResultItem->SetTitleL(aElementData);
								}
							}
							else if(aElementName.Compare(_L8("title")) == 0) {		
								if(aResultItem->GetId().Compare(aResultItem->GetTitle()) == 0) {
									aResultItem->SetTitleL(aElementData);
								}
							}
							else if(aElementName.Compare(_L8("description")) == 0) {		
								aResultItem->SetDescriptionL(aElementData);
							}
						} while(aItemXmlParser->MoveToNextElement());
						
						CleanupStack::PopAndDestroy(); // aItemXmlParser
						
						// Add item
						iExplorerLevels[0]->iResultItems.Append(aResultItem);
						CleanupStack::Pop(); // aResultItem
					}
				}
				else if(aIdEnum == EXmppIdGetNodeAffiliations) {
					if(aXmlParser->MoveToElement(_L8("affiliations"))) {
						// Set queried channel
						TPtrC aEncNodeId = iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("node")));
						TInt aFollowingId = iBuddycloudLogic->IsSubscribedTo(aEncNodeId, EItemChannel);
						
						if(aFollowingId > 0) {
							aChannelItem = static_cast <CFollowingChannelItem*> (iBuddycloudLogic->GetFollowingStore()->GetItemById(aFollowingId));
							
							iExplorerLevels[aLevel]->SetQueriedChannel(aChannelItem);	
						}
					
						// Handle affiliation items
						while(aXmlParser->MoveToElement(_L8("affiliation"))) {
							TPtrC aEncAttributeJid(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
							
							CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
							aResultItem->SetResultType(EExplorerItemPerson);
							aResultItem->SetIconId(KIconPerson);
							aResultItem->SetIdL(aEncAttributeJid);
							aResultItem->SetTitleL(aEncAttributeJid);
							
							// Set items affiliation
							TPtrC8 aAttributeAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation")));
							aResultItem->SetChannelAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aAttributeAffiliation));
							aResultItem->SetOverlayId(aResultItem->GetChannelAffiliation() != EPubsubAffiliationOutcast ? 0 : KOverlayLocked);
							
							// Sensor jid when not following or not a channel moderator
							if(aChannelItem == NULL || aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationModerator) {
								HBufC* aSensoredTitle = aResultItem->GetId().AllocLC();
								TPtr pSensoredTitle(aSensoredTitle->Des());
								
								if(!iBuddycloudLogic->IsSubscribedTo(pSensoredTitle, EItemRoster)) {
									// Sensor jid						
									for(TInt i = (pSensoredTitle.Locate('@') - 1), x = (i - 2); i > 1 && i > x; i--) {
										pSensoredTitle[i] = 46;
									}
								}
											
								aResultItem->SetTitleL(pSensoredTitle);
								CleanupStack::PopAndDestroy(); // aSensoredTitle
							}
							
							// Add item
							iExplorerLevels[aLevel]->AppendSortedItem(aResultItem);
							CleanupStack::Pop(); // aResultItem
						}
					}
				}
				else if(aIdEnum == EXmppIdGetMaitredList || aIdEnum == EXmppIdGetNearbyObjects) {
					if(aXmlParser->MoveToElement(_L8("items"))) {
						// Set queried channel
						if(aIdEnum == EXmppIdGetMaitredList) {
							TPtrC aEncItemsId = iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("id")));
							TInt aFollowingId = iBuddycloudLogic->IsSubscribedTo(aEncItemsId, EItemChannel);
							
							if(aFollowingId > 0) {
								aChannelItem = static_cast <CFollowingChannelItem*> (iBuddycloudLogic->GetFollowingStore()->GetItemById(aFollowingId));
								
								iExplorerLevels[aLevel]->SetQueriedChannel(aChannelItem);	
							}
						}
						
						TPtrC8 aItemsVar = aXmlParser->GetStringAttribute(_L8("var"));

						// Handle items
						while(aXmlParser->MoveToElement(_L8("item"))) {
							CExplorerResultItem* aResultItem = CExplorerResultItem::NewLC();
							TPtrC8 aAttributeVar = aXmlParser->GetStringAttribute(_L8("var"));
							
							// Get the item var
							if(aIdEnum == EXmppIdGetNearbyObjects && aAttributeVar.Length() > 0) {
								aResultItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("id"))));
								
								// Set result type & icon id
								if(aAttributeVar.Compare(_L8("channel")) == 0) {
									aResultItem->SetResultType(EExplorerItemChannel);
									aResultItem->SetIconId(KIconChannel);
								}
								else if(aAttributeVar.Compare(_L8("place")) == 0) {
									aResultItem->SetResultType(EExplorerItemPlace);
									aResultItem->SetIconId(KIconPlace);
									
									// Get place id
									TPtrC aPlaceUri(aResultItem->GetId());
									TInt aIntValue = aPlaceUri.LocateReverse('/');
									
									if(aIntValue != KErrNotFound) {
										// Convert place id
										TLex aLexValue(aPlaceUri.Mid(aIntValue + 1));
										
										if(aLexValue.Val(aIntValue) == KErrNone) {
											aResultItem->SetItemId(aIntValue);
										}
									}
								}
								else if(aAttributeVar.Compare(_L8("link")) == 0 || 
										aAttributeVar.Compare(_L8("wiki")) == 0 || 
										aAttributeVar.Compare(_L8("advert")) == 0) {
									
									aResultItem->SetResultType(EExplorerItemLink);
									aResultItem->SetIconId(KIconLink);
								}
							}
							
							// Parse the items data
							CXmlParser* aItemXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
							
							do {
								TPtrC8 aElementName = aItemXmlParser->GetElementName();
								TPtrC aElementData = iTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData());
								
								if(aIdEnum == EXmppIdGetMaitredList && aElementName.Compare(_L8("id")) == 0) {					
									aResultItem->SetIdL(aElementData);
									
									if(aResultItem->GetTitle().Length() == 0) {
										// Set title if not already set
										aResultItem->SetTitleL(aElementData);
									}
									
									if(aElementData.Find(_L("/channel")) != KErrNotFound) {
										// Set as a channel result type
										aResultItem->SetResultType(EExplorerItemChannel);
										aResultItem->SetIconId(KIconChannel);
										
										if(aItemsVar.Length() > 0 && aItemsVar.Compare(_L8("directory")) != 0) {
										    // Set channel affiliation
										    aResultItem->SetChannelAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aItemsVar));
										}
									}
								}
								else if((aIdEnum == EXmppIdGetMaitredList && aElementName.Compare(_L8("title")) == 0) || 
										(aIdEnum == EXmppIdGetNearbyObjects && aElementName.Compare(_L8("name")) == 0)) {					
									
									aResultItem->SetTitleL(aElementData);
								}
								else if(aIdEnum == EXmppIdGetMaitredList && aElementName.Compare(_L8("affiliation")) == 0) {
									aResultItem->SetChannelAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aItemXmlParser->GetStringData()));
								}				
								else if(aElementName.Compare(_L8("description")) == 0) {					
									aResultItem->SetDescriptionL(aElementData);
								}
								else if(aElementName.Compare(_L8("distance")) == 0) {
									aResultItem->SetDistance(aItemXmlParser->GetIntData());
								}				
								else if(aElementName.Compare(_L8("rank")) == 0) {	
									HBufC* aLocalizedRank = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CHANNELRANK);
									HBufC* aRankStatistic = HBufC::NewLC(aLocalizedRank->Des().Length() + 32);
									TPtr pRankStatistic(aRankStatistic->Des());
									pRankStatistic.Append(*aLocalizedRank);
									pRankStatistic.AppendFormat(_L(" : %d"), aItemXmlParser->GetIntData(0));
									
									aResultItem->AddStatisticL(pRankStatistic);
									CleanupStack::PopAndDestroy(2); // aRankStatistic, aLocalizedRank
								}
								else if(aItemXmlParser->MoveToElement(_L8("geoloc"))) {
									CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
									
									aResultItem->SetGeolocL(aGeolocParser->XmlToGeolocLC(aItemXmlParser->GetStringData()));
									aResultItem->UpdateFromGeolocL();
									CleanupStack::Pop(); // aGeoloc
									
									CleanupStack::PopAndDestroy(); // aGeolocParser
								}
							} while(aItemXmlParser->MoveToNextElement());
							
							CleanupStack::PopAndDestroy(); // aItemXmlParser
							
							// Sensor person jid when not following or not a channel moderator
							if(aResultItem->GetResultType() == EExplorerItemPerson && 
									(aChannelItem == NULL || aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationModerator)) {
								
								HBufC* aJid = aResultItem->GetId().AllocLC();
								TPtr pJid(aJid->Des());
								
								if(!iBuddycloudLogic->IsSubscribedTo(pJid, EItemRoster)) {
									// Sensor jid
									for(TInt i = (pJid.Locate('@') - 1), x = (i - 2); i > 1 && i > x; i--) {
										pJid[i] = 46;
									}
								}
											
								aResultItem->SetTitleL(pJid);
								CleanupStack::PopAndDestroy(); // aJid
							}
							else if(aResultItem->GetResultType() == EExplorerItemChannel) {
								// Set sub title
								TInt aLocate = aResultItem->GetId().LocateReverse('/');
								
								if(aLocate != KErrNotFound) {
									HBufC* aIdText = HBufC::NewLC(aResultItem->GetId().Length() - aLocate);
									TPtr pIdText(aIdText->Des());
									pIdText.Append('#');
									pIdText.Append(aResultItem->GetId().Mid(aLocate + 1));
									
									aResultItem->SetSubTitleL(pIdText);
									CleanupStack::PopAndDestroy(); // aIdText	
								}
								else {
									// TODO: Is an OG nearby channel workaround
									aResultItem->SetIdL(KNullDesC);
								}
							}
							
							if(aResultItem->GetId().Length() > 0) {
								// Add item
								if(aIdEnum == EXmppIdGetNearbyObjects) {
									iExplorerLevels[aLevel]->AppendSortedItem(aResultItem, ESortByDistance);
								}
								else {
									iExplorerLevels[aLevel]->AppendSortedItem(aResultItem);							
								}
								
								CleanupStack::Pop(); // aResultItem
							}
							else {
								CleanupStack::PopAndDestroy(); // aResultItem	
							}
						}
					}					
				}
				
				// Result set management
				iExplorerLevels[aLevel]->SetRsmAllowed(false);
				
				if(aXmlParser->MoveToElement(_L8("set"))) {
					if(aXmlParser->MoveToElement(_L8("count"))) {
						TInt aTotalCount = aXmlParser->GetIntData();
						
						if(iExplorerLevels[aLevel]->iResultItems.Count() < aTotalCount) {
							// More items are still available, enable rsm
							iExplorerLevels[aLevel]->SetRsmAllowed(true);
						}
					}
				}
			}
			
			CleanupStack::PopAndDestroy(); // aXmlParser
		}
	}
	
	iQueryResultCount++;
		
	if(iQueryResultCount == iQueryResultSize) {
		if(iExplorerLevels.Count() > 0 && iExplorerLevels[aLevel]->iResultItems.Count() == 0) {
			iExplorerState = EExplorerNoItems;
		}
		else {
			iExplorerState = EExplorerIdle;
		}
	}	
	
	// Update screen
	RepositionItems(false);
	RenderWrappedText(iSelectedItem);
	RenderScreen();
}

// End of File
