/*
============================================================================
 Name        : 	PhoneUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Get Phone Model and Firmware versions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef PHONEUTILITIES_H_
#define PHONEUTILITIES_H_

class CPhoneUtilities {		
	public:
		static void GetPhoneModelL(TDes& aPhoneModel);
		static void GetFirmwareVersionL(TDes& aFirmwareVersion);
};

#endif /*PHONEUTILITIES_H_*/
