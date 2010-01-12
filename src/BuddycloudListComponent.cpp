/*
============================================================================
 Name        : BuddycloudListComponent.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Nearby Container
============================================================================
*/

// INCLUDE FILES
#include <aknsdrawutils.h> 
#include <akntitle.h>
#include <eikenv.h>
#include <akndef.h>
#include <gdi.h>
#include <AknUtils.h>
#include <AknsDrawUtils.h>
#include <e32std.h>
#include <blddef.h>
#include <Buddycloud_lang.rsg>
#include "BuddycloudListComponent.h"
#include "FontUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudListComponent
--
----------------------------------------------------------------------------
*/

CBuddycloudListComponent::CBuddycloudListComponent(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic) {
	iViewAccessor = aViewAccessor;
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudListComponent::ConstructL() {
	// Initialize timer & set title
	iTimer = CCustomTimer::NewL(this, KTimeTimerId);
	
	// Scrollbar
	iScrollBar = new (ELeave) CEikScrollBarFrame(this, this);
	iScrollBar->CreateDoubleSpanScrollBarsL(true, false, true, false);
	iScrollBar->SetTypeOfVScrollBar(CEikScrollBarFrame::EDoubleSpan);
	iScrollBar->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOn);
	
#ifdef __SERIES60_40__
	// Touch
	iTouchFeedback = MTouchFeedback::Instance();
	
	EnableDragEvents();
	
	if(!iTouchFeedback->FeedbackEnabledForThisApp()) {
		iTouchFeedback->SetFeedbackEnabledForThisApp(true);
	}
	
	iDragTimer = CCustomTimer::NewL(this, KDragTimerId);
	
	iUnselectedItemIconTextOffset = 40;
	iSelectedItemIconTextOffset = 72;
	iItemIconSize = 64;
	iIconMidmapSize = ELevelLarge;
#else
	iUnselectedItemIconTextOffset = 20;
	iSelectedItemIconTextOffset = 40;
	iItemIconSize = 32;
	iIconMidmapSize = ELevelNormal;
#endif
	
	iSnapToItem = true;
	iShowMialog = true;
	
	iTextUtilities = CTextUtilities::NewL();

	ConfigureSkin();
	ConfigureFonts();

	// Buddycloud Logic
	iBuddycloudLogic->AddNotificationObserver(this);

	iAvatarRepository = iBuddycloudLogic->GetImageRepository();
}

CBuddycloudListComponent::~CBuddycloudListComponent() {
	if(iBuddycloudLogic) {
		iBuddycloudLogic->RemoveNotificationObserver(this);
	}

	if(iTimer)
		delete iTimer;
	
#ifdef __SERIES60_40__
	if(iDragTimer)
		delete iDragTimer;
#endif
	
	// Skins
	if(iBgContext)
		delete iBgContext;

	// Bitmaps
	if(iBufferBitmap)
		delete iBufferBitmap;
	if(iBufferDevice)
		delete iBufferDevice;
	if(iBufferGc)
		delete iBufferGc;
	
	// Wrapped text
	ClearWrappedText();
	iWrappedTextArray.Close();
	
	if(iTextUtilities)
		delete iTextUtilities;
	
	// Scrollbar
	if(iScrollBar)
		delete iScrollBar;

	// Fonts
	ReleaseFonts();
	
#ifdef __SERIES60_40__
	iListItems.Close();
#endif
}

void CBuddycloudListComponent::SetTitleL(const TDesC& aTitle) {
	CAknTitlePane* aTitlePane = static_cast<CAknTitlePane*>(iEikonEnv->AppUiFactory()->StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));

	aTitlePane->SetTextL(aTitle);
}

void CBuddycloudListComponent::ConfigureFonts() {
	i13BoldFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontPrimaryFont, true, false);
	i10ItalicFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontSecondaryFont, false, true);
	i10NormalFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontSecondaryFont, false, false);
	i10BoldFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontSecondaryFont, true, false);	
}

void CBuddycloudListComponent::ReleaseFonts() {
	CFontUtilities::ReleaseFont(i13BoldFont);
	CFontUtilities::ReleaseFont(i10ItalicFont);
	CFontUtilities::ReleaseFont(i10NormalFont);
	CFontUtilities::ReleaseFont(i10BoldFont);
}

