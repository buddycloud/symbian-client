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
#include "BuddycloudExplorer.h"
#include "BuddycloudFollowingContainer.h"
#include "ViewReference.h"

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

	// Edwin
	iEdwin = new (ELeave) CEikEdwin();
	iEdwin->MakeVisible(true);
	iEdwin->SetFocus(true);
	iEdwin->SetContainerWindowL(*this);

	TResourceReader aReader;
	iEikonEnv->CreateResourceReaderLC(aReader, R_SEARCH_EDWIN);
	iEdwin->ConstructFromResourceL(aReader);
	CleanupStack::PopAndDestroy(); // aReader

	iEdwin->SetAknEditorFlags(EAknEditorFlagNoT9 | EAknEditorFlagNoEditIndicators);
	iEdwin->SetAknEditorCase(EAknEditorLowerCase);
	ConfigureEdwinL();

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

	// Edwin
	if(iEdwin)
		delete iEdwin;
	
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
	TInt aIconWidth = (iUnselectedItemIconTextOffset / 4) * 3;

	// Calculate dimensions
	iArrow1Size.SetSize(aIconWidth, (iSecondaryFont->HeightInPixels() + iSecondaryBoldFont->HeightInPixels()));
	AknIconUtils::SetSize(iArrow1Bitmap, iArrow1Size, EAspectRatioNotPreserved);
	iArrow1Size = iArrow1Bitmap->SizeInPixels();
	
	iArrow2Size.SetSize(aIconWidth, ((iSecondaryFont->HeightInPixels() * 2) + iSecondaryBoldFont->HeightInPixels()));
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
	iEdwin->SetCharFormatLayer(aCharFormatLayer);  // Edwin takes the ownership
	CleanupStack::Pop(aCharFormatLayer);
}

void CBuddycloudFollowingContainer::DisplayEdwin(TBool aShowEdwin) {
	iRect = Rect();

	if(iEdwin) {
		TRect aFieldRect = iEdwin->Rect();
		
		if(aShowEdwin) {
			// Show edwin
			aFieldRect = TRect(iSecondaryFont->FontMaxHeight(), (iRect.Height() - iSecondaryFont->FontMaxHeight() - 2), (iRect.Width() - iSecondaryFont->FontMaxHeight()), (iRect.Height() - 2));
#ifdef __SERIES60_40__
			aFieldRect.iTl.iY = aFieldRect.iTl.iY - (aFieldRect.Height() / 2);
#endif
			iEdwin->SetRect(aFieldRect);
			
			iRect.SetHeight(iRect.Height() - iEdwin->Rect().Height() - 6);
			
			RepositionItems(iSnapToItem);
			iEdwinVisible = true;
		}
		else {
			// Dont show edwin
			aFieldRect = TRect(iSecondaryFont->FontMaxHeight(), (iRect.Height() + 2), (iRect.Width() - iSecondaryFont->FontMaxHeight()), (iRect.Height() + aFieldRect.Height() + 2));
			iEdwin->SetFocus(false);
			iEdwin->SetRect(aFieldRect);
			iEdwin->SetFocus(true);
			
			if(iEdwinVisible) {
				iSelectedItem = KErrNotFound;
				RepositionItems(true);
			}
			
			iEdwinVisible = false;
		}
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
		}
		
		RenderWrappedText(iSelectedItem);			
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
	
	// Edwin
	if(iEdwin) {
		iEdwin->SetTextL(&iItemStore->GetFilterText());
		iEdwin->HandleTextChangedL();

		if(iEdwin->TextLength() > 0 || iEdwinVisible) {
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
	return 1; // iEdwin
}

CCoeControl* CBuddycloudFollowingContainer::ComponentControl(TInt aIndex) const {
	if(aIndex == 0) {
		return iEdwin;
	}
	
	return NULL;
}

void CBuddycloudFollowingContainer::RenderWrappedText(TInt aIndex) {
	TInt aWidth = (iRect.Width() - iLeftBarSpacer - iRightBarSpacer - 10);
	
	// Clear
	ClearWrappedText();

	if(iItemStore->Count() > 0) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(aIndex));
		
		if(aItem) {		
			// Wrap
			iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryItalicFont, aItem->GetDescription(), aWidth);
		}
	}
	else {
		// No following items
		HBufC* aNoItems = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_FOLLOWINGLISTEMPTY);		
		iTextUtilities->WrapToArrayL(iWrappedTextArray, iSecondaryBoldFont, *aNoItems, aWidth);		
		CleanupStack::PopAndDestroy();	// aNoItems
	}
}

