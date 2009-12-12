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
#include <SecureSocket.h>
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

	iSecureSocket = NULL;
	iSocketReader = CSocketReader::NewL(&iTcpIpSocket, iEngineObserver);
	iSocketWriter = CSocketWriter::NewL(&iTcpIpSocket, iEngineObserver);
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

	if(iSocketReader)
		delete iSocketReader;

	if(iSocketWriter)
		delete iSocketWriter;
	
	if(iSecureSocket)
		delete iSecureSocket;
	
	iSocketServ.Close();
}

void CTcpIpEngine::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;

	iSocketReader->SetObserver(iEngineObserver);
	iSocketWriter->SetObserver(iEngineObserver);
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

void CTcpIpEngine::CloseConnection() {
#ifdef __3_2_ONWARDS__
	if(iMobility) {
		delete iMobility;
		iMobility = NULL;
	}
#endif
	
	iConnection.Close();
}

void CTcpIpEngine::CloseSocket() {
	if(iSecureSocket) {
		iSecureSocket->Close();
		
		delete iSecureSocket;
		iSecureSocket = NULL;
		
		iSocketReader->SecureSocket(iSecureSocket);
		iSocketWriter->SecureSocket(iSecureSocket);
	}
	
	iTcpIpSocket.Close();
}

void CTcpIpEngine::Connect(TSockAddr aAddr) {
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

void CTcpIpEngine::SecureConnectionL(const TDesC& aProtocol) {
	iEngineObserver->TcpIpDebug(_L8("TCP   CTcpIpEngine::SecureConnectionL"), iEngineStatus);

	if(!IsActive() && iEngineStatus == ETcpIpConnected) {
		TPtrC aSockHost(iSockHost->Des());

		// Cancel ongoing socket read
		iSocketReader->CancelRead();
		iSocketWriter->Cancel();
		
		// Initialize secure socket
		iSecureSocket = CSecureSocket::NewL(iTcpIpSocket, aProtocol);
		iSecureSocket->FlushSessionCache();
		
		// Set certificate domain
		HBufC8* aDomainName = HBufC8::NewLC(aSockHost.Length());
		TPtr8 pDomainName(aDomainName->Des());
		pDomainName.Copy(aSockHost);
			
		iSecureSocket->SetOpt(KSoSSLDomainName, KSolInetSSL, pDomainName);		
		CleanupStack::PopAndDestroy(); // aDomainName
		
		// Start handshake
		iEngineStatus = ETcpIpSecureConnection;
		iSecureSocket->StartClientHandshake(iStatus);
		SetActive();
	}
	else {
		iEngineObserver->Error(ETcpIpAlreadyBusy);
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
			CloseConnection();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		case ETcpIpConnecting:
		case ETcpIpConnected:
		case ETcpIpCarrierChangeReq:
			iSocketReader->Cancel();
			iSocketWriter->Reset();
			iSocketWriter->Cancel();
			
			iTcpIpSocket.Shutdown(RSocket::EImmediate, iStatus);
		
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
			CloseSocket();
			CloseConnection();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		case ETcpIpDisconnecting:
			CloseSocket();
			CloseConnection();
			
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
		default:
			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
			break;
	}
}

void CTcpIpEngine::Write(const TDesC16& aMessage) {
	HBufC8* aData = HBufC8::NewLC(aMessage.Length());
	aData->Des().Copy(aMessage);

	Write(*aData);

	CleanupStack::PopAndDestroy();
}

void CTcpIpEngine::Write(const TDesC8& aMessage) {
	if(iEngineStatus == ETcpIpConnected) {
		iSocketWriter->IssueWrite(aMessage);
	}
}

void CTcpIpEngine::Read() {
	if (iEngineStatus == ETcpIpConnected && !iSocketReader->IsActive()) {
		iSocketReader->IssueRead();
	}
}

void CTcpIpEngine::DoCancel() {
	switch (iEngineStatus) {
		case ETcpIpConnecting:
		case ETcpIpConnected:
		case ETcpIpCarrierChangeReq:
			iTcpIpSocket.CancelAll();
			break;
		case ETcpIpSecureConnection:
			iSecureSocket->CancelAll();
			break;
		case ETcpIpLookingUp:
			iResolver.Cancel();
			break;
		default:;
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
				Connect(aSockAddr);
			}
			else {
				CloseConnection();

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
			}
			else {
				CloseSocket();
				CloseConnection();

				iEngineStatus = ETcpIpDisconnected;
				iEngineObserver->Error(ETcpIpConnectFailed);
				iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Connect Error"), iStatus.Int());
			}
			break;
		case ETcpIpSecureConnection:
			if(iStatus == KErrNone) {
				iSocketReader->SecureSocket(iSecureSocket);
				iSocketWriter->SecureSocket(iSecureSocket);
				
				iEngineStatus = ETcpIpConnected;
				iEngineObserver->NotifyEvent(ETcpIpSecureConnection);
			}
			else {
				CloseSocket();
				CloseConnection();

				iEngineStatus = ETcpIpDisconnected;
				iEngineObserver->Error(ETcpIpSecureFailed);
				iEngineObserver->TcpIpDebug(_L8("TCP   CSecureSocket::StartClientHandshake Error"), iStatus.Int());
			}			
			break;
		case ETcpIpCarrierChanging:
			CloseSocket();
			
#ifdef __3_2_ONWARDS__
			// Migrate to new carrier
			iEngineObserver->TcpIpDebug(_L8("TCP   MigrateToPreferredCarrier"));
			iMobility->MigrateToPreferredCarrier();
#endif
			break;
		case ETcpIpDisconnecting:
			CloseSocket();
			CloseConnection();

			iEngineStatus = ETcpIpDisconnected;
			iEngineObserver->NotifyEvent(iEngineStatus);
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
			Connect(aSockAddr);
			
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
-- CSocketReader
--
----------------------------------------------------------------------------
*/

CSocketReader::CSocketReader() : CActive(EPriorityStandard) {
}

CSocketReader* CSocketReader::NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CSocketReader* self = NewLC(aTcpIpSocket, aEngineObserver);
	CleanupStack::Pop();
	return self;
}

CSocketReader* CSocketReader::NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CSocketReader* self = new(ELeave) CSocketReader;
	CleanupStack::PushL(self);
	self->ConstructL(aTcpIpSocket, aEngineObserver);
	return self;
}

