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

#include "TextUtilities.h"
#include "TimeUtilities.h"
#include "XmlParser.h"
#include "XmppUtilities.h"

/*
----------------------------------------------------------------------------
--
-- CXmppEnumerationConverter
--
----------------------------------------------------------------------------
*/

TPresenceSubscription CXmppEnumerationConverter::PresenceSubscription(const TDesC8& aPresenceSubscription) {
	TPresenceSubscription aSubscription = EPresenceSubscriptionNone;

	if(aPresenceSubscription.Compare(KPresenceSubscriptionBoth) == 0) {
		aSubscription = EPresenceSubscriptionBoth;
	}
	else if(aPresenceSubscription.Compare(KPresenceSubscriptionFrom) == 0) {
		aSubscription = EPresenceSubscriptionFrom;
	}
	else if(aPresenceSubscription.Compare(KPresenceSubscriptionTo) == 0) {
		aSubscription = EPresenceSubscriptionTo;
	}
	else if(aPresenceSubscription.Compare(KPresenceSubscriptionRemove) == 0) {
		aSubscription = EPresenceSubscriptionRemove;
	}
	
	return aSubscription;
}

TXmppPubsubAccessModel CXmppEnumerationConverter::PubsubAccessModel(const TDesC8& aPubsubAccessModel) {
	TXmppPubsubAccessModel aAccessModel = EPubsubAccessWhitelist;

	if(aPubsubAccessModel.Compare(KPubsubAccessModelOpen) == 0) {
		aAccessModel = EPubsubAccessOpen;
	}
	
	return aAccessModel;
}

TPtrC8 CXmppEnumerationConverter::PubsubAccessModel(TXmppPubsubAccessModel aPubsubAccessModel) {
	if(aPubsubAccessModel == EPubsubAccessWhitelist) {
		return KPubsubAccessModelWhitelist();
	}
	
	return KPubsubAccessModelOpen();
}

TXmppPubsubSubscription CXmppEnumerationConverter::PubsubSubscription(const TDesC8& aPubsubSubscription) {
	TXmppPubsubSubscription aSubscription = EPubsubSubscriptionNone;

	if(aPubsubSubscription.Compare(KPubsubSubscriptionSubscribed) == 0) {
		aSubscription = EPubsubSubscriptionSubscribed;
	}
	else if(aPubsubSubscription.Compare(KPubsubSubscriptionPending) == 0) {
		aSubscription = EPubsubSubscriptionPending;
	}
	else if(aPubsubSubscription.Compare(KPubsubSubscriptionUnconfigured) == 0) {
		aSubscription = EPubsubSubscriptionUnconfigured;
	}
	
	return aSubscription;
}

TPtrC8 CXmppEnumerationConverter::PubsubSubscription(TXmppPubsubSubscription aPubsubSubscription) {
	if(aPubsubSubscription == EPubsubSubscriptionSubscribed) {
		return KPubsubSubscriptionSubscribed();
	}
	else if(aPubsubSubscription == EPubsubSubscriptionPending) {
		return KPubsubSubscriptionPending();
	}
	else if(aPubsubSubscription == EPubsubSubscriptionUnconfigured) {
		return KPubsubSubscriptionUnconfigured();
	}
	
	return KPubsubSubscriptionNone();
}

TXmppPubsubAffiliation CXmppEnumerationConverter::PubsubAffiliation(const TDesC8& aPubsubAffiliation) {
	TXmppPubsubAffiliation aAffiliation = EPubsubAffiliationNone;

	if(aPubsubAffiliation.Compare(KPubsubAffiliationMember) == 0) {
		aAffiliation = EPubsubAffiliationMember;
	}
	else if(aPubsubAffiliation.Compare(KPubsubAffiliationPublisher) == 0) {
		aAffiliation = EPubsubAffiliationPublisher;
	}
	else if(aPubsubAffiliation.Compare(KPubsubAffiliationModerator) == 0) {
		aAffiliation = EPubsubAffiliationModerator;
	}
	else if(aPubsubAffiliation.Compare(KPubsubAffiliationOwner) == 0) {
		aAffiliation = EPubsubAffiliationOwner;
	}
	else if(aPubsubAffiliation.Compare(KPubsubAffiliationPublishOnly) == 0) {
		aAffiliation = EPubsubAffiliationPublishOnly;
	}
	else if(aPubsubAffiliation.Compare(KPubsubAffiliationOutcast) == 0) {
		aAffiliation = EPubsubAffiliationOutcast;
	}
	
	return aAffiliation;
}

