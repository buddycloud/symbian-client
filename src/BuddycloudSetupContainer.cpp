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
	
	EnableDragEvents();
	
	if(!iTouchFeedback->FeedbackEnabledForThisApp()) {
		iTouchFeedback->SetFeedbackEnabledForThisApp(true);
	}
#endif
	
	// Edwin
	ConfigureEdwinsL();
	
	// Text
	iLabelText1 = iCoeEnv->AllocReadResourceL(R_LOCALIZED_STRING_STEP3_LABEL1);
	iLabelText2 = iCoeEnv->AllocReadResourceL(R_LOCALIZED_STRING_STEP3_LABEL2);

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
	
	// Label data
	if(iUsernameEdwin)
		delete iUsernameEdwin;
	if(iPasswordEdwin)
		delete iPasswordEdwin;
	
	// Text
	if(iNextText)
		delete iNextText;

	if(iLabelText1)
		delete iLabelText1;
	if(iLabelText2)
		delete iLabelText2;
	
	ClearTextArrayL(iTitleArray);
	iTitleArray.Close();
	ClearTextArrayL(iTextArray);
	iTextArray.Close();
	ClearTextArrayL(iQuestionArray);
	iQuestionArray.Close();
	
	// Fonts
	ReleaseFonts();
}

void CBuddycloudSetupContainer::RepositionButtons() {
	TInt aFontHeight = iSymbolFont->FontMaxHeight();
	
	iLabelRect2.SetRect(aFontHeight, iRect.Height() - 5 - (aFontHeight * 2), iRect.Width() - aFontHeight, iRect.Height() - 5);
	iLabelRect1.SetRect(aFontHeight, iLabelRect2.iTl.iY - 5 - (aFontHeight * 2), iRect.Width() - aFontHeight, iLabelRect2.iTl.iY - 5);
}

void CBuddycloudSetupContainer::ConfigureEdwinsL() {
	// Username
	iUsernameEdwin = new (ELeave) CEikEdwin();
	iUsernameEdwin->ConstructL(EAknEditorFlagDefault, 0, 64, 1);
	iUsernameEdwin->SetAknEditorCase(EAknEditorLowerCase);
	iUsernameEdwin->SetAknEditorPermittedCaseModes(EAknEditorLowerCase);
	iUsernameEdwin->SetAknEditorAllowedInputModes(EAknEditorTextInputMode);
	iUsernameEdwin->SetAknEditorCurrentInputMode(EAknEditorTextInputMode);
	iUsernameEdwin->SetAknEditorLocalLanguage(ELangEnglish);
	iUsernameEdwin->SetOnlyASCIIChars(true);
	iUsernameEdwin->SetContainerWindowL(*this);
	iUsernameEdwin->MakeVisible(false);
	iUsernameEdwin->SetFocus(false);
	
#ifdef __SERIES60_40__
	TRect aFieldRect = iUsernameEdwin->Rect();
	aFieldRect.SetHeight(aFieldRect.Height() + (aFieldRect.Height() / 2));
	
	iUsernameEdwin->SetRect(aFieldRect);
#endif

	// Password
	iPasswordEdwin = new (ELeave) CEikSecretEditor();

	TResourceReader aReader;
	iCoeEnv->CreateResourceReaderLC(aReader, R_PASSWORD_EDWIN);
	iPasswordEdwin->ConstructFromResourceL(aReader);
	CleanupStack::PopAndDestroy();	
	
	iPasswordEdwin->SetBorder(TGulBorder::ENone);
	iPasswordEdwin->SetContainerWindowL(*this);
	iPasswordEdwin->MakeVisible(false);
	iPasswordEdwin->SetFocus(false);
}

