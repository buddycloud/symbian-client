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
#include <barsread.h>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hlp.hrh"
#include "BrowserLauncher.h"
#include "BuddycloudExplorer.h"
#include "BuddycloudMessagingContainer.h"
#include "MessagingParticipants.h"

/*
----------------------------------------------------------------------------
--
-- CFormattedBody
--
----------------------------------------------------------------------------
*/

CFormattedBody::CFormattedBody() {
}

CFormattedBody::~CFormattedBody() {
	for(TInt x = 0; x < iLines.Count(); x++) {
		if(iLines[x])
			delete iLines[x];
	}

	iLines.Close();
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
	
	iShowMialog = false;
	iRendering = false;
	
	RenderScreen();
	ActivateL();
}

CBuddycloudMessagingContainer::~CBuddycloudMessagingContainer() {
	iDiscussion->RemoveMessagingObserver();

	for(TInt x = 0; x < iMessages.Count(); x++) {
		if(iMessages[x])
			delete iMessages[x];
	}

	iMessages.Close();
	
#ifdef __SERIES60_40__
	ResetItemLinks();
	iItemLinks.Close();
#endif
}

void CBuddycloudMessagingContainer::InitializeMessageDataL() {
	// Destroy old message data
	if(iDiscussion) {
		iDiscussion->RemoveMessagingObserver();
	}
	
#ifdef __SERIES60_40__
	ResetItemLinks();
#endif

	iIsChannel = false;
	iIsPersonalChannel = false;
	
	iJumpToUnreadMessage = true;
	iSelectedItem = KErrNotFound;
	
	// Initialize message discussion
	iDiscussion = iBuddycloudLogic->GetDiscussion(iMessagingObject.iJid);
	iDiscussion->AddMessagingObserver(this);
	
	// Is a channel
	CFollowingItem* aItem = iBuddycloudLogic->GetFollowingStore()->GetItemById(iMessagingObject.iFollowerId);
	
	if(aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		if(aChannelItem->GetItemType() == EItemChannel) {
			iIsChannel = true;
		}
		else if(aChannelItem->GetItemType() == EItemRoster && aChannelItem->GetJid().Compare(iMessagingObject.iJid) == 0) {
			iIsPersonalChannel = true;
			iIsChannel = true;
		}
	}
	
	// Set size & load messages
	AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, iRect);
	SetRect(iRect);
	
	TimerExpired(0);
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

void CBuddycloudMessagingContainer::TopicChanged(TDesC& /*aTopic*/) {
	TimerExpired(0);
}

void CBuddycloudMessagingContainer::MessageDeleted(TInt aPosition) {
	if(aPosition >= 0 && aPosition < iMessages.Count()) {
		if(iSelectedItem > aPosition) {
			iSelectedItem--;
		}

		delete iMessages[aPosition];
		iMessages.Remove(aPosition);
	}
}

void CBuddycloudMessagingContainer::MessageReceived(CMessage* aMessage, TInt aPosition) {
	FormatWordWrap(aMessage, aPosition);
}

