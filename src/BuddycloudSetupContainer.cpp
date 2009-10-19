/*
============================================================================
 Name        : BuddycloudSetupContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Setup Container
============================================================================
*/

// INCLUDE FILES
#include <akniconutils.h>
#include <aknutils.h>
#include <bacline.h>
#include <barsread.h>
#include <blddef.h>
#include <coecobs.h>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.pan"
#include "Buddycloud.hrh"
#include "BuddycloudSetupContainer.h"
#include "FontUtilities.h"
#include "SetupIconIds.h"
#include "TextUtilities.h"

// ================= MEMBER FUNCTIONS =======================

CBuddycloudSetupContainer::CBuddycloudSetupContainer(CBuddycloudLogic* aBuddycloudLogic) {
	iBuddycloudLogic = aBuddycloudLogic;
}

void CBuddycloudSetupContainer::ConstructL() {	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	iNextText = aTextUtilities->BidiLogicalToVisualL(*iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_NEXT_MENU)).AllocL();
	CleanupStack::PopAndDestroy(2); // R_LOCALIZED_STRING_NEXT_MENU, aTextUtilities
	
	PrecacheImagesL();
	CreateWindowL();	
	
#ifdef __SERIES60_40__
	// Touch
	iTouchFeedback = MTouchFeedback::Instance();
	
	if(!iTouchFeedback->FeedbackEnabledForThisApp()) {
		iTouchFeedback->SetFeedbackEnabledForThisApp(true);
	}
#endif
	
	// Edwin
	iEdwin = new (ELeave) CEikEdwin();
	iEdwin->MakeVisible(false);
	iEdwin->SetFocus(false);
	iEdwin->SetContainerWindowL(*this);

	TResourceReader aReader;
	iEikonEnv->CreateResourceReaderLC(aReader, R_SEARCH_EDWIN);
	iEdwin->ConstructFromResourceL(aReader);
	CleanupStack::PopAndDestroy(); // aReader

	iEdwin->SetAknEditorLocalLanguage(ELangEnglish);
	iEdwin->SetAknEditorFlags(EAknEditorFlagDefault);
	iEdwin->SetOnlyASCIIChars(true);
	
	// Fonts
	ConfigureFonts();

	SetExtentToWholeScreen();	
	ResizeCachedImagesL();
	
	// Setup Step 1
	iSetupStep = EStep1;
	iColorFade = TRgb(255, 255, 255, 255);
	
	iTimer = CCustomTimer::NewL(this);
	iTimer->After(100000);
		
	RenderScreen();
	ActivateL();
}

CBuddycloudSetupContainer::~CBuddycloudSetupContainer() {
	if(iTimer)
		delete iTimer;
	
	// Images
	if(iBufferBitmap)
		delete iBufferBitmap;
	if(iBufferDevice)
		delete iBufferDevice;
	if(iBufferGc)
		delete iBufferGc;
	
	// Logo
	AknIconUtils::DestroyIconData(iLogoBitmap);	

	if(iLogoBitmap)
		delete iLogoBitmap;
	if(iLogoMask)
		delete iLogoMask;
	
	// Splash
	AknIconUtils::DestroyIconData(iSplashBitmap);	
	
	if(iSplashBitmap)
		delete iSplashBitmap;
	if(iSplashMask)
		delete iSplashMask;
	
	// Tick
	AknIconUtils::DestroyIconData(iTickBitmap);	
	
	if(iTickBitmap)
		delete iTickBitmap;
	if(iTickMask)
		delete iTickMask;
	
	// Text
	if(iEdwin)
		delete iEdwin;
	
	if(iNextText)
		delete iNextText;
	
	ClearTextArrayL(iTitleArray);
	iTitleArray.Close();
	ClearTextArrayL(iTextArray);
	iTextArray.Close();
	ClearTextArrayL(iQuestionArray);
	iQuestionArray.Close();
	
	// Fonts
	ReleaseFonts();
}

void CBuddycloudSetupContainer::ConfigureFonts() {
	i12BoldFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontPrimaryFont, true, false);
	iNormalFont = CFontUtilities::GetCustomSystemFont(EAknLogicalFontSecondaryFont, false, false);
	iSymbolFont = iEikonEnv->SymbolFont();
}

void CBuddycloudSetupContainer::ReleaseFonts() {
	CFontUtilities::ReleaseFont(i12BoldFont);
	CFontUtilities::ReleaseFont(iNormalFont);
}

