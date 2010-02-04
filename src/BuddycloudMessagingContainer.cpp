/*
============================================================================
 Name        : BuddycloudMessagingContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Messaging Container
============================================================================
*/

// INCLUDE FILES
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h> 
#include <aknquerydialog.h>
#include <eikmenup.h>
#include <baclipb.h>
#include <barsread.h>
#include <txtetext.h>
#include <Buddycloud_lang.rsg>
#include "BrowserLauncher.h"
#include "BuddycloudExplorer.h"
#include "BuddycloudMessagingContainer.h"
#include "ViewReference.h"

/*
----------------------------------------------------------------------------
--
-- CTextWrappedEntry
--
----------------------------------------------------------------------------
*/

CTextWrappedEntry* CTextWrappedEntry::NewLC(CAtomEntryData* aEntry, TBool aComment) {
	CTextWrappedEntry* self = new(ELeave) CTextWrappedEntry(aEntry, aComment);
	CleanupStack::PushL(self);
	return self;
}

CTextWrappedEntry::CTextWrappedEntry(CAtomEntryData* aEntry, TBool aComment) {
	iEntry = aEntry;
	iComment = aComment;
	iRead = iEntry->Read();
}

CTextWrappedEntry::~CTextWrappedEntry() {
	ResetLines();
	
	if(iEntry && !iEntry->Read() && iRead) {
		iEntry->SetRead(iRead);
	}
	
	iLines.Close();
}

CAtomEntryData* CTextWrappedEntry::GetEntry() {
	return iEntry;
}

TBool CTextWrappedEntry::Comment() {
	return iComment;
}

TBool CTextWrappedEntry::Read() {
	return iRead;
}

void CTextWrappedEntry::SetRead(TBool aRead) {
	iRead = aRead;
}

TInt CTextWrappedEntry::LineCount() {
	return iLines.Count();
}

TDesC& CTextWrappedEntry::GetLine(TInt aIndex) {
	return *iLines[aIndex];
}

void CTextWrappedEntry::ResetLines() {
	for(TInt x = 0; x < iLines.Count(); x++) {
		if(iLines[x])
			delete iLines[x];
	}

	iLines.Reset();
}

/*
----------------------------------------------------------------------------
--
-- CBuddycloudMessagingContainer
--
----------------------------------------------------------------------------
*/

CBuddycloudMessagingContainer::CBuddycloudMessagingContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic) : 
		CBuddycloudListComponent(aViewAccessor, aBuddycloudLogic) {
}

void CBuddycloudMessagingContainer::ConstructL(const TRect& aRect, TViewData aQueryData) {
	iRect = aRect;
	iMessagingObject = aQueryData;
	CreateWindowL();
	
	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	InitializeMessageDataL();
		
	iShowMialog = false;
	iRendering = false;
	
	RenderScreen();
	ActivateL();
	
#ifdef __SERIES60_40__
		DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif
}

CBuddycloudMessagingContainer::~CBuddycloudMessagingContainer() {
	if(iDiscussion) {
		iDiscussion->SetDiscussionUpdateObserver(NULL);
	}
	
	if(iMessagingId) {
		delete iMessagingId;
	}

	// Delete entries
	for(TInt i = 0; i < iEntries.Count(); i++) {
		if(iEntries[i]) {
			delete iEntries[i];
		}
	}

	iEntries.Close();
	
#ifdef __SERIES60_40__
	ResetItemLinks();
	iItemLinks.Close();
#endif
}

void CBuddycloudMessagingContainer::InitializeMessageDataL() {
	// Destroy old message data
	if(iDiscussion) {
		iDiscussion->SetDiscussionUpdateObserver(NULL);
	}
	
#ifdef __SERIES60_40__
	ResetItemLinks();
#endif

	iIsChannel = false;	
	iJumpToUnreadPost = true;
	iSelectedItem = KErrNotFound;
	
	// Set messaging id
	if(iMessagingId) {
		delete iMessagingId;
	}
	
	iMessagingId = iTextUtilities->Utf8ToUnicodeL(iMessagingObject.iData).AllocL();
	
	// Initialize message discussion
	iDiscussion = iBuddycloudLogic->GetDiscussion(*iMessagingId);
	iDiscussion->SetDiscussionUpdateObserver(this);
	
	// Is a channel
	CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
	iFollowingItemIndex = aItemStore->GetIndexById(iMessagingObject.iId);
	iItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(iMessagingObject.iId));
	
	if(iItem && iItem->GetItemType() >= EItemRoster) {
		if(iItem->GetItemType() == EItemChannel || 
				(iItem->GetItemType() == EItemRoster && iItem->GetId().Compare(*iMessagingId) == 0)) {
			
			// Is a channel discussion
			iIsChannel = true;
		}
	}
	
	// Remove old entries
	for(TInt i = 0; i < iEntries.Count(); i++) {
		if(iEntries[i]) {
			delete iEntries[i];
		}
	}

	iEntries.Reset();
	
	// Collect new entries
	for(TInt i = 0; i < iDiscussion->EntryCount(); i++) {
		CThreadedEntry* aThread = iDiscussion->GetThreadedEntryByIndex(i);
		
		// Entry
		iEntries.Append(CTextWrappedEntry::NewLC(aThread->GetEntry(), false));
		CleanupStack::Pop();
		
		// Comments
		for(TInt x = 0; x < aThread->CommentCount(); x++) {
			iEntries.Append(CTextWrappedEntry::NewLC(aThread->GetCommentByIndex(x), true));
			CleanupStack::Pop();
		}
	}
	
	// Set size (wraps entries)
	AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, iRect);
	SetRect(iRect);
	
	TimerExpired(KTimeTimerId);
}

void CBuddycloudMessagingContainer::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationFollowingItemDeleted || aEvent == ENotificationFollowingItemsReconfigured) {
		if(aId == iMessagingObject.iId) {
			if(aEvent == ENotificationFollowingItemDeleted) {
				// Channel or user is removed from following list
				iDiscussion = NULL;
				
				iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(aId), KNullDesC8);
			}
			else {
				// Channel node is reconfigured
				if(iIsChannel) {
					if(iMessagingId) {
						delete iMessagingId;
					}
					
					iMessagingId = iItem->GetId().AllocL();
				}
			}
		}
	}
	else {
		CBuddycloudListComponent::NotificationEvent(aEvent, aId);
	}
}

void CBuddycloudMessagingContainer::EntryAdded(CAtomEntryData* aAtomEntry) {
	TInt aIndex = KErrNotFound;
	TBool aAdded = false;
	
	for(TInt i = 0; i < iDiscussion->EntryCount() && !aAdded; i++) {
		CThreadedEntry* aThread = iDiscussion->GetThreadedEntryByIndex(i);
		
		aIndex++;
		
		if(aThread->GetEntry()->GetIndexerId() == aAtomEntry->GetIndexerId()) {
			iEntries.Insert(CTextWrappedEntry::NewLC(aAtomEntry, false), aIndex);
			CleanupStack::Pop();
			
			aAdded = true;
			break;
		}
		else {
			for(TInt x = 0; x < aThread->CommentCount(); x++) {
				aIndex++;
				
				if(aThread->GetCommentByIndex(x)->GetIndexerId() == aAtomEntry->GetIndexerId()) {
					iEntries.Insert(CTextWrappedEntry::NewLC(aAtomEntry, true), aIndex);
					CleanupStack::Pop();
					
					aAdded = true;
					break;
				}
			}
		}
	}
	
	// Move selection or redraw
	if(aAdded) {
		TextWrapEntry(aIndex);
		
		if(iJumpToUnreadPost) {
			HandleItemSelection(aIndex);
		}
		else {
			if(aIndex <= iSelectedItem) {
				iSelectedItem++;
			}
			
			RepositionItems(iSnapToItem);
		}
		
		RenderScreen();
	}
}

