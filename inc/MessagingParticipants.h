/*
============================================================================
 Name        : 	MessagingParticipants.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Storage of discussion participant data
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef MESSAGINGPARTICIPANTS_H_
#define MESSAGINGPARTICIPANTS_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TMessageChatState {
	EChatInactive, EChatActive, EChatComposing, EChatPaused, EChatGone
};

enum TMucAffiliation {
	EAffiliationOutcast, EAffiliationNone, EAffiliationMember, EAffiliationAdmin, EAffiliationOwner
};

enum TMucRole {
	ERoleNone, ERoleVisitor, ERoleParticipant, ERoleModerator
};

/*
----------------------------------------------------------------------------
--
-- CMessagingParticipant
--
----------------------------------------------------------------------------
*/

class CMessagingParticipant : public CBase {
	public:
		CMessagingParticipant(TDesC& aId);
		~CMessagingParticipant();

	public:
		TDesC& GetId();
		
		TDesC& GetJid();
		void SetJidL(TDesC& aJid);
		
		TInt GetAvatarId();
		void SetAvatarId(TInt aAvatarId);
		
		TMucAffiliation GetAffiliation();
		void SetAffiliation(TMucAffiliation aAffiliation);
		
		TMucRole GetRole();
		void SetRole(TMucRole aRole);

		TMessageChatState GetChatState();
		void SetChatState(TMessageChatState aChatState);

	protected:
		HBufC* iId;
		
		HBufC* iJid;
		
		TMucAffiliation iAffiliation;
		TMucRole iRole;
		
		TInt iAvatarId;
		TMessageChatState iChatState;
};

/*
----------------------------------------------------------------------------
--
-- CMessagingParticipantstore
--
----------------------------------------------------------------------------
*/

class CMessagingParticipantstore : public CBase {
	public:
		CMessagingParticipantstore();
		~CMessagingParticipantstore();
		
	public:
		CMessagingParticipant* AddParticipant(TDesC& aId);
		void RemoveParticipant(TDesC& aId);
		void RemoveAllParticipants();
				
	public:
		CMessagingParticipant* GetParticipant(TDesC& aId);
		CMessagingParticipant* GetParticipant(TInt aIndex);
		TInt CountParticipants();
		
	protected:
		RPointerArray<CMessagingParticipant> iParticipants;
};

#endif /*MESSAGINGPARTICIPANTS_H_*/
