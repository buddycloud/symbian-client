/*
============================================================================
 Name        : 	DiscussionManager.cpp
 Author      : 	Ross Savage
 Copyright   : 	2009 Buddycloud
 Description : 	Definition & management of discussions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <coemain.h>
#include <s32file.h>
#include "DiscussionManager.h"
#include "FileUtilities.h"
#include "XmlParser.h"
#include "XmppUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CThreadedEntry
--
----------------------------------------------------------------------------
*/

CThreadedEntry* CThreadedEntry::NewL(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver) {
	CThreadedEntry* self = NewLC(aEntry, aObserver);
	CleanupStack::Pop(self);
	return self;
}

CThreadedEntry* CThreadedEntry::NewLC(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver) {
	CThreadedEntry* self = new (ELeave) CThreadedEntry(aEntry, aObserver);
	CleanupStack::PushL(self);
	return self;	
}

CThreadedEntry::~CThreadedEntry() {
	if(iEntry) {
		delete iEntry;
	}
	
	// Comments
	for(TInt i = 0; i < iComments.Count(); i++) {
		delete iComments[i];
	}
	
	iComments.Close();
}

CThreadedEntry::CThreadedEntry(CAtomEntryData* aEntry, MThreadedEntryObserver* aObserver) {
	iEntry = aEntry;
	iObserver = aObserver;
}

CAtomEntryData* CThreadedEntry::GetEntry() {
	return iEntry;
}
	
TInt CThreadedEntry::CommentCount() {
	return iComments.Count();
}
		
TBool CThreadedEntry::AddCommentLD(CAtomEntryData* aComment) {
	TInt aInsertIndex = iComments.Count();
	
	for(TInt i = (iComments.Count() - 1); i >= 0; i--) {
		// Compare id's
		if(iComments[i]->GetId().Compare(aComment->GetId()) == 0) {
			CleanupStack::PopAndDestroy(); // aComment
			
			return false;
		}
		
		if(iComments[i]->GetPublishTime() > aComment->GetPublishTime()) {
			// Update insertion point
			aInsertIndex = i;
		}
		else {
			break;
		}
	}
	
	// Append comment
	iComments.Insert(aComment, aInsertIndex);
	CleanupStack::Pop(aComment);
	
	if(iObserver) {
		iObserver->EntryAdded(aComment);
	}
	
	return true;
}

TBool CThreadedEntry::DeleteAllComments() {
	while(iComments.Count() > 0) {
		DeleteCommentByIndex(0);
	}

	iComments.Reset();
	
	return true;
}
		
TBool CThreadedEntry::DeleteCommentByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iComments.Count()) {
		if(iObserver) {
			iObserver->EntryDeleted(iComments[aIndex]);
		}
		
		delete iComments[aIndex];
		iComments.Remove(aIndex);
		
		return true;
	}
	
	return false;
}

TBool CThreadedEntry::DeleteCommentById(const TDesC8& aId) {
	for(TInt i = 0; i < iComments.Count(); i++) {
		if(iComments[i]->GetId().Compare(aId) == 0) {
			return DeleteCommentByIndex(i);
		}
	}
	
	return false;
}
		
CAtomEntryData* CThreadedEntry::GetCommentByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iComments.Count()) {
		return iComments[aIndex];
	}
	
	return NULL;
}

CAtomEntryData* CThreadedEntry::GetCommentById(const TDesC8& aId) {
	for(TInt i = 0; i < iComments.Count(); i++) {
		if(iComments[i]->GetId().Compare(aId) == 0) {
			return iComments[i];
		}
	}
	
	return NULL;
}

/*
----------------------------------------------------------------------------
--
-- CDiscussionManager
--
----------------------------------------------------------------------------
*/

CDiscussion* CDiscussion::NewL(const TDesC& aDiscussionId) {
	CDiscussion* self = NewLC(aDiscussionId);
	CleanupStack::Pop(self);
	return self;
}

CDiscussion* CDiscussion::NewLC(const TDesC& aDiscussionId) {
	CDiscussion* self = new (ELeave) CDiscussion();
	CleanupStack::PushL(self);
	self->ConstructL(aDiscussionId);
	return self;	
}