void CBuddycloudMessagingContainer::EntryDeleted(CAtomEntryData* aAtomEntry) {
	for(TInt i = 0; i < iEntries.Count(); i++) {
		if(iEntries[i]->GetEntry()->GetIndexerId() == aAtomEntry->GetIndexerId()) {			
			// Delete from entries
			delete iEntries[i];
			iEntries.Remove(i);	
			
			// Move selection or redraw
			if(iSelectedItem >= i) {
				HandleItemSelection(iSelectedItem - 1);
			}
			else {
				RepositionItems(iSnapToItem);
			}
			
			RenderScreen();	
			break;
		}
	}
}

void CBuddycloudMessagingContainer::TimerExpired(TInt aExpiryId) {
	if(aExpiryId == KDragTimerId) {
#ifdef __SERIES60_40__		
		if(iDraggingAllowed) {
			iDragVelocity = iDragVelocity * 0.95;		
			iScrollbarHandlePosition += TInt(iDragVelocity);		
			
			CBuddycloudListComponent::RepositionItems(false);
			RenderScreen();
			
			if(Abs(iDragVelocity) > 0.05) {
				iDragTimer->After(50000);
			}
		}
#endif
	}
	else if(aExpiryId == KTimeTimerId) {
#ifdef __3_2_ONWARDS__
		SetTitleL(iMessagingObject.iTitle);
#else
		TTime aTime;
		aTime.HomeTime();
		TBuf<32> aTextTime;
		aTime.FormatL(aTextTime, _L("%J%:1%T%B"));
	
		HBufC* aTextTimeTopic = HBufC::NewLC((aTextTime.Length() + 3 + iMessagingObject.iTitle.Length()));
		TPtr pTextTimeTopic(aTextTimeTopic->Des());
		pTextTimeTopic.Copy(aTextTime);
	
		if(iMessagingObject.iTitle.Length() > 0) {
			pTextTimeTopic.Append(_L(" - "));
			pTextTimeTopic.Append(iMessagingObject.iTitle);
		}
	
		SetTitleL(pTextTimeTopic);
	
		TDateTime aDateTime = aTime.DateTime();
		iTimer->After((60 - aDateTime.Second() + 1) * 1000);
	
		CleanupStack::PopAndDestroy(); // aTextTimeTopic
#endif
	}
}

void CBuddycloudMessagingContainer::ComposeNewCommentL(const TDesC& aContent) {
	if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
		CTextWrappedEntry* aWrappedEntry = iEntries[iSelectedItem];
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetEntryType() != EEntryContentNotice) {
			TBuf8<32> aReferenceId(aEntry->GetId().Left(32));
			
			if(aWrappedEntry->Comment()) {
				// Find root reference id
				for(TInt i = iSelectedItem - 1; i >= 0; i--) {
					if(!iEntries[i]->Comment() && iEntries[i]->GetEntry()) {
						aReferenceId.Copy(iEntries[i]->GetEntry()->GetId().Left(32));
						
						break;
					}
				}
			}
			
			ComposeNewPostL(aContent, aReferenceId);
		}
	}
	else {
		ComposeNewPostL(aContent);
	}
}

void CBuddycloudMessagingContainer::ComposeNewPostL(const TDesC& aContent, const TDesC8& aReferenceId) {
	CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
	
	// Add comment
	if(!iIsChannel || (aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed && 
			aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember)) {
		
		HBufC* aMessage = HBufC::NewLC(1024);
		TPtr pMessage(aMessage->Des());
		pMessage.Append(aContent);
	
		CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(pMessage, CAknQueryDialog::ENoTone);
		aDialog->SetPredictiveTextInputPermitted(true);
	
		if(pMessage.Length() > 0) {
			aDialog->PrepareLC(R_MESSAGING_LOWER_DIALOG);
		}
		else {
			aDialog->PrepareLC(R_MESSAGING_UPPER_DIALOG);
		}
		
		if(aReferenceId.Length() > 0) {
			HBufC* aDialogLabel = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_DIALOG_NEWCOMMENT);
			aDialog->SetPromptL(*aDialogLabel);
			CleanupStack::PopAndDestroy(); // aDialogLabel
		}
	
		if(aDialog->RunLD() != 0) {
			iJumpToUnreadPost = true;
			
			iBuddycloudLogic->PostMessageL(iMessagingObject.iId, *iMessagingId, pMessage, aReferenceId);	
			
			if(!iIsChannel) {
				RepositionItems(iSnapToItem);
				RenderScreen();
			}
		}
		
		CleanupStack::PopAndDestroy(); // aMessage
	}
}

TBool CBuddycloudMessagingContainer::OpenPostedLinkL() {
	CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

	if(aEntry && aEntry->GetLinkCount() > 0) {
		CEntryLinkPosition aLinkPosition = aEntry->GetLink(iSelectedLink);
		
		if(aLinkPosition.iType == ELinkWebsite) {
			CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
			TPtrC aLinkText(aEntry->GetContent().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
			aLauncher->LaunchBrowserWithLinkL(aLinkText);	
			CleanupStack::PopAndDestroy(); // aLauncher
		}
		else {
			iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_MESSAGING_FOLLOW_MENUBAR);
			iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
			iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_MESSAGING_OPTIONS_MENUBAR);
		}
		
		return true;
	}
	
	return false;
}

void CBuddycloudMessagingContainer::TextWrapEntry(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iEntries.Count()) {
		TInt aWrapWidth = iRect.Width() - 8 - iLeftBarSpacer - iRightBarSpacer;
		CTextWrappedEntry* aFormattedEntry = iEntries[aIndex];		
		aFormattedEntry->ResetLines();
				
		// Comment
		if(aFormattedEntry->Comment()) {
			aWrapWidth -= (iItemIconSize / 2);
		}
		
		// Font
		CAtomEntryData* aEntry = aFormattedEntry->GetEntry();
		CFont* aUsedFont = iSecondaryFont;
	
		if(aEntry->GetEntryType() == EEntryContentAction) {
			aUsedFont = iSecondaryItalicFont;
		}
		else if(!aEntry->Read()) {
			aUsedFont = iSecondaryBoldFont;
		}
		
		// Wrap
		iTextUtilities->WrapToArrayL(aFormattedEntry->iLines, aUsedFont, aEntry->GetContent(), aWrapWidth);
	}
}

void CBuddycloudMessagingContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	if(!iLayoutMirrored) {
		iLeftBarSpacer += (iItemIconSize / 8);
	}
	else {
		iRightBarSpacer += (iItemIconSize / 8);
	}
	
	// Wrap entry lines
	for(TInt i = 0; i < iEntries.Count(); i++) {
		TextWrapEntry(i);
	}
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
}

