/*
============================================================================
 Name        : BuddycloudFollowingContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Following Container
============================================================================
*/

// INCLUDE FILES
#include <akniconutils.h>
#include <aknnavide.h>
#include <aknquerydialog.h>
#include <aknmessagequerydialog.h>
#include <bacline.h>
#include <barsread.h>
#include <cntitem.h>
#include <cntfldst.h>
#include <cntdef.h>
#include <eikmenup.h>
#include <txtfmlyr.h>
#include <txtfrmat.h>
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "ArrowIconIds.h"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudExplorer.h"
#include "BuddycloudFollowingContainer.h"
#include "BuddycloudMessagingContainer.h"
#include "TelephonyEngine.h"

// ================= MEMBER FUNCTIONS =======================

CBuddycloudFollowingContainer::CBuddycloudFollowingContainer(MViewAccessorObserver* aViewAccessor, 
		CBuddycloudLogic* aBuddycloudLogic) : CBuddycloudListComponent(aViewAccessor, aBuddycloudLogic) {
}

void CBuddycloudFollowingContainer::ConstructL(const TRect& aRect, TInt aItem) {
	iRect = aRect;
	iSelectedItem = aItem;

	PrecacheImagesL();
	CreateWindowL();

	// Tabs
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_DEFAULT_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	iTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	iTabGroup->SetActiveTabById(ETabFollowing);
	iTabGroup->SetObserver(this);

	iNaviPane->PushL(*iNaviDecorator);

	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	// Set title
	iTimer->After(15000000);
	HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_FOLLOWINGTAB_HINT);
	SetTitleL(*aTitle);
	CleanupStack::PopAndDestroy();

	// Search Field
	iSearchEdwin = new (ELeave) CEikEdwin();
	iSearchEdwin->MakeVisible(true);
	iSearchEdwin->SetFocus(true);
	iSearchEdwin->SetContainerWindowL(*this);

	TResourceReader aReader;
	iEikonEnv->CreateResourceReaderLC(aReader, R_SEARCH_EDWIN);
	iSearchEdwin->ConstructFromResourceL(aReader);
	CleanupStack::PopAndDestroy(); // aReader

	iSearchEdwin->SetAknEditorFlags(EAknEditorFlagNoT9 | EAknEditorFlagNoEditIndicators);
	iSearchEdwin->SetAknEditorCase(EAknEditorLowerCase);
	ConfigureEdwinL();
	ConfigureCbaAndMenuL();

	iBuddycloudLogic->AddStatusObserver(this);

	iItemStore = iBuddycloudLogic->GetFollowingStore();

	SetRect(iRect);	
	RenderScreen();
	ActivateL();
}

CBuddycloudFollowingContainer::~CBuddycloudFollowingContainer() {
	// Tabs
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;

	if(iBuddycloudLogic) {
		iBuddycloudLogic->RemoveStatusObserver();
	}

	// Search Field
	if(iSearchEdwin)
		delete iSearchEdwin;
	
	AknIconUtils::DestroyIconData(iArrow1Bitmap);	

	if(iArrow1Bitmap)
		delete iArrow1Bitmap;
	if(iArrow1Mask)
		delete iArrow1Mask;
	
	AknIconUtils::DestroyIconData(iArrow2Bitmap);	
	
	if(iArrow2Bitmap)
		delete iArrow2Bitmap;
	if(iArrow2Mask)
		delete iArrow2Mask;
}

void CBuddycloudFollowingContainer::PrecacheImagesL() {
	TFileName aFileName(_L(":\\resource\\apps\\Buddycloud_Arrow.mif"));
	
#ifndef __WINSCW__
	CCommandLineArguments* aArguments = CCommandLineArguments::NewLC();
	TFileName aDrive;
	if (aArguments->Count() > 0) {
		aDrive.Append(aArguments->Arg(0)[0]);
		aFileName.Insert(0, aDrive);
	}
	CleanupStack::PopAndDestroy();
#else
	aFileName.Insert(0, _L("z"));
#endif
	
	// Create bitmaps
	AknIconUtils::CreateIconL(iArrow1Bitmap, iArrow1Mask, aFileName, EMbmArrowiconidsArrow1, EMbmArrowiconidsArrow1);
	AknIconUtils::PreserveIconData(iArrow1Bitmap);	
	
	AknIconUtils::CreateIconL(iArrow2Bitmap, iArrow2Mask, aFileName, EMbmArrowiconidsArrow2, EMbmArrowiconidsArrow2);
	AknIconUtils::PreserveIconData(iArrow2Bitmap);	
}

void CBuddycloudFollowingContainer::ResizeCachedImagesL() {
	TSize aSvgSize;	

	// Calculate dimensions
	AknIconUtils::GetContentDimensions(iArrow1Bitmap, aSvgSize);
	iArrow1Size.SetSize(15, (i10NormalFont->HeightInPixels() + i10BoldFont->HeightInPixels()));
	AknIconUtils::SetSize(iArrow1Bitmap, iArrow1Size, EAspectRatioNotPreserved);
	iArrow1Size = iArrow1Bitmap->SizeInPixels();
	
	AknIconUtils::GetContentDimensions(iArrow2Bitmap, aSvgSize);
	iArrow2Size.SetSize(15, ((i10NormalFont->HeightInPixels() * 2) + i10BoldFont->HeightInPixels()));
	AknIconUtils::SetSize(iArrow2Bitmap, iArrow2Size, EAspectRatioNotPreserved);
	iArrow2Size = iArrow2Bitmap->SizeInPixels();
}

void CBuddycloudFollowingContainer::ConfigureEdwinL() {
	TCharFormat aCharFormat;
	TCharFormatMask aCharFormatMask;
	CCharFormatLayer* aCharFormatLayer = CEikonEnv::NewDefaultCharFormatLayerL();
	CleanupStack::PushL(aCharFormatLayer);
	
	aCharFormatLayer->Sense(aCharFormat, aCharFormatMask);
	aCharFormat.iFontPresentation.iTextColor = iColourText;
	aCharFormatMask.SetAll();
	aCharFormatLayer->SetL(aCharFormat, aCharFormatMask);
	iSearchEdwin->SetCharFormatLayer(aCharFormatLayer);  // Edwin takes the ownership
	CleanupStack::Pop(aCharFormatLayer);
}