CDiscussion::~CDiscussion() {
	WriteDiscussionToFileL();

	if(iDiscussionId) {
		delete iDiscussionId;
	}
	
	// Entries
	for(TInt i = 0; i < iEntries.Count(); i++) {
		delete iEntries[i];
	}
	
	iEntries.Close();
}
		
CDiscussion::CDiscussion() {
	iNotify = true;
	iItemId = KErrNotFound;
	iUnreadEntries = 0;
	iUnreadReplies = 0;
}

void CDiscussion::ConstructL(const TDesC& aDiscussionId) {
	iDiscussionId = aDiscussionId.AllocL();
}

TInt CDiscussion::GetUnreadEntries() {
	return iUnreadEntries;
}

TInt CDiscussion::GetUnreadReplies() {
	return iUnreadReplies;
}

void CDiscussion::SetUnreadData(TInt aEntries, TInt aReplies) {
	iUnreadEntries = aEntries;
	iUnreadReplies = aReplies;
}

void CDiscussion::SetDiscussionReadObserver(MDiscussionReadObserver* aObserver, TInt aItemId) {
	iDiscussionReadObserver = aObserver;
	iItemId = aItemId;
}

void CDiscussion::SetDiscussionUpdateObserver(MDiscussionUpdateObserver* aObserver) {
	ReadDiscussionToMemoryL();

	iDiscussionUpdateObserver = aObserver;
}

TDesC& CDiscussion::GetDiscussionId(){
	return *iDiscussionId;
}

void CDiscussion::SetDiscussionIdL(const TDesC& aDiscussionId) {
	// Rename existing cache file
	RFs aSession = CCoeEnv::Static()->FsSession();
	TFileName aOldFilePath = GetFileName(aSession);
	
	if(iDiscussionId) {
		delete iDiscussionId;
	}
	
	iDiscussionId = aDiscussionId.AllocL();
	
	TFileName aNewFilePath = GetFileName(aSession);
	
	if(aNewFilePath.Compare(aOldFilePath) != 0) {
		aSession.Rename(aOldFilePath, aNewFilePath);
	}
}

TInt CDiscussion::GetItemId() {
	return iItemId;
}

TBool CDiscussion::Notify() {
	ReadDiscussionToMemoryL();
	
	return iNotify;
}

void CDiscussion::SetNotify(TBool aNotify) {
	iNotify = aNotify;
}

void CDiscussion::CompressL(TBool aForced) {
	if(iDiscussionInMemory) {
		TTime aNow;
		aNow.UniversalTime();			
		
		if(aForced || (iLastUpdate + TTimeIntervalSeconds(450)) <= aNow) {
			// Save to file if older
			WriteDiscussionToFileL();
			
			if(iDiscussionUpdateObserver == NULL) {			
				// Free memory
				for(TInt i = 0; i < iEntries.Count(); i++) {
					delete iEntries[i];
				}

				iEntries.Reset();

				iDiscussionInMemory = false;
				iDiscussionIndexer = 0;
				
#ifdef _DEBUG
				if(iDiscussionReadObserver) {
					TBuf8<256> aBuf;
					aBuf.Format(_L8("DISC  Discussion %d (%d) saved to file and uncached"), iItemId, aForced);
					iDiscussionReadObserver->DiscussionDebug(aBuf);
				}
#endif
			}
		}
	}
}

void CDiscussion::CleanL() {
	DeleteDiscussionFileL();
	DeleteAllEntries();
	
	iDiscussionInMemory = false;
}

TInt CDiscussion::EntryCount() {
	return iEntries.Count();
}

CThreadedEntry* CDiscussion::GetThreadedEntryByIndex(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iEntries.Count()) {
		return iEntries[aIndex];
	}

	return NULL;
}

CThreadedEntry* CDiscussion::GetThreadedEntryById(const TDesC8& aEntryId) {
	if(aEntryId.Length() > 0) {
		ReadDiscussionToMemoryL();
	
		for(TInt i = 0; i < iEntries.Count(); i++) {
			if(aEntryId.Compare(iEntries[i]->GetEntry()->GetId()) == 0) {
				return iEntries[i];
			}
		}
	}
	
	return NULL;
}