void CBuddycloudMessagingContainer::RenderSelectedText(TInt& aDrawPos) {
	CTextWrappedEntry* aWrappedEntry = iEntries[iSelectedItem];
	CAtomEntryData* aEntry = aWrappedEntry->GetEntry();
	
	if(aEntry) {		
		TInt aMaxWidth = iRect.Width() - 8 - iLeftBarSpacer - iRightBarSpacer;
		TInt aTypingStartPos = iLeftBarSpacer + 5;
		
		// Comment position
		if(aWrappedEntry->Comment()) {
			aMaxWidth -= (iItemIconSize / 2);
			aTypingStartPos += (iItemIconSize / 2);
		}
		
		// Font
		CFont* aRenderingFont = iSecondaryFont;
		
		if(!aEntry->Read()) {
			aRenderingFont = iSecondaryBoldFont;
		}
		
		iBufferGc->UseFont(aRenderingFont);
		
		// Link positions
		TInt aCurrentLinkNumber = 0;
		CEntryLinkPosition aCurrentLink = aEntry->GetLink(aCurrentLinkNumber);
		TPtrC pCurrentLink(aEntry->GetContent().Mid(aCurrentLink.iStart, aCurrentLink.iLength));

#ifdef __SERIES60_40__
		// Initialize link regions
		iItemLinks.Reset();
		
		for(TInt i = 0; i < aEntry->GetLinkCount(); i++) {
			iItemLinks.Append(RRegion());
		}
#endif
		
		TPtrC pText(aEntry->GetContent());		
		TInt aGlobalPosition = 0;
				
		while(pText.Length() > 0) {
			TInt aCharLocation, aBurn = 1;
			
			// Find new paragraph
			if((aCharLocation = pText.Locate('\n')) == KErrNotFound) {
				aCharLocation = pText.Length();
				aBurn = 0;
			}
	
			// Copy
			TPtrC pTextParagraph(pText.Left(aCharLocation));
			pText.Set(pText.Mid(aCharLocation + aBurn));
						
			// Word wrap
			do {
				TInt aDisplayableWidth = aRenderingFont->TextCount(pTextParagraph, aMaxWidth);
				TPtrC pTextLine(pTextParagraph.Left(aDisplayableWidth));
	
				// Location last space in sentance
				if(aDisplayableWidth < pTextParagraph.Length()) {					
					if((aCharLocation = pTextLine.LocateReverse(' ')) != KErrNotFound) {
						pTextLine.Set(pTextLine.Left(aCharLocation));
						aDisplayableWidth = aCharLocation + 1;
					}
				}
				
				TInt aTypingPos = aTypingStartPos;
				TPtrC pDirectionalText(iTextUtilities->BidiLogicalToVisualL(pTextLine));
				
				aDrawPos += iSecondaryItalicFont->HeightInPixels();
				aGlobalPosition += aDisplayableWidth;
				
				if(pCurrentLink.Length() > 0 && aGlobalPosition > aCurrentLink.iStart) {
					while(pDirectionalText.Length() > 0) {
						TPtrC pTextWordAndSpace(pDirectionalText);
						TPtrC pTextWord(pTextWordAndSpace);
						TInt aWordLength = pTextWord.Length();
					
						// Word by word span for link
						if(pCurrentLink.Length() < pTextWord.Length()) {
							if((aCharLocation = pDirectionalText.Locate(' ')) != KErrNotFound) {
								pTextWordAndSpace.Set(pTextWord.Left(aCharLocation + 1));
								pTextWord.Set(pTextWord.Left(aCharLocation));
								aWordLength = aCharLocation + 1;
							}
							
							if(pCurrentLink.Length() > 0 && pTextWord.Length() >= pCurrentLink.Length()) {								
								if(pTextWord.Length() > pCurrentLink.Length()) {
									TInt aLocate = pTextWord.Find(pCurrentLink);
									
									if(aLocate != KErrNotFound) {
										if(aLocate > 0) {
											pTextWordAndSpace.Set(pTextWord.Left(aLocate));
										}
										else {
											pTextWordAndSpace.Set(pTextWord.Left(pCurrentLink.Length()));
											
											if(iSelectedLink == aCurrentLinkNumber) {
												iBufferGc->SetPenColor(iColourTextLink);
											}
											
											iBufferGc->SetUnderlineStyle(EUnderlineOn);							
											pCurrentLink.Set(pCurrentLink.Left(0));								
#ifdef __SERIES60_40__
											// Add link region
											iItemLinks[aCurrentLinkNumber].AddRect(TRect(aTypingPos, (aDrawPos - aRenderingFont->FontMaxAscent()), (aTypingPos + aRenderingFont->TextWidthInPixels(pTextWordAndSpace)), (aDrawPos + aRenderingFont->FontMaxDescent())));
#endif
										}
										
										pTextWord.Set(pTextWordAndSpace);
										aWordLength = pTextWord.Length();
									}									
								}
								else if(pTextWord.Compare(pCurrentLink) == 0) {
									if(iSelectedLink == aCurrentLinkNumber) {
										iBufferGc->SetPenColor(iColourTextLink);
									}
									
									iBufferGc->SetUnderlineStyle(EUnderlineOn);							
									pCurrentLink.Set(pCurrentLink.Left(0));								
#ifdef __SERIES60_40__
									// Add link region
									iItemLinks[aCurrentLinkNumber].AddRect(TRect(aTypingPos, (aDrawPos - aRenderingFont->FontMaxAscent()), (aTypingPos + aRenderingFont->TextWidthInPixels(pTextWordAndSpace)), (aDrawPos + aRenderingFont->FontMaxDescent())));
#endif
								}
							}
						}
						else if(pCurrentLink.Length() >= pTextWord.Length()) {
							TPtrC pPartialLink(pCurrentLink.Left(pTextWord.Length()));
								
							if(pTextWord.Compare(pPartialLink) == 0) {
								if(iSelectedLink == aCurrentLinkNumber) {
									iBufferGc->SetPenColor(iColourTextLink);
								}
									
								iBufferGc->SetUnderlineStyle(EUnderlineOn);										
								pCurrentLink.Set(pCurrentLink.Mid(pPartialLink.Length()));
#ifdef __SERIES60_40__
								// Add link region
								iItemLinks[aCurrentLinkNumber].AddRect(TRect(aTypingPos, (aDrawPos - aRenderingFont->FontMaxAscent()), (aTypingPos + aRenderingFont->TextWidthInPixels(pTextWordAndSpace)), (aDrawPos + aRenderingFont->FontMaxDescent())));
#endif
							}
						}					
						
						if(pCurrentLink.Length() == 0 && aCurrentLink.iLength > 0) {
							// Get next link
							aCurrentLink = aEntry->GetLink(++aCurrentLinkNumber);
							pCurrentLink.Set(aEntry->GetContent().Mid(aCurrentLink.iStart, aCurrentLink.iLength));							
						}
										
						iBufferGc->DrawText(pTextWord, TPoint(aTypingPos, aDrawPos));
						
						aTypingPos += aRenderingFont->TextWidthInPixels(pTextWordAndSpace);
						
						iBufferGc->SetPenColor(iColourTextSelected);
						iBufferGc->SetUnderlineStyle(EUnderlineOff);
											
						pDirectionalText.Set(pDirectionalText.Mid(aWordLength));
					}
				}
				else {
					iBufferGc->DrawText(pDirectionalText, TPoint(aTypingPos, aDrawPos));
				}

				pTextParagraph.Set(pTextParagraph.Mid(aDisplayableWidth));
			} while(pTextParagraph.Length() > 0);
		}
		
		iBufferGc->DiscardFont();
	}
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::ResetItemLinks() {
	for(TInt i = 0; i < iItemLinks.Count(); i++) {
		iItemLinks[i].Close();
	}
	
	iItemLinks.Reset();
}
#endif

TInt CBuddycloudMessagingContainer::CalculateMessageSize(TInt aIndex, TBool aSelected) {
	CAtomEntryData* aEntry = iEntries[aIndex]->GetEntry();
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;

#ifdef __SERIES60_40__
	if( !aSelected ) {
		aItemSize += (iSecondaryBoldFont->DescentInPixels() * 2);
	}
#endif

	if(aSelected) {
		aMinimumSize = iItemIconSize + 4;
		
		if(aEntry->GetEntryType() < EEntryContentAction) {
			// From
			if(aEntry->GetAuthorName().Length() > 0) {
				aItemSize += iPrimaryBoldFont->HeightInPixels();
				aItemSize += iPrimaryBoldFont->FontMaxDescent();
			}
		}

		// Wrapped text
		aItemSize += (iWrappedTextArray.Count() * iSecondaryItalicFont->HeightInPixels());
		
		if(aItemSize < (iItemIconSize + 2)) {
			aItemSize = (iItemIconSize + 2);
		}
	}
	
	if(aEntry->GetEntryType() < EEntryContentAction || !aSelected) {
		// Textual message body or unselected user action/event
		aItemSize += (iSecondaryItalicFont->HeightInPixels() * iEntries[aIndex]->iLines.Count());			
	}

	aItemSize += iSecondaryFont->FontMaxDescent() + 2;

	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudMessagingContainer::RenderScreen() {
	if(!iRendering) {
		iRendering = true;

		CBuddycloudListComponent::RenderScreen();
		
		iRendering = false;
	}
}

void CBuddycloudMessagingContainer::RenderWrappedText(TInt aIndex) {
	// Clear
	ClearWrappedText();
	
	if(aIndex >= 0 && aIndex < iEntries.Count()) {
		CAtomEntryData* aEntry = iEntries[aIndex]->GetEntry();
		
		if(aEntry) {
			TInt aWrapWidth = (iRect.Width() - iSelectedItemIconTextOffset - 5 - iLeftBarSpacer - iRightBarSpacer);
			
			if(iEntries[aIndex]->Comment()) {
				aWrapWidth -= (iItemIconSize / 2);
			}
			
			if(aEntry->GetEntryType() < EEntryContentAction) {
				TBuf<256> aBuf;
				
				// Time & date
				TTime aTimeReceived = aEntry->GetPublishTime();
				TTime aTimeNow;
				
				aTimeReceived += User::UTCOffset();
				aTimeNow.HomeTime();
	
				if(aTimeReceived.DayNoInYear() == aTimeNow.DayNoInYear()) {
					aTimeReceived.FormatL(aBuf, _L("%J%:1%T%B"));
				}
				else if(aTimeReceived > aTimeNow - TTimeIntervalDays(6)) {
					aTimeReceived.FormatL(aBuf, _L("%E at %J%:1%T%B"));
				}
				else {
					aTimeReceived.FormatL(aBuf, _L("%J%:1%T%B on %F%N %*D%X"));
				}
				
				TPtrC aLocation(aEntry->GetLocation()->GetString(EGeolocText));
				
				// Location
				if(aLocation.Length() > 0) {
					aBuf.Append(_L(" ~ "));
					aBuf.Append(aLocation.Left(aBuf.MaxLength() - aBuf.Length()));
				}
				
				// Wrap
				iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryItalicFont, aBuf, aWrapWidth);
			}		
			else {
				// Wrap
				iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryItalicFont, aEntry->GetContent(), aWrapWidth);
			}
		}
	}
}

