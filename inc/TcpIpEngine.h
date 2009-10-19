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
#include <e32cons.h>
#include <in_sock.h>
#include <nifman.h>

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
	ETcpIpStart, ETcpIpLookingUp, ETcpIpConnecting, ETcpIpConnected, ETcpIpCarrierChangeReq, ETcpIpCarrierChanging,
	ETcpIpWriteComplete, ETcpIpDisconnecting, ETcpIpDisconnected
};

enum TTcpIpEngineError {
	ETcpIpAlreadyBusy, ETcpIpCancelled, ETcpIpLookUpFailed,
	ETcpIpAccessPointFailed, ETcpIpConnectFailed, ETcpIpReadWriteError
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
		virtual void DataRead(const TDesC8& aMessage) = 0;
		virtual void DataWritten(const TDesC8& aMessage) = 0;

		virtual void NotifyEvent(TTcpIpEngineState aEngineState) = 0;
		virtual void Error(TTcpIpEngineError aError) = 0;

		virtual void TcpIpDebug(const TDesC8& aMessage, TInt aCode) = 0;
		virtual void TcpIpDebug(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CTcpIpRead
--
----------------------------------------------------------------------------
*/

class CTcpIpRead : public CActive {
	public:
		static CTcpIpRead* NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		static CTcpIpRead* NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		~CTcpIpRead();
		void ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);

		void SetObserver(MTcpIpEngineNotification* aEngineObserver);
		void IssueRead();

	public: //Implemented functions from CActive
		void DoCancel();
		void RunL();

	protected:
		CTcpIpRead();

	private:
		MTcpIpEngineNotification* iEngineObserver;
		RSocket* iTcpIpSocket;
		TBuf8<32> iBuffer;
		TSockXfrLength iReadLength;
};

/*
----------------------------------------------------------------------------
--
-- CTcpIpWrite
--
----------------------------------------------------------------------------
*/

class CTcpIpWrite : public CActive/*, public MTimeoutNotification*/ {
	public:
		enum TWriteState {
			EWriteIdle, EWriteSending
		};

	public:
		static CTcpIpWrite* NewL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		static CTcpIpWrite* NewLC(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);
		~CTcpIpWrite();
		void ConstructL(RSocket* aTcpIpSocket, MTcpIpEngineNotification* aEngineObserver);

		void SetObserver(MTcpIpEngineNotification* aEngineObserver);
		void IssueWrite(const TDesC8& aMessage);
		
		void Reset();

	private:
		TBool WriteBuffered();

	public:	//Implemented functions from CActive
		void DoCancel();
		void RunL();

	protected:
		CTcpIpWrite();

	private:
		TWriteState iWriteStatus;
		MTcpIpEngineNotification* iEngineObserver;
		RSocket* iTcpIpSocket;

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
	public:
		CTcpIpEngine();
		static CTcpIpEngine* NewL(MTcpIpEngineNotification* aEngineObserver);
		static CTcpIpEngine* NewLC(MTcpIpEngineNotification* aEngineObserver);
		~CTcpIpEngine();
		void ConstructL(MTcpIpEngineNotification* aEngineObserver);

		void SetObserver(MTcpIpEngineNotification* aEngineObserver);

	private: // Connection
#ifdef __3_2_ONWARDS__
		TBool SelectConnectionPrefL();
#endif
		void OpenConnectionL();
		void CloseConnectionL();
		void ConnectL(TSockAddr aAddr);
		
	public:
		void GetConnectionMode(TInt& aMode, TInt& aId);
		void SetConnectionMode(TInt aMode, TInt aId, const TDesC& aName);
		TDesC& GetConnectionName();
	
	private:
		void GetConnectionInformationL();

	public:
		void ConnectL(const TDesC& aServerName, TInt aPort);
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
		CTcpIpRead* iTcpIpRead;
		CTcpIpWrite* iTcpIpWrite;
		RSocket iTcpIpSocket;
		RConnection iConnection;
#ifdef __3_2_ONWARDS__
		CActiveCommsMobilityApiExt* iMobility;
#endif

		RSocketServ iSocketServ;
		RHostResolver iResolver;
		TNameEntry iNameEntry;

		HBufC* iSockHost;
		TInt iSockPort;

		TTcpIpEngineConnectionMode iConnectionMode;
		TUint32 iConnectionModeId;		
		HBufC* iConnectionName;
};

#endif // TCPIPENGINE_H_