void CBuddycloudListComponent::ConfigureSkin() {
	// Background
	if(iBgContext) {
		delete iBgContext;
	}
	
	iBgContext = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgAreaMain, iRect, false);
	
	// Colors
	AknsUtils::GetCachedColor(AknsUtils::SkinInstance(), iColourTextSelected, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG10);
	// AknsUtils::GetCachedColor(AknsUtils::SkinInstance(), iColourTextLink, KAknsIIDQsnHighlightColors, EAknsCIQsnHighlightColorsCG3);
	AknsUtils::GetCachedColor(AknsUtils::SkinInstance(), iColourTextLink, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG21);
	AknsUtils::GetCachedColor(AknsUtils::SkinInstance(), iColourText, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6);
	
	iColourTextLink = TRgb((iColourTextLink.Red() + iColourTextSelected.Red()) / 2, (iColourTextLink.Green() + iColourTextSelected.Green()) / 2, (iColourTextLink.Blue() + iColourTextSelected.Blue()) / 2);
	
	iColourTextSelectedTrans = iColourTextSelected;
	iColourTextSelectedTrans.SetAlpha(125);
	
	iColourTextTrans = iColourText;
	iColourTextTrans.SetAlpha(125);
}

void CBuddycloudListComponent::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt /*aId*/) {
	if(aEvent == ENotificationActivityChanged) {
		RenderScreen();
	}
	else if(aEvent == ENotificationLogicEngineDestroyed) {
		iBuddycloudLogic = NULL;
	}
}

void CBuddycloudListComponent::TimerExpired(TInt aExpiryId) {
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
		HBufC* aTitle = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_APPNAME);
		SetTitleL(*aTitle);
		CleanupStack::PopAndDestroy();
#else
		TTime aTime;
		aTime.HomeTime();
		TBuf<32> aTextTime;
		aTime.FormatL(aTextTime, _L("%J%:1%T%B"));
	
		SetTitleL(aTextTime);
	
		TDateTime aDateTime = aTime.DateTime();
		iTimer->After((60-aDateTime.Second()+1)*1000);
#endif
	}
}

TTypeUid::Ptr CBuddycloudListComponent::MopSupplyObject(TTypeUid aId) {
	if(aId.iUid == MAknsControlContext::ETypeId && iBgContext) {
		return MAknsControlContext::SupplyMopObject(aId, iBgContext);
	}

	return CCoeControl::MopSupplyObject(aId);
}

void CBuddycloudListComponent::HandleResourceChange(TInt aType) {
	if(aType == KEikDynamicLayoutVariantSwitch) {
		TRect aRect;
		AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, aRect);
		SetRect(aRect);
		RenderScreen();
	}
	else if(aType == KAknsMessageSkinChange){
		ReleaseFonts();
		ConfigureFonts();
		ConfigureSkin();

		if(iScrollBar) {
			iScrollBar->VerticalScrollBar()->HandleResourceChange(aType);
		}
		
		RenderScreen();
	}
}

void CBuddycloudListComponent::SizeChanged() {
	iRect = Rect();

	if(iBufferBitmap)
		delete iBufferBitmap;
	if(iBufferDevice)
		delete iBufferDevice;
	if(iBufferGc)
		delete iBufferGc;
	
	// Fonts
	ReleaseFonts();
	ConfigureFonts();
	
	// Scrollbar
	if(iScrollBar) {
		iLayoutMirrored = false;

		iLeftBarSpacer = 1;
		iRightBarSpacer = iScrollBar->VerticalScrollBar()->ScrollBarBreadth();
	}
	
	if(AknLayoutUtils::LayoutMirrored()) {
		iLayoutMirrored = true;
		
		iLeftBarSpacer = (iRightBarSpacer * 2);
		iRightBarSpacer = 1;
	}

	// Skins
	if(iBgContext) {
		iBgContext->SetRect(iRect);
		iBgContext->SetParentPos(PositionRelativeToScreen());
	}

	// Initialize Double Buffer
	iBufferBitmap = new (ELeave) CFbsBitmap();
	PanicIfError(_L("MC-BC"), iBufferBitmap->Create(iRect.Size(), CEikonEnv::Static()->ScreenDevice()->DisplayMode()));
	User::LeaveIfNull(iBufferDevice = CFbsBitmapDevice::NewL(iBufferBitmap));
	PanicIfError(_L("MC-CC"), iBufferDevice->CreateContext(iBufferGc));

	CWsScreenDevice * screenDevice = iCoeEnv->ScreenDevice();
	TPixelsAndRotation aPixelAndRotation;
	screenDevice->GetScreenModeSizeAndRotation(screenDevice->CurrentScreenMode(), aPixelAndRotation);
	iBufferGc->SetOrientation(aPixelAndRotation.iRotation);
}

