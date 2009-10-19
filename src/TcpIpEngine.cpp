/*
============================================================================
 Name        : 	TcpIpEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	TCP/IP Engine for Asynchronous Reading and Writing from/to
 		a TCP/IP Socket.
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#include <aknutils.h>
#include <commdbconnpref.h>
#include <cdbcols.h>
#include "TcpIpEngine.h"

#ifdef __3_2_ONWARDS__
#include <cmapplicationsettingsui.h>
#include <cmdefconnvalues.h>
#include <cmdestination.h>
#include <cmmanager.h>
#include <connpref.h>
#endif

const TInt KTimeOut = 30000000; // 30 seconds time-out

/*
----------------------------------------------------------------------------
--
-- CTcpIpEngine
--
----------------------------------------------------------------------------
*/

CTcpIpEngine::CTcpIpEngine() : CActive(EPriorityStandard) {
}

CTcpIpEngine* CTcpIpEngine::NewL(MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpEngine* self = NewLC(aEngineObserver);
	CleanupStack::Pop();
	return self;
}

CTcpIpEngine* CTcpIpEngine::NewLC(MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpEngine* self = new(ELeave) CTcpIpEngine;
	CleanupStack::PushL(self);
	self->ConstructL(aEngineObserver);
	return self;
}

void CTcpIpEngine::ConstructL(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
	CActiveScheduler::Add(this);

	iEngineStatus = ETcpIpDisconnected;

	iConnectionName = HBufC::NewL(KMaxName);

	User::LeaveIfError(iSocketServ.Connect());

	iTcpIpRead = CTcpIpRead::NewL(&iTcpIpSocket, iEngineObserver);
	iTcpIpWrite = CTcpIpWrite::NewL(&iTcpIpSocket, iEngineObserver);
}

CTcpIpEngine::~CTcpIpEngine() {
	// Disconnect from server
	Disconnect();
	
	if(IsActive()) {
		User::WaitForRequest(iStatus);
	}

	// Free allocated memory
	if(iSockHost)
		delete iSockHost;
	
	if(iConnectionName)
		delete iConnectionName;

	if(iTcpIpRead)
		delete iTcpIpRead;

	if(iTcpIpWrite)
		delete iTcpIpWrite;
	
	iSocketServ.Close();
}

void CTcpIpEngine::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;

	iTcpIpRead->SetObserver(iEngineObserver);
	iTcpIpWrite->SetObserver(iEngineObserver);
}

#ifdef __3_2_ONWARDS__
TBool CTcpIpEngine::SelectConnectionPrefL() {
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::SelectConnectionPrefL"), iEngineStatus);

	CCmApplicationSettingsUi* aSettingsUi = CCmApplicationSettingsUi::NewLC();
	TCmSettingSelection aUserSelection;
	TBearerFilterArray aFilter;
	TUint aListedModes = CMManager::EShowDefaultConnection | CMManager::EShowDestinations | CMManager::EShowConnectionMethods;

	// Request connection preference from user
	TBool aSelectionSuccess = aSettingsUi->RunApplicationSettingsL(aUserSelection, aListedModes, aFilter);				                                   
	CleanupStack::PopAndDestroy(); // aSettingsUi
	
	// Get preference
	if(aSelectionSuccess) {
		switch(aUserSelection.iResult) {
			case CMManager::EDestination:
				iConnectionMode = ETcpIpModeDestinations;
				iConnectionModeId = aUserSelection.iId;
				break;
			case CMManager::EConnectionMethod:
				iConnectionMode = ETcpIpModeSelectedIap;
				iConnectionModeId = aUserSelection.iId;
				break;
			default:
				iConnectionMode = ETcpIpModeUseDefault;
				iConnectionModeId = 0;
				break;				
		}
	}
	
	return aSelectionSuccess;
}
#endif