TBool CDiscussion::DeleteEntryByIndex(TInt aIndex) {
	ReadDiscussionToMemoryL();

	if(aIndex >= 0 && aIndex <= iEntries.Count()) {		
		// Comments deleted
		iEntries[aIndex]->DeleteAllComments();
		
		// Entry deleted
		EntryDeleted(iEntries[aIndex]->GetEntry());		
		
		// Delete
		delete iEntries[aIndex];
		iEntries.Remove(aIndex);
		
		return true;
	}
	
	return false;
}

TBool CDiscussion::DeleteEntryById(const TDesC8& aEntryId) {
	if(aEntryId.Length() > 0) {
		ReadDiscussionToMemoryL();
	
		for(TInt i = 0; i < iEntries.Count(); i++) {
			if(aEntryId.Compare(iEntries[i]->GetEntry()->GetId()) == 0) {
				return DeleteEntryByIndex(i);
			}
			else if(iEntries[i]->DeleteCommentById(aEntryId)) {
				return true;
			}
		}
	}
	
	return false;
}

TBool CDiscussion::DeleteAllEntries() {
	ReadDiscussionToMemoryL();
	
	while(iEntries.Count() > 0) {
		DeleteEntryByIndex(0);
	}

	iEntries.Reset();
	
	return true;
}

TBool CDiscussion::AddEntryOrCommentLD(CAtomEntryData* aEntry, const TDesC8& aEntryReference) {
	if(aEntry->GetContent().Length() > 0) {		
		ReadDiscussionToMemoryL();
		
		TInt aInsertIndex = iEntries.Count();
		
		// Set indexer
		aEntry->SetIndexerId(iDiscussionIndexer++);
		
		if(aEntry->GetId().Length() == 0 && aEntryReference.Length() == 0) {
			// Append entry
			iEntries.Append(CThreadedEntry::NewL(aEntry, this));
			CleanupStack::Pop(aEntry);
			
			EntryAdded(aEntry);
		
			return true;
		}
		else {
			for(TInt i = (iEntries.Count() - 1); i >= 0; i--) {
				// Compare references
				if(aEntryReference.Length() > 0) {
					// Find entry reference
					if(aEntryReference.Compare(iEntries[i]->GetEntry()->GetId()) == 0) {
						// Reference entry found
						return iEntries[i]->AddCommentLD(aEntry);
					}
				}
				else {
					if(aEntry->GetId().Compare(iEntries[i]->GetEntry()->GetId()) == 0) {			
						// Repeated entry
						CleanupStack::PopAndDestroy(); // aEntry
						
						return false;
					}
					
					if(iEntries[i]->GetEntry()->GetPublishTime() > aEntry->GetPublishTime()) {
						// Update insertion point
						aInsertIndex = i;
					}
					else {
						break;
					}
				}
			}			
		}
		
		if(aEntryReference.Length() > 0) {
			// Entry reference not found
			CleanupStack::PopAndDestroy(); // aEntry
			
			return false;		
		}
				
		// Append entry
		iEntries.Insert(CThreadedEntry::NewL(aEntry, this), aInsertIndex);
		CleanupStack::Pop(aEntry);
		
		EntryAdded(aEntry);
		
		return true;
	}
	else {
		// No entry content
		CleanupStack::PopAndDestroy(); // aEntry
		
		return false;		
	}
}

