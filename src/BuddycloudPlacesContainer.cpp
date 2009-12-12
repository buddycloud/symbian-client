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

	iPlaceStore = iBuddycloudLogic->GetPlaceStore();

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
				if(iBuddycloudLogic->GetMyMotionState() == EMotionMoving) {
					aPlace->SetDescriptionL(*iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_MOVING));
					CleanupStack::PopAndDestroy(); // R_LOCALIZED_STRING_POSITIONING_MOVING
				}
				else if(iBuddycloudLogic->GetMyMotionState() == EMotionStationary) {
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
					
					if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceCell)) {
						pTextToWrap.Append(*aCellText);
						
						if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceWlan) || iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceGps)) {
							pTextToWrap.Append(KNewSentance);
							pTextToWrap.Append(*aWifiGpsText);
						}
					}
					else if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceWlan) || iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceGps)) {
						HBufC* aStationaryText = iCoeEnv->AllocReadResourceLC(R_LOCALIZED_STRING_POSITIONING_STATIONARY);
						
						pTextToWrap.Append(*aStationaryText);
						CleanupStack::PopAndDestroy(); // aStationaryText
					}
					
					if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceBt)) {
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

void CBuddycloudPlacesContainer::GetHelpContext(TCoeHelpContext& aContext) const {
	aContext.iMajor = TUid::Uid(HLPUID);
	aContext.iContext = KPlacesTab;
	
	if(iPlaceStore->Count() > 0) {
		aContext.iContext = KPlaceManagement;
	}
}

void CBuddycloudPlacesContainer::SizeChanged() {
	CBuddycloudListComponent::SizeChanged();

	RenderWrappedText(iSelectedItem);
	RepositionItems(iSnapToItem);
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
				if(iBuddycloudLogic->GetMyMotionState() > EMotionStationary) {
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

	if(iPlaceStore->Count() > 0) {
		TInt aItemSize = 0;

		TTime aTimeNow;
		aTimeNow.HomeTime();
		
#ifdef __SERIES60_40__
		iListItems.Reset();			
#endif

		for(TInt i = 0; i < iPlaceStore->Count(); i++) {
			CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(i));

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
						if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceCell)) {
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
						}
						else {
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect(0, 0, (iItemIconSize / 2), (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
						}
						
						// Wifi
						if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceWlan)) {
							iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect((iItemIconSize / 2), 0, iItemIconSize, (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
						}
						else {
							iBufferGc->BitBltMasked(TPoint((iLeftBarSpacer + 6 + (iItemIconSize / 2)), (aItemDrawPos + 2)), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect((iItemIconSize / 2), 0, iItemIconSize, (iItemIconSize / 2)), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
						}
						
						// Gps
						if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceGps)) {
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconPositioning, false, iIconMidmapSize), TRect(0, (iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize), iAvatarRepository->GetImage(KIconPositioning, true, iIconMidmapSize), true);
						}
						else {
							iBufferGc->BitBltMasked(TPoint(iLeftBarSpacer + 6, (aItemDrawPos + 2 + (iItemIconSize / 2))), iAvatarRepository->GetImage(KIconNoPosition, false, iIconMidmapSize), TRect(0, (iItemIconSize / 2), (iItemIconSize / 2), iItemIconSize), iAvatarRepository->GetImage(KIconNoPosition, true, iIconMidmapSize), true);
						}
						
						// Bluetooth
						if(iBuddycloudLogic->GetLocationResourceDataAvailable(EResourceBt)) {
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
					if(aPlace->GetItemId() <= 0 || aPlace->GetItemId() == iBuddycloudLogic->GetMyPlaceId()) {
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
						if(aPlace->GetItemId() <= 0 && iBuddycloudLogic->GetMyMotionState() == EMotionStationary) {
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
						if(iBuddycloudLogic->GetMyMotionState() > EMotionStationary) {
							iBufferGc->UseFont(i10NormalFont);
							aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedLearningPlace));
							aBuf.AppendFormat(_L(": %d%%"), iBuddycloudLogic->GetMyPatternQuality());
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
							aBuf.AppendFormat(_L(": %d"), aPlace->GetPopulation());
	
							iBufferGc->UseFont(i10NormalFont);
							aItemDrawPos += i10NormalFont->HeightInPixels();
							iBufferGc->DrawText(aBuf, TPoint(iLeftBarSpacer + 5, aItemDrawPos));
							iBufferGc->DiscardFont();
						}
	
						// Visits
						if(aPlace->GetVisits() > 0) {
							aBuf.Copy(iTextUtilities->BidiLogicalToVisualL(*iLocalizedPlaceVisits));
							aBuf.AppendFormat(_L(": %d"), aPlace->GetVisits());
	
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
								aTime.FormatL(aBuf, _L(": %J%:1%T%B"));
							}
							else if(aTime > aTimeNow - TTimeIntervalDays(6)) {
								aTime.FormatL(aBuf, _L(": %E at %J%:1%T%B"));
							}
							else {
								aTime.FormatL(aBuf, _L(": %J%:1%T%B on %F%N %*D%X"));
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

			// Finish page drawing
			if(aDrawPos > iRect.Height()) {
				break;
			}
		}
	}
	else {
		// No places
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
		
		if(aPlace == NULL) {
			iSelectedItem = KErrNotFound;
		}
		
		for(TInt i = 0; i < iPlaceStore->Count(); i++) {
			aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemByIndex(i));
			
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
					if(aPlace->GetItemId() <= 0 || aPlace->GetItemId() == iBuddycloudLogic->GetMyPlaceId()) {
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
		aMenuPane->SetItemDimmed(EMenuSeePlacesCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeFollowingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeProducingCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeBeenHereCommand, true);
		aMenuPane->SetItemDimmed(EMenuSeeGoingHereCommand, true);
		
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace) {
			if(aPlace->GetGeoloc()->GetReal(EGeolocLatitude) != 0.0 && aPlace->GetGeoloc()->GetReal(EGeolocLongitude) != 0.0) {				
				aMenuPane->SetItemDimmed(EMenuSeeNearbyCommand, false);
			}
			
			aMenuPane->SetItemDimmed(EMenuSeeBeenHereCommand, false);
			aMenuPane->SetItemDimmed(EMenuSeeGoingHereCommand, false);
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
		iBuddycloudLogic->SetCurrentPlaceL();
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
			CGeolocData* aGeoloc = aPlace->GetGeoloc();
			TExplorerQuery aQuery = CExplorerStanzaBuilder::BuildNearbyXmppStanza(aGeoloc->GetReal(EGeolocLatitude), aGeoloc->GetReal(EGeolocLongitude));

			iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_NEARBYTO);
			aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$OBJECT")), 7, aGeoloc->GetString(EGeolocText).Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 7)));		
			TExplorerQueryPckg aQueryPckg(aQuery);		
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iSelectedItem), aQueryPckg);		
		}
	}
	else if(aCommand == EMenuSeeBeenHereCommand || aCommand == EMenuSeeGoingHereCommand) {
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceStore->GetItemById(iSelectedItem));

		if(aPlace) {
			TExplorerQuery aQuery;
			
			if(aCommand == EMenuSeeBeenHereCommand) {
				aQuery = CExplorerStanzaBuilder::BuildPlaceVisitorsXmppStanza(_L8("past"), aPlace->GetItemId());
				iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_WHOSBEENTO);
			}
			else {
				aQuery = CExplorerStanzaBuilder::BuildPlaceVisitorsXmppStanza(_L8("next"), aPlace->GetItemId());
				iEikonEnv->ReadResourceL(aQuery.iTitle, R_LOCALIZED_STRING_TITLE_WHOSGOINGTO);
			}
			
			aQuery.iTitle.Replace(aQuery.iTitle.Find(_L("$PLACE")), 6, aPlace->GetGeoloc()->GetString(EGeolocText).Left((aQuery.iTitle.MaxLength() - aQuery.iTitle.Length() + 6)));		
			
			TExplorerQueryPckg aQueryPckg(aQuery);		
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KExplorerViewId), TUid::Uid(iSelectedItem), aQueryPckg);	
		}
	}
}

TKeyResponse CBuddycloudPlacesContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			TInt aIndex = iPlaceStore->GetIndexById(iSelectedItem);

			if(aIndex > 0) {
				HandleItemSelection(iPlaceStore->GetItemByIndex(aIndex-1)->GetItemId());
			}

			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			TInt aIndex = iPlaceStore->GetIndexById(iSelectedItem);

			if(aIndex < iPlaceStore->Count() - 1) {
				HandleItemSelection(iPlaceStore->GetItemByIndex(aIndex+1)->GetItemId());
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
			iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), KChannelsViewId), TUid::Uid(0), _L8(""));

			aResult = EKeyWasConsumed;
		}
	}

	return aResult;
}

void CBuddycloudPlacesContainer::TabChangedL(TInt aIndex) {
	iCoeEnv->AppUi()->ActivateViewL(TVwsViewId(TUid::Uid(APPUID), TUid::Uid(iTabGroup->TabIdFromIndex(aIndex))), TUid::Uid(0), _L8(""));
}

// End of File