void CTcpIpEngine::OpenConnectionL() {
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::OpenConnectionL"), iEngineStatus);
	
	TInt aErrorCode;
	
	if((aErrorCode = iConnection.Open(iSocketServ)) == KErrNone) {
		iEngineStatus = ETcpIpStart;
		
#ifdef __3_2_ONWARDS__
		TBool aSelectionSuccess = true;
		
		iEngineObserver->TcpIpDebug(_L8("TCP   Connection open"));
	
		if(iConnectionMode == ETcpIpModeUnknown || iConnectionModeId == 0) {
			aSelectionSuccess = SelectConnectionPrefL();
		}
		
		if(aSelectionSuccess) {			
			iEngineObserver->TcpIpDebug(_L8("TCP   Starting connection"));
			
			// Start connection by mode
			switch(iConnectionMode) {
				case ETcpIpModeSelectedIap:
				{
			        TCommDbConnPref aPref;
			        aPref.SetIapId(iConnectionModeId);
			        aPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

			        iConnection.Start(aPref, iStatus);
				    break;
				}
				case ETcpIpModeDestinations:
				{
			        TConnSnapPref aPref;
			        aPref.SetSnap(iConnectionModeId);

			        iConnection.Start(aPref, iStatus);
			        break;
				}
				default:
				    iConnection.Start(iStatus);
				    break;
			}
			
			iEngineObserver->TcpIpDebug(_L8("TCP   Connection started"));
			
			SetActive();
		}
		else {
			iConnection.Close();
			iEngineStatus = ETcpIpDisconnected;
			
			iEngineObserver->Error(ETcpIpCancelled);
		}
#else
		TCommDbConnPref aPref;
		
		if(iConnectionMode == ETcpIpModeUnknown || iConnectionModeId == 0) {			
			// Request iap
			aPref.SetDialogPreference(ECommDbDialogPrefPrompt);
		}
		else {
			// Connect to prefered iap
			aPref.SetIapId(iConnectionModeId);
			aPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);			
		}
		
		iConnectionMode = ETcpIpModeSelectedIap;
		iConnection.Start(aPref, iStatus);		
		SetActive();
#endif
	}
	else {
		iEngineObserver->TcpIpDebug(_L8("TCP   RConnection::Open Error"), aErrorCode);
	}
}

void CTcpIpEngine::CloseConnectionL() {
#ifdef __3_2_ONWARDS__
	if(iMobility) {
		delete iMobility;
		iMobility = NULL;
	}
#endif
	
	iConnection.Close();
}

void CTcpIpEngine::ConnectL(TSockAddr aAddr) {
	TInt aErrorCode;
	
	if(!IsActive() && (aErrorCode = iTcpIpSocket.Open(iSocketServ, KAfInet, KSockStream, KProtocolInetTcp, iConnection)) == KErrNone) {
		iTcpIpSocket.Connect(aAddr, iStatus);
		iEngineStatus = ETcpIpConnecting;
		iEngineObserver->NotifyEvent(iEngineStatus);

		SetActive();
	}
	else {
		iConnection.Close();

		iEngineStatus = ETcpIpDisconnected;
		iEngineObserver->Error(ETcpIpConnectFailed);
		iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Open Error"), aErrorCode);
	}
}

void CTcpIpEngine::GetConnectionMode(TInt& aMode, TInt& aId) {
	aMode = TInt(iConnectionMode);
	aId = iConnectionModeId;	
}

void CTcpIpEngine::SetConnectionMode(TInt aMode, TInt aId, const TDesC& aName) {
	iConnectionMode = TTcpIpEngineConnectionMode(aMode);
	iConnectionModeId = aId;	
	
	if(iConnectionMode == ETcpIpModeUnknown || iConnectionModeId == 0) {
		TPtr pConnectionName(iConnectionName->Des());
		pConnectionName.Zero();
	}
	else {
		if(iConnectionName) {
			delete iConnectionName;
		}
		
		iConnectionName = aName.AllocL();
	}
}

TDesC& CTcpIpEngine::GetConnectionName() {
	return *iConnectionName;
}

void CTcpIpEngine::GetConnectionInformationL() {
	if(iConnectionName) {
		delete iConnectionName;
		iConnectionName = NULL;
	}

	if(iConnectionMode > ETcpIpModeDestinations) {
		iConnectionName = HBufC::NewL(KMaxName);		
		TPtr pConnectionName(iConnectionName->Des());
		
		// Get connection information
		iConnection.GetDesSetting(_L("IAP\\Name"), pConnectionName);
		iConnection.GetIntSetting(_L("IAP\\Id"), iConnectionModeId);
		
		iEngineObserver->TcpIpDebug(_L8("TCP   IAP ID"), iConnectionModeId);
	}
#ifdef __3_2_ONWARDS__
	else if(iConnectionMode == ETcpIpModeDestinations) {
		// Open manager
		RCmManager aCmManager;
		aCmManager.OpenLC();
		
		// Get destination
		RCmDestination aCmDestination = aCmManager.DestinationL(iConnectionModeId);
		CleanupClosePushL(aCmDestination);
		
		// Get destination name
		iConnectionName = aCmDestination.NameLC();
		CleanupStack::Pop(); // iConnectionName
		
		// Close objects
		CleanupStack::PopAndDestroy(2); // aCmDestination, aCmManager
	}
#endif
}