void CBuddycloudSetupContainer::ConstructNextStepL() {
	iTimer->Stop();
	
	if(iSetupStep == EStep1) {
		iSetupStep = EStep2;		
		ConstructTextL();
	}
	else if(iSetupStep == EStep2) {
		iSetupStep = EStep3;		
		ConstructTextL();
		
		iEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
		iEdwin->SelectAllL();

		iEdwin->SetAknEditorCurrentInputMode(EAknEditorTextInputMode);
		iEdwin->SetAknEditorPermittedCaseModes(EAknEditorLowerCase);
		iEdwin->SetAknEditorCurrentCase(EAknEditorLowerCase);
		
		iEdwin->MakeVisible(true);
		iEdwin->SetFocus(true);
	}
	else if(iSetupStep == EStep3) {
		iEdwin->GetText(iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
		iBuddycloudLogic->ValidateUsername();
		
		if(iBuddycloudLogic->GetDescSetting(ESettingItemUsername).Length() == 0) {
			iEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
			iEdwin->SelectAllL();
		}
		else {
			iSetupStep = EStep4;
			ConstructTextL();
			
			iEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemPassword));
			iEdwin->SelectAllL();
			
			iEdwin->SetAknEditorAllowedInputModes(EAknEditorSecretAlphaInputMode);
			iEdwin->SetAknEditorCurrentInputMode(EAknEditorSecretAlphaInputMode);
			iEdwin->SetAknEditorPermittedCaseModes(EAknEditorAllCaseModes);
			iEdwin->SetAknEditorCurrentCase(EAknEditorLowerCase);
			iEdwin->SetAknEditorFlags(EAknEditorFlagNoT9);
		}
	}
	else if(iSetupStep == EStep4) {
		iSetupStep = EStep5;
		ConstructTextL();

		iEdwin->GetText(iBuddycloudLogic->GetDescSetting(ESettingItemPassword));
		
		iEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemEmailAddress));
		iEdwin->SelectAllL();
		
		iEdwin->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
		iEdwin->SetAknEditorCurrentInputMode(EAknEditorTextInputMode);
		iEdwin->SetAknEditorPermittedCaseModes(EAknEditorAllCaseModes);
		iEdwin->SetAknEditorCurrentCase(EAknEditorLowerCase);
		iEdwin->SetAknEditorFlags(EAknEditorFlagDefault);
	}
	else if(iSetupStep == EStep5) {
		iSetupStep = EStep6;
		ConstructTextL();

		iEdwin->GetText(iBuddycloudLogic->GetDescSetting(ESettingItemEmailAddress));
		iBuddycloudLogic->LookupUserByEmailL(true);
		
		iEdwin->MakeVisible(false);
		iEdwin->SetFocus(false);
	}
}

void CBuddycloudSetupContainer::ConstructTextL() {
	if(iSetupStep == EStep2) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP2_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP2_TEXT, iRect.Width() - 6);
	}
	else if(iSetupStep == EStep3) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP3_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP3_TEXT, iRect.Width() - 6);
	}
	else if(iSetupStep == EStep4) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP4_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP4_TEXT, iRect.Width() - 6);
	}
	else if(iSetupStep == EStep5) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP5_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP5_TEXT, iRect.Width() - 6);
	}
	else if(iSetupStep == EStep6) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP6_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP6_TEXT, iRect.Width() - 6);
		WordWrapL(iQuestionArray, iNormalFont, R_LOCALIZED_STRING_STEP6_QUESTION, iRect.Width() - i12BoldFont->FontMaxHeight() - 6);
	}
}

void CBuddycloudSetupContainer::WordWrapL(RPointerArray<HBufC>& aArray, CFont* aFont, TInt aTextResource, TInt aWidth) {
	HBufC* aUnformattedText = iEikonEnv->AllocReadResourceLC(aTextResource);
	TPtr pUnformattedText(aUnformattedText->Des());
	
	// Clear
	ClearTextArrayL(aArray);
	
	// Wrap
	CTextUtilities* aWrapper = CTextUtilities::NewLC();
	aWrapper->WrapToArrayL(aArray, aFont, pUnformattedText, aWidth);
	CleanupStack::PopAndDestroy(2); // aWrapper, aUnformattedText
}

void CBuddycloudSetupContainer::ClearTextArrayL(RPointerArray<HBufC>& aArray) {
	for(TInt x = 0; x < aArray.Count(); x++) {
		delete aArray[x];
	}
	
	aArray.Reset();
}