void CBuddycloudMessagingContainer::TimerExpired(TInt /*aExpiryId*/) {
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

void CBuddycloudMessagingContainer::ComposeNewMessageL(const TDesC& aPretext, CMessage* aReferenceMessage) {
	if(!iIsChannel || iDiscussion->GetMyRole() > ERoleVisitor) {
		HBufC* aMessage = HBufC::NewLC(1024);
		TPtr pMessage(aMessage->Des());
		pMessage.Append(aPretext);
	
		if(!iIsChannel) {
			iBuddycloudLogic->SetChatStateL(EChatComposing, iMessagingObject.iJid);
		}
	
		CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(pMessage, CAknQueryDialog::ENoTone);
		aDialog->SetPredictiveTextInputPermitted(true);
	
		if(pMessage.Length() > 0) {
			aDialog->PrepareLC(R_MESSAGING_LOWER_DIALOG);
		}
		else {
			aDialog->PrepareLC(R_MESSAGING_UPPER_DIALOG);
		}
	
		if(aDialog->RunLD() != 0) {
			iJumpToUnreadMessage = true;
			
			iBuddycloudLogic->BuildNewMessageL(iMessagingObject.iJid, pMessage, iIsChannel, aReferenceMessage);
			
			if(!iIsChannel) {
				RepositionItems(iSnapToItem);
				RenderScreen();
			}
		}
		else if(!iIsChannel) {
			iBuddycloudLogic->SetChatStateL(EChatActive, iMessagingObject.iJid);
		}
		
		CleanupStack::PopAndDestroy(); // aMessage
	}
}

TBool CBuddycloudMessagingContainer::OpenMessageLinkL() {
	CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

	if(aMessage && aMessage->GetLinkCount() > 0) {
		CMessageLinkPosition aLinkPosition = aMessage->GetLink(iSelectedLink);
		
		if(aLinkPosition.iType == ELinkWebsite) {
			CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
			TPtrC aLinkText(aMessage->GetBody().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
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

void CBuddycloudMessagingContainer::FormatWordWrap(CMessage* aMessageItem, TInt aPosition) {
	const TInt aMaxWidth = iRect.Width() - 8 - iScrollbarOffset - iScrollbarWidth;
	CFont* aUsedFont = i10NormalFont;

	if(aMessageItem->GetMessageType() == EMessageUserAction) {
		aUsedFont = i10ItalicFont;
	}
	else if(aMessageItem->GetMessageType() == EMessageAdvertisment) {
		aUsedFont = i10BoldFont;
	}

	CFormattedBody* aFormattedMessage = new CFormattedBody();
	CleanupStack::PushL(aFormattedMessage);
	
	// Wrap
	iTextUtilities->WrapToArrayL(aFormattedMessage->iLines, aUsedFont, aMessageItem->GetBody(), aMaxWidth);
	iMessages.Insert(aFormattedMessage, aPosition);
	
	CleanupStack::Pop(); // aFormattedMessage
}

void CBuddycloudMessagingContainer::GetHelpContext(TCoeHelpContext& aContext) const {	
	CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

	aContext.iMajor = TUid::Uid(HLPUID);
		
	if(aMessage && aMessage->GetLinkCount() > 0) {
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

	// Message Word Wrap
	for(TInt x = 0; x < iMessages.Count(); x++) {
		if(iMessages[x])
			delete iMessages[x];
	}

	iMessages.Reset();

	for(TInt i = 0; i < iDiscussion->GetTotalMessages(); i++) {
		FormatWordWrap(iDiscussion->GetMessage(i), i);
	}
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
}

void CBuddycloudMessagingContainer::RenderSelectedText(TInt& aDrawPos) {
	CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
	
	if(aMessage) {
		const TInt aMaxWidth = iRect.Width() - 8 - iScrollbarOffset - iScrollbarWidth;
		const TInt aTypingStartPos = iScrollbarOffset + 5;
		CFont* aRenderingFont = i10NormalFont;
	
		if(aMessage->GetMessageType() == EMessageAdvertisment) {
			aRenderingFont = i10BoldFont;
		}
		
		iBufferGc->UseFont(aRenderingFont);
		
		// Link positions
		TInt aCurrentLinkNumber = 0;
		CMessageLinkPosition aCurrentLink = aMessage->GetLink(aCurrentLinkNumber);
		TPtrC pCurrentLink(aMessage->GetBody().Mid(aCurrentLink.iStart, aCurrentLink.iLength));

#ifdef __SERIES60_40__
		// Initialize link regions
		iItemLinks.Reset();
		
		for(TInt i = 0; i < aMessage->GetLinkCount(); i++) {
			iItemLinks.Append(RRegion());
		}
#endif
		
		TPtrC pText(aMessage->GetBody());		
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

				if(aMessage->GetMessageType() == EMessageAdvertisment) {
					aTypingPos = ((iRect.Width() - aTypingStartPos) / 2) - (aRenderingFont->TextWidthInPixels(pTextLine) / 2);
				}
				
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
							aCurrentLink = aMessage->GetLink(++aCurrentLinkNumber);
							pCurrentLink.Set(aMessage->GetBody().Mid(aCurrentLink.iStart, aCurrentLink.iLength));							
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
	CMessage* aMessage = iDiscussion->GetMessage(aIndex);
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;

#ifdef __SERIES60_40__
	if( !aSelected ) {
		aItemSize += (i10BoldFont->DescentInPixels() * 2);
	}
#endif

	if(aMessage->GetMessageType() == EMessageAdvertisment) {
		// Advertisment message
		aItemSize += (i10BoldFont->HeightInPixels() * iMessages[aIndex]->iLines.Count());
	}
	else {
		if(aSelected) {
			aMinimumSize = iItemIconSize + 4;
			
			if(aMessage->GetMessageType() == EMessageTextual) {
				// From
				if(aMessage->GetName().Length() > 0) {
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
		
		if(aMessage->GetMessageType() == EMessageTextual) {
			// Textual message body
			aItemSize += (i10NormalFont->HeightInPixels() * iMessages[aIndex]->iLines.Count());			
		}
		else if(!aSelected) {
			// Unselected user action/event
			aItemSize += (i10ItalicFont->HeightInPixels() * iMessages[aIndex]->iLines.Count());			
		}
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
	
	CMessage* aMessage = iDiscussion->GetMessage(aIndex);
	
	if(aMessage) {
		if(aMessage->GetMessageType() == EMessageTextual) {
			TBuf<256> aBuf;
			
			// Time & date
			TTime aTimeReceived = aMessage->GetReceivedAt();
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
			
			// Location
			if(aMessage->GetLocation().Length() > 0) {
				aBuf.Append(_L(" ~ "));
				aBuf.Append(aMessage->GetLocation().Left(aBuf.MaxLength() - aBuf.Length()));
			}
			
			// Wrap
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aBuf, (iRect.Width() - iSelectedItemIconTextOffset - 5 - iScrollbarOffset - iScrollbarWidth));
		}		
		else if(aMessage->GetMessageType() >= EMessageUserAction) {
			// Wrap
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aMessage->GetBody(), (iRect.Width() - iSelectedItemIconTextOffset - 5 - iScrollbarOffset - iScrollbarWidth));
		}
	}
}

TInt CBuddycloudMessagingContainer::CalculateItemSize(TInt /*aIndex*/) {
	return 0;
}

void CBuddycloudMessagingContainer::RenderListItems() {
	TInt aDrawPos = -iScrollbarHandlePosition;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iDiscussion->GetTotalMessages() > 0) {
		TInt aItemSize = 0;
		TRect aItemRect;

#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iDiscussion->GetTotalMessages(); i++) {
			CMessage* aMessage = iDiscussion->GetMessage(i);

			// Calculate item size
			aItemSize = CalculateMessageSize(i, (i == iSelectedItem));

			// Test to start the page drawing
			if(aDrawPos + aItemSize > 0) {
				TInt aItemDrawPos = aDrawPos;

				if(!aMessage->GetRead() && aItemDrawPos + aItemSize <= iRect.Height()) {
					iDiscussion->SetMessageRead(i);
				}

				aItemRect = TRect(iScrollbarOffset + 1, aItemDrawPos, (iRect.Width() - iScrollbarWidth), (aItemDrawPos + aItemSize));
				
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
				
				if(aMessage->GetMessageType() == EMessageAdvertisment) {
					aRenderingFont = i10BoldFont;
				}
				else if(aMessage->GetMessageType() >= EMessageUserAction) {
					aRenderingFont = i10ItalicFont;
				}

				iBufferGc->SetClippingRect(TRect(iScrollbarOffset + 3, aItemDrawPos, (iRect.Width() - iScrollbarWidth - 2), iRect.Height()));
#ifdef __SERIES60_40__
				if(i != iSelectedItem) {
					aItemDrawPos += i10BoldFont->DescentInPixels();
				}
#endif
				
				if(aMessage->GetMessageType() != EMessageAdvertisment && i == iSelectedItem) {	
					// Avatar
					iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
					iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(aMessage->GetAvatarId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aMessage->GetAvatarId(), true, iIconMidmapSize), true);

					if(aMessage->GetMessageType() == EMessageTextual) {
						CMessagingParticipant* aParticipant = iDiscussion->GetParticipants()->GetParticipant(aMessage->GetJid());
						
						if(aParticipant && aParticipant->GetChatState() == EChatComposing) {
							// Chat State
							iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KOverlayTyping, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KOverlayTyping, true, iIconMidmapSize), true);
						}
						else if(aMessage->GetPrivate()) {
							// Private message
							iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KOverlayLocked, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KOverlayLocked, true, iIconMidmapSize), true);
						}
					}

					iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

					if(aMessage->GetMessageType() == EMessageTextual) {
						// Name
						TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aMessage->GetName()));
						
						if(i13BoldFont->TextCount(aDirectionalText, (iRect.Width() - iSelectedItemIconTextOffset - 2 - iScrollbarOffset - iScrollbarWidth)) < aDirectionalText.Length()) {
							iBufferGc->UseFont(i10BoldFont);
						}
						else {
							iBufferGc->UseFont(i13BoldFont);
						}
	
						aItemDrawPos += i13BoldFont->HeightInPixels();
						iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + iSelectedItemIconTextOffset, aItemDrawPos));
						aItemDrawPos += i13BoldFont->FontMaxDescent();
						iBufferGc->DiscardFont();
					}
					
					// Wrapped text
					iBufferGc->UseFont(i10ItalicFont);
						
					for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
						aItemDrawPos += i10ItalicFont->HeightInPixels();
						iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iScrollbarOffset + iSelectedItemIconTextOffset, aItemDrawPos));
					}
						
					iBufferGc->DiscardFont();
					
					if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
						aItemDrawPos = (aDrawPos + iItemIconSize + 2);
					}
				}
					
				// Render message text
				if(aMessage->GetMessageType() <= EMessageAdvertisment || i != iSelectedItem) {
					if(i == iSelectedItem && aMessage->GetLinkCount() > 0) {						
						RenderSelectedText(aItemDrawPos);
					}
					else {
						iBufferGc->UseFont(aRenderingFont);
						
						for(TInt x = 0; x < iMessages[i]->iLines.Count(); x++) {
							TPtrC aText(iMessages[i]->iLines[x]->Des());
							TInt aRenderingPosition = iScrollbarOffset + 5;
							
							if(aMessage->GetMessageType() == EMessageAdvertisment) {
								aRenderingPosition = ((iRect.Width() - aRenderingPosition) / 2) - (aRenderingFont->TextWidthInPixels(aText) / 2);
							}
							
							aItemDrawPos += aRenderingFont->HeightInPixels();
							iBufferGc->DrawText(aText, TPoint(aRenderingPosition, aItemDrawPos));
						}	
						
						iBufferGc->DiscardFont();
					}
				}

				iBufferGc->CancelClippingRect();
				aItemDrawPos = (aDrawPos + aItemSize - 1);

				// Separator line
				if((i != iSelectedItem) && ((i + 1) != iSelectedItem) && (i < (iDiscussion->GetTotalMessages() - 1))) {
					iBufferGc->SetPenColor(iColourHighlightBorder);
					iBufferGc->SetPenStyle(CGraphicsContext::EDashedPen);
					iBufferGc->DrawLine(TPoint(iScrollbarOffset + 5, aItemDrawPos), TPoint((iRect.Width() - iScrollbarWidth - 5), aItemDrawPos));
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
		iBufferGc->DrawText(KNoMessages, TPoint(iScrollbarOffset + ((iRect.Width() - iScrollbarOffset - iScrollbarWidth) / 2) - (i10BoldFont->TextWidthInPixels(KNoMessages) / 2), (iRect.Height() / 2) + (i10BoldFont->HeightInPixels() / 2)));
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
		
		if(iJumpToUnreadMessage) {
			iSelectedItem = KErrNotFound;
		}
	}
	
	if(iDiscussion->GetTotalMessages() > 0) {
		TInt aItemSize;

		// Check if current item exists
		if(iSelectedItem < 0 || iSelectedItem >= iDiscussion->GetTotalMessages()) {
			for(TInt i = 0; i < iDiscussion->GetTotalMessages(); i++) {
				iSelectedItem = i;
			
				if( !iDiscussion->GetMessage(i)->GetRead() ) {
					iJumpToUnreadMessage = false;
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
		
		for(TInt i = 0; i < iDiscussion->GetTotalMessages(); i++) {
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
		iJumpToUnreadMessage = false;
		
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
		if(iDiscussion->GetTotalMessages() == 0 || !OpenMessageLinkL()) {
			ComposeNewMessageL(_L(""));				
		}
	}
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar) {
	if(aResourceId == R_MESSAGING_TOOLBAR) {
		aToolbar->SetItemDimmed(EMenuReplyToMessageCommand, true, true);
		aToolbar->SetItemDimmed(EMenuPostMessageCommand, true, true);
		
		if(iIsChannel && iDiscussion->GetMyRole() > ERoleVisitor) {
			aToolbar->SetItemDimmed(EMenuPostMessageCommand, false, true);
			
			if(iDiscussion->GetTotalMessages() > 0) {
				CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

				if(aMessage && aMessage->GetMessageType() == EMessageTextual) {
					if(!aMessage->GetOwn() && aMessage->GetJid().Length() > 0) {							
						aToolbar->SetItemDimmed(EMenuReplyToMessageCommand, false, true);
					}
				}
			}
		}
		else if(!iIsChannel) {
			aToolbar->SetItemDimmed(EMenuPostMessageCommand, false, true);
		}
	}
}
#endif

void CBuddycloudMessagingContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_MESSAGING_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuPostMessageCommand, true);
		aMenuPane->SetItemDimmed(EMenuPostMediaCommand, true);
		aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsGroupCommand, true);
		
		if(!iIsChannel || iDiscussion->GetMyRole() > ERoleVisitor) {
			aMenuPane->SetItemDimmed(EMenuPostMessageCommand, false);
			
			if(iIsChannel) {
				aMenuPane->SetItemDimmed(EMenuPostMediaCommand, false);
			}
		}

		if(iDiscussion->GetUnreadMessages() > 0) {
			aMenuPane->SetItemDimmed(EMenuJumpToUnreadCommand, false);
		}

		if(iDiscussion->GetTotalMessages() > 0) {
			CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

			if(aMessage->GetMessageType() < EMessageUserAction) {
				if(!aMessage->GetOwn() && aMessage->GetJid().Length() > 0) {						
					aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aMessage->GetName().Left(32));
					aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
				}
			}
		}

		if(iIsChannel) {
			aMenuPane->SetItemDimmed(EMenuOptionsGroupCommand, false);
		}
				
		if(iDiscussion->GetNotificationOn()) {
			aMenuPane->SetItemDimmed(EMenuNotificationOnCommand, true);
			aMenuPane->SetItemDimmed(EMenuNotificationOffCommand, false);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuNotificationOnCommand, false);
			aMenuPane->SetItemDimmed(EMenuNotificationOffCommand, true);
		}
	}
	else if(aResourceId == R_MESSAGING_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuReplyToMessageCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuInviteToGroupCommand, true);
		aMenuPane->SetItemDimmed(EMenuBanCommand, true);
		aMenuPane->SetItemDimmed(EMenuKickCommand, true);
		aMenuPane->SetItemDimmed(EMenuChangeRoleCommand, true);
		
		if(iIsChannel || iDiscussion->GetMyRole() > ERoleVisitor) {
			aMenuPane->SetItemDimmed(EMenuReplyToMessageCommand, false);
		}

		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
			
		if(aMessage) {
			if(aMessage->GetJidType() == EMessageJidRoster) {
				// Follow
				if(!iBuddycloudLogic->IsSubscribedTo(iBuddycloudLogic->GetRawJid(aMessage->GetJid()), EItemRoster)) {
					aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
				}
				
				if(iIsChannel && !iIsPersonalChannel) {							
					// Affiliation management
					if(iDiscussion->GetMyAffiliation() >= EAffiliationAdmin) {
						aMenuPane->SetItemDimmed(EMenuBanCommand, false);
						
						if(iDiscussion->GetMyAffiliation() == EAffiliationOwner) {
							aMenuPane->SetItemDimmed(EMenuChangeRoleCommand, false);
						}
					}
				}
				
				aMenuPane->SetItemDimmed(EMenuInviteToGroupCommand, false);
			}
			
			if(iIsChannel) {
				// Role management
				if(iDiscussion->GetMyRole() == ERoleModerator) {
					aMenuPane->SetItemDimmed(EMenuKickCommand, false);
				}
			}
		}
		
	}
	else if(aResourceId == R_MESSAGING_OPTIONS_GROUP_MENU) {
		aMenuPane->SetItemDimmed(EMenuSetTopicCommand, true);
		aMenuPane->SetItemDimmed(EMenuInviteCommand, true);
		
		if(!iIsPersonalChannel) {
			aMenuPane->SetItemDimmed(EMenuInviteCommand, false);
			
			if(iDiscussion->GetMyAffiliation() >= EAffiliationAdmin) {
				aMenuPane->SetItemDimmed(EMenuSetTopicCommand, false);
			}
		}
	}
	else if(aResourceId == R_MESSAGING_FOLLOW_MENU) {
		aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

		if(aMessage && aMessage->GetLinkCount() > 0) {
			CMessageLinkPosition aLinkPosition = aMessage->GetLink(iSelectedLink);
			
			if(aLinkPosition.iType != ELinkWebsite) {
				HBufC* aJid = HBufC::NewLC(aLinkPosition.iLength + KBuddycloudChannelsServer().Length());
				TPtr pJid(aJid->Des());
				pJid.Append(aMessage->GetBody().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
					
				if(aLinkPosition.iType == ELinkUsername) {
					pJid.Append(KBuddycloudRosterServer);
				}
				else {
					pJid.Append(KBuddycloudChannelsServer);
				}
				
				TInt aItemId = iBuddycloudLogic->IsSubscribedTo(pJid, EItemRoster|EItemChannel);
				
				if(aItemId == 0) {
					// Not yet following
					aMenuPane->SetItemDimmed(EMenuFollowLinkCommand, false);
				}
				else {
					CFollowingItem* aItem = iBuddycloudLogic->GetFollowingStore()->GetItemById(aItemId);
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
						
						if(aChannelItem->GetJid().Length() > 0) {
							aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
						}
						
						if(aItem->GetItemType() == EItemRoster) {
							aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
						}					
					}
				}
				
				CleanupStack::PopAndDestroy(); // aJid
			}
		}
	}
}

void CBuddycloudMessagingContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuPostMessageCommand) {
		ComposeNewMessageL(_L(""));
	}
	else if(aCommand == EMenuPostMediaCommand) {
		iBuddycloudLogic->MediaPostRequestL(iMessagingObject.iJid);
		
		iJumpToUnreadMessage = true;
	}
	else if(aCommand == EMenuReplyToMessageCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
		TInt aLocateResult = KErrNotFound;
		
		if(aMessage) {
			// Get @reply: text
			HBufC* aAddressTo = HBufC::NewLC(aMessage->GetJid().Length() + 3);
			TPtr pAddressTo(aAddressTo->Des());			
			pAddressTo.Append(aMessage->GetJid());
			
			if(aMessage->GetJidType() == EMessageJidRoster && (aLocateResult = pAddressTo.Locate('@')) != KErrNotFound) {
				pAddressTo.Delete(aLocateResult, pAddressTo.Length());
			}
			else if(aMessage->GetJidType() == EMessageJidRoom && (aLocateResult = pAddressTo.LocateReverse('/')) != KErrNotFound) {
				pAddressTo.Delete(0, aLocateResult + 1);				
			}
			
			if((aLocateResult = pAddressTo.Locate(' ')) != KErrNotFound) {
				pAddressTo.Delete(aLocateResult, pAddressTo.Length());
			}
			
			pAddressTo.Insert(0, _L("@"));
			pAddressTo.Append(_L(" "));

			ComposeNewMessageL(pAddressTo, aMessage);
			CleanupStack::PopAndDestroy(); // aAddressTo
		}
	}
	else if(aCommand == EMenuJumpToUnreadCommand) {
		for(TInt i = 0; i < iDiscussion->GetTotalMessages(); i++) {
			if( !iDiscussion->GetMessage(i)->GetRead() ) {
				HandleItemSelection(i);
				
				break;
			}
		}
	}
	else if(aCommand == EMenuSetTopicCommand) {
		TBuf<32> aTopic;

		CAknTextQueryDialog* dlg = CAknTextQueryDialog::NewL(aTopic, CAknQueryDialog::ENoTone);
		dlg->SetPredictiveTextInputPermitted(true);

		if(dlg->ExecuteLD(R_TOPIC_DIALOG) != 0) {
			iBuddycloudLogic->SetTopicL(aTopic, iMessagingObject.iJid, iIsChannel);
		}
	}
	else if(aCommand == EMenuSeeFollowersCommand) {
		TPtrC8 aSubjectJid = iTextUtilities->UnicodeToUtf8L(iMessagingObject.iJid);
		
		TExplorerQuery aQuery;
		aQuery.iStanza.Append(_L8("<iq to='maitred.buddycloud.com' type='get' id='exp_users1'><query xmlns='http://buddycloud.com/protocol/channels#followers'><channel><jid></jid></channel></query></iq>"));
		aQuery.iStanza.Insert(138, aSubjectJid);
		
		iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_FOLLOWEDBY);
		aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, iMessagingObject.iTitle.Left(aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 7));
	
		TExplorerQueryPckg aQueryPckg(aQuery);		
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iMessagingObject.iFollowerId), aQueryPckg);		
	}
	else if(aCommand == EMenuFollowCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
		
		if(aMessage) {
			iBuddycloudLogic->FollowContactL(iBuddycloudLogic->GetRawJid(aMessage->GetJid()));
		}
	}
	else if(aCommand == EMenuFollowLinkCommand || aCommand == EMenuChannelMessagesCommand || aCommand == EMenuPrivateMessagesCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);

		if(aMessage && aMessage->GetLinkCount() > 0) {
			CMessageLinkPosition aLinkPosition = aMessage->GetLink(iSelectedLink);
			
			if(aLinkPosition.iType != ELinkWebsite) {
				TPtrC aLinkText(aMessage->GetBody().Mid(aLinkPosition.iStart, aLinkPosition.iLength));
				
				if(aCommand == EMenuFollowLinkCommand) {
					if(aLinkPosition.iType == ELinkUsername) {
						iBuddycloudLogic->FollowContactL(aLinkText);
					}
					else {
						iBuddycloudLogic->CreateChannelL(aLinkText);
					}
				}
				else {
					HBufC* aJid = HBufC::NewLC(aLinkText.Length() + KBuddycloudChannelsServer().Length());
					TPtr pJid(aJid->Des());
					pJid.Append(aLinkText);
						
					if(aLinkPosition.iType == ELinkUsername) {
						pJid.Append(KBuddycloudRosterServer);
					}
					else {
						pJid.Append(KBuddycloudChannelsServer);
					}
					
					CFollowingItem* aItem = iBuddycloudLogic->GetFollowingStore()->GetItemById(iBuddycloudLogic->IsSubscribedTo(pJid, EItemRoster|EItemChannel));
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {
						iMessagingObject.iFollowerId = aItem->GetItemId();
						iMessagingObject.iTitle = aItem->GetTitle();
						iMessagingObject.iJid = pJid;
						
						if(aCommand == EMenuChannelMessagesCommand && aItem->GetItemType() == EItemRoster) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
							
							iMessagingObject.iJid = aRosterItem->GetJid(EJidChannel);
						}
						
						InitializeMessageDataL();
						RenderScreen();
					}
					
					CleanupStack::PopAndDestroy(); // aJid
				}
			}
		}
	}
	else if(aCommand == EMenuInviteToGroupCommand) {
		if(iDiscussion->GetTotalMessages() > 0) {
			CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
			
			if(aMessage->GetJid().Locate('@') != KErrNotFound) {
				iBuddycloudLogic->InviteToChannelL(aMessage->GetJid(), iMessagingObject.iFollowerId);
			}
			else {
				iBuddycloudLogic->InviteToChannelL(iMessagingObject.iFollowerId);
			}
		}
		else {
			iBuddycloudLogic->InviteToChannelL(iMessagingObject.iFollowerId);
		}
	}
	else if(aCommand == EMenuInviteCommand) {
		CFollowingItem* aItem = iBuddycloudLogic->GetFollowingStore()->GetItemById(iMessagingObject.iFollowerId);
		
		if(aItem && aItem->GetItemType() >= EItemRoster) {
			if(aItem->GetItemType() == EItemChannel) {
				iBuddycloudLogic->InviteToChannelL(iMessagingObject.iFollowerId);
			}
			else {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
				iBuddycloudLogic->InviteToChannelL(aRosterItem->GetJid(), iMessagingObject.iFollowerId);
			}
		}
	}
	else if(aCommand == EMenuNotificationOnCommand || aCommand == EMenuNotificationOffCommand) {
		iDiscussion->SetNotificationOn(aCommand == EMenuNotificationOnCommand);
	}
	else if(aCommand == EMenuBanCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
		
		if(aMessage) {
			HBufC* aHeaderText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_BANCONFIRMATION_TITLE);
			HBufC* aMessageText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_BANCONFIRMATION_TEXT);
			
			CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
			aDialog->PrepareLC(R_EXIT_DIALOG);
			aDialog->SetHeaderText(*aHeaderText);
			aDialog->SetMessageText(*aMessageText);
	
			if(aDialog->RunLD() != 0) {
				TPtrC aRawJid(iBuddycloudLogic->GetRawJid(aMessage->GetJid()));			
				iBuddycloudLogic->ChangeUsersChannelAffiliationL(aRawJid, iMessagingObject.iJid, EAffiliationOutcast);
			}
			
			CleanupStack::PopAndDestroy(2); // aMessageText, aHeaderText
		}
	}
	else if(aCommand == EMenuKickCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
		
		if(aMessage) {
			HBufC* aHeaderText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_KICKCONFIRMATION_TITLE);
			HBufC* aMessageText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_KICKCONFIRMATION_TEXT);
			
			CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
			aDialog->PrepareLC(R_EXIT_DIALOG);
			aDialog->SetHeaderText(*aHeaderText);
			aDialog->SetMessageText(*aMessageText);
	
			if(aDialog->RunLD() != 0) {
				if(aMessage->GetJidType() == EMessageJidRoster) {
					iBuddycloudLogic->ChangeUsersChannelRoleL(aMessage->GetJid(), iMessagingObject.iJid, ERoleNone);
				}
				else {
					TPtrC aResource(aMessage->GetJid());
					TInt aLocateResult = aResource.Locate('/');
					
					if(aLocateResult != KErrNotFound) {
						iBuddycloudLogic->ChangeUsersChannelRoleL(aResource.Mid(aLocateResult + 1), iMessagingObject.iJid, ERoleNone);
					}
				}
			}
			
			CleanupStack::PopAndDestroy(2); // aMessageText, aHeaderText
		}
	}
	else if(aCommand == EMenuChangeRoleCommand) {
		CMessage* aMessage = iDiscussion->GetMessage(iSelectedItem);
		
		if(aMessage) {
//			TInt aLocateResult = KErrNotFound;
			TInt aCurrentAffiliationIndex = 0;
			TInt aSelectedIndex = 0;
			
			// Check affiliation of user
			CMessagingParticipant* aParticipant = iDiscussion->GetParticipants()->GetParticipant(aMessage->GetJid());
			
			if(aParticipant) {
				if(aParticipant->GetAffiliation() == EAffiliationAdmin) {
					aCurrentAffiliationIndex = 1;
				}
//				else if(aParticipant->GetRole() > ERoleNone) {
//					aCurrentAffiliationIndex = (aParticipant->GetRole() - 1);
//				}
			}	
			
			// Show list dialog
			CAknListQueryDialog* aDialog = new( ELeave ) CAknListQueryDialog(&aSelectedIndex);
			aDialog->PrepareLC(R_LIST_OWNER_CHANGEROLE);
			aDialog->ListBox()->SetCurrentItemIndex(aCurrentAffiliationIndex);

			if(aDialog->RunLD()) {
				switch(aSelectedIndex) {
					// TODO: WA: Removal of visitor role
					case 0: // Follower
						iBuddycloudLogic->ChangeUsersChannelAffiliationL(iBuddycloudLogic->GetRawJid(aMessage->GetJid()), iMessagingObject.iJid, EAffiliationMember);
						break;
					case 1: // Moderator
						iBuddycloudLogic->ChangeUsersChannelAffiliationL(iBuddycloudLogic->GetRawJid(aMessage->GetJid()), iMessagingObject.iJid, EAffiliationAdmin);
						break;
					default:;					
//					case 0: // Visitor
//					case 1: // Contributor
//						if(aMessage->GetJidType() == EMessageJidRoster) {
//							iBuddycloudLogic->ChangeUsersChannelRoleL(aMessage->GetJid(), iMessagingObject.iJid, TMucRole(aSelectedIndex + 1));
//						}
//						else {
//							if((aLocateResult = aMessage->GetJid().Locate('/')) != KErrNotFound) {		
//								iBuddycloudLogic->ChangeUsersChannelRoleL(aMessage->GetJid().Mid(aLocateResult + 1), iMessagingObject.iJid, TMucRole(aSelectedIndex + 1));
//							}
//						}
//						break;
//					case 2: // Moderator
//						iBuddycloudLogic->ChangeUsersChannelAffiliationL(iBuddycloudLogic->GetRawJid(aMessage->GetJid()), iMessagingObject.iJid, EAffiliationAdmin);
//						break;
//					default:;
				}
			}
		}
	}
	else if(aCommand == EAknSoftkeyBack) {
		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(iMessagingObject.iFollowerId), _L8(""));
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
			else if(iSelectedItem < (iDiscussion->GetTotalMessages() - 1)) {
				HandleItemSelection(iSelectedItem + 1);
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyLeftArrow) {
			if(iDiscussion->GetTotalMessages() > 0) {
				if(iSelectedLink == 0) {
					iSelectedLink = iDiscussion->GetMessage(iSelectedItem)->GetLinkCount() - 1;
				}
				else {
					iSelectedLink--;
				}
				
				aRenderNeeded = true;
			}
			
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyRightArrow) {
			if(iDiscussion->GetTotalMessages() > 0) {
				if(iSelectedLink == iDiscussion->GetMessage(iSelectedItem)->GetLinkCount() - 1) {
					iSelectedLink = 0;
				}
				else {
					iSelectedLink++;
				}
				
				aRenderNeeded = true;
			}
			
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == 63557) { // Center Dpad
			HandleItemSelection(iSelectedItem);
			aResult = EKeyWasConsumed;
		}

		if(aResult == EKeyWasNotConsumed && aKeyEvent.iCode >= 32 && aKeyEvent.iCode <= 255) {
			aResult = EKeyWasConsumed;
			
			TBuf<8> aMessage;
	
			if(aKeyEvent.iCode >= 58) {
				aMessage.Append(TChar(aKeyEvent.iCode));
				aMessage.Capitalize();
			}
	
			ComposeNewMessageL(aMessage);
		}
	}
	
	if(aRenderNeeded) {
		RenderScreen();
	}

	return aResult;
}

#ifdef __SERIES60_40__
void CBuddycloudMessagingContainer::HandlePointerEventL(const TPointerEvent &aPointerEvent) {
	CCoeControl::HandlePointerEventL(aPointerEvent);
	
	if(aPointerEvent.iType == TPointerEvent::EButton1Up) {			
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
								OpenMessageLinkL();
							}
						}
					}
				}
				
				break;
			}
		}
	}
}
#endif

// End of File
