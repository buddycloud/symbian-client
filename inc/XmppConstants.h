/*
============================================================================
 Name        : 	XmppConstants.h
 Author      : 	Ross Savage
 Copyright   : 	2009 Buddycloud
 Description : 	XMPP Constants & Enumerations
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef XMPPCONSTANTS_H_
#define XMPPCONSTANTS_H_

/*
----------------------------------------------------------------------------
--
-- Constants
--
----------------------------------------------------------------------------
*/

static const TInt READ_INTERVAL = 25000;

_LIT8(KPresenceSubscriptionBoth, "both");
_LIT8(KPresenceSubscriptionFrom, "from");
_LIT8(KPresenceSubscriptionTo, "to");
_LIT8(KPresenceSubscriptionRemove, "remove");

_LIT8(KPubsubAccessModelOpen, "open");
_LIT8(KPubsubAccessModelWhitelist, "whitelist");

_LIT8(KPubsubSubscriptionNone, "none");
_LIT8(KPubsubSubscriptionPending, "pending");
_LIT8(KPubsubSubscriptionUnconfigured, "unconfigured");
_LIT8(KPubsubSubscriptionSubscribed, "subscribed");

_LIT8(KPubsubAffiliationOutcast, "outcast");
_LIT8(KPubsubAffiliationNone, "none");
_LIT8(KPubsubAffiliationMember, "member");
_LIT8(KPubsubAffiliationPublishOnly, "publish-only");
_LIT8(KPubsubAffiliationPublisher, "publisher");
_LIT8(KPubsubAffiliationModerator, "moderator");
_LIT8(KPubsubAffiliationOwner, "owner");

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

// Outbox message priority

enum TXmppMessagePriority {
	EXmppPriorityNormal, EXmppPriorityHigh
};

// Stream features

enum TXmppStreamFeatures {
	EStreamFeatureNone = 0, EStreamFeatureBind = 2, 
	EStreamFeaturePlain = 4, EStreamFeatureDigestMd5 = 8, EStreamFeatureAnonymous = 16,
	EStreamFeatureTls = 32, EStreamFeatureTlsRequired = 64, 
	EStreamFeatureCompression = 128, EStreamFeatureRegistration = 256
};

// Presence subscription enumerations

enum TPresenceSubscription {
	EPresenceSubscriptionUnknown, EPresenceSubscriptionRemove, EPresenceSubscriptionNone, 
	EPresenceSubscriptionTo, EPresenceSubscriptionFrom, EPresenceSubscriptionBoth
};

// Pubsub enumerations

enum TXmppPubsubAccessModel {
	EPubsubAccessOpen, EPubsubAccessWhitelist
};

enum TXmppPubsubAffiliation {
	EPubsubAffiliationOutcast = -1, EPubsubAffiliationNone = 0, EPubsubAffiliationMember, 
	EPubsubAffiliationPublishOnly, EPubsubAffiliationPublisher, EPubsubAffiliationModerator, EPubsubAffiliationOwner
};

enum TXmppPubsubSubscription {
	EPubsubSubscriptionUnknown, EPubsubSubscriptionNone, EPubsubSubscriptionPending, 
	EPubsubSubscriptionUnconfigured, EPubsubSubscriptionSubscribed
};

enum TXmppPubsubEventType {
	EPubsubEventNone, EPubsubEventItems, EPubsubEventSubscription, 
	EPubsubEventAffiliation, EPubsubEventPurge, EPubsubEventDelete
};

enum TXmppPubsubItemType {
	EPubsubItemAtom, EPubsubItemGeoloc, EPubsubItemMood
};

#endif /* XMPPCONSTANTS_H_ */