void CBuddycloudSetupContainer::PrecacheImagesL() {
	TFileName aFileName(_L(":\\resource\\apps\\Buddycloud_Setup.mif"));
	
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
	AknIconUtils::CreateIconL(iLogoBitmap, iLogoMask, aFileName, EMbmSetupiconidsLogo, EMbmSetupiconidsLogo);
	AknIconUtils::PreserveIconData(iLogoBitmap);	
	
	AknIconUtils::CreateIconL(iSplashBitmap, iSplashMask, aFileName, EMbmSetupiconidsSplash, EMbmSetupiconidsSplash);
	AknIconUtils::PreserveIconData(iSplashBitmap);	
	
	AknIconUtils::CreateIconL(iTickBitmap, iTickMask, aFileName, EMbmSetupiconidsTick, EMbmSetupiconidsTick);
	AknIconUtils::PreserveIconData(iTickBitmap);	
}

void CBuddycloudSetupContainer::ResizeCachedImagesL() {
	TSize aSvgSize;	

	// Calculate dimensions
	AknIconUtils::GetContentDimensions(iLogoBitmap, aSvgSize);
	iLogoSize.SetSize(iRect.Width(), TInt(TReal(aSvgSize.iHeight) * (TReal(iRect.Width()) / TReal(aSvgSize.iWidth))));
	
	if(iLogoSize.iHeight > iRect.Height()) {
		iLogoSize.SetSize(iRect.Width() - (iLogoSize.iHeight - iRect.Height()), iRect.Height());
	}
	
	AknIconUtils::SetSize(iLogoBitmap, iLogoSize);
	iLogoSize = iLogoBitmap->SizeInPixels();
	
	AknIconUtils::GetContentDimensions(iSplashBitmap, aSvgSize);
	iSplashSize.SetSize(iRect.Width() / 2, iRect.Width() / 2);
	AknIconUtils::SetSize(iSplashBitmap, iSplashSize);
	iSplashSize = iSplashBitmap->SizeInPixels();
	
	iTickSize.SetSize(i12BoldFont->FontMaxHeight(), i12BoldFont->FontMaxHeight());
	AknIconUtils::SetSize(iTickBitmap, iTickSize);
	iTickSize = iTickBitmap->SizeInPixels();	
}

