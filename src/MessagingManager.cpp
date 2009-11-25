/*
============================================================================
 Name        : 	MessagingManager.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Management of message discussions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <coemain.h>
#include <f32file.h>
#include <s32file.h>
#include "FileUtilities.h"
#include "MessagingManager.h"
#include "TextUtilities.h"
#include "TimeUtilities.h"
#include "XmlParser.h"

/*
----------------------------------------------------------------------------
--
-- CMessage
--
----------------------------------------------------------------------------
*/

CMessageLinkPosition::CMessageLinkPosition(TInt aStart, TInt aLength, TMessageLinkType aType) {
	iStart = aStart;
	iLength = aLength;
	iType = aType;
}

CMessage* CMessage::NewL() {
	CMessage* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CMessage* CMessage::NewLC() {
	CMessage* self = new (ELeave) CMessage();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CMessage::~CMessage() {
	if(iJid)
		delete iJid;
	
	if(iName)
		delete iName;

	if(iBody)
		delete iBody;

	if(iThread)
		delete iThread;

	if(iLocation)
		delete iLocation;
	
	iLinks.Close();
}

CMessage::CMessage() {
	iMessageType = EMessageTextual;

	iReceivedAt.UniversalTime();
	iAvatarId = -1;

	iRead = false;
}
	
void CMessage::ConstructL() {
	iJid = HBufC::NewL(0);
	iName = HBufC::NewL(0);
	iBody = HBufC::NewL(0);
	iThread = HBufC8::NewL(0);
	iLocation = HBufC::NewL(0);
}

TMessageContentType CMessage::GetMessageType() {
	return iMessageType;
}

TTime CMessage::GetReceivedAt() {
	return iReceivedAt;
}

void CMessage::SetReceivedAt(TTime aTime) {
	iReceivedAt = aTime;
}

TInt CMessage::GetAvatarId() {
	return iAvatarId;
}

void CMessage::SetAvatarId(TInt aAvatarId) {
	iAvatarId = aAvatarId;
}

TBool CMessage::GetRead() {
	return iRead;
}

void CMessage::SetRead(TBool aRead) {
	iRead = aRead;
}

TBool CMessage::GetDirectReply() {
	return iDirectReply;
}

void CMessage::SetDirectReply(TBool aDirectReply) {
	iDirectReply = aDirectReply;
}

TBool CMessage::GetPrivate() {
	return iPrivate;
}

void CMessage::SetPrivate(TBool aPrivate) {
	iPrivate = aPrivate;
}

TBool CMessage::GetOwn() {
	return iOwn;
}

void CMessage::SetOwn(TBool aOwn) {
	iOwn = aOwn;
}

TDesC& CMessage::GetJid() {
	return *iJid;
}

TMessageJidType CMessage::GetJidType() {
	return iJidType;
}

void CMessage::SetJidL(TDesC& aJid, TMessageJidType aType) {
	if(iJid)
		delete iJid;

	iJid = aJid.AllocL();
	iJidType = aType;
}

TDesC& CMessage::GetName() {
	return *iName;
}

void CMessage::SetNameL(const TDesC& aName) {
	if(iName)
		delete iName;

	iName = aName.AllocL();
}

TDesC& CMessage::GetBody() {
	return *iBody;
}

void CMessage::SetBodyL(TDesC& aBody, const TDesC& aUsername, TMessageContentType aMessageType) {
	if(iBody) {
		delete iBody;
	}

	iMessageType = aMessageType;
		
	if(aBody.Length() > 0 && aMessageType <= EMessageAdvertisment) {
		if(iMessageType == EMessageTextual && aBody[0] == TInt('/') && aBody.Length() > 4 && 
				(aBody.Left(3)).Compare(_L("/me")) == 0) {
			
			// Test for '/me' type messages
			TPtrC pName(iName->Des());
			
			iMessageType = EMessageUserAction;
			iBody = HBufC::NewL(pName.Length() + aBody.Length());
			TPtr pMeAction(iBody->Des());
			pMeAction.Append(aBody);
			pMeAction.Replace(0, 3, pName);
		}
		else {	
			// Parse message body for hyperlinks
			iBody = aBody.AllocL();
			TPtrC pBody(iBody->Des());
			
			TInt aGlobalPosition = 0;
			TInt aLinkSearch = pBody.MatchF(_L("*http*://*"));
			TInt aWwwSearch = pBody.FindF(_L("www."));
			TInt aAtReplySearch = pBody.Locate('@');
			TInt aChannelSearch = pBody.Locate('#');
			
			while(aLinkSearch != KErrNotFound || aWwwSearch != KErrNotFound || 
					aAtReplySearch != KErrNotFound || aChannelSearch != KErrNotFound) {	
				
				TMessageLinkType aLinkType = ELinkWebsite;
				TInt aMinLinkLength = 10;

				// Check result against www
				if(aWwwSearch != KErrNotFound && (aWwwSearch < aLinkSearch || aLinkSearch == KErrNotFound)) {
					aLinkSearch = aWwwSearch;
					aMinLinkLength = 7;
				}
				
				// Check '@' result
				if(aAtReplySearch != KErrNotFound && (aAtReplySearch < aLinkSearch || aLinkSearch == KErrNotFound)) {
					aLinkSearch = aAtReplySearch;
					aLinkType = ELinkUsername;
					aMinLinkLength = 2;
				}
				
				// Check '#' result
				if(aChannelSearch != KErrNotFound && (aChannelSearch < aLinkSearch || aLinkSearch == KErrNotFound)) {
					aLinkSearch = aChannelSearch;
					aLinkType = ELinkChannel;
					aMinLinkLength = 2;
				}
				
				aGlobalPosition += aLinkSearch;
				pBody.Set(pBody.Mid(aLinkSearch));		
				TPtrC pLink(pBody);
				
				if((aLinkSearch = pBody.Locate(' ')) != KErrNotFound) {
					pLink.Set(pBody.Left(aLinkSearch));			
				}
				
				if(aLinkType == ELinkWebsite || (aGlobalPosition == 0 || aBody[aGlobalPosition - 1] == ' ')) {
					while((aLinkType != ELinkWebsite && 
									// Non website links
									((aLinkSearch = pLink.Locate(':')) != KErrNotFound || 
									(aLinkSearch = pLink.Locate(',')) != KErrNotFound || 
									(aLinkSearch = pLink.Locate('#')) > 0 || 
									(aLinkSearch = pLink.Locate('@')) > 0)) ||
							// General
							(aLinkSearch = pLink.Locate('\'')) != KErrNotFound || 
							(aLinkSearch = pLink.Locate('\"')) != KErrNotFound || 
							(aLinkSearch = pLink.Locate('>')) != KErrNotFound || 
							(aLinkSearch = pLink.Locate(']')) != KErrNotFound || 
							(aLinkSearch = pLink.Locate(')')) != KErrNotFound || 
							((aLinkSearch = pLink.LocateReverse('.')) != KErrNotFound && aLinkSearch == pLink.Length() - 1) ||
							((aLinkSearch = pLink.LocateReverse('?')) != KErrNotFound && aLinkSearch == pLink.Length() - 1) ||
							((aLinkSearch = pLink.LocateReverse('!')) != KErrNotFound && aLinkSearch == pLink.Length() - 1)) {
						
						pLink.Set(pLink.Left(aLinkSearch));
					}
					
					if(pLink.Length() >= aMinLinkLength) {
						iLinks.Append(CMessageLinkPosition(aGlobalPosition, pLink.Length(), aLinkType));
						
						if(aLinkType == ELinkUsername && aUsername.CompareF(pLink.Mid(1)) == 0) {
							iDirectReply = true;
						}
					}
				}
				
				aGlobalPosition += pLink.Length();
				pBody.Set(pBody.Mid(pLink.Length()));	
				
				aLinkSearch = pBody.MatchF(_L("*http*://*"));
				aWwwSearch = pBody.FindF(_L("www."));
				aAtReplySearch = pBody.Locate('@');
				aChannelSearch = pBody.Locate('#');
			}
		}
	}
	else {
		iBody = aBody.AllocL();
	}
}

TDesC8& CMessage::GetThread() {
	return *iThread;
}

void CMessage::SetThreadL(const TDesC8& aThread) {
	if(iThread)
		delete iThread;

	iThread = aThread.AllocL();
}

TDesC& CMessage::GetLocation() {
	return *iLocation;
}

void CMessage::SetLocationL(TDesC& aLocation, TBool aAppend) {
	if(aAppend) {
		TPtr pLocation(iLocation->Des());
		
		iLocation = iLocation->ReAlloc(pLocation.Length() + 2 + aLocation.Length());
		pLocation.Set(iLocation->Des());
		
		if(pLocation.Length() > 0) {
			pLocation.Append(_L(", "));
		}
		
		pLocation.Append(aLocation);		
	}
	else {
		if(iLocation) {
			delete iLocation;
		}
		
		iLocation = aLocation.AllocL();
	}
}

TInt CMessage::GetLinkCount() {
	return iLinks.Count();
}

CMessageLinkPosition CMessage::GetLink(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iLinks.Count()) {
		return iLinks[aIndex];
	}
	
	return CMessageLinkPosition(0, 0);
}

TBool CMessage::Equals(CMessage* aMessage) {
	if((aMessage->GetReceivedAt() == iReceivedAt || 
			(aMessage->GetReceivedAt() > (iReceivedAt - TTimeIntervalSeconds(90)) &&
			aMessage->GetReceivedAt() < (iReceivedAt + TTimeIntervalSeconds(90)))) && 
			aMessage->GetBody().Compare(*iBody) == 0) {
		
		return true;
	}

	return false;
}
	
/*
----------------------------------------------------------------------------
--
-- CDiscussion
--
----------------------------------------------------------------------------
*/
	
CDiscussion* CDiscussion::NewL(const TDesC& aJid) {
	CDiscussion* self = NewLC(aJid);
	CleanupStack::Pop(self);
	return self;
}

CDiscussion* CDiscussion::NewLC(const TDesC& aJid){
	CDiscussion* self = new (ELeave) CDiscussion();
	CleanupStack::PushL(self);
	self->ConstructL(aJid);
	return self;	
}

CDiscussion::~CDiscussion() {
	SaveDiscussionToFileL();
	
	if(iJid)
		delete iJid;

	for(TInt i = 0; i < iMessages.Count(); i++) {
		delete iMessages[i];
	}

	iMessages.Close();
	
	if(iParticipantStore)
		delete iParticipantStore;
}

CDiscussion::CDiscussion() {
	iUnreadMessages = 0;
	iUnreadReplies = 0;
	iMessageLimit = 50;
	iNotificationOn = true;
}

void CDiscussion::ConstructL(const TDesC& aJid){
	iJid = aJid.AllocL();
	
	iParticipantStore = new CMessagingParticipantstore();
}

TDesC& CDiscussion::GetJid(){
	return *iJid;
}

void CDiscussion::SetJidL(const TDesC& aJid) {
	// Rename existing cache file
	RFs aSession = CCoeEnv::Static()->FsSession();
	TFileName aOldFilePath = GetFileName(aSession);
	
	if(iJid) {
		delete iJid;
	}
	
	iJid = aJid.AllocL();
	
	TFileName aNewFilePath = GetFileName(aSession);
	
	if(aNewFilePath.Compare(aOldFilePath) != 0) {
		aSession.Rename(aOldFilePath, aNewFilePath);
	}
}

CFollowingItem* CDiscussion::GetFollowingItem() {
	return iFollowingItem;
}

void CDiscussion::AddMessageReadObserver(MMessageReadObserver* aObserver) {
	iMessageReadObserver = aObserver;
}

void CDiscussion::AddDiscussionReadObserver(MDiscussionReadObserver* aObserver, CFollowingItem* aFollowingItem) {
	iDiscussionReadObserver = aObserver;
	iFollowingItem = aFollowingItem;
}

void CDiscussion::AddMessagingObserver(MDiscussionMessagingObserver* aObserver) {
	LoadDiscussionFromFileL();
	
	iMessagingObserver = aObserver;
}

void CDiscussion::RemoveMessagingObserver() {
	iMessagingObserver = NULL;
}

void CDiscussion::CompressL() {
	if(iMessagingObserver == NULL) {
		if(iMessagesCached) {
			TTime aNow;
			aNow.UniversalTime();			
			
			if((iLastUpdate + TTimeIntervalSeconds(450)) <= aNow) {
				// Save to file if older
				SaveDiscussionToFileL();
				
				// Compress (free memory) of used messages
				for(TInt i = 0; i < iMessages.Count(); i++) {
					delete iMessages[i];
				}

				iMessages.Reset();

				iMessagesCached = false;
			}
		}
	}
}

void CDiscussion::CleanL() {
	DeleteDiscussionFileL();
	DeleteAll();
	
	iMessagesCached = false;
}

TInt CDiscussion::GetTotalMessages() {
	return iMessages.Count();
}

TInt CDiscussion::GetUnreadMessages() {
	return iUnreadMessages;
}

TInt CDiscussion::GetUnreadReplies() {
	return iUnreadReplies;
}

void CDiscussion::SetUnreadMessages(TInt aMessages, TInt aReplies) {
	iUnreadMessages = aMessages;
	iUnreadReplies = aReplies;
	
	if(iMessageReadObserver) {
		iMessageReadObserver->MessageRead(*iJid, 0, iUnreadMessages, iUnreadReplies);
	}
}

void CDiscussion::SetMessageRead(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iMessages.Count()) {
		if(!iMessages[aIndex]->GetRead()) {
			iMessages[aIndex]->SetRead(true);
			iLastUpdate.UniversalTime();
			iUnreadMessages--;
			
			if(iMessages[aIndex]->GetDirectReply()) {
				iUnreadReplies--;
			}
			
			if(iMessageReadObserver) {
				iMessageReadObserver->MessageRead(*iJid, aIndex, iUnreadMessages, iUnreadReplies);
			}

			if(iUnreadMessages == 0) {
				if(iDiscussionReadObserver) {
					iDiscussionReadObserver->AllMessagesRead(iFollowingItem);
				}
			}
		}
	}
}

