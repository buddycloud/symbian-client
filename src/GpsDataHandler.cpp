/*
============================================================================
 Name        : 	GpsDataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Handle data for Gps resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "GpsDataHandler.h"
#include <aknnotewrappers.h>

CGpsDataHandler::CGpsDataHandler(MGpsNotification* aObserver) {
	iObserver = aObserver;
}

CGpsDataHandler* CGpsDataHandler::NewL(MGpsNotification* aObserver) {
	CGpsDataHandler* self = new (ELeave) CGpsDataHandler(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CGpsDataHandler::~CGpsDataHandler() {
//	Stop();

#ifndef __SERIES60_3X__
	if(iBuffer)
		delete iBuffer;
#endif
}

void CGpsDataHandler::ConstructL() {
	iEngineStatus = EGpsDisconnected;

#ifndef __SERIES60_3X__
	iGpsDeviceResolved = false;
	iBuffer = HBufC8::NewL(82);
#endif

	CActiveScheduler::Add(this);
}

void CGpsDataHandler::StartL() {
	if(!IsActive()) {	
		if(iEngineStatus == EGpsDisconnected) {
			ResolveAndConnectL();
		}
#ifdef __SERIES60_3X__
		else if(iEngineStatus == EGpsConnected) {
			iEngineStatus = EGpsReading;

			iPositioner.NotifyPositionUpdate(iPositionInfo, iStatus);

			SetActive();
		}
#endif
	}	
}

void CGpsDataHandler::Stop() {
	DoCancel();
	
	iGpsWarmedUp = false;
}

TBool CGpsDataHandler::IsConnected() {
	return (iEngineStatus != EGpsDisconnected);
}

void CGpsDataHandler::ResolveAndConnectL() {
	TInt aResult;

#ifndef __SERIES60_3X__

	if((aResult = iSocketServ.Connect()) == KErrNone) {
		if(iGpsDeviceResolved) {
			if((aResult = iSocket.Open(iSocketServ, KBTAddrFamily, KSockStream, KRFCOMM)) == KErrNone) {
				iEngineStatus = EGpsConnecting;

				iSocket.Connect(iBTAddr, iStatus);
				SetActive();
			}
		}
		else {
			_LIT(KLinkMan, "BTLinkManager");
			//_LIT(KLinkMan, "RFCOMM");
			TProtocolName aProtocolName(KLinkMan);

			if((aResult = iSocketServ.FindProtocol(aProtocolName, iProtocolDesc)) == KErrNone) {
				if((aResult = iResolver.Open(iSocketServ, iProtocolDesc.iAddrFamily, iProtocolDesc.iProtocol)) == KErrNone) {
					TInquirySockAddr aInquiry;
					aInquiry.SetIAC(KGIAC);
					//aInquiry.SetMajorServiceClass(EMajorServicePositioning);
					aInquiry.SetAction(KHostResName | KHostResInquiry | KHostResIgnoreCache);
					iResolver.GetByAddress(aInquiry, iNameEntry, iStatus);

					iEngineStatus = EGpsResolving;
					SetActive();
				}
			}
		}
	}

	if(aResult != KErrNone) {
		iEngineStatus = EGpsDisconnected;
		iSocketServ.Close();
		iObserver->GpsError(EGpsConnectionFailed);
	}
#else
	TBool aDeviceFound = false;

	if((aResult = iPositionServ.Connect()) == KErrNone) {
		iEngineStatus = EGpsConnecting;
		TUint aNumOfModules;

		if((aResult = iPositionServ.GetNumModules(aNumOfModules)) == KErrNone) {
			TPositionModuleInfo aModuleInfo;
			TUid aModuleId;
			
			for(TUint i = 0; i < aNumOfModules && aResult == KErrNone && !aDeviceFound; i++) {
				if((aResult = iPositionServ.GetModuleInfoByIndex(i, aModuleInfo)) == KErrNone) {
					aModuleId = aModuleInfo.ModuleId();
					
					if(aModuleInfo.IsAvailable() && aModuleInfo.TechnologyType() != TPositionModuleInfo::ETechnologyNetwork) {
						aDeviceFound = true;
					}
				}
			}

			if(aResult == KErrNone && aDeviceFound) {
				if((aResult = iPositioner.Open(iPositionServ, aModuleId)) == KErrNone) {
					iEngineStatus = EGpsConnected;

					if((aResult = iPositioner.SetRequestor(CRequestor::ERequestorService, CRequestor::EFormatApplication, _L("Buddycloud"))) == KErrNone) {
						iEngineStatus = EGpsReading;
						
						iPositioner.NotifyPositionUpdate(iPositionInfo, iStatus);
						SetActive();
					}

					if(aResult != KErrNone) {
						iPositioner.Close();
					}
				}
			}
		}
	}

	if(aResult != KErrNone || !aDeviceFound) {
		iEngineStatus = EGpsDisconnected;
		iPositionServ.Close();
		iObserver->GpsError(EGpsConnectionFailed);
	}
#endif
}

#ifndef __SERIES60_3X__
TReal CGpsDataHandler::ParseCoordinateData() {
	TPtr8 pBuffer = iBuffer->Des();
	TReal aAngle;
	TReal aResult = 0.0;
	TInt aFindResult;

	aFindResult = pBuffer.Find(_L8(","));
	if(aFindResult != KErrNotFound) {
		HBufC8* aField = HBufC8::NewL(aFindResult);
		CleanupStack::PushL(aField);
		TPtr8 pField = aField->Des();

		pField.Copy(pBuffer.Ptr(), aFindResult);
		pBuffer.Delete(0, aFindResult+1);

		TLex8 aFieldLex(pField.Ptr());
		if(aFieldLex.Val(aAngle, '.') == KErrNone) {
			TInt32 aDegrees;
			Math::Int(aDegrees, aAngle / 100.0);

			TInt32 aMinutes;
			Math::Int(aMinutes, aAngle - aDegrees * 100);

			TReal aDecimal;
			Math::Frac(aDecimal, aAngle);

			aResult = aDegrees + (aMinutes + aDecimal) / 60.0;

			if(pBuffer[0] == TUint('S') || pBuffer[0] == TUint('W')) {
				aResult = -aResult;
			}

			aFindResult = pBuffer.Find(_L8(","));

			if(aFindResult != KErrNotFound) {
				pBuffer.Delete(0, aFindResult+1);
			}
		}

		CleanupStack::PopAndDestroy();
	}

	return aResult;
}

void CGpsDataHandler::ProcessData() {
	TPtr8 pBuffer = iBuffer->Des();

	// Check sentence is complete
	if(pBuffer[0] != TUint('$')) {
		// Burn invalid data
		pBuffer.Delete(0, 1);

		iEngineStatus = EGpsReading;
		iSocket.Recv(iChar, 0, iStatus);
		SetActive();
	}
	else if(pBuffer[pBuffer.Length()-1] == TUint('\n')) {
		// Sentence is complete
		TInt aStartField;
		TInt aFindResult;

		// Ready to parse data
		if(pBuffer.Find(_L8("$GPGLL")) != KErrNotFound) {
			aStartField = 1;
		}
		else if(pBuffer.Find(_L8("$GPGGA")) != KErrNotFound) {
			aStartField = 2;
		}
		else if(pBuffer.Find(_L8("$GPRMC")) != KErrNotFound) {
			aStartField = 3;
		}
		else {
			pBuffer.Zero();

			iEngineStatus = EGpsReading;
			iSocket.Recv(iChar, 0, iStatus);
			SetActive();

			return;
		}

		// Burn useless data
		for(TInt i = 0;i < aStartField;i++) {
			aFindResult = pBuffer.Find(_L8(","));

			if(aFindResult != KErrNotFound) {
				pBuffer.Delete(0, aFindResult+1);
			}
		}

		// Read latitude
		TReal aLatitude = ParseCoordinateData();
		TReal aLongitude = ParseCoordinateData();

		pBuffer.Zero();

		if(aLatitude != 0.0 || aLongitude != 0.0) {
			iObserver->GpsData(aLatitude, aLongitude, 10.0);

			iSocket.Close();
			iSocketServ.Close();
			iEngineStatus = EGpsDisconnected;
		}
	}
	else {
		iEngineStatus = EGpsReading;
		iSocket.Recv(iChar, 0, iStatus);
		SetActive();
	}
}
#endif

void CGpsDataHandler::RunL() {
#ifndef __SERIES60_3X__
	TPtr8 pBuffer = iBuffer->Des();

	THostName aHostName;
	TInquirySockAddr aInquiry;
	TInt aServiceClass;

	switch(iEngineStatus) {
		case EGpsResolving:
			if(iStatus == KErrNone) {
				// Next device found
				aHostName = iNameEntry().iName;

				aInquiry = TInquirySockAddr::Cast(iNameEntry().iAddr);
				aServiceClass = aInquiry.MajorServiceClass();
				iBTAddr.SetBTAddr(aInquiry.BTAddr());
				iBTAddr.SetPort(1);

				if(aServiceClass & 0x08 || aHostName.Find(_L("GPS")) != KErrNotFound || aHostName.Find(_L("gps")) != KErrNotFound) {
					// Found Positioning device
					if(iSocket.Open(iSocketServ, KBTAddrFamily, KSockStream, KRFCOMM) == KErrNone) {
						iEngineStatus = EGpsConnecting;
						//iResolver.Close();

						iSocket.Connect(iBTAddr, iStatus);
						SetActive();
					}
					else {
						iSocketServ.Close();
						iEngineStatus = EGpsDisconnected;
						iObserver->GpsError(EGpsConnectionFailed);
					}
				}
				else {
					// Get next
					iResolver.Next(iNameEntry, iStatus);
    				SetActive();
				}

				// TODO - Is correct device?

			}
			else if(iStatus == KErrHostResNoMoreResults) {
				// No (more) devices found
				iSocketServ.Close();
				iEngineStatus = EGpsDisconnected;
				iGpsDeviceResolved = false;
				iObserver->GpsError(EGpsDeviceUnavailable);
			}
			else {
				iSocketServ.Close();
				iEngineStatus = EGpsDisconnected;
				iObserver->GpsError(EGpsDeviceUnavailable);
			}
			break;
		case EGpsConnecting:
			if(iStatus == KErrNone) {
				iEngineStatus = EGpsReading;
				iGpsDeviceResolved = true;
				iResolver.Close();

				pBuffer.Zero();

				iSocket.Recv(iChar, 0, iStatus);
				SetActive();
			}
			else {
				if(iGpsDeviceResolved) {
					iSocket.Close();
					iSocketServ.Close();
					iEngineStatus = EGpsDisconnected;
					iObserver->GpsError(EGpsConnectionFailed);
				}
				else {
					// Get next
					iEngineStatus = EGpsResolving;
					iResolver.Next(iNameEntry, iStatus);
					SetActive();
				}
			}
			break;
		case EGpsReading:
			if(iStatus == KErrNone) {
				iEngineStatus = EGpsConnected;

				if(pBuffer.Length() + iChar.Length() > pBuffer.MaxLength()) {
					iBuffer = iBuffer->ReAlloc(pBuffer.MaxLength()+iChar.Length());
					pBuffer.Set(iBuffer->Des());
				}

				pBuffer.Append(iChar);
				ProcessData();
			}
			else {
				iSocket.Close();
				iSocketServ.Close();
				iEngineStatus = EGpsDisconnected;
				iObserver->GpsError(EGpsConnectionFailed);
			}
			break;
		case EGpsDisconnecting:
			iResolver.Close();
			iSocket.Close();
			iSocketServ.Close();
			iEngineStatus = EGpsDisconnected;
			break;
		default:;
	}
#else
	switch(iEngineStatus) {
		case EGpsReading:
			iEngineStatus = EGpsConnected;
			
			if(iStatus == KErrNone) {
				TPosition aPosition;
				iPositionInfo.GetPosition(aPosition);

				iObserver->GpsData(aPosition.Latitude(), aPosition.Longitude(), aPosition.HorizontalAccuracy());
				
				if( !iGpsWarmedUp ) {
					iGpsWarmedUp = true;
					
					TPositionUpdateOptions aOptions;
					aOptions.SetUpdateTimeOut(30000000);						
					iPositioner.SetUpdateOptions(aOptions);
				}
			}
			else {
				iObserver->GpsError(EGpsDeviceUnavailable);
			}
			break;
		case EGpsDisconnecting:
			iPositioner.Close();
			iPositionServ.Close();
			iEngineStatus = EGpsDisconnected;
			break;
		default:;
	}
#endif
}

TInt CGpsDataHandler::RunError(TInt /*aError*/) {
	Stop();

	return KErrNone;
}

void CGpsDataHandler::DoCancel() {
#ifndef __SERIES60_3X__
	switch(iEngineStatus) {
		case EGpsResolving:
			iEngineStatus = EGpsDisconnecting;
			iResolver.Cancel();
			break;
		case EGpsConnecting:
			iEngineStatus = EGpsDisconnecting;
			iSocket.CancelConnect();
			break;
		case EGpsReading:
			iEngineStatus = EGpsDisconnecting;
			iSocket.CancelRecv();
			break;
		default:
			iEngineStatus = EGpsDisconnected;
			break;
	}
#else
	switch(iEngineStatus) {
		case EGpsConnecting:
			iEngineStatus = EGpsDisconnected;
			iPositionServ.Close();
			break;
		case EGpsConnected:
			iEngineStatus = EGpsDisconnected;
			iPositioner.Close();
			iPositionServ.Close();
			break;
		case EGpsReading:
			iEngineStatus = EGpsDisconnecting;
			iPositioner.CancelRequest(EPositionerNotifyPositionUpdate);
			break;
		default:
			iEngineStatus = EGpsDisconnected;
			break;
	}
#endif
}

// End of File