void CBuddycloudFollowingContainer::DisplayEdwin(TBool aShowEdwin) {
	iRect = Rect();

	if(iSearchEdwin) {
		TRect aFieldRect = iSearchEdwin->Rect();
		
		if(aShowEdwin) {
			// Show edwin
			aFieldRect = TRect(i10NormalFont->FontMaxHeight(), (iRect.Height() - i10NormalFont->FontMaxHeight() - 2), (iRect.Width() - i10NormalFont->FontMaxHeight()), (iRect.Height() - 2));
			iSearchEdwin->SetRect(aFieldRect);
			
			iRect.SetHeight(iRect.Height() - iSearchEdwin->Rect().Height() - 6);
			
			RepositionItems(iSnapToItem);
			iSearchVisible = true;
		}
		else {
			// Dont show edwin
			aFieldRect = TRect(i10NormalFont->FontMaxHeight(), (iRect.Height() + 2), (iRect.Width() - i10NormalFont->FontMaxHeight()), (iRect.Height() + aFieldRect.Height() + 2));
			iSearchEdwin->SetFocus(false);
			iSearchEdwin->SetRect(aFieldRect);
			iSearchEdwin->SetFocus(true);
			
			if(iSearchVisible) {
				iSelectedItem = KErrNotFound;
				RepositionItems(true);
			}
			
			iSearchVisible = false;
		}
	}
}

void CBuddycloudFollowingContainer::ConfigureCbaAndMenuL() {
	if(iBuddycloudLogic->GetCall()->GetTelephonyState() > ETelephonyIdle) {
		CEikonEnv::Static()->RootWin().SetOrdinalPosition(0, ECoeWinPriorityAlwaysAtFront);
		
		iViewAccessor->ViewMenuBar()->StopDisplayingMenuBar();
		iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_CALL_OPTIONS_MENUBAR);
		iViewAccessor->ViewCba()->SetCommandSetL(R_SOFTKEYS_OPTIONS_ENDCALL);
		iViewAccessor->ViewCba()->DrawDeferred();
	}
	else {
		CEikonEnv::Static()->RootWin().SetOrdinalPosition(0, ECoeWinPriorityNormal);
		
		iViewAccessor->ViewMenuBar()->StopDisplayingMenuBar();
		iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_OPTIONS_MENUBAR);
		iViewAccessor->ViewCba()->SetCommandSetL(R_SOFTKEYS_OPTIONS_HIDE);
		iViewAccessor->ViewCba()->DrawDeferred();
	}
}

void CBuddycloudFollowingContainer::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationFollowingItemsUpdated) {
		if(aId == iSelectedItem) {
			RenderWrappedText(iSelectedItem);
		}
		
		RepositionItems(iSnapToItem);
		RenderScreen();
	}
	else if(aEvent == ENotificationFollowingItemDeleted) {
		if(aId == iSelectedItem) {
			TInt aIndex = iItemStore->GetIndexById(iSelectedItem);
			
			if(aIndex == iItemStore->Count() - 1) {
				iSelectedItem = iItemStore->GetIdByIndex(aIndex - 1);
			}
			else {
				iSelectedItem = iItemStore->GetIdByIndex(aIndex + 1);
			}
					
#ifdef __SERIES60_40__
			DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
			RenderWrappedText(iSelectedItem);
		}
		
		RenderScreen();
	}
	else if(aEvent == ENotificationTelephonyChanged) {
		ConfigureCbaAndMenuL();		
		RenderScreen();
	}
	else {
		CBuddycloudListComponent::NotificationEvent(aEvent, aId);
	}
}

void CBuddycloudFollowingContainer::JumpToItem(TInt aItemId) {
	iSelectedItem = aItemId;
	
#ifdef __SERIES60_40__
	DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
	
	RenderWrappedText(iSelectedItem);
	RepositionItems(true);
	RenderScreen();
}

void CBuddycloudFollowingContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KFollowingTab;
	
	if(iItemStore->Count() > 0) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem->GetItemType() == EItemContact) {
			aContext.iContext = KSendingInvites;			
		}
		else if(aItem->GetItemType() == EItemNotice) {
			CFollowingNoticeItem* aNoticeItem = static_cast <CFollowingNoticeItem*> (aItem);

			if(aNoticeItem->GetJidType() == EJidRoster) {
				aContext.iContext = KFollowingRequests;	
			}
			else {
				aContext.iContext = KChannelInvites;
			}
		}
		else if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(iBuddycloudLogic->GetMyMotionState() == EMotionStationary && iBuddycloudLogic->GetMyPlaceId() <= 0) {
				aContext.iContext = KNamingPlaces;
			}
			else {
				if(aRosterItem->GetIsOwn()) {				
					aContext.iContext = KMyBuddycloud;
				}
				else {
					CDiscussion* aDiscussion = iBuddycloudLogic->GetDiscussion(aRosterItem->GetJid(EJidChannel));				
					
					if(aDiscussion->GetUnreadReplies() > 0) {
						aContext.iContext = KDirectReplies;
					}
					else if(aDiscussion->GetUnreadMessages() > 0) {
						aContext.iContext = KViewingChannels;
					}
					else {
						aContext.iContext = KBuddycloudFollowing;
					}
				}
			}
		}
		else if(aItem->GetItemType() == EItemChannel) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);			
			CDiscussion* aDiscussion = iBuddycloudLogic->GetDiscussion(aChannelItem->GetJid());				
			
			if(aDiscussion->GetUnreadReplies() > 0) {
				aContext.iContext = KDirectReplies;
			}
			else if(aDiscussion->GetUnreadMessages() > 0) {
				aContext.iContext = KViewingChannels;
			}
			else {
				aContext.iContext = KBuddycloudChannels;
			}
		}
	}
}

void CBuddycloudFollowingContainer::HandleResourceChange(TInt aType) {
	CBuddycloudListComponent::HandleResourceChange(aType);
	
	if(aType == KAknsMessageSkinChange){
		ConfigureEdwinL();
	}
}

void CBuddycloudFollowingContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();
	
	ResizeCachedImagesL();
	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
	
	// Search Field
	if(iSearchEdwin) {
		iSearchEdwin->SetTextL(&iBuddycloudLogic->GetContactFilter());
		iSearchEdwin->HandleTextChangedL();

		if(iSearchEdwin->TextLength() > 0 || iSearchVisible) {
			// Show
			DisplayEdwin(true);
		}
		else {
			// Dont Show
			DisplayEdwin(false);
		}
	}
}

TInt CBuddycloudFollowingContainer::CountComponentControls() const {
	return 1; // iSearchEdwin
}

CCoeControl* CBuddycloudFollowingContainer::ComponentControl(TInt aIndex) const {
	switch (aIndex) {
		case 0:
			return iSearchEdwin;
		default:
			return NULL;
	}
}

