/*
============================================================================
 Name        : 	MessagingParticipants.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Storage of discussion participant data
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "MessagingParticipants.h"

/*
----------------------------------------------------------------------------
--
-- CMessagingParticipant
--
----------------------------------------------------------------------------
*/

CMessagingParticipant::CMessagingParticipant(TDesC& aId) {
	iId = aId.AllocL();	
	iJid = HBufC::NewL(0);
	
	iAvatarId = -1;	
	// TODO: WA: Should be none
	iAffiliation = EAffiliationMember;
	iRole = ERoleParticipant;
}

CMessagingParticipant::~CMessagingParticipant() {
	if(iId)
		delete iId;
	
	if(iJid)
		delete iJid;
}

TDesC& CMessagingParticipant::GetId() {
	return *iId;
}

TDesC& CMessagingParticipant::GetJid() {
	return *iJid;
}

void CMessagingParticipant::SetJidL(TDesC& aJid) {
	if(iJid)
		delete iJid;

	iJid = aJid.AllocL();
}

TInt CMessagingParticipant::GetAvatarId() {
	return iAvatarId;
}

void CMessagingParticipant::SetAvatarId(TInt aAvatarId) {
	iAvatarId = aAvatarId;
}

TMucAffiliation CMessagingParticipant::GetAffiliation() {
	return iAffiliation;
}

void CMessagingParticipant::SetAffiliation(TMucAffiliation aAffiliation) {
	iAffiliation = aAffiliation;
}

TMucRole CMessagingParticipant::GetRole() {
	return iRole;
}

void CMessagingParticipant::SetRole(TMucRole aRole) {
	iRole = aRole;
}

TMessageChatState CMessagingParticipant::GetChatState() {
	return iChatState;
}

void CMessagingParticipant::SetChatState(TMessageChatState aChatState) {
	iChatState = aChatState;
}

/*
----------------------------------------------------------------------------
--
-- CMessagingParticipantstore
--
----------------------------------------------------------------------------
*/

CMessagingParticipantstore::CMessagingParticipantstore() {
}

CMessagingParticipantstore::~CMessagingParticipantstore() {
	for(TInt i = 0;i < iParticipants.Count();i++) {
		delete iParticipants[i];
	}

	iParticipants.Close();
}

CMessagingParticipant* CMessagingParticipantstore::AddParticipant(TDesC& aId) {
	for(TInt i = 0; i < iParticipants.Count(); i++) {
		if(iParticipants[i]->GetId().CompareF(aId) == 0) {
			return iParticipants[i];
		}
	}

	CMessagingParticipant* aParticipant = new CMessagingParticipant(aId);
	iParticipants.Append(aParticipant);
	
	return aParticipant;
}

void CMessagingParticipantstore::RemoveParticipant(TDesC& aId) {
	for(TInt i = 0; i < iParticipants.Count(); i++) {
		if(iParticipants[i]->GetId().CompareF(aId) == 0) {
			delete iParticipants[i];
			iParticipants.Remove(i);
			break;
		}
	}
}

void CMessagingParticipantstore::RemoveAllParticipants() {
	for(TInt i = 0; i < iParticipants.Count(); i++) {
		delete iParticipants[i];
	}

	iParticipants.Reset();
}

CMessagingParticipant* CMessagingParticipantstore::GetParticipant(TDesC& aId) {
	for(TInt i = 0; i < iParticipants.Count(); i++) {
		if(iParticipants[i]->GetId().CompareF(aId) == 0 || iParticipants[i]->GetJid().CompareF(aId) == 0) {
			return iParticipants[i];
		}
	}
	
	return NULL;
}

CMessagingParticipant* CMessagingParticipantstore::GetParticipant(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iParticipants.Count()) {
		return iParticipants[aIndex];
	}
	
	return NULL;
}

TInt CMessagingParticipantstore::CountParticipants() {
	return iParticipants.Count();
}
