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
#include "Buddycloud.hlp.hrh"
#include "BrowserLauncher.h"
#include "BuddycloudExplorer.h"
#include "BuddycloudMessagingContainer.h"

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
	
	if(!iEntry->Read() && iRead) {
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

void CBuddycloudMessagingContainer::ConstructL(const TRect& aRect, TMessagingViewObject aObject) {
	iRect = aRect;
	iMessagingObject = aObject;
	CreateWindowL();
	
	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	InitializeMessageDataL();
	
	iFollowingItemIndex = iBuddycloudLogic->GetFollowingStore()->GetIndexById(iMessagingObject.iFollowerId);
	
	iShowMialog = false;
	iRendering = false;
	
	RenderScreen();
	ActivateL();
}

CBuddycloudMessagingContainer::~CBuddycloudMessagingContainer() {
	iDiscussion->SetDiscussionUpdateObserver(NULL);

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
	
	// Initialize message discussion
	iDiscussion = iBuddycloudLogic->GetDiscussion(iMessagingObject.iId);
	iDiscussion->SetDiscussionUpdateObserver(this);
	
	// Is a channel
	CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
	iItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(iMessagingObject.iFollowerId));
	
	if(iItem && iItem->GetItemType() >= EItemRoster) {
		if(iItem->GetItemType() == EItemChannel || 
				(iItem->GetItemType() == EItemRoster && iItem->GetId().Compare(iMessagingObject.iId) == 0)) {
			
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
	if(aEvent == ENotificationFollowingItemDeleted) {
		if(aId == iMessagingObject.iFollowerId) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(0), _L8(""));
		}
	}
	else {
		if(aEvent == ENotificationMessageNotifiedEvent || aEvent == ENotificationMessageSilentEvent) {
			if(aId == iMessagingObject.iFollowerId) {
				RepositionItems(iSnapToItem);
				RenderScreen();
			}
		}
		
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
			
			TextWrapEntry(aIndex);
			
			aAdded = true;
			break;
		}
		else {
			for(TInt x = 0; x < aThread->CommentCount(); x++) {
				aIndex++;
				
				if(aThread->GetCommentByIndex(x)->GetIndexerId() == aAtomEntry->GetIndexerId()) {
					iEntries.Insert(CTextWrappedEntry::NewLC(aAtomEntry, true), aIndex);
					CleanupStack::Pop();
					
					TextWrapEntry(aIndex);
					
					aAdded = true;
					break;
				}
			}
		}
	}
	
	if(aAdded && iJumpToUnreadPost) {
		HandleItemSelection(aIndex);
	}
}

void CBuddycloudMessagingContainer::EntryDeleted(CAtomEntryData* aAtomEntry) {
	for(TInt i = 0; i < iEntries.Count(); i++) {
		if(iEntries[i]->GetEntry()->GetIndexerId() == aAtomEntry->GetIndexerId()) {
			if(iSelectedItem > i) {
				HandleItemSelection(iSelectedItem - 1);
			}
			
			delete iEntries[i];
			iEntries.Remove(i);	
			
			break;
		}
	}
}

