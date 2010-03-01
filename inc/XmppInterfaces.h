/*
============================================================================
 Name        : 	XmppInterfaces.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	XMPP interfaces for listening to an XMPP protocol server
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPINTERFACES_H_
#define XMPPINTERFACES_H_

#include "XmppConstants.h"

class MXmppRosterObserver {
	public:
		virtual void XmppRosterL(const TDesC8& aStanza) = 0;
};

class MXmppStanzaObserver {
	public:
		virtual void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) = 0;
};

class MXmppWriteInterface {
	public:
		virtual void SendAndForgetXmppStanza(const TDesC8& aStanza, TBool aPersisant, TXmppMessagePriority aPriority = EXmppPriorityNormal) = 0;
		virtual void SendAndAcknowledgeXmppStanza(const TDesC8& aStanza, MXmppStanzaObserver* aObserver, TBool aPersisant = false, TXmppMessagePriority aPriority = EXmppPriorityNormal) = 0;
		
	public:
		virtual void CancelXmppStanzaAcknowledge(MXmppStanzaObserver* aObserver) = 0;
	
};

#endif /* XMPPINTERFACES_H_ */