void CBuddycloudListComponent::ClearWrappedText() {
	for(TInt i = 0; i < iWrappedTextArray.Count(); i++) {
		delete iWrappedTextArray[i];
	}
	
	iWrappedTextArray.Reset();
}

void CBuddycloudListComponent::RenderItemFrame(TRect aFrame) {
	TRect aInner(aFrame);
	aInner.Shrink(7, 7);

	iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
	AknsDrawUtils::DrawFrame(AknsUtils::SkinInstance(), *iBufferGc, aFrame, aInner, KAknsIIDQsnFrList, KAknsIIDDefault);
	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
}

void CBuddycloudListComponent::RenderScreen() {
	// Draw skin
	AknsDrawUtils::DrawBackground(AknsUtils::SkinInstance(), iBgContext, this, *iBufferGc, TPoint(0,0), Rect(), KAknsDrawParamDefault);
	
	// Call list renderer
	RenderListItems();
	
	// Activity dialog
	if(iShowMialog) {
		TBuf<256> aMialogLine1(iBuddycloudLogic->GetActivityStatus().Left(256));
		TBuf<256> aMialogLine2;
		
		iBufferGc->SetClippingRect(iRect);
		
		if(aMialogLine1.Length() > 0) {
			// Draw Activity Status
			TInt aSearch = aMialogLine1.Locate('\n');
			
			if(aSearch != KErrNotFound) {
				aMialogLine2.Copy(aMialogLine1);
				aMialogLine1.Delete(aSearch, aMialogLine1.Length());
				aMialogLine2.Delete(0, (aSearch + 1));
				
				// Redraw frame background & frame
				TInt aMialogHeight = (i10ItalicFont->FontMaxHeight() * 2) + 4;
				TInt aLongestLine = (i10ItalicFont->TextWidthInPixels(aMialogLine1) > i10ItalicFont->TextWidthInPixels(aMialogLine2) ? i10ItalicFont->TextWidthInPixels(aMialogLine1) : i10ItalicFont->TextWidthInPixels(aMialogLine2));			
				TRect aFrame = TRect((iRect.Width() - aLongestLine - 35), (iRect.Height() - aMialogHeight), (iRect.Width() - 25), (iRect.Height() + 5));
				AknsDrawUtils::DrawBackground(AknsUtils::SkinInstance(), iBgContext, this, *iBufferGc, aFrame.iTl, aFrame, KAknsDrawParamDefault);
				RenderItemFrame(aFrame);
				
				iBufferGc->SetPenColor(iColourTextSelected);		
				iBufferGc->UseFont(i10ItalicFont);
				iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aMialogLine1), TPoint((iRect.Width() - aLongestLine - 31), (iRect.Height() - i10ItalicFont->FontMaxHeight() - i10ItalicFont->FontMaxDescent() - 2)));
				iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aMialogLine2), TPoint((iRect.Width() - aLongestLine - 31), (iRect.Height() - i10ItalicFont->FontMaxDescent() - 2)));
				iBufferGc->DiscardFont();
			}
			else {
				TRect aFrame = TRect((iRect.Width() - i10ItalicFont->TextWidthInPixels(aMialogLine1) - 35), (iRect.Height() - i10ItalicFont->FontMaxHeight() - 4), (iRect.Width() - 25), (iRect.Height() + 5));
				AknsDrawUtils::DrawBackground(AknsUtils::SkinInstance(), iBgContext, this, *iBufferGc, aFrame.iTl, aFrame, KAknsDrawParamDefault);
				RenderItemFrame(aFrame);
				
				iBufferGc->SetPenColor(iColourTextSelected);
				iBufferGc->UseFont(i10ItalicFont);
				iBufferGc->DrawText(iTextUtilities->BidiLogicalToVisualL(aMialogLine1), TPoint((iRect.Width() - i10ItalicFont->TextWidthInPixels(aMialogLine1) - 30), (iRect.Height() - i10ItalicFont->FontMaxDescent() - 2)));
				iBufferGc->DiscardFont();
			}		
		}
	
		iBufferGc->CancelClippingRect();
	}
	
	DrawDeferred();
}