void CTcpIpEngine::ConnectL(const TDesC& aServerName, TInt aPort) {
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::ConnectL"), iEngineStatus);

	if(!IsActive() && iEngineStatus == ETcpIpDisconnected) {
		if(iSockHost)
			delete iSockHost;

		iSockHost = aServerName.Alloc();
		iSockPort = aPort;

		// Open channel to Socket Server
		if(iSocketServ.Handle()) {
			OpenConnectionL();
		}
	}
	else {
		iEngineObserver->Error(ETcpIpAlreadyBusy);
	}
}

void CTcpIpEngine::Write(const TDesC16& aMessage) {
	HBufC8* aData = HBufC8::NewLC(aMessage.Length());
	TPtr8 pData(aData->Des());
	pData.Copy(aMessage);

	Write(*aData);

	CleanupStack::PopAndDestroy();
}

void CTcpIpEngine::Write(const TDesC8& aMessage) {
	if(iEngineStatus == ETcpIpConnected) {
		iTcpIpWrite->IssueWrite(aMessage);
	}
}

void CTcpIpEngine::Read() {
	if ((iEngineStatus == ETcpIpConnected) && !iTcpIpRead->IsActive()) {
		iTcpIpRead->IssueRead();
	}
}

void CTcpIpEngine::RunL() {
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::RunL"), iEngineStatus);

	switch(iEngineStatus) {
		case ETcpIpStart:
			if(iStatus == KErrNone) {
				if(iResolver.Open(iSocketServ, KAfInet, KProtocolInetTcp, iConnection) == KErrNone) {
					// Resolving Host
					// DNS request for name resolution
					iEngineStatus = ETcpIpLookingUp;
					iEngineObserver->NotifyEvent(iEngineStatus);					
#ifdef __3_2_ONWARDS__
					if(iMobility == NULL) {
						iMobility = CActiveCommsMobilityApiExt::NewL(iConnection, *this);
					}
#endif
					iResolver.GetByName(*iSockHost, iNameEntry, iStatus);
					SetActive();
				}
				else {
					iConnection.Close();

					iEngineStatus = ETcpIpDisconnected;
					iEngineObserver->Error(ETcpIpAccessPointFailed);
					iEngineObserver->TcpIpDebug(_L8("TCP   RHostResolver::Open Error"), iStatus.Int());
				}
			}
			else {
				iConnection.Close();
				iEngineStatus = ETcpIpDisconnected;

				if(iStatus == KErrCancel) {
					iEngineObserver->Error(ETcpIpCancelled);
				}
				else {
					iEngineObserver->Error(ETcpIpAccessPointFailed);
				}

				iEngineObserver->TcpIpDebug(_L8("TCP   RConnection::Start Error"), iStatus.Int());
			}
			break;
		case ETcpIpLookingUp:
			iResolver.Close();

			if(iStatus == KErrNone) {
				// Connecting...
				iEngineStatus = ETcpIpDisconnected;
				
				GetConnectionInformationL();

				// Open socket
				TSockAddr aSockAddr = iNameEntry().iAddr;
				aSockAddr.SetPort(iSockPort);
				ConnectL(aSockAddr);
			}
			else {
				CloseConnectionL();

				iEngineStatus = ETcpIpDisconnected;
				iEngineObserver->Error(ETcpIpLookUpFailed);
				iEngineObserver->TcpIpDebug(_L8("TCP   RHostResolver::GetByName Error"), iStatus.Int());
			}
			break;
		case ETcpIpConnecting:
			if(iStatus == KErrNone) {
				iEngineStatus = ETcpIpConnected;
				iEngineObserver->NotifyEvent(iEngineStatus);
				
				GetConnectionInformationL();

				// Start reading data
				Read();
			}
			else {
				iTcpIpSocket.Close();
				CloseConnectionL();

				iEngineStatus = ETcpIpDisconnected;
				iEngineObserver->Error(ETcpIpConnectFailed);
				iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Connect Error"), iStatus.Int());
			}
			break;
		case ETcpIpCarrierChanging:
			iTcpIpSocket.Close();
			break;
		case ETcpIpDisconnecting:
			iTcpIpSocket.Close();
			CloseConnectionL();

			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		default:;
	}
}