void CDiscussion::SetMaxMessagesStored(TInt aLimit) {
	iMessageLimit = aLimit;

	// Auto-Purge
	while(iMessages.Count() > iMessageLimit) {
		DeleteMessage(0);
	}
}

void CDiscussion::DeleteAll() {
	if(iUnreadMessages > 0) {
		if(iDiscussionReadObserver) {
			iDiscussionReadObserver->AllMessagesRead(iFollowingItem);
		}
	}

	while(iMessages.Count() > 0) {
		DeleteMessage(0);
	}

	iMessages.Reset();

	iUnreadMessages = 0;
	iUnreadReplies = 0;
	
	if(iMessageReadObserver) {
		iMessageReadObserver->MessageRead(*iJid, 0, 0);
	}
}

void CDiscussion::DeleteMessage(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iMessages.Count()) {
		if(!iMessages[aIndex]->GetRead()) {
			iUnreadMessages--;
			
			if(iMessages[aIndex]->GetDirectReply()) {
				iUnreadReplies--;
			}
						
			if(iMessageReadObserver) {
				iMessageReadObserver->MessageRead(*iJid, aIndex, iUnreadMessages, iUnreadReplies);
			}
		}

		delete iMessages[aIndex];
		iMessages.Remove(aIndex);

		if(iMessagingObserver) {
			iMessagingObserver->MessageDeleted(aIndex);
		}
	}
}

