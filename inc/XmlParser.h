/*
============================================================================
 Name        : 	XmlParser.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	XML Stream & Stanza Parser customized for XMPP
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

// INCLUDES
#include <e32base.h>

class CXmlParser : public CBase {
	public:
		static CXmlParser* NewL(const TDesC8& aXml);
		static CXmlParser* NewLC(const TDesC8& aXml);

	private:
		CXmlParser(const TDesC8& aXml);

	public: // Setting & Search
		void ResetElementPointer();

	private:
		TInt Find(const TDesC8& aString, TInt aStart);
		TInt Locate(TChar aChar, TInt aStart);
		TInt Locate(TChar aChar);
		
	private:
		TBool EvaluateAsBool(const TDesC8& aText);
		TInt EvaluateAsInt(const TDesC8& aText, TInt aDefault);

	public: // Element traversing
		TBool MoveToElement(const TDesC8& aElement);
		TBool MoveToElement(const TDesC8& aElement, TInt aNumber);
		TBool MoveToNextElement();
		TBool MoveToPreviousElement();

	public: // Data retrieval
		TPtrC8 GetElementName();
		
		TPtrC8 GetStringAttribute(const TDesC8& aAttribute);
		TBool GetBoolAttribute(const TDesC8& aAttribute);
		TInt GetIntAttribute(const TDesC8& aAttribute, TInt aDefault = 0);
		
		TPtrC8 GetStringData();
		TBool GetBoolData();
		TInt GetIntData(TInt aDefault = 0);

	private:
		TPtrC8 pXml;

		TInt iCurrentElementLocation;
};

#endif // XMLPARSER_H_*/
