/*
============================================================================
 Name        : 	TcpIpEngine.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	TCP/IP Engine for Asynchronous Reading and Writing from/to
 		a TCP/IP Socket.
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef TCPIPENGINE_H_
#define TCPIPENGINE_H_

#include <blddef.h>
#include <dns_qry.h>
#include <e32cons.h>
#include <in_sock.h>
#include <nifman.h>
#include <securesocket.h>

#ifdef __3_2_ONWARDS__
#include <comms-infras/cs_mobility_apiext.h>
#endif

#include "Timer.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TTcpIpEngineState {
	ETcpIpStartResolve, ETcpIpStartConnection, ETcpIpLookUpHostName, ETcpIpLookUpAddress, ETcpIpConnecting, 
	ETcpIpConnected, ETcpIpSecureConnection, ETcpIpCarrierChangeReq, ETcpIpCarrierChanging, ETcpIpWriteComplete, 
	ETcpIpDisconnecting, ETcpIpDisconnected
};

enum TTcpIpEngineError {
	ETcpIpAlreadyBusy, ETcpIpCancelled, ETcpIpAccessPointFailed, ETcpIpHostNameLookUpFailed, 
	ETcpIpAddressLookUpFailed, ETcpIpDnsTimeout, ETcpIpConnectFailed, ETcpIpSecureFailed, ETcpIpReadWriteError
};

enum TTcpIpEngineConnectionMode {
	ETcpIpModeUnknown, ETcpIpModeDestinations, ETcpIpModeSelectedIap, ETcpIpModeUseDefault
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MTcpIpEngineNotification {
	public:
		virtual void HostResolved(const TDesC& aHostName, TInt aHostPort) = 0;
		virtual void NotifyEvent(TTcpIpEngineState aEngineState) = 0;
		virtual void Error(TTcpIpEngineError aError) = 0;
		
		virtual void DataRead(const TDesC8& aMessage) = 0;
		virtual void DataWritten(const TDesC8& aMessage) = 0;

		virtual void TcpIpDebug(const TDesC8& aMessage, TInt aCode) = 0;
		virtual void TcpIpDebug(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CSocketReader
--
----------------------------------------------------------------------------
*/

class CSocketReader : public CActive {
	protected:
		CSocketReader();
	
	public:
		static CSocketReader* NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		static CSocketReader* NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		
		void ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		~CSocketReader();

	public:
		void SetObserver(MTcpIpEngineNotification* aEngineObserver);
		void SecureSocket(CSecureSocket* aSecureSocket);
		
		void IssueRead();
		void CancelRead();
		
	private:
		void Read();

	public: //Implemented functions from CActive
		void DoCancel();
		void RunL();

	private:
		TInt iErrorCount;
		TBool iAllowRead;
		
		MTcpIpEngineNotification* iEngineObserver;
		
		RSocket* iTcpIpSocket;
		CSecureSocket* iSecureSocket;
		
		TBuf8<32> iBuffer;
		TSockXfrLength iReadLength;
};

/*
----------------------------------------------------------------------------
--
-- CSocketWriter
--
----------------------------------------------------------------------------
*/

class CSocketWriter : public CActive/*, public MTimeoutNotification*/ {
	public:
		enum TWriteState {
			EWriteIdle, EWriteSending
		};

	protected:
		CSocketWriter();

	public:
		static CSocketWriter* NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		static CSocketWriter* NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		
		void ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		~CSocketWriter();

	public:
		void SetObserver(MTcpIpEngineNotification* aEngineObserver);
		void SecureSocket(CSecureSocket* aSecureSocket);
		
		void IssueWrite(const TDesC8& aMessage);	
		void Reset();

	private:
		TBool WriteBuffered();

	public:	//Implemented functions from CActive
		void DoCancel();
		void RunL();

	private:
		TWriteState iWriteStatus;
		MTcpIpEngineNotification* iEngineObserver;
		
		RSocket* iTcpIpSocket;
		CSecureSocket* iSecureSocket;

		HBufC8* iWaitingBuffer;
		HBufC8* iSendingBuffer;
};

/*
----------------------------------------------------------------------------
--
-- CTcpIpEngine
--
----------------------------------------------------------------------------
*/

#ifdef __3_2_ONWARDS__
class CTcpIpEngine : public CActive, MMobilityProtocolResp {
#else
class CTcpIpEngine : public CActive {
#endif
	protected:
		CTcpIpEngine();

	public:
		static CTcpIpEngine* NewL(MTcpIpEngineNotification* aEngineObserver);
		static CTcpIpEngine* NewLC(MTcpIpEngineNotification* aEngineObserver);
		
		void ConstructL(MTcpIpEngineNotification* aEngineObserver);
		~CTcpIpEngine();
	
	public:
		void SetObserver(MTcpIpEngineNotification* aEngineObserver);

	private: // Connection
#ifdef __3_2_ONWARDS__
		TBool SelectConnectionPrefL();
#endif
		void OpenConnectionL();
		void CloseConnection();
		void CloseSocket();
		
		void Connect(TSockAddr aAddr);
		
	public:
		void GetConnectionMode(TInt& aMode, TInt& aId);
		void SetConnectionMode(TInt aMode, TInt aId, const TDesC& aName);
		TDesC& GetConnectionName();
	
	private:
		void GetConnectionInformationL();

	public:
		void ResolveHostNameL(const TDesC8& aDataQuery);
		
		void ConnectL(const TDesC& aServerName, TInt aPort);
		void SecureConnectionL(const TDesC& aProtocol);
		
		void Disconnect();

	public: // Read/Write
		void Write(const TDesC16& aMessage);
		void Write(const TDesC8& aMessage);
		void Read();

	public:	//Implemented functions from CActive
		void DoCancel();
		void RunL();
		
#ifdef __3_2_ONWARDS__
	public: // From MMobilityProtocolResp
		void PreferredCarrierAvailable(TAccessPointInfo aOldAP, TAccessPointInfo aNewAP, TBool aIsUpgrade, TBool aIsSeamless);
		void NewCarrierActive(TAccessPointInfo aNewAP, TBool aIsSeamless);
		void Error(TInt aError);
#endif

	private:
		TTcpIpEngineState iEngineStatus;
		MTcpIpEngineNotification* iEngineObserver;
		CSocketReader* iSocketReader;
		CSocketWriter* iSocketWriter;
		
		RSocket iTcpIpSocket;
		CSecureSocket* iSecureSocket;
		
		RConnection iConnection;
#ifdef __3_2_ONWARDS__
		CActiveCommsMobilityApiExt* iMobility;
#endif

		RSocketServ iSocketServ;
		RHostResolver iResolver;
		TNameEntry iNameEntry;
		TDnsQueryBuf iDnsQuery;
		TDnsRespSRVBuf iDnsResponse;

		HBufC* iSocketHost;
		TInt iSocketPort;

		TTcpIpEngineConnectionMode iConnectionMode;
		TUint32 iConnectionModeId;		
		HBufC* iConnectionName;
};

#endif // TCPIPENGINE_H_
