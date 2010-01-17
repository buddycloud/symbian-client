/*
============================================================================
 Name        : 	TextUtilities.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Text Utilities for wrapping & characher coding
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <bidi.h>
#include <biditext.h>
#include <e32std.h>
#include <imcvcodc.h>
#include <utf.h>
#include "TextUtilities.h"

CTextUtilities* CTextUtilities::NewL() {
	CTextUtilities* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CTextUtilities* CTextUtilities::NewLC() {
	CTextUtilities* self = new (ELeave) CTextUtilities();
	CleanupStack::PushL(self);
	return self;
}

CTextUtilities::~CTextUtilities() {
	if(iUtf8)
		delete iUtf8;	
	
	if(iUnicode)
		delete iUnicode;
	
	if(iBidi)
		delete iBidi;
}

void CTextUtilities::WrapToArrayL(RPointerArray<HBufC>& aArray, CFont* aFont, const TDesC& aText, TInt aWidth) {
	TPtrC pText(aText);

	if(pText.Length() > 0) {
		while(pText.Length() > 0) {
			TInt aDisplayWidth, aCharLocation, aBurn = 1;	
			
			// Find new line
			if((aCharLocation = pText.Locate('\n')) == KErrNotFound) {
				aCharLocation = pText.Length();
				aBurn = 0;
			}
	
			// Copy
			TPtrC pTextParagraph(pText.Left(aCharLocation));
			pText.Set(pText.Mid(aCharLocation + aBurn));
	
			// Word wrap
			while((aDisplayWidth = aFont->TextCount(pTextParagraph, aWidth)) < pTextParagraph.Length()) {
				TPtrC pTextLine(pTextParagraph.Left(aDisplayWidth));
	
				if((aCharLocation = pTextLine.LocateReverse(' ')) != KErrNotFound) {
					pTextLine.Set(pTextLine.Left(aCharLocation));
					pTextParagraph.Set(pTextParagraph.Mid(aCharLocation + 1));
				}
				else {
					pTextParagraph.Set(pTextParagraph.Mid(aDisplayWidth));
				}
	
				aArray.Append(BidiLogicalToVisualL(pTextLine).AllocL());
			}
	
			aArray.Append(BidiLogicalToVisualL(pTextParagraph).AllocL());
		}
	}
	else {
		aArray.Append(BidiLogicalToVisualL(pText).AllocL());
	}
}

void CTextUtilities::AppendToString(TDes& aString, TDesC& aAppend, const TDesC& aSeperator) {
	if((aString.MaxLength() - aString.Length()) >= (aAppend.Length() + aSeperator.Length())) {
		if(aString.Length() > 0 && aAppend.Length() > 0) {
			aString.Append(aSeperator);
		}
		
		aString.Append(aAppend);
	}
}

TDesC8& CTextUtilities::UnicodeToUtf8L(const TDesC& aUnicode) {
	if(iUtf8)
		delete iUtf8;

	iUtf8 = HBufC8::NewL(aUnicode.Length());
	
	if(aUnicode.Length() > 0) {
		TPtr8 pUtf8(iUtf8->Des());
		
		// First convert to UTF8
		TBuf8<32> aConvertedBuf;
		TPtrC pRemainingUnicode(aUnicode);
		
		while(true) {
			TInt aResult = CnvUtfConverter::ConvertFromUnicodeToUtf8(aConvertedBuf, pRemainingUnicode);
			
			if(aResult < 0)
				User::Leave(KErrGeneral);
			
			if(pUtf8.Length() + aConvertedBuf.Length() > pUtf8.MaxLength()) {
				iUtf8 = iUtf8->ReAlloc(pUtf8.Length()+aConvertedBuf.Length());
				pUtf8.Set(iUtf8->Des());
			}
			
			pUtf8.Append(aConvertedBuf);
			
			if(aResult == 0)
				break;
			
			pRemainingUnicode.Set(pRemainingUnicode.Right(aResult));
		}
		
		// Second convert UTF8 to HTML
		FindAndReplace(_L8("&"), _L8("&amp;"));
		FindAndReplace(_L8("'"), _L8("&apos;"));
		FindAndReplace(_L8("\""), _L8("&quot;"));
		FindAndReplace(_L8("<"), _L8("&lt;"));
		FindAndReplace(_L8(">"), _L8("&gt;"));
//		FindAndReplace(_L8("\n"), _L8("&#10;"));
//		FindAndReplace(_L8("\r"), _L8("&#13;"));
//		FindAndReplace(_L8("\x01"), _L8("&#10;"));
	}
	
	return *iUtf8;
}

TDesC& CTextUtilities::Utf8ToUnicodeL(const TDesC8& aUtf8) {
	if(iUtf8)
		delete iUtf8;

	if(iUnicode)
		delete iUnicode;

	iUtf8 = aUtf8.AllocL();
	iUnicode = HBufC::NewL(aUtf8.Length());
	
	if(aUtf8.Length() > 0) {
		TPtr pUnicode(iUnicode->Des());
		
		// First convert HTML to UTF8
//		FindAndReplace(_L8("&#13;"), _L8("\r"));
//		FindAndReplace(_L8("&#10;"), _L8("\n"));
		FindAndReplace(_L8("&gt;"), _L8(">"));
		FindAndReplace( _L8("&lt;"), _L8("<"));
		FindAndReplace(_L8("&quot;"), _L8("\""));
		FindAndReplace(_L8("&apos;"), _L8("'"));
		FindAndReplace(_L8("&amp;"), _L8("&"));
		
		// Second convert UTF8 to Unicode
		TBuf<32> aConvertedBuf;
		TPtrC8 pRemainingUtf8(iUtf8->Des());
		
		while(true) {
			TInt aResult = CnvUtfConverter::ConvertToUnicodeFromUtf8(aConvertedBuf, pRemainingUtf8);
			
			if(aResult < 0) {
				User::Leave(KErrGeneral);
			}
			
			if(pUnicode.Length() + aConvertedBuf.Length() > pUnicode.MaxLength()) {
				iUnicode = iUnicode->ReAlloc(pUnicode.Length()+aConvertedBuf.Length());
				pUnicode.Set(iUnicode->Des());
			}
			
			pUnicode.Append(aConvertedBuf);
			
			if(!aResult) {
				break;
			}
			
			pRemainingUtf8.Set(pRemainingUtf8.Right(aResult));
		}
	}
	
	return *iUnicode;
}

void CTextUtilities::Base64Encode(const TDesC8& aSrc, TDes8& aDest) {
	TImCodecB64 aBase64Encoder;
	
	aBase64Encoder.Initialise();
	aBase64Encoder.Encode(aSrc, aDest);
}

TPtrC CTextUtilities::BidiLogicalToVisualL(const TDesC& aText) {
	if(iBidi) {
		delete iBidi;
		iBidi = NULL;
	}

	TPtrC aResult(aText);	
	
	if(TBidiText::TextDirectionality(aText) == TBidiText::ERightToLeft) {
		TInt aTextLength = aText.Length();
		TText* aVisualText = 0;	
		TBidirectionalState aBidiState;
		
		if(aBidiState.ReorderText(aText.Ptr(), aText.Length(), true, aVisualText) == KErrNone && aVisualText) {
			if(aVisualText != aText.Ptr()) {
				// Reordering success	
				TPtr pVisualText(aVisualText, aTextLength, aTextLength);
				
				iBidi = HBufC::NewL(aTextLength);
				TPtr pBidi(iBidi->Des());
				pBidi.Copy(pVisualText);
				
				aResult.Set(iBidi->Des());
				
				delete[] aVisualText;
			}
		}
	}
	
	return aResult;
}

void CTextUtilities::FindAndReplace(TPtrC8 aFind, TPtrC8 aReplace) {
	TPtr8 pUtf8(iUtf8->Des());
	TInt aOldResult = 0, aFindResult;

	while((aFindResult = pUtf8.Mid(aOldResult).Find(aFind)) != KErrNotFound) {
		aOldResult += aFindResult;

		if((pUtf8.Length() - aFind.Length() + aReplace.Length()) > pUtf8.MaxLength()) {
			iUtf8 = iUtf8->ReAlloc(pUtf8.Length() - aFind.Length() + aReplace.Length());
			pUtf8.Set(iUtf8->Des());
		}

		pUtf8.Replace(aOldResult, aFind.Length(), aReplace);
		aOldResult += aReplace.Length();
	}
}

void CTextUtilities::FindAndReplace(TDes& aString, TChar aFind, TChar aReplace) {
	TInt aLocate = KErrNotFound;

	while((aLocate = aString.Locate(aFind)) != KErrNotFound) {
		aString[aLocate] = aReplace;
	}
}