TPtrC8 CXmppEnumerationConverter::PubsubAffiliation(TXmppPubsubAffiliation aPubsubAffiliation) {
	if(aPubsubAffiliation == EPubsubAffiliationMember) {
		return KPubsubAffiliationMember();
	}
	else if(aPubsubAffiliation == EPubsubAffiliationPublisher) {
		return KPubsubAffiliationPublisher();
	}
	else if(aPubsubAffiliation == EPubsubAffiliationModerator) {
		return KPubsubAffiliationModerator();
	}
	else if(aPubsubAffiliation == EPubsubAffiliationOwner) {
		return KPubsubAffiliationOwner();
	}
	else if(aPubsubAffiliation == EPubsubAffiliationPublishOnly) {
		return KPubsubAffiliationPublishOnly();
	}
	else if(aPubsubAffiliation == EPubsubAffiliationOutcast) {
		return KPubsubAffiliationOutcast();
	}

	return KPubsubAffiliationNone();
}

/*
----------------------------------------------------------------------------
--
-- CXmppPubsubNodeParser
--
----------------------------------------------------------------------------
*/

CXmppPubsubNodeParser* CXmppPubsubNodeParser::NewLC(const TDesC8& aNode) {
	CXmppPubsubNodeParser* self = new (ELeave) CXmppPubsubNodeParser();
	CleanupStack::PushL(self);
	self->ConstructL(aNode);
	return self;
}

CXmppPubsubNodeParser::~CXmppPubsubNodeParser() {
	iNodes.Close();
	
	if(iNodeData) {
		delete iNodeData;
	}
}

void CXmppPubsubNodeParser::ConstructL(const TDesC8& aNode) {	
	iNodeData = aNode.AllocL();	
	TPtrC8 pNodeData(iNodeData->Des());
	
	TInt aLocate = KErrNotFound;
	TPtrC8 pNode(aNode);
	
	while((aLocate = pNode.LocateReverse('/')) != KErrNotFound) {
		iNodes.Insert(pNodeData.Mid(aLocate + 1, pNode.Length() - (aLocate + 1)), 0);
		pNode.Set(pNode.Left(aLocate));
	}
	
	if(pNode.Length() > 0) {
		iNodes.Insert(pNodeData.Left(pNode.Length()), 0);
	}
}

TInt CXmppPubsubNodeParser::Count() {
	return iNodes.Count();
}

TPtrC8 CXmppPubsubNodeParser::GetNode(TInt aLevel) {
	if(aLevel < iNodes.Count()) {
		return iNodes[aLevel];
	}
	
	return (iNodeData->Des()).Left(0);
}

/*
----------------------------------------------------------------------------
--
-- CXmppGeolocParser
--
----------------------------------------------------------------------------
*/

CXmppGeolocParser* CXmppGeolocParser::NewLC() {
	CXmppGeolocParser* self = new (ELeave) CXmppGeolocParser();
	CleanupStack::PushL(self);
	return self;
}

CXmppGeolocParser::~CXmppGeolocParser() {
	if(iXml) {
		delete iXml;
	}
}

