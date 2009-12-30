/*
============================================================================
 Name        : 	PhoneUtilities.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Get Phone Model and Firmware versions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <s32file.h>
#include <etel3rdparty.h>
#include <f32file.h>
#include "PhoneUtilities.h"

void CPhoneUtilities::GetPhoneModelL(TDes& aPhoneModel) {
	RFs aSession;
	RFile aFile;
	TFileText text;
	
	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);
	
		_LIT(KModelFile, "z:\\resource\\versions\\model.txt");
		
		if(aFile.Open(aSession, KModelFile, EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);

			text.Set(aFile);
			text.Read(aPhoneModel);
			
			CleanupStack::PopAndDestroy(&aFile);	
		}
		
		CleanupStack::PopAndDestroy(&aSession);	
	}
}

void CPhoneUtilities::GetFirmwareVersionL(TDes& aFirmwareVersion) {
	RFs aSession;
	RFile aFile;
	TFileText text;
	
	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);
	
		_LIT(KModelFile, "z:\\resource\\versions\\sw.txt");
		
		if(aFile.Open(aSession, KModelFile, EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);

			text.Set(aFile);
			text.Read(aFirmwareVersion);
			
			CleanupStack::PopAndDestroy(&aFile);	
		}
		
		CleanupStack::PopAndDestroy(&aSession);	
	}
}

TBool CPhoneUtilities::InCall() {
	TBool aResult = false;
	
	CTelephony* aTelephony = CTelephony::NewLC();
	
	CTelephony::TCallStatusV1 aCallStatus;
	CTelephony::TCallStatusV1Pckg aCallStatusV1Pckg(aCallStatus);
	
	if(aTelephony->GetLineStatus(CTelephony::EVoiceLine, aCallStatusV1Pckg) == KErrNone) {
		if(aCallStatus.iStatus > CTelephony::EStatusIdle) {
			aResult = true;
		}
	}
	
	CleanupStack::PopAndDestroy(); // aTelephony
	
	return aResult;
}
