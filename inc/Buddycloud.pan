/*
============================================================================
 Name        : Buddycloud.pan
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
 Description : Buddycloud Panic
============================================================================
*/

#ifndef __BUDDYCLOUD_PAN__
#define __BUDDYCLOUD_PAN__

/** Buddycloud application panic codes */

inline void Panic(TInt aReason) {
	//_LIT(applicationName,"Buddycloud");
	User::Panic(_L("Buddycloud"), aReason);
}

inline void PanicIfError(TInt aPanicCode) {
	if (aPanicCode != KErrNone) {
		Panic(aPanicCode);
	}
}

inline void PanicIfError(const TDesC& aCategory, TInt aPanicCode) {
	if (aPanicCode != KErrNone) {
		User::Panic(aCategory, aPanicCode);
	}	
}

#endif // __BUDDYCLOUD_PAN__