void CDiscussion::ParseEntryForLinksL(CAtomEntryData* aEntry) {
	if(aEntry->GetEntryType() < EEntryContentAction) {
		TPtrC aContent(aEntry->GetContent());
		TInt aGlobalPosition = 0;
		
		TInt aLinkSearch = aContent.MatchF(_L("*http*://*"));
		TInt aWwwSearch = aContent.FindF(_L("www."));
		TInt aUsernameSearch = aContent.Locate('@');
		TInt aChannelSearch = aContent.Locate('#');
		
		while(aLinkSearch != KErrNotFound || aWwwSearch != KErrNotFound || 
				aUsernameSearch != KErrNotFound || aChannelSearch != KErrNotFound) {	
			
			TEntryLinkType aLinkType = ELinkWebsite;
			TInt aMinLinkLength = 10;
	
			// Check result against www
			if(aWwwSearch != KErrNotFound && (aWwwSearch < aLinkSearch || aLinkSearch == KErrNotFound)) {
				aLinkSearch = aWwwSearch;
				aMinLinkLength = 7;
			}
			
			// Check '@' result
			if(aUsernameSearch != KErrNotFound && (aUsernameSearch < aLinkSearch || aLinkSearch == KErrNotFound)) {
				aLinkSearch = aUsernameSearch + 1;
				aLinkType = ELinkNone;
				
				TInt aLocate = (aContent.Left(aUsernameSearch).LocateReverse(' ') + 1);		
			
				if(aLocate < aUsernameSearch) {			
					aLinkSearch = aLocate;
					aLinkType = ELinkUsername;
					aMinLinkLength = 5;
				}	
			}
			
			// Check '#' result
			if(aChannelSearch != KErrNotFound && (aChannelSearch <= aLinkSearch || aLinkSearch == KErrNotFound)) {
				aLinkSearch = aChannelSearch;
				aMinLinkLength = 2;
				
				if(aLinkType != ELinkUsername) {
					aLinkType = ELinkChannel;
				}
			}
			
			aGlobalPosition += aLinkSearch;
			aContent.Set(aContent.Mid(aLinkSearch));		
			TPtrC pLink(aContent);
			
			if((aLinkSearch = aContent.Locate(' ')) != KErrNotFound) {
				pLink.Set(aContent.Left(aLinkSearch));			
			}
			
			TInt aPreTrimLinkLength = pLink.Length();
			
			if(aLinkType != ELinkNone && (aGlobalPosition == 0 || aEntry->GetContent()[aGlobalPosition - 1] == ' ')) {		
				while((aLinkType != ELinkWebsite && 
								// Non website links
								((aLinkSearch = pLink.Locate(':')) != KErrNotFound || 
								(aLinkSearch = pLink.Locate(',')) != KErrNotFound)) ||
						// General
						(aLinkSearch = pLink.Locate('\'')) != KErrNotFound || 
						(aLinkSearch = pLink.Locate('\"')) != KErrNotFound || 
						(aLinkSearch = pLink.Locate('>')) != KErrNotFound || 
						(aLinkSearch = pLink.Locate(']')) != KErrNotFound || 
						((aLinkSearch = pLink.LocateReverse(')')) != KErrNotFound && aLinkSearch == pLink.Length() - 1) || 
						((aLinkSearch = pLink.LocateReverse('.')) != KErrNotFound && aLinkSearch == pLink.Length() - 1) ||
						((aLinkSearch = pLink.LocateReverse('?')) != KErrNotFound && aLinkSearch == pLink.Length() - 1) ||
						((aLinkSearch = pLink.LocateReverse('!')) != KErrNotFound && aLinkSearch == pLink.Length() - 1)) {
					
					pLink.Set(pLink.Left(aLinkSearch));
				}
				
				if(pLink.Length() >= aMinLinkLength) {
					aEntry->AddLink(CEntryLinkPosition(aGlobalPosition, pLink.Length(), aLinkType));
				}
			}
			
			aGlobalPosition += aPreTrimLinkLength;
			aContent.Set(aContent.Mid(aPreTrimLinkLength));	
			
			aLinkSearch = aContent.MatchF(_L("*http*://*"));
			aWwwSearch = aContent.FindF(_L("www."));
			aUsernameSearch = aContent.Locate('@');
			aChannelSearch = aContent.Locate('#');
		}
	}
}

