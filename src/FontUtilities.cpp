/*
============================================================================
 Name        : 	FontUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Get Font based on system fonts
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <aknutils.h>
#include <w32std.h>
#include "FontUtilities.h"

CFont* CFontUtilities::GetCustomSystemFont(TInt aFontId, TBool aBold, TBool aItalic) {
	CFont* aReturnedFont = NULL;
	
	// Get logical system font by Id
	const CFont* aLogicalFont = AknLayoutUtils::FontFromId(aFontId);
	
	if(aLogicalFont) {
		// Set new font spec
		TFontSpec aFontSpec = aLogicalFont->FontSpecInTwips();
		aFontSpec.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
		aFontSpec.iFontStyle.SetStrokeWeight( aBold ? EStrokeWeightBold : EStrokeWeightNormal );
		aFontSpec.iFontStyle.SetPosture( aItalic ? EPostureItalic : EPostureUpright );

		// Obtain new font
		CWsScreenDevice *aDeviceMap = CEikonEnv::Static()->ScreenDevice();
		aDeviceMap->GetNearestFontInTwips(aReturnedFont, aFontSpec);
	}

	return aReturnedFont;
}

void CFontUtilities::ReleaseFont(CFont* aFont) {
	// Release font
	CWsScreenDevice *aDeviceMap = CEikonEnv::Static()->ScreenDevice();
	aDeviceMap->ReleaseFont(aFont);
}
