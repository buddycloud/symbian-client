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
		static void BuildButlerXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aReferenceJid, TInt aOptionsLimit = 10);
		static void BuildButlerXmppStanza(TDes8& aString, TInt aStampId, TReal aPointLatitude, TReal aPointLongitude, TInt aOptionsLimit = 10);
			
		// Broadcaster stanza
		static void BuildBroadcasterXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aNodeId);
		
		// Maitred stanza
		static void BuildMaitredXmppStanza(TDes8& aString, TInt aStampId, const TDesC8& aId,  const TDesC8& aVar);
		
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
		
	public:
		TDesC& GetQueryTitle();
		void SetQueryTitleL(const TDesC& aTitle);
		
	public:
		TDesC8& GetQueriedStanza();
		void SetQueriedStanzaL(const TDesC8& aStanza);
		
	public:
		CFollowingChannelItem* GetQueriedChannel();
		void SetQueriedChannel(CFollowingChannelItem* aChannelItem);
		
	public:
		void ClearResultItems();
		void AppendSortedItem(CExplorerResultItem* aResultItem, TSortByType aSort = ESortByUnsorted);
	
	protected:
		HBufC* iQueryTitle;
		HBufC8* iQueriedStanza;
		
		CFollowingChannelItem* iQueriedChannel;
				
	public:
		TInt iSelectedResultItem;

		RPointerArray<CExplorerResultItem> iResultItems;
};

#endif /* BUDDYCLOUDEXPLORER_H_ */