void CTcpIpEngine::Disconnect() {
	// Cancel active request
	Cancel();

	// Disconnect
	switch (iEngineStatus) {
		case ETcpIpStart:
			iConnection.Close();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		case ETcpIpLookingUp:
			iResolver.Close();
			CloseConnectionL();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		case ETcpIpConnecting:
		case ETcpIpConnected:
		case ETcpIpCarrierChangeReq:
			iTcpIpSocket.Shutdown(RSocket::EImmediate, iStatus);
			iTcpIpRead->Cancel();
			iTcpIpWrite->Reset();
			iTcpIpWrite->Cancel();
			
			if(iEngineStatus == ETcpIpCarrierChangeReq) {
				iEngineObserver->NotifyEvent(iEngineStatus);
				iEngineStatus = ETcpIpCarrierChanging;
			}
			else {
				iEngineStatus = ETcpIpDisconnecting;
			}
			
			SetActive();
			break;
		case ETcpIpCarrierChanging:
			iTcpIpSocket.Close();
			CloseConnectionL();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		case ETcpIpDisconnecting:
			iTcpIpSocket.Close();
			CloseConnectionL();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		default:
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
	}
}

void CTcpIpEngine::DoCancel() {
	switch (iEngineStatus) {
		case ETcpIpConnecting:
		case ETcpIpConnected:
		case ETcpIpCarrierChangeReq:
			iTcpIpSocket.CancelAll();
			break;
		case ETcpIpLookingUp:
			iResolver.Cancel();
			break;
		default:;
	}
}

#ifdef __3_2_ONWARDS__
void CTcpIpEngine::PreferredCarrierAvailable(TAccessPointInfo /*aOldAp*/, TAccessPointInfo /*aNewAp*/, TBool /*aIsUpgrade*/, TBool aIsSeamless) {
	if( !aIsSeamless ) {
		if(iEngineStatus == ETcpIpConnected) {			
			// Disconnect sockets
			iEngineObserver->TcpIpDebug(_L8("TCP   PreferredCarrierAvailable - disconnect"));
			iEngineStatus = ETcpIpCarrierChangeReq;
			Disconnect();
			
			// Migrate to new carrier
			iEngineObserver->TcpIpDebug(_L8("TCP   MigrateToPreferredCarrier"));
			iMobility->MigrateToPreferredCarrier();
		}
		else {
			// Ingore carrier change
			iEngineObserver->TcpIpDebug(_L8("TCP   IgnorePreferredCarrier"));
			iMobility->IgnorePreferredCarrier();
		}
	}
}

void CTcpIpEngine::NewCarrierActive(TAccessPointInfo /*aNewAp*/, TBool aIsSeamless) {
	if( !aIsSeamless ) {
		if(iEngineStatus == ETcpIpCarrierChanging) {
			// Start socket reconnect
			iEngineObserver->TcpIpDebug(_L8("TCP   NewCarrierActive - Socket connect"));
			iEngineStatus = ETcpIpDisconnected;
			
			GetConnectionInformationL();
	
			// Open socket
			TSockAddr aSockAddr = iNameEntry().iAddr;
			aSockAddr.SetPort(iSockPort);
			ConnectL(aSockAddr);
			
			// Accept new carrier
			iEngineObserver->TcpIpDebug(_L8("TCP   NewCarrierAccepted"));
			iMobility->NewCarrierAccepted();
		}
		else {
			// Reject new carrier
			iEngineObserver->TcpIpDebug(_L8("TCP   NewCarrierRejected"));
			iMobility->NewCarrierRejected();
		}
	}
}

void CTcpIpEngine::Error(TInt aError) {	
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::Error"), aError);
}
#endif

/*
----------------------------------------------------------------------------
--
-- CTcpIpRead
--
----------------------------------------------------------------------------
*/

CTcpIpRead::CTcpIpRead() : CActive(EPriorityStandard) {
}

CTcpIpRead* CTcpIpRead::NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpRead* self = NewLC(aTcpIpSocket, aEngineObserver);
	CleanupStack::Pop();
	return self;
}

CTcpIpRead* CTcpIpRead::NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpRead* self = new(ELeave) CTcpIpRead;
	CleanupStack::PushL(self);
	self->ConstructL(aTcpIpSocket, aEngineObserver);
	return self;
}

void CTcpIpRead::ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	iTcpIpSocket = aTcpIpSocket;
	iEngineObserver = aEngineObserver;
	CActiveScheduler::Add(this);
}