void CBuddycloudFollowingContainer::RenderWrappedText(TInt aIndex) {
	// Clear
	ClearWrappedText();
	
	iNewMessagesOffset = 0;

	if(iItemStore->Count() > 0) {
		CFollowingItem* aItem = iItemStore->GetItemById(aIndex);
		TInt aWidth = (iRect.Width() - iSelectedItemIconTextOffset - 2 - iScrollbarOffset - iScrollbarWidth);
		TBuf<32> aBuf;
		
		if(aItem) {	
			if(aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
	
				if(aChannelItem->GetUnread() > 0) {
					aBuf.Format(_L("%d"), aChannelItem->GetUnread());
					
					iNewMessagesOffset = (i10NormalFont->TextWidthInPixels(aBuf) + (iItemIconSize / 2) + 6);
				}
				
				if(aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
		
					if(aRosterItem->GetUnread() > 0) {
						aBuf.Format(_L("%d"), aRosterItem->GetUnread());
						
						if((i10NormalFont->TextWidthInPixels(aBuf) + (iItemIconSize / 2) + 6) > iNewMessagesOffset) {
							iNewMessagesOffset = (i10NormalFont->TextWidthInPixels(aBuf) + (iItemIconSize / 2) + 6);
						}
					}
				}
			}
			
			aWidth -= iNewMessagesOffset;
	
			// Wrap
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aItem->GetDescription(), aWidth);
		}
	}
	else {
		// No following items
		TInt aWidth = (iRect.Width() - 2 - iScrollbarOffset - iScrollbarWidth);

		HBufC* aNoItems = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_FOLLOWINGLISTEMPTY);		
		iTextUtilities->WrapToArrayL(iWrappedTextArray, i10BoldFont, *aNoItems, aWidth);		
		CleanupStack::PopAndDestroy();	// aNoItems
	}
}

TBool CBuddycloudFollowingContainer::IsFilteredItem(TInt aIndex) {
	CFollowingItem* aItem = iItemStore->GetItemByIndex(aIndex);

	if(aItem) {
		return aItem->IsFiltered();
	}

	return false;
}

