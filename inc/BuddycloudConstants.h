#ifndef BUDDYCLOUDCONSTANTS_H
#define BUDDYCLOUDCONSTANTS_H

// Constants

// View IDs
const TUid KFollowingViewId              = {1};
const TUid KPlacesViewId                 = {2};
const TUid KChannelsViewId               = {3};
const TUid KExplorerViewId               = {4};
const TUid KSetupViewId                  = {5};
const TUid KAccountSettingsViewId        = {6};
const TUid KPreferencesSettingsViewId    = {7};
const TUid KNotificationsSettingsViewId  = {8};
const TUid KBeaconSettingsViewId         = {9};
const TUid KMessagingViewId              = {10};
const TUid KEditPlaceViewId              = {11};
const TUid KCommunitiesViewId            = {12};

// String constants
_LIT(KBuddycloudRosterServer, "@buddycloud.com");
_LIT(KBuddycloudChannelsServer, "@channels.buddycloud.com");

_LIT8(KRosterSubscriptionBoth, "both");
_LIT8(KRosterSubscriptionFrom, "from");
_LIT8(KRosterSubscriptionTo, "to");
_LIT8(KRosterSubscriptionRemove, "remove");

_LIT8(KMessageTypeChat, "chat");
_LIT8(KMessageTypeGroupchat, "groupchat");

_LIT8(KChatstateActive, "active");
_LIT8(KChatstateComposing, "composing");
_LIT8(KChatstatePaused, "paused");
_LIT8(KChatstateInactive, "inactive");
_LIT8(KChatstateGone, "gone");

_LIT8(KAffiliationNone, "none");
_LIT8(KAffiliationOutcast, "outcast");
_LIT8(KAffiliationMember, "member");
_LIT8(KAffiliationAdmin, "admin");
_LIT8(KAffiliationOwner, "owner");

_LIT8(KRoleNone, "none");
_LIT8(KRoleVisitor, "visitor");
_LIT8(KRoleParticipant, "participant");
_LIT8(KRoleModerator, "moderator");

_LIT8(KIqTypeSet, "set");
_LIT8(KIqTypeGet, "get");

_LIT8(KPlaceAdd, "add");
_LIT8(KPlaceEdit, "edit");
_LIT8(KPlaceCreate, "create");
_LIT8(KPlaceSearch, "search");
_LIT8(KPlaceRemove, "remove");
_LIT8(KPlaceDelete, "delete");
_LIT8(KPlaceCurrent, "current");

#endif