TInt CBuddycloudFollowingContainer::CalculateItemSize(TInt aIndex) {
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemByIndex(aIndex));
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;

	if(aItem) {
		if(aItem->GetItemId() == iSelectedItem) {
			aMinimumSize = iItemIconSize + 4;
			
			// Title
			aItemSize += iPrimaryBoldFont->HeightInPixels();
			aItemSize += iPrimarySmallFont->DescentInPixels();
			
			// Sub-title
			if(aItem->GetSubTitle().Length() > 0) {
				aItemSize += iPrimarySmallFont->HeightInPixels();
				aItemSize += iPrimarySmallFont->DescentInPixels();
			}
			
			// Description
			if(aItem->GetDescription().Length() > 0) {
				if(aItemSize < (iItemIconSize + 2)) {
					aItemSize = (iItemIconSize + 2);
				}
				
				aItemSize += (iWrappedTextArray.Count() * iSecondaryItalicFont->HeightInPixels());
			}
			
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				// Place history
				if(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText).Length() > 0 || 
						aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0 || 
						aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {	
					
					if(aItemSize < (iItemIconSize + 2)) {
						aItemSize = (iItemIconSize + 2);
					}
					
					aItemSize += iSecondaryItalicFont->FontMaxDescent();
					aItemSize += iSecondaryFont->HeightInPixels();
					aItemSize += iSecondaryBoldFont->HeightInPixels();
					
					if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
						aItemSize += iSecondaryFont->HeightInPixels();
					}
				}
			}
		}
		else {
			aMinimumSize = (iItemIconSize / 2) + 2;
#ifdef __SERIES60_40__
			aMinimumSize += (iSecondaryBoldFont->DescentInPixels() * 2);
			aItemSize += (iSecondaryBoldFont->DescentInPixels() * 2);
#endif
			
			aItemSize += iSecondaryBoldFont->HeightInPixels();

			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0) {
					aItemSize += iSecondaryBoldFont->FontMaxDescent();
					aItemSize += iSecondaryFont->HeightInPixels();
				}
			}
		}
		
		aItemSize += iSecondaryFont->FontMaxDescent() + 2;
	}
	
	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudFollowingContainer::RenderListItems() {
	TBuf<256> aBuf;
	TInt aDrawPos = -iScrollbarHandlePosition;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iItemStore->Count() > 0 && iTotalListSize > 0) {
		TInt aItemSize = 0;
		
#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iItemStore->Count(); i++) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemByIndex(i));

			if(aItem) {
				if(aItem->Filtered()) {
					// Calculate item size
					aItemSize = CalculateItemSize(i);
					
					// Test to start the page drawing
					if(aDrawPos + aItemSize > 0) {
						TInt aItemDrawPos = aDrawPos;
						TInt aMessageIconOffset = 0;
						
#ifdef __SERIES60_40__
						iListItems.Append(TListItem(aItem->GetItemId(), TRect(0, aItemDrawPos, (Rect().Width() - iRightBarSpacer), (aItemDrawPos + aItemSize))));
#endif

						TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aItem->GetTitle()));
						
						if(iSelectedItem == aItem->GetItemId()) {
							// Currently selected item
							RenderItemFrame(TRect(iLeftBarSpacer + 1, aItemDrawPos, (iRect.Width() - iRightBarSpacer), (aItemDrawPos + aItemSize)));
							
							iBufferGc->SetPenColor(iColourTextSelected);
							
							if(aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel) {
								// Drawing of unread messages
								TInt aUnreadPos = aItemDrawPos + 2;
								CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
								
								iBufferGc->UseFont(iSecondaryFont);
							
								if(aChannelItem->GetUnread() > 0) {
									// Draw channel messages
									aBuf.Format(_L("%d"), aChannelItem->GetUnread());
									aMessageIconOffset = (iItemIconSize / 2) + iSecondaryFont->TextWidthInPixels(aBuf);
									
									// Message Icon
									iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
									iBufferGc->BitBltMasked(TPoint((iRect.Width() - iRightBarSpacer - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KIconMessage2, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage2, true, iIconMidmapSize), true);								
									
									if(aChannelItem->GetReplies() > 0) {
										// Draw direct reply icon
										iBufferGc->BitBltMasked(TPoint((iRect.Width() - iRightBarSpacer - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KOverlayReply, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KOverlayReply, true, iIconMidmapSize), true);
									}

									iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
									
									// Unread number
									iBufferGc->DrawText(aBuf, TPoint((iRect.Width() - iRightBarSpacer - 3 - aMessageIconOffset), aUnreadPos + (iSecondaryFont->HeightInPixels() / 2) + (iItemIconSize / 4)));
									aUnreadPos += (iItemIconSize / 2);									
								}
								
								if(aItem->GetItemType() == EItemRoster) {
									CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
									
									if(aRosterItem->GetUnread() > 0) {
										// Draw private messages
										aBuf.Format(_L("%d"), aRosterItem->GetUnread());
										
										if(aRosterItem->GetUnread() > aChannelItem->GetUnread()) {
											aMessageIconOffset = (iItemIconSize / 2) + iSecondaryFont->TextWidthInPixels(aBuf);
										}
										
										// Message Icon
										iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
										iBufferGc->BitBltMasked(TPoint((iRect.Width() - iRightBarSpacer - 3 - (iItemIconSize / 2)), aUnreadPos), iAvatarRepository->GetImage(KIconMessage1, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconMessage1, true, iIconMidmapSize), true);
										iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
										
										// Unread number
										iBufferGc->DrawText(aBuf, TPoint((iRect.Width() - iRightBarSpacer - 3 - (iItemIconSize / 2) - iSecondaryFont->TextWidthInPixels(aBuf)), aUnreadPos + (iSecondaryFont->HeightInPixels() / 2) + (iItemIconSize / 4)));
									}
								}
								
								iBufferGc->DiscardFont();	
							}
							
							// Draw icon
							iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, aItemDrawPos + 2), iAvatarRepository->GetImage(aItem->GetIconId(), false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(aItem->GetIconId(), true, iIconMidmapSize), true);
							iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
							
							iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 3, aItemDrawPos, (iRect.Width() - iRightBarSpacer - aMessageIconOffset - 6), iRect.Height()));

							// Title
							if(iPrimaryBoldFont->TextCount(aDirectionalText, (iRect.Width() - iLeftBarSpacer - iRightBarSpacer - aMessageIconOffset - 6 - iSelectedItemIconTextOffset)) < aDirectionalText.Length()) {
								iBufferGc->UseFont(iSecondaryBoldFont);
							}
							else {
								iBufferGc->UseFont(iPrimaryBoldFont);
							}

							aItemDrawPos += iPrimaryBoldFont->HeightInPixels();
							iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
							aItemDrawPos += iPrimarySmallFont->DescentInPixels();
							iBufferGc->DiscardFont();
							
							if(aItem->GetSubTitle().Length() > 0) {
								iBufferGc->UseFont(iSecondaryFont);
								aItemDrawPos += iPrimarySmallFont->HeightInPixels();
								iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aItem->GetSubTitle()), TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
								aItemDrawPos += iPrimarySmallFont->DescentInPixels();
								iBufferGc->DiscardFont();
							}
						
							// Move to under icon
							if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
								aItemDrawPos = (aDrawPos + iItemIconSize + 2);
							}
							
							iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 3, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 3), iRect.Height()));

							// Description						
							if(aItem->GetDescription().Length() > 0) {
								iBufferGc->UseFont(iSecondaryItalicFont);
								
								for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
									aItemDrawPos += iSecondaryItalicFont->HeightInPixels();
									iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + 5, aItemDrawPos));
								}
								
								iBufferGc->DiscardFont();
							}
							
							if(aItem->GetItemType() == EItemRoster) {
								CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
								
								// Place history
								if(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText).Length() > 0 || 
										aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0 ||
										aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
									
									aItemDrawPos += iSecondaryItalicFont->FontMaxDescent();
									
									if(aItem->GetDescription().Length() > 0) {
										iBufferGc->SetPenColor(iColourTextSelectedTrans);
										iBufferGc->SetPenStyle(CGraphicsContext::EDashedPen);
										
										iBufferGc->DrawLine(TPoint(iLeftBarSpacer + 5, aItemDrawPos), TPoint((iRect.Width() - iRightBarSpacer - 5), aItemDrawPos));		
										
										iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
										iBufferGc->SetPenColor(iColourTextSelected);
									}
									
									iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
									
									if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
										iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + ((iUnselectedItemIconTextOffset / 2) - (iArrow2Size.iWidth / 2)), aItemDrawPos), iArrow2Bitmap, TRect(0, 0, iArrow2Size.iWidth, iArrow2Size.iHeight), iArrow2Mask, true);
									}
									else {
										iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + ((iUnselectedItemIconTextOffset / 2) - (iArrow2Size.iWidth / 2)), aItemDrawPos), iArrow1Bitmap, TRect(0, 0, iArrow1Size.iWidth, iArrow1Size.iHeight), iArrow1Mask, true);
									}
									
									iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
									
									// Previous place
									iBufferGc->UseFont(iSecondaryFont);
									iBufferGc->SetStrikethroughStyle(EStrikethroughOn);
									aItemDrawPos += iSecondaryFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText)));
									iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
									iBufferGc->SetStrikethroughStyle(EStrikethroughOff);
									iBufferGc->DiscardFont();

									// Current place
									iBufferGc->UseFont(iSecondaryBoldFont);
									aItemDrawPos += iSecondaryBoldFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText)));
									iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
									iBufferGc->DiscardFont();

									// Future place
									if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
										iBufferGc->UseFont(iSecondaryFont);
										aItemDrawPos += iSecondaryFont->HeightInPixels();
										aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText)));
										iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
										iBufferGc->DiscardFont();
									}
								}
							}							
						}
						else {
							// Non selected items
							TRect aRect = TRect(0, aItemDrawPos, iRect.Width(), iRect.Height());
#ifdef __SERIES60_40__
							aItemDrawPos += iSecondaryBoldFont->DescentInPixels();
#endif					
							iBufferGc->SetPenColor(iColourText);
							
							iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
							
							if(aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel) {
								aMessageIconOffset = (iRightBarSpacer + 3);
							
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
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(aItem->GetIconId(), false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(aItem->GetIconId(), true, (iIconMidmapSize + 1)), true);							
							iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);							
							
							iBufferGc->SetClippingRect(aRect);

							// Title
							iBufferGc->UseFont(iSecondaryBoldFont);
							aItemDrawPos += iSecondaryBoldFont->HeightInPixels();
							iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
							iBufferGc->DiscardFont();
							
							if(aItem->GetItemType() == EItemRoster) {
								CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
								
								// Current place
								if(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0) {
									iBufferGc->UseFont(iSecondaryFont);
									aItemDrawPos += iSecondaryBoldFont->FontMaxDescent();
									aItemDrawPos += iSecondaryFont->HeightInPixels();
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText)));
									iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
									iBufferGc->DiscardFont();
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
		iBufferGc->UseFont(iSecondaryBoldFont);
		iBufferGc->SetPenColor(iColourText);

		if(iItemStore->Count() > 0) {
			// No results
			aDrawPos = ((iRect.Height() / 2) - (iSecondaryBoldFont->HeightInPixels() / 2));
			
			HBufC* aNoResults = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);		
			iBufferGc->DrawText(*aNoResults, TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2)) - (iSecondaryBoldFont->TextWidthInPixels(*aNoResults) / 2)), aDrawPos));
			CleanupStack::PopAndDestroy();// aNoResults	
		}
		else {
			// No items
			aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * iSecondaryBoldFont->HeightInPixels()) / 2));
						
			for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
				aDrawPos += iSecondaryItalicFont->HeightInPixels();
				iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2)) - (iSecondaryBoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2)), aDrawPos));
			}
		}
		
		iBufferGc->DiscardFont();
		
