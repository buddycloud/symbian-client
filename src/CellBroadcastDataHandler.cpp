/*
============================================================================
 Name        : 	CellTowerDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for Cell Broadcast Identification
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
//#include <commdb.h>
#include "CellBroadcastDataHandler.h"

CCellBroadcastDataHandler::CCellBroadcastDataHandler(MCellBroadcastNotification* aObserver) : iBroadcastAttributesPckg(iBroadcastAttributes) {
	iObserver = aObserver;
}

CCellBroadcastDataHandler* CCellBroadcastDataHandler::NewL(MCellBroadcastNotification* aObserver) {
	CCellBroadcastDataHandler* self = new (ELeave) CCellBroadcastDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CCellBroadcastDataHandler::~CCellBroadcastDataHandler() {
//	Stop();
}

void CCellBroadcastDataHandler::ConstructL() {
	CActiveScheduler::Add(this);
}

void CCellBroadcastDataHandler::StartL() {
	TInt aResult;

	if(!IsActive() && iEngineStatus == ECellBroadcastDisconnected) {
		RTelServer::TPhoneInfo aPhoneInfo;
		TInt aEnumPhones;

		if((aResult = iServer.Connect()) == KErrNone) {
	 		if((aResult = iServer.LoadPhoneModule(_L("PHONETSY"))) == KErrNone) {
	 			iServer.EnumeratePhones(aEnumPhones);

	 			if(aEnumPhones > 0) {
	 				if((aResult = iServer.GetPhoneInfo(0, aPhoneInfo)) == KErrNone) {
	 					if((aResult = iMobileNetworkInfo.Open(iServer, aPhoneInfo.iName)) == KErrNone) {
	 						if((aResult = iBroadcastMessageInfo.Open(iMobileNetworkInfo)) == KErrNone) {
	 							iEngineStatus = ECellBroadcastConnected;
	 							RMobileBroadcastMessaging::TMobileBroadcastCapsV1Pckg aCapsPckg(iCaps);

	 							if((aResult = iBroadcastMessageInfo.GetCaps(aCapsPckg)) == KErrNone) {

	 								RMobileBroadcastMessaging::TMobilePhoneBroadcastFilter aFilter;
	 								iBroadcastMessageInfo.GetFilterSetting(aFilter);

	 								if(aFilter != RMobileBroadcastMessaging::EBroadcastAcceptAll) {
										iBroadcastMessageInfo.SetFilterSetting(iStatus, RMobileBroadcastMessaging::EBroadcastAcceptAll);
										iEngineStatus = ECellBroadcastSetFilter;
									}
									else {
	 									if(iCaps.iModeCaps == RMobileBroadcastMessaging::KCapsGsmTpduFormat) {
	 										iBroadcastMessageInfo.ReceiveMessage(iStatus, iGsmBroadcastMessage, iBroadcastAttributesPckg);
	 									}
	 									else {
	 										iBroadcastMessageInfo.ReceiveMessage(iStatus, iCdmaBroadcastMessage, iBroadcastAttributesPckg);
	 									}

	 									iEngineStatus = ECellBroadcastReading;
									}

	 								SetActive();
	 							}
	 						}
	 					}
	 				}
	 			}
	 		}
	 	}

		if(aResult != KErrNone) {
			iBroadcastMessageInfo.Close();
			iMobileNetworkInfo.Close();
			iServer.Close();
			iObserver->CellBroadcastError(ECellBroadcastUnavailable);
		}
	}
}

TBool CCellBroadcastDataHandler::IsConnected() {
	return (iEngineStatus != ECellBroadcastDisconnected);
}

void CCellBroadcastDataHandler::Stop() {
	DoCancel();
}

void CCellBroadcastDataHandler::ProcessData() {
	TBuf16<255> aBroadcastData;
	int i;
	TBuf8<512> aLocationString;
	TChar cbuf;
	unsigned int bb = 0;
	unsigned char ur,curr,prev = 0;

	if(iCaps.iModeCaps == RMobileBroadcastMessaging::KCapsGsmTpduFormat) {
		aBroadcastData.Copy(iGsmBroadcastMessage);
	}
	else {
		aBroadcastData.Copy(iCdmaBroadcastMessage);
	}

	// Header
	TUint16 aIdentifier = (aBroadcastData[2] << 8) | aBroadcastData[3];

	if(aIdentifier == 50 || aIdentifier == 221) {
		for(i=6;i<aBroadcastData.Length();i++) {
			cbuf = aBroadcastData[i];
			unsigned char aa = (1 << (7 - bb%7)) - 1;
			ur = cbuf & aa;
			ur = (ur << (bb)) | prev;
			curr = cbuf & (0xff ^ aa);
			curr = curr >> (7 - bb);
			prev = curr;
			if(ur == 0xd) {
				break;
			}

			aLocationString.Append(ur);

			bb = ++bb % 7;

			if(bb==0) {
				aLocationString.Append(prev);
				prev =0;
			}
		}

		if(aLocationString.Length() <= aBroadcastData.MaxLength()) {
			aBroadcastData.Copy(aLocationString);
		}

		iObserver->CellBroadcastData(aLocationString);
	}
	else {
		iObserver->CellBroadcastError(ECellBroadcastReadError);
	}
}

void CCellBroadcastDataHandler::RunL() {
	switch(iEngineStatus) {
		case ECellBroadcastSetFilter:
			if(iStatus == KErrNone) {
	 			if(iCaps.iModeCaps == RMobileBroadcastMessaging::KCapsGsmTpduFormat) {
	 				iBroadcastMessageInfo.ReceiveMessage(iStatus, iGsmBroadcastMessage, iBroadcastAttributesPckg);
	 			}
	 			else {
	 				iBroadcastMessageInfo.ReceiveMessage(iStatus, iCdmaBroadcastMessage, iBroadcastAttributesPckg);
	 			}

	 			iEngineStatus = ECellBroadcastReading;
	 			SetActive();
			}
			else {
				iObserver->CellBroadcastError(ECellBroadcastReadError);

				iBroadcastMessageInfo.Close();
				iMobileNetworkInfo.Close();
				iServer.Close();

				iEngineStatus = ECellBroadcastDisconnected;
			}
			break;
		case ECellBroadcastReading:
			iEngineStatus = ECellBroadcastConnected;

			if(iStatus == KErrNone) {
				ProcessData();
			}
			else {
				iObserver->CellBroadcastError(ECellBroadcastReadError);
			}

			iBroadcastMessageInfo.Close();
			iMobileNetworkInfo.Close();
			iServer.Close();

			iEngineStatus = ECellBroadcastDisconnected;
			break;
		case ECellBroadcastDisconnecting:
			iBroadcastMessageInfo.Close();
			iMobileNetworkInfo.Close();
			iServer.Close();

			iEngineStatus = ECellBroadcastDisconnected;
			break;
		default:;
	}
}

TInt CCellBroadcastDataHandler::RunError(TInt aError) {
	Stop();

	return KErrNone;
}

void CCellBroadcastDataHandler::DoCancel() {
	switch(iEngineStatus) {
		case ECellBroadcastConnected:
			iBroadcastMessageInfo.Close();
			iMobileNetworkInfo.Close();
			iServer.Close();

			iEngineStatus = ECellBroadcastDisconnected;
			break;
		case ECellBroadcastSetFilter:
			iEngineStatus = ECellBroadcastDisconnecting;
			iBroadcastMessageInfo.CancelAsyncRequest(4803);
			break;
		case ECellBroadcastReading:
			iEngineStatus = ECellBroadcastDisconnecting;
			iBroadcastMessageInfo.CancelAsyncRequest(4801);
			break;
		default:;
	}
}

// End of File

