/*
============================================================================
 Name        : 	XmppEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	XMPP Engine for talking to an XMPP protocol server
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <e32math.h>
#include <imcvcodc.h>
#include "Md5Wrapper.h"
#include "TextUtilities.h"
#include "XmppEngine.h"

/*
----------------------------------------------------------------------------
--
-- CXmppObservedStanza
--
----------------------------------------------------------------------------
*/

CXmppObservedStanza* CXmppObservedStanza::NewL(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver) {
	CXmppObservedStanza* self = NewLC(aStanzaId, aObserver);
	CleanupStack::Pop(self);
	return self;
}

CXmppObservedStanza* CXmppObservedStanza::NewLC(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver) {
	CXmppObservedStanza* self = new (ELeave) CXmppObservedStanza(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL(aStanzaId);
	return self;
}

CXmppObservedStanza::~CXmppObservedStanza() {
	if(iStanzaId) {
		delete iStanzaId;
	}
}

CXmppObservedStanza::CXmppObservedStanza(MXmppStanzaObserver* aObserver) {
	iObserver = aObserver;
}

void CXmppObservedStanza::ConstructL(const TDesC8& aStanzaId) {
	iStanzaId = aStanzaId.AllocL();
}

TDesC8& CXmppObservedStanza::GetStanzaId() {
	return *iStanzaId;
}

MXmppStanzaObserver* CXmppObservedStanza::GetStanzaObserver() {
	return iObserver;
}

/*
----------------------------------------------------------------------------
--
-- CXmppEngine
--
----------------------------------------------------------------------------
*/

CXmppEngine* CXmppEngine::NewL(MXmppEngineObserver* aEngineObserver) {
	CXmppEngine* self = NewLC(aEngineObserver);
	CleanupStack::Pop(self);
	return self;

}

CXmppEngine* CXmppEngine::NewLC(MXmppEngineObserver* aEngineObserver) {
	CXmppEngine* self = new (ELeave) CXmppEngine();
	CleanupStack::PushL(self);
	self->ConstructL(aEngineObserver);
	return self;
}

CXmppEngine::~CXmppEngine() {
	if(iStateTimer)
		delete iStateTimer;

	if(iReadTimer)
		delete iReadTimer;

	if(iTcpIpEngine)
		delete iTcpIpEngine;

	if(iInputBuffer)
		delete iInputBuffer;
	
	for(TInt i = 0;i < iStanzaObserverArray.Count();i++) {
		delete iStanzaObserverArray[i];
	}

	iStanzaObserverArray.Close();

	if(iXmppOutbox)
		delete iXmppOutbox;

	if(iUsername)
		delete iUsername;
	if(iPassword)
		delete iPassword;
	if(iHostName)
		delete iHostName;
	if(iXmppServer)
		delete iXmppServer;
	if(iResource)
		delete iResource;

	if(iConnectionMonitor)
		delete iConnectionMonitor;

	if(iCompressionEngine)
		delete iCompressionEngine;
}

CXmppEngine::CXmppEngine() {
}

void CXmppEngine::ConstructL(MXmppEngineObserver* aEngineObserver) {
	iEngineObserver = aEngineObserver;

	iEngineState = EXmppDisconnected;
	iLastError = EXmppNone;
	iConnectionAttempts = 0;

	iCompressionEngine = CCompressionEngine::NewL(this);

	iConnectionMonitor = CConnectionMonitor::NewL(this);
	iConnectionId = 0;

	iStateTimer = CCustomTimer::NewL(this, EXmppStateTimer);
	iReadTimer = CCustomTimer::NewL(this, EXmppReadTimer);

	iTcpIpEngine = CTcpIpEngine::NewL(this);

	iInputBuffer = HBufC8::NewL(256);

	iXmppOutbox = CXmppOutbox::NewL();

	iUsername = HBufC8::NewL(32);
	iPassword = HBufC8::NewL(32);
	
	iHostName = HBufC::NewL(32);
	iXmppServer = HBufC8::NewL(32);
	iResource = HBufC8::NewL(32);
}

void CXmppEngine::SetObserver(MXmppEngineObserver* aEngineObserver) {
	iEngineObserver = aEngineObserver;
}

void CXmppEngine::TimerExpired(TInt aExpiryId) {
	if(aExpiryId == EXmppReadTimer) {
		// Read timer timeout
		iTcpIpEngine->Read();
	}
	else if(aExpiryId == EXmppStateTimer) {
		// State timer timeout
		switch(iEngineState) {
			case EXmppReconnect:
				iEngineState = EXmppInitialize;
				iStateTimer->After(45000000);
	
				// Connect or resolve
				ConnectOrResolveL();
				break;
			case EXmppInitialize:
			case EXmppConnecting:
			case EXmppLoggingIn:
				iTcpIpEngine->Disconnect();
				break;
			case EXmppOnline:
				// Connection exists
				if(iSilenceState == EXmppSilenceTest) {				
					// Send a ping to the server
					if(iConnectionMonitor->ConnectionBearerMobile(iConnectionId)) {
						if(iConnectionMonitor->GetNetworkStatus() == EConnMonStatusSuspended) {
							iSilenceState = EXmppReconnectRequired;
						}
					}
					
					if(iSilenceState == EXmppSilenceTest) {
						// Reset Silence Test
						iStateTimer->After(30000000);
						iSilenceState = EXmppPingSent;
		
						// Send 'Ping'
						QueryServerTimeL();
					}
				}
				else if(iSilenceState == EXmppPingSent) {
					// Reconnect to server
					iSilenceState = EXmppReconnectRequired;
	
					if(iConnectionMonitor->ConnectionBearerMobile(iConnectionId)) {
						// Mobile
						if(iConnectionMonitor->GetNetworkStatus() != EConnMonStatusSuspended) {
							iEngineState = EXmppReconnect;
							iTcpIpEngine->Disconnect();
						}
					}
					else {
						// Other
						iEngineState = EXmppReconnect;
						iTcpIpEngine->Disconnect();
					}
				}
				break;
			default:;
		}
	}
}

void CXmppEngine::AddRosterObserver(MXmppRosterObserver* aRosterObserver) {
	iRosterObserver = aRosterObserver;
}

void CXmppEngine::SetAccountDetailsL(TDesC& aUsername, TDesC& aPassword) {
#ifdef _DEBUG
	iEngineObserver->XmppDebug(_L8("XMPP  CXmppEngine::SetAccountDetailsL"));
#endif

	if(iUsername)
		delete iUsername;

	if(iPassword)
		delete iPassword;

	iUsername = HBufC8::NewL(aUsername.Length());
	TPtr8 pUsername(iUsername->Des());
	pUsername.Copy(aUsername);

	iPassword = HBufC8::NewL(aPassword.Length());
	iPassword->Des().Copy(aPassword);

	// Validate xmpp server domain
	TInt aLocate = pUsername.Locate('@');
	
	if(aLocate != KErrNotFound) {
		SetXmppServerL(pUsername.Mid(aLocate + 1));
		
		pUsername.Delete(aLocate, pUsername.Length());
	}	
}

void CXmppEngine::SetServerDetailsL(const TDesC& aHostName, TInt aPort) {
	iHostPort = aPort;
	
	if(iHostName)
		delete iHostName;

	iHostName = aHostName.AllocL();	
	TPtr pHostName(iHostName->Des());
	
	// Set port
	TInt aLocate = pHostName.Locate(':');
	
	if(aLocate != KErrNotFound) {
		TLex aPortLex(pHostName.Mid(aLocate + 1));		
		aPortLex.Val(iHostPort);
		
		pHostName.Delete(aLocate, pHostName.Length());
	}
}

void CXmppEngine::GetServerDetails(TDes& aHost) {
	aHost.Zero();
	aHost.Append(*iHostName);
	aHost.AppendFormat(_L(":%d"), iHostPort);
}

void CXmppEngine::SetResourceL(const TDesC8& aResource) {
	if(iResource)
		delete iResource;

	iResource = aResource.AllocL();
}

void CXmppEngine::ConnectL() {
#ifdef _DEBUG
	iEngineObserver->XmppDebug(_L8("XMPP  CXmppEngine::ConnectL"));
#endif
	
	TPtr8 aUsername(iUsername->Des());
	TPtr8 aPassword(iPassword->Des());

	if(aUsername.Length() > 0 && aPassword.Length() > 0) {
		iEngineState = EXmppInitialize;
		iLastError = EXmppNone;
		iConnectionAttempts = 0;
		
		// Connect or resolve
		ConnectOrResolveL();	
	}
	else {
		iEngineObserver->XmppError(EXmppAuthorizationNotDefined);
	}
}

void CXmppEngine::Disconnect() {
#ifdef _DEBUG
	iEngineObserver->XmppDebug(_L8("XMPP  CXmppEngine::Disconnect"));
#endif
	
	iEngineState = EXmppDisconnected;
	
	iStateTimer->Stop();
	iReadTimer->Stop();

	iEngineObserver->XmppStateChanged(EXmppDisconnected);
	iConnectionId = 0;

	iTcpIpEngine->Disconnect();
}

void CXmppEngine::PrepareShutdown() {
#ifdef _DEBUG
	iEngineObserver->XmppDebug(_L8("XMPP  CXmppEngine::PrepareShutdown"));
#endif

	if(iEngineState >= EXmppLoggingIn) {
		WriteToStreamL(_L8("</stream:stream>\r\n"));
	}

	iEngineState = EXmppShutdown;

	iTcpIpEngine->Disconnect();
}

void CXmppEngine::GetConnectionMode(TInt& aMode, TInt& aId) {
	iTcpIpEngine->GetConnectionMode(aMode, aId);
}

void CXmppEngine::SetConnectionMode(TInt aMode, TInt aId, const TDesC& aName) {
	iTcpIpEngine->SetConnectionMode(aMode, aId, aName);

	if(iEngineState >= EXmppReconnect) {
		iEngineState = EXmppReset;
		iConnectionId = 0;

		iTcpIpEngine->Disconnect();
	}
}

TDesC& CXmppEngine::GetConnectionName() {
	return iTcpIpEngine->GetConnectionName();
}

void CXmppEngine::GetConnectionStatistics(TInt& aDataSent, TInt& aDataReceived) {
	aDataSent = iDataSent;
	aDataReceived = iDataReceived;
}

void CXmppEngine::SetXmppServerL(const TDesC8& aXmppServer) {
	if(iXmppServer)
		delete iXmppServer;
	
	iXmppServer = aXmppServer.AllocL();
}

void CXmppEngine::ConnectOrResolveL() {
	TPtr aHostName(iHostName->Des());
	
	iConnectionAttempts++;
	TcpIpDebug(_L8("XMPP  Connection Attempt"), iConnectionAttempts);

	if(aHostName.Length() > 0) {
		// Connect to server
		iTcpIpEngine->ConnectL(aHostName, iHostPort);
	}
	else {
		// Server resolve required
		TBuf8<255> aQueryData(*iXmppServer);
		aQueryData.Insert(0, _L8("_xmpp-client._tcp."));
		
		iTcpIpEngine->ResolveHostNameL(aQueryData);
	}	
}

void CXmppEngine::SendQueuedXmppStanzas() {
	iSendQueuedMessages = true;
	
	SendNextQueuedStanzaL();
}

CXmppOutbox* CXmppEngine::GetMessageOutbox() {
	return iXmppOutbox;
}

void CXmppEngine::SendNextQueuedStanzaL() {
	iXmppOutbox->SetMessageSent(true);
	
	if((iEngineState == EXmppOnline || iLastError != EXmppNone) && iSilenceState == EXmppSilenceTest) {
		// Send next in outbox
		if(iSendQueuedMessages || iXmppOutbox->ContainsPriorityMessages(EXmppPriorityHigh)) {
			if(iXmppOutbox->Count() > 0) {
				WriteToStreamL(iXmppOutbox->GetNextMessage()->GetStanza());
			}			
		}
	}		
}

void CXmppEngine::AddStanzaObserverL(const TDesC8& aStanzaId, MXmppStanzaObserver* aObserver) {
	if(aStanzaId.Length() > 0) {
		iStanzaObserverArray.Append(CXmppObservedStanza::NewL(aStanzaId, aObserver));
	}	
}

void CXmppEngine::HandlePlainAuthorizationL() {
	TPtr8 pUsername(iUsername->Des());
	TPtr8 pPassword(iPassword->Des());
	TPtr8 pXmppServer(iXmppServer->Des());

	// Create source auth string
	HBufC8* aSrc = HBufC8::NewLC(pUsername.Length() + pXmppServer.Length() + pPassword.Length() + 3);
	TPtr8 pSrc(aSrc->Des());
	pSrc.Append(TChar(0));
	pSrc.Append(pUsername);
	
	pSrc.Append(TChar(0));
	pSrc.Append(pPassword);

	HBufC8* aDest = HBufC8::NewLC(pSrc.Length() * 2);
	TPtr8 pDest(aDest->Des());

	// Encode to Base64
	TImCodecB64 aBase64Encoder;	
	aBase64Encoder.Initialise();
	aBase64Encoder.Encode(pSrc, pDest);

	// Send auth
	_LIT8(KAuthStanza, "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'></auth>");
	HBufC8* aAuthStanza = HBufC8::NewLC(KAuthStanza().Length() + pDest.Length());
	TPtr8 pAuthStanza(aAuthStanza->Des());
	pAuthStanza.Copy(KAuthStanza);
	pAuthStanza.Insert(65, pDest);

	WriteToStreamL(pAuthStanza);
	CleanupStack::PopAndDestroy(3); // aAuthStanza, aDest, aSrc
}

void CXmppEngine::HandleDigestMd5AuthorizationL(const TDesC8& aChallenge) {
	HBufC8* aDecodedChallenge = HBufC8::NewLC(aChallenge.Length());
	TPtr8 pDecodedChallenge(aDecodedChallenge->Des());
	
	TImCodecB64 aBase64Encoder;		
	aBase64Encoder.Initialise();
	aBase64Encoder.Decode(aChallenge, pDecodedChallenge);
	
	TInt aFind = pDecodedChallenge.Find(_L8("rspauth="));
	
	if(aFind != KErrNotFound) {
		// Challenge successful
		WriteToStreamL(_L8("<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>"));
	}
	else {
		TPtrC8 aRealm, aNonce, aQop;
		
		// Realm
		if((aFind = pDecodedChallenge.Find(_L8("realm="))) != KErrNotFound) {
			aRealm.Set(pDecodedChallenge.Mid(aFind + 7));
			aRealm.Set(aRealm.Left(aRealm.Locate('\"')));
		}
		
		// Nonce
		if((aFind = pDecodedChallenge.Find(_L8("nonce="))) != KErrNotFound) {
			aNonce.Set(pDecodedChallenge.Mid(aFind + 7));
			aNonce.Set(aNonce.Left(aNonce.Locate('\"')));
		}
		
		// Qop
		if((aFind = pDecodedChallenge.Find(_L8("qop="))) != KErrNotFound) {
			aQop.Set(pDecodedChallenge.Mid(aFind + 5));
			aQop.Set(aQop.Left(aQop.Locate('\"')));
		}
		
		if(aNonce.Length() > 0 && aQop.Length() > 0) {
			_LIT8(KDigestDelimiter, ":");
			
			TBuf8<16> aNonceCount;
			aNonceCount.Format(_L8("%08X"), (iConnectionAttempts + 1));
			aNonceCount.LowerCase();
			
			TBuf8<32> aClientNonce;
			aClientNonce.Format(_L8("%X%X%X"), Math::Random(), Math::Random(), Math::Random());
			aClientNonce.LowerCase();
					
			// Begin hashing
			CMD5Wrapper* aHashDigestA1H = CMD5Wrapper::NewLC();
			aHashDigestA1H->Update(*iUsername);
			aHashDigestA1H->Update(KDigestDelimiter);
			aHashDigestA1H->Update(aRealm);
			aHashDigestA1H->Update(KDigestDelimiter);
			aHashDigestA1H->Update(*iPassword);
			
			CMD5Wrapper* aHashDigestA1 = CMD5Wrapper::NewLC();						
			aHashDigestA1->Update(aHashDigestA1H->Final());
			aHashDigestA1->Update(KDigestDelimiter);
			aHashDigestA1->Update(aNonce);
			aHashDigestA1->Update(KDigestDelimiter);
			aHashDigestA1->Update(aClientNonce);
			
			CMD5Wrapper* aHashDigestA2 = CMD5Wrapper::NewLC();
			aHashDigestA2->Update(_L8("AUTHENTICATE:xmpp/"));
			aHashDigestA2->Update(*iXmppServer);

			CMD5Wrapper* aFinalDigest = CMD5Wrapper::NewLC();		
			aFinalDigest->Update(aHashDigestA1->FinalHex());
			aFinalDigest->Update(KDigestDelimiter);
			aFinalDigest->Update(aNonce);
			aFinalDigest->Update(KDigestDelimiter);
			aFinalDigest->Update(aNonceCount);
			aFinalDigest->Update(KDigestDelimiter);
			aFinalDigest->Update(aClientNonce);
			aFinalDigest->Update(KDigestDelimiter);
			aFinalDigest->Update(aQop);
			aFinalDigest->Update(KDigestDelimiter);
			aFinalDigest->Update(aHashDigestA2->FinalHex());
			
			TPtrC8 aDigest(aFinalDigest->FinalHex());
			
			// Format response data
			_LIT8(KRealm, "realm=\"\",");
			_LIT8(KFormattedResponse, "username=\"\",nonce=\"\",cnonce=\"\",nc=,qop=,digest-uri=\"xmpp/\",response=,charset=utf-8");
			HBufC8* aResponseData = HBufC8::NewLC(KFormattedResponse().Length() + iUsername->Des().Length() + KRealm().Length() + aRealm.Length() + aNonce.Length() + aClientNonce.Length() + aNonceCount.Length() + aQop.Length() + iXmppServer->Des().Length() + aDigest.Length());
			TPtr8 pResponseData(aResponseData->Des());
			pResponseData.Append(KFormattedResponse);
			pResponseData.Insert(68, aDigest);
			pResponseData.Insert(57, *iXmppServer);
			pResponseData.Insert(39, aQop);
			pResponseData.Insert(34, aNonceCount);
			pResponseData.Insert(29, aClientNonce);
			pResponseData.Insert(19, aNonce);
			
			if(aRealm.Length() > 0) {
				pResponseData.Insert(12, KRealm);
				pResponseData.Insert(19, aRealm);
			}
			
			pResponseData.Insert(10, *iUsername);
			
			// Base64 encode response
			HBufC8* aEncodedResponse = HBufC8::NewLC(pResponseData.Length() * 2);
			TPtr8 pEncodedResponse(aEncodedResponse->Des());
			aBase64Encoder.Encode(pResponseData, pEncodedResponse);			
			
			// Send response stanza
			_LIT8(KResponseStanza, "<response xmlns='urn:ietf:params:xml:ns:xmpp-sasl'></response>");
			HBufC8* aResponseStanza = HBufC8::NewLC(KResponseStanza().Length() + aEncodedResponse->Des().Length());
			TPtr8 pResponseStanza(aResponseStanza->Des());
			pResponseStanza.Append(KResponseStanza);
			pResponseStanza.Insert(51, *aEncodedResponse);
			
			WriteToStreamL(pResponseStanza);
			
			CleanupStack::PopAndDestroy(7); // aResponseStanza, aEncodedResponse, aResponseData, aHashDigestA2, aHashDigestA1H, aHashDigestA1, aFinalDigest
		}
	}
	
	CleanupStack::PopAndDestroy(); // aDecodedChallenge
}

void CXmppEngine::OpenStream() {
	// Open stream
	TPtr8 pXmppServer(iXmppServer->Des());
	_LIT8(KStreamStanza, "<?xml version='1.0'?><stream:stream xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' to='' version='1.0'>");

	HBufC8* aStreamStanza = HBufC8::NewLC(KStreamStanza().Length() + pXmppServer.Length());
	TPtr8 pStreamStanza(aStreamStanza->Des());
	pStreamStanza.Copy(KStreamStanza);
	pStreamStanza.Insert(110, pXmppServer);

	WriteToStreamL(pStreamStanza);
	CleanupStack::PopAndDestroy();
}

void CXmppEngine::QueryServerTimeL() {
	WriteToStreamL(_L8("<iq to='buddycloud.com' type='get' id='time1'><time xmlns='urn:xmpp:time'/></iq>\r\n"));
}

void CXmppEngine::WriteToStreamL(const TDesC8& aData) {
	if(iStreamCompressed) {
		iEngineObserver->XmppStanzaWritten(aData);
		iCompressionEngine->DeflateL(aData);
	}
	else {
		iTcpIpEngine->Write(aData);
	}
}

void CXmppEngine::HandleXmppStanzaL(const TDesC8& aStanza) {
	TBool aStanzaIsProcessed = false;
	
	iStateTimer->Stop();

	// Initialize XML Parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeId = aXmlParser->GetStringAttribute(_L8("id"));

	// Debug of XML Stanza
	iEngineObserver->XmppStanzaRead(aStanza);
	
	// Handle stanza observers
	if(aAttributeId.Length() > 0) {
		for(TInt i = 0; i < iStanzaObserverArray.Count(); i++) {
			if(aAttributeId.Compare(iStanzaObserverArray[i]->GetStanzaId()) == 0) {
				if(iStanzaObserverArray[i]->GetStanzaObserver() != NULL) {
					aStanzaIsProcessed = true;
					
					// Forward to observer
					iStanzaObserverArray[i]->GetStanzaObserver()->XmppStanzaAcknowledgedL(aStanza, aAttributeId);
				}
				
				// Delete and remove
				delete iStanzaObserverArray[i];
				iStanzaObserverArray.Remove(i);
				
				break;
			}
		}
	}
	
	// Handle stream negotiation
	if( !aStanzaIsProcessed ) {
		TPtrC8 aElement = aXmlParser->GetElementName();

		if(aElement.Compare(_L8("/stream:stream")) == 0) {
			// End of stream
			iTcpIpEngine->Disconnect();
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("stream:features")) == 0) {
			iStreamFeatures = EStreamFeatureNone;
			
			// Get stream features
			while(aXmlParser->MoveToNextElement()) {
				TPtrC8 aFeatureName = aXmlParser->GetElementName();
				
				if(aFeatureName.Compare(_L8("starttls")) == 0) {
					iStreamFeatures |= EStreamFeatureTls;
					
					if(aXmlParser->GetStringData().Compare(_L8("<required/>")) == 0) {
						iStreamFeatures |= EStreamFeatureTlsRequired;
					}
				}
				else if(aXmlParser->MoveToElement(_L8("compression"))) {
					while(aXmlParser->MoveToElement(_L8("method"))) {
						TPtrC8 aDataMethod(aXmlParser->GetStringData());
		
						if(aDataMethod.Compare(_L8("zlib")) == 0) {
							iStreamFeatures |= EStreamFeatureCompression;
							break;
						}
					}
				}
				else if(aXmlParser->MoveToElement(_L8("mechanisms"))) {
					while(aXmlParser->MoveToElement(_L8("mechanism"))) {
						TPtrC8 aDataMechanism(aXmlParser->GetStringData());
		
						if(aDataMechanism.Compare(_L8("DIGEST-MD5")) == 0) {
							iStreamFeatures |= EStreamFeatureDigestMd5;
						}
						else if(aDataMechanism.Compare(_L8("PLAIN")) == 0) {
							iStreamFeatures |= EStreamFeaturePlain;
						}
						else if(aDataMechanism.Compare(_L8("ANONYMOUS")) == 0) {
							iStreamFeatures |= EStreamFeatureAnonymous;
						}
					}
				}
				else if(aXmlParser->MoveToElement(_L8("bind"))) {
					iStreamFeatures |= EStreamFeatureBind;
				}
				else if(aXmlParser->MoveToElement(_L8("register"))) {
					iStreamFeatures |= EStreamFeatureRegistration;
				}
			}
			
			// Process stream features
			if(iStreamFeatures & EStreamFeatureTlsRequired) {
				// TLS negotiation required
				iNegotiatingSecureConnection = false;
				WriteToStreamL(_L8("<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>"));
			}
			else if(iStreamFeatures & EStreamFeatureCompression) {
				// Request compression
				WriteToStreamL(_L8("<compress xmlns='http://jabber.org/protocol/compress'><method>zlib</method></compress>\r\n"));
			}
			else if(iStreamFeatures & EStreamFeatureDigestMd5) {
				// Select DIGEST-MD5 authorization
				WriteToStreamL(_L8("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='DIGEST-MD5'/>\r\n"));
			}
			else if(iStreamFeatures & EStreamFeaturePlain) {
				// Select PLAIN authorization
				HandlePlainAuthorizationL();
			}
			else if(iStreamFeatures & EStreamFeatureBind) {
				// Bind resource
				TPtr8 aResource(iResource->Des());
				_LIT8(KBindStanza, "<iq type='set' id='bind1'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource></resource></bind></iq>\r\n");
	
				HBufC8* aBindStanza = HBufC8::NewLC(KBindStanza().Length() + aResource.Length());
				TPtr8 pBindStanza(aBindStanza->Des());
				pBindStanza.Copy(KBindStanza);
				pBindStanza.Insert(83, aResource);
	
				AddStanzaObserverL(_L8("bind1"), this);
				WriteToStreamL(pBindStanza);
				CleanupStack::PopAndDestroy();
			}
			else if(iStreamFeatures & EStreamFeatureTls) {
				// Start TLS negotiation
				iNegotiatingSecureConnection = false;
				WriteToStreamL(_L8("<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>"));
			}
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("proceed")) == 0) {
			// Secure the connection
			iTcpIpEngine->SecureConnectionL(KXmppTLSProtocol);
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("compressed")) == 0) {
			// Start compressed stream
			iStreamCompressed = true;

			OpenStream();
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("challenge")) == 0) {
			// Handle authorization challenge
			HandleDigestMd5AuthorizationL(aXmlParser->GetStringData());
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("success")) == 0) {
			// Authorization success
			iLastError = EXmppNone;
			OpenStream();
			
			aStanzaIsProcessed = true;
		}
		else if(aElement.Compare(_L8("failure")) == 0) {
			// Authorization failed
			TPtr8 aUsername(iUsername->Des());
			TInt aLocate = aUsername.Locate('@');
			
			if(aLocate == KErrNotFound && iStreamFeatures & EStreamFeaturePlain) {
				// Add xmpp server to username
				aUsername.Append(_L8("@"));
				aUsername.Append(*iXmppServer);

				// Select PLAIN authorization
				HandlePlainAuthorizationL();
			}
			else {		
				TPtrC8 aAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));
				
				if(aAttributeXmlns.Compare(_L8("urn:ietf:params:xml:ns:xmpp-tls")) == 0) {
					// TLS negotiation failure
					iLastError = EXmppTlsFailed;
				}
				else {
					// Authorization failure
					iLastError = EXmppBadAuthorization;
					iEngineObserver->XmppError(iLastError);
				}
			}
			
			aStanzaIsProcessed = true;
		}
	}
	
	// Pass to observer if still unprocessed
	if(!aStanzaIsProcessed) {
		iEngineObserver->XmppUnhandledStanza(aStanza);
	}

	CleanupStack::PopAndDestroy(); // aXmlParser
	
	// Send next queued message
	if(iSilenceState != EXmppSilenceTest && !iXmppOutbox->SendInProgress()) {
		SendNextQueuedStanzaL();
	}

	// Reset Silence Test
	iStateTimer->After(90000000);
	iSilenceState = EXmppSilenceTest;
}

