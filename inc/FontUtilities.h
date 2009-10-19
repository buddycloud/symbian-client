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

#include <gdi.h>

#ifndef FONTUTILITIES_H_
#define FONTUTILITIES_H_

class CFontUtilities {
	public:
		static CFont* GetCustomSystemFont(TInt aFontId, TBool aBold, TBool aItalic);
		static void ReleaseFont(CFont* aFont);
};

#endif /*FONTUTILITIES_H_*/