void CSocketReader::ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	iTcpIpSocket = aTcpIpSocket;
	iSecureSocket = NULL;

	iEngineObserver = aEngineObserver;
	
	CActiveScheduler::Add(this);
}

CSocketReader::~CSocketReader() {
	CancelRead();
}

void CSocketReader::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

void CSocketReader::SecureSocket(CSecureSocket* aSecureSocket) {
	iSecureSocket = aSecureSocket;
}

void CSocketReader::IssueRead() {
	iAllowRead = true;
	
	Read();
}

void CSocketReader::CancelRead() {
	iAllowRead = false;
	iErrorCount = 0;
	
	Cancel();
}

void CSocketReader::Read() {	
	if(iAllowRead && !IsActive()) {
		iBuffer.Zero();
		
		if(iSecureSocket) {
			iSecureSocket->RecvOneOrMore(iBuffer, iStatus, iReadLength);
		}
		else {
			iTcpIpSocket->RecvOneOrMore(iBuffer, 0, iStatus, iReadLength);
		}
		
		SetActive();
	}
}

void CSocketReader::DoCancel() {
	if(iSecureSocket) {
		iSecureSocket->CancelRecv();
	}
	else {
		iTcpIpSocket->CancelRecv();
	}
}

void CSocketReader::RunL() {
	switch(iStatus.Int()) {
		case KErrNone:			
			iEngineObserver->DataRead(iBuffer);
			
			iErrorCount = 0;
			
			Read();
			break;
		case KErrEof:
			iErrorCount++;
			
			if(iErrorCount < 10) {
				Read();
			}
			else {
				iEngineObserver->TcpIpDebug(_L8("TCP   RSocket::Recv EOF"), iStatus.Int());
				iEngineObserver->Error(ETcpIpReadWriteError);
			}
			break;
		case KErrTimedOut:
			Read();
			break;
		default:;			
	}
}

