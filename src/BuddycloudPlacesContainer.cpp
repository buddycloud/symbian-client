/*
============================================================================
 Name        : BuddycloudPlacesContainer.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Places Container
============================================================================
*/

// INCLUDE FILES
#include <aknnavide.h>
#include <aknmessagequerydialog.h>
#include <aknquerydialog.h>
#include <eikmenup.h>
#include <barsread.h>
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "AutoSelectTextQueryDialog.h"
#include "Buddycloud.hlp.hrh"
#include "BuddycloudPlacesContainer.h"
#include "BuddycloudExplorer.h"
#include "ViewReference.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudPlacesContainer
--
----------------------------------------------------------------------------
*/

CBuddycloudPlacesContainer::CBuddycloudPlacesContainer(MViewAccessorObserver* aViewAccessor, 
		CBuddycloudLogic* aBuddycloudLogic) : CBuddycloudListComponent(aViewAccessor, aBuddycloudLogic) {
}

void CBuddycloudPlacesContainer::ConstructL(const TRect& aRect, TInt aPlaceId) {
	iRect = aRect;
	iSelectedItem = aPlaceId;
	CreateWindowL();

	// Tabs
	iNaviPane = (CAknNavigationControlContainer*) iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi));
	TResourceReader aNaviReader;
	iEikonEnv->CreateResourceReaderLC(aNaviReader, R_DEFAULT_NAVI_DECORATOR);
	iNaviDecorator = iNaviPane->ConstructNavigationDecoratorFromResourceL(aNaviReader);
	CleanupStack::PopAndDestroy();

	iTabGroup = (CAknTabGroup*)iNaviDecorator->DecoratedControl();
	iTabGroup->SetActiveTabById(ETabPlaces);
	iTabGroup->SetObserver(this);

	iNaviPane->PushL(*iNaviDecorator);

	// Construct super
	CBuddycloudListComponent::ConstructL();
	
	// Strings
	iLocalizedPlaceLastSeen = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_PLACELASTSEEN);
	iLocalizedPlaceVisits = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_PLACEVISITS);
	iLocalizedPlacePopulation = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_PLACEPOPULATION);
	iLocalizedLearningPlace = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_LEARNINGPLACE);
	iLocalizedUpdatingPlace = iEikonEnv->AllocReadResourceL(R_LOCALIZED_STRING_NOTE_UPDATINGPLACE);
	
	// Set title
	iTimer->After(15000000);
	HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_PLACESTAB_HINT);
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

	iPlaceStore = iBuddycloudLogic->GetPlaceStore();
	
	iLocationInterface = iBuddycloudLogic->GetLocationInterface();

	SetRect(iRect);
	RenderScreen();
	ActivateL();
}

CBuddycloudPlacesContainer::~CBuddycloudPlacesContainer() {
	// Tabs
	iNaviPane->Pop(iNaviDecorator);

	if(iNaviDecorator)
		delete iNaviDecorator;
	
	// Strings
	if(iLocalizedPlaceLastSeen)
		delete iLocalizedPlaceLastSeen;
	
	if(iLocalizedPlaceVisits)
		delete iLocalizedPlaceVisits;
	
	if(iLocalizedPlacePopulation)
		delete iLocalizedPlacePopulation;
	
	if(iLocalizedLearningPlace)
		delete iLocalizedLearningPlace;
	
	if(iLocalizedUpdatingPlace)
		delete iLocalizedUpdatingPlace;
	
	// Edwin
	if(iEdwin)
		delete iEdwin;
}