TBool CDiscussion::GetNotificationOn() {
	LoadDiscussionFromFileL();

	return iNotificationOn;
}

void CDiscussion::SetNotificationOn(TBool aNotificationOn) {
	iNotificationOn = aNotificationOn;
	iLastUpdate.UniversalTime();
}

CMessage* CDiscussion::GetMessage(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iMessages.Count()) {
		return iMessages[aIndex];
	}

	return NULL;
}

TBool CDiscussion::AddMessageLD(CMessage* aMessage, TBool aDelayed) {
	LoadDiscussionFromFileL();

	if(aDelayed) {
		for(TInt i = 0; i < iMessages.Count(); i++) {
			if(aMessage->Equals(iMessages[i])) {
				// Message already exists
				CleanupStack::PopAndDestroy();				
				return false;
			}
		}
		
		// Add Message in chronological order
		for(TInt i = (iMessages.Count() - 1); i >= 0; i--) {
			if(aMessage->GetReceivedAt() >= iMessages[i]->GetReceivedAt()) {
				// New message
				CleanupStack::Pop();
				InsertMessage(aMessage, (i + 1));
				return true;
			}
		}
	}

	CleanupStack::Pop();
	InsertMessage(aMessage, iMessages.Count());
	return true;
}

void CDiscussion::InsertMessage(CMessage* aMessage, TInt aIndex) {
	// Auto-Purge
	while(iMessages.Count() >= iMessageLimit) {
		DeleteMessage(0);
		aIndex--;
	}

	iMessages.Insert(aMessage, aIndex);
	iLastUpdate.UniversalTime();
	
	if(!aMessage->GetRead()) {
		iUnreadMessages++;
		
		if(aMessage->GetDirectReply()) {
			iUnreadReplies++;
		}
		
		if(iMessageReadObserver) {
			iMessageReadObserver->MessageRead(*iJid, aIndex, iUnreadMessages, iUnreadReplies);
		}
	}

	if(iMessagingObserver) {
		iMessagingObserver->MessageReceived(aMessage, aIndex);
	}
}