void CBuddycloudSetupContainer::RenderScreen() {
	iBufferGc->SetPenStyle(CGraphicsContext::ESolidPen);
	iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
	iBufferGc->SetBrushColor(KRgbWhite);
	iBufferGc->Clear(iRect);
	
#ifdef __SERIES60_40__
	iNextButton = TRect();
	iCheckbox = TRect();
	
	if(iSetupStep == EStep1) {
		iNextButton = iRect;
	}
#endif
	
	if(iSetupStep == EStep1) {
		iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
		iBufferGc->BitBltMasked(TPoint((iRect.Width() / 2) - (iLogoSize.iWidth / 2), (iRect.Height() / 2) - (iLogoSize.iHeight / 2)), iLogoBitmap, TRect(0, 0, iLogoSize.iWidth, iLogoSize.iHeight), iLogoMask, true);
		iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
		
		// Fader
		iBufferGc->SetPenColor(iColorFade);
		iBufferGc->SetBrushColor(iColorFade);
		iBufferGc->DrawRect(iRect);	
	}
	else {
		// Splash
		iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
		iBufferGc->DrawBitmapMasked(TRect(iRect.Width() - (iSplashSize.iWidth / 2), 0 - (iSplashSize.iHeight / 2), iRect.Width() + (iSplashSize.iWidth / 2), (iSplashSize.iHeight / 2)), iSplashBitmap, TRect(0, 0, iSplashSize.iWidth, iSplashSize.iHeight), iSplashMask, true);
		iBufferGc->DrawBitmapMasked(TRect(0 - (iSplashSize.iWidth / 2), iRect.Height() - (iSplashSize.iHeight / 2), (iSplashSize.iWidth / 2), iRect.Height() + (iSplashSize.iHeight / 2)), iSplashBitmap, TRect(0, 0, iSplashSize.iWidth, iSplashSize.iHeight), iSplashMask, true);
		iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
		
		// Text
		TInt aTextPos = (iRect.Height() / 2) - (((iTextArray.Count() + iQuestionArray.Count()) * iNormalFont->HeightInPixels()) / 2);
		TInt aTitlePos = (aTextPos / 2) - ((iTitleArray.Count() * i12BoldFont->HeightInPixels()) / 2);
		
		iBufferGc->SetPenColor(KRgbBlack);		
		iBufferGc->UseFont(i12BoldFont);
		
		for(TInt i = 0; i < iTitleArray.Count(); i++) {
			aTitlePos += i12BoldFont->HeightInPixels();
			iBufferGc->DrawText(*iTitleArray[i], TPoint((iRect.Width() / 2) - (i12BoldFont->TextWidthInPixels(*iTitleArray[i]) / 2), aTitlePos));
		}
		
		iBufferGc->DiscardFont();
		iBufferGc->UseFont(iNormalFont);
		
		for(TInt i = 0; i < iTextArray.Count(); i++) {
			aTextPos += iNormalFont->HeightInPixels();
			iBufferGc->DrawText(*iTextArray[i], TPoint(3, aTextPos));
		}
		
		if(iSetupStep == EStep6) {
			// Checkbox
			aTextPos += iNormalFont->HeightInPixels();
					
			iBufferGc->SetPenColor(TRgb(28, 81, 128));
			iBufferGc->SetBrushColor(TRgb(220, 220, 220));
			iBufferGc->DrawRect(TRect(3, aTextPos, 3 + i12BoldFont->FontMaxHeight(), aTextPos + i12BoldFont->FontMaxHeight()));			
			iBufferGc->SetPenColor(KRgbBlack);
	
			if(iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart)) {
				iBufferGc->SetBrushStyle(CGraphicsContext::ENullBrush);
				iBufferGc->BitBltMasked(TPoint(3, aTextPos), iTickBitmap, TRect(0, 0, iTickSize.iWidth, iTickSize.iHeight), iTickMask, true);
				iBufferGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
			}
			
#ifdef __SERIES60_40__
			iCheckbox = TRect(3, aTextPos, 3 + i12BoldFont->FontMaxHeight(), aTextPos + i12BoldFont->FontMaxHeight());
#endif
			
			for(TInt i = 0; i < iQuestionArray.Count(); i++) {
				aTextPos += iNormalFont->HeightInPixels();
				iBufferGc->DrawText(*iQuestionArray[i], TPoint(3 + i12BoldFont->FontMaxHeight()+ 3, aTextPos));
			}
		}
		
		iBufferGc->DiscardFont();
	}
	
	// Edwin
	if(iEdwin->IsVisible()) {
		TRect aFieldRect = iEdwin->Rect();
		aFieldRect.Grow(2, 2);
		iBufferGc->SetBrushColor(KRgbBlack);
		iBufferGc->SetPenColor(KRgbBlack);
		iBufferGc->DrawRoundRect(aFieldRect, TSize(2, 2));
	}
	
	if(iSetupStep == EStep2 || iSetupStep > EStep4 || iEdwin->TextLength() > 0) {
		TPtr pNextText(iNextText->Des());

		iBufferGc->UseFont(iSymbolFont);
		iBufferGc->SetPenColor(KRgbBlack);		
#ifdef __SERIES60_40__
		iBufferGc->DrawText(pNextText, TPoint((iRect.Width() - iSymbolFont->TextWidthInPixels(pNextText) - iSymbolFont->FontMaxHeight() - 1), (iRect.Height() - iSymbolFont->FontMaxHeight() - 2)));
		iNextButton = TRect((iRect.Width() - iSymbolFont->TextWidthInPixels(pNextText) - (iSymbolFont->FontMaxHeight() * 2) - 4), (iRect.Height() - (iSymbolFont->FontMaxHeight() * 5)), iRect.Width(), iRect.Height());
#else
		iBufferGc->DrawText(pNextText, TPoint((iRect.Width() - iSymbolFont->TextWidthInPixels(pNextText) - 1), (iRect.Height() - 2)));
#endif		
		iBufferGc->DiscardFont();
	}
	
	DrawDeferred();
}

void CBuddycloudSetupContainer::TimerExpired(TInt /*aExpiryId*/) {
	User::ResetInactivityTime();
	
	if(iSetupStep == EStep1) {
		if(iColorFade.Alpha() == 0) {
			ConstructNextStepL();
		}
		else {
			iColorFade.SetAlpha(iColorFade.Alpha() - 5);
		}
		
		RenderScreen();
		
		if(iColorFade.Alpha() == 0) {
			iTimer->After(5000000);
		}
		else {
			iTimer->After(100000);
		}
	}
}

void CBuddycloudSetupContainer::HandleResourceChange(TInt aType) {
	if(aType == KEikDynamicLayoutVariantSwitch) {
		SetExtentToWholeScreen();
		RenderScreen();
	}
	else if(aType == KAknsMessageSkinChange){
		ReleaseFonts();
		ConfigureFonts();

		RenderScreen();
	}
}

