#ifndef BUDDYCLOUDCONSTANTS_H
#define BUDDYCLOUDCONSTANTS_H

/*
----------------------------------------------------------------------------
--
-- Constants
--
----------------------------------------------------------------------------
*/

// View IDs
const TUid KFollowingViewId              = {1};
const TUid KPlacesViewId                 = {2};
const TUid KExplorerViewId               = {3};
const TUid KSetupViewId                  = {4};
const TUid KAccountSettingsViewId        = {5};
const TUid KPreferencesSettingsViewId    = {6};
const TUid KNotificationsSettingsViewId  = {7};
const TUid KBeaconSettingsViewId         = {8};
const TUid KMessagingViewId              = {9};
const TUid KEditPlaceViewId              = {10};
const TUid KEditChannelViewId            = {11};
const TUid KChannelInfoViewId            = {12};
const TUid KCommunitiesViewId            = {13};

// String constants
_LIT8(KBuddycloudPubsubServer, "pubsub-bridge@broadcaster.buddycloud.com");

_LIT8(KIqTypeSet, "set");
_LIT8(KIqTypeGet, "get");

_LIT8(KPlaceAdd, "add");
_LIT8(KPlaceEdit, "edit");
_LIT8(KPlaceCreate, "create");
_LIT8(KPlaceSearch, "search");
_LIT8(KPlaceRemove, "remove");
_LIT8(KPlaceDelete, "delete");
_LIT8(KPlaceCurrent, "current");

_LIT8(KTaggerFlag, "flag");
_LIT8(KTaggerTag, "tag");

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TBuddycloudXmppIdEnumerations {
	EXmppIdRegistration = 1, EXmppIdSetCredentials, EXmppIdGetNearbyPlaces, 
	// Place id's
	EXmppIdGetPlaceSubscriptions, EXmppIdAddPlaceSubscription, EXmppIdRemovePlaceSubscription, EXmppIdDeletePlace, 
	EXmppIdSetCurrentPlace, EXmppIdCreatePlace, EXmppIdSearchForPlace, EXmppIdGetPlaceDetails, EXmppEditPlaceDetails, 
	// Pubsub id's
	EXmppIdGetPubsubSubscriptions, EXmppIdGetUsersPubsubNodeAffiliations, EXmppIdPublishMood, EXmppIdPublishFuturePlace, 
	EXmppIdCreateChannel, EXmppIdPublishChannelPost, EXmppIdChangeChannelAffiliation, EXmppIdAddChannelSubscription, 
	EXmppIdGetChannelMetadata, EXmppIdRequestMediaPost,
	// Explorer id's
	EXmppIdGetDirectories, EXmppIdGetNearbyObjects, EXmppIdGetNodeAffiliations, EXmppIdGetMaitredList
};

#endif