/*
----------------------------------------------------------------------------
--
-- CSocketWriter
--
----------------------------------------------------------------------------
*/

CSocketWriter::CSocketWriter() : CActive(EPriorityStandard) {
}

CSocketWriter* CSocketWriter::NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CSocketWriter* self = NewLC(aTcpIpSocket, aEngineObserver);
	CleanupStack::Pop();
	return self;
}

CSocketWriter* CSocketWriter::NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	CSocketWriter* self = new(ELeave) CSocketWriter;
	CleanupStack::PushL(self);
	self->ConstructL(aTcpIpSocket, aEngineObserver);
	return self;
}

void CSocketWriter::ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver) {
	iTcpIpSocket = aTcpIpSocket;
	iSecureSocket = NULL;
	
	iEngineObserver = aEngineObserver;

	iSendingBuffer = HBufC8::NewL(256);
	iWaitingBuffer = HBufC8::NewL(256);

	CActiveScheduler::Add(this);
}

CSocketWriter::~CSocketWriter() {
	Cancel();

	if(iWaitingBuffer)
		delete iWaitingBuffer;

	if(iSendingBuffer)
		delete iSendingBuffer;
}

void CSocketWriter::SetObserver(MTcpIpEngineNotification* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

void CSocketWriter::SecureSocket(CSecureSocket* aSecureSocket) {
	iSecureSocket = aSecureSocket;
}

void CSocketWriter::IssueWrite(const TDesC8& aMessage) {
	TPtr8 pWaitingBuffer(iWaitingBuffer->Des());

	// Buffer Waiting Data
	if((pWaitingBuffer.Length() + aMessage.Length()) > pWaitingBuffer.MaxLength()) {
		iWaitingBuffer = iWaitingBuffer->ReAlloc(pWaitingBuffer.MaxLength() + aMessage.Length());
		pWaitingBuffer.Set(iWaitingBuffer->Des());
	}

	pWaitingBuffer.Append(aMessage);

	WriteBuffered();
}

void CSocketWriter::Reset() {
	TPtr8 pWaitingBuffer(iWaitingBuffer->Des());
	pWaitingBuffer.Zero();
}

TBool CSocketWriter::WriteBuffered() {
	TPtr8 pWaitingBuffer(iWaitingBuffer->Des());
	TPtr8 pSendingBuffer(iSendingBuffer->Des());
	TInt aLength = pWaitingBuffer.Length();

	if(!IsActive() && aLength > 0) {
		if(aLength >= pSendingBuffer.MaxLength()) {
			// Send next block of bytes
			aLength = pSendingBuffer.MaxLength();
		}

		// Copy data block
		pSendingBuffer.Copy(pWaitingBuffer.Ptr(), aLength);

		// Notify Observer
		iEngineObserver->DataWritten(pSendingBuffer);

		// Write Buffered Data
		if(iSecureSocket) {
			iSecureSocket->Send(*iSendingBuffer, iStatus);
		}
		else {
			iTcpIpSocket->Write(*iSendingBuffer, iStatus);
		}
		
		SetActive();

		// Delete from waiting buffer
		pWaitingBuffer.Delete(0, aLength);

		return true;
	}
	else {
		return false;
	}
}

void CSocketWriter::DoCancel() {
	if(iSecureSocket) {
		iSecureSocket->CancelSend();
	}
	else {
		iTcpIpSocket->CancelWrite();
	}
}

void CSocketWriter::RunL() {
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