TInt CBuddycloudFollowingContainer::CalculateItemSize(TInt aIndex) {
	CFollowingItem* aItem = iItemStore->GetItemByIndex(aIndex);
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;

	if(aItem) {
		if(aItem->GetItemId() == iSelectedItem) {
			aMinimumSize = iItemIconSize + 4;
			
			// Title
			aItemSize += i13BoldFont->HeightInPixels();
			
			// Description
			if(aItem->GetDescription().Length() > 0) {
				aItemSize += i13BoldFont->FontMaxDescent();
				aItemSize += (iWrappedTextArray.Count() * i10ItalicFont->HeightInPixels());
			}
			
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				// Place history
				if(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName().Length() > 0 || 
						aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0 || 
						aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {	
					
					if(aItemSize < (iItemIconSize + 2)) {
						aItemSize = (iItemIconSize + 2);
					}
					
					aItemSize += i10NormalFont->HeightInPixels();
					aItemSize += i10BoldFont->HeightInPixels();
					
					if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
						aItemSize += i10NormalFont->HeightInPixels();
					}
				}
			}
		}
		else {
			aMinimumSize = (iItemIconSize / 2) + 2;
#ifdef __SERIES60_40__
			aMinimumSize += (i10BoldFont->DescentInPixels() * 2);
			aItemSize += (i10BoldFont->DescentInPixels() * 2);
#endif
			
			aItemSize += i10BoldFont->HeightInPixels();

			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
					aItemSize += i10BoldFont->FontMaxDescent();
					aItemSize += i10NormalFont->HeightInPixels();
				}
			}
		}
		
		aItemSize += i10NormalFont->FontMaxDescent() + 2;
	}
	
	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudFollowingContainer::RenderListItems() {
	TBuf<256> aBuf;
	TInt aDrawPos = -iScrollbarHandlePosition;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iItemStore->Count() > 0 && iTotalListSize > 0) {
		TBool aDynamicContentRequested = false;
		TInt aItemSize = 0;
		
#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iItemStore->Count(); i++) {
			CFollowingItem* aItem = iItemStore->GetItemByIndex(i);

			if(aItem) {
				if(IsFilteredItem(i)) {
					// Calculate item size
					aItemSize = CalculateItemSize(i);
					
					// Test to start the page drawing
					if(aDrawPos + aItemSize > 0) {
						TInt aItemDrawPos = aDrawPos;
						
#ifdef __SERIES60_40__
						iListItems.Append(TListItem(aItem->GetItemId(), TRect(0, aItemDrawPos, (Rect().Width() - iScrollbarWidth), (aItemDrawPos + aItemSize))));
#endif

						if(aItem->GetItemType() == EItemContact && aItem->GetTitle().Length() == 0) {
							// Collect contact details if empty
							iBuddycloudLogic->CollectContactDetailsL(aItem->GetItemId());
						}
						
						TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aItem->GetTitle()));
						
						if(iSelectedItem == aItem->GetItemId()) {
							// Currently selected item
							TRect aRect = TRect(iScrollbarOffset + 1, aItemDrawPos, (iRect.Width() - iScrollbarWidth), (aItemDrawPos + aItemSize));
							RenderItemFrame(aRect);
							
							iBufferGc->SetPenColor(iColourTextSelected);
							
							if(aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel) {
								// Drawing of unread messages
								TInt aUnreadPos = aItemDrawPos + 2;
								CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
								
								iBufferGc->UseFont(i10NormalFont);
							
								if(aChannelItem->GetUnread() > 0) {
									// Draw channel messages
									aBuf.Format(_L("%d"), aChannelItem->GetUnread());
									
									// Message Icon
									iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
									iBufferGc->BitBltMasked(TPoint((iRect.Width() - iScrollbarWidth - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KIconMessage2, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage2, true, iIconMidmapSize), true);								
									
									if(aChannelItem->GetReplies() > 0) {
										// Draw direct reply icon
										iBufferGc->BitBltMasked(TPoint((iRect.Width() - iScrollbarWidth - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KOverlayReply, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KOverlayReply, true, iIconMidmapSize), true);
									}

									iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
									
									// Unread number
									iBufferGc->DrawText(aBuf, TPoint((iRect.Width() - iScrollbarWidth - 3 - (iItemIconSize / 2) - i10NormalFont->TextWidthInPixels(aBuf)), aUnreadPos + (i10NormalFont->HeightInPixels() / 2) + (iItemIconSize / 4)));
									aUnreadPos += (iItemIconSize / 2);									
								}
								
								if(aItem->GetItemType() == EItemRoster) {
									CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
									
									if(aRosterItem->GetUnread() > 0) {
										// Draw private messages
										aBuf.Format(_L("%d"), aRosterItem->GetUnread());
										
										// Message Icon
										iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
										iBufferGc->BitBltMasked(TPoint((iRect.Width() - iScrollbarWidth - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KIconMessage1, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage1, true, iIconMidmapSize), true);
										iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
										
										// Unread number
										iBufferGc->DrawText(aBuf, TPoint((iRect.Width() - iScrollbarWidth - 3 - (iItemIconSize / 2) - i10NormalFont->TextWidthInPixels(aBuf)), aUnreadPos + (i10NormalFont->HeightInPixels() / 2) + (iItemIconSize / 4)));
									}
								}
								
								iBufferGc->DiscardFont();	
							}
							
							// Draw avatar
							iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
							iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 6, aItemDrawPos + 2), iAvatarRepository->GetImage(aItem->GetAvatarId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aItem->GetAvatarId(), true, iIconMidmapSize), true);
							iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
							
							aRect = TRect(iScrollbarOffset + 3, aItemDrawPos, (iRect.Width() - iScrollbarWidth - iNewMessagesOffset - 3), iRect.Height());
							iBufferGc->SetClippingRect(aRect);

							// Name
							if(i13BoldFont->TextCount(aDirectionalText, (iRect.Width() - iScrollbarOffset - iScrollbarWidth - iNewMessagesOffset - 3 - iSelectedItemIconTextOffset)) < aDirectionalText.Length()) {
								iBufferGc->UseFont(i10BoldFont);
							}
							else {
								iBufferGc->UseFont(i13BoldFont);
							}

							aItemDrawPos += i13BoldFont->HeightInPixels();
							iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + iSelectedItemIconTextOffset, aItemDrawPos));
							aItemDrawPos += i13BoldFont->FontMaxDescent();
							iBufferGc->DiscardFont();

							// Description
							if(aItem->GetDescription().Length() > 0) {
								iBufferGc->UseFont(i10ItalicFont);
								
								for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
									aItemDrawPos += i10ItalicFont->HeightInPixels();
									iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iScrollbarOffset + iSelectedItemIconTextOffset, aItemDrawPos));
								}
								
								iBufferGc->DiscardFont();
							}
							
							if(aItem->GetItemType() == EItemRoster) {
								CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
								
								// Place history
								if(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName().Length() > 0 || 
										aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0 ||
										aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
									
									if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
										aItemDrawPos = (aDrawPos + iItemIconSize + 2);
									}
									
									iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
									
									if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
										iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 9, aItemDrawPos), iArrow2Bitmap, TRect(0, 0, iArrow2Size.iWidth, iArrow2Size.iHeight), iArrow2Mask, true);
									}
									else {
										iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + 9, aItemDrawPos), iArrow1Bitmap, TRect(0, 0, iArrow1Size.iWidth, iArrow1Size.iHeight), iArrow1Mask, true);
									}
									
									iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
									
									// Previous place
									iBufferGc->UseFont(i10NormalFont);
									iBufferGc->SetStrikethroughStyle(EStrikethroughOn);
									aItemDrawPos += i10NormalFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName()));
									iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + 30, aItemDrawPos));
									iBufferGc->SetStrikethroughStyle(EStrikethroughOff);
									iBufferGc->DiscardFont();

									// Current place
									iBufferGc->UseFont(i10BoldFont);
									aItemDrawPos += i10BoldFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName()));
									iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + 30, aItemDrawPos));
									iBufferGc->DiscardFont();

									// Next place
									if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
										iBufferGc->UseFont(i10NormalFont);
										aItemDrawPos += i10NormalFont->HeightInPixels();
										aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName()));
										iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + 30, aItemDrawPos));
										iBufferGc->DiscardFont();
									}
								}
								
								// Request pubsub if not collected
								if( !aDynamicContentRequested && !aRosterItem->GetPubsubCollected() ) {
									iBuddycloudLogic->GetFriendDetailsL(aRosterItem->GetItemId());
									aDynamicContentRequested = true;
								}
							}							
						}
						else {
							// Non selected items
							TRect aRect = TRect(0, aItemDrawPos, iRect.Width(), iRect.Height());
#ifdef __SERIES60_40__
							aItemDrawPos += i10BoldFont->DescentInPixels();
#endif					
							iBufferGc->SetPenColor(iColourText);
							
							iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
							
							if(aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel) {
								TInt aMessageIconOffset = (iScrollbarWidth + 3);
							
								// Private messages
								if(aItem->GetItemType() == EItemRoster) {
									CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
									
									if(aRosterItem->GetUnread() > 0) {
										// Draw private message icon
										aMessageIconOffset += (iItemIconSize / 2);
										iBufferGc->BitBltMasked(TPoint(iRect.Width() - aMessageIconOffset, (aItemDrawPos + 1)), iAvatarRepository->GetImage(KIconMessage1, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage1, true, iIconMidmapSize), true);										
									}
								}
								
								// Channel messages
								CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
															
								if(aChannelItem->GetUnread() > 0) {
									// Draw private message icon
									aMessageIconOffset += (iItemIconSize / 2);
									iBufferGc->BitBltMasked(TPoint(iRect.Width() - aMessageIconOffset, (aItemDrawPos + 1)), iAvatarRepository->GetImage(KIconMessage2, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage2, true, iIconMidmapSize), true);
								
									if(aChannelItem->GetReplies() > 0) {
										// Draw direct reply icon
										iBufferGc->BitBltMasked(TPoint(iRect.Width() - aMessageIconOffset, (aItemDrawPos + 1)), iAvatarRepository->GetImage(KOverlayReply, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KOverlayReply, true, iIconMidmapSize), true);
									}
								}
	
								aRect.SetWidth(aRect.Width() - aMessageIconOffset - 1);
							}
							
							// Draw avatar
							iBufferGc->BitBltMasked(TPoint(iScrollbarOffset + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(aItem->GetAvatarId(), false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(aItem->GetAvatarId(), true, (iIconMidmapSize + 1)), true);							
							iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);							
							
							iBufferGc->SetClippingRect(aRect);

							// Title
							iBufferGc->UseFont(i10BoldFont);
							aItemDrawPos += i10BoldFont->HeightInPixels();
							iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + iUnselectedItemIconTextOffset, aItemDrawPos));
							iBufferGc->DiscardFont();
							
							if(aItem->GetItemType() == EItemRoster) {
								CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
								
								if(!aRosterItem->GetPubsubCollected()) {
									iBufferGc->SetPenColor(iColourTextSelectedTrans);
								}
								
								// Current place
								if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
									if( !aRosterItem->GetPubsubCollected() ) {
										iBufferGc->SetPenColor(iColourTextTrans);
									}
	
									iBufferGc->UseFont(i10NormalFont);
									aItemDrawPos += i10BoldFont->FontMaxDescent();
									aItemDrawPos += i10NormalFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName()));
									iBufferGc->DrawText(aDirectionalText, TPoint(iScrollbarOffset + iUnselectedItemIconTextOffset, aItemDrawPos));
									iBufferGc->DiscardFont();
								}
								
								// Request pubsub if not collected
								if( !aDynamicContentRequested && !aRosterItem->GetPubsubCollected() ) {
									iBuddycloudLogic->GetFriendDetailsL(aRosterItem->GetItemId());
									aDynamicContentRequested = true;
								}
							}
						}

						iBufferGc->CancelClippingRect();
					}
					
					aDrawPos += (aItemSize + 1);
				}

				// Finish page drawing
				if(aDrawPos > iRect.Height()) {
					break;
				}
			}
		}

		iBufferGc->CancelClippingRect();
	}
	else {
		iBufferGc->UseFont(i10BoldFont);
		iBufferGc->SetPenColor(iColourText);

		if(iItemStore->Count() > 0) {
			// No results
			aDrawPos = ((iRect.Height() / 2) - (i10BoldFont->HeightInPixels() / 2));
			
			HBufC* aNoResults = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);		
			iBufferGc->DrawText(*aNoResults, TPoint(((iScrollbarOffset + ((iRect.Width() - iScrollbarOffset - iScrollbarWidth) / 2)) - (i10BoldFont->TextWidthInPixels(*aNoResults) / 2)), aDrawPos));
			CleanupStack::PopAndDestroy();// aNoResults	
		}
		else {
			// No items
			aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * i10BoldFont->HeightInPixels()) / 2));
						
			for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
				aDrawPos += i10ItalicFont->HeightInPixels();
				iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iScrollbarOffset + ((iRect.Width() - iScrollbarOffset - iScrollbarWidth) / 2)) - (i10BoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2)), aDrawPos));
			}
		}
		
		iBufferGc->DiscardFont();
		
