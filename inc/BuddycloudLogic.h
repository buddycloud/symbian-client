/*
============================================================================
 Name        : 	BuddycloudLogic.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Access for Buddycloud Client UI to Buddycloud low level
 				operations
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef BUDDYCLOUDLOGIC_H_
#define BUDDYCLOUDLOGIC_H_

#include <e32base.h>
#include <cntdb.h>
#include <cntdbobs.h>
#include <flogger.h>
#include "Buddycloud.hrh"
#include "AvatarRepository.h"
#include "BuddycloudContactStreamer.h"
#include "BuddycloudContactSearcher.h"
#include "BuddycloudFollowing.h"
#include "BuddycloudList.h"
#include "BuddycloudPlaces.h"
#include "BuddycloudNearby.h"
#include "DiscussionManager.h"
#include "LocationEngine.h"
#include "NotificationEngine.h"
#include "Timer.h"
#include "TelephonyEngine.h"
#include "XmppEngine.h"
#include "XmppInterfaces.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TDescSettingItems {
	ESettingItemFullName, ESettingItemEmailAddress, ESettingItemUsername, ESettingItemPassword, 
	ESettingItemPrivateMessageToneFile, ESettingItemChannelPostToneFile, ESettingItemDirectReplyToneFile,
	ESettingItemTwitterUsername, ESettingItemTwitterPassword
};

enum TIntSettingItems {
	ESettingItemNotifyChannelsFollowing, ESettingItemNotifyChannelsModerating, 
	ESettingItemPrivateMessageTone, ESettingItemChannelPostTone, ESettingItemDirectReplyTone,
	ESettingItemServerId, ESettingItemLanguage
};

enum TBoolSettingItems {
	ESettingItemCellOn, ESettingItemWifiOn, ESettingItemBtOn, ESettingItemGpsOn, 
	ESettingItemCellAvailable, ESettingItemWifiAvailable, ESettingItemBtAvailable, ESettingItemGpsAvailable, 
	ESettingItemNewInstall, ESettingItemNotifyReplyTo, ESettingItemAutoStart, ESettingItemShowName, 
	ESettingItemShowContacts, ESettingItemContactsLoaded, ESettingItemAccessPoint, ESettingItemMessageBlocking
};

enum TBuddycloudLogicState {
	ELogicShutdown, ELogicOffline, ELogicConnecting, ELogicOnline
};

enum TBuddycloudLocationResource {
	EResourceCell, EResourceWlan, EResourceBt, EResourceGps
};

enum TBuddycloudLogicNotificationType {
	ENotificationLogicEngineStarted, ENotificationLogicEngineDestroyed, 
	ENotificationFollowingItemsUpdated, ENotificationFollowingItemDeleted, 
	ENotificationPlaceItemsUpdated, ENotificationLocationUpdated, 
	ENotificationMessageNotifiedEvent, ENotificationMessageSilentEvent, 
	ENotificationEditPlaceRequested, ENotificationEditPlaceCompleted, 
	ENotificationActivityChanged, ENotificationConnectivityChanged, 
	ENotificationTelephonyChanged, ENotificationAuthenticationFailed,
	ENotificationEditChannelRequested, ENotificationFollowingChannelEvent
};

enum TBuddycloudLogicTimeoutState {
	ETimeoutNone, ETimeoutStartConnection, ETimeoutConnected, ETimeoutSaveSettings, ETimeoutSavePlaces
};

enum TBuddycloudContactNameOrder {
	ENameOrderFirstLast, ENameOrderLastFirst
};

enum TBuddycloudLocationRequestType {
	ELocationRequestNone, ELocationRequestSubscribed, ELocationRequestNearby, ELocationRequestSearch
};

enum TBuddycloudToneType {
	EToneDefault, EToneNone, EToneUserDefined
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MBuddycloudLogicNotificationObserver {
	public:
		virtual void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound) = 0;
};

class MBuddycloudLogicStatusObserver {
	public:
		virtual void JumpToItem(TInt aItemId) = 0;
};

class MBuddycloudLogicOwnerObserver {
	public:
		virtual void StateChanged(TBuddycloudLogicState aState) = 0;
		virtual void LanguageChanged(TInt aLanguage) = 0;
		virtual void ShutdownComplete() = 0;
};

class MBuddycloudLogicDebugObserver {
	public:
		virtual void StanzaSent(const TDesC8& aMessage) = 0;
		virtual void StanzaReceived(const TDesC8& aMessage) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudLogic
--
----------------------------------------------------------------------------
*/