CGeolocData* CXmppGeolocParser::XmlToGeolocLC(const TDesC8& aStanza) {
	CGeolocData* aGeoloc = CGeolocData::NewLC();
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	do {
		TPtrC8 aElementName = aXmlParser->GetElementName();
		TPtrC aElementData = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData());
		
		if(aElementName.Compare(_L8("uri")) == 0 || aElementName.Compare(_L8("id")) == 0) {
			aGeoloc->SetStringL(EGeolocUri, aElementData);
		}
		else if(aElementName.Compare(_L8("text")) == 0 || aElementName.Compare(_L8("name")) == 0) {
			aGeoloc->SetStringL(EGeolocText, aElementData);
		}
		else if(aElementName.Compare(_L8("street")) == 0) {
			aGeoloc->SetStringL(EGeolocStreet, aElementData);
		}
		else if(aElementName.Compare(_L8("area")) == 0) {
			aGeoloc->SetStringL(EGeolocArea, aElementData);
		}
		else if(aElementName.Compare(_L8("locality")) == 0 || aElementName.Compare(_L8("city")) == 0) {
			aGeoloc->SetStringL(EGeolocLocality, aElementData);
		}
		else if(aElementName.Compare(_L8("postalcode")) == 0) {
			aGeoloc->SetStringL(EGeolocPostalcode, aElementData);
		}
		else if(aElementName.Compare(_L8("region")) == 0) {
			aGeoloc->SetStringL(EGeolocRegion, aElementData);
		}
		else if(aElementName.Compare(_L8("country")) == 0) {
			aGeoloc->SetStringL(EGeolocCountry, aElementData);
		}
		else if(aElementName.Compare(_L8("lat")) == 0) {
			aGeoloc->SetRealL(EGeolocLatitude, aXmlParser->GetRealData());
		}
		else if(aElementName.Compare(_L8("lon")) == 0) {
			aGeoloc->SetRealL(EGeolocLongitude, aXmlParser->GetRealData());
		}
		else if(aElementName.Compare(_L8("accuracy")) == 0) {
			aGeoloc->SetRealL(EGeolocAccuracy, aXmlParser->GetRealData());
		}
	} while(aXmlParser->MoveToNextElement());
	
	CleanupStack::PopAndDestroy(2); // aElementData, aXmlParser
	
	return aGeoloc;
}

TDesC8& CXmppGeolocParser::GeolocToXmlL(CGeolocData* aGeoloc, TBool aBroadOnly) {
	if(iXml) {
		delete iXml;
	}
	
	_LIT8(KGeolocContainer, "<geoloc xmlns='http://jabber.org/protocol/geoloc'></geoloc>");
	_LIT8(KTextContainer, "<text></text>");
	_LIT8(KLocalityContainer, "<locality></locality>");
	_LIT8(KCountryContainer, "<country></country>");
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	HBufC8* aGeolocText = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocText)).AllocLC();
	HBufC8* aGeolocLocality = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocLocality)).AllocLC();
	HBufC8* aGeolocCountry = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocCountry)).AllocLC();
	
	iXml = HBufC8::NewL(KGeolocContainer().Length() + 
			KTextContainer().Length() + aGeolocText->Des().Length() + 
			KLocalityContainer().Length() + aGeolocLocality->Des().Length() + 
			KCountryContainer().Length() + aGeolocCountry->Des().Length());
	TPtr8 aXml(iXml->Des());
	
	if(aGeolocText->Des().Length() > 0 || aGeolocLocality->Des().Length() > 0 || 
			aGeolocCountry->Des().Length() > 0) {
		
		aXml.Append(KGeolocContainer);
		
		if(aGeolocCountry->Des().Length() > 0) {
			aXml.Insert(50, KCountryContainer);
			aXml.Insert(59, *aGeolocCountry);
		}
		
		if(aGeolocLocality->Des().Length() > 0) {
			aXml.Insert(50, KLocalityContainer);
			aXml.Insert(60, *aGeolocLocality);
		}
		
		if(aGeolocText->Des().Length() > 0) {
			aXml.Insert(50, KTextContainer);
			aXml.Insert(56, *aGeolocText);
		}
	}
	
	CleanupStack::PopAndDestroy(4); // aGeolocText, aGeolocLocality, aGeolocCountry, aTextUtilities
	
	return *iXml;
}

/*
----------------------------------------------------------------------------
--
-- CXmppAtomEntryParser
--
----------------------------------------------------------------------------
*/

CXmppAtomEntryParser* CXmppAtomEntryParser::NewLC() {
	CXmppAtomEntryParser* self = new (ELeave) CXmppAtomEntryParser();
	CleanupStack::PushL(self);
	return self;
}

CXmppAtomEntryParser::~CXmppAtomEntryParser() {
	if(iXml) {
		delete iXml;
	}
}