#ifdef __SERIES60_40__
		DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
	}
	
	// Draw Search Field
	if(iSearchEdwin->IsVisible()) {
		iBufferGc->SetClippingRect(Rect());

		TRect aFieldRect = iSearchEdwin->Rect();
		aFieldRect.Grow(2, 2);
		iBufferGc->SetBrushColor(iColourText);
		iBufferGc->SetPenColor(iColourText);
		iBufferGc->DrawRoundRect(aFieldRect, TSize(2, 2));
	}
}

void CBuddycloudFollowingContainer::RepositionItems(TBool aSnapToItem) {
	iTotalListSize = 0;
	
	// Enable snapping again
	if(aSnapToItem) {
		iSnapToItem = aSnapToItem;
		iScrollbarHandlePosition = 0;
	}
	
	if(iItemStore->Count() > 0) {
		TInt aItemSize;

		// Check if current item exists
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem == NULL || !aItem->IsFiltered()) {
			iSelectedItem = KErrNotFound;
		}
		
		for(TInt i = 0; i < iItemStore->Count(); i++) {
			// If item is in filtering list
			if(IsFilteredItem(i)) {
				if(iSelectedItem == KErrNotFound) {
					iSelectedItem = iItemStore->GetItemByIndex(i)->GetItemId();
					
#ifdef __SERIES60_40__
					DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
					RenderWrappedText(iSelectedItem);		
				}
				
				aItemSize = CalculateItemSize(i);
				
				if(aSnapToItem && iItemStore->GetItemByIndex(i)->GetItemId() == iSelectedItem) {
					if(iTotalListSize + (aItemSize / 2) > (iRect.Height() / 2)) {
						iScrollbarHandlePosition = (iTotalListSize + (aItemSize / 2)) - (iRect.Height() / 2);
					}
				}
				
				// Increment total list size
				iTotalListSize += (aItemSize + 1);
			}
		}
	}
	
	CBuddycloudListComponent::RepositionItems(aSnapToItem);
}

void CBuddycloudFollowingContainer::HandleItemSelection(TInt aItemId) {
	if(iSelectedItem != aItemId) {
		// New item selected
		iSelectedItem = aItemId;
		
#ifdef __SERIES60_40__
		DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
		
		RenderWrappedText(iSelectedItem);		
		RepositionItems(true);
		RenderScreen();
	}
	else {
		// Trigger item event
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			if(aItem->GetIsOwn()) {
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_UPDATE_MENUBAR);
				iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_OPTIONS_MENUBAR);
			}
			else if(aItem->GetItemType() == EItemContact) {
				if(iBuddycloudLogic->GetCall()->GetTelephonyState() == ETelephonyIdle) {
					iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_ITEM_MENUBAR);
					iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
					iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_OPTIONS_MENUBAR);
				}
			}
			else if(aItem->GetItemType() != EItemNotice || iBuddycloudLogic->GetState() == ELogicOnline) {
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_ITEM_MENUBAR);
				iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_OPTIONS_MENUBAR);
			}
		}		
	}
}

#ifdef __SERIES60_40__
void CBuddycloudFollowingContainer::DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar) {
	if(aResourceId == R_STATUS_TOOLBAR) {
		aToolbar->SetItemDimmed(EMenuCallCommand, true, true);
		aToolbar->SetItemDimmed(EMenuMessagesCommand, true, true);
		
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			if(aItem->GetItemType() >= EItemContact) {
				CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
				
				// Call dimming
				if(aContactItem->GetAddressbookId() >= 0) {
					CContactItem* aContact = iBuddycloudLogic->GetContactDetailsLC(aContactItem->GetAddressbookId());
					
					if(aContact != NULL) {
						if(aContact->CardFields().FindNext(KUidContactFieldPhoneNumber) != KErrNotFound) {
							if( !iBuddycloudLogic->GetCall()->IsTelephonyBusy() ) {
								aToolbar->SetItemDimmed(EMenuCallCommand, false, true);
							}				
						}
						
						CleanupStack::PopAndDestroy(); // aContact
					}
				}
				
				// Message dimming
				if(aItem->GetItemType() >= EItemRoster) {
					aToolbar->SetItemDimmed(EMenuMessagesCommand, false, true);
				}
			}
		}		
	}
}
#endif

void CBuddycloudFollowingContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_CALL_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuLoudspeakerCommand, true);
		aMenuPane->SetItemDimmed(EMenuHandsetCommand, true);
		aMenuPane->SetItemDimmed(EMenuHoldCallCommand, true);
		aMenuPane->SetItemDimmed(EMenuResumeCallCommand, true);
		aMenuPane->SetItemDimmed(EMenuEndCallCommand, false);
		
		// Loudspeaker
		if(iBuddycloudLogic->GetCall()->LoudspeakerAvailable()) {
			if(iBuddycloudLogic->GetCall()->LoudspeakerActive()) {
				aMenuPane->SetItemDimmed(EMenuHandsetCommand, false);
			}
			else {
				aMenuPane->SetItemDimmed(EMenuLoudspeakerCommand, false);
			}
		}
		
		// Hold/Resume
		if(iBuddycloudLogic->GetCall()->GetTelephonyState() == ETelephonyActive) {
			if(iBuddycloudLogic->GetCall()->OnHold()) {
				aMenuPane->SetItemDimmed(EMenuResumeCallCommand, false);
			}
			else {
				aMenuPane->SetItemDimmed(EMenuHoldCallCommand, false);
			}
		}
	}
	else if(aResourceId == R_STATUS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuConnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuDisconnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsSettingsCommand, false);

		if(iBuddycloudLogic->GetState() == ELogicOffline) {
			aMenuPane->SetItemDimmed(EMenuConnectCommand, false);
			aMenuPane->SetItemDimmed(EMenuInviteCommand, true);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuDisconnectCommand, false);
			aMenuPane->SetItemDimmed(EMenuInviteCommand, false);
		}

		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			if(aItem->GetItemType() >= EItemRoster && iBuddycloudLogic->GetState() == ELogicOnline) {
				aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, false);
			}
			
			if(aItem->GetItemType() != EItemNotice || iBuddycloudLogic->GetState() == ELogicOnline) {
				aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aItem->GetTitle().Left(32));
				aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);
			}
		}
	}
	else if(aResourceId == R_STATUS_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuCallCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuGetPlaceInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuGetChannelInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuInviteToGroupCommand, true);
		aMenuPane->SetItemDimmed(EMenuSendPlaceBySmsCommand, true);
		aMenuPane->SetItemDimmed(EMenuInviteToBuddycloudCommand, true);
		aMenuPane->SetItemDimmed(EMenuUnfollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptAndFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeclineCommand, true);

		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			// Item dimming
			if(aItem->GetItemType() == EItemNotice) {
				CFollowingNoticeItem* aNoticeItem = static_cast <CFollowingNoticeItem*> (aItem);
				
				aMenuPane->SetItemDimmed(EMenuAcceptCommand, false);
				aMenuPane->SetItemDimmed(EMenuDeclineCommand, false);
				
				if(aNoticeItem->GetJidType() == EJidRoster) {
					CFollowingItem* aSubscribedItem = iItemStore->GetItemById(iBuddycloudLogic->IsSubscribedTo(aNoticeItem->GetJid(), EItemRoster));
					
					if(aSubscribedItem == NULL) {
						aMenuPane->SetItemDimmed(EMenuAcceptAndFollowCommand, false);
					}
				}
			}
			else if(aItem->GetItemType() == EItemContact) {
				CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
				
				if(aContactItem->GetAddressbookId() >= 0) {
					CContactItem* aContact = iBuddycloudLogic->GetContactDetailsLC(aContactItem->GetAddressbookId());
					
					if(aContact) {
						if(aContact->CardFields().FindNext(KUidContactFieldPhoneNumber) != KErrNotFound) {
							if( !iBuddycloudLogic->GetCall()->IsTelephonyBusy() ) {
								aMenuPane->SetItemDimmed(EMenuCallCommand, false);
							}				
						}
						
						if(iBuddycloudLogic->GetMyPlaceId() > 0) {
							aMenuPane->SetItemDimmed(EMenuSendPlaceBySmsCommand, false);
						}
						
						aMenuPane->SetItemDimmed(EMenuInviteToBuddycloudCommand, false);
						
						CleanupStack::PopAndDestroy(); // aContact
					}
				}
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);				
				
				if(!aRosterItem->GetIsOwn()) {
					aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
					aMenuPane->SetItemDimmed(EMenuInviteToGroupCommand, false);
				}
				
				if(aRosterItem->GetJid(EJidChannel).Length() > 0) {
					aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
					aMenuPane->SetItemDimmed(EMenuGetChannelInfoCommand, false);
				}

				if(aRosterItem->GetAddressbookId() >= 0) {
					CContactItem* aContact = iBuddycloudLogic->GetContactDetailsLC(aRosterItem->GetAddressbookId());
					
					if(aContact) {
						if(aContact->CardFields().FindNext(KUidContactFieldPhoneNumber) != KErrNotFound) {
							if( !iBuddycloudLogic->GetCall()->IsTelephonyBusy() ) {
								aMenuPane->SetItemDimmed(EMenuCallCommand, false);
								
								if(iBuddycloudLogic->GetMyPlaceId() > 0) {
									aMenuPane->SetItemDimmed(EMenuSendPlaceBySmsCommand, false);
								}
							}
						}
						
						CleanupStack::PopAndDestroy(); // aContact
					}
				}
				
				if(!aRosterItem->GetIsOwn()) {
					if(aRosterItem->GetSubscriptionType() <= ESubscriptionFrom) {
						aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
					}
					
					if(aRosterItem->GetSubscriptionType() >= ESubscriptionFrom) {
						aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
					}
				}
				
				if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
					aMenuPane->SetItemDimmed(EMenuGetPlaceInfoCommand, false);
				}
			}
			else if(aItem->GetItemType() == EItemChannel) {
				aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
				aMenuPane->SetItemDimmed(EMenuGetChannelInfoCommand, false);
				aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
			}
		}
	}
	else if(aResourceId == R_STATUS_OPTIONS_UPDATE_MENU) {
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);				
			
			if(aRosterItem->GetJid(EJidChannel).Length() > 0) {
				aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
			}
			
			if(aRosterItem->GetUnread() > 0) {
				aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
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
		
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
			
			if(aChannelItem->GetJid().Length() > 0) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
			}
			
			if(aChannelItem->GetItemType() == EItemRoster) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, false);
			}

			aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
		}
	}
}

void CBuddycloudFollowingContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuEndCallCommand) {
		iBuddycloudLogic->GetCall()->Hangup();
	}
	else if(aCommand == EMenuHoldCallCommand) {
		iBuddycloudLogic->GetCall()->Hold();
	}
	else if(aCommand == EMenuResumeCallCommand) {
		iBuddycloudLogic->GetCall()->Resume();
	}
	else if(aCommand == EMenuLoudspeakerCommand) {
		iBuddycloudLogic->GetCall()->SetLoudspeakerActive(true);
	}
	else if(aCommand == EMenuHandsetCommand) {
		iBuddycloudLogic->GetCall()->SetLoudspeakerActive(false);
	}
	else if(aCommand == EMenuDisconnectCommand) {
		iBuddycloudLogic->Disconnect();
	}
	else if(aCommand == EMenuConnectCommand) {
		iBuddycloudLogic->ConnectL();
	}
	else if(aCommand == EMenuSetStatusCommand) {
		TBuf<256> aMood;
		iEikonEnv->ReadResource(aMood, R_LOCALIZED_STRING_DIALOG_MOODIS);
		
		CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
		
		if(aOwnItem && aOwnItem->GetDescription().Length() > 0) {
			aMood.Copy(aOwnItem->GetDescription());
		}

		CAknTextQueryDialog* dlg = CAknTextQueryDialog::NewL(aMood, CAknQueryDialog::ENoTone);
		dlg->SetPredictiveTextInputPermitted(true);

		if(dlg->ExecuteLD(R_MOOD_DIALOG) != 0) {
			iBuddycloudLogic->SetMoodL(aMood);
		}
	}
	else if(aCommand == EMenuFollowCommand || aCommand == EMenuInviteToGroupCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem != NULL) {
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aCommand == EMenuFollowCommand) {
					iBuddycloudLogic->FollowContactL(aRosterItem->GetJid());
				}
				else {
					iBuddycloudLogic->InviteToChannelL(aRosterItem->GetJid());
				}
			}
		}
	}
	else if(aCommand == EMenuSendPlaceBySmsCommand) {
		iBuddycloudLogic->SendPlaceL(iSelectedItem);
	}
	else if(aCommand == EMenuInviteToBuddycloudCommand) {
		iBuddycloudLogic->SendInviteL(iSelectedItem);
	}
	else if(aCommand == EMenuInviteCommand) {
		TBuf<64> aContact;

		CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(aContact, CAknQueryDialog::ENoTone);
		aDialog->SetPredictiveTextInputPermitted(true);

		if(aDialog->ExecuteLD(R_ADDCONTACT_DIALOG) != 0) {
			iBuddycloudLogic->FollowContactL(aContact);
		}
	}
	else if(aCommand == EMenuUnfollowCommand) {
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();

		if(aDialog->ExecuteLD(R_DELETE_DIALOG) != 0) {
			iBuddycloudLogic->DeleteItemL(iSelectedItem);
		}
	}
	else if(aCommand == EMenuMessagesCommand || aCommand == EMenuPrivateMessagesCommand || aCommand == EMenuChannelMessagesCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			TMessagingViewObject aObject;			
			aObject.iFollowerId = iSelectedItem;
			aObject.iTitle.Append(aItem->GetTitle());
			
			if(aCommand == EMenuMessagesCommand) {
				// Toolbar messages, get discussion with unread messages
				aCommand = EMenuChannelMessagesCommand;
				
				if(aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
					if(aRosterItem->GetJid(EJidChannel).Length() == 0 || 
							(aRosterItem->GetUnread(EJidRoster) > 0 && aRosterItem->GetUnread(EJidChannel) == 0)) {
						
						// Open private messages if channel is read/not existing
						aCommand = EMenuPrivateMessagesCommand;
					}
				}
			}
			
			if(aCommand == EMenuPrivateMessagesCommand) {
				// Private messages
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				aObject.iJid.Append(aRosterItem->GetJid());
			}
			else {
				// Topic or personal channel
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
				
				aObject.iJid.Append(aChannelItem->GetJid());
			}
			
			TMessagingViewObjectPckg aObjectPckg(aObject);		
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(iSelectedItem), aObjectPckg);			
		}
	}
	else if(aCommand == EMenuGetPlaceInfoCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			CBuddycloudBasicPlace* aUserPlace = aRosterItem->GetPlace(EPlaceCurrent);
			
			// Prepare dialog header
			HBufC* aHeader = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_ITEM_PLACEINFO);
			TPtr pHeader(aHeader->Des());
							
			// Prepare dialog message
			HBufC* aMessage = HBufC::NewLC(aUserPlace->GetPlaceName().Length() + 
					aUserPlace->GetPlaceStreet().Length() + aUserPlace->GetPlaceArea().Length() + 
					aUserPlace->GetPlaceCity().Length() + aUserPlace->GetPlacePostcode().Length() + 
					aUserPlace->GetPlaceRegion().Length() + aUserPlace->GetPlaceCountry().Length() + 11);
			
			TPtr pMessage(aMessage->Des());
			pMessage.Append(aUserPlace->GetPlaceName());
			
			if(pMessage.Length() && aUserPlace->GetPlaceStreet().Length() > 0) {
				pMessage.Append(_L(",\n"));
			}
			
			pMessage.Append(aUserPlace->GetPlaceStreet());
			
			if(pMessage.Length() && aUserPlace->GetPlaceArea().Length() > 0) {
				pMessage.Append(_L(",\n"));
			}
			
			pMessage.Append(aUserPlace->GetPlaceArea());
			
			if(pMessage.Length() && (aUserPlace->GetPlaceCity().Length() > 0 || aUserPlace->GetPlacePostcode().Length() > 0)) {
				pMessage.Append(_L(",\n"));
			}
			
			pMessage.Append(aUserPlace->GetPlaceCity());
			
			if(aUserPlace->GetPlaceCity().Length() > 0) {
				pMessage.Append(_L(" "));
			}
			
			pMessage.Append(aUserPlace->GetPlacePostcode());
							
			if(pMessage.Length() && aUserPlace->GetPlaceRegion().Length() > 0) {
				pMessage.Append(_L(",\n"));
			}
			
			pMessage.Append(aUserPlace->GetPlaceRegion());

			if(pMessage.Length() && aUserPlace->GetPlaceCountry().Length() > 0) {
				pMessage.Append(_L(",\n"));
			}
			
			pMessage.Append(aUserPlace->GetPlaceCountry());
			
			// Prepare dialog
			CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
			aDialog->PrepareLC(R_ABOUT_DIALOG);				
			aDialog->SetHeaderText(pHeader);
			aDialog->SetMessageText(pMessage);
			
			// Show dialog
			aDialog->RunLD();			
			
			CleanupStack::PopAndDestroy(2); // aMessage, aHeader
		}
	}
	else if(aCommand == EMenuGetChannelInfoCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			_LIT(KChannelName, "#\n");
			_LIT(KChannelRank, ": %d (%d)");
			HBufC* aRankResource = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CHANNELRANK);
			TPtrC pRankResource(aRankResource->Des());
			
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
			TPtrC pChannelName(aChannelItem->GetJid());
			TInt aLocate = pChannelName.Locate('@');
			
			if(aLocate != KErrNotFound) {
				pChannelName.Set(pChannelName.Left(aLocate));
			}
			
			HBufC* aMessage = HBufC::NewLC(KChannelName().Length() + pChannelName.Length() + pRankResource.Length() + KChannelRank().Length() + 32);
			TPtr pMessage(aMessage->Des());
			pMessage.Append(pRankResource);
			pMessage.AppendFormat(KChannelRank(), aChannelItem->GetRank(), aChannelItem->GetRankShift());
			
			if(aItem->GetItemType() == EItemChannel) {
				pMessage.Insert(0, KChannelName);
				pMessage.Insert(1, pChannelName);
			}
			
			if(aChannelItem->GetRankShift() > 0 && (aLocate = pMessage.Locate('(')) != KErrNotFound) {
				pMessage.Insert(aLocate + 1, _L("+"));
			}
			
			// Prepare dialog
			CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
			aDialog->PrepareLC(R_ABOUT_DIALOG);				
			aDialog->SetHeaderText(aChannelItem->GetTitle());
			aDialog->SetMessageText(pMessage);
			
			// Show dialog
			aDialog->RunLD();			
			
			CleanupStack::PopAndDestroy(2); // aMessage, aRankResource
		}
	}
	else if(aCommand == EMenuAcceptCommand) {
		iBuddycloudLogic->RespondToNoticeL(iSelectedItem, ENoticeAccept);
	}
	else if(aCommand == EMenuAcceptAndFollowCommand) {
		iBuddycloudLogic->RespondToNoticeL(iSelectedItem, ENoticeAcceptAndFollow);
	}
	else if(aCommand == EMenuDeclineCommand) {
		iBuddycloudLogic->RespondToNoticeL(iSelectedItem, ENoticeDecline);
	}
	else if(aCommand == EMenuCallCommand) {
		iBuddycloudLogic->CallL(iSelectedItem);
	}
	else if(aCommand == EMenuNewSearchCommand || aCommand == EAknSoftkeyBack) {
		// Reset filter
		iBuddycloudLogic->SetContactFilterL(_L(""));
		iSearchEdwin->SetTextL(&iBuddycloudLogic->GetContactFilter());
		iSearchEdwin->HandleTextChangedL();
		
		if(aCommand == EMenuNewSearchCommand && !iSearchVisible) {
			// Show search filter
			DisplayEdwin(true);
		}
		else {
			// Hide search filter
			DisplayEdwin(false);
		}
		
		// Back to top
		RenderScreen();
	}
	else if(aCommand == EMenuSeeFollowingCommand || aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeProducingCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			TExplorerQuery aQuery;
			
			if(aItem->GetItemType() == EItemChannel || (aItem->GetItemType() == EItemRoster && aCommand == EMenuSeeFollowersCommand)) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
				TPtrC8 aSubjectJid = iTextUtilities->UnicodeToUtf8L(aChannelItem->GetJid());
				
				aQuery.iStanza.Append(_L8("<iq to='maitred.buddycloud.com' type='get' id='exp_users1'><query xmlns='http://buddycloud.com/protocol/channels#followers'><channel><jid></jid></channel></query></iq>"));
				aQuery.iStanza.Insert(138, aSubjectJid);
				
				iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_FOLLOWEDBY);
				aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, aItem->GetTitle());
			}
			else {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				TPtrC8 aSubjectJid = iTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid());
				
				if(aCommand == EMenuSeeProducingCommand) {
					aQuery = CExplorerStanzaBuilder::BuildChannelsXmppStanza(aSubjectJid, EChannelProducer);
					iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_ISPRODUCING);
				}
				else {
					aQuery = CExplorerStanzaBuilder::BuildChannelsXmppStanza(aSubjectJid, EChannelAll);
					iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_ISFOLLOWING);
				}
				
				aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$USER")), 5, aItem->GetTitle());
			}
						
			TExplorerQueryPckg aQueryPckg(aQuery);		
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iSelectedItem), aQueryPckg);		
		}
	}
	else if(aCommand == EMenuSeeNearbyCommand) {
		CFollowingItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem) {
			TExplorerQuery aQuery;
			
			// Build stanza
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(EExplorerItemPerson, iTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid()));
			}
			else {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
				
				aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(EExplorerItemChannel, iTextUtilities->UnicodeToUtf8L(aChannelItem->GetJid()));
			}
			
			// Add title
			iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO);
			aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, aItem->GetTitle());
			
			TExplorerQueryPckg aQueryPckg(aQuery);		
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iSelectedItem), aQueryPckg);		
		}
	}
}

