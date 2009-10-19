/*
============================================================================
 Name        : 	XmppObservers.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	XMPP observers for listening to an XMPP protocol server
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPOBSERVERS_H_
#define XMPPOBSERVERS_H_

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
		virtual void XmppStanzaNotificationL(const TDesC8& aStanza, const TDesC8& aId) = 0;
};

#endif /* XMPPOBSERVERS_H_ */
