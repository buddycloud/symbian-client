/*
============================================================================
 Name        : 	XmppInterfaces.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	XMPP interfaces for listening to an XMPP protocol server
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPINTERFACES_H_
#define XMPPINTERFACES_H_

#include <XmppOutbox.h>

class MXmppRosterObserver {
	public:
		virtual void RosterItemsL(const TDesC8& aItems, TBool aUpdate) = 0;
		virtual void RosterOwnJidL(TDesC& aJid) = 0;
		virtual void RosterPubsubEventL(TDesC& aFrom, const TDesC8& aItem, TBool aNew) = 0;
		virtual void RosterPubsubDeleteL(TDesC& aFrom, const TDesC8& aNode) = 0;
		virtual void RosterSubscriptionL(TDesC& aJid, const TDesC8& aData) = 0;
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