CTcpIpRead::~CTcpIpRead() {
	Cancel();
}

void CTcpIpRead::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

void CTcpIpRead::DoCancel() {
	iTcpIpSocket->CancelRead();
}

void CTcpIpRead::RunL() {
	switch(iStatus.Int()) {
		case KErrNone:
			iEngineObserver->DataRead(iBuffer);
			IssueRead();
			break;
		case KErrEof:
			iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Recv EOF"), iStatus.Int());
			iEngineObserver->Error(ETcpIpReadWriteError);
			break;
		case KErrTimedOut:
			IssueRead();
			break;
		default:;			
	}
}

void CTcpIpRead::IssueRead() {
	if (!IsActive()) {
		iBuffer.Zero();
		iTcpIpSocket->RecvOneOrMore(iBuffer, 0, iStatus, iReadLength);
		SetActive();
	}
}

/*
----------------------------------------------------------------------------
--
-- CTcpIpWrite
--
----------------------------------------------------------------------------
*/

CTcpIpWrite::CTcpIpWrite() : CActive(EPriorityStandard) {
}

CTcpIpWrite* CTcpIpWrite::NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpWrite* self = NewLC(aTcpIpSocket, aEngineObserver);
	CleanupStack::Pop();
	return self;
}

CTcpIpWrite* CTcpIpWrite::NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CTcpIpWrite* self = new(ELeave) CTcpIpWrite;
	CleanupStack::PushL(self);
	self->ConstructL(aTcpIpSocket, aEngineObserver);
	return self;
}

void CTcpIpWrite::ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	iTcpIpSocket = aTcpIpSocket;
	iEngineObserver = aEngineObserver;

	iSendingBuffer = HBufC8::NewL(256);
	iWaitingBuffer = HBufC8::NewL(256);

	CActiveScheduler::Add(this);
}

CTcpIpWrite::~CTcpIpWrite() {
	Cancel();

	if(iWaitingBuffer)
		delete iWaitingBuffer;

	if(iSendingBuffer)
		delete iSendingBuffer;
}

void CTcpIpWrite::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

TBool CTcpIpWrite::WriteBuffered() {
	TPtr8 pWaitingBuffer = iWaitingBuffer->Des();
	TPtr8 pSendingBuffer = iSendingBuffer->Des();
	TInt aLength = pWaitingBuffer.Length();

	if(!IsActive() && aLength > 0) {
		if(aLength >= pSendingBuffer.MaxLength()) {
			// Send next block of bytes
			aLength = pSendingBuffer.MaxLength();
		}

		// Copy data block
		pSendingBuffer.Zero();
		pSendingBuffer.Copy(pWaitingBuffer.Ptr(), aLength);

		// Notify Observer
		iEngineObserver->DataWritten(pSendingBuffer);

		// Write Buffered Data
		iTcpIpSocket->Write(*iSendingBuffer, iStatus);
		SetActive();

		// Delete from waiting buffer
		pWaitingBuffer.Delete(0, aLength);

		return true;
	}
	else {
		return false;
	}
}

void CTcpIpWrite::DoCancel() {
	iTcpIpSocket->CancelWrite();
}

void CTcpIpWrite::RunL() {
	switch(iStatus.Int()) {
		case KErrNone:
			if(WriteBuffered() == false) {
				iEngineObserver->NotifyEvent(ETcpIpWriteComplete);
			}
			break;
		case KErrEof:
		case KErrDisconnected:
			iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Write EOF"), iStatus.Int());
			iEngineObserver->Error(ETcpIpReadWriteError);
			break;		
		default:
			iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Write"), iStatus.Int());
			break;
	}
}

void CTcpIpWrite::IssueWrite(const TDesC8& aMessage) {
	TPtr8 pWaitingBuffer = iWaitingBuffer->Des();

	// Buffer Waiting Data
	if(pWaitingBuffer.Length() + aMessage.Length() > pWaitingBuffer.MaxLength()) {
		iWaitingBuffer = iWaitingBuffer->ReAlloc(pWaitingBuffer.MaxLength()+aMessage.Length());
		pWaitingBuffer.Set(iWaitingBuffer->Des());
	}

	pWaitingBuffer.Append(aMessage);

	WriteBuffered();
}

void CTcpIpWrite::Reset() {
	TPtr8 pWaitingBuffer = iWaitingBuffer->Des();
	pWaitingBuffer.Zero();
}