TInt CBuddycloudMessagingContainer::CalculateItemSize(TInt /*aIndex*/) {
	return 0;
}

void CBuddycloudMessagingContainer::RenderListItems() {
	TInt aDrawPos = -iScrollbarHandlePosition;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iEntries.Count() > 0) {
		TInt aItemSize = 0;
		TRect aItemRect;

#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iEntries.Count(); i++) {
			CAtomEntryData* aEntry = iEntries[i]->GetEntry();
			
			// Calculate item size
			aItemSize = CalculateMessageSize(i, (i == iSelectedItem));

			// Test to start the page drawing
			if(aDrawPos + aItemSize > 0) {
				TInt aItemDrawPos = aDrawPos;
				TInt aItemLeftOffset = 1 + (iEntries[i]->Comment() * (iItemIconSize / 2));
				
				if(!iEntries[i]->Read() && aItemDrawPos + aItemSize <= iRect.Height()) {
					iEntries[i]->SetRead(true);
				}

				aItemRect = TRect(iLeftBarSpacer + aItemLeftOffset, aItemDrawPos, (iRect.Width() - iRightBarSpacer), (aItemDrawPos + aItemSize));
				
				if(i == iSelectedItem) {
					iSelectedItemBox = aItemRect;
				}

#ifdef __SERIES60_40__
				iListItems.Append(TListItem(i, aItemRect));
#endif

				// Limit size
				if(aItemRect.iTl.iY < -7) {
					aItemRect.iTl.iY = -7;
				}

				if(aItemRect.iBr.iY > iRect.Height() + 7) {
					aItemRect.iBr.iY = iRect.Height() + 7;
				}
				
				// Affiliation
				if(aEntry->GetAuthorAffiliation() > EPubsubAffiliationPublisher) {
					if(aEntry->GetAuthorAffiliation() == EPubsubAffiliationOwner) {
						// Owner
						iBufferGc->SetBrushColor(TRgb(237, 88, 47));
						iBufferGc->SetPenColor(TRgb(237, 88, 47, 125));			
					}
					else {
						// Moderator
						iBufferGc->SetBrushColor(TRgb(246, 170, 44));
						iBufferGc->SetPenColor(TRgb(246, 170, 44, 125));	
					}
					
					TRect aAffiliationBox(TRect(2, aItemRect.iTl.iY, iLeftBarSpacer - 1, aItemRect.iBr.iY));
					
					if(iLayoutMirrored) {
						aAffiliationBox = TRect(iRightBarSpacer + 2, aItemRect.iTl.iY, iRect.Width() - 1, aItemRect.iBr.iY);
					}				
								
					iBufferGc->SetPenStyle(CGraphicsContext::ENullPen);
					iBufferGc->DrawRect(aAffiliationBox);
					iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
					iBufferGc->DrawLine(TPoint(aAffiliationBox.iTl.iX - 1, aAffiliationBox.iTl.iY), TPoint(aAffiliationBox.iTl.iX - 1, aAffiliationBox.iBr.iY));
					iBufferGc->DrawLine(TPoint(aAffiliationBox.iBr.iX, aAffiliationBox.iTl.iY), aAffiliationBox.iBr);
				}
				
				// Render frame & select font colour
				if(i == iSelectedItem) {
					RenderItemFrame(aItemRect);
					iBufferGc->SetPenColor(iColourTextSelected);
				}
				else {
					iBufferGc->SetPenColor(iColourText);
				}
				
				// Select font based of message type
				CFont* aRenderingFont = iSecondaryFont;
				
				if(aEntry->GetEntryType() == EEntryContentAction) {
					aRenderingFont = iSecondaryItalicFont;
				}
				else if(!aEntry->Read()) {
					aRenderingFont = iSecondaryBoldFont;
				}

				iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + aItemLeftOffset + 2, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 2), iRect.Height()));
