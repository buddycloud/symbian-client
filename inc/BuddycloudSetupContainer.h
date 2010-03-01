/*
============================================================================
 Name        : BuddycloudSetupContainer.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Setup Container
============================================================================
*/

#ifndef BUDDYCLOUDSETUPCONTAINER_H_
#define BUDDYCLOUDSETUPCONTAINER_H_

#include <coecntrl.h>
#include <eikedwin.h>
#include <eikseced.h>
#include <fbs.h>
#include "BuddycloudLogic.h"
#include "Timer.h"

#ifdef __SERIES60_40__
#include <touchfeedback.h>
#endif

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TSetupStep {
	EStep1, EStep2, EStep3, EStep4, EStep5
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudSetupContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudSetupContainer : public CCoeControl, MTimeoutNotification {
	public: // Constructors and destructor
		CBuddycloudSetupContainer(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL();
		~CBuddycloudSetupContainer();

	private:
		void RepositionButtons();
		
		void ConfigureEdwinsL();
		void RepositionEdwins();
		
		void ConfigureFonts();
		void ReleaseFonts();

	private:	
		void ConstructNextStepL();
		void ConstructTextL();
		void WordWrapL(RPointerArray<HBufC>& aArray, CFont* aFont, TInt aTextResource, TInt aWidth);
		void ClearTextArrayL(RPointerArray<HBufC>& aArray);
		void PrecacheImagesL();
		void ResizeCachedImagesL();
		void RenderScreen();
	
	public:
		void TimerExpired(TInt aExpiryId);
		
	private: // From CCoeControl	
		void HandleResourceChange(TInt aType);
		void SizeChanged();
		void Draw(const TRect& aRect) const;
		TInt CountComponentControls() const;
		CCoeControl* ComponentControl(TInt aIndex) const;
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
		
#ifdef __SERIES60_40__
	private:
		void HandlePointerEventL(const TPointerEvent &aPointerEvent);
#endif
        
	private: // Variables
		CBuddycloudLogic* iBuddycloudLogic;
		
		TRect iRect;
		TSetupStep iSetupStep;
		TRgb iColorFade;
		TBool iNewRegistration;
		TBool iEditingError;
		
		// Screen Buffer
		CFbsBitmap* iBufferBitmap;
		CFbsBitmapDevice* iBufferDevice;
		CFbsBitGc* iBufferGc;
		
		// Images
		TSize iLogoSize;
		CFbsBitmap* iLogoBitmap;
		CFbsBitmap* iLogoMask;	
 		
		TSize iSplashSize;
		CFbsBitmap* iSplashBitmap;
		CFbsBitmap* iSplashMask;	
 		
		TSize iTickSize;
		CFbsBitmap* iTickBitmap;
		CFbsBitmap* iTickMask;	
		
		// Fonts
 		CFont* i12BoldFont;
		CFont* iNormalFont;
		const CFont* iSymbolFont;
		
		// Timer
		CCustomTimer* iTimer;
		
		// Text
		HBufC* iNextText;
		
		RPointerArray<HBufC> iTitleArray;
		RPointerArray<HBufC> iTextArray;
		RPointerArray<HBufC> iQuestionArray;
		
		// Label data
		HBufC* iLabelText1;
		TRect iLabelRect1;
		
		HBufC* iLabelText2;		
		TRect iLabelRect2;
		
		CEikEdwin* iUsernameEdwin;
		CEikSecretEditor* iPasswordEdwin;
		
#ifdef __SERIES60_40__
		MTouchFeedback* iTouchFeedback;
		
		TRect iNextButton;
		TRect iCheckbox;
#endif
};

#endif

// End of File