void CBuddycloudPlacesContainer::ConfigureEdwinL() {
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

void CBuddycloudPlacesContainer::DisplayEdwin(TBool aShowEdwin) {
	iRect = Rect();

	if(iEdwin) {
		TRect aFieldRect = iEdwin->Rect();
		
		if(aShowEdwin) {
			// Show edwin
			aFieldRect = TRect(i10NormalFont->FontMaxHeight(), (iRect.Height() - i10NormalFont->FontMaxHeight() - 2), (iRect.Width() - i10NormalFont->FontMaxHeight()), (iRect.Height() - 2));
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
			aFieldRect = TRect(i10NormalFont->FontMaxHeight(), (iRect.Height() + 2), (iRect.Width() - i10NormalFont->FontMaxHeight()), (iRect.Height() + aFieldRect.Height() + 2));
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

void CBuddycloudPlacesContainer::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationPlaceItemsUpdated || aEvent == ENotificationLocationUpdated) {
		if(aId == iSelectedItem) {
			RenderWrappedText(iSelectedItem);
		}
		
		RepositionItems(iSnapToItem);
		RenderScreen();
	}
	else {
		CBuddycloudListComponent::NotificationEvent(aEvent, aId);
	}
}

void CBuddycloudPlacesContainer::RenderWrappedText(TInt aIndex) {
	// Clear
	ClearWrappedText();

	if(iPlaceStore->Count() > 0) {
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(aIndex));
		TInt aWidth = (iRect.Width() - iSelectedItemIconTextOffset - 2 - iLeftBarSpacer - iRightBarSpacer);

		if(aPlace) {
			// Wrap
			if(aPlace->GetItemId() <= 0) {
				if(iLocationInterface->GetLastMotionState() == EMotionMoving) {
					aPlace->SetDescriptionL(*iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_MOVING));
					CleanupStack::PopAndDestroy(); // R_LOCALIZED_STRING_POSITIONING_MOVING
				}
				else if(iLocationInterface->GetLastMotionState() == EMotionStationary) {
					aPlace->SetDescriptionL(*iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_STATIONARY));
					CleanupStack::PopAndDestroy(); // R_LOCALIZED_STRING_POSITIONING_STATIONARY
				}
				else {
					_LIT(KNewSentance, ". ");
					
					HBufC* aCellText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_CELL);
					HBufC* aWifiGpsText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_WIFIGPS);
					HBufC* aBtText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_BT);
	
					HBufC* aTextToWrap = HBufC::NewLC(aCellText->Des().Length() + aWifiGpsText->Des().Length() + aBtText->Des().Length() + (KNewSentance().Length() * 2));
					TPtr pTextToWrap(aTextToWrap->Des());
					
					if(iLocationInterface->CellDataAvailable()) {
						pTextToWrap.Append(*aCellText);
						
						if(iLocationInterface->WlanDataAvailable() || iLocationInterface->GpsDataAvailable()) {
							pTextToWrap.Append(KNewSentance);
							pTextToWrap.Append(*aWifiGpsText);
						}
					}
					else if(iLocationInterface->WlanDataAvailable() || iLocationInterface->GpsDataAvailable()) {
						HBufC* aStationaryText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_STATIONARY);
						
						pTextToWrap.Append(*aStationaryText);
						CleanupStack::PopAndDestroy(); // aStationaryText
					}
					
					if(iLocationInterface->BtDataAvailable()) {
						if(pTextToWrap.Length() > 0) {
							pTextToWrap.Append(KNewSentance);
						}
						
						pTextToWrap.Append(*aBtText);
					}
					
					if(pTextToWrap.Length() == 0) {
						HBufC* aNoPositioningText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_NONE);
						
						pTextToWrap.Append(*aNoPositioningText);
						CleanupStack::PopAndDestroy(); // aNoPositioningText
					}
					
					aPlace->SetDescriptionL(pTextToWrap);
					CleanupStack::PopAndDestroy(4); // aTextToWrap, aBtText, aWifiGpsText, aCellText
				}				
			}
				
			iTextUtilities->WrapToArrayL(iWrappedTextArray, i10ItalicFont, aPlace->GetDescription(), aWidth);
		}
	}
	else {
		// No places
		TInt aWidth = (iRect.Width() - 2 - iLeftBarSpacer - iRightBarSpacer);

		HBufC* aNoPlaces = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_PLACESLISTEMPTY);		
		iTextUtilities->WrapToArrayL(iWrappedTextArray, i10BoldFont, *aNoPlaces, aWidth);		
		CleanupStack::PopAndDestroy();	// aNoItems
	}
}

void CBuddycloudPlacesContainer::HandleResourceChange(TInt aType) {
	CBuddycloudListComponent::HandleResourceChange(aType);
	
	if(aType == KAknsMessageSkinChange){
		ConfigureEdwinL();
	}
}

void CBuddycloudPlacesContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
	
	// Edwin
	if(iEdwin) {
		iEdwin->SetTextL(&iPlaceStore->GetFilterText());
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

TInt CBuddycloudPlacesContainer::CountComponentControls() const {
	return 1; // iEdwin
}

CCoeControl* CBuddycloudPlacesContainer::ComponentControl(TInt aIndex) const {
	if(aIndex == 0) {
		return iEdwin;
	}
	
	return NULL;
}

TInt CBuddycloudPlacesContainer::CalculateItemSize(TInt aIndex) {
	CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(aIndex));
	TInt aItemSize = 0;
	TInt aMinimumSize = 0;

	if(aPlace) {
		if(aPlace->GetItemId() == iSelectedItem) {
			aMinimumSize = iItemIconSize + 4;

			aItemSize += i13BoldFont->HeightInPixels();
			aItemSize += i13BoldFont->FontMaxDescent();
			
			if(aPlace->GetDescription().Length() > 0) {
				aItemSize += (iWrappedTextArray.Count() * i10ItalicFont->HeightInPixels());
			}

			if(aItemSize < (iItemIconSize + 2)) {
				aItemSize = (iItemIconSize + 2);
			}

			if(aPlace->GetItemId() <= 0) {				
				if(iLocationInterface->GetLastMotionState() > EMotionStationary) {
					aItemSize += i10NormalFont->HeightInPixels();
				}
			}
			else {	
				if(aPlace->GetPopulation() > 0) {
					aItemSize += i10NormalFont->HeightInPixels();
				}
	
				if(aPlace->GetVisits() > 0) {
					aItemSize += i10NormalFont->HeightInPixels();
				}
	
				if(aPlace->GetPlaceSeen() != EPlaceNotSeen) {
					aItemSize += i10NormalFont->HeightInPixels();
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
			aItemSize += i10BoldFont->FontMaxDescent();
			
			if(aPlace->GetItemId() > 0 && aPlace->GetDescription().Length() > 0) {
				aItemSize += i10ItalicFont->HeightInPixels();
			}
		}

		aItemSize += i10NormalFont->FontMaxDescent() + 2;
	}

	return (aItemSize < aMinimumSize) ? aMinimumSize : aItemSize;
}

void CBuddycloudPlacesContainer::RenderListItems() {
	TBuf<256> aBuf;
	TInt aDrawPos = -iScrollbarHandlePosition;

	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);

	if(iPlaceStore->Count() > 0 && iTotalListSize > 0) {
		TInt aItemSize = 0;

		TTime aTimeNow;
		aTimeNow.HomeTime();
		
#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iPlaceStore->Count(); i++) {
			CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(i));

			if(aPlace->Filtered()) {
				// Calculate item size
				aItemSize = CalculateItemSize(i);
	
				// Test to start the page drawing
				if(aDrawPos + aItemSize > 0) {
					TInt aItemDrawPos = aDrawPos;
					TPtrC aDirectionalText(iTextUtilities->BidiLogicalToVisualL(aPlace->GetTitle()));
					
#ifdef __SERIES60_40__
					iListItems.Append(TListItem(aPlace->GetItemId(), TRect(0, aItemDrawPos, (Rect().Width() - iRightBarSpacer), (aItemDrawPos + aItemSize))));
#endif
					
					// Render frame & avatar
					if(aPlace->GetItemId() == iSelectedItem) {
						RenderItemFrame(TRect(iLeftBarSpacer + 1, aItemDrawPos, (Rect().Width() - iRightBarSpacer), (aItemDrawPos + aItemSize)));
						
						iBufferGc->SetPenColor(iColourTextSelected);
								
						iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 3, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 3), iRect.Height()));
						iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
	
						if(i == 0) {						
							// Cell
							if(iLocationInterface->CellDataAvailable()) {
								iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
							}
							else {
								iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
							}
							
							// Wifi
							if(iLocationInterface->WlanDataAvailable()) {
								iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect((iItemIconSize / 2), 0, iItemIconSize, (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
							}
							else {
								iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect((iItemIconSize / 2), 0, iItemIconSize, (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
							}
							
							// Gps
							if(iLocationInterface->GpsDataAvailable()) {
								iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect(0, (iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
							}
							else {
								iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect(0, (iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
							}
							
							// Bluetooth
							if(iLocationInterface->BtDataAvailable()) {
								iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect((iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
							}
							else {
								iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect((iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
							}
						}
						else {
							// Avatar
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconPlace, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KIconPlace, true, iIconMidmapSize), true);
		
							// Public
							if(!aPlace->Shared()) {
								iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KOverlayLocked, false, iIconMidmapSize), TRect(0, 0, iItemIconSize, iItemIconSize), iAvatarRepository->GetImage(KOverlayLocked, true, iIconMidmapSize), true);
							}
						}
						
						iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
					}
					else {
						iBufferGc->SetClippingRect(TRect(iLeftBarSpacer + 0, aItemDrawPos, (iRect.Width() - iRightBarSpacer - 1), iRect.Height()));
						iBufferGc->SetPenColor(iColourText);
						
#ifdef __SERIES60_40__
						aItemDrawPos += i10BoldFont->DescentInPixels();
#endif
	
						// Avatar
						iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
						iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(KIconPlace, false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconPlace, true, (iIconMidmapSize + 1)), true);
						
						// Public
						if(!aPlace->Shared()) {
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + (iUnselectedItemIconTextOffset / 2) - (iItemIconSize / 4), (aItemDrawPos + 1)), iAvatarRepository->GetImage(KOverlayLocked, false, (iIconMidmapSize + 1)), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KOverlayLocked, true, (iIconMidmapSize + 1)), true);
						}
						
						iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);	
					}
					
					if(i == 0) {
						if(aPlace->GetItemId() <= 0 || aPlace->GetItemId() == iLocationInterface->GetLastPlaceId()) {
							// This is top place item (On the road..., Near... etc)
							CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
							
							if(aOwnItem) {
								CGeolocData* aGeoloc = aOwnItem->GetGeolocItem(EGeolocItemCurrent);
								
								if(aGeoloc->GetString(EGeolocText).Length() > 0) {
									aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(aGeoloc->GetString(EGeolocText)));
								}
							}
						}
					}
					
					if(aDirectionalText.Length() == 0) {
						aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(*iLocalizedUpdatingPlace));
						iBufferGc->SetPenColor(iColourTextSelectedTrans);
					}
					
					// Render content header
					if(aPlace->GetItemId() == iSelectedItem) {
						// Name
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
					}
					else {
						// Name
						iBufferGc->UseFont(i10BoldFont);
						aItemDrawPos += i10BoldFont->HeightInPixels();
						iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
						aItemDrawPos += i10BoldFont->FontMaxDescent();
						iBufferGc->DiscardFont();
					}	
					
					// Render content								
					iBufferGc->UseFont(i10ItalicFont);
				
					if(aPlace->GetItemId() == iSelectedItem) {					
						if(aPlace->GetDescription().Length() > 0) {
							if(aPlace->GetItemId() <= 0 && iLocationInterface->GetLastMotionState() == EMotionStationary) {
								iBufferGc->SetUnderlineStyle(EUnderlineOn);					
							}
							
							// Wrapped text
							for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
								aItemDrawPos += i10ItalicFont->HeightInPixels();
								iBufferGc->DrawText(*iWrappedTextArray[i], TPoint(iLeftBarSpacer + iSelectedItemIconTextOffset, aItemDrawPos));
							}
						}
						
						if(aItemDrawPos - aDrawPos < (iItemIconSize + 2)) {
							aItemDrawPos = (aDrawPos + iItemIconSize + 2);
						}
						
						if(aPlace->GetItemId() <= 0) {
							iBufferGc->SetUnderlineStyle(EUnderlineOff);
							
							// Pattern quality
							if(iLocationInterface->GetLastMotionState() > EMotionStationary) {
								iBufferGc->UseFont(i10NormalFont);
								aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedLearningPlace));
								aBuf.AppendFormat(_L(" : %d%%"), iLocationInterface->GetLastPatternQuality());
								aItemDrawPos += i10NormalFont->HeightInPixels();
								iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
								iBufferGc->DiscardFont();
							}												
						}
						else {						
							iBufferGc->SetPenColor(iColourTextSelected);
		
							// Population
							if(aPlace->GetPopulation() > 0) {
								aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedPlacePopulation));
								aBuf.AppendFormat(_L(" : %d"), aPlace->GetPopulation());
		
								iBufferGc->UseFont(i10NormalFont);
								aItemDrawPos += i10NormalFont->HeightInPixels();
								iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
								iBufferGc->DiscardFont();
							}
		
							// Visits
							if(aPlace->GetVisits() > 0) {
								aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedPlaceVisits));
								aBuf.AppendFormat(_L(" : %d"), aPlace->GetVisits());
		
								if(aPlace->GetVisits() > 1 && aPlace->GetTotalSeconds() > 0) {
									TReal aVisitTime;
									if(aPlace->GetTotalSeconds() < 3600) {
										aVisitTime = (TReal)aPlace->GetTotalSeconds() / 60.0;
										aBuf.AppendFormat(_L(" (%.1f mins)"), aVisitTime);
									}
									else if(aPlace->GetTotalSeconds() < 86400) {
										aVisitTime = ((TReal)aPlace->GetTotalSeconds() / 60.0) / 60.0;
										aBuf.AppendFormat(_L(" (%.1f hours)"), aVisitTime);
									}
									else {
										aVisitTime = (((TReal)aPlace->GetTotalSeconds() / 60.0) / 60.0) / 24.0;
										aBuf.AppendFormat(_L(" (%.1f days)"), aVisitTime);
									}
								}
		
								iBufferGc->UseFont(i10NormalFont);
								aItemDrawPos += i10NormalFont->HeightInPixels();
								iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
								iBufferGc->DiscardFont();
							}
		
							if(aPlace->GetPlaceSeen() != EPlaceNotSeen) {
								// Time
								TTime aTime = aPlace->GetLastSeen();
								aTime += User::UTCOffset();
		
								if(aTime.DayNoInYear() == aTimeNow.DayNoInYear()) {
									aTime.FormatL(aBuf, _L(" : %J%:1%T%B"));
								}
								else if(aTime > aTimeNow - TTimeIntervalDays(6)) {
									aTime.FormatL(aBuf, _L(" : %E at %J%:1%T%B"));
								}
								else {
									aTime.FormatL(aBuf, _L(" : %J%:1%T%B on %F%N %*D%X"));
								}
								
								aBuf.Insert(0, iTextUtilities->BidiLogicalToVisualL(*iLocalizedPlaceLastSeen));
		
								// Visit Length
								if(aPlace->GetVisitSeconds() > 0) {
									TReal aVisitTime;
		
									if(aPlace->GetVisitSeconds() < 3600) {
										aVisitTime = (TReal)aPlace->GetVisitSeconds() / 60.0;
										aBuf.AppendFormat(_L(" (%.1f mins)"), aVisitTime);
									}
									else if(aPlace->GetVisitSeconds() < 86400) {
										aVisitTime = ((TReal)aPlace->GetVisitSeconds() / 60.0) / 60.0;
										aBuf.AppendFormat(_L(" (%.1f hours)"), aVisitTime);
									}
									else {
										aVisitTime = (((TReal)aPlace->GetVisitSeconds() / 60.0) / 60.0) / 24.0;
										aBuf.AppendFormat(_L(" (%.1f days)"), aVisitTime);
									}
								}
		
								iBufferGc->UseFont(i10NormalFont);
								aItemDrawPos += i10NormalFont->HeightInPixels();
								iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
								iBufferGc->DiscardFont();
							}					
						}
					}
					else if(aPlace->GetItemId() > 0 && aPlace->GetDescription().Length() > 0) {
						HBufC* aAddress = aPlace->GetDescription().AllocLC();
						TPtr pAddress(aAddress->Des());
						
						TInt aDisplayedText = i10ItalicFont->TextCount(pAddress, (iRect.Width() - iUnselectedItemIconTextOffset - 2 - iRightBarSpacer));
							
						if(aDisplayedText < pAddress.Length()) {
							pAddress.Delete((aDisplayedText - 3), pAddress.Length());
							pAddress.Append(_L("..."));
						}
							
						aDirectionalText.Set(iTextUtilities->BidiLogicalToVisualL(pAddress));
						aItemDrawPos += i10ItalicFont->HeightInPixels();
						iBufferGc->DrawText(aDirectionalText, TPoint(iLeftBarSpacer + iUnselectedItemIconTextOffset, aItemDrawPos));
	
						CleanupStack::PopAndDestroy(); // aAddress
					}
					
					iBufferGc->DiscardFont();
					
					// Collect details if required
					if(aPlace->GetRevision() == KErrNotFound) {
						iBuddycloudLogic->GetPlaceDetailsL(aPlace->GetItemId());
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
	else {
		iBufferGc->UseFont(i10BoldFont);
		iBufferGc->SetPenColor(iColourText);

		if(iPlaceStore->Count() > 0) {
			// No results
			aDrawPos = ((iRect.Height() / 2) - (i10BoldFont->HeightInPixels() / 2));
			
			HBufC* aNoResults = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);		
			iBufferGc->DrawText(*aNoResults, TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2)) - (i10BoldFont->TextWidthInPixels(*aNoResults) / 2)), aDrawPos));
			CleanupStack::PopAndDestroy();// aNoResults	
		}
		else {
			// No places
			aDrawPos = ((iRect.Height() / 2) - ((iWrappedTextArray.Count() * i10BoldFont->HeightInPixels()) / 2));
						
			for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
				aDrawPos += i10ItalicFont->HeightInPixels();
				iBufferGc->DrawText(iWrappedTextArray[i]->Des(), TPoint(((iLeftBarSpacer + ((iRect.Width() - iLeftBarSpacer - iRightBarSpacer) / 2)) - (i10BoldFont->TextWidthInPixels(iWrappedTextArray[i]->Des()) / 2)), aDrawPos));
			}
		}
		
		iBufferGc->DiscardFont();
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

