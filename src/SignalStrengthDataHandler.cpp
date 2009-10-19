/*
============================================================================
 Name        : 	SignalStrengthDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Handle data for Signal Strength Information
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "SignalStrengthDataHandler.h"

CSignalStrengthDataHandler::CSignalStrengthDataHandler(MSignalStrengthNotification* aObserver) {
	iObserver = aObserver;
}

CSignalStrengthDataHandler* CSignalStrengthDataHandler::NewL(MSignalStrengthNotification* aObserver) {
	CSignalStrengthDataHandler* self = new (ELeave) CSignalStrengthDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CSignalStrengthDataHandler::~CSignalStrengthDataHandler() {
//	Stop();

#ifndef __SERIES60_3X__
	iMobileNetworkInfo.Close();
	iServer.Close();
#else
	if(iTelephony)
		delete iTelephony;
#endif
}

void CSignalStrengthDataHandler::ConstructL() {
#ifndef __SERIES60_3X__
	RTelServer::TPhoneInfo info;
 	User::LeaveIfError(iServer.Connect());

	User::LeaveIfError(iServer.LoadPhoneModule(_L("phonetsy.tsy")));
 	User::LeaveIfError(iServer.GetPhoneInfo(0, info));

	User::LeaveIfError(iMobileNetworkInfo.Open(iServer, info.iName));
#else
	iTelephony = CTelephony::NewL();
#endif

	CActiveScheduler::Add(this);
}

void CSignalStrengthDataHandler::StartL() {
	if(!IsActive() && iEngineStatus == ESignalStrengthDisconnected) {
#ifdef __WINSCW__
		iObserver->SignalStrengthData(99, 7);
#else
		iEngineStatus = ESignalStrengthReading;

#ifndef __SERIES60_3X__
		iMobileNetworkInfo.GetSignalStrength(iStatus, iSignalStrength, iSignalBars);
#else
		CTelephony::TSignalStrengthV1Pckg aSignalStrengthV1Pckg(iSignalStrengthData);
		iTelephony->GetSignalStrength(iStatus, aSignalStrengthV1Pckg);
#endif

		SetActive();
#endif
	}
}

TBool CSignalStrengthDataHandler::IsConnected() {
	return (iEngineStatus != ESignalStrengthDisconnected);
}

void CSignalStrengthDataHandler::Stop() {
	DoCancel();
}

void CSignalStrengthDataHandler::RunL() {
	switch(iEngineStatus) {
		case ESignalStrengthReading:
			iEngineStatus = ESignalStrengthDisconnected;

			if(iStatus == KErrNone) {
#ifndef __SERIES60_3X__
				iObserver->SignalStrengthData(iSignalStrength, iSignalBars);
#else
				iObserver->SignalStrengthData(iSignalStrengthData.iSignalStrength, iSignalStrengthData.iBar);
#endif
			}
			else {
				iObserver->SignalStrengthError(ESignalStrengthReadError);
			}
			break;
		case ESignalStrengthDisconnecting:
			iEngineStatus = ESignalStrengthDisconnected;
			break;
		default:;
	}
}

TInt CSignalStrengthDataHandler::RunError(TInt /*aError*/) {
	return KErrNone;
}

void CSignalStrengthDataHandler::DoCancel() {
	switch(iEngineStatus) {
		case ESignalStrengthReading:
#ifndef __SERIES60_3X__
			iEngineStatus = ESignalStrengthDisconnected;
#else
			iEngineStatus = ESignalStrengthDisconnecting;
			iTelephony->CancelAsync(CTelephony::EGetSignalStrengthCancel);
#endif
			break;
		default:;
	}
}

// End of File