void CBuddycloudSetupContainer::RepositionEdwins() {
	if(iUsernameEdwin && iPasswordEdwin) {
		// Password
		TRect aFieldRect = iUsernameEdwin->Rect();
		TInt aFontHeight = iSymbolFont->FontMaxHeight();
				
		aFieldRect.SetRect(aFontHeight, (iRect.Height() - aFieldRect.Height() - aFontHeight - 2), (iRect.Width() - aFontHeight), (iRect.Height() - aFontHeight - 2));		
#ifdef __SERIES60_40__
		aFieldRect.iBr.iX = (iRect.Width() - iSymbolFont->TextWidthInPixels(*iNextText) - (aFontHeight * 2) - 4);
#endif
		
		iPasswordEdwin->SetRect(aFieldRect);
		
		//Username
		aFieldRect.iBr.iX = (iRect.Width() - aFontHeight);
		aFieldRect.iBr.iY = aFieldRect.iTl.iY - 5;
		aFieldRect.iTl.iY = aFieldRect.iBr.iY - iPasswordEdwin->Rect().Height();
		
		if(iPasswordEdwin->IsFocused()) {
			aFieldRect.iBr.iY = aFieldRect.iBr.iY - 4 - iNormalFont->HeightInPixels();
			aFieldRect.iTl.iY = aFieldRect.iTl.iY - 4 - iNormalFont->HeightInPixels();
		}
		
		iUsernameEdwin->SetRect(aFieldRect);
	}	
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
	}
	else if(iSetupStep == EStep3) {	
		iSetupStep = EStep4;		
		ConstructTextL();
		RepositionEdwins();
		
		iUsernameEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
		iUsernameEdwin->SelectAllL();
		iUsernameEdwin->MakeVisible(true);
		iUsernameEdwin->SetFocus(true);
		
		iPasswordEdwin->SetText(iBuddycloudLogic->GetDescSetting(ESettingItemPassword));
		iPasswordEdwin->MakeVisible(true);
	}
	else if(iSetupStep == EStep4) {
		iUsernameEdwin->GetText(iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
		iPasswordEdwin->GetText(iBuddycloudLogic->GetDescSetting(ESettingItemPassword));
		
		iBuddycloudLogic->ValidateUsername(iNewRegistration);
		
		iUsernameEdwin->SetTextL(&iBuddycloudLogic->GetDescSetting(ESettingItemUsername));
		
		TPtrC aUsername(iBuddycloudLogic->GetDescSetting(ESettingItemUsername));	
		
		if(aUsername.Length() == 0 || 
				(!iNewRegistration && (aUsername.Locate('@') == KErrNotFound || aUsername.Locate('.') == KErrNotFound))) {		
			
			iUsernameEdwin->SetFocus(true);
			iPasswordEdwin->SetFocus(false);
			
			RepositionEdwins();
		}
		else if(iBuddycloudLogic->GetDescSetting(ESettingItemPassword).Length() == 0) {		
			iPasswordEdwin->SetFocus(true);
			iUsernameEdwin->SetFocus(false);
			
			RepositionEdwins();
		}
		else {
			iSetupStep = EStep5;
			ConstructTextL();
			
			iUsernameEdwin->MakeVisible(false);
			iUsernameEdwin->SetFocus(false);
			
			iPasswordEdwin->MakeVisible(false);
			iPasswordEdwin->SetFocus(false);
		}
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
		
		if(!iNewRegistration) {
			WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP4_TEXT1, iRect.Width() - 6);
		}
		else {
			WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP4_TEXT2, iRect.Width() - 6);
		}
		
		if(iLabelText1)
			delete iLabelText1;
		if(iLabelText2)
			delete iLabelText2;

		iLabelText1 = iCoeEnv->AllocReadResourceL(R_LOCALIZED_STRING_STEP4_LABEL1);
		iLabelText2 = iCoeEnv->AllocReadResourceL(R_LOCALIZED_STRING_STEP4_LABEL2);
	}
	else if(iSetupStep == EStep5) {
		WordWrapL(iTitleArray, i12BoldFont, R_LOCALIZED_STRING_STEP5_TITLE, iRect.Width() - 6);
		WordWrapL(iTextArray, iNormalFont, R_LOCALIZED_STRING_STEP5_TEXT, iRect.Width() - 6);
		WordWrapL(iQuestionArray, iNormalFont, R_LOCALIZED_STRING_STEP5_QUESTION, iRect.Width() - i12BoldFont->FontMaxHeight() - 6);
	}
}

