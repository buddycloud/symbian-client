/*
============================================================================
 Name        : 	XmppUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	XMPP Pubsub utilities
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPUTILITIES_H_
#define XMPPUTILITIES_H_

// INCLUDES
#include <e32base.h>
#include "AtomEntryData.h"
#include "GeolocData.h"
#include "XmppConstants.h"

/*
----------------------------------------------------------------------------
--
-- CXmppEnumerationConverter
--
----------------------------------------------------------------------------
*/

class CXmppEnumerationConverter {
	public:
		static TPresenceSubscription PresenceSubscription(const TDesC8& aPresenceSubscription);
		
	public:
		static TXmppPubsubAccessModel PubsubAccessModel(const TDesC8& aPubsubAccessModel);
		static TPtrC8 PubsubAccessModel(TXmppPubsubAccessModel aPubsubAccessModel);
		
	public:
		static TXmppPubsubSubscription PubsubSubscription(const TDesC8& aPubsubSubscription);
		static TPtrC8 PubsubSubscription(TXmppPubsubSubscription aPubsubSubscription);
		
	public:
		static TXmppPubsubAffiliation PubsubAffiliation(const TDesC8& aPubsubAffiliation);
		static TPtrC8 PubsubAffiliation(TXmppPubsubAffiliation aPubsubAffiliation);
};

/*
----------------------------------------------------------------------------
--
-- CXmppPubsubNodeParser
--
----------------------------------------------------------------------------
*/

class CXmppPubsubNodeParser : public CBase {
	public:
		static CXmppPubsubNodeParser* NewLC(const TDesC8& aNode);
		~CXmppPubsubNodeParser();

	private:
		void ConstructL(const TDesC8& aNode);
		
	public:
		TInt Count();
		TPtrC8 GetNode(TInt aLevel);
		
	protected:
		HBufC8* iNodeData;
		
		RArray<TPtrC8> iNodes;
};

/*
----------------------------------------------------------------------------
--
-- CXmppGeolocParser
--
----------------------------------------------------------------------------
*/

class CXmppGeolocParser : public CBase {
	public:
		static CXmppGeolocParser* NewLC();
		~CXmppGeolocParser();
	
	public:
		CGeolocData* XmlToGeolocLC(const TDesC8& aStanza);
		TDesC8& GeolocToXmlL(CGeolocData* aGeoloc, TBool aBroadOnly = false);
		
	protected:
		HBufC8* iXml;		
};

/*
----------------------------------------------------------------------------
--
-- CXmppAtomEntryParser
--
----------------------------------------------------------------------------
*/

class CXmppAtomEntryParser : public CBase {
	public:
		static CXmppAtomEntryParser* NewLC();
		~CXmppAtomEntryParser();
	
	public:
		CAtomEntryData* XmlToAtomEntryLC(const TDesC8& aStanza, TDes8& aReferenceId, TBool aExtended = false);
		TDesC8& AtomEntryToXmlL(CAtomEntryData* aAtomEntry, const TDesC8& aReferenceId, TBool aExtended = false);
		
	protected:
		HBufC8* iXml;
};

#endif /* XMPPUTILITIES_H_ */
