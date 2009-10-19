/*
============================================================================
 Name        : 	TimeCoder.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Encode and Decode formatted time
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#include <e32std.h>
#include "TimeCoder.h"

CTimeCoder* CTimeCoder::NewL() {
	CTimeCoder* self = NewLC();
	CleanupStack::Pop(self);
	return self;

}

CTimeCoder* CTimeCoder::NewLC() {
	CTimeCoder* self = new (ELeave) CTimeCoder();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CTimeCoder::~CTimeCoder() {
}

CTimeCoder::CTimeCoder() {
}

void CTimeCoder::ConstructL() {
}

TTime CTimeCoder::DecodeL(const TDesC8& aFormattedTime) {
	TTime aTime;
	aTime.UniversalTime();

	HBufC* aTimeDelay = HBufC::NewLC(aFormattedTime.Length());
	TPtr pTimeDelay(aTimeDelay->Des());
	pTimeDelay.Copy(aFormattedTime);

	TInt aLocateResult = pTimeDelay.Locate('T');
	
	if(aLocateResult != KErrNotFound && pTimeDelay.Length() >= 17) {
		TBuf<8> aDigits;
		TInt aResult;
		// Valid jabber:x:delay
		// Delay Formats: YYYYMMDDTHH:MM:SS.000Z
		//                YYYY-MM-DDTHH:MM:SSZ
		// TTime Format:  YYYYMMDD:HHMMSS.MMMMMM

		// Remove '-'
		while((aLocateResult = pTimeDelay.Locate('-')) != KErrNotFound) {
			pTimeDelay.Delete(aLocateResult, 1);
		}

		// Remove ':'
		while((aLocateResult = pTimeDelay.Locate(':')) != KErrNotFound) {
			pTimeDelay.Delete(aLocateResult, 1);
		}

		// Replace 'T' with ':'
		pTimeDelay.Replace(pTimeDelay.Locate('T'), 1, _L(":"));

		// Remove all after '.'
		if(pTimeDelay.Length() > 15) {
			pTimeDelay.Delete(15, pTimeDelay.Length());
		}

		pTimeDelay.Append(_L("."));

		// Fix Month for conversion
		aDigits.Zero();
		aDigits.Append(TChar(pTimeDelay[4]));
		aDigits.Append(TChar(pTimeDelay[5]));

		TLex aLexMonth(aDigits);
		aLexMonth.Val(aResult);
		aResult--;
		aDigits.Format(_L("%02d"), aResult);
		pTimeDelay.Replace(4, 2, aDigits);

		// Fix Day for conversion
		aDigits.Zero();
		aDigits.Append(TChar(pTimeDelay[6]));
		aDigits.Append(TChar(pTimeDelay[7]));

		TLex aLexDay(aDigits);
		aLexDay.Val(aResult);
		aResult--;
		aDigits.Format(_L("%02d"), aResult);
		pTimeDelay.Replace(6, 2, aDigits);

		aTime.Set(pTimeDelay);
	}

	CleanupStack::PopAndDestroy(); // aTimeDelay

	return aTime;
}

void CTimeCoder::EncodeL(TTime aTime, TFormattedTimeDesc& aFormattedTime) {
	TBuf<32> aFormattedTime16;

	aTime.FormatL(aFormattedTime16, _L("%F%Y-%M-%DT%H:%T:%SZ"));

	if(aFormattedTime.MaxLength() <= aFormattedTime16.Length()) {
		aFormattedTime.Copy(aFormattedTime16);
	}
}