void CBuddycloudPlacesContainer::RepositionItems(TBool aSnapToItem) {
	iTotalListSize = 0;
	
	// Enable snapping again
	if(aSnapToItem) {
		iSnapToItem = aSnapToItem;
		iScrollbarHandlePosition = 0;
	}
	
	if(iPlaceStore->Count() > 0) {
		TInt aItemSize;

		// Check if current item exists
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));
		
		if(aPlace == NULL || !aPlace->Filtered()) {
			iSelectedItem = KErrNotFound;
		}
		
		for(TInt i = 0; i < iPlaceStore->Count(); i++) {
			aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(i));
			
			if(aPlace && aPlace->Filtered()) {
				if(iSelectedItem == KErrNotFound) {
					iSelectedItem = aPlace->GetItemId();
					RenderWrappedText(iSelectedItem);
				}
				
				aItemSize = CalculateItemSize(i);
				
				if(aSnapToItem && aPlace->GetItemId() == iSelectedItem) {
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

void CBuddycloudPlacesContainer::HandleItemSelection(TInt aItemId) {
	if(iSelectedItem != aItemId) {
		// New item selected
		iSelectedItem = aItemId;
		
		RenderWrappedText(iSelectedItem);		
		RepositionItems(true);
		RenderScreen();
	}
	else {
		// Trigger item event
		if(iPlaceStore->Count() > 0) {
			iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_PLACES_ITEM_MENUBAR);
			iViewAccessor->ViewMenuBar()->TryDisplayMenuBarL();
			iViewAccessor->ViewMenuBar()->SetMenuTitleResourceId(R_PLACES_OPTIONS_MENUBAR);
		}
	}
}

void CBuddycloudPlacesContainer::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane) {
	if(aResourceId == R_PLACES_OPTIONS_MENU) {
		aMenuPane->SetItemDimmed(EMenuConnectCommand, true);
		aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, true);
		aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetNextPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuDisconnectCommand, true);
		
		if(iBuddycloudLogic->GetState() == ELogicOnline) {
			aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, false);
			aMenuPane->SetItemDimmed(EMenuSetNextPlaceCommand, false);
			aMenuPane->SetItemDimmed(EMenuDisconnectCommand, false);
		}
		else {
			aMenuPane->SetItemDimmed(EMenuConnectCommand, false);
		}
		
		if(iPlaceStore->Count() > 0) {
			CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));
			TInt aPlaceIndex = iPlaceStore->GetIndexById(iSelectedItem);
			
			if(aPlace) {
				TPtrC aPlaceName(aPlace->GetGeoloc()->GetString(EGeolocText));
				
				if(aPlaceIndex == 0) {
					if(aPlace->GetItemId() <= 0 || aPlace->GetItemId() == iLocationInterface->GetLastPlaceId()) {
						CFollowingRosterItem* aOwnItem = iBuddycloudLogic->GetOwnItem();
						
						if(aOwnItem) {
							CGeolocData* aGeoloc = aOwnItem->GetGeolocItem(EGeolocItemCurrent);
							
							if(aGeoloc->GetString(EGeolocText).Length() > 0) {
								aPlaceName.Set(aGeoloc->GetString(EGeolocText));
							}
						}
					}
				}

				if(aPlaceName.Length() > 32) {
					aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aPlaceName.Left(32));
				}
				else {
					aMenuPane->SetItemTextL(EMenuOptionsItemCommand, aPlaceName);
				}
					
				aMenuPane->SetItemDimmed(EMenuOptionsItemCommand, false);				
			}
		}
	}
	else if(aResourceId == R_PLACES_OPTIONS_ITEM_MENU) {
		aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuEditPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuSetAsPlaceCommand, true);
		aMenuPane->SetItemDimmed(EMenuDeletePlaceCommand, true);

		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace) {
			if(aPlace->GetItemId() <= 0) {
				aMenuPane->SetItemDimmed(EMenuBookmarkPlaceCommand, false);
			}
			else {
				if(aPlace->GetPlaceSeen() != EPlaceHere) {
					aMenuPane->SetItemDimmed(EMenuSetAsNextPlaceCommand, false);
				}
	
				aMenuPane->SetItemDimmed(EMenuSetAsPlaceCommand, false);
				aMenuPane->SetItemDimmed(EMenuEditPlaceCommand, false);
				aMenuPane->SetItemDimmed(EMenuDeletePlaceCommand, false);
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
		
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace && (aPlace->GetGeoloc()->GetReal(EGeolocLatitude) != 0.0 || aPlace->GetGeoloc()->GetReal(EGeolocLongitude) != 0.0)) {				
			aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
		}
	}
}