void CBuddycloudMessagingContainer::TimerExpired(TInt aExpiryId) {
	if(aExpiryId == KDragTimerId) {
#ifdef __SERIES60_40__		
		iDragVelocity = iDragVelocity * 0.95;		
		iScrollbarHandlePosition += TInt(iDragVelocity);		
		
		CBuddycloudListComponent::RepositionItems(false);
		RenderScreen();
		
		if(Abs(iDragVelocity) > 0.05) {
			iDragTimer->After(50000);
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
		
		if(aEntry) {
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
	if(!iIsChannel || aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
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
			
			iBuddycloudLogic->PostMessageL(iMessagingObject.iFollowerId, iMessagingObject.iId, pMessage, aReferenceId);	
			
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
		CFont* aUsedFont = i10NormalFont;
	
		if(aEntry->GetEntryType() == EEntryContentAction) {
			aUsedFont = i10ItalicFont;
		}
		else if(!aEntry->Read()) {
			aUsedFont = i10BoldFont;
		}
		
		// Wrap
		iTextUtilities->WrapToArrayL(aFormattedEntry->iLines, aUsedFont, aEntry->GetContent(), aWrapWidth);
	}
}

void CBuddycloudMessagingContainer::GetHelpContext(TCoeHelpContext& aContext) const {	
	CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

	aContext.iMajor = TUid::Uid(HLPUID);
		
	if(aEntry && aEntry->GetLinkCount() > 0) {
		aContext.iContext = KLinkedMessages;
	}
	else if(iIsChannel) {
		aContext.iContext = KChannelMessaging;
	}
	else {
		aContext.iContext = KPrivateMessaging;
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
		CFont* aRenderingFont = i10NormalFont;
		
		if(!aEntry->Read()) {
			aRenderingFont = i10BoldFont;
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
				
				aDrawPos += aRenderingFont->HeightInPixels();
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
		aItemSize += (i10BoldFont->DescentInPixels() * 2);
	}
#endif

	if(aSelected) {
		aMinimumSize = iItemIconSize + 4;
		
		if(aEntry->GetEntryType() == EEntryContentPost) {
			// From
			if(aEntry->GetAuthorName().Length() > 0) {
				aItemSize += i13BoldFont->HeightInPixels();
				aItemSize += i13BoldFont->FontMaxDescent();
			}
		}

		// Wrapped text
		aItemSize += (iWrappedTextArray.Count() * i10ItalicFont->HeightInPixels());
		
		if(aItemSize < (iItemIconSize + 2)) {
			aItemSize = (iItemIconSize + 2);
		}
	}
	
	if(aEntry->GetEntryType() == EEntryContentPost) {
		// Textual message body
		aItemSize += (i10NormalFont->HeightInPixels() * iEntries[aIndex]->iLines.Count());			
	}
	else if(!aSelected) {
		// Unselected user action/event
		aItemSize += (i10ItalicFont->HeightInPixels() * iEntries[aIndex]->iLines.Count());			
	}

	aItemSize += i10NormalFont->FontMaxDescent() + 2;

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
			
			if(aEntry->GetEntryType() == EEntryContentPost) {
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
				iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aBuf, aWrapWidth);
			}		
			else if(aEntry->GetEntryType() == EEntryContentAction) {
				// Wrap
				iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aEntry->GetContent(), aWrapWidth);
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
			
			if(i == iSelectedItem && !aEntry->Read()) {
				iEntries[i]->SetRead(true);
				aEntry->SetRead(true);
					
				TextWrapEntry(iSelectedItem);
			}

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
				if(aEntry->GetAuthorAffiliation() == EPubsubAffiliationOwner) {
					iBufferGc->SetBrushColor(TRgb(237, 88, 47));
					iBufferGc->SetPenColor(TRgb(237, 88, 47, 125));			
				}
				else if(aEntry->GetAuthorAffiliation() == EPubsubAffiliationModerator) {
					iBufferGc->SetBrushColor(TRgb(246, 170, 44));
					iBufferGc->SetPenColor(TRgb(246, 170, 44, 125));	
				}
				else {
					iBufferGc->SetBrushColor(TRgb(175, 175, 175));
					iBufferGc->SetPenColor(TRgb(175, 175, 175, 125));					
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

				// Render frame & select font colour
				if(i == iSelectedItem) {
					RenderItemFrame(aItemRect);
					iBufferGc->SetPenColor(iColourTextSelected);
				}
				else {
					iBufferGc->SetPenColor(iColourText);
				}
				
				// Select font based of message type
				CFont* aRenderingFont = i10NormalFont;
				
				if(aEntry->GetEntryType() == EEntryContentAction) {
					aRenderingFont = i10ItalicFont;
				}
				else if(!aEntry->Read()) {
					aRenderingFont = i10BoldFont;
				}

				iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + aItemLeftOffset + 2, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 2), iRect.Height()));
#ifdef __SERIES60_40__
				if(i != iSelectedItem) {
					aItemDrawPos += i10BoldFont->DescentInPixels();
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

					if(aEntry->GetEntryType() == EEntryContentPost) {
						// Name
						TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aEntry->GetAuthorName()));
						
						if(i13BoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - aItemLeftOffset - iRightBarSpacer)) < aDirectionalText.Length()) {
							iBufferGc->UseFont(i10BoldFont);
						}
						else {
							iBufferGc->UseFont(i13BoldFont);
						}
	
						aItemDrawPos += i13BoldFont->HeightInPixels();
						iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + aItemLeftOffset + iSelectedItemIconTextOffset, aItemDrawPos));
						aItemDrawPos += i13BoldFont->FontMaxDescent();
						iBufferGc->DiscardFont();
					}
					
					// Wrapped text
					iBufferGc->UseFont(i10ItalicFont);
						
					for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
						aItemDrawPos += i10ItalicFont->HeightInPixels();
						iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + aItemLeftOffset + iSelectedItemIconTextOffset, aItemDrawPos));
					}
						
					iBufferGc->DiscardFont();
					
					if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
						aItemDrawPos = (aDrawPos + iItemIconSize + 2);
					}
				}
					
				// Render message text
				if(aEntry->GetEntryType() == EEntryContentPost && i == iSelectedItem && aEntry->GetLinkCount() > 0) {						
					RenderSelectedText(aItemDrawPos);
				}
				else if(aEntry->GetEntryType() == EEntryContentPost || i != iSelectedItem) {
					TInt aRenderingPosition = iLeftBarSpacer + aItemLeftOffset + 4;
				
					iBufferGc->UseFont(aRenderingFont);
						
					for(TInt x = 0; x < iEntries[i]->LineCount(); x++) {
						aItemDrawPos += i10NormalFont->HeightInPixels();
						iBufferGc->DrawText(iEntries[i]->GetLine(x), TPoint(aRenderingPosition, aItemDrawPos));
					}	
						
					iBufferGc->DiscardFont();
				}

				iBufferGc->CancelClippingRect();
				aItemDrawPos = (aDrawPos + aItemSize - 1);

				// Separator line
				if((i != iSelectedItem) && ((i + 1) != iSelectedItem) && (i < (iEntries.Count() - 1))) {
					iBufferGc->SetPenColor(iColourHighlightBorder);
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
		iBufferGc->UseFont(i10BoldFont);
		iBufferGc->DrawText(KNoMessages, TPoint(iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2) - (i10BoldFont->TextWidthInPixels(KNoMessages) / 2), (iRect.Height() / 2) + (i10BoldFont->HeightInPixels() / 2)));
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
			DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif
		}
		
		for(TInt i = 0; i < iEntries.Count(); i++) {
			aItemSize = CalculateMessageSize(i, (i == iSelectedItem));
			
			if(aSnapToItem && i == iSelectedItem) {
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
		
		RenderWrappedText(iSelectedItem);
		RepositionItems(true);	
		
#ifdef __SERIES60_40__
		ResetItemLinks();
		DynInitToolbarL(R_MESSAGING_TOOLBAR, iViewAccessor->ViewToolbar());
#endif
		RenderScreen();
	}
	else {
		// Trigger item event
		if(iEntries.Count() == 0 || !OpenPostedLinkL()) {
			ComposeNewCommentL(_L(""));				
		}
	}
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar) {
	if(aResourceId == R_MESSAGING_TOOLBAR) {
		aToolbar->SetItemDimmed(EMenuAddCommentCommand, true, true);
		aToolbar->SetItemDimmed(EMenuWritePostCommand, true, true);
		
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
			
		if(!iIsChannel || aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
			aToolbar->SetItemDimmed(EMenuWritePostCommand, false, true);
			
			if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
				CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

				if(aEntry && aEntry->GetId().Length() > 0) {
					aToolbar->SetItemDimmed(EMenuAddCommentCommand, false, true);
				}
			}
		}
	}
}
#endif

void CBuddycloudMessagingContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_MESSAGING_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuRequestToPostCommand, true);				
		aMenuPane->SetItemDimmed(EMenuWritePostCommand, true);				
		aMenuPane->SetItemDimmed(EMenuPostMediaCommand, true);
		aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsChannelCommand, true);
		aMenuPane->SetItemDimmed(EMenuNotificationOnCommand, true);
		aMenuPane->SetItemDimmed(EMenuNotificationOffCommand, true);
		
		if(iIsChannel) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
			
			if(aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
				aMenuPane->SetItemDimmed(EMenuWritePostCommand, false);				
				aMenuPane->SetItemDimmed(EMenuPostMediaCommand, false);
			}
			else {
				aMenuPane->SetItemDimmed(EMenuRequestToPostCommand, false);				
			}
		}
		else {
			aMenuPane->SetItemDimmed(EMenuWritePostCommand, false);				
		}

		if(iDiscussion->GetUnreadEntries() > 0) {
			aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, false);
		}

		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

			if(aEntry && aEntry->GetAuthorJid().Length() > 0) {
				aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aEntry->GetAuthorJid().Left(32));
				aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
			}
		}

		if(iIsChannel) {
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
		aMenuPane->SetItemDimmed(EMenuAddCommentCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuCopyPostCommand, true);
		aMenuPane->SetItemDimmed(EMenuLikePostCommand, true);
		aMenuPane->SetItemDimmed(EMenuReportPostCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeletePostCommand, true);
		aMenuPane->SetItemDimmed(EMenuChangePermissionCommand, true);
		
		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

			if(aEntry) {				
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iItem);	
				
				aMenuPane->SetItemDimmed(EMenuCopyPostCommand, false);
				
				// Add comment
				if(!iIsChannel || aChannelItem->GetPubsubAffiliation() > EPubsubAffiliationMember) {
					// Add comment
					if(aEntry->GetId().Length() > 0) {
						aMenuPane->SetItemDimmed(EMenuAddCommentCommand, false);
					}
					
					// Channel management
					if(iIsChannel) {
						// Delete post
						if(aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator) {
							aMenuPane->SetItemDimmed(EMenuLikePostCommand, false);
							aMenuPane->SetItemDimmed(EMenuDeletePostCommand, false);
						}
						
						// When not own post
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
				if(!iBuddycloudLogic->IsSubscribedTo(aEntry->GetAuthorJid(), EItemRoster)) {
					aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
				}				
			}
		}
	}
	else if(aResourceId == R_MESSAGING_FOLLOW_MENU) {
		aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		
		if(iEntries.Count() > 0 && iSelectedItem < iEntries.Count()) {
			CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();

			if(aEntry && aEntry->GetLinkCount() > 0) {
				CEntryLinkPosition aLinkPosition = aEntry->GetLink(iSelectedLink);
				
				if(aLinkPosition.iType != ELinkWebsite) {
					_LIT(KRootNode, "/channel/");
					HBufC* aLinkId = HBufC::NewLC(aLinkPosition.iLength + KRootNode().Length());
					TPtr pLinkId(aLinkId->Des());
					pLinkId.Append(aEntry->GetContent().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
						
					if(aLinkPosition.iType == ELinkChannel) {
						pLinkId = pLinkId.Mid(1);
						
						if(pLinkId.Find(KRootNode) != 0) {
							pLinkId.Insert(0, KRootNode);
						}
					}
					
					TInt aItemId = iBuddycloudLogic->IsSubscribedTo(pLinkId, EItemRoster|EItemChannel);
					
					if(aItemId == 0) {
						// Not yet following
						aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, false);
					}
					else {
						CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
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

void CBuddycloudMessagingContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuRequestToPostCommand) {		
	}
	else if(aCommand == EMenuWritePostCommand) {
		ComposeNewPostL(_L(""));
	}
	else if(aCommand == EMenuPostMediaCommand) {
		iBuddycloudLogic->MediaPostRequestL(iMessagingObject.iFollowerId);
		
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
		ComposeNewCommentL(_L(""));
	}
	else if(aCommand == EMenuCopyPostCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry) {
			CClipboard* aClipbroad = CClipboard::NewForWritingLC(CCoeEnv::Static()->FsSession());
			aClipbroad->StreamDictionary().At(KClipboardUidTypePlainText);
		 
			CPlainText* aPlainText = CPlainText::NewL();
			CleanupStack::PushL(aPlainText);
		 
			aPlainText->InsertL(0, aEntry->GetContent());		 
			aPlainText->CopyToStoreL(aClipbroad->Store(), aClipbroad->StreamDictionary(), 0, aPlainText->DocumentLength());
			aClipbroad->CommitL();
		 
			CleanupStack::PopAndDestroy(2); // aPlainText, aClipbroad
		}
	}
	else if(aCommand == EMenuDeletePostCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry) {
			iBuddycloudLogic->RetractPubsubNodeItemL(iItem->GetId(), aEntry->GetId());
		}
	}
	else if(aCommand == EMenuReportPostCommand) {
	}
	else if(aCommand == EMenuSeeFollowersCommand) {
		TPtrC8 aSubjectJid = iTextUtilities->UnicodeToUtf8L(iMessagingObject.iId);
		
		TExplorerQuery aQuery;
		aQuery.iStanza.Append(_L8("<iq to='maitred.buddycloud.com' type='get' id='exp_users1'><query xmlns='http://buddycloud.com/protocol/channels#followers'><channel><jid></jid></channel></query></iq>"));
		aQuery.iStanza.Insert(138, aSubjectJid);
		
		iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_FOLLOWEDBY);
		aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, iMessagingObject.iTitle.Left(aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 7));
	
		TExplorerQueryPckg aQueryPckg(aQuery);		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iMessagingObject.iFollowerId), aQueryPckg);		
	}
	else if(aCommand == EMenuFollowCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry) {
			iBuddycloudLogic->FollowContactL(aEntry->GetAuthorJid());
		}
	}
	else if(aCommand == EMenuFollowLinkCommand || aCommand == EMenuChannelMessagesCommand || aCommand == EMenuPrivateMessagesCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetLinkCount() > 0) {
			CEntryLinkPosition aLinkPosition = aEntry->GetLink(iSelectedLink);
			
			if(aLinkPosition.iType != ELinkWebsite) {
				_LIT(KRootNode, "/channel/");
				HBufC* aLinkId = HBufC::NewLC(aLinkPosition.iLength + KRootNode().Length());
				TPtr pLinkId(aLinkId->Des());
				pLinkId.Append(aEntry->GetContent().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
					
				if(aLinkPosition.iType == ELinkChannel) {
					pLinkId = pLinkId.Mid(1);
					
					if(pLinkId.Find(KRootNode) != 0) {
						pLinkId.Insert(0, KRootNode);
					}
				}
				
				if(aCommand == EMenuFollowLinkCommand) {
					if(aLinkPosition.iType == ELinkUsername) {
						iBuddycloudLogic->FollowContactL(pLinkId);
					}
					else {
						iBuddycloudLogic->FollowChannelL(pLinkId);
					}
				}
				else {
					CBuddycloudListStore* aItemStore = iBuddycloudLogic->GetFollowingStore();
					CFollowingItem* aItem = static_cast <CFollowingItem*> (aItemStore->GetItemById(iBuddycloudLogic->IsSubscribedTo(pLinkId, EItemRoster|EItemChannel)));
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						iMessagingObject.iFollowerId = aItem->GetItemId();
						iMessagingObject.iTitle = aItem->GetTitle();
						iMessagingObject.iId = aItem->GetId();
						
						if(aItem->GetItemType() == EItemRoster && aCommand == EMenuPrivateMessagesCommand) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);							
							iMessagingObject.iId = aRosterItem->GetId();
						}
						
						InitializeMessageDataL();
						RenderScreen();
					}
				}
				
				CleanupStack::PopAndDestroy(); // aLinkId
			}
		}
	}
	else if(aCommand == EMenuNotificationOnCommand || aCommand == EMenuNotificationOffCommand) {
		iDiscussion->SetNotify(aCommand == EMenuNotificationOnCommand);
	}
	else if(aCommand == EMenuChangePermissionCommand) {
		CAtomEntryData* aEntry = iEntries[iSelectedItem]->GetEntry();
		
		if(aEntry && aEntry->GetAuthorJid().Length() > 0) {
			TInt aSelectedIndex = 0;
			
			// Show list dialog
			CAknListQueryDialog* aDialog = new( ELeave ) CAknListQueryDialog(&aSelectedIndex);
			aDialog->PrepareLC(R_LIST_CHANGEPERMISSION);
			aDialog->ListBox()->SetCurrentItemIndex(1);

			if(aDialog->RunLD()) {
				TXmppPubsubAffiliation aAffiliation = EPubsubAffiliationNone;
				
				switch(aSelectedIndex) {
					case 0: // Remove
						aAffiliation = EPubsubAffiliationOutcast;
						break;
					case 2: // Publisher
						aAffiliation = EPubsubAffiliationPublisher;
						break;
					case 3: // Moderator
						aAffiliation = EPubsubAffiliationModerator;
						break;
					default: // Follower
						aAffiliation = EPubsubAffiliationMember;
						break;					
				}
				
				iBuddycloudLogic->SetPubsubNodeAffiliationL(aEntry->GetAuthorJid(), iMessagingObject.iId, aAffiliation);		
			}
		}
	}
	else if(aCommand == EAknSoftkeyBack) {
		TInt aResultId = iMessagingObject.iFollowerId;
		
		if(iDiscussion->GetUnreadEntries() == 0) {
			aResultId = iBuddycloudLogic->GetFollowingStore()->GetIdByIndex(iFollowingItemIndex);
		}
		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(aResultId), _L8(""));
	}
}

TKeyResponse CBuddycloudMessagingContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;
	TBool aRenderNeeded = false;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			if(iSelectedItemBox.iTl.iY < 0) {
				iScrollbarHandlePosition -= (iRect.Height() - i10NormalFont->HeightInPixels());	

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
				iScrollbarHandlePosition += (iRect.Height() - i10NormalFont->HeightInPixels());	

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

		if(aResult == EKeyWasNotConsumed && aKeyEvent.iCode >= 32 && aKeyEvent.iCode <= 255) {
			aResult = EKeyWasConsumed;
			
			TBuf<1> aMessage;
	
			if(aKeyEvent.iCode > 30) {
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
					if(iListItems[i].iId != iSelectedItem) {
						iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);
						HandleItemSelection(iListItems[i].iId);	
					}
					else {
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
							}
						}
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