void CDiscussion::EntryRead(CAtomEntryData* aEntry) {
	TInt aLastUnread = iUnreadEntries;
	
	iLastUpdate.UniversalTime();

	if(!aEntry->Read()) {
		iUnreadEntries--;
		
		if(aEntry->DirectReply()) {
			iUnreadReplies--;
		}
	}
	
	if(iUnreadEntries <= 0) {
		iUnreadEntries = 0;
		iUnreadReplies = 0;
		
		if(iDiscussionReadObserver && aLastUnread != iUnreadEntries) {
			iDiscussionReadObserver->DiscussionRead(*iDiscussionId, iItemId);
		}
	}
}

void CDiscussion::EntryAdded(CAtomEntryData* aEntry) {
	iLastUpdate.UniversalTime();
	
	aEntry->SetObserver(this);

	// Set counters
	if(!aEntry->Read()) {
		iUnreadEntries++;
		
		if(aEntry->DirectReply()) {
			iUnreadReplies++;
		}
	}
	
	// Remove old posts
	while(iEntries.Count() > 50) {
		DeleteEntryByIndex(0);
	}
	
	// Parse links
	ParseEntryForLinksL(aEntry);
	
	if(iDiscussionUpdateObserver) {
		iDiscussionUpdateObserver->EntryAdded(aEntry);
	}	
}

void CDiscussion::EntryDeleted(CAtomEntryData* aEntry) {
	EntryRead(aEntry);
	
	if(iDiscussionUpdateObserver) {
		iDiscussionUpdateObserver->EntryDeleted(aEntry);
	}	
}


void CDiscussion::ReadDiscussionToMemoryL() {
	if(!iDiscussionInMemory) {
		RFs aSession = CCoeEnv::Static()->FsSession();	
		TFileName aFilePath = GetFileName(aSession);
		
		RFile aFile;		
		TInt aFileSize;
		
		iUnreadEntries = 0;
		iUnreadReplies = 0;
		
		iDiscussionInMemory = true;
		
		if(aFile.Open(aSession, aFilePath, EFileStreamText|EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);
			aFile.Size(aFileSize);

			// Create buffer & read file
			HBufC8* aBuf = HBufC8::NewLC(aFileSize);
			TPtr8 pBuf(aBuf->Des());
			aFile.Read(pBuf);

			CXmlParser* aXmlParser = CXmlParser::NewLC(*aBuf);

			if(aXmlParser->MoveToElement(_L8("discussion"))) {
				// Buzz
				iNotify = aXmlParser->GetBoolAttribute(_L8("notify"));
				
				CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();
				TBuf8<32> aReferenceId;
				
				while(aXmlParser->MoveToElement(_L8("entry"))) {
					CAtomEntryData* aAtomEntry = aAtomEntryParser->XmlToAtomEntryLC(aXmlParser->GetStringData(), aReferenceId, true);
					
					AddEntryOrCommentLD(aAtomEntry, aReferenceId);
				}
				
				CleanupStack::PopAndDestroy(); // CXmppAtomEntryParser
			}

			CleanupStack::PopAndDestroy(2); // aXmlParser, aBuf
			CleanupStack::PopAndDestroy(&aFile);
			
#ifdef _DEBUG
			if(iDiscussionReadObserver) {
				TBuf8<256> aBuf;
				aBuf.Format(_L8("DISC  Discussion %d cached to memory: %d bytes"), iItemId, aFileSize);
				iDiscussionReadObserver->DiscussionDebug(aBuf);
			}
#endif
		}
	}
}