CMessagingParticipantstore* CDiscussion::GetParticipants() {
	return iParticipantStore;
}

TMucAffiliation CDiscussion::GetMyAffiliation() {
	return iAffiliation;
}

TMucRole CDiscussion::GetMyRole() {
	return iRole;
}

void CDiscussion::SetMyRoleAndAffiliation(TMucRole aRole, TMucAffiliation aAffiliation) {
	iRole = aRole;
	iAffiliation = aAffiliation;
}

void CDiscussion::LoadDiscussionFromFileL() {
	if( !iMessagesCached ) {
		RFs aSession = CCoeEnv::Static()->FsSession();	
		TFileName aFilePath = GetFileName(aSession);
		
		RFile aFile;		
		TInt aFileSize;
		
		iUnreadMessages = 0;
		iUnreadReplies = 0;
		
		if(aFile.Open(aSession, aFilePath, EFileStreamText|EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);
			aFile.Size(aFileSize);

			// Create buffer & read file
			HBufC8* aBuf = HBufC8::NewLC(aFileSize);
			TPtr8 pBuf(aBuf->Des());
			aFile.Read(pBuf);

			CXmlParser* aXmlParser = CXmlParser::NewLC(*aBuf);
			CTextUtilities* aCharCoder = CTextUtilities::NewLC();
			CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();

			if(aXmlParser->MoveToElement(_L8("discussion"))) {
				// Buzz
				iNotificationOn = aXmlParser->GetBoolAttribute(_L8("notify"));
				
				while(aXmlParser->MoveToElement(_L8("message"))) {
					CMessage* aMessage = CMessage::NewLC();
					aMessage->SetJidL(aCharCoder->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))), TMessageJidType(aXmlParser->GetIntAttribute(_L8("jidtype"))));
					aMessage->SetNameL(aCharCoder->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("from"))));
					aMessage->SetThreadL(aXmlParser->GetStringAttribute(_L8("thread")));
					aMessage->SetLocationL(aCharCoder->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("location"))));

					TPtrC8 pAttributeAt = aXmlParser->GetStringAttribute(_L8("at"));
					if(pAttributeAt.Length() > 0) {
						aMessage->SetReceivedAt(aTimeUtilities->DecodeL(pAttributeAt));
					}

					aMessage->SetAvatarId(aXmlParser->GetIntAttribute(_L8("avatar")));
					aMessage->SetRead(aXmlParser->GetBoolAttribute(_L8("read")));
					aMessage->SetDirectReply(aXmlParser->GetBoolAttribute(_L8("reply")));
					aMessage->SetPrivate(aXmlParser->GetBoolAttribute(_L8("private")));
					aMessage->SetOwn(aXmlParser->GetBoolAttribute(_L8("own")));
					aMessage->SetBodyL(aCharCoder->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("body"))), _L(""), TMessageContentType(aXmlParser->GetIntAttribute(_L8("type"))));
					
					if(!aMessage->GetRead()) {
						iUnreadMessages++;
						
						if(aMessage->GetDirectReply()) {
							iUnreadReplies++;
						}
					}

					iMessages.Append(aMessage);
					CleanupStack::Pop();
				}
			}

			CleanupStack::PopAndDestroy(4); // aTimeUtilities, aCharCoder, aXmlParser, aBuf
			CleanupStack::PopAndDestroy(&aFile);
		}
			
		iMessagesCached = true;
		
		if(iMessageReadObserver) {
			iMessageReadObserver->MessageRead(*iJid, 0, iUnreadMessages);
		}
	}
}