void CBuddycloudSetupContainer::WordWrapL(RPointerArray<HBufC>& aArray, CFont* aFont, TInt aTextResource, TInt aWidth) {
	HBufC* aUnformattedText = iCoeEnv->AllocReadResourceLC(aTextResource);
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
		
		if(iSetupStep == EStep3) {
			// Button 1
			iBufferGc->SetBrushColor(!iNewRegistration ? KRgbGreen : KRgbGray);
			iBufferGc->SetPenColor(!iNewRegistration ? KRgbDarkGreen : KRgbDarkGray);
			iBufferGc->DrawRoundRect(iLabelRect1, TSize(3, 3));	
			
			iBufferGc->SetPenColor(!iNewRegistration ? KRgbBlack : KRgbDarkGray);
			iBufferGc->DrawText(*iLabelText1, TPoint((iRect.Width() / 2) - (iNormalFont->TextWidthInPixels(*iLabelText1) / 2), iLabelRect1.iTl.iY + (iLabelRect1.Height() / 2) + iNormalFont->DescentInPixels()));
			
			// Button 2
			iBufferGc->SetBrushColor(iNewRegistration ? KRgbGreen : KRgbGray);
			iBufferGc->SetPenColor(iNewRegistration ? KRgbDarkGreen : KRgbDarkGray);
			iBufferGc->DrawRoundRect(iLabelRect2, TSize(3, 3));
			
			iBufferGc->SetPenColor(iNewRegistration ? KRgbBlack : KRgbDarkGray);
			iBufferGc->DrawText(*iLabelText2, TPoint((iRect.Width() / 2) - (iNormalFont->TextWidthInPixels(*iLabelText2) / 2), iLabelRect2.iTl.iY + (iLabelRect2.Height() / 2) + iNormalFont->DescentInPixels()));
		}
		else if(iSetupStep == EStep4) {
			// Edwin
			if(iUsernameEdwin->IsVisible()) {
				TRect aFieldRect = iUsernameEdwin->Rect();
				aFieldRect.Grow(2, 2);
				
				if(iUsernameEdwin->IsFocused()) {
					iBufferGc->SetPenColor(KRgbBlack);
					iBufferGc->DrawText(*iLabelText1, TPoint(aFieldRect.iTl.iX + 10, aFieldRect.iTl.iY - 5));
					
					iBufferGc->SetBrushColor(KRgbGreen);
					iBufferGc->SetPenColor(KRgbGreen);
				}
				else {
					iBufferGc->SetBrushColor(KRgbGray);
					iBufferGc->SetPenColor(KRgbGray);
				}
				
				iBufferGc->DrawRoundRect(aFieldRect, TSize(2, 2));
			}
			
			if(iPasswordEdwin->IsVisible()) {
				TRect aFieldRect = iPasswordEdwin->Rect();				
#ifdef __SERIES60_40__
				aFieldRect.SetHeight(aFieldRect.Height() + 2);
#else
				aFieldRect.Grow(2, 2);
#endif
				
				if(iPasswordEdwin->IsFocused()) {
					iBufferGc->SetPenColor(KRgbBlack);
					iBufferGc->DrawText(*iLabelText2, TPoint(aFieldRect.iTl.iX + 10, aFieldRect.iTl.iY - 5));
					
					iBufferGc->SetBrushColor(KRgbGreen);
					iBufferGc->SetPenColor(KRgbGreen);
				}
				else {
					iBufferGc->SetBrushColor(KRgbGray);
					iBufferGc->SetPenColor(KRgbGray);
				}
				
				iBufferGc->DrawRoundRect(aFieldRect, TSize(2, 2));
			}
		}
		else if(iSetupStep == EStep5) {
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
	
	if(iSetupStep == EStep2 || iSetupStep == EStep5 || 
			(iSetupStep == EStep4 && (iUsernameEdwin->TextLength() > 0 || iPasswordEdwin->Buffer().Length() > 0))) {
		
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
	
	// Fonts
	ReleaseFonts();
	ConfigureFonts();
	
	// Images & text
	ResizeCachedImagesL();
	ConstructTextL();
	
	RepositionButtons();
	
	// Edwin
	iPasswordEdwin->AknSetFont(*iNormalFont);

	RepositionEdwins();
		
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
	return 2;
}

CCoeControl* CBuddycloudSetupContainer::ComponentControl(TInt aIndex) const {	
	if(aIndex == 0) {
		return iUsernameEdwin;
	}
	else if(aIndex == 1) {
		return iPasswordEdwin;
	}
	
	return NULL;
}

TKeyResponse CBuddycloudSetupContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;
	
	if(aType == EEventKey) {
		if(iSetupStep == EStep3 && (aKeyEvent.iCode == EKeyUpArrow || aKeyEvent.iCode == EKeyDownArrow)) {
			iNewRegistration = !iNewRegistration;
			
			aResult = EKeyWasConsumed;
		}
		else if(iSetupStep == EStep4 && (aKeyEvent.iCode == EKeyUpArrow || aKeyEvent.iCode == EKeyDownArrow)) {
			if(iUsernameEdwin->IsFocused()) {
				iUsernameEdwin->SetFocus(false);
				iPasswordEdwin->SetFocus(true);
			}
			else {
				iPasswordEdwin->SetFocus(false);
				iUsernameEdwin->SetFocus(true);
			}
			
			RepositionEdwins();
			
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode >= 63554 && aKeyEvent.iCode <= 63557) {
			if(iSetupStep < EStep5) {
				ConstructNextStepL();
			}
			else if(iSetupStep == EStep5) {
				if(aKeyEvent.iCode == 63557) {
					TBool& aAutoStart = iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart);
					aAutoStart = !aAutoStart;
				}
				else {
					iBuddycloudLogic->ConnectL();
					ReportEventL(MCoeControlObserver::EEventRequestExit);
				}				
			}
			
			aResult = EKeyWasConsumed;
		}
	}
	
	if(aResult == EKeyWasNotConsumed && iSetupStep == EStep4) {
		if(iUsernameEdwin->IsFocused()) {
			aResult = iUsernameEdwin->OfferKeyEventL(aKeyEvent, aType);
		}
		else if(iPasswordEdwin->IsFocused()) {
			aResult = iPasswordEdwin->OfferKeyEventL(aKeyEvent, aType);
		}
	}
	
	RenderScreen();
	
	return aResult;
}