#ifdef __SERIES60_40__
				if(i != iSelectedItem) {
					aItemDrawPos += iSecondaryBoldFont->DescentInPixels();
				}
#endif
				
				if(i == iSelectedItem) {	
					// Avatar
					iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
					iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + aItemLeftOffset + 5, (aItemDrawPos + 2)), iAvatarRepository->GetImage(aEntry->GetIconId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aEntry->GetIconId(), true, iIconMidmapSize), true);

					if(aEntry->Private()) {
						// Private message
						iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + aItemLeftOffset + 5, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KOverlayLocked, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KOverlayLocked, true, iIconMidmapSize), true);
					}

					iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

					if(aEntry->GetEntryType() < EEntryContentAction) {
						// Name
						TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aEntry->GetAuthorName()));
						
						if(iPrimaryBoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - aItemLeftOffset - iRightBarSpacer)) < aDirectionalText.Length()) {
							iBufferGc->UseFont(iSecondaryBoldFont);
						}
						else {
							iBufferGc->UseFont(iPrimaryBoldFont);
						}
	
						aItemDrawPos += iPrimaryBoldFont->HeightInPixels();
						iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + aItemLeftOffset + iSelectedItemIconTextOffset, aItemDrawPos));
						aItemDrawPos += iPrimaryBoldFont->FontMaxDescent();
						iBufferGc->DiscardFont();
					}
					
					// Wrapped text
					iBufferGc->UseFont(iSecondaryItalicFont);
						
					for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
						aItemDrawPos += iSecondaryItalicFont->HeightInPixels();
						iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + aItemLeftOffset + iSelectedItemIconTextOffset, aItemDrawPos));
					}
						
					iBufferGc->DiscardFont();
					
					if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
						aItemDrawPos = (aDrawPos + iItemIconSize + 2);
					}
				}
					
				// Render message text
				if(aEntry->GetEntryType() < EEntryContentAction && i == iSelectedItem && aEntry->GetLinkCount() > 0) {						
					RenderSelectedText(aItemDrawPos);
				}
				else if(aEntry->GetEntryType() < EEntryContentAction || i != iSelectedItem) {
					TInt aRenderingPosition = iLeftBarSpacer + aItemLeftOffset + 4;
				
					iBufferGc->UseFont(aRenderingFont);
						
					for(TInt x = 0; x < iEntries[i]->LineCount(); x++) {
						aItemDrawPos += iSecondaryItalicFont->HeightInPixels();
						iBufferGc->DrawText(iEntries[i]->GetLine(x), TPoint(aRenderingPosition, aItemDrawPos));
					}	
						
					iBufferGc->DiscardFont();
				}

				iBufferGc->CancelClippingRect();
				aItemDrawPos = (aDrawPos + aItemSize - 1);

				// Separator line
				if((i != iSelectedItem) && ((i + 1) != iSelectedItem) && (i < (iEntries.Count() - 1))) {
					iBufferGc->SetPenColor(iColourTextTrans);
					iBufferGc->SetPenStyle(CGraphicsContext::EDashedPen);
					iBufferGc->DrawLine(TPoint(iLeftBarSpacer + aItemLeftOffset + 4, aItemDrawPos), TPoint((iRect.Width() - iRightBarSpacer - 5), aItemDrawPos));
					iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
				}
			}
			
			aDrawPos += aItemSize;

			// Finish page drawing
			if(aDrawPos > iRect.Height()) {
				break;
			}
		}
	}
	else {
		// No messages
		_LIT(KNoMessages, "(No Messages)");
		iBufferGc->SetPenColor(iColourText);
		iBufferGc->UseFont(iSecondaryBoldFont);
		iBufferGc->DrawText(KNoMessages, TPoint(iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2) - (iSecondaryBoldFont->TextWidthInPixels(KNoMessages) / 2), (iRect.Height() / 2) + (iSecondaryBoldFont->HeightInPixels() / 2)));
		iBufferGc->DiscardFont();
		
		iSelectedItem = KErrNotFound;
		
#ifdef __SERIES60_40__
		ResetItemLinks();
		DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
	}
}

void CBuddycloudMessagingContainer::RepositionItems(TBool aSnapToItem) {
	iTotalListSize = 0;
	iSelectedItemBox = TRect();
	
	// Enable snapping again
	if(aSnapToItem) {
		iSnapToItem = aSnapToItem;
		iScrollbarHandlePosition = 0;
		
		if(iJumpToUnreadPost) {
			iSelectedItem = KErrNotFound;
		}
	}
	
	if(iEntries.Count() > 0) {
		TInt aItemSize;

		// Check if current item exists
		if(iSelectedItem < 0 || iSelectedItem >= iEntries.Count()) {
			for(TInt i = 0; i < iEntries.Count(); i++) {
				iSelectedItem = i;
			
				if(!iEntries[i]->GetEntry()->Read()) {
					iJumpToUnreadPost = false;
					break;
				}
			}
			
			RenderWrappedText(iSelectedItem);
			iSelectedLink = 0;
		
#ifdef __SERIES60_40__
			ResetItemLinks();
#endif
		}
		
#ifdef __SERIES60_40__
		DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif
		
		for(TInt i = 0; i < iEntries.Count(); i++) {
			aItemSize = CalculateMessageSize(i, (i == iSelectedItem));
			
			if(aSnapToItem && i == iSelectedItem) {
				// Mark item read
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
				
				if(!aEntry->Read()) {
					iEntries[i]->SetRead(true);
					aEntry->SetRead(true);
						
					TextWrapEntry(iSelectedItem);
					
					aItemSize = CalculateMessageSize(i, true);
				}
				
				// Calculate select item position
				if(aItemSize > iRect.Height()) {
					iScrollbarHandlePosition = iTotalListSize;
					iSelectedItemBox = TRect(0, 0, iRect.Width(), aItemSize);
					iSnapToItem = false;
				}
				else if(iTotalListSize + (aItemSize / 2) > (iRect.Height() / 2)) {
					iScrollbarHandlePosition = (iTotalListSize + (aItemSize / 2)) - (iRect.Height() / 2);
					iSelectedItemBox = TRect(0, ((iRect.Height() / 2) - (aItemSize / 2)), iRect.Width(), aItemSize);
				}
				else {
					iSelectedItemBox = TRect(0, iTotalListSize, iRect.Width(), (iTotalListSize + aItemSize));					
				}
			}
			
			// Increment total list size
			iTotalListSize += aItemSize;
		}
	}
	
	CBuddycloudListComponent::RepositionItems(aSnapToItem);
}

void CBuddycloudMessagingContainer::HandleItemSelection(TInt aItemId) {
	if(iSelectedItem != aItemId) {
		// New item selected
		iSelectedItem = aItemId;
		iSelectedLink = 0;
		iJumpToUnreadPost = false;
#ifdef __SERIES60_40__
		iDraggingAllowed = false;
#endif
		
		RenderWrappedText(iSelectedItem);
		RepositionItems(true);	
		
#ifdef __SERIES60_40__
		ResetItemLinks();
#endif
		RenderScreen();	
#ifdef __SERIES60_40__
		DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif
	}
	else {
		// Trigger item event
#ifdef __SERIES60_40__
		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
			
			if(aEntry && aEntry->GetEntryType() == EEntryContentNotice) {
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_MESSAGING_FOLLOW_MENUBAR);
				iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_MESSAGING_OPTIONS_MENUBAR);
			}
			else {
				ComposeNewCommentL(KNullDesC);	
			}
		}
		else {
			ComposeNewPostL(KNullDesC);	
		}