CAtomEntryData* CXmppAtomEntryParser::XmlToAtomEntryLC(const TDesC8& aStanza, TDes8& aReferenceId, TBool aExtended) {
	CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	TEntryContentType aEntryType = EEntryContentPost;
	
	aReferenceId.Zero();
	
	do {
		TPtrC8 aElementName = aXmlParser->GetElementName();
		
		if(aElementName.Compare(_L8("published")) == 0) {
			aAtomEntry->SetPublishTime(CTimeUtilities::DecodeL(aXmlParser->GetStringData()));
		}
		else if(aElementName.Compare(_L8("in-reply-to")) == 0) {
			aReferenceId.Copy(aXmlParser->GetStringData().Left(aReferenceId.MaxLength()));
		}
		else if(aElementName.Compare(_L8("headers")) == 0) {
			// TODO: remove old format
			while(aXmlParser->MoveToElement(_L8("header"))) {
				TPtrC8 aAttributeName = aXmlParser->GetStringAttribute(_L8("name"));
				
				if(aAttributeName.Compare(_L8("In-Reply-To")) == 0) {
					aReferenceId.Copy(aXmlParser->GetStringData().Left(aReferenceId.MaxLength()));
					break;
				}
			}
		}
		else if(aElementName.Compare(_L8("author")) == 0) {
			CXmlParser* aAuthorXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
			
			do {
				aElementName.Set(aAuthorXmlParser->GetElementName());
				
				if(aElementName.Compare(_L8("name")) == 0) {
					aAtomEntry->SetAuthorNameL(aTextUtilities->Utf8ToUnicodeL(aAuthorXmlParser->GetStringData()));
				}
				else if(aElementName.Compare(_L8("jid")) == 0) {
					aAtomEntry->SetAuthorJidL(aTextUtilities->Utf8ToUnicodeL(aAuthorXmlParser->GetStringData()));
				}
				else if(aElementName.Compare(_L8("affiliation")) == 0) {
					aAtomEntry->SetAuthorAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aAuthorXmlParser->GetStringData()));
				}
			} while(aAuthorXmlParser->MoveToNextElement());
			
			CleanupStack::PopAndDestroy(); // aAuthorXmlParser
			
			if(aAtomEntry->GetAuthorJid().Length() == 0) {
				aAtomEntry->SetAuthorJidL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
			}			
		}
		else if(aElementName.Compare(_L8("content")) == 0) {
			aAtomEntry->SetContentL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()), aEntryType);
		}	
		else if(aElementName.Compare(_L8("geoloc")) == 0) {
			CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
			CGeolocData* aGeoloc = aGeolocParser->XmlToGeolocLC(aXmlParser->GetStringData());
			
			aAtomEntry->GetLocation()->SetStringL(EGeolocText, aGeoloc->GetString(EGeolocText));
			aAtomEntry->GetLocation()->SetStringL(EGeolocLocality, aGeoloc->GetString(EGeolocLocality));
			aAtomEntry->GetLocation()->SetStringL(EGeolocCountry, aGeoloc->GetString(EGeolocCountry));
			
			if(aGeoloc->GetString(EGeolocText).Length() == 0) {
				HBufC* aLocation = HBufC::NewLC(aGeoloc->GetString(EGeolocLocality).Length() + 2 + aGeoloc->GetString(EGeolocCountry).Length());
				TPtr pLocation(aLocation->Des());
				aTextUtilities->AppendToString(pLocation, aGeoloc->GetString(EGeolocLocality), _L(""));
				aTextUtilities->AppendToString(pLocation, aGeoloc->GetString(EGeolocCountry), _L(", "));
				
				aAtomEntry->GetLocation()->SetStringL(EGeolocText, pLocation);
				CleanupStack::PopAndDestroy(); // aLocation
			}
			
			CleanupStack::PopAndDestroy(2); // aGeoloc, aGeolocParser
		}
		else if(aElementName.Compare(_L8("star")) == 0) {
			aAtomEntry->SetHighlighted(aXmlParser->GetBoolData());
		}
		else if(aExtended && aElementName.Compare(_L8("x")) == 0) {
			aAtomEntry->SetIdL(aXmlParser->GetStringAttribute(_L8("id")));
			aAtomEntry->SetRead(aXmlParser->GetBoolAttribute(_L8("read")));
			aAtomEntry->SetHighlighted(aXmlParser->GetBoolAttribute(_L8("star")));
			aAtomEntry->SetPrivate(aXmlParser->GetBoolAttribute(_L8("private")));
			aAtomEntry->SetDirectReply(aXmlParser->GetBoolAttribute(_L8("reply")));
			aAtomEntry->SetIconId(aXmlParser->GetIntAttribute(_L8("icon")));
			aAtomEntry->SetAuthorAffiliation((TXmppPubsubAffiliation)aXmlParser->GetIntAttribute(_L8("affiliation")));
			aEntryType = (TEntryContentType)aXmlParser->GetIntAttribute(_L8("type"));
		}
	} while(aXmlParser->MoveToNextElement());
	
	CleanupStack::PopAndDestroy(2); // aElementData, aXmlParser
	
	return aAtomEntry;
}

