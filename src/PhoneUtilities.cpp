/*
============================================================================
 Name        : 	PhoneUtilities.cpp
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Get Phone Model and Firmware versions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <e32property.h>
#include <f32file.h>
#include <s32file.h>
#include <TelephonyInternalPSKeys.h>
#include "PhoneUtilities.h"

CPhoneUtilities* CPhoneUtilities::NewL() {
	CPhoneUtilities* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CPhoneUtilities* CPhoneUtilities::NewLC() {
	CPhoneUtilities* self = new (ELeave) CPhoneUtilities();
	CleanupStack::PushL(self);
	return self;
}

void CPhoneUtilities::GetPhoneModelL(TDes& aPhoneModel) {
	RFs aSession;
	RFile aFile;
	TFileText aText;
	
	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);
	
		_LIT(KModelFile, "z:\\resource\\versions\\model.txt");
		
		if(aFile.Open(aSession, KModelFile, EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);

			aText.Set(aFile);
			aText.Read(aPhoneModel);
			
			CleanupStack::PopAndDestroy(&aFile);	
		}
		
		CleanupStack::PopAndDestroy(&aSession);	
	}
}

void CPhoneUtilities::GetFirmwareVersionL(TDes& aFirmwareVersion) {
	RFs aSession;
	RFile aFile;
	TFileText aText;
	
	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);
	
		_LIT(KModelFile, "z:\\resource\\versions\\sw.txt");
		
		if(aFile.Open(aSession, KModelFile, EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);

			aText.Set(aFile);
			aText.Read(aFirmwareVersion);
			
			CleanupStack::PopAndDestroy(&aFile);	
		}
		
		CleanupStack::PopAndDestroy(&aSession);	
	}
}

TBool CPhoneUtilities::InCall() {
	TInt aCallStatus = EPSTelephonyCallStateNone;		
	TInt aCallType = EPSTelephonyCallTypeNone;		
	
	RProperty::Get(KPSUidTelephonyCallHandling, KTelephonyCallState, aCallStatus);
	RProperty::Get(KPSUidTelephonyCallHandling, KTelephonyCallType, aCallType);
	
	if(aCallStatus > EPSTelephonyCallStateNone || aCallType > EPSTelephonyCallTypeNone) {		
		return true;
	}
	
	return false;
}