#ifdef __SERIES60_40__
		DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
	}
	
	// Draw edwin
	if(iEdwin->IsVisible()) {
		iBufferGc->CancelClippingRect();

		TRect aFieldRect = iEdwin->Rect();
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
		CBuddycloudListItem* aItem = iItemStore->GetItemById(iSelectedItem);

		if(aItem == NULL || !aItem->Filtered()) {
			iSelectedItem = KErrNotFound;
		}
		
		for(TInt i = 0; i < iItemStore->Count(); i++) {
			aItem = iItemStore->GetItemByIndex(i);

			// If item is in filtering list
			if(aItem && aItem->Filtered()) {
				if(iSelectedItem == KErrNotFound) {
					iSelectedItem = aItem->GetItemId();
					RenderWrappedText(iSelectedItem);		
				}
				
#ifdef __SERIES60_40__
				DynInitToolbarL(R_STATUS_TOOLBAR, iViewAccessor->ViewToolbar());
#endif		
				
				aItemSize = CalculateItemSize(i);
				
				if(aSnapToItem && aItem->GetItemId() == iSelectedItem) {
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
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));
		CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();

		if(aItem) {
			if(aOwnItem && aItem->GetItemId() == aOwnItem->GetItemId()) {
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_UPDATE_MENUBAR);
				iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
				iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_STATUS_OPTIONS_MENUBAR);
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
		aToolbar->SetItemDimmed(EMenuChannelMessagesCommand, true, true);
		aToolbar->SetItemDimmed(EMenuPrivateMessagesCommand, true, true);
		
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			if(aItem->GetId().Length() > 0) {
				aToolbar->SetItemDimmed(EMenuChannelMessagesCommand, false, true);
			}
			
			if(aItem->GetItemType() == EItemRoster) {
				aToolbar->SetItemDimmed(EMenuPrivateMessagesCommand, false, true);
			}
		}
	}
}
#endif

void CBuddycloudFollowingContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_STATUS_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuConnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuDisconnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsExploreCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsSettingsCommand, false);

		if(iBuddycloudLogic->GetState() == ELogicOffline) {
			aMenuPane->SetItemDimmed(EMenuConnectCommand, false);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuDisconnectCommand, false);
		}

		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

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
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuGetPlaceInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, true);
		aMenuPane->SetItemDimmed(EMenuFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuSendPlaceBySmsCommand, true);
		aMenuPane->SetItemDimmed(EMenuInviteToBuddycloudCommand, true);
		aMenuPane->SetItemDimmed(EMenuUnfollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptCommand, true);
		aMenuPane->SetItemDimmed(EMenuAcceptAndFollowCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeclineCommand, true);

		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem) {
			// Item dimming
			if(aItem->GetItemType() == EItemNotice) {
				aMenuPane->SetItemDimmed(EMenuAcceptCommand, false);
				aMenuPane->SetItemDimmed(EMenuDeclineCommand, false);
				
				if(aItem->GetIdType() == EIdRoster && !iBuddycloudLogic->IsSubscribedTo(aItem->GetId(), EItemRoster)) {
					aMenuPane->SetItemDimmed(EMenuAcceptAndFollowCommand, false);
				}
				
				if(iBuddycloudLogic->GetState() == ELogicOnline) {
					aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
				}
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);				
				
				if(!aRosterItem->OwnItem()) {
					aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
				}
				
				if(aRosterItem->GetId(EIdChannel).Length() > 0) {
					aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
					
					if(iBuddycloudLogic->GetState() == ELogicOnline) {
						aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
					}
				}

				// Follow & Unfollow
				if(!aRosterItem->OwnItem()) {
					if(aRosterItem->GetSubscription() == EPresenceSubscriptionFrom) {
						aMenuPane->SetItemDimmed(EMenuFollowCommand, false);
					}
					
					if(aRosterItem->GetSubscription() == EPresenceSubscriptionTo || 
							aRosterItem->GetSubscription() == EPresenceSubscriptionBoth) {
						
						aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
					}
				}
				
				if(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0) {
					aMenuPane->SetItemDimmed(EMenuGetPlaceInfoCommand, false);
				}
			}
			else if(aItem->GetItemType() == EItemChannel) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);								
				
				aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
				
				if(iBuddycloudLogic->GetState() == ELogicOnline) {
					aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
				}
				
				if(aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationOwner) {
					aMenuPane->SetItemDimmed(EMenuUnfollowCommand, false);
				}
			}
		}
	}
	else if(aResourceId == R_STATUS_OPTIONS_UPDATE_MENU) {
		aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, true);
		aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, true);
		
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);				
			
			if(aRosterItem->GetId(EIdChannel).Length() > 0) {
				aMenuPane->SetItemDimmed(EMenuChannelMessagesCommand, false);
			}
			
			if(aRosterItem->GetUnread() > 0) {
				aMenuPane->SetItemDimmed(EMenuPrivateMessagesCommand, false);
			}
			
			if(iBuddycloudLogic->GetState() == ELogicOnline) {
				aMenuPane->SetItemDimmed(EMenuChannelInfoCommand, false);
			}
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
		
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			if(aItem->GetId().Length() > 0) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowersCommand, false);
				
				if(aItem->GetItemType() == EItemChannel) {
					aMenuPane->SetItemDimmed(EMenuSeeModeratorsCommand, false);
				}
				else {
					aMenuPane->SetItemDimmed(EMenuSeperator, false);
				}
			}
			
			if(aItem->GetItemType() == EItemRoster) {
				aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeModeratingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, false);
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
			}
		}
	}
}

void CBuddycloudFollowingContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuDisconnectCommand) {
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
			aMood.Copy(aOwnItem->GetDescription().Left(aMood.MaxLength()));
		}

		CAknTextQueryDialog* dlg = CAknTextQueryDialog::NewL(aMood, CAknQueryDialog::ENoTone);
		dlg->SetPredictiveTextInputPermitted(true);

		if(dlg->ExecuteLD(R_MOOD_DIALOG) != 0) {
			iBuddycloudLogic->SetMoodL(aMood);
		}
	}
	else if(aCommand == EMenuFollowCommand) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(aCommand == EMenuFollowCommand) {
				iBuddycloudLogic->FollowContactL(aRosterItem->GetId());
			}
		}
	}
	else if(aCommand == EMenuSendPlaceBySmsCommand) {
		iBuddycloudLogic->SendPlaceL(iSelectedItem);
	}
	else if(aCommand == EMenuInviteToBuddycloudCommand) {
		iBuddycloudLogic->SendInviteL(iSelectedItem);
	}
	else if(aCommand == EMenuAddContactCommand) {
		TBuf<64> aFollow;

		CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(aFollow, CAknQueryDialog::ENoTone);
		aDialog->SetPredictiveTextInputPermitted(true);

		if(aDialog->ExecuteLD(R_ADDCONTACT_DIALOG) != 0) {
			if(aFollow.Locate('@') != KErrNotFound) {
				iBuddycloudLogic->FollowContactL(aFollow);
			}
			else if(aFollow.Locate('#') != KErrNotFound) {
				TViewReferenceBuf aViewReference;	
				aViewReference().iCallbackRequested = true;
				aViewReference().iCallbackViewId = KFollowingViewId;
				aViewReference().iOldViewData.iId = iSelectedItem;
				aViewReference().iNewViewData.iTitle.Copy(aFollow);
				aViewReference().iNewViewData.iData.Copy(_L8("/channel/"));
				aViewReference().iNewViewData.iData.Append(aFollow.Mid(aFollow.Locate('#') + 1));

				iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelInfoViewId), TUid::Uid(0), aViewReference);			
			}
		}
	}
	else if(aCommand == EMenuUnfollowCommand) {
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();

		if(aDialog->ExecuteLD(R_DELETE_DIALOG) != 0) {
			iBuddycloudLogic->UnfollowItemL(iSelectedItem);
			
			RenderScreen();
		}
	}
	else if(aCommand == EMenuPrivateMessagesCommand || aCommand == EMenuChannelMessagesCommand) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem) {
			TViewReferenceBuf aViewReference;	
			aViewReference().iNewViewData.iId = iSelectedItem;
			aViewReference().iNewViewData.iTitle.Copy(aItem->GetTitle());
			aViewReference().iNewViewData.iData.Copy(aItem->GetId());
			
			if(aCommand == EMenuPrivateMessagesCommand) {
				// Private messages
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);			
				aViewReference().iNewViewData.iData.Copy(aRosterItem->GetId());
			}

			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KMessagingViewId), TUid::Uid(iSelectedItem), aViewReference);					
		}
	}
	else if(aCommand == EMenuChannelInfoCommand) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetId().Length() > 0) {
			TViewReferenceBuf aViewReference;	
			aViewReference().iCallbackRequested = true;
			aViewReference().iCallbackViewId = KFollowingViewId;
			aViewReference().iOldViewData.iId = iSelectedItem;

			if(aItem->GetItemType() == EItemNotice) {
				aViewReference().iNewViewData.iTitle.Copy(aItem->GetId());
				aViewReference().iNewViewData.iData.Copy(aItem->GetId());
				aViewReference().iNewViewData.iData.Insert(0, _L8("/user/"));
				aViewReference().iNewViewData.iData.Append(_L8("/channel"));				
			}
			else if(aItem->GetItemType() >= EItemRoster) {
				aViewReference().iNewViewData.iTitle.Copy(aItem->GetTitle());
				aViewReference().iNewViewData.iData.Copy(aItem->GetId());
			}
			
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelInfoViewId), TUid::Uid(0), aViewReference);			
		}
	}
	else if(aCommand == EMenuGetPlaceInfoCommand) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			CGeolocData* aGeoloc = aRosterItem->GetGeolocItem(EGeolocItemCurrent);
						
			// Prepare dialog header
			HBufC* aHeader = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_ITEM_PLACEINFO);
			TPtr pHeader(aHeader->Des());
							
			// Prepare dialog message
			HBufC* aMessage = HBufC::NewLC(aGeoloc->GetString(EGeolocText).Length() + 
					aGeoloc->GetString(EGeolocStreet).Length() + aGeoloc->GetString(EGeolocArea).Length() + 
					aGeoloc->GetString(EGeolocLocality).Length() + aGeoloc->GetString(EGeolocPostalcode).Length() + 
					aGeoloc->GetString(EGeolocRegion).Length() + aGeoloc->GetString(EGeolocCountry).Length() + 11);
			TPtr pMessage(aMessage->Des());
			
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocText), KNullDesC);
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocStreet), _L(",\n"));
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocArea), _L(",\n"));
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocLocality), _L(",\n"));
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocPostalcode), _L(" "));
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocRegion), _L(",\n"));
			aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocCountry), _L(",\n"));
					
			// Prepare dialog
			CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
			aDialog->PrepareLC(R_ABOUT_DIALOG);				
			aDialog->SetHeaderText(pHeader);
			aDialog->SetMessageText(pMessage);
			
			// Show dialog
			aDialog->RunLD();			
			
			CleanupStack::PopAndDestroy(3); // aTextUtilities, aMessage, aHeader
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
	else if(aCommand == EMenuSeeFollowingCommand || aCommand == EMenuSeeModeratingCommand || aCommand == EMenuSeeProducingCommand || 
			aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeModeratorsCommand) {	
		
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() >= EItemRoster) {
			TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aItem->GetId()));	
			TInt aResourceId = KErrNotFound;
			
			TViewReferenceBuf aViewReference;
			aViewReference().iCallbackRequested = true;
			aViewReference().iCallbackViewId = KFollowingViewId;
			aViewReference().iOldViewData.iId = iSelectedItem;
			
			if(aCommand == EMenuSeeFollowersCommand || aCommand == EMenuSeeModeratorsCommand) {
				if(aCommand == EMenuSeeModeratorsCommand) {
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("owner"));
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("moderator"));
					aResourceId = R_LOCALIZED_STRING_TITLE_MODERATEDBY;
				}
				else {
					CExplorerStanzaBuilder::FormatBroadcasterXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId);
					aResourceId = R_LOCALIZED_STRING_TITLE_FOLLOWEDBY;
				}
				
				CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, aResourceId, _L("$OBJECT"), aItem->GetTitle());
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				aEncId.Set(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));	
				
				if(aCommand == EMenuSeeProducingCommand) {
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("owner"));
					aResourceId = R_LOCALIZED_STRING_TITLE_ISPRODUCING;
				}
				else if(aCommand == EMenuSeeModeratingCommand) {
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("moderator"));
					aResourceId = R_LOCALIZED_STRING_TITLE_ISMODERATING;
				}
				else {
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("publisher"));
					CExplorerStanzaBuilder::AppendMaitredXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aEncId, _L8("member"));
					aResourceId = R_LOCALIZED_STRING_TITLE_ISFOLLOWING;
				}
				
				CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, aResourceId, _L("$USER"), aItem->GetTitle());
			}
					
			if(aViewReference().iNewViewData.iData.Length() > 0) {
				iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), aViewReference);			
			}
		}
	}
	else if(aCommand == EMenuSeeNearbyCommand) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iItemStore->GetItemById(iSelectedItem));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			TViewReferenceBuf aViewReference;
			aViewReference().iCallbackRequested = true;
			aViewReference().iCallbackViewId = KFollowingViewId;
			aViewReference().iOldViewData.iId = iSelectedItem;
			
			CExplorerStanzaBuilder::FormatButlerXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));
			CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO, _L("$OBJECT"), aRosterItem->GetTitle());
			
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), aViewReference);		
		}
	}
	else if(aCommand == EMenuNewSearchCommand || aCommand == EAknSoftkeyBack) {
		if(aCommand == EAknSoftkeyBack && iItemStore->GetFilterText().Length() == 0) {
			// Hide application
			TApaTask aTask(iEikonEnv->WsSession());
			aTask.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
			aTask.SendToBackground();
		}
		else {	
			// Reset filter
			iItemStore->SetFilterTextL(KNullDesC);
			iEdwin->SetTextL(&iItemStore->GetFilterText());
			iEdwin->HandleTextChangedL();
			
			if(aCommand == EMenuNewSearchCommand && !iEdwinVisible) {
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
	}
}

TKeyResponse CBuddycloudFollowingContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			TInt aIndex = iItemStore->GetIndexById(iSelectedItem);

			if(aIndex > 0) {
				for(TInt i = (aIndex - 1); i >= 0; i--) {
					CBuddycloudListItem* aItem = iItemStore->GetItemByIndex(i);
					
					if(aItem && aItem->Filtered()) {
						HandleItemSelection(aItem->GetItemId());
						break;
					}
				}
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			if(iItemStore->Count() > 0) {
				TInt aIndex = iItemStore->GetIndexById(iSelectedItem);
				
				if(aIndex == iItemStore->Count() - 1) {
					// Last item
					aIndex = KErrNotFound;
				}
	
				if(aIndex < iItemStore->Count() - 1) {
					for(TInt i = (aIndex + 1); i < iItemStore->Count(); i++) {
						CBuddycloudListItem* aItem = iItemStore->GetItemByIndex(i);
						
						if(aItem && aItem->Filtered()) {
							HandleItemSelection(aItem->GetItemId());
							break;
						}
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
	else if(aType == EEventKeyDown) {
		if(iEdwin->TextLength() == 0 && aKeyEvent.iScanCode == EStdKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KPlacesViewId), TUid::Uid(0), KNullDesC8);
			aResult = EKeyWasConsumed;
		}
	}

	// If key not consumed pass to iEdwin
	if(aResult == EKeyWasNotConsumed) {
		iEdwin->OfferKeyEventL(aKeyEvent, aType);

		if(iEdwinLength == 0 && iEdwin->TextLength() > 0) {
			// Show
			DisplayEdwin(true);
		}
		else if(iEdwinLength > 0 && iEdwin->TextLength() == 0) {
			// Dont Show
			DisplayEdwin(false);
		}
		
		// Filter Contacts
		TBuf<64> aFilterText;
		iEdwin->GetText(aFilterText);
		iEdwinLength = iEdwin->TextLength();
		
		if(iItemStore->SetFilterTextL(aFilterText)) {
			RepositionItems(iSnapToItem);
			RenderScreen();
		}
		
		aResult = EKeyWasConsumed;
	}

	return aResult;
}

void CBuddycloudFollowingContainer::TabChangedL(TInt aIndex) {
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), KNullDesC8);
}

// End of File