#else
		if(iEntries.Count() == 0 || !OpenPostedLinkL()) {
			ComposeNewPostL(KNullDesC);				
		}
#endif
	}
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar) {
	if(aResourceId == R_MESSAGING_TOOLBAR) {
		aToolbar->SetItemDimmed(EMenuAddCommentCommand, true, true);
		aToolbar->SetItemDimmed(EMenuWritePostCommand, true, true);
		aToolbar->SetItemDimmed(EMenuJumpToUnreadCommand, true, true);
		
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
		
		if(!iIsChannel || (aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed && 
				aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember)) {
			
			aToolbar->SetItemDimmed(EMenuWritePostCommand, false, true);
			
			if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

				if(aEntry && aEntry->GetEntryType() != EEntryContentNotice && aEntry->GetId().Length() > 0) {
					aToolbar->SetItemDimmed(EMenuAddCommentCommand, false, true);
				}
			}
		}
		
		if(iDiscussion->GetUnreadEntries() > 0) {
			aToolbar->SetItemDimmed(EMenuJumpToUnreadCommand, false, true);
		}
	}
}
#endif

void CBuddycloudMessagingContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	iJumpToUnreadPost = false;
	
	if(aResourceId == R_MESSAGING_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, true);
		aMenuPane->SetItemDimmed(EMenuRequestToPostCommand, true);				
		aMenuPane->SetItemDimmed(EMenuAddCommentCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuWritePostCommand, true);				
		aMenuPane->SetItemDimmed(EMenuPostMediaCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsChannelCommand, true);
		aMenuPane->SetItemDimmed(EMenuNotificationOnCommand, true);
		aMenuPane->SetItemDimmed(EMenuNotificationOffCommand, true);

		// Jump to unread
		if(iDiscussion->GetUnreadEntries() > 0) {
			aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, false);
		}
		
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
		
		if(!iIsChannel || aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed) {			
			// Write comment & Item menu
			if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
				
				if(aEntry) {
					// Add comment
					if(aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
						if(aEntry->GetEntryType() != EEntryContentNotice && aEntry->GetId().Length() > 0) {
							aMenuPane->SetItemDimmed(EMenuAddCommentCommand, false);
						}
					}
		
					if(aEntry->GetAuthorName().Length() > 0) {
						aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aEntry->GetAuthorName().Left(32));
						aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
					}
				}
			}
			
			// Write post
			if(!iIsChannel || aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
				aMenuPane->SetItemDimmed(EMenuWritePostCommand, false);								
			}
			
			// Post media & Request to post
			if(iIsChannel) {
				if(aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
					aMenuPane->SetItemDimmed(EMenuPostMediaCommand, false);
				}
				else if(aChannelItem->GetPubsubAffiliation() == EPubsubAffiliationMember) {
					aMenuPane->SetItemDimmed(EMenuRequestToPostCommand, false);				
				}
			}
		}

		if(iIsChannel && (iDiscussion->GetUnreadEntries() > 0 || iBuddycloudLogic->GetState() == ELogicOnline)) {
			aMenuPane->SetItemDimmed(EMenuOptionsChannelCommand, false);
		}
				
		if(iDiscussion->Notify()) {
			aMenuPane->SetItemDimmed(EMenuNotificationOffCommand, false);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuNotificationOnCommand, false);
		}
	}
	else if(aResourceId == R_MESSAGING_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuCopyPostCommand, true);
		aMenuPane->SetItemDimmed(EMenuLikePostCommand, true);
		aMenuPane->SetItemDimmed(EMenuReportPostCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeletePostCommand, true);
		aMenuPane->SetItemDimmed(EMenuChangePermissionCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeclineCommand, true);		
		
		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

			if(aEntry) {
				if(aEntry->GetEntryType() == EEntryContentNotice) {
					// Content notice
					aMenuPane->SetItemDimmed(EMenuAcceptCommand, false);
					aMenuPane->SetItemDimmed(EMenuDeclineCommand, false);		
				}
				else {
					// Content item or action
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
					
					aMenuPane->SetItemDimmed(EMenuCopyPostCommand, false);
					
					// Channel only options
					if(iIsChannel && aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed && 
							aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
						
						// Delete post
						if(aEntry->Private() || (aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
								aEntry->GetAuthorAffiliation() <= aChannelItem->GetPubsubAffiliation())) {
							
							aMenuPane->SetItemDimmed(EMenuDeletePostCommand, false);
						}
						
						if(aEntry->GetAuthorJid().Length() > 0) {
							// Like post
							if(aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator) {
								aMenuPane->SetItemDimmed(EMenuLikePostCommand, false);
							}
						
							if(aEntry->GetAuthorJid().Compare(iBuddycloudLogic->GetOwnItem()->GetId()) != 0) {
								// Change user permissions
								if(aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
										aEntry->GetAuthorAffiliation() < aChannelItem->GetPubsubAffiliation()) {
									
									aMenuPane->SetItemDimmed(EMenuChangePermissionCommand, false);
								}
								
								// Report post
								if(aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationPublisher) {
									aMenuPane->SetItemDimmed(EMenuReportPostCommand, false);
								}
							}
						}
					}
					
					// Follow
					if(aEntry->GetAuthorJid().Length() > 0 && 
							!iBuddycloudLogic->IsSubscribedTo(aEntry->GetAuthorJid(), EItemRoster)) {
						
						aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
					}	
				}
			}
		}
	}
	else if(aResourceId == R_MESSAGING_OPTIONS_CHANNEL_MENU) {
		aMenuPane->SetItemDimmed(EMenuCopyChannelIdCommand, true);
		aMenuPane->SetItemDimmed(EMenuMarkReadCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditChannelCommand, true);
		
		if(iItem->GetItemType() == EItemChannel && iItem->GetSubTitle().Length() > 0) {
			aMenuPane->SetItemDimmed(EMenuCopyChannelIdCommand, false);
		}
		
		if(iDiscussion->GetUnreadEntries() > 0) {
			aMenuPane->SetItemDimmed(EMenuMarkReadCommand, false);
		}
		
		if(iBuddycloudLogic->GetState() == ELogicOnline) {
			aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
			aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, false);
			aMenuPane->SetItemDimmed(EMenuEditChannelCommand, false);
		}		
	}
	else if(aResourceId == R_MESSAGING_FOLLOW_MENU) {
		aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeclineCommand, true);		
		
		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

			if(aEntry) {
				if(aEntry->GetEntryType() == EEntryContentNotice) {
					// Content notice
					aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
					aMenuPane->SetItemDimmed(EMenuAcceptCommand, false);
					aMenuPane->SetItemDimmed(EMenuDeclineCommand, false);		
				}
				else if(aEntry->GetLinkCount() > 0) {
					// Content contains links
					CEntryLinkPosition aLinkPosition = aEntry->GetLink(iSelectedLink);
					
					if(aLinkPosition.iType != ELinkWebsite) {
						_LIT(KRootNode, "/channel/");
						HBufC* aLinkId = HBufC::NewLC(aLinkPosition.iLength + KRootNode().Length());
						TPtr pLinkId(aLinkId->Des());
						pLinkId.Append(aEntry->GetContent().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
						
						// Remove '#'
						if(pLinkId[0] == '#') {
							pLinkId.Delete(0, 1);
						}
							
						// Add '/channel/'
						if(aLinkPosition.iType == ELinkChannel && pLinkId.Find(KRootNode) != 0) {
							pLinkId.Insert(0, KRootNode);
						}
						
						TInt aItemId = iBuddycloudLogic->IsSubscribedTo(pLinkId, EItemRoster|EItemChannel);
						
						if(aItemId == 0) {
							// Not yet following
							if(aLinkPosition.iType == ELinkUsername) {
								aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, false);
							}
							else {
								aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
							}
						}
						else {
							CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
							CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(aItemId));
							
							if(aItem && aItem->GetItemType() >= EItemRoster) {
								if(aItem->GetId().Length() > 0) {
									aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
								}
								
								if(aItem->GetItemType() == EItemRoster) {
									aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
								}					
							}
						}
						
						CleanupStack::PopAndDestroy(); // aLinkId
					}	
				}
			}
		}
	}
}

void CBuddycloudMessagingContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuRequestToPostCommand) {		
		HBufC* aMessageText = HBufC::NewLC(1024);
		TPtr pMessageText(aMessageText->Des());
	
		CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(pMessageText, CAknQueryDialog::ENoTone);
		aDialog->SetPredictiveTextInputPermitted(true);
		aDialog->PrepareLC(R_MESSAGING_UPPER_DIALOG);
		aDialog->SetPromptL(*iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_DIALOG_REQUESTCOMMENT));
		CleanupStack::PopAndDestroy(); // R_LOCALIZED_STRING_DIALOG_REQUESTCOMMENT
	
		if(aDialog->RunLD() != 0) {
			iBuddycloudLogic->RequestPubsubNodeAffiliationL(*iMessagingId, EPubsubAffiliationPublisher, pMessageText);
		}
		
		CleanupStack::PopAndDestroy(); // aMessageText
	}
	else if(aCommand == EMenuWritePostCommand) {
		ComposeNewPostL(KNullDesC);
	}
	else if(aCommand == EMenuPostMediaCommand) {
		iBuddycloudLogic->MediaPostRequestL(iMessagingObject.iId);
		
		iJumpToUnreadPost = true;
	}
	else if(aCommand == EMenuJumpToUnreadCommand) {
		for(TInt i = 0; i < iEntries.Count(); i++) {
			CAtomEntryData* aEntry = iEntries[i]->GetEntry();
			
			if(aEntry && !aEntry->Read()) {
				HandleItemSelection(i);
				
				break;
			}
		}
	}
	else if(aCommand == EMenuAddCommentCommand) {
		ComposeNewCommentL(KNullDesC);
	}
	else if(aCommand == EMenuCopyPostCommand || aCommand == EMenuCopyChannelIdCommand) {
		CClipboard* aClipbroad = CClipboard::NewForWritingLC(CCoeEnv::Static()->FsSession());
		aClipbroad->StreamDictionary().At(KClipboardUidTypePlainText);
	 
		CPlainText* aPlainText = CPlainText::NewL();
		CleanupStack::PushL(aPlainText);
	 
		if(aCommand == EMenuCopyPostCommand) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
			
			if(aEntry) {
				aPlainText->InsertL(0, aEntry->GetContent());
			}
		}
		else {
			aPlainText->InsertL(0, iItem->GetSubTitle());	
		}
		
		aPlainText->CopyToStoreL(aClipbroad->Store(), aClipbroad->StreamDictionary(), 0, aPlainText->DocumentLength());
		aClipbroad->CommitL();
	 
		CleanupStack::PopAndDestroy(2); // aPlainText, aClipbroad
	}
	else if(aCommand == EMenuDeletePostCommand || aCommand == EMenuLikePostCommand || aCommand == EMenuReportPostCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry) {
			if(aCommand == EMenuDeletePostCommand) {
				if(aEntry->Private()) {
					iDiscussion->DeleteEntryById(aEntry->GetId());
				}
				else {
					iBuddycloudLogic->RetractPubsubNodeItemL(iItem->GetId(), aEntry->GetId());
				}
			}
			else if(aCommand == EMenuLikePostCommand) {
				iBuddycloudLogic->FlagTagNodeItemL(KTaggerTag, iItem->GetId(), aEntry->GetId());
			}
			else if(aCommand == EMenuReportPostCommand) {
				iBuddycloudLogic->FlagTagNodeItemL(KTaggerFlag, iItem->GetId(), aEntry->GetId());
			}
		}
	}
	else if(aCommand == EMenuMarkReadCommand) {
		for(TInt i = 0; i < iEntries.Count(); i++) {
			CAtomEntryData* aEntry = iEntries[i]->GetEntry();
			
			if(aEntry && !aEntry->Read()) {
				iEntries[i]->SetRead(true);
				aEntry->SetRead(true);
			}
		}
		
		RenderScreen();
	}
	else if(aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeModeratorsCommand) {
		TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(*iMessagingId));
		TInt aResourceId = KErrNotFound;
		
		TViewReferenceBuf aViewReference;
		aViewReference().iCallbackRequested = true;
		aViewReference().iCallbackViewId = KMessagingViewId;
		aViewReference().iOldViewData.iId = iItem->GetItemId();
		
		if(aCommand == EMenuSeeModeratorsCommand) {
			CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("owner"));
			CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("moderator"));
			aResourceId = R_LOCALIZED_STRING_TITLE_MODERATEDBY;
		}
		else {
			CExplorerStanzaBuilder::FormatBroadcasterXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId);
			aResourceId = R_LOCALIZED_STRING_TITLE_FOLLOWEDBY;
		}
		
		CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, aResourceId, _L("$OBJECT"), iMessagingObject.iTitle);
		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), aViewReference);					
	}	
	else if(aCommand == EMenuEditChannelCommand) {
		TViewReferenceBuf aViewReference;	
		aViewReference().iCallbackRequested = true;
		aViewReference().iCallbackViewId = KMessagingViewId;
		aViewReference().iOldViewData = iMessagingObject;
		aViewReference().iNewViewData = iMessagingObject;

		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelInfoViewId), TUid::Uid(0), aViewReference);					
	}
	else if(aCommand == EMenuAcceptCommand || aCommand == EMenuDeclineCommand) {
		// Accept & decline request
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetEntryType() == EEntryContentNotice) {
			TPtrC aJid(iTextUtilities->Utf8ToUnicodeL(aEntry->GetId()));

			if(aEntry->GetAuthorJid().Compare(_L("http://jabber.org/protocol/pubsub#subscribe_authorization")) == 0) {
				if(aCommand == EMenuAcceptCommand) {
					// Accept channel subscription
					iBuddycloudLogic->SetPubsubNodeSubscriptionL(aJid, *iMessagingId, EPubsubSubscriptionSubscribed);	
				}
				else {
					// Decline channel subscription
					iBuddycloudLogic->SetPubsubNodeSubscriptionL(aJid, *iMessagingId, EPubsubSubscriptionNone);		
				}
			}
			else if(aCommand == EMenuAcceptCommand) {
				// Accept publisher privilage
				iBuddycloudLogic->SetPubsubNodeAffiliationL(aJid, *iMessagingId, EPubsubAffiliationPublisher);		
			}
			
			// Delete item from discussion
			iDiscussion->DeleteEntryById(aEntry->GetId());
		}		
	}
	else if(aCommand == EMenuFollowCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry) {
			iBuddycloudLogic->FollowContactL(aEntry->GetAuthorJid());
		}
	}
	else if(aCommand == EMenuFollowLinkCommand || aCommand == EMenuChannelInfoCommand || 
			aCommand == EMenuChannelMessagesCommand || aCommand == EMenuPrivateMessagesCommand) {
		
		// Handle action on a highlighted link
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetLinkCount() > 0) {
			CEntryLinkPosition aLinkPosition = aEntry->GetLink(iSelectedLink);
			
			if(aLinkPosition.iType != ELinkWebsite) {
				TPtrC aLinkText(aEntry->GetContent().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
				
				if(aLinkText[0] == '#') {
					aLinkText.Set(aLinkText.Mid(1));
				}

				if(aCommand == EMenuFollowLinkCommand) {
					// Follow contact
					iBuddycloudLogic->FollowContactL(aLinkText);
				}
				else if(aCommand == EMenuChannelInfoCommand) {
					// View channel info
					TViewReferenceBuf aViewReference;	
					aViewReference().iCallbackRequested = true;
					aViewReference().iCallbackViewId = KMessagingViewId;
					aViewReference().iOldViewData.iId = iItem->GetItemId();
					aViewReference().iNewViewData.iTitle.Copy(aLinkText);
					aViewReference().iNewViewData.iData.Copy(aLinkText);
					
					if(aLinkPosition.iType == ELinkChannel) {
						aViewReference().iNewViewData.iData.Insert(0, _L8("/channel/"));
					}
					else {
						aViewReference().iNewViewData.iData.Insert(0, _L8("/user/"));
						aViewReference().iNewViewData.iData.Append(_L8("/channel"));
					}

					iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelInfoViewId), TUid::Uid(0), aViewReference);			
				}
				else {
					// View channel/private messages
					_LIT(KRootNode, "/channel/");
					HBufC* aLinkId = HBufC::NewLC(aLinkText.Length() + KRootNode().Length());
					TPtr pLinkId(aLinkId->Des());
					pLinkId.Append(aLinkText);
						
					if(aLinkPosition.iType == ELinkChannel) {
						pLinkId.Insert(0, KRootNode);
					}
						
					CBuddycloudFollowingStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
					CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(iBuddycloudLogic->IsSubscribedTo(pLinkId, EItemRoster|EItemChannel)));
						
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						iMessagingObject.iId = aItem->GetItemId();
						iMessagingObject.iTitle.Copy(aItem->GetTitle());
						iMessagingObject.iData.Copy(aItem->GetId());
						
						if(aItem->GetItemType() == EItemRoster && aCommand == EMenuPrivateMessagesCommand) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);							
							iMessagingObject.iData.Copy(aRosterItem->GetId());
						}
						
						InitializeMessageDataL();
						RenderScreen();
					}
					
					CleanupStack::PopAndDestroy(); // aLinkId
				}
			}
		}
	}
	else if(aCommand == EMenuNotificationOnCommand || aCommand == EMenuNotificationOffCommand) {
		iDiscussion->SetNotify(aCommand == EMenuNotificationOnCommand);
	}
	else if(aCommand == EMenuChangePermissionCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetAuthorJid().Length() > 0) {
			iBuddycloudLogic->ShowAffiliationDialogL(aEntry->GetAuthorJid(), *iMessagingId, aEntry->GetAuthorAffiliation(), true);		
		}
	}
	else if(aCommand == EAknSoftkeyBack) {		
		// Remove entries
		for(TInt i = 0; i < iEntries.Count(); i++) {
			if(iEntries[i]) {
				delete iEntries[i];
			}
		}

		iEntries.Reset();
		
		// Get item's index
		TInt aFollowingIdsIndex = iBuddycloudLogic->GetFollowingStore()->GetIndexById(iMessagingObject.iId);
		TInt aReturnItemId = iMessagingObject.iId;

		// Switch index if all entries read
		if(iDiscussion->GetUnreadEntries() == 0 && iFollowingItemIndex < aFollowingIdsIndex) {
			aReturnItemId = iBuddycloudLogic->GetFollowingStore()->GetIdByIndex(iFollowingItemIndex);
		}
		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(aReturnItemId), KNullDesC8);
	}
}

