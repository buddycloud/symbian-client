/*
============================================================================
 Name        : 	XmppOutbox.h
 Author      : 	Ross Savage
 Copyright   : 	2009 Buddycloud
 Description : 	XMPP Outbox for storage & management of outgoing messages
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef XMPPOUTBOX_H_
#define XMPPOUTBOX_H_

// INCLUDES
#include <e32base.h>
#include "XmppConstants.h"

/*
----------------------------------------------------------------------------
--
-- CXmppOutboxMessage
--
----------------------------------------------------------------------------
*/

class CXmppOutboxMessage : public CBase {
	public:
		static CXmppOutboxMessage* NewLC();
		~CXmppOutboxMessage();
	
	private:
		void ConstructL();
	
	public:
		void SetStanzaL(const TDesC8& aStanza);
		void SetPriority(TXmppMessagePriority aPriority);
		void SetPersistance(TBool aPersistant);
	
	public:
		TDesC8& GetStanza();
		TXmppMessagePriority GetPriority();
		TBool GetPersistance();
	
	private:
		HBufC8* iStanza;
		TXmppMessagePriority iPriority;
		TBool iPersistant;
};

/*
----------------------------------------------------------------------------
--
-- CXmppOutbox
--
----------------------------------------------------------------------------
*/

class CXmppOutbox : public CBase {
	public:
		static CXmppOutbox* NewL();
		~CXmppOutbox();

	public:
		void AddMessage(CXmppOutboxMessage* aMessage);
		CXmppOutboxMessage* GetNextMessage();
		
	public:
		void SetMessageSent(TBool aSent);
		TBool SendInProgress();
		
	public:
		TBool ContainsPriorityMessages(TXmppMessagePriority aPriority);
		void ClearNonPersistantMessages();

	public:
		CXmppOutboxMessage* GetMessage(TInt aIndex);
		TInt Count();

	private:
		RPointerArray<CXmppOutboxMessage> iMessages;
		
		TBool iSendInProgress;
};


#endif /* XMPPOUTBOX_H_ */
