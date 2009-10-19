/*
============================================================================
 Name        : 	CellTowerDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Handle data for Cell Tower Identification
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "CellTowerDataHandler.h"

CCellTowerDataHandler::CCellTowerDataHandler(MCellTowerNotification* aObserver) {
	iObserver = aObserver;
}

CCellTowerDataHandler* CCellTowerDataHandler::NewL(MCellTowerNotification* aObserver) {
	CCellTowerDataHandler* self = new (ELeave) CCellTowerDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CCellTowerDataHandler::~CCellTowerDataHandler() {
	if(iTimer) {
		delete iTimer;
	}

#ifndef __SERIES60_3X__
	iMobileNetworkInfo.Close();
	iServer.Close();
#else
	if(iTelephony) {
		delete iTelephony;
	}
#endif
}

void CCellTowerDataHandler::ConstructL() {
#ifndef __SERIES60_3X__
	RTelServer::TPhoneInfo info;
 	User::LeaveIfError(iServer.Connect());

	User::LeaveIfError(iServer.LoadPhoneModule(_L("phonetsy.tsy")));
 	User::LeaveIfError(iServer.GetPhoneInfo(0, info));

	User::LeaveIfError(iMobileNetworkInfo.Open(iServer, info.iName));
#else
	iTelephony = CTelephony::NewL();
#endif

	iTimer = CCustomTimer::NewL(this);
	CActiveScheduler::Add(this);
}

void CCellTowerDataHandler::StartL() {
	if(!IsActive() && iEngineStatus == ECellTowerDisconnected) {
		iEngineStatus = ECellTowerReading;

#ifndef __SERIES60_3X__
		RMobilePhone::TMobilePhoneNetworkInfoV1Pckg aNetworkPckg(iNetwork);
		iMobileNetworkInfo.GetCurrentNetwork(iStatus, aNetworkPckg, iArea);
#else
		CTelephony::TNetworkInfoV1Pckg aNetworkInfoV1Pckg(iCellTowerData);
		iTelephony->GetCurrentNetworkInfo(iStatus, aNetworkInfoV1Pckg);
#endif
		SetActive();
	}

	iTimer->After(15000000);
}

TBool CCellTowerDataHandler::IsConnected() {
	return (iEngineStatus != ECellTowerDisconnected);
}

void CCellTowerDataHandler::Stop() {
	iTimer->Stop();

	DoCancel();
}

void CCellTowerDataHandler::RunL() {
	switch(iEngineStatus) {
		case ECellTowerReading:
			if(iStatus == KErrNone) {
				iEngineStatus = ECellTowerDisconnected;

#ifndef __SERIES60_3X__
				iObserver->CellTowerData(iNetwork.iCountryCode, iNetwork.iNetworkId, iArea.iLocationAreaCode, iArea.iCellId);
#else
				iObserver->CellTowerData(iCellTowerData.iCountryCode, iCellTowerData.iNetworkId, iCellTowerData.iLocationAreaCode, iCellTowerData.iCellId);
#endif
			}
			else {
				iEngineStatus = ECellTowerDisconnected;
				iObserver->CellTowerError(ECellTowerReadError);
			}
			break;
		case ECellTowerDisconnecting:
			iEngineStatus = ECellTowerDisconnected;
			break;
		default:;
	}
}

TInt CCellTowerDataHandler::RunError(TInt /*aError*/) {
	return KErrNone;
}

void CCellTowerDataHandler::DoCancel() {
	switch(iEngineStatus) {
		case ECellTowerReading:
#ifndef __SERIES60_3X__
			iEngineStatus = ECellTowerDisconnected;
#else
			iEngineStatus = ECellTowerDisconnecting;
			iTelephony->CancelAsync(CTelephony::EGetCurrentNetworkInfoCancel);
#endif
			break;
		default:;
	}
}

void CCellTowerDataHandler::TimerExpired(TInt /*aExpiryId*/) {
	StartL();
}

// End of File
