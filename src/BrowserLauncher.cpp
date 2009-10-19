/*
============================================================================
 Name        : 	BrowserLauncher.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description :  Get the platform version & launch web browser
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#include <e32std.h>
#include <eikenv.h>
#include <f32file.h>
#include <apgtask.h>
#include <apgcli.h>
#include "BrowserLauncher.h"

CBrowserLauncher* CBrowserLauncher::NewL() {
	CBrowserLauncher* self = NewLC();
	CleanupStack::Pop(self);
	return self;

}

CBrowserLauncher* CBrowserLauncher::NewLC() {
	CBrowserLauncher* self = new (ELeave) CBrowserLauncher();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CBrowserLauncher::~CBrowserLauncher() {
}

CBrowserLauncher::CBrowserLauncher() {
}

void CBrowserLauncher::ConstructL() {
}

void CBrowserLauncher::GetPlatformVersionL(TUint& aMajor, TUint& aMinor) {
	_LIT(KS60ProductIDFile, "Series60v*.sis");
	_LIT(KROMInstallDir, "z:\\system\\install\\");

	RFs aSession;

	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);

		TFindFile aFindFile(aSession);
		CDir* aResult;

		if(aFindFile.FindWildByDir(KS60ProductIDFile, KROMInstallDir, aResult) == KErrNone) {
			CleanupStack::PushL(aResult);

			if(aResult->Sort(ESortByName|EDescending) == KErrNone) {
				aMajor = (*aResult)[0].iName[9] - '0';
				aMinor = (*aResult)[0].iName[11] - '0';
			}

			CleanupStack::PopAndDestroy(); // result
		}

		CleanupStack::PopAndDestroy(&aSession);
	}
}

void CBrowserLauncher::LaunchBrowserWithLinkL(const TDesC& aLink) {
	TUint aMajor = 0, aMinor = 0;
	TUid aBrowserAppUid = {0x10008D39};

	// Check platform version
	GetPlatformVersionL(aMajor, aMinor);

	if(aMajor < 3 || aMajor == 3 && aMinor == 0) {
		aBrowserAppUid = TUid::Uid(0x1020724D);
	}
	
	// Create custom message
	HBufC* aMessage = HBufC::NewLC(2 + aLink.Length());
	TPtr pMessage(aMessage->Des());
	pMessage.Append(_L("4 "));
	pMessage.Append(aLink);

	// Find task
	TApaTaskList aTaskList(CEikonEnv::Static()->WsSession());
	TApaTask aTask = aTaskList.FindApp(aBrowserAppUid);

	if(aTask.Exists()) {
		aTask.BringToForeground();
		HBufC8* aMessage8 = HBufC8::NewLC(pMessage.Length());
		TPtr8 pMessage8(aMessage8->Des());
		pMessage8.Append(pMessage);

		aTask.SendMessage(TUid::Uid(0), pMessage8);
		CleanupStack::PopAndDestroy(); // aMessage8
	}
	else {
		RApaLsSession aApaLsSession;
		if(aApaLsSession.Connect() == KErrNone) {
			TThreadId aThread;
			aApaLsSession.StartDocument(pMessage, aBrowserAppUid, aThread);
			aApaLsSession.Close();
		}
	}

	CleanupStack::PopAndDestroy(); // aMessage
}