void CXmppEngine::SendAndForgetXmppStanza(const TDesC8& aStanza, TBool aPersisant, TXmppMessagePriority aPriority) {
	CXmppOutboxMessage* aMessage = CXmppOutboxMessage::NewLC();
	aMessage->SetStanzaL(aStanza);
	aMessage->SetPersistance(aPersisant);
	aMessage->SetPriority(aPriority);
	
	iXmppOutbox->AddMessage(aMessage);
	CleanupStack::Pop(); // aMessage
	
	if(iEngineState > EXmppConnecting && iSilenceState == EXmppSilenceTest && !iXmppOutbox->SendInProgress()) {
		if(iSendQueuedMessages || iXmppOutbox->ContainsPriorityMessages(EXmppPriorityHigh)) {			
			WriteToStreamL(iXmppOutbox->GetNextMessage()->GetStanza());
		}
	}
}

void CXmppEngine::SendAndAcknowledgeXmppStanza(const TDesC8& aStanza, MXmppStanzaObserver* aObserver, TBool aPersisant, TXmppMessagePriority aPriority) {
	if(aPersisant || aPriority == EXmppPriorityHigh || (!aPersisant && (iEngineState > EXmppConnecting || iLastError != EXmppNone))) {
		// Add observer to observing list
		CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
		TPtrC8 aStanzaId = aXmlParser->GetStringAttribute(_L8("id"));
		
		AddStanzaObserverL(aStanzaId, aObserver);
		CleanupStack::PopAndDestroy(); // aXmlParser
		
		// Send xmpp stanza
		SendAndForgetXmppStanza(aStanza, aPersisant, aPriority);
	}
}