void CBuddycloudListComponent::RepositionItems(TBool /*aSnapToItem*/) {	
	// If near to bottom, adjust handle positioning
	if(iTotalListSize > iRect.Height() && iTotalListSize - iScrollbarHandlePosition < iRect.Height()) {
		iScrollbarHandlePosition = iTotalListSize - iRect.Height();
#ifdef __SERIES60_40__
		iDraggingAllowed = false;
#endif
	}
	else if(iScrollbarHandlePosition < 0 || iTotalListSize <= iRect.Height()) {
		iScrollbarHandlePosition = 0;
#ifdef __SERIES60_40__
		iDraggingAllowed = false;
#endif
	}
	
	// Scrollbar
	TRect aScrollBarFrameRect(iRect);
	TEikScrollBarFrameLayout aLayout;
	aLayout.iTilingMode = TEikScrollBarFrameLayout::EInclusiveRectConstant;

	iScrollBarVModel.iScrollSpan = iTotalListSize;
	iScrollBarVModel.iThumbSpan = (iTotalListSize < iRect.Height() ? iTotalListSize : iRect.Height());
	iScrollBarVModel.iThumbPosition = iScrollbarHandlePosition;	
	iScrollBarVModel.CheckBounds();	
	
	iScrollBar->TileL(&iScrollBarHModel, &iScrollBarVModel, aScrollBarFrameRect, aScrollBarFrameRect, aLayout);
	iScrollBar->MoveVertThumbTo(iScrollBarVModel.iThumbPosition);
}

void CBuddycloudListComponent::Draw(const TRect& /*aRect*/) const {
	CWindowGc& gc = SystemGc();
	gc.BitBlt(TPoint(0,0), iBufferBitmap);
}

void CBuddycloudListComponent::HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent /*aEventType*/) {
#ifdef __SERIES60_40__
	if(aScrollBar) {
		iSnapToItem = false;
		
		iDragTimer->Stop();
		iDragVelocity = 0.0;
		
		iScrollBarVModel.iThumbPosition = aScrollBar->ThumbPosition();	
		iScrollBarVModel.CheckBounds();	
		
		iScrollbarHandlePosition = iScrollBarVModel.iThumbPosition;	
		
		RenderScreen();
	}
#endif
}
		
#ifdef __SERIES60_40__
void CBuddycloudListComponent::HandlePointerEventL(const TPointerEvent &aPointerEvent) {
	CCoeControl::HandlePointerEventL(aPointerEvent);
	
	if(aPointerEvent.iType == TPointerEvent::EButton1Up) {			
		if(iDraggingAllowed) {
			if(Abs(iDragVelocity) > 5.0) {
				TimerExpired(KDragTimerId);
			}
		}
		else {
			for(TInt i = 0; i < iListItems.Count(); i++) {
				if(iListItems[i].iRect.Contains(aPointerEvent.iPosition)) {
					// Provide feedback
					iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);
					
					HandleItemSelection(iListItems[i].iId);			
					break;
				}
			}
		}
	}
	else if(aPointerEvent.iType == TPointerEvent::EButton1Down) {
		iDragTimer->Stop();
		iDragVelocity = 0.0;
		iDraggingAllowed = false;
		
		iStartDragPosition = aPointerEvent.iPosition.iY;
		iStartDragHandlePosition = iScrollbarHandlePosition;
		
		iLastDragTime.UniversalTime();
		iLastDragPosition = iStartDragPosition;
	}
	else if(aPointerEvent.iType == TPointerEvent::EDrag) {	
		if(!iDraggingAllowed && (aPointerEvent.iPosition.iY + 32 < iStartDragPosition || aPointerEvent.iPosition.iY - 32 > iStartDragPosition)) {
			iDraggingAllowed = true;			
			iSnapToItem = false;
		}
		
		if(iDraggingAllowed) {
			TTime aNow;			
			aNow.UniversalTime();
			
			iDragVelocity = TReal(TReal(iLastDragPosition - aPointerEvent.iPosition.iY) * (1000000.0 / TReal(aNow.MicroSecondsFrom(iLastDragTime).Int64()))) / 20.0;
			
			iLastDragTime.UniversalTime();
			iLastDragPosition = aPointerEvent.iPosition.iY;
			
			iScrollbarHandlePosition = iStartDragHandlePosition + (iStartDragPosition - aPointerEvent.iPosition.iY);
			
			CBuddycloudListComponent::RepositionItems(false);
			RenderScreen();
		}
	}
}
#endif

// End of File