void CDiscussion::WriteDiscussionToFileL() {
	if(iDiscussionInMemory) {
		RFs aSession = CCoeEnv::Static()->FsSession();	
		TFileName aFilePath = GetFileName(aSession);

		RFileWriteStream aFile;
		TBuf8<128> aBuf;
	
		if(aFile.Replace(aSession, aFilePath, EFileStreamText|EFileWrite) == KErrNone) {
			CleanupClosePushL(aFile);

			CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();
			
			aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>\r\n"));		
			aBuf.Format(_L8("\t<discussion notify='%d'>\r\n"), iNotify);
			aFile.WriteL(aBuf);

			for(TInt i = 0; i < iEntries.Count(); i++) {
				CThreadedEntry* aThread = iEntries[i];			
				CAtomEntryData* aEntry = aThread->GetEntry();	
				
				aFile.WriteL(_L8("\t\t"));						
				aFile.WriteL(aAtomEntryParser->AtomEntryToXmlL(aEntry, KNullDesC8, true));
				aFile.WriteL(_L8("\r\n"));						
				
				if(aThread->CommentCount() > 0) {
					aFile.WriteL(_L8("\t\t<comments>\r\n"));	
					
					for(TInt x = 0; x < aThread->CommentCount(); x++) {
						CAtomEntryData* aComment = aThread->GetCommentByIndex(x);
						
						aFile.WriteL(_L8("\t\t\t"));						
						aFile.WriteL(aAtomEntryParser->AtomEntryToXmlL(aComment, aEntry->GetId(), true));
						aFile.WriteL(_L8("\r\n"));						
					}
					
					aFile.WriteL(_L8("\t\t</comments>\r\n"));	
				}
			}

			aFile.WriteL(_L8("\t</discussion>\r\n</?xml?>"));

			CleanupStack::PopAndDestroy(); // CXmppAtomEntryParser
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
	TPtr pId(iDiscussionId->Des());
	TInt aLocate;
	
	aFilePath.Append(pId.Left(aFilePath.MaxLength() - 24));
	aFilePath.Append(_L(".xml"));
	
	while((aLocate = aFilePath.Locate('/')) != KErrNotFound) {
		aFilePath[aLocate] = TChar('~');
	}
	
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);
	
	return aFilePath;
}

/*
----------------------------------------------------------------------------
--
-- CDiscussionManager
--
----------------------------------------------------------------------------
*/

CDiscussionManager* CDiscussionManager::NewL() {
	CDiscussionManager* self = new (ELeave) CDiscussionManager();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);	
	return self;	
}

CDiscussionManager::~CDiscussionManager() {
	if(iTimer)
		delete iTimer;
	
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		delete iDiscussions[i];
	}
	
	iDiscussions.Close();
}

void CDiscussionManager::ConstructL() {	
	iTimer = CCustomTimer::NewL(this);
	iTimer->After(300000000);
}

CDiscussion* CDiscussionManager::GetDiscussionL(const TDesC& aId) {
	TBool aCompressionNeeded = CompressionNeeded();
	CDiscussion* aDiscussion;
	
	// Find discussion from cache
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		if(aId.Compare(iDiscussions[i]->GetDiscussionId()) == 0) {	
			aDiscussion = iDiscussions[i];
			
			iDiscussions.Remove(i);
			iDiscussions.Insert(aDiscussion, 0);
			
			return aDiscussion;
		}
		else if(aCompressionNeeded) {
			iDiscussions[i]->CompressL(true);
			
			aCompressionNeeded = CompressionNeeded();
		}		
	}
	
	aDiscussion = CDiscussion::NewLC(aId);
	iDiscussions.Insert(aDiscussion, 0);
	CleanupStack::Pop(); // aDiscussion
		
	return aDiscussion;
}

void CDiscussionManager::DeleteDiscussionL(const TDesC& aId) {
	// Find discussion from cache
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		if(aId.Compare(iDiscussions[i]->GetDiscussionId()) == 0) {
			iDiscussions[i]->CleanL();
			
			delete iDiscussions[i];
			iDiscussions.Remove(i);
			
			break;
		}
	}
}

void CDiscussionManager::CompressDiscussionL(TBool aForced) {
	for(TInt i = 0; i < iDiscussions.Count(); i++) {
		iDiscussions[i]->CompressL(aForced);
	}
	
	iTimer->After(300000000);
}

void CDiscussionManager::TimerExpired(TInt /*aExpiryId*/) {
	CompressDiscussionL(CompressionNeeded());	
}

TBool CDiscussionManager::CompressionNeeded() {
	RHeap aHeap = User::Heap();
	TReal aMaxHeapSize(aHeap.MaxLength());
	TReal aHeapSize(aHeap.Size());
	
	if(((100.0 / aMaxHeapSize) * aHeapSize) > 85.0) {
		return true;
	}
	
	return false;
}
