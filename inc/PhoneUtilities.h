/*
============================================================================
 Name        : 	PhoneUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Get Phone Model and Firmware versions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef PHONEUTILITIES_H_
#define PHONEUTILITIES_H_

#include <e32base.h>

class CPhoneUtilities {		
	public:
		static CPhoneUtilities* NewL();
		static CPhoneUtilities* NewLC();

	public:
		void GetPhoneModelL(TDes& aPhoneModel);
		void GetFirmwareVersionL(TDes& aFirmwareVersion);
		
	public:
		TBool InCall();
};

#endif /*PHONEUTILITIES_H_*/
