/*
============================================================================
 Name        : 	AtomEntryData.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Atom Entry data store
 History     : 	1.0
============================================================================
*/

#include "AtomEntryData.h"


/*
----------------------------------------------------------------------------
--
-- CEntryLinkPosition
--
----------------------------------------------------------------------------
*/

CEntryLinkPosition::CEntryLinkPosition(TInt aStart, TInt aLength, TEntryLinkType aType) {
	iStart = aStart;
	iLength = aLength;
	iType = aType;
}

/*
----------------------------------------------------------------------------
--
-- CAtomEntryData
--
----------------------------------------------------------------------------
*/

CAtomEntryData* CAtomEntryData::NewL() {
	CAtomEntryData* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CAtomEntryData* CAtomEntryData::NewLC() {
	CAtomEntryData* self = new (ELeave) CAtomEntryData();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;	
}

CAtomEntryData::~CAtomEntryData() {
	if(iId) {
		delete iId;
	}
	
	if(iAuthorName) {
		delete iAuthorName;
	}
	
	if(iAuthorJid) {
		delete iAuthorJid;
	}
	
	if(iLocation) {
		delete iLocation;
	}
	
	if(iContent) {
		delete iContent;
	}
	
	iLinks.Close();
}
		
CAtomEntryData::CAtomEntryData() {	
	iObserver = NULL;
	iPublishTime.UniversalTime();
	iIconId = -1;
}

void CAtomEntryData::ConstructL() {
	iId = HBufC8::NewL(0);
	
	iLocation = CGeolocData::NewL();
}

void CAtomEntryData::SetObserver(MAtomEntryObserver* aObserver) {
	iObserver = aObserver;
}

TEntryContentType CAtomEntryData::GetEntryType() {
	return iEntryType;
}

TTime CAtomEntryData::GetPublishTime() {
	return iPublishTime;
}

void CAtomEntryData::SetPublishTime(TTime aPublishTime) {
	iPublishTime = aPublishTime;
}

TDesC8& CAtomEntryData::GetId() {
	return *iId;
}

void CAtomEntryData::SetIdL(const TDesC8& aId) {
	if(iId) {
		delete iId;
	}
	
	iId = aId.AllocL();
}

TDesC& CAtomEntryData::GetAuthorName() {
	if(iAuthorName) {
		return *iAuthorName;
	}
	
	return iNullString;
}

void CAtomEntryData::SetAuthorNameL(const TDesC& aAuthorName) {
	if(iAuthorName) {
		delete iAuthorName;
	}
	
	iAuthorName = aAuthorName.AllocL();
}

TDesC& CAtomEntryData::GetAuthorJid() {
	if(iAuthorJid) {
		return *iAuthorJid;
	}
	
	return iNullString;	
}

void CAtomEntryData::SetAuthorJidL(const TDesC& aAuthorJid, TBool aSensor) {
	if(iAuthorJid) {
		delete iAuthorJid;
	}
	
	iAuthorJid = aAuthorJid.AllocL();
	
	if(GetAuthorName().Length() == 0) {
		SetAuthorNameL(aAuthorJid);
		
		if(aSensor) {
			TPtr aAuthorName(iAuthorName->Des());
			
			for(TInt i = (aAuthorName.Locate('@') - 1), x = (i - 2); i >= 0 && i > x; i--) {
				aAuthorName[i] = 46;
			}
		}
	}
}

TXmppPubsubAffiliation CAtomEntryData::GetAuthorAffiliation() {
	return iAffiliation;
}

void CAtomEntryData::SetAuthorAffiliation(TXmppPubsubAffiliation aAffiliation) {
	iAffiliation = aAffiliation;
}

CGeolocData* CAtomEntryData::GetLocation() {
	return iLocation;
}

TDesC& CAtomEntryData::GetContent() {
	if(iContent) {
		return *iContent;
	}
	
	return iNullString;
}

void CAtomEntryData::SetContentL(const TDesC& aContent, TEntryContentType aEntryType) {
	if(iContent) {
		delete iContent;
		iContent = NULL;
	}
	
	iEntryType = aEntryType;
	
	if(aContent.Length() > 0) {
		if(iEntryType == EEntryContentPost && aContent[0] == TInt('/') && 
				aContent.Length() > 4 && (aContent.Left(3)).Compare(_L("/me")) == 0) {
			
			// Is a /me post
			iEntryType = EEntryContentAction;
			
			TPtrC aAuthorName(iAuthorName->Des());
			TInt aLocate = aAuthorName.Locate('@');
			
			if(aLocate != KErrNotFound) {
				aAuthorName.Set(aAuthorName.Left(aLocate));
			}
			
			iContent = HBufC::NewL(aAuthorName.Length() + aContent.Length() - 3);
			TPtr pContent(iContent->Des());
			pContent.Append(aContent);
			pContent.Replace(0, 3, aAuthorName);
		}
		else {
			iContent = aContent.AllocL();
		}
	}
}

TBool CAtomEntryData::Read() {
	return iRead;
}

void CAtomEntryData::SetRead(TBool aRead) {
	if(!iRead && aRead && iObserver) {
		iObserver->EntryRead(this);
	}
	
	iRead = aRead;
}

TBool CAtomEntryData::Highlighted() {
	return iHighlighted;
}

void CAtomEntryData::SetHighlighted(TBool aHighlighted) {
	iHighlighted = aHighlighted;
}


TBool CAtomEntryData::Private() {
	return iPrivate;
}

void CAtomEntryData::SetPrivate(TBool aPrivate) {
	iPrivate = aPrivate;
}

TBool CAtomEntryData::DirectReply() {
	return iDirectReply;
}

void CAtomEntryData::SetDirectReply(TBool aDirectReply) {
	iDirectReply = aDirectReply;
}

TInt CAtomEntryData::GetIndexerId() {
	return iIndexerId;
}

void CAtomEntryData::SetIndexerId(TInt aIndexerId) {
	iIndexerId = aIndexerId;
}

TInt CAtomEntryData::GetIconId() {
	return iIconId;
}

void CAtomEntryData::SetIconId(TInt aIconId) {
	iIconId = aIconId;
}

TInt CAtomEntryData::GetLinkCount() {
	return iLinks.Count();
}

CEntryLinkPosition CAtomEntryData::GetLink(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iLinks.Count()) {
		return iLinks[aIndex];
	}
	
	return CEntryLinkPosition(0, 0);
}

void CAtomEntryData::AddLink(CEntryLinkPosition aLink) {
	iLinks.Append(aLink);
}