class CBuddycloudLogic : public CBase, MContactDbObserver, MLocationEngineNotification, MTimeInterface,
									MXmppEngineObserver, MXmppRosterObserver, MXmppStanzaObserver,
									MTimeoutNotification, MAvatarRepositoryObserver,
									MDiscussionReadObserver, MTelephonyEngineNotification,
									MBuddycloudContactStreamerObserver, MBuddycloudContactSearcherObserver {

	public:
		CBuddycloudLogic(MBuddycloudLogicOwnerObserver* aObserver);
		static CBuddycloudLogic* NewL(MBuddycloudLogicOwnerObserver* aObserver);
		~CBuddycloudLogic();

	public:
		void Startup();
		void PrepareShutdown();

	private:
		void ConstructL();

	public: // XMPP connection
		void ConnectL();
		void Disconnect();
		
		TBuddycloudLogicState GetState();
		
		MXmppWriteInterface* GetXmppInterface();
		
		void GetConnectionStatistics(TInt& aDataSent, TInt& aDataReceived);

	private:
		void ConnectToXmppServerL();
		
		void SendPresenceL();
		void SendPresenceSubscriptionL(const TDesC8& aTo, const TDesC8& aType, const TDesC8& aOptionalData = KNullDesC8);
		
		TInt GetNewIdStamp();
		
	public: // Settings
		void ResetStoredDataL();
		void ResetConnectionSettings();
		
		void SetLocationResource(TBuddycloudLocationResource aResource, TBool aEnable);
		TBool GetLocationResourceDataAvailable(TBuddycloudLocationResource aResource);
		
		void ValidateUsername();
		
		void LanguageSettingChanged();
		void NotificationSettingChanged(TIntSettingItems aItem);
		
		TDes& GetDescSetting(TDescSettingItems aItem);
		TInt& GetIntSetting(TIntSettingItems aItem);
		TBool& GetBoolSetting(TBoolSettingItems aItem);
		
	public: 
		TDesC& GetActivityStatus();
		void SetActivityStatus(const TDesC& aText);
	
	private: 
		void SetActivityStatus(TInt aResource);

	private:
		TInt DisplaySingleLinePopupMenuL(RPointerArray<HBufC>& aMenuItems);
		TInt DisplayDoubleLinePopupMenuL(RPointerArray<HBufC>& aMenuItems);
		
		void SendSmsOrEmailL(TDesC& aAddress, TDesC& aSubject, TDesC& aBody);
		
	public: // Friends
		void SendInviteL(TInt aFollowerId);
		void SendPlaceL(TInt aFollowerId);
		
	public: // Follow & pubsub collect
		void FollowContactL(const TDesC& aContact);
		
		void RecollectFollowerDetailsL(TInt aFollowerId);

	private: // Pubsub
		void SetLastNodeIdReceived(const TDesC8& aNodeItemId);
		
		void CollectPubsubSubscriptionsL();
		void ProcessPubsubSubscriptionsL(const TDesC8& aStanza);
		
		void CollectUsersPubsubNodeSubscribersL();
		void ProcessUsersPubsubNodeSubscribersL(const TDesC8& aStanza);
		
		void CollectLastPubsubNodeItemsL(const TDesC& aNode, const TDesC8& aLastIdReceived);		
		void CollectUserPubsubNodeL(const TDesC& aJid, const TDesC& aNodeLeaf);
		
		void CollectChannelMetadataL(const TDesC& aNode);
		void ProcessChannelMetadataL(const TDesC8& aStanza);
		
	public: // Pubsub
		void SetPubsubNodeAffiliationL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubAffiliation aAffiliation);
		void SetPubsubNodeSubscriptionL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubSubscription aSubscription);
    	
		void RetractPubsubNodeItemL(const TDesC& aNode, const TDesC8& aNodeItemId);
		
    private: // Pubsub handling
		void HandlePubsubEventL(const TDesC8& aStanza, TBool aNewEvent);
	
	public: // Channels
		void FollowChannelL(const TDesC& aNode);
		void UnfollowChannelL(TInt aItemId);
		
		void PublishChannelMetadataL(TInt aItemId);
		
		TInt CreateChannelL(CFollowingChannelItem* aChannelItem);

	public: // User Status & Place
		void SetMoodL(TDesC& aMood);

		void SetCurrentPlaceL();
		void SetCurrentPlaceL(TInt aPlaceId);
		void SetNextPlaceL(TDesC& aPlace, TInt aPlaceId = KErrNotFound);

	private:
		TInt GetBubbleToPosition(CFollowingItem* aBubblingItem);

	public: // From MDiscussionReadObserver
		void DiscussionRead(TDesC& aDiscussionId, TInt aItemId);

	public: // Contact Services
		void PairItemIdsL(TInt aPairItemId1, TInt aPairItemId2);
		void UnpairItemsL(TInt aItem);

		void LoadAddressBookContacts();
		void RemoveAddressBookContacts();
		
		CContactItem* GetContactDetailsLC(TInt aContactId);
		void CollectContactDetailsL(TInt aFollowerId);
		
		void CallL(TInt aItemId);
		CTelephonyEngine* GetCall();

		TDesC& GetContactFilter();
		void SetContactFilterL(const TDesC& aFilter);

	private:
		void LoadAddressBookContactsL();

		void FilterContactsL();
		
	private: // MBuddycloudContactStreamerObserver
		void HandleStreamingContactL(TInt aContactId, CContactItem* aContact);
		void FinishedStreamingCycle();
		
	private: // MBuddycloudContactSearcherObserver
		void FinishedContactSearch();

	private: // MContactDbObserver
		void HandleDatabaseEventL(TContactDbObserverEvent aEvent);

	private: // Reading/Writing state to file
		void LoadSettingsAndItems();
		void LoadSettingsAndItemsL();
		void SaveSettingsAndItemsL();

		void AddChannelItemL(TPtrC aId, TPtrC aName, TPtrC aTopic, TInt aRank = 0);

		void LoadPlaceItems();
		void LoadPlaceItemsL();
		void SavePlaceItemsL();

		void BackupOldLog();
		void WriteToLog(const TDesC8& aText);

	private: // Time Stamp
		TTime TimeStamp();

	public: // Observers
		void AddNotificationObserver(MBuddycloudLogicNotificationObserver* aNotificationObserver);
		void RemoveNotificationObserver(MBuddycloudLogicNotificationObserver* aNotificationObserver);
		void NotifyNotificationObservers(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

		void AddStatusObserver(MBuddycloudLogicStatusObserver* aStatusObserver);
		void RemoveStatusObserver();

	public: // Friends
		CBuddycloudListStore* GetFollowingStore();
		
		CFollowingRosterItem* GetOwnItem();
		void UnfollowItemL(TInt aItemId);
		
	public: // Notices
		void RespondToNoticeL(TInt aItemId, TNoticeResponse aResponse);
		
	public: // Media posting
		void MediaPostRequestL(TInt aItemId);

	public: // Messaging
		CDiscussion* GetDiscussion(const TDesC& aId);

		TInt IsSubscribedTo(const TDesC& aId, TInt aItemOptions = EItemAll);

		void PostMessageL(TInt aItemId, TDesC& aId, TDesC& aContent, const TDesC8& aReferenceId = KNullDesC8);		
		
	public: // Communities
		void SendCommunityCredentials(TCommunityItems aCommunity);

	public: // Avatar
		CAvatarRepository* GetImageRepository();

	public: // Places
		TInt GetMyPlaceId();
		TLocationMotionState GetMyMotionState();
		TInt GetMyPatternQuality();
		
	public:
		CBuddycloudPlaceStore* GetPlaceStore();

	public:
		void HandlePlaceQueryResultL(const TDesC8& aStanza, const TDesC8& aId);
		
	public: 
		void SendEditedPlaceDetailsL(TBool aSearchOnly);
		void GetPlaceDetailsL(TInt aPlaceId);
		void EditMyPlacesL(TInt aPlaceId, TBool aAddPlace);
	
	private:	
		void CollectMyPlacesL();
		void ProcessMyPlacesL(CBuddycloudPlaceStore* aPlaceStore);
		void ProcessPlaceSearchL(CBuddycloudPlaceStore* aPlaceStore);
		void SendPlaceQueryL(TInt aPlaceId, const TDesC8& aAction, TBool aAcknowledge = false);

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	public: // From MAvatarRepositoryObserver
		void AvatarLoaded();

	public: // MTelephonyEngineNotification
		void TelephonyStateChanged(TTelephonyEngineState aState);
	   	void TelephonyDebug(const TDesC8& aMessage);

	public: // From CLocationEngine
    	void HandleLocationServerResult(TLocationMotionState aMotionState, TInt aPatternQuality, TInt aPlaceId);
 		void LocationShutdownComplete();
	
	public: // From MTimeInterface
    	TTime GetTime();

    public: // From MXmppEngineNotification
		void XmppStanzaRead(const TDesC8& aMessage);
		void XmppStanzaWritten(const TDesC8& aMessage);
		void XmppStateChanged(TXmppEngineState aState);
		void XmppUnhandledStanza(const TDesC8& aStanza);
		void XmppError(TXmppEngineError aError);
		void XmppShutdownComplete();

		void XmppDebug(const TDesC8& aMessage);

    public: // From MXmppRosterObserver
    	void RosterItemsL(const TDesC8& aItems, TBool aPush = false);
    	void RosterOwnJidL(TDesC& aJid);
	
    public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);
 
    private: // Presence handling
    	void HandleIncomingPresenceL(TDesC& aFrom, const TDesC8& aStanza);
       	void ProcessPresenceSubscriptionL(TDesC& aJid, const TDesC8& aData);
   
    private: // Message handling
		void HandleIncomingMessageL(TDesC& aFrom, const TDesC8& aData);

    protected:
    	MBuddycloudLogicOwnerObserver* iOwnerObserver;
		MBuddycloudLogicStatusObserver* iStatusObserver;
		RPointerArray<MBuddycloudLogicNotificationObserver> iNotificationObservers;
		
		CNotificationEngine* iNotificationEngine;

		CCustomTimer* iTimer;
		TBuddycloudLogicTimeoutState iTimerState;

		CFollowingRosterItem* iOwnItem;

		// Contact Database
		CContactDatabase* iContactDatabase;
		CContactChangeNotifier* iContactDbNotifier;
		TBuddycloudContactNameOrder iContactNameOrder;

		CBuddycloudContactStreamer* iContactStreamer;
		CBuddycloudContactSearcher* iContactSearcher;
		TBool iContactsLoaded;

		// Connection State
		TBuddycloudLogicState iState;
		TBool iConnectionCold;
		
		HBufC* iServerActivityText;
        HBufC* iBroadLocationText;        
        HBufC8* iLastNodeIdReceived;

		// Time Stamps
		TTimeIntervalSeconds iServerOffset;
		
		TInt iIdStamp;
		
		TBool iOffsetReceived;
		TBool iRosterSynchronized;
		TBool iPubsubSubscribedTo;
		TBool iMyChannelMembersRequested;

		CXmppEngine* iXmppEngine;
        TBool iXmppEngineReady;

        // Setting Items
        TInt iSettingConnectionMode;
        TInt iSettingConnectionModeId;
        TBool iSettingNewInstall;
        
        // Positioning settings
        TBool iSettingCellOn;
        TBool iSettingWlanOn;
        TBool iSettingBtOn;
        TBool iSettingGpsOn;
        
        // Notifications settings
        TInt iSettingNotifyChannelsFollowing;
        TInt iSettingNotifyChannelsModerating;
        TInt iSettingPrivateMessageTone;
        TFileName iSettingPrivateMessageToneFile;
        TInt iSettingChannelPostTone;
        TFileName iSettingChannelPostToneFile;
        TInt iSettingDirectReplyTone;
        TFileName iSettingDirectReplyToneFile;
        TBool iSettingNotifyReplyTo;
        
        // Preferences settings
        TInt iSettingPreferredLanguage;
        TBool iSettingMessageBlocking;
        TBool iSettingAccessPoint;
        TBool iSettingShowName;
        TBool iSettingAutoStart;
        TBool iSettingShowContacts;
       
        // Account settings
        TBuf<32> iSettingFullName;
        TBuf<32> iSettingEmailAddress;
        TBuf<32> iSettingUsername;
        TBuf<32> iSettingPassword;
        TInt iSettingServerId;
        
        // Community settings
        TBuf<32> iSettingTwitterUsername;
        TBuf<32> iSettingTwitterPassword;

        // Location services
        CLocationEngine* iLocationEngine;
        TBool iLocationEngineReady;
        TLocationMotionState iLastMotionState;
        TInt iStablePlaceId;
        
        // Nearby places
        RPointerArray<CBuddycloudNearbyPlace> iNearbyPlaces;
 
		// Stores
        CBuddycloudListStore* iFollowingList;
        CBuddycloudPlaceStore* iPlaceList;
        CDiscussionManager* iDiscussionManager;
       
        TInt iNextItemId;

        HBufC* iContactFilter;
        TBool iFilteringContacts;

        // Debug
        RFileLogger iLog;

        // Avatars
        CAvatarRepository* iAvatarRepository;
        
        // Telephony
        CTelephonyEngine* iTelephonyEngine;
        
        // Save required
        TBool iSettingsSaveNeeded;
        TBool iPlacesSaveNeeded;
};

#endif /*BUDDYCLOUDLOGIC_H_*/