#ifdef __SERIES60_40__
void CBuddycloudSetupContainer::HandlePointerEventL(const TPointerEvent &aPointerEvent) {
	CCoeControl::HandlePointerEventL(aPointerEvent);	
	
	// Handle touch events
	if(aPointerEvent.iType == TPointerEvent::EButton1Down || aPointerEvent.iType == TPointerEvent::EDrag) {
		if(iSetupStep == EStep3) {
			if(iLabelRect1.Contains(aPointerEvent.iPosition)) {
				iNewRegistration = false;
			}
			else if(iLabelRect2.Contains(aPointerEvent.iPosition)) {
				iNewRegistration = true;
			}
		}
	}
	else if(aPointerEvent.iType == TPointerEvent::EButton1Up) {
		TBool aFeedback = false;

		if(iSetupStep <= EStep2) {
			ConstructNextStepL();
			
			aFeedback = true;
		}
		else if(iSetupStep == EStep3) {
			if(iLabelRect1.Contains(aPointerEvent.iPosition) || iLabelRect2.Contains(aPointerEvent.iPosition)) {
				
				// Move on to next step
				iNewRegistration = iLabelRect2.Contains(aPointerEvent.iPosition);
				ConstructNextStepL();
				
				aFeedback = true;
			}
		}
		else if(iSetupStep == EStep4) {
			if(iPasswordEdwin->Rect().Contains(aPointerEvent.iPosition) || iUsernameEdwin->Rect().Contains(aPointerEvent.iPosition)) {
				iPasswordEdwin->SetFocus(iPasswordEdwin->Rect().Contains(aPointerEvent.iPosition));
				iUsernameEdwin->SetFocus(iUsernameEdwin->Rect().Contains(aPointerEvent.iPosition));
				
				RepositionEdwins();
				
				aFeedback = true;
			}
			else if(iNextButton.Contains(aPointerEvent.iPosition)) {
				ConstructNextStepL();
				
				aFeedback = true;
			}
		}
		else if(iSetupStep == EStep5) {
			if(iCheckbox.Contains(aPointerEvent.iPosition)) {
				TBool& aAutoStart = iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart);
				
				aAutoStart = !aAutoStart;
				aFeedback = true;
			}
			else if(iNextButton.Contains(aPointerEvent.iPosition)) {
				iBuddycloudLogic->ConnectL();
				ReportEventL(MCoeControlObserver::EEventRequestExit);
			}
		}
		
		if(aFeedback) {
			// Provide feedback
			iTouchFeedback->InstantFeedback(ETouchFeedbackBasic);
		}
	}
	
	RenderScreen();
}
#endif

// End of File  
