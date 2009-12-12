/*
============================================================================
 Name        : 	AtomEntryData.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Atom Entry data store
 History     : 	1.0
============================================================================
*/

#ifndef ATOMENTRYDATA_H_
#define ATOMENTRYDATA_H_

#include <e32base.h>
#include "GeolocData.h"
#include "XmppConstants.h"

class CAtomEntryData;

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TEntryContentType {
	EEntryContentPost, EEntryContentNotice, EEntryContentAction
};

enum TEntryLinkType {
	ELinkNone, ELinkWebsite, ELinkUsername, ELinkChannel
};

/*
----------------------------------------------------------------------------
--
-- CEntryLinkPosition
--
----------------------------------------------------------------------------
*/

class CEntryLinkPosition {
	public:
		CEntryLinkPosition(TInt aStart, TInt aLength, TEntryLinkType aType = ELinkWebsite);
	
	public:
		TInt iStart;
		TInt iLength;
		
		TEntryLinkType iType;
};

/*
----------------------------------------------------------------------------
--
-- MAtomEntryObserver
--
----------------------------------------------------------------------------
*/

class MAtomEntryObserver {
	public:
		virtual void EntryRead(CAtomEntryData* aEntry) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CAtomEntryData
--
----------------------------------------------------------------------------
*/

class CAtomEntryData : CBase {
	public:
		static CAtomEntryData* NewL();
		static CAtomEntryData* NewLC();
		virtual ~CAtomEntryData();
		
	private:
		CAtomEntryData();
		void ConstructL();
		
	public:
		void SetObserver(MAtomEntryObserver* aObserver);
		
	public:
		TEntryContentType GetEntryType();
		
	public:
		TTime GetPublishTime();
		void SetPublishTime(TTime aPublishTime);
	
	public:
		TDesC8& GetId();
		void SetIdL(const TDesC8& aId);
		
	public:
		TDesC& GetAuthorName();
		void SetAuthorNameL(const TDesC& aAuthorName);
		
		TDesC& GetAuthorJid();
		void SetAuthorJidL(const TDesC& aAuthorJid, TBool aSensor = true);
		
		TXmppPubsubAffiliation GetAuthorAffiliation();
		void SetAuthorAffiliation(TXmppPubsubAffiliation aAffiliation);		
		
	public:
		CGeolocData* GetLocation();
		
		TDesC& GetContent();
		void SetContentL(const TDesC& aContent, TEntryContentType aEntryType = EEntryContentPost);
		
	public:
		TBool Read();
		void SetRead(TBool aRead);
		
		TBool Highlighted();
		void SetHighlighted(TBool aHighlighted);
		
		TBool Private();
		void SetPrivate(TBool aPrivate);
		
		TBool DirectReply();
		void SetDirectReply(TBool aDirectReply);
		
	public:		
		TInt GetIndexerId();
		void SetIndexerId(TInt aIndexerId);

		TInt GetIconId();
		void SetIconId(TInt aIconId);
		
		TInt GetLinkCount();
		CEntryLinkPosition GetLink(TInt aIndex);
		void AddLink(CEntryLinkPosition aLink);
				
	protected:
		MAtomEntryObserver* iObserver;
		
		TEntryContentType iEntryType;
		TXmppPubsubAffiliation iAffiliation;
		TTime iPublishTime;
		TInt iIndexerId;
		TInt iIconId;

		// Data
		HBufC8* iId;		
		HBufC* iAuthorName;
		HBufC* iAuthorJid;
		HBufC* iContent;
		
		CGeolocData* iLocation;
		
		// Links
		RArray<CEntryLinkPosition> iLinks;
	
		// Flags
		TBool iRead;
		TBool iHighlighted;
		TBool iPrivate;
		TBool iDirectReply;
		
		TPtrC iNullString;
};

#endif /* ATOMENTRYDATA_H_ */