TDesC8& CXmppAtomEntryParser::AtomEntryToXmlL(CAtomEntryData* aAtomEntry, const TDesC8& aReferenceId, TBool aExtended) {
	if(iXml) {
		delete iXml;
	}
	
	_LIT8(KAtomEntryContainer, "<entry xmlns='http://www.w3.org/2005/Atom'><published></published><author></author><content type='text'></content></entry>");
	_LIT8(KHeaderContainer, "<in-reply-to xmlns='http://buddycloud.com/atom-elements-0'></in-reply-to>");
	_LIT8(KExtendedContainer, "<x id='' read='%d' star='%d' private='%d' reply='%d' icon='%d' affiliation='%d' type='%d'/>");
	_LIT8(KAuthorNameContainer, "<name></name>");
	_LIT8(KAuthorJidContainer, "<jid xmlns='http://buddycloud.com/atom-elements-0'></jid>");
	
	TFormattedTimeDesc aFormatedTime;
	CTimeUtilities::EncodeL(aAtomEntry->GetPublishTime(), aFormatedTime);
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	// Data
	HBufC8* aAtomContent = aTextUtilities->UnicodeToUtf8L(aAtomEntry->GetContent()).AllocLC();
	HBufC8* aAtomAuthorName = aTextUtilities->UnicodeToUtf8L(aAtomEntry->GetAuthorName()).AllocLC();
	HBufC8* aAtomAuthorJid = aTextUtilities->UnicodeToUtf8L(aAtomEntry->GetAuthorJid()).AllocLC();
	
	CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
	TPtrC8 aGeolocXml(aGeolocParser->GeolocToXmlL(aAtomEntry->GetLocation()));
	
	// Extended (stored) data
	HBufC8* aAtomExtended = HBufC8::NewLC(KExtendedContainer().Length() + aAtomEntry->GetId().Length() + 32);
	TPtr8 pAtomExtended(aAtomExtended->Des());
	
	if(aExtended) {
		pAtomExtended.AppendFormat(KExtendedContainer, aAtomEntry->Read(), aAtomEntry->Highlighted(), aAtomEntry->Private(), aAtomEntry->DirectReply(), aAtomEntry->GetIconId(), aAtomEntry->GetAuthorAffiliation(), aAtomEntry->GetEntryType());
		pAtomExtended.Insert(7, aAtomEntry->GetId());
	}
	
	iXml = HBufC8::NewL(KAtomEntryContainer().Length() + aFormatedTime.Length() + aAtomContent->Des().Length() +
			KAuthorNameContainer().Length() + KAuthorJidContainer().Length() + aAtomAuthorName->Des().Length() + 
			aAtomAuthorJid->Des().Length() + aGeolocXml.Length() + KHeaderContainer().Length() + 
			aReferenceId.Length() + pAtomExtended.Length());
	TPtr8 aXml(iXml->Des());
	aXml.Append(KAtomEntryContainer);
	aXml.Insert(114, aGeolocXml);
	aXml.Insert(104, *aAtomContent);
	
	if(aAtomAuthorJid->Des().Length() > 0) {
		aXml.Insert(74, KAuthorJidContainer);
		aXml.Insert(74 + 51, *aAtomAuthorJid);
	}
	
	if(aAtomAuthorName->Des().Length() > 0) {
		aXml.Insert(74, KAuthorNameContainer);
		aXml.Insert(74 + 6, *aAtomAuthorName);
	}
	
	if(aReferenceId.Length() > 0) {
		aXml.Insert(66, KHeaderContainer);
		aXml.Insert(66 + 59, aReferenceId);	
	}
	
	aXml.Insert(54, aFormatedTime);
	aXml.Insert(43, pAtomExtended);
	
	CleanupStack::PopAndDestroy(6); // aAtomExtended, aGeolocParser, aAtomAuthorJid, aAtomAuthorName, aAtomContent, aTextUtilities
	
	return *iXml;
}