TKeyResponse CBuddycloudMessagingContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;
	TBool aRenderNeeded = false;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			if(iSelectedItemBox.iTl.iY < 0) {
				iScrollbarHandlePosition -= (iRect.Height() - iSecondaryFont->HeightInPixels());	

				CBuddycloudListComponent::RepositionItems(iSnapToItem);
				aRenderNeeded = true;
			}
			else if(iSelectedItem > 0) {
				HandleItemSelection(iSelectedItem - 1);
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			if(iSelectedItemBox.iBr.iY > iRect.Height()) {
				iScrollbarHandlePosition += (iRect.Height() - iSecondaryFont->HeightInPixels());	

				CBuddycloudListComponent::RepositionItems(iSnapToItem);
				aRenderNeeded = true;
			}
			else if(iSelectedItem < (iEntries.Count() - 1)) {
				HandleItemSelection(iSelectedItem + 1);
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyLeftArrow) {
			if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
				
				if(aEntry) {
					if(iSelectedLink == 0) {
						iSelectedLink = aEntry->GetLinkCount() - 1;
					}
					else {
						iSelectedLink--;
					}
					
					aRenderNeeded = true;
				}
			}
			
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyRightArrow) {
			if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
				
				if(aEntry) {
					if(iSelectedLink == aEntry->GetLinkCount() - 1) {
						iSelectedLink = 0;
					}
					else {
						iSelectedLink++;
					}
					
					aRenderNeeded = true;
				}
			}
			
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == 63557) { // Center Dpad
			HandleItemSelection(iSelectedItem);
			aResult = EKeyWasConsumed;
		}

		if(aResult == EKeyWasNotConsumed && aKeyEvent.iCode >= 33 && aKeyEvent.iCode <= 255) {
			aResult = EKeyWasConsumed;
			
			TBuf<8> aMessage;
	
			if(aKeyEvent.iCode <= 47 || aKeyEvent.iCode >= 58) {
				aMessage.Append(TChar(aKeyEvent.iCode));
				aMessage.Capitalize();
			}

			ComposeNewCommentL(aMessage);
		}
	}
	
	if(aRenderNeeded) {
		RenderScreen();
	}

	return aResult;
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::HandlePointerEventL(const TPointerEvent &aPointerEvent) {
	if(aPointerEvent.iType == TPointerEvent::EButton1Up) {			
		if(iDraggingAllowed) {
			if(Abs(iDragVelocity) > 5.0) {
				TimerExpired(KDragTimerId);
			}
		}
		else {
			for(TInt i = 0; i < iListItems.Count(); i++) {
				if(iListItems[i].iRect.Contains(aPointerEvent.iPosition)) {
					TBool aConsumed = false;
					
					if(iListItems[i].iId == iSelectedItem) {
						for(TInt x = 0; x < iItemLinks.Count(); x++) {
							if(iItemLinks[x].Contains(aPointerEvent.iPosition)) {
								iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);
								
								if(iSelectedLink != x) {
									// Select a link
									iSelectedLink = x;
									RenderScreen();
								}
								else {
									// Open link
									OpenPostedLinkL();
								}
								
								aConsumed = true;
							}
						}
					}
					
					if(!aConsumed) {
						iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);
						HandleItemSelection(iListItems[i].iId);	
					}
					
					break;
				}
			}
		}
	}
	else {
		CBuddycloudListComponent::HandlePointerEventL(aPointerEvent);
	}
}
#endif

// End of File