void CDiscussion::SaveDiscussionToFileL() {
	if(iMessagesCached) {
		RFs aSession = CCoeEnv::Static()->FsSession();	
		TFileName aFilePath = GetFileName(aSession);

		RFileWriteStream aFile;
		TBuf8<128> aBuf;
	
		if(aFile.Replace(aSession, aFilePath, EFileStreamText|EFileWrite) == KErrNone) {
			CleanupClosePushL(aFile);

			CTextUtilities* aCharCoder = CTextUtilities::NewLC();
			CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
			TFormattedTimeDesc aTime;

			aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>"));		
			aBuf.Format(_L8("<discussion notify='%d'>"), iNotificationOn);
			aFile.WriteL(aBuf);

			for(TInt x = 0;x < iMessages.Count(); x++) {
				CMessage* aMessage = iMessages[x];

				aFile.WriteL(_L8("<message"));

				if(aMessage->GetJid().Length() > 0) {
					aFile.WriteL(_L8(" jid='"));
					aFile.WriteL(aCharCoder->UnicodeToUtf8L(aMessage->GetJid()));
					aBuf.Format(_L8("' jidtype='%d'"), aMessage->GetJidType());
					aFile.WriteL(aBuf);
				}

				if(aMessage->GetName().Length() > 0) {
					aFile.WriteL(_L8(" from='"));
					aFile.WriteL(aCharCoder->UnicodeToUtf8L(aMessage->GetName()));
					aFile.WriteL(_L8("'"));
				}

				if(aMessage->GetThread().Length() > 0) {
					aFile.WriteL(_L8(" thread='"));
					aFile.WriteL(aMessage->GetThread());
					aFile.WriteL(_L8("'"));
				}

				if(aMessage->GetLocation().Length() > 0) {
					aFile.WriteL(_L8(" location='"));
					aFile.WriteL(aCharCoder->UnicodeToUtf8L(aMessage->GetLocation()));
					aFile.WriteL(_L8("'"));
				}

				aFile.WriteL(_L8(" at='"));
				aTimeUtilities->EncodeL(aMessage->GetReceivedAt(), aTime);
				aFile.WriteL(aTime);

				aBuf.Format(_L8("' avatar='%d' read='%d' reply='%d' private='%d' own='%d' type='%d' body='"), aMessage->GetAvatarId(), aMessage->GetRead(), aMessage->GetDirectReply(), aMessage->GetPrivate(), aMessage->GetOwn(), aMessage->GetMessageType());
				aFile.WriteL(aBuf);

				aFile.WriteL(aCharCoder->UnicodeToUtf8L(aMessage->GetBody()));
				aFile.WriteL(_L8("'/>"));
			}

			aFile.WriteL(_L8("</discussion></?xml?>"));

			CleanupStack::PopAndDestroy(2); // aTimeUtilities, aCharCoder
			CleanupStack::PopAndDestroy(&aFile);
		}
	}
}