TKeyResponse CBuddycloudFollowingContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			TInt aIndex = iItemStore->GetIndexById(iSelectedItem);

			if(aIndex > 0) {
				for(TInt i = aIndex-1; i >= 0; i--) {
					if(IsFilteredItem(i)) {
						HandleItemSelection(iItemStore->GetItemByIndex(i)->GetItemId());
						break;
					}
				}
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			TInt aIndex = iItemStore->GetIndexById(iSelectedItem);

			if(aIndex < iItemStore->Count() - 1) {
				for(TInt i = aIndex+1; i < iItemStore->Count(); i++) {
					if(IsFilteredItem(i)) {
						HandleItemSelection(iItemStore->GetItemByIndex(i)->GetItemId());
						break;
					}
				}
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == 63557) { // Center Dpad
			HandleItemSelection(iSelectedItem);
			aResult = EKeyWasConsumed;
		}
	}
	else if(aType == EEventKeyUp) {
		if(aKeyEvent.iScanCode == EStdKeyYes) {
			if( !iBuddycloudLogic->GetCall()->IsTelephonyBusy() ) {
				iBuddycloudLogic->CallL(iSelectedItem);
				
				aResult = EKeyWasConsumed;
			}
		}
	}
	else if(aType == EEventKeyDown) {
		if(iSearchEdwin->TextLength() == 0 && aKeyEvent.iScanCode == EStdKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPlacesViewId), TUid::Uid(0), _L8(""));
			aResult = EKeyWasConsumed;
		}
	}

	// If key not consumed pass to iSearchEdwin
	if(aResult == EKeyWasNotConsumed) {
		iSearchEdwin->OfferKeyEventL(aKeyEvent, aType);

		if(iSearchLength == 0 && iSearchEdwin->TextLength() > 0) {
			// Show
			DisplayEdwin(true);
		}
		else if(iSearchLength > 0 && iSearchEdwin->TextLength() == 0) {
			// Dont Show
			DisplayEdwin(false);
		}
		
		// Filter Contacts
		TBuf<64> aFilterText;
		iSearchEdwin->GetText(aFilterText);
		iBuddycloudLogic->SetContactFilterL(aFilterText);
		
		iSearchLength = iSearchEdwin->TextLength();
		aResult = EKeyWasConsumed;
	}

	return aResult;
}

void CBuddycloudFollowingContainer::TabChangedL(TInt aIndex) {
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), _L8(""));
}

// End of File
