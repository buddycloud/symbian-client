/*
============================================================================
 Name        : 	XmlParser.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	XML Stream & Stanza Parser customized for XMPP
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "XmlParser.h"

CXmlParser* CXmlParser::NewL(const TDesC8& aXml) {
	CXmlParser* self = NewLC(aXml);
	CleanupStack::Pop(self);
	return self;
}

CXmlParser* CXmlParser::NewLC(const TDesC8& aXml) {
	CXmlParser* self = new (ELeave) CXmlParser(aXml);
	CleanupStack::PushL(self);
	return self;
}

CXmlParser::CXmlParser(const TDesC8& aXml) : pXml(aXml) {
	ResetElementPointer();
}

TInt CXmlParser::Find(const TDesC8& aString, TInt aStart) {
	TInt aResult = pXml.Mid(aStart).Find(aString);
	if(aResult != KErrNotFound) {
		aResult += aStart;
	}
	
	return aResult;
}

TInt CXmlParser::Locate(TChar aChar, TInt aStart) {
	TInt aResult = pXml.Mid(aStart).Locate(aChar);
	if(aResult != KErrNotFound) {
		aResult += aStart;
	}
	
	return aResult;
}

TInt CXmlParser::Locate(TChar aChar) {
	return Locate(aChar, iCurrentElementLocation);
}

TBool CXmlParser::EvaluateAsBool(const TDesC8& aText, TBool aDefault) {
	if(aText.Length() > 0) {
		if(aText.Compare(_L8("1")) == 0 || aText.Compare(_L8("true")) == 0) {
			return true;
		}
		
		return false;
	}
	
	return aDefault;
}

TReal CXmlParser::EvaluateAsReal(const TDesC8& aText, TReal aDefault) {
	TLex8 aLex(aText);
	aLex.Val(aDefault);
	
	return aDefault;
}

TInt CXmlParser::EvaluateAsInt(const TDesC8& aText, TInt aDefault) {
	TLex8 aLex(aText);
	aLex.Val(aDefault);
	
	return aDefault;
}

void CXmlParser::ResetElementPointer() {
	iCurrentElementLocation = 0;
}

TBool CXmlParser::MoveToElement(const TDesC8& aElement) {
	TInt aFoundElement = iCurrentElementLocation;

	if(pXml.Length() > 0) {
		while((aFoundElement = Find(aElement, (aFoundElement + 1))) != KErrNotFound) {
			if(pXml[(aFoundElement - 1)] == TUint('<')) {
				if(pXml[(aFoundElement + aElement.Length())] == TUint('>') ||
						pXml[(aFoundElement + aElement.Length())] == TUint(' ') ||
						pXml[(aFoundElement + aElement.Length())] == TUint('/')) {

					iCurrentElementLocation = aFoundElement;
					return true;
				}
			}
		}
	}
	
	return false;
}

TBool CXmlParser::MoveToElement(const TDesC8& aElement, TInt aNumber) {
	TBool aMoveSuccessful = true;

	for(TInt i = 0; i < aNumber && aMoveSuccessful; i++) {
		aMoveSuccessful = MoveToElement(aElement);
	}

	return aMoveSuccessful;
}

TBool CXmlParser::MoveToNextElement() {
	for(TInt i = (iCurrentElementLocation + 1); i < pXml.Length(); i++) {
		if(pXml[i] == TUint('<') && (i + 1) < pXml.Length()) {
			if(pXml[(i + 1)] != TUint('/')) {
				iCurrentElementLocation = i;
				return true;
			}
		}
	}

	return false;
}

TBool CXmlParser::MoveToPreviousElement() {
	for(TInt i = (iCurrentElementLocation - 1); i >= 0; i--) {
		if(pXml[i] == TUint('<') && (i + 1) < pXml.Length()) {
			if(pXml[(i + 1)] != TUint('/')) {
				iCurrentElementLocation = i;
				return true;
			}
		}
	}

	return false;
}

TPtrC8 CXmlParser::GetElementName() {
	TPtrC8 pElementName(pXml.Mid(iCurrentElementLocation));
	
	// Locate '>'
	TInt aLocateResult = pElementName.Locate('>');
	
	if(aLocateResult != KErrNotFound) {
		pElementName.Set(pElementName.Left(aLocateResult));
	}
	
	// Locate ' '
	if((aLocateResult = pElementName.Locate(' ')) != KErrNotFound) {
		pElementName.Set(pElementName.Left(aLocateResult));
	}
	
	// Locate '/'
	if((aLocateResult = pElementName.Locate('/')) != KErrNotFound) {
		pElementName.Set(pElementName.Left(aLocateResult));
	}
	
	// Locate '<'
	if((aLocateResult = pElementName.Locate('<')) != KErrNotFound) {
		pElementName.Set(pElementName.Mid(aLocateResult + 1));
	}

	return pElementName;
}

TPtrC8 CXmlParser::GetStringAttribute(const TDesC8& aAttribute) {
	TPtrC8 aAttributeData(pXml.Mid(iCurrentElementLocation));
	TInt aSearchResult = aAttributeData.Locate('>');
	
	if(aSearchResult != KErrNotFound) {
		aAttributeData.Set(aAttributeData.Left(aSearchResult));
		
		while((aSearchResult = aAttributeData.Find(aAttribute)) != KErrNotFound) {
			if(aSearchResult > 0 && aAttributeData[aSearchResult - 1] == TUint(' ') && 
					aAttributeData[aSearchResult + aAttribute.Length()] == TUint('=')) {
				
				TChar aDelimiter(aAttributeData[aSearchResult + aAttribute.Length() + 1]);

				// Attribute found
				aAttributeData.Set(aAttributeData.Mid(aSearchResult + aAttribute.Length() + 2));
							
				if((aSearchResult = aAttributeData.Locate(aDelimiter)) != KErrNotFound) {
					// Delimited data found
					return aAttributeData.Left(aSearchResult);
				}
			}
				
			aAttributeData.Set(aAttributeData.Mid(aSearchResult + aAttribute.Length()));
		}		
	}
	
	// Return empty data
	return pXml.Mid(0, 0);
}

TBool CXmlParser::GetBoolAttribute(const TDesC8& aAttribute, TBool aDefault) {
	return EvaluateAsBool(GetStringAttribute(aAttribute), aDefault);
}

TInt CXmlParser::GetIntAttribute(const TDesC8& aAttribute, TInt aDefault) {
	return EvaluateAsInt(GetStringAttribute(aAttribute), aDefault);
}

TPtrC8 CXmlParser::GetStringData() {
	TInt aDataStartMarker = Locate('>', iCurrentElementLocation);
	TInt aDataEndMarker = aDataStartMarker;

	if(aDataStartMarker != KErrNotFound) {
		if(pXml[aDataStartMarker-1] != '/') {
			TPtrC8 pElementName = GetElementName();

			HBufC8* aCloseElement = HBufC8::NewLC(pElementName.Length()+3);
			TPtr8 pCloseElement(aCloseElement->Des());
			pCloseElement.Append(_L8("</>"));
			pCloseElement.Insert(2, pElementName);

			aDataEndMarker = Find(pCloseElement, (aDataEndMarker + 1));
			CleanupStack::PopAndDestroy(); // aCloseElement

			if(aDataEndMarker != KErrNotFound) {
				return pXml.Mid((aDataStartMarker + 1), (aDataEndMarker - aDataStartMarker - 1));
			}
		}
	}

	// Return empty data
	return pXml.Mid(0, 0);
}

TBool CXmlParser::GetBoolData(TBool aDefault) {
	return EvaluateAsBool(GetStringData(), aDefault);
}

TReal CXmlParser::GetRealData(TReal aDefault) {
	return EvaluateAsReal(GetStringData(), aDefault);
}

TInt CXmlParser::GetIntData(TInt aDefault) {
	return EvaluateAsInt(GetStringData(), aDefault);
}

// End of File