void CBuddycloudSetupContainer::SizeChanged() {
	iRect = Rect();

	if(iBufferBitmap)
		delete iBufferBitmap;
	if(iBufferDevice)
		delete iBufferDevice;
	if(iBufferGc)
		delete iBufferGc;
	
	ResizeCachedImagesL();
	ConstructTextL();
	
	// Fonts
	ReleaseFonts();
	ConfigureFonts();
	
	// Edwin
	if(iEdwin) {
		TPtr pNextText(iNextText->Des());
		TRect aFieldRect = iEdwin->Rect();
		TInt aFontHeight = iSymbolFont->FontMaxHeight();
#ifdef __SERIES60_40__
		aFieldRect = TRect(aFontHeight, (iRect.Height() - aFieldRect.Height() - aFontHeight - 2), (iRect.Width() - iSymbolFont->TextWidthInPixels(pNextText) - (aFontHeight * 2) - 4), (iRect.Height() - aFontHeight - 2));
#else
		aFieldRect = TRect(aFontHeight, (iRect.Height() - aFieldRect.Height() - aFontHeight - 2), (iRect.Width() - aFontHeight), (iRect.Height() - aFontHeight - 2));
#endif
		iEdwin->SetRect(aFieldRect);
	}
	
	// Initialize Double Buffer
	iBufferBitmap = new (ELeave) CFbsBitmap();
	PanicIfError(_L("SC-BC"), iBufferBitmap->Create(iRect.Size(), CEikonEnv::Static()->ScreenDevice()->DisplayMode()));
	User::LeaveIfNull(iBufferDevice = CFbsBitmapDevice::NewL(iBufferBitmap));
	PanicIfError(_L("SC-CC"), iBufferDevice->CreateContext(iBufferGc));

	CWsScreenDevice * screenDevice = iCoeEnv->ScreenDevice();
	TPixelsAndRotation aPixelAndRotation;
	screenDevice->GetScreenModeSizeAndRotation(screenDevice->CurrentScreenMode(), aPixelAndRotation);
	iBufferGc->SetOrientation(aPixelAndRotation.iRotation);
}

void CBuddycloudSetupContainer::Draw(const TRect& /*aRect*/) const {
	CWindowGc& gc = SystemGc();
	gc.BitBlt(TPoint(0,0), iBufferBitmap);
}

TInt CBuddycloudSetupContainer::CountComponentControls() const {
	if(iSetupStep >= EStep3 && iSetupStep <= EStep5) {
		return 1;
	}
	
	return 0;			
}

CCoeControl* CBuddycloudSetupContainer::ComponentControl(TInt aIndex) const {
	if(iSetupStep >= EStep3 && iSetupStep <= EStep5) {
		if(aIndex == 0) {
			return iEdwin;
		}
	}
	
	return NULL;
}

TKeyResponse CBuddycloudSetupContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;
	
	if(aKeyEvent.iCode >= 63554 && aKeyEvent.iCode <= 63557) {
		aResult = EKeyWasConsumed;
		
		switch(iSetupStep) {
			case EStep1:
			case EStep2:
			case EStep5:
				ConstructNextStepL();
				break;
			case EStep6:
				if(aKeyEvent.iCode == 63557) {
					TBool& aAutoStart = iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart);
					aAutoStart = !aAutoStart;
				}
				else {
					iBuddycloudLogic->ConnectL();
					ReportEventL(MCoeControlObserver::EEventRequestExit);
				}
				break;
			default:
				if(iEdwin->TextLength() > 0) {
					ConstructNextStepL();
				}
				break;
		}
	}
	
	if(aResult == EKeyWasNotConsumed) {
		aResult = EKeyWasConsumed;
	
		if(iEdwin->IsVisible()) {
			iEdwin->OfferKeyEventL(aKeyEvent, aType);
		}
	}
	
	RenderScreen();
	
	return aResult;
}

#ifdef __SERIES60_40__
void CBuddycloudSetupContainer::HandlePointerEventL(const TPointerEvent &aPointerEvent) {
	CCoeControl::HandlePointerEventL(aPointerEvent);
	
	// Handle touch events
	if(aPointerEvent.iType == TPointerEvent::EButton1Up) {
		TKeyEvent aKeyEvent;
		
		if(iNextButton.Contains(aPointerEvent.iPosition)) {
			// Provide feedback
			iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);

			// Trigger event
			aKeyEvent.iCode = 63555;
			OfferKeyEventL(aKeyEvent, EEventKeyUp);			
		}
		else if(iCheckbox.Contains(aPointerEvent.iPosition)) {
			// Provide feedback
			iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);

			// Trigger event
			aKeyEvent.iCode = 63557;
			OfferKeyEventL(aKeyEvent, EEventKeyUp);			
		}	
	}
}
#endif

// End of File  
