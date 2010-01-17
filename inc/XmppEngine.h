/*
============================================================================
 Name        : 	XmppEngine.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	XMPP Engine for talking to an XMPP protocol server
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPENGINE_H_
#define XMPPENGINE_H_

// INCLUDES
#include <e32base.h>
#include "TcpIpEngine.h"
#include "XmlParser.h"
#include "Timer.h"
#include "ConnectionMonitor.h"
#include "CompressionEngine.h"
#include "XmppInterfaces.h"
#include "XmppOutbox.h"

/*
----------------------------------------------------------------------------
--
-- Constants
--
----------------------------------------------------------------------------
*/

_LIT(KXmppTLSProtocol, "TLS1.0");

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TXmppEngineState {
	EXmppShutdown, EXmppDisconnected, EXmppRegistered, EXmppReset, EXmppReconnect,
	EXmppInitialize, EXmppConnecting, EXmppLoggingIn, EXmppOnline
};

enum TXmppEngineError {
	EXmppNone, EXmppAuthorizationNotDefined, EXmppBadAuthorization, EXmppAlreadyConnected, 
	EXmppTlsFailed, EXmppServerUnresolved
};

enum TXmppSilenceState {
	EXmppSilenceTest, EXmppPingSent, EXmppReconnectRequired
};

enum TXmppTimers {
	EXmppStateTimer, EXmppReadTimer
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MXmppEngineObserver {
	public:
		virtual void XmppStanzaRead(const TDesC8& aMessage) = 0;
		virtual void XmppStanzaWritten(const TDesC8& aMessage) = 0;
		virtual void XmppStateChanged(TXmppEngineState aState) = 0;
		virtual void XmppUnhandledStanza(const TDesC8& aStanza) = 0;
		virtual void XmppError(TXmppEngineError aError) = 0;
		virtual void XmppShutdownComplete() = 0;

		virtual void XmppDebug(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CXmppObservedStanza
--
----------------------------------------------------------------------------
*/

class CXmppObservedStanza : public CBase {
	public:
		static CXmppObservedStanza* NewL(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver);
		static CXmppObservedStanza* NewLC(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver);
		~CXmppObservedStanza();

	private:
		CXmppObservedStanza(MXmppStanzaObserver* aObserver);
		void ConstructL(const TDesC8& aStanzaId);
		
	public:
		TDesC8& GetStanzaId();
		MXmppStanzaObserver* GetStanzaObserver();
		
	protected:
		MXmppStanzaObserver* iObserver;
		HBufC8* iStanzaId;
};

/*
----------------------------------------------------------------------------
--
-- CXmppEngine
--
----------------------------------------------------------------------------
*/

class CXmppEngine : public CBase, MTcpIpEngineNotification, MConnectionMonitorNotification,
								MTimeoutNotification, MCompressionObserver, 
								MXmppStanzaObserver, MXmppWriteInterface {

	public:
		static CXmppEngine* NewL(MXmppEngineObserver* aEngineObserver);
		static CXmppEngine* NewLC(MXmppEngineObserver* aEngineObserver);
		~CXmppEngine();

	private:
		CXmppEngine();
		void ConstructL(MXmppEngineObserver* aEngineObserver);

	public: // Observers
		void SetObserver(MXmppEngineObserver* aEngineObserver);
		void AddRosterObserver(MXmppRosterObserver* aRosterObserver);

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	public: // Connection & Setup
		void SetAccountDetailsL(TDesC& aUsername, TDesC& aPassword);
		
		void SetServerDetailsL(const TDesC& aHostName, TInt aPort = 5222);
		void GetServerDetails(TDes& aHost);
		
		void SetResourceL(const TDesC8& aResource);

		void ConnectL();
		void SendAuthorization();
		void Disconnect();
		
		void GetConnectionMode(TInt& aMode, TInt& aId);
		void SetConnectionMode(TInt aMode, TInt aId, const TDesC& aName);
		TDesC& GetConnectionName();
		void GetConnectionStatistics(TInt& aDataSent, TInt& aDataReceived);

		void PrepareShutdown();
		
	private:
		void SetXmppServerL(const TDesC8& aXmppServer);
		void ConnectOrResolveL();
		
	public: // Stanza handling
		void SendQueuedXmppStanzas();
		CXmppOutbox* GetMessageOutbox();
		
	private:
		void SendNextQueuedStanzaL();
		
	private:
		void AddStanzaObserverL(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver);

	private:
		void OpenStream();
		void QueryServerTimeL();
		void WriteToStreamL(const TDesC8& aData);
		void ReadFromStreamL(const TDesC8& aData);
		void ProcessStanzaInBufferL(TInt aLength);
		void HandleXmppStanzaL(const TDesC8& aStanza);

	public: // From MXmppWriteInterface
		void SendAndForgetXmppStanza(const TDesC8& aStanza, TBool aPersisant, TXmppMessagePriority aPriority = EXmppPriorityNormal);
		void SendAndAcknowledgeXmppStanza(const TDesC8& aStanza, MXmppStanzaObserver* aObserver, TBool aPersisant = false, TXmppMessagePriority aPriority = EXmppPriorityNormal);
		void CancelXmppStanzaAcknowledge(MXmppStanzaObserver* aObserver);
		
	public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);
		
	public: // From MCompressionObserver
		void DataDeflated(const TDesC8& aDeflatedData);
		void DataInflated(const TDesC8& aInflatedData);

	public: // From MTcpIpEngineNotification
		void HostResolved(const TDesC& aHostName, TInt aHostPort);
		void NotifyEvent(TTcpIpEngineState aEngineState);
		void Error(TTcpIpEngineError aError);

		void DataRead(const TDesC8& aMessage);
		void DataWritten(const TDesC8& aMessage);

		void TcpIpDebug(const TDesC8& aMessage, TInt aCode);
		void TcpIpDebug(const TDesC8& aMessage);

	public: // From MConnectionMonitorNotification
		void ConnectionCreated(TUint aConnectionId);
		void ConnectionDeleted(TUint aConnectionId);
		void ConnectionStatusChanged();

		void ConnectionMonitorDebug(const TDesC8& aMessage);

	private:
		MXmppEngineObserver* iEngineObserver;
		MXmppRosterObserver* iRosterObserver;

		CConnectionMonitor* iConnectionMonitor;
		TUint iConnectionId;

		TXmppEngineState iEngineState;
		TXmppEngineError iLastError;

		CCustomTimer* iStateTimer;
		CCustomTimer* iReadTimer;

		TInt iConnectionAttempts;
		TXmppSilenceState iSilenceState;

 		CTcpIpEngine* iTcpIpEngine;
		HBufC8* iInputBuffer;
		
		RPointerArray<CXmppObservedStanza> iStanzaObserverArray;

		CXmppOutbox* iXmppOutbox;
		TBool iSendQueuedMessages;

		HBufC8* iUsername;
		HBufC8* iPassword;

		HBufC* iHostName;
		TInt iHostPort;
		
		HBufC8* iXmppServer;
		HBufC8* iResource;

		TBool iNegotiatingSecureConnection;
		
		TBool iStreamCompressed;
		CCompressionEngine* iCompressionEngine;
		
		TInt iDataSent;
		TInt iDataReceived;
};

#endif // XMPPENGINE_H_
