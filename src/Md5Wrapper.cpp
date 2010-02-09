/*
============================================================================
 Name        : 	Md5Wrapper.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2010
 Description : 	Wrapper class for CMD5
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "Md5Wrapper.h"

CMD5Wrapper* CMD5Wrapper::NewLC() {
	CMD5Wrapper* self = new (ELeave) CMD5Wrapper();
	CleanupStack::PushL(self);
	self->ConstructL();
	
	return self;
}

CMD5Wrapper::~CMD5Wrapper() {
	if(iMd5) {
		delete iMd5;
	}
	
	if(iHex) {
		delete iHex;
	}
}

void CMD5Wrapper::ConstructL() {
	iMd5 = CMD5::NewL();
}

void CMD5Wrapper::Update(const TDesC8& aMessage) {
	iMd5->Update(aMessage);
}

TPtrC8 CMD5Wrapper::Final() {
	return iMd5->Final();
}

TPtrC8 CMD5Wrapper::FinalHex(const TDesC8& aMessage) {
	iMd5->Update(aMessage);
	
	return FinalHex();
}

TPtrC8 CMD5Wrapper::FinalHex() {
	if(iHex) {
		delete iHex;
	}
	
	TPtrC8 aFinal = iMd5->Final();
	
	iHex = HBufC8::NewL(aFinal.Length() * 2);
	TPtr8 pHex(iHex->Des());
	
	for(TInt i = 0; i < aFinal.Length(); i++) {
		pHex.AppendFormat(_L8("%02X"), aFinal[i]);
	}
	
	pHex.LowerCase();

	return pHex;
}