void CBuddycloudPlacesContainer::HandleCommandL(TInt aCommand) {
	if(aCommand == EMenuConnectCommand) {
		iBuddycloudLogic->ConnectL();
	}
	else if(aCommand == EMenuDisconnectCommand) {
		iBuddycloudLogic->Disconnect();
	}
	else if(aCommand == EMenuGetPlaceInfoCommand) {
		iBuddycloudLogic->GetPlaceDetailsL(iSelectedItem);
	}
	else if(aCommand == EMenuBookmarkPlaceCommand) {
		if(iBuddycloudLogic->GetState() == ELogicOnline) {
			iBuddycloudLogic->SetCurrentPlaceL();
		}
	}
	else if(aCommand == EMenuSetAsPlaceCommand) {
		HBufC* aHeaderText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_RELEARNPLACE_TITLE);
		HBufC* aMessageText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_RELEARNPLACE_TEXT);
		
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
		aDialog->PrepareLC(R_EXIT_DIALOG);
		aDialog->SetHeaderText(*aHeaderText);
		aDialog->SetMessageText(*aMessageText);

		if(aDialog->RunLD() != 0) {
			iBuddycloudLogic->SetCurrentPlaceL(iSelectedItem);
		}
		
		CleanupStack::PopAndDestroy(2); // aMessageText, aHeaderText
	}
	else if(aCommand == EMenuSetNextPlaceCommand) {
		TInt aPlaceId;
		TBuf<256> aPlaceLabel;

		CAutoSelectTextQueryDialog* aAutoDialog = new (ELeave) CAutoSelectTextQueryDialog(aPlaceId, aPlaceLabel);
		aAutoDialog->PrepareLC(R_PLACE_DIALOG);
		
		HBufC* aPrompt = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_DIALOG_SETNEXTLOCATION);
		aAutoDialog->SetPromptL(*aPrompt);
		aAutoDialog->SetPredictiveTextInputPermitted(true);
		CleanupStack::PopAndDestroy();

		// Get My Places
		for(TInt i = 0;i < iPlaceStore->Count();i++) {
			CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(i));

			if(aPlace && aPlace->GetPlaceSeen() != EPlaceHere && aPlace->GetGeoloc()->GetString(EGeolocText).Length() > 0) {
				aAutoDialog->AddAutoSelectTextL(aPlace->GetItemId(), aPlace->GetGeoloc()->GetString(EGeolocText));
			}
		}

		if(aAutoDialog->RunLD() != 0) {
			iBuddycloudLogic->SetNextPlaceL(aPlaceLabel, aPlaceId);
		}	
	}
	else if(aCommand == EMenuSetAsNextPlaceCommand) {
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace) {
			iBuddycloudLogic->SetNextPlaceL(aPlace->GetGeoloc()->GetString(EGeolocText), aPlace->GetItemId());
		}
	}
	else if(aCommand == EMenuEditPlaceCommand) {
		iPlaceStore->SetEditedPlace(iSelectedItem);

		iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KEditPlaceViewId), TUid::Uid(iSelectedItem), _L8(""));
	}
	else if(aCommand == EMenuDeletePlaceCommand) {
		CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();

		if(dlg->ExecuteLD(R_DELETE_DIALOG) != 0) {
			iBuddycloudLogic->EditMyPlacesL(iSelectedItem, false);
			RenderScreen();
		}
	}
	else if(aCommand == EMenuSeeNearbyCommand) {
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace) {
			TViewReferenceBuf aViewReference;
			aViewReference().iCallbackRequested = true;
			aViewReference().iCallbackViewId = KPlacesViewId;
			aViewReference().iOldViewData.iId = iSelectedItem;
			
			CExplorerStanzaBuilder::FormatButlerXmppStanza(aViewReference().iNewViewData.iData, iBuddycloudLogic->GetNewIdStamp(), aPlace->GetGeoloc()->GetReal(EGeolocLatitude), aPlace->GetGeoloc()->GetReal(EGeolocLongitude));
			CExplorerStanzaBuilder::BuildTitleFromResource(aViewReference().iNewViewData.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO, _L("$OBJECT"), aPlace->GetGeoloc()->GetString(EGeolocText));
			
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), aViewReference);		
		}
	}
	else if(aCommand == EMenuNewSearchCommand || aCommand == EAknSoftkeyBack) {
		if(aCommand == EAknSoftkeyBack && iPlaceStore->GetFilterText().Length() == 0) {
			// Back to Following tab
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId));
		}
		else {	
			// Reset filter
			iPlaceStore->SetFilterTextL(_L(""));
			iEdwin->SetTextL(&iPlaceStore->GetFilterText());
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

TKeyResponse CBuddycloudPlacesContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			TInt aIndex = iPlaceStore->GetIndexById(iSelectedItem);

			if(aIndex > 0) {
				for(TInt i = (aIndex - 1); i >= 0; i--) {
					CBuddycloudListItem* aPlace = iPlaceStore->GetItemByIndex(i);
					
					if(aPlace && aPlace->Filtered()) {
						HandleItemSelection(aPlace->GetItemId());
						break;
					}
				}
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			if(iPlaceStore->Count() > 0) {
				TInt aIndex = iPlaceStore->GetIndexById(iSelectedItem);
				
				if(aIndex == iPlaceStore->Count() - 1) {
					// Last item
					aIndex = KErrNotFound;
				}
	
				if(aIndex < iPlaceStore->Count() - 1) {
					for(TInt i = (aIndex + 1); i < iPlaceStore->Count(); i++) {
						CBuddycloudListItem* aPlace = iPlaceStore->GetItemByIndex(i);
						
						if(aPlace && aPlace->Filtered()) {
							HandleItemSelection(aPlace->GetItemId());
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
		if(aKeyEvent.iScanCode == EStdKeyLeftArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KFollowingViewId), TUid::Uid(0), _L8(""));

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iScanCode == EStdKeyRightArrow) {
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(0), _L8(""));

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
		
		if(iPlaceStore->SetFilterTextL(aFilterText)) {
			RepositionItems(iSnapToItem);
			RenderScreen();
		}
		
		aResult = EKeyWasConsumed;
	}

	return aResult;
}

void CBuddycloudPlacesContainer::TabChangedL(TInt aIndex) {
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), _L8(""));
}

// End of File
