/*
============================================================================
 Name        : 	BuddycloudExplorer.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Storage types for Buddycloud explorer data
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDEXPLORER_H_
#define BUDDYCLOUDEXPLORER_H_

#include <e32base.h>
#include "BuddycloudConstants.h"
#include "BuddycloudList.h"
#include "BuddycloudFollowing.h"
#include "GeolocData.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TExplorerItemType {
	EExplorerItemUnknown, EExplorerItemDirectory, EExplorerItemPlace, 
	EExplorerItemPerson, EExplorerItemChannel, EExplorerItemLink, EExplorerItemEvent
};

enum TExplorerState {
	EExplorerIdle, EExplorerRequesting
};

/*
----------------------------------------------------------------------------
--
-- CExplorerStanzaBuilder
--
----------------------------------------------------------------------------
*/

class CExplorerStanzaBuilder {
	public:
		// Nearby stanzas
		static void FormatButlerXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aReferenceJid, TInt aOptionsLimit = 10);
		static void FormatButlerXmppStanza(TDes8& aString, TInt aStampId, TReal aPointLatitude, TReal aPointLongitude, TInt aOptionsLimit = 10);
			
		// Broadcaster stanza
		static void FormatBroadcasterXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aNodeId, TInt aRsmMax = 30);
		
		// Maitred stanza
		static void AppendMaitredXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aId,  const TDesC8& aVar);
	
	public:
		static void AppendXmlLangToStanza(TDes8& aString, const TDesC8& aLang);
		
	public:	
		// Title editor
		static void BuildTitleFromResource(TDes& aString, TInt aResourceId, const TDesC& aReplaceData, const TDesC& aWithData);
};

/*
----------------------------------------------------------------------------
--
-- CExplorerResultItem
--
----------------------------------------------------------------------------
*/

class CExplorerResultItem : public CBuddycloudListItem {
	public:
		static CExplorerResultItem* NewL();
		static CExplorerResultItem* NewLC();
		~CExplorerResultItem();
		
	private:
		CExplorerResultItem();
		void ConstructL();
	
	public:
		TExplorerItemType GetResultType();
		void SetResultType(TExplorerItemType aResultType);
		
		TInt GetOverlayId();
		void SetOverlayId(TInt aOverlayId);
		
	public:
		TDesC& GetId();
		void SetIdL(const TDesC& aId);
		
	public:
		TInt StatisticCount();
		TDesC& GetStatistic(TInt aIndex);
		void AddStatisticL(const TDesC& aStatistic);
		
	public:
		TXmppPubsubAffiliation GetChannelAffiliation();
		void SetChannelAffiliation(TXmppPubsubAffiliation aAffiliation);
		
	public:
		CGeolocData* GetGeoloc();
		void SetGeolocL(CGeolocData* aGeoloc);
		
		void UpdateFromGeolocL();
	
	protected:
		TExplorerItemType iResultType;		
		TInt iOverlayId;
		
		HBufC* iId;
		TPtrC iNullString;
		
		RPointerArray<HBufC> iStatistics;
		
		TXmppPubsubAffiliation iAffiliation;
		
		CGeolocData* iGeoloc;
};

/*
----------------------------------------------------------------------------
--
-- CExplorerQueryLevel
--
----------------------------------------------------------------------------
*/

class CExplorerQueryLevel : public CBase {
	public:
		static CExplorerQueryLevel* NewL();
		static CExplorerQueryLevel* NewLC();
		~CExplorerQueryLevel();
	
	private:
		void ConstructL();
		
	public: // Title data
		TDesC& GetQueryTitle();
		void SetQueryTitleL(const TDesC& aTitle);
		
	public: // Stanza data
		TDesC8& GetQueriedStanza();
		void SetQueriedStanzaL(const TDesC8& aStanza);
		
	public: // Channel reference
		CFollowingChannelItem* GetQueriedChannel();
		void SetQueriedChannel(CFollowingChannelItem* aChannelItem);
		
	public: // Result set management
		TBool RsmAllowed();
		void SetRsmAllowed(TBool aRsmAllowed);
		
	public: // Result data
		void ClearResultItems();
		void AppendSortedItem(CExplorerResultItem* aResultItem, TSortByType aSort = ESortByUnsorted);
	
	protected:
		HBufC* iQueryTitle;
		HBufC8* iQueriedStanza;
		
		CFollowingChannelItem* iQueriedChannel;
		
		TBool iRsmAllowed;
				
	public:
		TInt iSelectedResultItem;

		RPointerArray<CExplorerResultItem> iResultItems;
};

#endif /* BUDDYCLOUDEXPLORER_H_ */