void CXmppEngine::CancelXmppStanzaAcknowledge(MXmppStanzaObserver* aObserver) {
	for(TInt i = iStanzaObserverArray.Count() - 1; i >= 0; i--) {
		if(iStanzaObserverArray[i]->GetStanzaObserver() == aObserver) {
			delete iStanzaObserverArray[i];
			iStanzaObserverArray.Remove(i);
		}
	}
}

void CXmppEngine::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aId.Compare(_L8("bind1")) == 0) {
		// Send session
		_LIT8(KSessionStanza, "<iq type='set' id='sess1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>\r\n");
			
		AddStanzaObserverL(_L8("sess1"), this);
		WriteToStreamL(KSessionStanza);
	}
	else if(aId.Compare(_L8("sess1")) == 0) {
		// Established Session
		QueryServerTimeL();		
		
		// Request Roster
		_LIT8(KRosterStanza, "<iq type='get' id='roster1'><query xmlns='jabber:iq:roster'/></iq>\r\n");
		
		AddStanzaObserverL(_L8("roster1"), this);
		WriteToStreamL(KRosterStanza);
	}
	else if(aId.Compare(_L8("roster1")) == 0) {
		// Receive Roster				
		if(iRosterObserver) {
			if(aAttributeType.Compare(_L8("result")) == 0) {
				iRosterObserver->XmppRosterL(aStanza);
			}
		}

		// Online
		iEngineState = EXmppOnline;
		iEngineObserver->XmppStateChanged(EXmppOnline);
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CXmppEngine::DataDeflated(const TDesC8& aDeflatedData) {
	if(iStreamCompressed) {
		iTcpIpEngine->Write(aDeflatedData);
	}
}

void CXmppEngine::DataInflated(const TDesC8& aInflatedData) {
	ReadFromStreamL(aInflatedData);
}

void CXmppEngine::DataRead(const TDesC8& aMessage) {
	iDataReceived += aMessage.Length();
	
	// Handle data received
	if(iStreamCompressed) {
		iCompressionEngine->InflateL(aMessage);
	}
	else {
		ReadFromStreamL(aMessage);
	}
	
	// Set timeout for next read
	iReadTimer->After(READ_INTERVAL);
}

void CXmppEngine::ReadFromStreamL(const TDesC8& aData) {
	TPtr8 pInputBuffer(iInputBuffer->Des());
	TBool aProcessedStanza;

	// Check if the stream buffer has room, if not increase size
	if((pInputBuffer.Length() + aData.Length()) > pInputBuffer.MaxLength()) {
		iInputBuffer = iInputBuffer->ReAlloc(pInputBuffer.Length() + aData.Length());
		pInputBuffer.Set(iInputBuffer->Des());
	}

	// Add new data to stream buffer & trim whitespaces
	pInputBuffer.Append(aData);
	pInputBuffer.TrimLeft();

	do {
		aProcessedStanza = false;

		//  Locate first close bracket
		TInt aStanzaClosedMarker = pInputBuffer.Locate('>');

		if(aStanzaClosedMarker != KErrNotFound) {
			if(iEngineState == EXmppLoggingIn) {
				// Test for '<?xml...' and '<stream:stream...'
				if(pInputBuffer.Find(_L8("<?xml")) != KErrNotFound || pInputBuffer.Find(_L8("<stream:stream")) != KErrNotFound) {
					pInputBuffer.Delete(0, (aStanzaClosedMarker + 1));
					
					aProcessedStanza = true;
				}
			}
			
			if(!aProcessedStanza) {
				if(pInputBuffer[aStanzaClosedMarker - 1] == TUint('/')) {
					// Test for <example/>
					ProcessStanzaInBufferL(aStanzaClosedMarker + 1);

					pInputBuffer.Set(iInputBuffer->Des());	
					aProcessedStanza = true;
				}
				else {
					// Locate the end of the element name
					TInt aElementMarker = pInputBuffer.Locate(' ');
					
					if(aElementMarker == KErrNotFound || aStanzaClosedMarker < aElementMarker) {
						aElementMarker = aStanzaClosedMarker;
					}
	
					// Get and cleanup Element name
					// '<example' to '</example>'
					HBufC8* aElementName = HBufC8::NewLC(aElementMarker + 2);
					TPtr8 pElementName(aElementName->Des());
					pElementName.Append(pInputBuffer.Left(aElementMarker));
					pElementName.Append('>');
					pElementName.Insert(1, _L8("/"));

					// Find stanza close
					if((aStanzaClosedMarker = pInputBuffer.Find(pElementName)) != KErrNotFound) {
						// We have a complete stanza
						ProcessStanzaInBufferL(aStanzaClosedMarker + pElementName.Length());
						
						pInputBuffer.Set(iInputBuffer->Des());
						aProcessedStanza = true;
					}

					CleanupStack::PopAndDestroy(aElementName);
				}				
			}
		}
	} while( aProcessedStanza );
}

void CXmppEngine::ProcessStanzaInBufferL(TInt aLength) {
	TPtr8 pInputBuffer(iInputBuffer->Des());
	
	HandleXmppStanzaL(pInputBuffer.Left(aLength));
	
	pInputBuffer.Delete(0, aLength);
}

void CXmppEngine::DataWritten(const TDesC8& aMessage) {
	iDataSent += aMessage.Length();
	
	if(!iStreamCompressed) {
		iEngineObserver->XmppStanzaWritten(aMessage);
	}
}

void CXmppEngine::HostResolved(const TDesC& aHostName, TInt aHostPort) {
	if(iHostName) {
		delete iHostName;
	}
	
	iHostName = aHostName.AllocL();
	iHostPort = aHostPort;
}

void CXmppEngine::NotifyEvent(TTcpIpEngineState aTcpEngineState) {
	TPtr8 pXmppServer(iXmppServer->Des());
	TPtr8 pInputBuffer(iInputBuffer->Des());

	switch(aTcpEngineState) {
		case ETcpIpLookUpAddress:
			// Connection initialized
			iEngineState = EXmppConnecting;
			break;
		case ETcpIpConnecting:
			// Connecting to server
			iEngineObserver->XmppStateChanged(iEngineState);
			break;
		case ETcpIpConnected:
			// Connected to server
			iStateTimer->After(30000000);
			iEngineState = EXmppLoggingIn;
			iEngineObserver->XmppStateChanged(iEngineState);
			
			// Begin socket listening
			iTcpIpEngine->Read();

			// Reset compression engine
			iCompressionEngine->ResetL();

			iConnectionAttempts = 0;
			iStreamCompressed = false;

			// Initialize xmpp stream
			OpenStream();
			break;
		case ETcpIpSecureConnection:
			// Connection is secured
			iTcpIpEngine->Read();

			// Initialize xmpp stream
			OpenStream();
			break;
		case ETcpIpCarrierChangeReq:
		case ETcpIpDisconnected:
			iXmppOutbox->SetMessageSent(false);
			iXmppOutbox->ClearNonPersistantMessages();
			iSendQueuedMessages = false;

			// Reset stream
			pInputBuffer.Zero();

			if(aTcpEngineState == ETcpIpCarrierChangeReq) {
				iEngineState = EXmppReconnect;
				iEngineObserver->XmppStateChanged(iEngineState);
				iStateTimer->After(45000000);
			}
			else {
				switch(iEngineState) {
					case EXmppLoggingIn:
					case EXmppOnline:
						if(iLastError == EXmppNone) {
							iEngineState = EXmppReconnect;
							iEngineObserver->XmppStateChanged(iEngineState);
							iStateTimer->After(15000000);
						}
						break;
					case EXmppReset:
						iEngineState = EXmppReconnect;
						iEngineObserver->XmppStateChanged(iEngineState);
						iStateTimer->After(1000000);
						break;
					case EXmppReconnect:
					case EXmppInitialize:
					case EXmppConnecting:
						if(iLastError == EXmppNone) {
							iEngineState = EXmppReconnect;
							iEngineObserver->XmppStateChanged(iEngineState);
							iStateTimer->After(30000000);
						}
						break;
					case EXmppShutdown:
						iEngineState = EXmppDisconnected;
						iEngineObserver->XmppShutdownComplete();
						break;
					default:;
				}
			}
			break;
		case ETcpIpWriteComplete:
			// Send next stanza
			SendNextQueuedStanzaL();
			break;
		default:;
	}
}

void CXmppEngine::Error(TTcpIpEngineError aError) {
	switch(aError) {
		case ETcpIpCancelled:
			iEngineState = EXmppDisconnected;
			iEngineObserver->XmppStateChanged(EXmppDisconnected);
			break;
		case ETcpIpAlreadyBusy:
			iEngineObserver->XmppError(EXmppAlreadyConnected);
			break;
		case ETcpIpReadWriteError:
			if(iEngineState >= EXmppConnecting) {
				// Connection lost after connect
				iTcpIpEngine->Disconnect();
			}
			break;
		case ETcpIpHostNameLookUpFailed:
			iEngineObserver->XmppError(EXmppServerUnresolved);
			break;
		case ETcpIpSecureFailed:
			iLastError = EXmppTlsFailed;
		default:
			if(aError != ETcpIpDnsTimeout && iConnectionAttempts > 5) {
				// Connection failed too many times
				HostResolved(KNullDesC, 5222);
				
				iConnectionAttempts = 0;
			}
			
			// Attempt reconnect
			iEngineState = EXmppReconnect;
			iEngineObserver->XmppStateChanged(EXmppReconnect);
			iStateTimer->After(30000000);
			break;
	}
}

void CXmppEngine::TcpIpDebug(const TDesC8& aMessage, TInt aCode) {
	TBuf8<256> aPrint(aMessage);

	aPrint.AppendFormat(_L8(": %d"), aCode);
	iEngineObserver->XmppDebug(aPrint);
}

void CXmppEngine::TcpIpDebug(const TDesC8& aMessage) {
	iEngineObserver->XmppDebug(aMessage);
}

void CXmppEngine::ConnectionCreated(TUint aConnectionId) {
	TcpIpDebug(_L8("XMPP  Connection Created"), iEngineState);

	if(iEngineState >= EXmppInitialize && iEngineState <= EXmppLoggingIn) {
		TcpIpDebug(_L8("XMPP  This Connection"), aConnectionId);

		iConnectionId = aConnectionId;
	}
}

void CXmppEngine::ConnectionDeleted(TUint aConnectionId) {
	TcpIpDebug(_L8("XMPP  Connection Deleted"), iEngineState);

	if(iConnectionId == aConnectionId) {
		iConnectionId = 0;

		if(iEngineState > EXmppReconnect) {
			TcpIpDebug(_L8("XMPP  This Connection"), aConnectionId);

			iTcpIpEngine->Disconnect();
		}
	}
}

void CXmppEngine::ConnectionStatusChanged() {
	TcpIpDebug(_L8("XMPP  Connection Changed"), iEngineState);

	if(iConnectionMonitor->ConnectionBearerMobile(iConnectionId)) {
		if(iSilenceState == EXmppReconnectRequired) {
			if(iConnectionMonitor->GetNetworkStatus() != EConnMonStatusSuspended) {
				iSilenceState = EXmppSilenceTest;
				iConnectionId = 0;
				
				iEngineState = EXmppReconnect;
				iTcpIpEngine->Disconnect();
			}
		}
	}
}

void CXmppEngine::ConnectionMonitorDebug(const TDesC8& aMessage) {
	iEngineObserver->XmppDebug(aMessage);
}

// End of File

