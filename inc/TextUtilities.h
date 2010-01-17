/*
============================================================================
 Name        : 	TextUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Text Utilities for wrapping & characher coding
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef TEXTUTILITIES_H_
#define TEXTUTILITIES_H_

#include <e32base.h>
#include <e32cmn.h>
#include <gdi.h>

/*
----------------------------------------------------------------------------
--
-- CTextUtilities
--
----------------------------------------------------------------------------
*/

class CTextUtilities : public CBase {
	public:
		static CTextUtilities* NewL();
		static CTextUtilities* NewLC();
		~CTextUtilities();

	public: // Word Wrapping
		void WrapToArrayL(RPointerArray<HBufC>& aArray, CFont* aFont, const TDesC& aText, TInt aWidth);
		
	public: // Append
		void AppendToString(TDes& aString, TDesC& aAppend, const TDesC& aSeperator);
		
	public: // Character coding
		TDesC& Utf8ToUnicodeL(const TDesC8& aUtf8);
		TDesC8& UnicodeToUtf8L(const TDesC& aUnicode);
		
	public: // Base64 encoding
		void Base64Encode(const TDesC8& aSrc, TDes8& aDest);
		
	public: // Bidi coding
		TPtrC BidiLogicalToVisualL(const TDesC& aText);

	private:
		void FindAndReplace(TPtrC8 aFind, TPtrC8 aReplace);

	public:
		void FindAndReplace(TDes& aString, TChar aFind, TChar aReplace);
		
	private:
		HBufC* iUnicode;
		HBufC8* iUtf8;
		
		HBufC* iBidi;
};

#endif /*TEXTUTILITIES_H_*/
