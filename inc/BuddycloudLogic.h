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
#include <e32std.h>
#include <cntdb.h>
#include <cntdbobs.h>
#include <flogger.h>
#include "Buddycloud.hrh"
#include "AvatarRepository.h"
#include "BuddycloudContactStreamer.h"
#include "BuddycloudContactSearcher.h"
#include "MessagingParticipants.h"
#include "BuddycloudFollowing.h"
#include "BuddycloudPlaces.h"
#include "BuddycloudNearby.h"
#include "LocationEngine.h"
#include "MessagingManager.h"
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
	ESettingItemFullName, ESettingItemPhoneNumber, ESettingItemEmailAddress, ESettingItemUsername, 
	ESettingItemPassword, ESettingItemTwitterUsername, ESettingItemTwitterPassword,
	ESettingItemPrivateMessageToneFile, ESettingItemChannelPostToneFile, ESettingItemDirectReplyToneFile
};

enum TIntSettingItems {
	ESettingItemGroupNotification, ESettingItemServerId, ESettingItemLanguage, 
	ESettingItemPrivateMessageTone, ESettingItemChannelPostTone, ESettingItemDirectReplyTone
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
	ENotificationTelephonyChanged, ENotificationAuthenticationFailed
};

enum TBuddycloudLogicTimeoutState {
	ETimeoutNone            = 0, 
	ETimeoutStartConnection = 2, 
	ETimeoutConnected       = 4, 
	ETimeoutSendPresence    = 8, 
	ETimeoutSaveSettings    = 16, 
	ETimeoutSavePlaces      = 32
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

	public: // Settings
		void MasterResetL();
		void ResetConnectionSettings();
		
		void SetLocationResource(TBuddycloudLocationResource aResource, TBool aEnable);
		TBool GetLocationResourceDataAvailable(TBuddycloudLocationResource aResource);
		
		void ValidateUsername();
		void LookupUserByEmailL(TBool aForcePublish = false);
		void LookupUserByNumberL(TBool aForcePublish = false);
		void PublishUserDataL(CContactIdArray* aContactIdArray, TBool aForcePublish = false);
		
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
		void FollowContactL(const TDesC& aContact);
		
		void GetFriendDetailsL(TInt aFollowerId);

	public: // Channels
		TInt FollowTopicChannelL(TDesC& aChannelJid, TDesC& aTitle, const TDesC& aDescription = KNullDesC);
		void FollowPersonalChannelL(TInt aFollowerId);
		void UnfollowChannelL(TInt aFollowerId);
		
		TInt CreateChannelL(const TDesC& aChannelTitleOrJid);
		void InviteToChannelL(TInt aChannelId);
		void InviteToChannelL(TDesC& aToJid, TInt aChannelId = 0);
		void DeclineChannelInviteL(TDesC& aJid);
	
	private:
		void CollectMyChannelsL();
		void ProcessMyChannelsL(const TDesC8& aStanza);
		
		void SynchronizePersonalChannelMembersL(const TDesC8& aStanza);
		
		void SendChannelPresenceL(TDesC& aJid, const TDesC8& aHistory = KNullDesC8);
		void SendChannelUnavailableL(TDesC& aChannelJid);
		void SendChannelConfigurationL(TInt aFollowerId);
		
		void GetChannelInfoL(TDesC& aChannelJid);
		void SendChannelInviteL(TDesC& aJid, TInt aChannelId);
		
	public:
		void ChangeUsersChannelAffiliationL(const TDesC& aJid, TDesC& aChannelJid, TMucAffiliation aAffiliation);
		void ChangeUsersChannelRoleL(const TDesC& aJidOrNick, TDesC& aChannelJid, TMucRole aRole);

	public: // User Status & Place
		void SetMoodL(TDesC& aMood);

		void SetCurrentPlaceL();
		void SetCurrentPlaceL(TInt aPlaceId);
		void SetNextPlaceL(TDesC& aPlace, TInt aPlaceId = KErrNotFound);

	private:
		void CollectPubsubNodesL(TDesC& aJid);
		TInt GetBubbleToPosition(CFollowingItem* aBubblingItem);

	public: // From MDiscussionReadObserver
		void AllMessagesRead(CFollowingItem* aFollowingItem);

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

		void AddChannelItemL(TPtrC aJid, TPtrC aName, TPtrC aTopic, TInt aRank = 0);

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
		CBuddycloudFollowingStore* GetFollowingStore();
		
		CFollowingRosterItem* GetOwnItem();
		void DeleteItemL(TInt aItemId);
		
	public: // Notices
		void RespondToNoticeL(TInt aItemId, TNoticeResponse aResponse);
		
	public: // Media posting
		void MediaPostRequestL(const TDesC& aJid);

	public: // Messaging
		CDiscussion* GetDiscussion(const TDesC& aJid);

		TPtrC GetRawJid(TDesC& aJid);
		TInt IsSubscribedTo(const TDesC& aJid, TInt aItemOptions = EItemAll);

		void SetTopicL(TDesC& aTopic, TDesC& aToJid, TBool aIsChannel);
		void BuildNewMessageL(TDesC& aToJid, TDesC& aMessageText, TBool aIsChannel, CMessage* aReferenceMessage = NULL);
		void SetChatStateL(TMessageChatState aChatState, TDesC& aToJid);
		
	private:
		TDesC8& GenerateNewThreadLC();
		
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
    	void RosterItemsL(const TDesC8& aItems, TBool aUpdate = false);
    	void RosterOwnJidL(TDesC& aJid);
		void RosterPubsubEventL(TDesC& aFrom, const TDesC8& aData, TBool aNew);
		void RosterPubsubDeleteL(TDesC& aFrom, const TDesC8& aNode);
    	void RosterSubscriptionL(TDesC& aJid, const TDesC8& aData);
	
    public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);

    private: // Presence handling
    	void HandleIncomingPresenceL(TDesC& aFrom, const TDesC8& aData);
    
    private: // Message handling
		void HandleIncomingMessageL(TDesC& aFrom, const TDesC8& aData);
		void ProcessNewMessageL(TDesC& aFrom, const TDesC8& aData);

    protected:
    	MBuddycloudLogicOwnerObserver* iOwnerObserver;
		MBuddycloudLogicStatusObserver* iStatusObserver;
		RPointerArray<MBuddycloudLogicNotificationObserver> iNotificationObservers;
		
		CNotificationEngine* iNotificationEngine;

		CCustomTimer* iTimer;
		TInt iTimerState;

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

		// Time Stamps
		TTime iLastReceivedAt;
		TTime iHistoryFrom;
		TTimeIntervalSeconds iServerOffset;
		
		TBool iOffsetReceived;
		TBool iRosterSynchronized;
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
        TInt iSettingGroupNotification;
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
        TBuf<32> iSettingPhoneNumber;
        TBuf<32> iSettingEmailAddress;
        TBuf<32> iSettingUsername;
        TBuf<32> iSettingPassword;
        TInt iSettingServerId;
        
        TPtrC iSettingUsernameOnly;
        
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
        CBuddycloudFollowingStore* iMasterFollowingList;
        CBuddycloudPlaceStore* iMasterPlaceList;
        CMessagingManager* iMessagingManager;
       
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
