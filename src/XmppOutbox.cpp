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

// INCLUDES
#include "XmppOutbox.h"

/*
----------------------------------------------------------------------------
--
-- CXmppOutboxMessage
--
----------------------------------------------------------------------------
*/
		
CXmppOutboxMessage* CXmppOutboxMessage::NewLC() {
	CXmppOutboxMessage* self = new (ELeave) CXmppOutboxMessage();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CXmppOutboxMessage::~CXmppOutboxMessage() {
	if(iStanza)
		delete iStanza;
}

void CXmppOutboxMessage::ConstructL() {
	iStanza = HBufC8::NewL(0);
}

void CXmppOutboxMessage::SetStanzaL(const TDesC8& aStanza) {
	if(iStanza)
		delete iStanza;

	iStanza = aStanza.AllocL();
}

void CXmppOutboxMessage::SetPriority(TXmppMessagePriority aPriority) {
	iPriority = aPriority;	
}

void CXmppOutboxMessage::SetPersistance(TBool aPersistant) {
	iPersistant = aPersistant;
}
		
TDesC8& CXmppOutboxMessage::GetStanza() {
	return *iStanza;	
}
		
TXmppMessagePriority CXmppOutboxMessage::GetPriority() {
	return iPriority;
}
		
TBool CXmppOutboxMessage::GetPersistance() {
	return iPersistant;
}

/*
----------------------------------------------------------------------------
--
-- CXmppOutbox
--
----------------------------------------------------------------------------
*/

CXmppOutbox* CXmppOutbox::NewL() {
	CXmppOutbox* self = new (ELeave) CXmppOutbox();
	return self;
}

CXmppOutbox::~CXmppOutbox() {
	for(TInt i = 0; i < iMessages.Count(); i++) {
		delete iMessages[i];
	}

	iMessages.Close();
}

void CXmppOutbox::AddMessage(CXmppOutboxMessage* aMessage) {
	if(aMessage->GetPriority() != EXmppPriorityNormal) {
		// Insert other priority message
		for(TInt i = 0; i < iMessages.Count(); i++) {
			if(iMessages[i]->GetPriority() < aMessage->GetPriority()) {
				if(iSendInProgress && i == 0) {
					i++;
				}
				
				iMessages.Insert(aMessage, i);
				
				return;
			}
		}
	}
	
	iMessages.Append(aMessage);
}

CXmppOutboxMessage* CXmppOutbox::GetNextMessage() {
	if(iMessages.Count() > 0) {
		iSendInProgress = true;
		return iMessages[0];
	}
		
	return NULL;
}

void CXmppOutbox::SetMessageSent(TBool aSent) {
	if(aSent && iSendInProgress) {
		if(iMessages.Count() > 0) {
			delete iMessages[0];
			iMessages.Remove(0);
		}
	}
	
	iSendInProgress = false;
}

TBool CXmppOutbox::SendInProgress() {
	return iSendInProgress;
}

TBool CXmppOutbox::ContainsPriorityMessages(TXmppMessagePriority aPriority) {
	for(TInt i = 0; i < iMessages.Count(); i++) {
		if(iMessages[i]->GetPriority() == aPriority) {
			return true;
		}
	}
	
	return false;
}

void CXmppOutbox::ClearNonPersistantMessages() {
	for(TInt i = (iMessages.Count() - 1); i >= 0; i--) {
		if(!iMessages[i]->GetPersistance()) {
			delete iMessages[i];
			iMessages.Remove(i);
		}
	}
}

CXmppOutboxMessage* CXmppOutbox::GetMessage(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iMessages.Count()) {
		return iMessages[aIndex];
	}
	
	return NULL;
}

TInt CXmppOutbox::Count() {
	return iMessages.Count();
}