void CDiscussion::DeleteDiscussionFileL() {
	RFs aSession = CCoeEnv::Static()->FsSession();	
	TFileName aFilePath = GetFileName(aSession);

	aSession.Delete(aFilePath);
}

TFileName CDiscussion::GetFileName(RFs& aSession) {
	TFileName aFilePath;	
	TPtr pJid(iJid->Des());	
	
	aFilePath.Append(pJid.Left(20));
	aFilePath.Append(_L(".xml"));
	
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);
	
	return aFilePath;
}

/*
----------------------------------------------------------------------------
--
-- CMessagingManager
--
----------------------------------------------------------------------------
*/

CMessagingManager* CMessagingManager::NewL() {
	CMessagingManager* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CMessagingManager* CMessagingManager::NewLC() {
	CMessagingManager* self = new (ELeave) CMessagingManager();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CMessagingManager::~CMessagingManager() {
	if(iTimer)
		delete iTimer;
	
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		delete iDiscussions[i];
	}
	
	iDiscussions.Close();
}

void CMessagingManager::ConstructL() {	
	iTimer = CCustomTimer::NewL(this);
	iTimer->After(300000000);
}

void CMessagingManager::CompressMessagesL() {
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		iDiscussions[i]->CompressL();
	}
	
	iTimer->After(300000000);
}

CDiscussion* CMessagingManager::GetDiscussionL(const TDesC& aJid) {
	CDiscussion* aDiscussion;
	
	// Find discussion from cache
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		if(aJid.Compare(iDiscussions[i]->GetJid()) == 0) {	
			aDiscussion = iDiscussions[i];
			
			iDiscussions.Remove(i);
			iDiscussions.Insert(aDiscussion, 0);
			
			return aDiscussion;
		}
	}
	
	aDiscussion = CDiscussion::NewLC(aJid);
	iDiscussions.Insert(aDiscussion, 0);
	CleanupStack::Pop(); // aDiscussion
		
	return aDiscussion;
}

void CMessagingManager::DeleteDiscussionL(const TDesC& aJid) {
	// Find discussion from cache
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		if(aJid.Compare(iDiscussions[i]->GetJid()) == 0) {
			iDiscussions[i]->CleanL();
			
			delete iDiscussions[i];
			iDiscussions.Remove(i);
			
			break;
		}
	}
}

void CMessagingManager::TimerExpired(TInt /*aExpiryId*/) {
	CompressMessagesL();	
}
