/*
============================================================================
 Name        : 	BuddycloudLogic.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Access for Buddycloud Client UI to Buddycloud low level
 				operations
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <akniconarray.h>
#include <aknlists.h>
#include <aknnotewrappers.h> 
#include <bacline.h>
#include <badesca.h>
#include <bautils.h>
#include <cntdef.h>
#include <cntfield.h>
#include <cntfldst.h>
#include <cntitem.h>
#include <cmessagedata.h>
#include <cpbkcontactengine.h>
#include <eikclb.h>
#include <eikclbd.h>
#include <f32file.h>
#include <s32file.h>
#include <sendui.h>
#include <senduiconsts.h>
#include <txtfmlyr.h>
#include <txtrich.h>
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"
#include "FileUtilities.h"
#include "PhoneUtilities.h"
#include "SearchResultMessageQueryDialog.h"
#include "TimeUtilities.h"
#include "XmlParser.h"
#include "XmppUtilities.h"

/*
----------------------------------------------------------------------------
--
-- Initialization
--
----------------------------------------------------------------------------
*/

CBuddycloudLogic::CBuddycloudLogic(MBuddycloudLogicOwnerObserver* aObserver) {
	iOwnerObserver = aObserver;
}

CBuddycloudLogic* CBuddycloudLogic::NewL(MBuddycloudLogicOwnerObserver* aObserver) {
	CBuddycloudLogic* self = new (ELeave) CBuddycloudLogic(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CBuddycloudLogic::~CBuddycloudLogic() {
	if(iTimer)
		delete iTimer;
	
	if(iTextUtilities)
		delete iTextUtilities;

	// Avatars & text
	if(iAvatarRepository)
		delete iAvatarRepository;

	if(iServerActivityText)
		delete iServerActivityText;
	
	if(iBroadLocationText)
		delete iBroadLocationText;
	
	if(iLastNodeIdReceived) 
		delete iLastNodeIdReceived;

	// Engines
	if(iLocationEngine)
		delete iLocationEngine;

	if(iXmppEngine) {
		delete iXmppEngine;
		iXmppEngine = NULL;
	}
	
	if(iNotificationEngine) {
		delete iNotificationEngine;
	}

	// Stores
	if(iFollowingList)
		delete iFollowingList;
	
	if(iPlaceList)
		delete iPlaceList;
	
	if(iDiscussionManager)
		delete iDiscussionManager;
	
	// Generic list
	if(iGenericList)
		delete iGenericList;
	
	// Nearby places
	for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
		delete iNearbyPlaces[i];
	}
	
	iNearbyPlaces.Close();

#ifdef _DEBUG
	// Log
	if(iLog.Handle()) {
		iLog.CloseLog();
	}

	iLog.Close();
#endif
	
	NotifyNotificationObservers(ENotificationLogicEngineDestroyed);
	iNotificationObservers.Close();
}

void CBuddycloudLogic::ConstructL() {
#ifdef _DEBUG
	// Backup log
	BackupOldLog();

	// Start logging
	if(iLog.Connect() == KErrNone) {		
		iLog.CreateLog(_L("Buddycloud"), _L("LogicLog.txt"), EFileLoggingModeOverwrite);
		WriteToLog(_L8("========== Buddycloud LogicLog Started =========="));
	}
#endif

	iState = ELogicOffline;
	iServerActivityText = HBufC::NewL(0);
	iBroadLocationText = HBufC::NewL(0);
	iLastNodeIdReceived = _L8("1").AllocL();

	iTimer = CCustomTimer::NewL(this);
	
	iTextUtilities = CTextUtilities::NewL();
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CAvatarRepository"));
#endif

	iAvatarRepository = CAvatarRepository::NewL(this);

	iNextItemId = 1;

	// Preferences defaults
	iSettingPreferredLanguage = -1;
	iSettingNotifyChannelsModerating = 1;
	iSettingMessageBlocking = true;
	iSettingNotifyReplyTo = true;
	
	// Positioning defaults
	iSettingCellOn = true;
	iSettingWlanOn = true;
	iSettingGpsOn = false;
	iSettingBtOn = false;
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CBuddycloudFollowingStore"));
#endif

	iFollowingList = CBuddycloudFollowingStore::NewL();
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CXmppEngine"));
#endif

	iXmppEngineReady = false;
	iXmppEngine = CXmppEngine::NewL(this);
	iXmppEngine->AddRosterObserver(this);
 	iXmppEngine->SetResourceL(_L8("s60/bcloud"));
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CBuddycloudPlaceStore/CBuddycloudNearbyStore"));
#endif

 	iPlaceList = CBuddycloudPlaceStore::NewL();
	
	// Messaging manager
	iDiscussionManager = CDiscussionManager::NewL();
	
	// Notification engine
	iNotificationEngine = CNotificationEngine::NewL();

 	LoadSettingsAndItems();
 	
 	if(iOwnerObserver) {
 		iOwnerObserver->LanguageChanged(iSettingPreferredLanguage);
 	}
 	
 	SettingsItemChanged(ESettingItemPrivateMessageTone);
 	SettingsItemChanged(ESettingItemChannelPostTone);
 	SettingsItemChanged(ESettingItemDirectReplyTone);
}

void CBuddycloudLogic::Startup() {	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CLocationEngine"));
#endif
	
	iGenericList = CBuddycloudListStore::NewL();

	// Initialize Location Engine
	iLocationEngineReady = false;
	iLocationEngine = CLocationEngine::NewL(this);
	iLocationEngine->SetXmppWriteInterface((MXmppWriteInterface*)iXmppEngine);
	iLocationEngine->SetTimeInterface(this);
	
	HBufC* aLangCode = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE);
	iLocationEngine->SetLanguageCodeL(iTextUtilities->UnicodeToUtf8L(*aLangCode));
	CleanupStack::PopAndDestroy(); // aLangCode
	
	iLocationEngine->SetCellActive(iSettingCellOn);
	iLocationEngine->SetWlanActive(iSettingWlanOn);
	iLocationEngine->SetBtActive(iSettingBtOn);
	iLocationEngine->SetGpsActive(iSettingGpsOn);
	
	// Load places
	LoadPlaceItems();
	
	// Auto Connect
	if(iSettingUsername.Length() > 0 && iSettingPassword.Length() > 0) {
		iTimer->After(1000000);
		iTimerState = ETimeoutStartConnection;
	}
	else {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_OFFLINE);
	}
	
	NotifyNotificationObservers(ENotificationLogicEngineStarted);
}

void CBuddycloudLogic::PrepareShutdown() {	
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::PrepareShutdown"));
#endif
	
	if(iState != ELogicShutdown) {
		iState = ELogicShutdown;
		iTimer->Stop();
		
		// Save settings & place items
		SaveSettingsAndItemsL();
		SavePlaceItemsL();
		
		// Close messaging manager
		if(iDiscussionManager) {
			delete iDiscussionManager;
			iDiscussionManager = NULL;
		}
	
		// Shutdown xmpp & location engines
		iXmppEngineReady = false;
		iLocationEngineReady = false;
	
		iXmppEngine->PrepareShutdown();
		iLocationEngine->PrepareShutdown();
	}
}

/*
----------------------------------------------------------------------------
--
-- Connection
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::ConnectL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ConnectL"));
#endif

	if(iSettingUsername.Length() == 0 || iSettingPassword.Length() == 0) {
		// No account details
		CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
		dlg->ExecuteLD(R_AUTHERROR_DIALOG);
		
		NotifyNotificationObservers(ENotificationAuthenticationFailed);
	}
	else {
		ConnectToXmppServerL();
	}
}

void CBuddycloudLogic::Disconnect() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::Disconnect"));
#endif

	iXmppEngine->Disconnect();
}

TBuddycloudLogicState CBuddycloudLogic::GetState() {
	return iState;
}

MXmppWriteInterface* CBuddycloudLogic::GetXmppInterface() {
	return (MXmppWriteInterface*)iXmppEngine;
}

TInt CBuddycloudLogic::GetNewIdStamp() {
	return (++iIdStamp) % 100;
}

void CBuddycloudLogic::GetConnectionStatistics(TInt& aDataSent, TInt& aDataReceived) {
	iXmppEngine->GetConnectionStatistics(aDataSent, aDataReceived);
}

void CBuddycloudLogic::ConnectToXmppServerL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ConnectToXmppServerL"));
#endif

	iConnectionCold = true;

	SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
	
	iXmppEngine->SetAccountDetailsL(iSettingUsername, iSettingPassword);
	iXmppEngine->SetServerDetailsL(iSettingXmppServer);
	iXmppEngine->ConnectL();
}

void CBuddycloudLogic::SendPresenceL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceL"));
#endif
	
	// Nickname
	HBufC8* aEncNick = iTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
	
	// Broad location
	TPtrC8 aEncLocation(iTextUtilities->UnicodeToUtf8L(*iBroadLocationText));
	
	_LIT8(KPresenceStanza, "<presence><priority>10</priority><status></status><c xmlns='http://jabber.org/protocol/caps' node='http://buddycloud.com/caps' ver='s60-1.0.00'/><nick xmlns='http://jabber.org/protocol/nick'></nick></presence>\r\n");
	HBufC8* aPresenceStanza = HBufC8::NewLC(KPresenceStanza().Length() + aEncLocation.Length() + aEncNick->Des().Length());
	TPtr8 pPresenceStanza(aPresenceStanza->Des());
	pPresenceStanza.Copy(KPresenceStanza);
	pPresenceStanza.Insert(191, *aEncNick);
	pPresenceStanza.Insert(41, aEncLocation);
	
	iXmppEngine->SendAndForgetXmppStanza(pPresenceStanza, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(2); // aPresenceStanza, aEncNick
}

void CBuddycloudLogic::AddRosterManagementItemL(const TDesC8& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::AddRosterManagementItemL"));
#endif
	
	// Add roster item
	_LIT8(KRosterItemStanza, "<iq type='set' id='additem1'><query xmlns='jabber:iq:roster'><item jid=''/></query></iq>\r\n");
	HBufC8* aRosterItemStanza = HBufC8::NewLC(KRosterItemStanza().Length() + aJid.Length());
	TPtr8 pRosterItemStanza(aRosterItemStanza->Des());
	pRosterItemStanza.Append(KRosterItemStanza);
	pRosterItemStanza.Insert(72, aJid);
	
	iXmppEngine->SendAndForgetXmppStanza(pRosterItemStanza, true, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aRosterItemStanza
}

void CBuddycloudLogic::SendPresenceSubscriptionL(const TDesC8& aJid, const TDesC8& aType, const TDesC8& aOptionalData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceSubscriptionL"));
#endif
	
	// Send presence subscription
	_LIT8(KSubscriptionStanza, "<presence to='' type=''/>\r\n");
	_LIT8(KExtendedSubscriptionStanza, "<presence to='' type=''></presence>\r\n");
	HBufC8* aSubscribeStanza = HBufC8::NewLC(KExtendedSubscriptionStanza().Length() + aJid.Length() + aType.Length() + aOptionalData.Length());
	TPtr8 pSubscribeStanza(aSubscribeStanza->Des());
	
	if(aOptionalData.Length() > 0) {
		pSubscribeStanza.Copy(KExtendedSubscriptionStanza);
		pSubscribeStanza.Insert(24, aOptionalData);
	}
	else {
		pSubscribeStanza.Copy(KSubscriptionStanza);
	}

	pSubscribeStanza.Insert(22, aType);
	pSubscribeStanza.Insert(14, aJid);
	
	iXmppEngine->SendAndForgetXmppStanza(pSubscribeStanza, true, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aSubscribeStanza
}

/*
----------------------------------------------------------------------------
--
-- Settings
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::ResetConnectionSettings() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ResetConnectionSettings"));
#endif

	// Perform Reset/Reconnect
	iSettingConnectionMode = 0;
	iSettingConnectionModeId = 0;
	iXmppEngine->SetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId, KNullDesC);

	if(iState > ELogicOffline) {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
	}

	// If offline start new connection
	if(iState == ELogicOffline) {
		ConnectL();
	}
}

void CBuddycloudLogic::ValidateUsername(TBool aCheckServer) {
	iSettingUsername.LowerCase();
	
	for(TInt i = iSettingUsername.Length() - 1; i >= 0; i--) {
		if(iSettingUsername[i] <= 45 || iSettingUsername[i] == 47 || (iSettingUsername[i] >= 58 && iSettingUsername[i] <= 63) ||
				(iSettingUsername[i] >= 91 && iSettingUsername[i] <= 96) || iSettingUsername[i] >= 123) {
			
			iSettingUsername.Delete(i, 1);
		}
	}
	
	if(aCheckServer && iSettingUsername.Length() > 0 && iSettingUsername.Locate('@') == KErrNotFound) {
		iSettingUsername.Append(_L("@buddycloud.com"));
		iSettingXmppServer.Copy(_L("cirrus.buddycloud.com:443"));
	}
}

TDes& CBuddycloudLogic::GetDescSetting(TDescSettingItems aItem) {
	switch(aItem) {
		case ESettingItemUsername:
			return iSettingUsername;
		case ESettingItemPassword:
			return iSettingPassword;			
		case ESettingItemServer:
			return iSettingXmppServer;			
		case ESettingItemTwitterUsername:
			return iSettingTwitterUsername;
		case ESettingItemTwitterPassword:
			return iSettingTwitterPassword;			
		case ESettingItemPrivateMessageToneFile:
			return iSettingPrivateMessageToneFile;
		case ESettingItemChannelPostToneFile:
			return iSettingChannelPostToneFile;
		case ESettingItemDirectReplyToneFile:
			return iSettingDirectReplyToneFile;
		default:
			return iSettingFullName;
	}
}

TInt& CBuddycloudLogic::GetIntSetting(TIntSettingItems aItem) {
	switch(aItem) {
		case ESettingItemPrivateMessageTone:
			return iSettingPrivateMessageTone;
		case ESettingItemChannelPostTone:
			return iSettingChannelPostTone;
		case ESettingItemDirectReplyTone:
			return iSettingDirectReplyTone;
		case ESettingItemNotifyChannelsFollowing:
			return iSettingNotifyChannelsFollowing;
		case ESettingItemNotifyChannelsModerating:
			return iSettingNotifyChannelsModerating;
		case ESettingItemLanguage:
			return iSettingPreferredLanguage;
		default:
			return iSettingNotifyChannelsFollowing;
	}
}

TBool& CBuddycloudLogic::GetBoolSetting(TBoolSettingItems aItem) {
	switch(aItem) {
		case ESettingItemCellOn:
			return iSettingCellOn;
		case ESettingItemWifiOn:
			return iSettingWlanOn;
		case ESettingItemBtOn:
			return iSettingBtOn;
		case ESettingItemGpsOn:
			return iSettingGpsOn;
		case ESettingItemNotifyReplyTo:
			return iSettingNotifyReplyTo;
		case ESettingItemAutoStart:
			return iSettingAutoStart;
		case ESettingItemAccessPoint:
			return iSettingAccessPoint;
		case ESettingItemMessageBlocking:
			return iSettingMessageBlocking;
		default:
			return iSettingNewInstall;
	}
}

void CBuddycloudLogic::SettingsItemChanged(TInt aSettingsItemId) {
	if(aSettingsItemId == ESettingItemFullName) {
		if(iState == ELogicOnline) {
			// Resend presence with new full name
			SendPresenceL();
		}
	}
	else if(aSettingsItemId == ESettingItemUsername) {
		// Username changed
		ResetStoredDataL();

		ValidateUsername();		
	}
	else if(aSettingsItemId == ESettingItemLanguage) {
		// Language changed
		if(iOwnerObserver) {
			iOwnerObserver->LanguageChanged(iSettingPreferredLanguage);
		}
	}
	else if(aSettingsItemId == ESettingItemPrivateMessageTone) {
		// Private mesage tone
		if(iSettingPrivateMessageTone == EToneNone) {
			iSettingPrivateMessageToneFile.Zero();
		}
		else if(iSettingPrivateMessageTone == EToneDefault) {
			iSettingPrivateMessageToneFile.Copy(_L("pm.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingPrivateMessageToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemPrivateMessageTone, iSettingPrivateMessageToneFile);
	}
	else if(aSettingsItemId == ESettingItemChannelPostTone) {
		// Channel post tone
		if(iSettingChannelPostTone == EToneNone) {
			iSettingChannelPostToneFile.Zero();
		}
		else if(iSettingChannelPostTone == EToneDefault) {
			iSettingChannelPostToneFile.Copy(_L("cp.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingChannelPostToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemChannelPostTone, iSettingChannelPostToneFile);
	}
	else if(aSettingsItemId == ESettingItemDirectReplyTone) {
		// Direct reply tone
		if(iSettingDirectReplyTone == EToneNone) {
			iSettingDirectReplyToneFile.Zero();
		}
		else if(iSettingDirectReplyTone == EToneDefault) {
			iSettingDirectReplyToneFile.Copy(_L("dr.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingDirectReplyToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemDirectReplyTone, iSettingDirectReplyToneFile);
	}
	else if(aSettingsItemId == ESettingItemCellOn) {
		iLocationEngine->SetCellActive(iSettingCellOn);
	}
	else if(aSettingsItemId == ESettingItemWifiOn) {
		iLocationEngine->SetWlanActive(iSettingWlanOn);
	}
	else if(aSettingsItemId == ESettingItemGpsOn) {
		iLocationEngine->SetGpsActive(iSettingGpsOn);
	}
	else if(aSettingsItemId == ESettingItemBtOn) {
		iLocationEngine->SetBtActive(iSettingBtOn);
	}

	iSettingsSaveNeeded = true;
}

void CBuddycloudLogic::ResetStoredDataL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ResetStoredDataL"));
#endif

	iOwnItem = NULL;
	
	iSettingPassword.Zero();
	iSettingXmppServer.Zero();
	
	// Reset broad location & last node id received
	iBroadLocationText->Des().Zero();
	iLastNodeIdReceived->Des().Copy(_L8("1"));
		
	// Remove following items
	for(TInt i = iFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem->GetItemType() >= EItemRoster) {
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				iDiscussionManager->DeleteDiscussionL(aRosterItem->GetId(EIdRoster));				
			}

			iDiscussionManager->DeleteDiscussionL(aItem->GetId());
		}
			
		iFollowingList->DeleteItemByIndex(i);
	}
	
	iSettingsSaveNeeded = true;
	
	// Remove Place items
	while(iPlaceList->Count() > 0) {
		iPlaceList->DeleteItemByIndex(0);
	}
	
	iPlacesSaveNeeded = true;
	
	// Remove Nearby items
	for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
		delete iNearbyPlaces[i];
	}
	
	iNearbyPlaces.Reset();
}

TDesC& CBuddycloudLogic::GetActivityStatus() {
	return *iServerActivityText;
}

void CBuddycloudLogic::SetActivityStatus(const TDesC& aText) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetActivityStatus(const TDesC& aText)"));
#endif

	if(iServerActivityText)
		delete iServerActivityText;

	iServerActivityText = aText.AllocL();
	
	NotifyNotificationObservers(ENotificationActivityChanged);
}

void CBuddycloudLogic::SetActivityStatus(TInt aResourceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetActivityStatus(TInt aResourceId)"));
#endif
	
	if(aResourceId == R_LOCALIZED_STRING_NOTE_CONNECTING) {
		TPtrC pConnectionName = iXmppEngine->GetConnectionName();
		
		if(pConnectionName.Length() > 0) {		
			HBufC* aNoteText = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CONNECTINGWITH);
			TPtr pNoteText(aNoteText->Des());
			
			HBufC* aConnectText = HBufC::NewLC(pNoteText.Length() + pConnectionName.Length());
			TPtr pConnectText(aConnectText->Des());
			pConnectText.Append(pNoteText);
			pConnectText.Replace(pConnectText.Find(_L("$APN")), 4, pConnectionName);
			
			SetActivityStatus(pConnectText);
			CleanupStack::PopAndDestroy(2); // aConnectText, aNoteText
		}
		else {
			HBufC* aNoteText = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CONNECTING);
			SetActivityStatus(*aNoteText);
			CleanupStack::PopAndDestroy(); // aNoteText
		}	
	}
	else {
		HBufC* aNoteText = CCoeEnv::Static()->AllocReadResourceLC(aResourceId);
		SetActivityStatus(*aNoteText);
		CleanupStack::PopAndDestroy(); // aNoteText
	}
}

void CBuddycloudLogic::ShowInformationDialogL(TInt aResourceId) {
	HBufC* aMessage = CCoeEnv::Static()->AllocReadResourceLC(aResourceId);
	CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
	aDialog->ExecuteLD(*aMessage);
	CleanupStack::PopAndDestroy(); // aMessage
}

TXmppPubsubAffiliation CBuddycloudLogic::ShowAffiliationDialogL(const TDesC& aNode, const TDesC& aJid, TXmppPubsubAffiliation aAffiliation, TBool aNotifyResult) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ShowAffiliationDialogL"));
#endif
	
	CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iFollowingList->GetItemById(IsSubscribedTo(aNode, EItemChannel)));
	
	if(aChannelItem && aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
			aChannelItem->GetPubsubAffiliation() > aAffiliation) {
		
		TInt aSelectedIndex = 0;
		TInt aCurrentIndex = 0;
		
		if(aAffiliation == EPubsubAffiliationMember) {
			aCurrentIndex = 1;
		}
		else if(aAffiliation == EPubsubAffiliationPublisher) {
			aCurrentIndex = 2;
		}
		else if(aAffiliation == EPubsubAffiliationModerator) {				
			aCurrentIndex = 3;
		}
		
		// Initialize dialog
		CAknListQueryDialog* aDialog = new (ELeave) CAknListQueryDialog(&aSelectedIndex);
		aDialog->PrepareLC(R_LIST_CHANGEPERMISSION);
		
		// Configure options
		CDesCArrayFlat* aOptionsList = CCoeEnv::Static()->ReadDesCArrayResourceL(R_ARRAY_CHANGEPERMISSION);
		CleanupStack::PushL(aOptionsList);
		
		if(aChannelItem->GetPubsubAffiliation() == EPubsubAffiliationModerator) {
			// Remove moderator option
			aOptionsList->Delete(3);
		}
	
		aDialog->SetItemTextArray(aOptionsList);
		aDialog->SetOwnershipType(ELbmOwnsItemArray);
		CleanupStack::Pop(); // aOptionsList
		
		// Select item & show dialog
		aDialog->ListBox()->SetCurrentItemIndex(aCurrentIndex);		
		
		if(aDialog->RunLD()) {
			switch(aSelectedIndex) {
				case 0: // Remove
					aAffiliation = EPubsubAffiliationOutcast;
					break;
				case 2: // Publisher
					aAffiliation = EPubsubAffiliationPublisher;
					break;
				case 3: // Moderator
					aAffiliation = EPubsubAffiliationModerator;
					break;
				default: // Follower
					aAffiliation = EPubsubAffiliationMember;
					break;					
			}
			
			SetPubsubNodeAffiliationL(aJid, aNode, aAffiliation, aNotifyResult);	
		}		
	}
	
	return aAffiliation;
}

TInt CBuddycloudLogic::DisplaySingleLinePopupMenuL(RPointerArray<HBufC>& aMenuItems) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::DisplaySingleLinePopupMenuL"));
#endif
	
	TInt aResult = KErrNotFound;
	
	if(aMenuItems.Count() > 1) {
		CDesCArrayFlat* aItems = new (ELeave) CDesCArrayFlat(aMenuItems.Count());
		CleanupStack::PushL(aItems);
		
		for(TInt i = 0; i < aMenuItems.Count(); i++) {
			aItems->AppendL(aMenuItems[i]->Des());
		}
		
		CAknSingleGraphicPopupMenuStyleListBox* aListBox = new (ELeave) CAknSingleGraphicPopupMenuStyleListBox;
	    CleanupStack::PushL(aListBox);
	    
	    CAknPopupList* aPopupList = CAknPopupList::NewL(aListBox, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);  
	    CleanupStack::PushL(aPopupList);
	    		 
	    aListBox->ConstructL(aPopupList, 0);
	    aListBox->CreateScrollBarFrameL(ETrue);
	    aListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	    aListBox->Model()->SetItemTextArray(aItems);
	    aListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	    
	    // Icons
	    CAknIconArray* aIcons = new (ELeave) CAknIconArray(7);
	    CleanupStack::PushL(aIcons); 
	    aIcons->ConstructFromResourceL(R_ICONS_LIST);
	    CleanupStack::Pop();  // aListBox
	    aListBox->ItemDrawer()->ColumnData()->SetIconArray(aIcons);    
	    
		if(aPopupList->ExecuteLD()) {
			aResult = aListBox->CurrentItemIndex();
		}
		
		CleanupStack::Pop(); // aPopupList
		CleanupStack::PopAndDestroy(2); // aListBox, aItems
	}
	else if(aMenuItems.Count() == 1) {
		aResult = 0;
	}
	
	return aResult;
}

TInt CBuddycloudLogic::DisplayDoubleLinePopupMenuL(RPointerArray<HBufC>& aMenuItems) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::DisplayDoubleLinePopupMenuL"));
#endif
	
	TInt aResult = KErrNotFound;
	
	if(aMenuItems.Count() > 1) {
		CDesCArrayFlat* aItems = new (ELeave) CDesCArrayFlat(aMenuItems.Count());
		CleanupStack::PushL(aItems);
		
		for(TInt i = 0; i < aMenuItems.Count(); i++) {
			aItems->AppendL(aMenuItems[i]->Des());
		}
		
		CAknDoubleGraphicPopupMenuStyleListBox* aListBox = new (ELeave) CAknDoubleGraphicPopupMenuStyleListBox;
	    CleanupStack::PushL(aListBox);
	    
	    CAknPopupList* aPopupList = CAknPopupList::NewL(aListBox, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuDoubleWindow);  
	    CleanupStack::PushL(aPopupList);
	    		 
	    aListBox->ConstructL(aPopupList, 0);
	    aListBox->CreateScrollBarFrameL(ETrue);
	    aListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
	
	    aListBox->Model()->SetItemTextArray(aItems);
	    aListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
	    
	    // Icons
	    CAknIconArray* aIcons = new (ELeave) CAknIconArray(7);
	    CleanupStack::PushL(aIcons); 
	    aIcons->ConstructFromResourceL(R_ICONS_LIST);
	    CleanupStack::Pop();  // aListBox
	    aListBox->ItemDrawer()->ColumnData()->SetIconArray(aIcons);    
	    
		if(aPopupList->ExecuteLD()) {
			aResult = aListBox->CurrentItemIndex();
		}
		
		CleanupStack::Pop(); // aPopupList
		CleanupStack::PopAndDestroy(2); // aListBox, aItems
	}
	else if(aMenuItems.Count() == 1) {
		aResult = 0;
	}
	
	return aResult;
}

//void CBuddycloudLogic::SendSmsOrEmailL(TDesC& aAddress, TDesC& aSubject, TDesC& aBody) {
//#ifdef _DEBUG
//	WriteToLog(_L8("BL    CBuddycloudLogic::SendSmsOrEmailL"));
//#endif
//
//	// Create sender & message
//	CSendUi* aSendUi = CSendUi::NewLC();
//	TUid aServiceUid = KSenduiMtmSmsUid;
//	CMessageData* aMessage = CMessageData::NewLC();
//	aMessage->AppendToAddressL(aAddress);
//	
//	if(aAddress.Locate('@') != KErrNotFound) {
//		// Email
//		aServiceUid = KSenduiMtmSmtpUid;
//		aMessage->SetSubjectL(&aSubject);
//	}
//	
//	// Create Body
//	CParaFormatLayer* aParaFormatLayer = CParaFormatLayer::NewL();
//	CleanupStack::PushL(aParaFormatLayer);
//	CCharFormatLayer* aCharFormatLayer = CCharFormatLayer::NewL();
//	CleanupStack::PushL(aCharFormatLayer);
//	CRichText* aRichText = CRichText::NewL(aParaFormatLayer, aCharFormatLayer);
//	CleanupStack::PushL(aRichText);
//	aRichText->InsertL(0, aBody);
//	aMessage->SetBodyTextL(aRichText);
//
//	// Send
//	aSendUi->CreateAndSendMessageL(aServiceUid, aMessage, KNullUid, false);
//	CleanupStack::PopAndDestroy(5); // aRichText, aCharFormatLayer, aParaFormatLayer, aMessage, aSendUi
//}
//
//void CBuddycloudLogic::SendInviteL(TInt aFollowerId) {
//#ifdef _DEBUG
//	WriteToLog(_L8("BL    CBuddycloudLogic::SendInviteL"));
//#endif
//
//	// Maitre'd
//	_LIT8(KInviteStanza, "<iq to='maitred.buddycloud.com' type='set' id='invite1'><invite xmlns='http://buddycloud.com/protocol/invite'><details></details></invite></iq>\r\n");		
//	_LIT8(KDetailElement, "<detail></detail>");
//	
//	// Maitre'd
//	HBufC8* aInviteStanza = HBufC8::NewL(KInviteStanza().Length());
//	TPtr8 pInviteStanza(aInviteStanza->Des());
//	pInviteStanza.Append(KInviteStanza);
//
//	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));
//
//	if(aItem && aItem->GetItemType() == EItemContact) {
//		CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
//
//		// Select contact
//		CContactItem* aContact = GetContactDetailsLC(aContactItem->GetAddressbookId());
//		
//		if(aContact != NULL) {
//			CContactItemFieldSet& aContactFieldSet = aContact->CardFields();
//			RPointerArray<HBufC> aDynItems;				
//			
//			// List phone numbers & email addresses
//			for(TInt x = 0; x < aContactFieldSet.Count(); x++) {
//				if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber) ||
//						aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
//					
//					// Add detail
//					HBufC* aDetailLine = HBufC::NewLC(2 + aContactFieldSet[x].Label().Length() + 1 + aContactFieldSet[x].TextStorage()->Text().Length());
//					TPtr pDetailLine(aDetailLine->Des());
//					pDetailLine.Append(aContactFieldSet[x].TextStorage()->Text());				
//					pDetailLine.Insert(0, _L("\t"));
//					pDetailLine.Insert(0, aContactFieldSet[x].Label());
//					
//					if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber)) {
//						pDetailLine.Insert(0, _L("0\t"));
//					}
//					else {
//						pDetailLine.Insert(0, _L("1\t"));
//					}
//					
//					aDynItems.Append(aDetailLine);
//					CleanupStack::Pop();
//					
//					// ---------------------------------------------------------
//					// Maitre'd
//					// ---------------------------------------------------------
//					TPtrC8 aEncDetail(iTextUtilities->UnicodeToUtf8L(aContactFieldSet[x].TextStorage()->Text()));
//
//					// Package detail
//					HBufC8* aDetailElement = HBufC8::NewLC(KDetailElement().Length() + aEncDetail.Length());
//					TPtr8 pDetailElement(aDetailElement->Des());
//					pDetailElement.Append(KDetailElement);
//					pDetailElement.Insert(8, aEncDetail);
//					
//					// Add to stanza
//					aInviteStanza = aInviteStanza->ReAlloc(pInviteStanza.Length()+pDetailElement.Length());
//					pInviteStanza.Set(aInviteStanza->Des());
//					pInviteStanza.Insert(pInviteStanza.Length() - 26, pDetailElement);
//
//					CleanupStack::PopAndDestroy(); // aDetailElement
//				}
//			}
//			
//			if(aDynItems.Count() == 0) {
//				aDynItems.Append(_L("\t").AllocL());
//			}
//			
//			// Selection result
//			TInt aSelectedItem = DisplayDoubleLinePopupMenuL(aDynItems);
//			
//			if(aSelectedItem != KErrNotFound) {
//				TPtr pSelectedAddress(aDynItems[aSelectedItem]->Des());
//				pSelectedAddress.Delete(0, pSelectedAddress.LocateReverse('\t') + 1);
//				
//				// Send invite to contact
//				HBufC* aSubject = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_INVITEMESSAGE_TITLE);
//				HBufC* aBody = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_INVITEMESSAGE_TEXT);					
//				SendSmsOrEmailL(pSelectedAddress, *aSubject, *aBody);
//				CleanupStack::PopAndDestroy(2); // aBody, aSubject
//				
//				// Send invite to maitre'd
//				iXmppEngine->SendAndForgetXmppStanza(pInviteStanza, true);
//			}
//				
//			while(aDynItems.Count() > 0) {
//				delete aDynItems[0];
//				aDynItems.Remove(0);
//			}
//			
//			aDynItems.Close();		
//			CleanupStack::PopAndDestroy(); // aContact
//		}			
//	}
//	
//	if(aInviteStanza) {
//		delete aInviteStanza;
//	}
//}
//
//void CBuddycloudLogic::SendPlaceL(TInt aFollowerId) {
//#ifdef _DEBUG
//	WriteToLog(_L8("BL    CBuddycloudLogic::SendPlaceL"));
//#endif
//
//	if(iLocationEngine->GetLastPlaceId() > 0) {
//		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));
//	
//		if(aItem && (aItem->GetItemType() == EItemContact || aItem->GetItemType() == EItemRoster)) {
//			CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
//
//			if(aContactItem->GetAddressbookId() > KErrNotFound) {
//				// Select contact
//				CContactItem* aContact = GetContactDetailsLC(aContactItem->GetAddressbookId());
//				
//				if(aContact != NULL) {
//					CContactItemFieldSet& aContactFieldSet = aContact->CardFields();
//					RPointerArray<HBufC> aDynItems;				
//					
//					// List phone numbers & email addresses
//					for(TInt x = 0; x < aContactFieldSet.Count(); x++) {
//						if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber) ||
//								aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
//							
//							// Add detail
//							HBufC* aDetailLine = HBufC::NewLC(2 + aContactFieldSet[x].Label().Length() + 1 + aContactFieldSet[x].TextStorage()->Text().Length());
//							TPtr pDetailLine(aDetailLine->Des());
//							pDetailLine.Append(aContactFieldSet[x].TextStorage()->Text());				
//							pDetailLine.Insert(0, _L("\t"));
//							pDetailLine.Insert(0, aContactFieldSet[x].Label());
//							
//							if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber)) {
//								pDetailLine.Insert(0, _L("0\t"));
//							}
//							else {
//								pDetailLine.Insert(0, _L("1\t"));
//							}
//						
//							aDynItems.Append(aDetailLine);
//							CleanupStack::Pop();
//						}
//					}
//					
//					if(aDynItems.Count() == 0) {
//						aDynItems.Append(_L("\t").AllocL());
//					}
//					
//					// Selection result
//					TInt aSelectedItem = DisplayDoubleLinePopupMenuL(aDynItems);
//					
//					if(aSelectedItem != KErrNotFound) {
//						TPtr pSelectedAddress(aDynItems[aSelectedItem]->Des());
//						pSelectedAddress.Delete(0, pSelectedAddress.LocateReverse('\t') + 1);
//						
//						// Construct body
//						TBuf<32> aId;
//						aId.Format(_L("%d"), iLocationEngine->GetLastPlaceId());
//						HBufC* aSendPlaceText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SENDPLACEMESSAGE_TEXT);
//						TPtr pSendPlaceText(aSendPlaceText->Des());
//						HBufC* aBody = HBufC::NewLC(pSendPlaceText.Length() + aId.Length() + iOwnItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length());
//						
//						TPtr pBody(aBody->Des());
//						pBody.Copy(pSendPlaceText);
//						pBody.Replace(pBody.Find(_L("$PLACE")), 6, iOwnItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText));
//						pBody.Replace(pBody.Find(_L("$ID")), 3, aId);
//						
//						HBufC* aSubject = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SENDPLACEMESSAGE_TITLE);			
//						
//						SendSmsOrEmailL(pSelectedAddress, *aSubject, pBody);
//						CleanupStack::PopAndDestroy(3); // aSubject, aBody, aSendPlaceText
//					}
//						
//					while(aDynItems.Count() > 0) {
//						delete aDynItems[0];
//						aDynItems.Remove(0);
//					}
//					
//					aDynItems.Close();		
//					CleanupStack::PopAndDestroy(); // aContact
//				}	
//			}
//		}
//	}
//}

void CBuddycloudLogic::FollowContactL(const TDesC& aContact) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowContactL"));
#endif

	if(aContact.Locate('@') != KErrNotFound || aContact.Locate('.') != KErrNotFound) {	
		HBufC8* aEncJid = iTextUtilities->UnicodeToUtf8L(aContact).AllocLC();
		
		// Prepare data
		_LIT8(KNickElement, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
		HBufC8* aEncFullName = iTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtr8 pEncFullName(aEncFullName->Des());
		
		_LIT8(KGeolocElement, "<geoloc xmlns='http://jabber.org/protocol/geoloc'><text></text></geoloc>");
		HBufC8* aEncLocation = iTextUtilities->UnicodeToUtf8L(*iBroadLocationText).AllocLC();
		TPtr8 pEncLocation(aEncLocation->Des());
		
		// Build & send request
		HBufC8* aOptionalData = HBufC8::NewLC(KNickElement().Length() + pEncFullName.Length() + KGeolocElement().Length() + pEncLocation.Length());
		TPtr8 pOptionalData(aOptionalData->Des());
		
		if(pEncFullName.Length() > 0) {
			pOptionalData.Append(KNickElement);
			pOptionalData.Insert(pOptionalData.Length() - 7, pEncFullName);
		}
		
		if(pEncLocation.Length() > 0) {
			pOptionalData.Append(KGeolocElement);
			pOptionalData.Insert(pOptionalData.Length() - 16, pEncLocation);
		}
		
		// Check following item exists, if not, add roster item
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(IsSubscribedTo(aContact, EItemRoster)));	

		if(aItem == NULL) {
			AddRosterManagementItemL(*aEncJid);
		}
		
		SendPresenceSubscriptionL(*aEncJid, _L8("subscribe"), pOptionalData);
		CleanupStack::PopAndDestroy(4); // aOptionalData, aEncLocation, aEncFullName, aEncJid
				
		// Acknowledge request sent
		ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_REQUESTSENT);		
	}
}

void CBuddycloudLogic::SendPresenceToPubsubL(const TDesC8& aLastNodeId, TXmppMessagePriority aPriority) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceToPubsubL"));
#endif

	// Send direct presence
	_LIT8(KRsmElement, "<set xmlns='http://jabber.org/protocol/rsm'><after></after></set>");
	_LIT8(KPubsubPresence, "<presence to=''></presence>\r\n");
	HBufC8* aPubsubPresence = HBufC8::NewLC(KPubsubPresence().Length() + KRsmElement().Length() + KBuddycloudPubsubServer().Length() + aLastNodeId.Length());
	TPtr8 pPubsubPresence(aPubsubPresence->Des());
	pPubsubPresence.Append(KPubsubPresence);
	
	if(aLastNodeId.Length() > 0) {
		pPubsubPresence.Insert(16, KRsmElement);
		pPubsubPresence.Insert(16 + 51, aLastNodeId);
	}
	
	pPubsubPresence.Insert(14, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndForgetXmppStanza(pPubsubPresence, false, aPriority);
	CleanupStack::PopAndDestroy(); // aPubsubPresence	
}

void CBuddycloudLogic::SetLastNodeIdReceived(const TDesC8& aNodeItemId) {
	if(aNodeItemId.Length() > 0) {
		if(iLastNodeIdReceived) {
			delete iLastNodeIdReceived;
		}
		
		iLastNodeIdReceived = aNodeItemId.Alloc();
	}
}

void CBuddycloudLogic::CollectPubsubSubscriptionsL(const TDesC8& aAfter) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectPubsubSubscriptionsL"));
#endif

	// Collect users pubsub subscriptions
	_LIT8(KPubsubSubscriptions, "<iq to='' type='get' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><subscriptions/><set xmlns='http://jabber.org/protocol/rsm'><max>50</max></set></pubsub></iq>\r\n");
	_LIT8(KRsmAfterElement, "<after></after>");

	HBufC8* aPubsubSubscriptions = HBufC8::NewLC(KPubsubSubscriptions().Length() + KBuddycloudPubsubServer().Length() + KRsmAfterElement().Length() + aAfter.Length());
	TPtr8 pPubsubSubscriptions(aPubsubSubscriptions->Des());
	pPubsubSubscriptions.AppendFormat(KPubsubSubscriptions, EXmppIdGetPubsubSubscriptions, GetNewIdStamp());
	
	if(aAfter.Length() > 0) {
		pPubsubSubscriptions.Insert(pPubsubSubscriptions.Length() - 22, KRsmAfterElement);
		pPubsubSubscriptions.Insert(pPubsubSubscriptions.Length() - 30, aAfter);
	}

	pPubsubSubscriptions.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pPubsubSubscriptions, this, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aPubsubSubscriptions	
}

void CBuddycloudLogic::ProcessPubsubSubscriptionsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPubsubSubscriptionsL"));
#endif
	
	TPtrC8 pLastNodeIdReceived(iLastNodeIdReceived->Des());
	TBool aItemFound = false;
	
	// Synchronize local channel to received channel subscriptions
	for(TInt i = (iFollowingList->Count() - 1); i >= 0; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem && aItem->GetItemType() >= EItemRoster && aItem->GetId().Length() > 0) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
			
			aItemFound = false;
			
			for(TInt x = 0; x < iGenericList->Count(); x++) {
				CFollowingChannelItem* aSubscribedItem = static_cast <CFollowingChannelItem*> (iGenericList->GetItemByIndex(x));
				
				if(aChannelItem->GetId().CompareF(aSubscribedItem->GetId()) == 0) {	
					// Item found, update subscription & affiliation
					aChannelItem->SetPubsubSubscription(aSubscribedItem->GetPubsubSubscription());
					aChannelItem->SetPubsubAffiliation(aSubscribedItem->GetPubsubAffiliation());
					
					iGenericList->DeleteItemByIndex(x);
					
					aItemFound = true;
					break;
				}
			}
			
			if(!aItemFound) {
				// No longer subscribed to a channel, remove
				iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());	
				
				aChannelItem->SetUnreadData(NULL);
				aChannelItem->SetIdL(KNullDesC);
				
				if(aItem->GetItemType() == EItemChannel) {
					iFollowingList->DeleteItemByIndex(i);
				}
			}
		}
	}
	
	// Add new channel items
	while(iGenericList->Count() > 0) {
		CFollowingChannelItem* aSubscribedItem = static_cast <CFollowingChannelItem*> (iGenericList->GetItemByIndex(0));
		
		if(aSubscribedItem->GetId().Find(_L("/channel")) != KErrNotFound) {
			CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aSubscribedItem->GetId());
			
			TPtrC8 aEncNodeId(iTextUtilities->UnicodeToUtf8L(aSubscribedItem->GetId()));		
			CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aEncNodeId);
			
			aItemFound = false;
			
			if(aNodeParser->GetNode(0).Compare(_L8("user")) == 0) {
				// Is a user channel
				TInt aItemId = KErrNotFound;
				
				if((aItemId = IsSubscribedTo(iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)), EItemRoster)) > 0) {
					// User is followed, add bind channel to user
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iFollowingList->GetItemById(aItemId));
					aChannelItem->SetIdL(aSubscribedItem->GetId());
					aChannelItem->SetPubsubSubscription(aSubscribedItem->GetPubsubSubscription());
					aChannelItem->SetPubsubAffiliation(aSubscribedItem->GetPubsubAffiliation());
					aChannelItem->SetUnreadData(aDiscussion);
	
					aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());
					
//					iGenericList->DeleteItemByIndex(0);
//					aItemFound = true;
				}
				
				// TODO: remove user channel limited
				iGenericList->DeleteItemByIndex(0);
				aItemFound = true;
			}
			
			if(!aItemFound) {
				// Set channel data
				aSubscribedItem->SetItemId(iNextItemId++);
				aSubscribedItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)));
				aSubscribedItem->SetUnreadData(aDiscussion);
							
				// Add to discussion manager
				aDiscussion->SetDiscussionReadObserver(this, aSubscribedItem->GetItemId());		
			
				iFollowingList->InsertItem(GetBubbleToPosition(aSubscribedItem), aSubscribedItem);		
				iGenericList->RemoveItemByIndex(0);
				
				// Collect metadata
				CollectChannelMetadataL(aSubscribedItem->GetId());		
			}
			
			CleanupStack::PopAndDestroy(); // aNodeParser
			
			// Collect node items			
			if(pLastNodeIdReceived.Length() > 1) {
				CollectLastPubsubNodeItemsL(aDiscussion->GetDiscussionId(), _L8("1"));
			}
		}
		else {
			// Not a channel
			iGenericList->DeleteItemByIndex(0);
		}
	}
	
	iConnectionCold = false;
	iSettingsSaveNeeded = true;
	
	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
	
	// Send presence to pubsub
	SendPresenceToPubsubL(pLastNodeIdReceived, EXmppPriorityHigh);
	
	// Send queued stanzas
	iXmppEngine->SendQueuedXmppStanzas();

	// Collect users pubsub node subscribers
	CollectUsersPubsubNodeAffiliationsL();
	
	// TODO: Workaround for google
	SendPresenceToPubsubL(KNullDesC8);
	
	// Enable google presence queuing
	if(iSettingXmppServer.Match(_L("*google.com*")) != KErrNotFound) {
		iXmppEngine->SendAndForgetXmppStanza(_L8("<iq type='set'><query xmlns='google:queue'><enable/></query></iq>"), false);
	}
}

void CBuddycloudLogic::CollectUsersPubsubNodeAffiliationsL(const TDesC8& aAfter) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectUsersPubsubNodeAffiliationsL"));
#endif
	
	if(iOwnItem) {
		TPtrC8 aEncId = iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId(EIdChannel));
		
		// Collect users pubsub node affiliations
		_LIT8(KNodeAffiliations, "<iq to='' type='get' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''/><set xmlns='http://jabber.org/protocol/rsm'><max>50</max></set></pubsub></iq>\r\n");
		_LIT8(KRsmAfterElement, "<after></after>");
		
		HBufC8* aNodeAffiliations = HBufC8::NewLC(KNodeAffiliations().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length() + KRsmAfterElement().Length() + aAfter.Length());
		TPtr8 pNodeAffiliations(aNodeAffiliations->Des());
		pNodeAffiliations.AppendFormat(KNodeAffiliations, EXmppIdGetUsersPubsubNodeAffiliations, GetNewIdStamp());
		
		if(aAfter.Length() > 0) {
			pNodeAffiliations.Insert(pNodeAffiliations.Length() - 22, KRsmAfterElement);
			pNodeAffiliations.Insert(pNodeAffiliations.Length() - 30, aAfter);
		}
		
		pNodeAffiliations.Insert(108, aEncId);
		pNodeAffiliations.Insert(8, KBuddycloudPubsubServer);
	
		iXmppEngine->SendAndAcknowledgeXmppStanza(pNodeAffiliations, this, false, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(); // aNodeAffiliations
	}
}

void CBuddycloudLogic::ProcessUsersPubsubNodeAffiliationsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessUsersPubsubNodeAffiliationsL"));
#endif
	
	// Users node id
	HBufC8* aEncId = iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId(EIdChannel)).AllocLC();
	TPtrC8 pEncId(aEncId->Des());
	
	_LIT8(KAffiliationItem, "<affiliation jid='' affiliation=''/>");
	HBufC8* aDeltaList = HBufC8::NewLC(0);
	TPtr8 pDeltaList(aDeltaList->Des());
	
	TInt aDeltaCount = 0;
	
	// Syncronization of roster items & subscribers
	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom) {
				TBool aUserFound = false;
					
				for(TInt x = 0; x < iGenericList->Count(); x++) {
					CFollowingRosterItem* aSubscriberItem = static_cast <CFollowingRosterItem*> (iGenericList->GetItemByIndex(x));
					
					if(aRosterItem->GetId().CompareF(aSubscriberItem->GetId()) == 0) {						
						// Delete subscriber from list
						iGenericList->DeleteItemByIndex(x);
						
						aUserFound = true;
						break;
					}
				}
				
				if(!aUserFound) {
					// Add user to subscribers
					TPtrC8 aEncJid = iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId());
					
					if(pDeltaList.MaxLength() < (pDeltaList.Length() + KAffiliationItem().Length() + aEncJid.Length() + KPubsubAffiliationPublisher().Length())) {
						CleanupStack::Pop(aDeltaList);
						aDeltaList = aDeltaList->ReAlloc(pDeltaList.Length() + KAffiliationItem().Length() + aEncJid.Length() + KPubsubAffiliationPublisher().Length());
						pDeltaList.Set(aDeltaList->Des());
						CleanupStack::PushL(aDeltaList);
					}
					
					pDeltaList.Append(KAffiliationItem);
					pDeltaList.Insert(pDeltaList.Length() - 18, aEncJid);
					pDeltaList.Insert(pDeltaList.Length() - 3, KPubsubAffiliationPublisher);
					
					aDeltaCount++;
					
					if(aDeltaCount >= 30) {
						// Send current delta list
						SetPubsubNodeAffiliationsL(pEncId, pDeltaList);
						
						// Reset delta list
						aDeltaCount = 0;	
						pDeltaList.Zero();
					}
				}
			}
		}
	}
	
	// Send affiliation delta
	if(pDeltaList.Length() > 0) {
		SetPubsubNodeAffiliationsL(pEncId, pDeltaList);
		
		pDeltaList.Zero();
	}
	
	// Remove users from subscribers
	if(iGenericList->Count() > 0) {
		_LIT8(KSubscriptionStanza, "<iq to='' type='set' id='subscription:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><subscriptions node=''></subscriptions></pubsub></iq>\r\n");	
		_LIT8(KSubscriptionItem, "<subscription jid='' subscription=''/>");
		
		while(iGenericList->Count() > 0) {
			CFollowingRosterItem* aSubscriberItem = static_cast <CFollowingRosterItem*> (iGenericList->GetItemByIndex(0));
			TPtrC8 aEncJid = iTextUtilities->UnicodeToUtf8L(aSubscriberItem->GetId());
			
			if(pDeltaList.MaxLength() < (pDeltaList.Length() + KSubscriptionItem().Length() + aEncJid.Length() + KPubsubSubscriptionNone().Length())) {
				CleanupStack::Pop(aDeltaList);
				aDeltaList = aDeltaList->ReAlloc(pDeltaList.Length() + KSubscriptionItem().Length() + aEncJid.Length() + KPubsubSubscriptionNone().Length());
				pDeltaList.Set(aDeltaList->Des());
				CleanupStack::PushL(aDeltaList);
			}

			pDeltaList.Append(KSubscriptionItem);
			pDeltaList.Insert(pDeltaList.Length() - 19, aEncJid);
			pDeltaList.Insert(pDeltaList.Length() - 3, KPubsubSubscriptionNone);					

			iGenericList->DeleteItemByIndex(0);
		}
		
		// Send subscription delta
		HBufC8* aSubscriptionStanza = HBufC8::NewLC(KSubscriptionStanza().Length() + KBuddycloudPubsubServer().Length() + pEncId.Length() + pDeltaList.Length());
		TPtr8 pSubscriptionStanza(aSubscriptionStanza->Des());
		pSubscriptionStanza.AppendFormat(KSubscriptionStanza, GetNewIdStamp());
		pSubscriptionStanza.Insert(121, pDeltaList);
		pSubscriptionStanza.Insert(119, pEncId);
		pSubscriptionStanza.Insert(8, KBuddycloudPubsubServer);

		iXmppEngine->SendAndForgetXmppStanza(pSubscriptionStanza, true);
		CleanupStack::PopAndDestroy(); // aSubscriptionStanza
	}

	CleanupStack::PopAndDestroy(2); // aDeltaList, aEncId	
	
#ifdef _DEBUG
	// Pre-follow debug channels
	_LIT(KBetaTestersChannel, "/channel/beta-testers");
	_LIT(KTranslatorsChannel, "/channel/translators");
	
	if(!IsSubscribedTo(KBetaTestersChannel, EItemChannel)) {
		FollowChannelL(KBetaTestersChannel);
	}
	
	if(!IsSubscribedTo(KTranslatorsChannel, EItemChannel)) {
		FollowChannelL(KTranslatorsChannel);
	}
#endif
}

void CBuddycloudLogic::CollectLastPubsubNodeItemsL(const TDesC& aNode, const TDesC8& aHistoryAfterItem) {
	if(aNode.Length() > 0) {
		TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));
	
		_LIT8(KRsmContainer, "<set xmlns='http://jabber.org/protocol/rsm'><after></after></set>");
		_LIT8(KNodeItemsStanza, "<iq to='' type='get' id='nodeitems:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node=''></items></pubsub></iq>\r\n");
		HBufC8* aNodeItemsStanza = HBufC8::NewLC(KNodeItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + KRsmContainer().Length() + aHistoryAfterItem.Length());
		TPtr8 pNodeItemsStanza(aNodeItemsStanza->Des());
		pNodeItemsStanza.AppendFormat(KNodeItemsStanza, GetNewIdStamp());
		
		if(aHistoryAfterItem.Length() > 0) {
			pNodeItemsStanza.Insert(104, KRsmContainer);
			pNodeItemsStanza.Insert(155, aHistoryAfterItem);
		}
		
		pNodeItemsStanza.Insert(102, aEncNode);
		pNodeItemsStanza.Insert(8, KBuddycloudPubsubServer);
		
		iXmppEngine->SendAndForgetXmppStanza(pNodeItemsStanza, false);
		CleanupStack::PopAndDestroy(); // aNodeItemsStanza
	}
}

void CBuddycloudLogic::CollectUserPubsubNodeL(const TDesC& aJid, const TDesC& aNodeLeaf, const TDesC8& aHistoryAfterItem) {
	if(aJid.Locate('@') != KErrNotFound && aNodeLeaf.Length() > 0) {
		_LIT(KNodeRoot, "/user/");
		HBufC* aNode = HBufC::NewLC(KNodeRoot().Length() + aJid.Length() + 1 + aNodeLeaf.Length());
		TPtr pNode(aNode->Des());
		pNode.Append(KNodeRoot);
		pNode.Append(aJid);
		
		if(aNodeLeaf.Locate('/') != 0) {
			pNode.Append('/');
		}
		
		pNode.Append(aNodeLeaf);

		CollectLastPubsubNodeItemsL(pNode, aHistoryAfterItem);
		CleanupStack::PopAndDestroy(); // aNode
	}
}

void CBuddycloudLogic::CollectChannelMetadataL(const TDesC& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectChannelMetadataL"));
#endif
	
	TPtrC8 aEncId = iTextUtilities->UnicodeToUtf8L(aNode);
	
	_LIT8(KDiscoItemsStanza, "<iq to='' type='get' id='%02d:%02d'><query xmlns='http://jabber.org/protocol/disco#info' node=''/></iq>\r\n");
	HBufC8* aDiscoItemsStanza = HBufC8::NewLC(KDiscoItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length());
	TPtr8 pDiscoItemsStanza(aDiscoItemsStanza->Des());
	pDiscoItemsStanza.AppendFormat(KDiscoItemsStanza, EXmppIdGetChannelMetadata, GetNewIdStamp());
	pDiscoItemsStanza.Insert(91, aEncId);
	pDiscoItemsStanza.Insert(8, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pDiscoItemsStanza, this, true);
	CleanupStack::PopAndDestroy(); // aDiscoItemsStanza
}

void CBuddycloudLogic::ProcessChannelMetadataL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessChannelMetadataL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);

	if(aXmlParser->MoveToElement(_L8("query")) || aXmlParser->MoveToElement(_L8("configuration"))) {		
		TPtrC8 aAttributeNode(aXmlParser->GetStringAttribute(_L8("node")));
		TPtrC aEncAttributeNode(iTextUtilities->Utf8ToUnicodeL(aAttributeNode));
		
		if(aAttributeNode.Length() > 0) {
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
				
				if(aItem && aItem->GetItemType() == EItemChannel) {
					if(aItem->GetId().CompareF(aEncAttributeNode) == 0) {
						CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
						
						// Set sub title
						if(aChannelItem->GetSubTitle().Length() == 0) {
							CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aAttributeNode);
							TPtrC aEncId(iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)));
								
							HBufC* aIdText = HBufC::NewLC(aEncId.Length() + 1);
							TPtr pIdText(aIdText->Des());
							pIdText.Append('#');
							pIdText.Append(aEncId);
								
							aChannelItem->SetSubTitleL(pIdText);
							CleanupStack::PopAndDestroy(2); // aIdText, aNodeParser
						}
	
						// Collect metadata
						while(aXmlParser->MoveToElement(_L8("field"))) {
							TPtrC8 aAttributeVar(aXmlParser->GetStringAttribute(_L8("var")));
							
							if(aXmlParser->MoveToElement(_L8("value"))) {
								TPtrC aEncDataValue(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));							
								
								if(aAttributeVar.Compare(_L8("pubsub#title")) == 0) {
									aChannelItem->SetTitleL(aEncDataValue);
								}
								else if(aAttributeVar.Compare(_L8("pubsub#description")) == 0) {
									aChannelItem->SetDescriptionL(aEncDataValue);
								}
							}
						}
						
						NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aChannelItem->GetItemId());
						
						break;
					}
				}
				
			}
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::SetPubsubNodeAffiliationL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubAffiliation aAffiliation, TBool aNotifyResult) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetPubsubNodeAffiliationL"));
#endif
	
	HBufC8* aEncJid = iTextUtilities->UnicodeToUtf8L(aJid).AllocLC();	
	TPtrC8 pEncJid(aEncJid->Des());	
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	TPtrC8 aAffiliationText(CXmppEnumerationConverter::PubsubAffiliation(aAffiliation));
	
	_LIT8(KAffiliationStanza, "<iq to='' type='set' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''><affiliation jid='' affiliation=''/></affiliations></pubsub></iq>\r\n");	
	HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + pEncJid.Length() + aAffiliationText.Length());
	TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
	pAffiliationStanza.AppendFormat(KAffiliationStanza, EXmppIdChangeChannelAffiliation, GetNewIdStamp());
	pAffiliationStanza.Insert(143, aAffiliationText);
	pAffiliationStanza.Insert(128, pEncJid);
	pAffiliationStanza.Insert(108, aEncNode);
	pAffiliationStanza.Insert(8, KBuddycloudPubsubServer);

	if(aNotifyResult) {
		iXmppEngine->SendAndAcknowledgeXmppStanza(pAffiliationStanza, this, true);
	}
	else {
		iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
	}
	
	CleanupStack::PopAndDestroy(2); // aAffiliationStanza, aEncJid
}

void CBuddycloudLogic::SetPubsubNodeAffiliationsL(const TDesC8& aNode, const TDesC8& aAffiliations) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetPubsubNodeAffiliationsL"));
#endif
	
	_LIT8(KAffiliationStanza, "<iq to='' type='set' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''></affiliations></pubsub></iq>\r\n");	
	HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + KBuddycloudPubsubServer().Length() + aAffiliations.Length() + aAffiliations.Length());
	TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
	pAffiliationStanza.AppendFormat(KAffiliationStanza, EXmppIdChangeChannelAffiliation, GetNewIdStamp());
	pAffiliationStanza.Insert(110, aAffiliations);
	pAffiliationStanza.Insert(108, aNode);
	pAffiliationStanza.Insert(8, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
	CleanupStack::PopAndDestroy(); // aAffiliationStanza
}

void CBuddycloudLogic::RequestPubsubNodeAffiliationL(const TDesC& aNode, TXmppPubsubAffiliation aAffiliation, const TDesC& aText) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RequestPubsubNodeAffiliationL"));
#endif
	
	HBufC8* aEncText = iTextUtilities->UnicodeToUtf8L(aText).AllocLC();
	HBufC8* aEncJid = iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()).AllocLC();
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	TPtrC8 aAffiliationText(CXmppEnumerationConverter::PubsubAffiliation(aAffiliation));
	
	_LIT8(KAffiliationStanza, "<iq to='' type='set' id='requestaffiliation:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><affiliation node='' jid='' affiliation=''><text></text></affiliation></pubsub></iq>\r\n");	
	HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + aEncJid->Des().Length() + aAffiliationText.Length() + aEncText->Des().Length());
	TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
	pAffiliationStanza.AppendFormat(KAffiliationStanza, GetNewIdStamp());
	pAffiliationStanza.Insert(147, *aEncText);
	pAffiliationStanza.Insert(139, aAffiliationText);
	pAffiliationStanza.Insert(124, *aEncJid);
	pAffiliationStanza.Insert(117, aEncNode);
	pAffiliationStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
	CleanupStack::PopAndDestroy(3); // aAffiliationStanza, aEncJid, aEncText
	
	// Show request sent dialog
	ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_REQUESTSENT);
}

void CBuddycloudLogic::SetPubsubNodeSubscriptionL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubSubscription aSubscription) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetPubsubNodeSubscriptionL"));
#endif
	
	HBufC8* aEncJid = iTextUtilities->UnicodeToUtf8L(aJid).AllocLC();	
	TPtrC8 pEncJid(aEncJid->Des());	
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	TPtrC8 aSubscriptionText(CXmppEnumerationConverter::PubsubSubscription(aSubscription));
	
	_LIT8(KSubscriptionStanza, "<iq to='' type='set' id='subscription:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><subscriptions node=''><subscription jid='' subscription=''/></subscriptions></pubsub></iq>\r\n");	
	HBufC8* aSubscriptionStanza = HBufC8::NewLC(KSubscriptionStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + pEncJid.Length() + aSubscriptionText.Length());
	TPtr8 pSubscriptionStanza(aSubscriptionStanza->Des());
	pSubscriptionStanza.AppendFormat(KSubscriptionStanza, GetNewIdStamp());
	pSubscriptionStanza.Insert(156, aSubscriptionText);
	pSubscriptionStanza.Insert(140, pEncJid);
	pSubscriptionStanza.Insert(119, aEncNode);
	pSubscriptionStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pSubscriptionStanza, true);
	CleanupStack::PopAndDestroy(2); // aSubscriptionStanza, aEncJid
}

void CBuddycloudLogic::RetractPubsubNodeItemL(const TDesC& aNode, const TDesC8& aNodeItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RetractPubsubNodeItemL"));
#endif
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	
	_LIT8(KRetractStanza, "<iq to='' type='set' id='retractitem:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><retract node='' notify='1'><item id=''/></retract></pubsub></iq>\r\n");	
	HBufC8* aRetractStanza = HBufC8::NewLC(KRetractStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + aNodeItemId.Length());
	TPtr8 pRetractStanza(aRetractStanza->Des());
	pRetractStanza.AppendFormat(KRetractStanza, GetNewIdStamp());
	pRetractStanza.Insert(129, aNodeItemId);
	pRetractStanza.Insert(106, aEncNode);
	pRetractStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pRetractStanza, true);
	CleanupStack::PopAndDestroy(); // aRetractStanza
}

void CBuddycloudLogic::HandlePubsubEventL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePubsubEventL"));
#endif
	
	TBool aBubbleEvent = false;
	TInt aNotifiedItemId = KErrNotFound;
	TInt aNotifiedAudioId = KErrNotFound;
	
	TXmppPubsubEventType aPubsubEventType;
	
	// Initialize xml parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	// Get pubsub event type
	if(aXmlParser->MoveToElement(_L8("items"))) {
		// Items published or retracted
		aPubsubEventType = EPubsubEventItems;
	}
	else if(aXmlParser->MoveToElement(_L8("subscription"))) {
		// Subscriptions changed
		aPubsubEventType = EPubsubEventSubscription;
	}
	else if(aXmlParser->MoveToElement(_L8("affiliation"))) {
		// Affiliations changed
		aPubsubEventType = EPubsubEventAffiliation;
	}
	else if(aXmlParser->MoveToElement(_L8("configuration"))) {
		// Metadata changed
		ProcessChannelMetadataL(aStanza);
	}
	else if(aXmlParser->MoveToElement(_L8("purge"))) {
		// Purge items
		aPubsubEventType = EPubsubEventPurge;
	}
	else if(aXmlParser->MoveToElement(_L8("delete"))) {
		// Purge items
		aPubsubEventType = EPubsubEventDelete;
	}
	
	if(aPubsubEventType > EPubsubEventNone) {
		// Handle event type
		TPtrC8 aAttributeNode(aXmlParser->GetStringAttribute(_L8("node")));
		CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aAttributeNode);
			 	
		if(aNodeParser->Count() > 1) {
			HBufC* aUserJid = iTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)).AllocLC();
			TPtrC pUserJid(aUserJid->Des());
			
			TPtrC aNodeId(iTextUtilities->Utf8ToUnicodeL(aAttributeNode));		
			
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
				
				if(aItem && aItem->GetItemType() >= EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
					if((aItem->GetItemType() == EItemRoster && aRosterItem->GetId().CompareF(pUserJid) == 0) || 
							aItem->GetId().CompareF(aNodeId) == 0) {
						
						aNotifiedItemId = aItem->GetItemId();
						
						if(aPubsubEventType == EPubsubEventItems) {
							// Handle item or retract event
							while(aXmlParser->MoveToElement(_L8("item")) || aXmlParser->MoveToElement(_L8("retract"))) {
								// Handle item or retract event
								TPtrC8 aElement(aXmlParser->GetElementName());
								TPtrC8 aItemId(aXmlParser->GetStringAttribute(_L8("id")));
								
								if(aItem->GetItemType() == EItemRoster && aNodeParser->GetNode(2).Compare(_L8("mood")) == 0) {						
									if(aElement.Compare(_L8("item")) == 0 && aXmlParser->MoveToElement(_L8("mood"))) {
										// XEP-0107: User mood
										if(aXmlParser->MoveToElement(_L8("text"))) {
											TPtrC aMood(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
											
											if(!aRosterItem->OwnItem() && aRosterItem->GetDescription().CompareF(aMood) != 0) {
												aBubbleEvent = true;
											}
											
											aRosterItem->SetDescriptionL(aMood);
										}
										
										// Set last node id received
										SetLastNodeIdReceived(aItemId);
									}
									else if(aElement.Compare(_L8("retract")) == 0) {
										// Deleted item
										aRosterItem->SetDescriptionL(KNullDesC);
									}
								}
								else if(aItem->GetItemType() == EItemRoster && aNodeParser->GetNode(2).Compare(_L8("geo")) == 0) {
									TGeolocItemType aGeolocItemType = EGeolocItemBroad;
									
									if(aNodeParser->GetNode(3).Compare(_L8("current")) == 0) {
										aGeolocItemType = EGeolocItemCurrent;
									}
									else if(aNodeParser->GetNode(3).Compare(_L8("previous")) == 0) {
										aGeolocItemType = EGeolocItemPrevious;
									}
									else if(aNodeParser->GetNode(3).Compare(_L8("future")) == 0) {
										aGeolocItemType = EGeolocItemFuture;
									}
									
									if(aGeolocItemType < EGeolocItemBroad) {
										if(aElement.Compare(_L8("item")) == 0 && aXmlParser->MoveToElement(_L8("geoloc"))) {
											// XEP-0080: User Geoloc
											CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
											CGeolocData* aGeoloc = aGeolocParser->XmlToGeolocLC(aXmlParser->GetStringData());
											TPtr aBroadLocation(iBroadLocationText->Des());
											
											if(aRosterItem->OwnItem()) {
												if(aGeolocItemType == EGeolocItemCurrent) {										
													// Handle broad location
													CGeolocData* aOldGeoloc = aRosterItem->GetGeolocItem(EGeolocItemCurrent);
													
													if(aBroadLocation.Length() == 0 || 
															(aOldGeoloc->GetString(EGeolocLocality).Compare(aGeoloc->GetString(EGeolocLocality)) != 0 ||
															 aOldGeoloc->GetString(EGeolocCountry).Compare(aGeoloc->GetString(EGeolocCountry)) != 0)) {
														
														// Broad location changed
														aBroadLocation.Zero();
														
														iBroadLocationText = iBroadLocationText->ReAlloc(aGeoloc->GetString(EGeolocLocality).Length() + 2 + aGeoloc->GetString(EGeolocCountry).Length());
														aBroadLocation.Set(iBroadLocationText->Des());
														
														iTextUtilities->AppendToString(aBroadLocation, aGeoloc->GetString(EGeolocLocality), KNullDesC);
														iTextUtilities->AppendToString(aBroadLocation, aGeoloc->GetString(EGeolocCountry), _L(", "));
														
														// Send presence
														SendPresenceL();
													}
													
													// Check future location
													TPtrC aOwnFuturePlace(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText));
													
													if(aOwnFuturePlace.Length() > 0 && aOwnFuturePlace.FindF(aGeoloc->GetString(EGeolocText)) != KErrNotFound) {
														iXmppEngine->SendAndForgetXmppStanza(_L8("<iq to='butler.buddycloud.com' type='set' id='next2'><query xmlns='http://buddycloud.com/protocol/place#next'><place><name/></place></query></iq>\r\n"), true);
													}
												}
											}
											else if(aGeolocItemType > EGeolocItemPrevious && 
													aRosterItem->GetGeolocItem(aGeolocItemType)->GetString(EGeolocText).Compare(aGeoloc->GetString(EGeolocText)) != 0) {
													
												// Trigger bubbling
												aBubbleEvent = true;
											}
											
											aRosterItem->SetGeolocItemL(aGeolocItemType, aGeoloc);
											CleanupStack::Pop(); // aGeoloc
											
											CleanupStack::PopAndDestroy(); // aGeolocParser
											
											// Set last node id received
											SetLastNodeIdReceived(aItemId);
										}
										else if(aElement.Compare(_L8("retract")) == 0) {
											// Deleted item
											aRosterItem->SetGeolocItemL(aGeolocItemType, NULL);
										}
									}
								}
								else if(aAttributeNode.Find(_L8("/channel")) != KErrNotFound) {
									CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
									CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
									
									if(aElement.Compare(_L8("item")) == 0 && aXmlParser->MoveToElement(_L8("entry"))) {
										// Channel post
										TBuf8<32> aReferenceId;
										
										CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();						
										CAtomEntryData* aAtomEntry = aAtomEntryParser->XmlToAtomEntryLC(aXmlParser->GetStringData(), aReferenceId);									
										aAtomEntry->SetIdL(aItemId);
										
										// Overwrite & check jid sensoring
										HBufC* aAuthorName = aAtomEntry->GetAuthorJid().AllocLC();
										TPtr pAuthorName(aAuthorName->Des());
										
										if(aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationModerator && 
												!IsSubscribedTo(pAuthorName, EItemRoster)) {
											
											for(TInt i = (pAuthorName.Locate('@') - 1), x = (i - 2); i > 1 && i > x; i--) {
												pAuthorName[i] = 46;
											}
										}
										
										aAtomEntry->SetAuthorNameL(pAuthorName);
										CleanupStack::PopAndDestroy(); // aAuthorName
										
										// Set entry flags
										if(aAtomEntry->GetAuthorJid().Compare(iOwnItem->GetId()) == 0) {
											// Own post
											aAtomEntry->SetRead(true);
										}
										else if(aReferenceId.Length() > 0) {
											CThreadedEntry* aReferenceEntry = aDiscussion->GetThreadedEntryById(aReferenceId);
											
											if(aReferenceEntry && aReferenceEntry->GetEntry()->GetAuthorJid().Compare(iOwnItem->GetId()) == 0) {
												// Comment to own post
												aAtomEntry->SetDirectReply(true);
											}
										}
										
										if(aDiscussion->AddEntryOrCommentLD(aAtomEntry, aReferenceId)) {
											// Bubble
											if(iFollowingList->GetIndexById(aNotifiedItemId) > 0) {
												aBubbleEvent = true;
											}
											
											// Notification
											if(aAtomEntry->DirectReply() && iSettingNotifyReplyTo) {
												// Direct replay
												aNotifiedAudioId = ESettingItemDirectReplyTone;
											}
											else if(aNotifiedAudioId != ESettingItemDirectReplyTone && !aAtomEntry->Read() && aDiscussion->Notify() && 
													((aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
															(iSettingNotifyChannelsModerating == 1 || 
															(iSettingNotifyChannelsModerating == 0 && aDiscussion->GetUnreadEntries() == 1))) || 
													((aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationModerator && 
															(iSettingNotifyChannelsFollowing == 1 || 
															(iSettingNotifyChannelsFollowing == 0 && aDiscussion->GetUnreadEntries() == 1)))))) {
												
												// Channel post
												aNotifiedAudioId = ESettingItemChannelPostTone;
											}
										}
										
										CleanupStack::PopAndDestroy(); // aAtomEntryParser
										
										// Set last node id received
										SetLastNodeIdReceived(aItemId);
									}
									else if(aElement.Compare(_L8("retract")) == 0) {
										// Channel post deleted
										aDiscussion->DeleteEntryById(aItemId);
									}
								}
							}
						}
						else if(aAttributeNode.Find(_L8("/channel")) != KErrNotFound) {
							// Handle other event types for channels
							CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
							CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
							TInt aResourceNotificationId = KErrNotFound;
							
							// User node changes
							if(aChannelItem->GetItemType() == EItemRoster && aChannelItem->GetId().CompareF(aNodeId) != 0) {
								// Set new node id
								aChannelItem->SetIdL(aNodeId);
								aChannelItem->SetUnreadData(aDiscussion);
								
								// Set discussion id
								aDiscussion->SetDiscussionIdL(aNodeId);								
								aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());
								
								// Collect node items
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/mood"));
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/current"));
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/previous"));
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/future"));
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/channel"), _L8("1"));
								
								iSettingsSaveNeeded = true;
							}
							
							if(aPubsubEventType == EPubsubEventAffiliation) {
								// Handle affiliation event
								TPtrC aAttributeJid(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
								
								if(iOwnItem->GetId().CompareF(aAttributeJid) == 0) {
									// Own affiliation to channel change
									TXmppPubsubAffiliation aAffiliation = CXmppEnumerationConverter::PubsubAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation")));
									
									if(aAffiliation != aChannelItem->GetPubsubAffiliation()) {
										if(aAffiliation == EPubsubAffiliationOutcast) {
											aResourceNotificationId = R_LOCALIZED_STRING_NOTE_AFFILIATIONOUTCAST;
										}
										else if(aAffiliation == EPubsubAffiliationMember) {
											aResourceNotificationId = R_LOCALIZED_STRING_NOTE_AFFILIATIONMEMBER;
										}
										else if(aAffiliation == EPubsubAffiliationPublisher) {
											aResourceNotificationId = R_LOCALIZED_STRING_NOTE_AFFILIATIONPUBLISHER;
										}
										else if(aAffiliation == EPubsubAffiliationModerator) {
											aResourceNotificationId = R_LOCALIZED_STRING_NOTE_AFFILIATIONMODERATOR;
										}
									}
									
									aChannelItem->SetPubsubAffiliation(aAffiliation);
									
									// Set node as subcribed
									if(aAffiliation > EPubsubAffiliationNone) {
										aChannelItem->SetPubsubSubscription(EPubsubSubscriptionSubscribed);
									}
									// Set subscription if included
									TPtrC8 aAttributeSubscription(aXmlParser->GetStringAttribute(_L8("subscription")));
									
									if(aAttributeSubscription.Length() > 0) {
										aChannelItem->SetPubsubSubscription(CXmppEnumerationConverter::PubsubSubscription(aAttributeSubscription));
									}			
								}
							}
							else if(aPubsubEventType == EPubsubEventSubscription) {
								// Handle subscription event
								TPtrC aSubscriptionJid(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
								
								if(iOwnItem->GetId().CompareF(aSubscriptionJid) == 0) {
									// Own subscription to channel change
									TXmppPubsubSubscription aSubscription = CXmppEnumerationConverter::PubsubSubscription(aXmlParser->GetStringAttribute(_L8("subscription")));
									
									if(aSubscription == EPubsubSubscriptionPending) {	
										// Pending access to channel
										aResourceNotificationId = R_LOCALIZED_STRING_NOTE_SUBSCRIPTIONPENDING;
									}
									else if(aSubscription == EPubsubSubscriptionSubscribed) {	
										// Now subscribed to channel
										if(aChannelItem->GetPubsubSubscription() < EPubsubSubscriptionSubscribed) {
											// Collect node items
											CollectLastPubsubNodeItemsL(aChannelItem->GetId(), _L8("1"));
										}
										
										if(aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionPending) {
											// Accepted to channel
											aResourceNotificationId = R_LOCALIZED_STRING_NOTE_SUBSCRIPTIONACCEPTED;
										}
									}
									else if((aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionPending || 
												aChannelItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed) && 
											aSubscription == EPubsubSubscriptionNone) {
										
										// Rejected from channel
										aResourceNotificationId = R_LOCALIZED_STRING_NOTE_SUBSCRIPTIONREJECTED;
									}
									
									aChannelItem->SetPubsubSubscription(aSubscription);
									
									// Set affiliation if included
									TPtrC8 aAttributeAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation")));
									
									if(aAttributeAffiliation.Length() > 0) {
										aChannelItem->SetPubsubAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aAttributeAffiliation));
									}
								}
							}
							else if(aPubsubEventType == EPubsubEventPurge) {
								// Handle purge event
								aDiscussion->DeleteAllEntries();
							}
							else if(aPubsubEventType == EPubsubEventDelete) {
								// Handle delete event
								if(aXmlParser->MoveToElement(_L8("redirect"))) {
									// Pubsub redirects to a new node
									TPtrC aNewNodeId(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("node"))));
									
									aDiscussion->SetDiscussionIdL(aNewNodeId);
									aChannelItem->SetIdL(aNewNodeId);
									
									if(aChannelItem->GetItemType() == EItemChannel) {
										HBufC* aSubTitle = aNewNodeId.AllocLC();
										TPtr pSubTitle(aSubTitle->Des());
										pSubTitle.Replace(0, pSubTitle.LocateReverse('/') + 1, _L("#"));
										
										aChannelItem->SetSubTitleL(pSubTitle);
										CleanupStack::PopAndDestroy(); // aSubTitle
									}
									
									NotifyNotificationObservers(ENotificationFollowingItemsReconfigured, aChannelItem->GetItemId());
								}
								else {
									// Pubsub deletes node
									iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());
									
									NotifyNotificationObservers(ENotificationFollowingItemDeleted, aChannelItem->GetItemId());			
									
									if(aChannelItem->GetItemType() == EItemRoster) {
										// Clear user channel id
										aChannelItem->SetIdL(KNullDesC);
										aChannelItem->SetUnreadData(NULL);
									
										// Reset status & geoloc data
										aRosterItem->SetDescriptionL(KNullDesC);
										aRosterItem->SetGeolocItemL(EGeolocItemPrevious, CGeolocData::NewL());
										aRosterItem->SetGeolocItemL(EGeolocItemCurrent, CGeolocData::NewL());
										aRosterItem->SetGeolocItemL(EGeolocItemFuture, CGeolocData::NewL());
									}
									else if(aChannelItem->GetItemType() == EItemChannel) {
										// Delete channel
										iFollowingList->DeleteItemById(aChannelItem->GetItemId());
										
										aNotifiedItemId = KErrNotFound;
									}
								}
								
								iSettingsSaveNeeded = true;
							}
							
							// Add notification to channel
							if(aResourceNotificationId != KErrNotFound) {
								HBufC* aResourceText = CCoeEnv::Static()->AllocReadResourceLC(aResourceNotificationId);
								
								CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
								aAtomEntry->SetPublishTime(TimeStamp());
								aAtomEntry->SetContentL(*aResourceText, EEntryContentAction);			
								aAtomEntry->SetIconId(KIconChannel);	
								aAtomEntry->SetAuthorAffiliation(EPubsubAffiliationOwner);
								aAtomEntry->SetDirectReply(true);
								aAtomEntry->SetPrivate(true);
								
								// Add to discussion
								if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {
									if(iFollowingList->GetIndexById(aNotifiedItemId) > 0) {
										aBubbleEvent = true;
									}
									
									aNotifiedAudioId = ESettingItemDirectReplyTone;
								}
								
								CleanupStack::PopAndDestroy(); // aResourceText
							}
						}
						
						if(aBubbleEvent) {
							aItem->SetLastUpdated(TimeStamp());
							
							iFollowingList->RemoveItemById(aItem->GetItemId());
							iFollowingList->InsertItem(GetBubbleToPosition(aItem), aItem);
						}
						
						break;
					}
				}
			}
			
			CleanupStack::PopAndDestroy(); // aUserJid
		}
		 	
		CleanupStack::PopAndDestroy(); // aNodeParser	
	}
	
	CleanupStack::PopAndDestroy(); // aItemsXmlParser
	
	// Notifications
	if(aNotifiedItemId != KErrNotFound || aNotifiedAudioId != KErrNotFound) {
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aNotifiedItemId);
		
		if(aNotifiedAudioId != KErrNotFound && !CPhoneUtilities::InCall()) {
			NotifyNotificationObservers(ENotificationNotifiedMessageEvent, aNotifiedItemId);

			iNotificationEngine->NotifyL(aNotifiedAudioId);
		}
	}
}

void CBuddycloudLogic::HandlePubsubRequestL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePubsubRequestL"));
#endif
	
	TPtrC8 aNode;	

	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);	
	CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
	
	while(aXmlParser->MoveToElement(_L8("field"))) {
		TPtrC8 aAttributeVar(aXmlParser->GetStringAttribute(_L8("var")));
		
		if(aXmlParser->MoveToElement(_L8("value"))) {
			TPtrC8 aDataValue(aXmlParser->GetStringData());							
			
			if(aAttributeVar.Compare(_L8("FORM_TYPE")) == 0) {
				TInt aTitleResourceId = R_LOCALIZED_STRING_NOTE_AFFILIATIONREQUEST_TITLE;
				TInt aTextResourceId = R_LOCALIZED_STRING_NOTE_AFFILIATIONREQUEST_TEXT;
				
				if(aDataValue.Compare(_L8("http://jabber.org/protocol/pubsub#subscribe_authorization")) == 0) {
					aTitleResourceId = R_LOCALIZED_STRING_FOLLOWINGREQUEST_TITLE;
					aTextResourceId = R_LOCALIZED_STRING_NOTE_SUBSCRIPTIONREQUEST_TEXT;
				}
				
				// Set title & form type
				HBufC* aTitle = CCoeEnv::Static()->AllocReadResourceLC(aTitleResourceId);
				aAtomEntry->SetAuthorNameL(*aTitle);
				aAtomEntry->SetAuthorJidL(iTextUtilities->Utf8ToUnicodeL(aDataValue));
				
				// Set text
				HBufC* aText = CCoeEnv::Static()->AllocReadResourceLC(aTextResourceId);
				aAtomEntry->SetContentL(*aText, EEntryContentNotice);				
				CleanupStack::PopAndDestroy(2); // aText, aTitle
			}
			else if(aAttributeVar.Compare(_L8("pubsub#node")) == 0) {
				aNode.Set(aXmlParser->GetStringData());
			}
			else if(aAttributeVar.Compare(_L8("pubsub#subscriber_jid")) == 0) {
				aAtomEntry->SetIdL(aXmlParser->GetStringData());
			}
			else if(aAttributeVar.Compare(_L8("description")) == 0) {
				TPtrC aEncData(iTextUtilities->Utf8ToUnicodeL(aDataValue));
				
				if(aEncData.Length() > 0) {
					HBufC* aContent = HBufC::NewLC(aAtomEntry->GetContent().Length() + 2 + aEncData.Length());
					TPtr pContent(aContent->Des());
					pContent.Append(aAtomEntry->GetContent());
					pContent.Append(_L("\n\n"));
					pContent.Append(aEncData);
					
					aAtomEntry->SetContentL(pContent, EEntryContentNotice);			
					CleanupStack::PopAndDestroy(); // aContent
				}
			}
		}
	}
	
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(iTextUtilities->Utf8ToUnicodeL(aNode));
	CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iFollowingList->GetItemById(aDiscussion->GetItemId()));
	
	if(aChannelItem && aChannelItem->GetItemType() >= EItemRoster) {		
		// Set content
		TPtrC aEncJid = iTextUtilities->Utf8ToUnicodeL(aAtomEntry->GetId());
		HBufC* aContent = HBufC::NewLC(aAtomEntry->GetContent().Length() + aEncJid.Length());
		TPtr pContent(aContent->Des());
		pContent.Append(aAtomEntry->GetContent());
		
		TInt aLocate = pContent.Find(_L("$USER"));
		
		if(aLocate != KErrNotFound) {
			pContent.Replace(aLocate, 5, aEncJid);			
			aAtomEntry->SetContentL(pContent, EEntryContentNotice);
		}
		
		CleanupStack::PopAndDestroy(); // aContent
		
		aAtomEntry->SetPrivate(true);
		aAtomEntry->SetIconId(KIconChannel);
		aAtomEntry->SetPublishTime(TimeStamp());
		aAtomEntry->SetAuthorAffiliation(aChannelItem->GetPubsubAffiliation());

		// Add to discussion
		if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {
			aChannelItem->SetLastUpdated(TimeStamp());
			
			if(iFollowingList->GetIndexById(aChannelItem->GetItemId()) > 0) {
				// Bubble			
				iFollowingList->RemoveItemById(aChannelItem->GetItemId());
				iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
			}
			
			NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aChannelItem->GetItemId());
			NotifyNotificationObservers(ENotificationNotifiedMessageEvent, aChannelItem->GetItemId());
			
			iNotificationEngine->NotifyL(ESettingItemChannelPostTone);
		}		
	}
	else {
		CleanupStack::PopAndDestroy(); // aAtomEntry		
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::AcknowledgePubsubEventL(const TDesC8& aId) {
	_LIT8(KPubsubAcknowledgeStanza, "<iq to='' type='result' id=''/>\r\n");	
	HBufC8* aPubsubAcknowledgeStanza = HBufC8::NewLC(KPubsubAcknowledgeStanza().Length() + KBuddycloudPubsubServer().Length() + aId.Length());
	TPtr8 pPubsubAcknowledgeStanza(aPubsubAcknowledgeStanza->Des());
	pPubsubAcknowledgeStanza.Append(KPubsubAcknowledgeStanza);
	pPubsubAcknowledgeStanza.Insert(28, aId);
	pPubsubAcknowledgeStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pPubsubAcknowledgeStanza, true);
	CleanupStack::PopAndDestroy(); // aPubsubAcknowledgeStanza	
}

void CBuddycloudLogic::FlagTagNodeL(const TDesC8& aType, const TDesC& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FlagTagNodeL"));
#endif
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	
	_LIT8(KFlagTagNodeStanza, "<iq to='tagger.buddycloud.com' type='set' id='tagger:%02d'><flagtag xmlns='http://buddycloud.com/feedback'><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE' type='hidden'><value>buddycloud::channel</value></field><field var='flag#channel'><value></value></field></x></flagtag></iq>\r\n");	
	HBufC8* aFlagTagNodeStanza = HBufC8::NewLC(KFlagTagNodeStanza().Length() + aType.Length() + aEncNode.Length());
	TPtr8 pFlagTagNodeStanza(aFlagTagNodeStanza->Des());
	pFlagTagNodeStanza.AppendFormat(KFlagTagNodeStanza, GetNewIdStamp());
	pFlagTagNodeStanza.Insert(256, aEncNode);
	pFlagTagNodeStanza.Insert(199, aType);

	iXmppEngine->SendAndForgetXmppStanza(pFlagTagNodeStanza, true);
	CleanupStack::PopAndDestroy(); // aFlagTagNodeStanza
	
	// Feedback set dialog
	ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_FEEDBACKSENT);
}

void CBuddycloudLogic::FlagTagNodeItemL(const TDesC8& aType, const TDesC& aNode, const TDesC8& aNodeItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FlagTagNodeItemL"));
#endif
	
	TPtrC8 aEncNode(iTextUtilities->UnicodeToUtf8L(aNode));			
	
	_LIT8(KFlagTagPostStanza, "<iq to='tagger.buddycloud.com' type='set' id='tagger:%02d'><flagtag xmlns='http://buddycloud.com/feedback'><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE' type='hidden'><value>buddycloud::post</value></field><field var='flag#channel'><value></value></field><field var='flag#itemid'><value></value></field></x></flagtag></iq>\r\n");	
	HBufC8* aFlagTagPostStanza = HBufC8::NewLC(KFlagTagPostStanza().Length() + aType.Length() + aEncNode.Length() + aNodeItemId.Length());
	TPtr8 pFlagTagPostStanza(aFlagTagPostStanza->Des());
	pFlagTagPostStanza.AppendFormat(KFlagTagPostStanza, GetNewIdStamp());
	pFlagTagPostStanza.Insert(301, aNodeItemId);
	pFlagTagPostStanza.Insert(253, aEncNode);
	pFlagTagPostStanza.Insert(199, aType);

	iXmppEngine->SendAndForgetXmppStanza(pFlagTagPostStanza, true);
	CleanupStack::PopAndDestroy(); // aFlagTagPostStanza
	
	// Feedback set dialog
	ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_FEEDBACKSENT);
}

TInt CBuddycloudLogic::FollowChannelL(const TDesC& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowChannelL"));
#endif
	
	_LIT(KRootNode, "/channel");
	HBufC* aNodeId = HBufC::NewLC(aNode.Length() + KRootNode().Length() + 1);
	TPtr pNodeId(aNodeId->Des());
	pNodeId.Append(aNode);
	pNodeId.LowerCase();
	
	if(pNodeId.Find(KRootNode) != 0) {
		// Add node root if not found
		pNodeId.Insert(0, _L("/"));		
		pNodeId.Insert(0, KRootNode);		
	}	
	
	// Set channel data
	CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
	aChannelItem->SetItemId(iNextItemId++);
	aChannelItem->SetIdL(pNodeId);
	aChannelItem->SetTitleL(pNodeId.Mid(pNodeId.LocateReverse('/') + 1));
	
	// Set discussion
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
	aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());		
	aChannelItem->SetUnreadData(aDiscussion);
	
	iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
	CleanupStack::Pop(); // aChannelItem
	CleanupStack::PopAndDestroy(); // aNodeId
	
	// Get metatdata
	CollectChannelMetadataL(aChannelItem->GetId());
	
	// Send subscribe stanza
	HBufC8* aEncNode = iTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()).AllocLC();
	TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()));
	
	_LIT8(KSubscribeStanza, "<iq to='' type='set' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><subscribe node='' jid=''/></pubsub></iq>\r\n");
	HBufC8* aSubscribeStanza = HBufC8::NewLC(KSubscribeStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode->Des().Length() + aEncId.Length());
	TPtr8 pSubscribeStanza(aSubscribeStanza->Des());
	pSubscribeStanza.AppendFormat(KSubscribeStanza, EXmppIdAddChannelSubscription, GetNewIdStamp());
	pSubscribeStanza.Insert(106, aEncId);
	pSubscribeStanza.Insert(99, *aEncNode);
	pSubscribeStanza.Insert(8, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndForgetXmppStanza(pSubscribeStanza, true);
	CleanupStack::PopAndDestroy(2); // aSubscribeStanza, aEncNode
	
	return aChannelItem->GetItemId();
}

void CBuddycloudLogic::UnfollowChannelL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnfollowChannelL"));
#endif
	
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));

	if(aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		aChannelItem->SetUnreadData(NULL);

		if(aChannelItem->GetId().Length() > 0) {
			HBufC8* aEncNode = iTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()).AllocLC();
			
			if(aChannelItem->GetItemType() >= EItemChannel && aChannelItem->GetPubsubAffiliation() == EPubsubAffiliationOwner) {
				// Channel owner deletes node
				_LIT8(KDeleteStanza, "<iq to='' type='set' id='deletenode:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><delete node=''/></pubsub></iq>\r\n");
				HBufC8* aDeleteStanza = HBufC8::NewLC(KDeleteStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode->Des().Length());
				TPtr8 pDeleteStanza(aDeleteStanza->Des());
				pDeleteStanza.AppendFormat(KDeleteStanza, GetNewIdStamp());
				pDeleteStanza.Insert(110, *aEncNode);
				pDeleteStanza.Insert(8, KBuddycloudPubsubServer);
				
				iXmppEngine->SendAndForgetXmppStanza(pDeleteStanza, true);
				CleanupStack::PopAndDestroy(); // aDeleteStanza
			}
			else {
				// Unsubscribe from channel node
				TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()));

				_LIT8(KUnsubscribeStanza, "<iq to='' type='set' id='unsubscribe:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><unsubscribe node='' jid=''/></pubsub></iq>\r\n");
				HBufC8* aUnsubscribeStanza = HBufC8::NewLC(KUnsubscribeStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode->Des().Length() + aEncId.Length());
				TPtr8 pUnsubscribeStanza(aUnsubscribeStanza->Des());
				pUnsubscribeStanza.AppendFormat(KUnsubscribeStanza, GetNewIdStamp());
				pUnsubscribeStanza.Insert(117, aEncId);
				pUnsubscribeStanza.Insert(110, *aEncNode);
				pUnsubscribeStanza.Insert(8, KBuddycloudPubsubServer);
				
				iXmppEngine->SendAndForgetXmppStanza(pUnsubscribeStanza, true);
				CleanupStack::PopAndDestroy(); // aUnsubscribeStanza
			}
			
			CleanupStack::PopAndDestroy(); // aEncNode
			
			// Delete discussion
			iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());
		}
		
		if(aItem->GetItemType() == EItemChannel) {
			NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);		
			
			iFollowingList->DeleteItemById(aItemId);
					
			iSettingsSaveNeeded = true;
		}
	}
}

TInt CBuddycloudLogic::CreateChannelL(const TDesC& aNodeId, const TDesC& aTitle, const TDesC& aDescription, TXmppPubsubAccessModel aAccessModel) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CreateChannelL"));
#endif

	CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
	aChannelItem->SetTitleL(aTitle);
	aChannelItem->SetDescriptionL(aDescription);
	aChannelItem->SetItemId(iNextItemId++);
	aChannelItem->SetPubsubAffiliation(EPubsubAffiliationOwner);
	aChannelItem->SetPubsubSubscription(EPubsubSubscriptionSubscribed);
	
	// Set sub title
	HBufC* aSubTitle = HBufC::NewLC(aNodeId.Length() + 1);
	TPtr pSubTitle(aSubTitle->Des());
	pSubTitle.Append('#');
	pSubTitle.Append(aNodeId);
		
	aChannelItem->SetSubTitleL(pSubTitle);
	CleanupStack::PopAndDestroy(); // aSubTitle
		
	// Create correct node id
	_LIT(KRootNode, "/channel/");	
	HBufC* aFullNodeId = HBufC::NewLC(KRootNode().Length() + aNodeId.Length());
	TPtr pFullNodeId(aFullNodeId->Des());
	pFullNodeId.Append(KRootNode);
	pFullNodeId.Append(aNodeId);
		
	aChannelItem->SetIdL(pFullNodeId);
	CleanupStack::PopAndDestroy(); // aFullNodeId
	
	// Add to discussion
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
	aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());
	
	aChannelItem->SetUnreadData(aDiscussion);
	
	iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
	CleanupStack::Pop(); // aChannelItem
		
	// Get data
	HBufC8* aEncTitle = iTextUtilities->UnicodeToUtf8L(aTitle).AllocLC();
	HBufC8* aEncDescription = iTextUtilities->UnicodeToUtf8L(aDescription).AllocLC();
	TPtrC8 aEncId(iTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()));
	TPtrC8 aEncAccess = CXmppEnumerationConverter::PubsubAccessModel(aAccessModel);
	
	// Send stanza
	_LIT8(KCreateStanza, "<iq to='' type='set' id='%02d:%02d:%d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><create node=''/><configure><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE' type='hidden'><value>http://jabber.org/protocol/pubsub#node_config</value></field><field var='pubsub#access_model'><value></value></field><field var='pubsub#title'><value></value></field><field var='pubsub#description'><value></value></field></x></configure></pubsub></iq>\r\n");
	HBufC8* aCreateStanza = HBufC8::NewLC(KCreateStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length() + aEncAccess.Length() + aEncTitle->Des().Length() + aEncDescription->Des().Length() + 8);
	TPtr8 pCreateStanza(aCreateStanza->Des());
	pCreateStanza.AppendFormat(KCreateStanza, EXmppIdCreateChannel, GetNewIdStamp(), aChannelItem->GetItemId());
	pCreateStanza.Insert(pCreateStanza.Length() - 350, aEncId);
	pCreateStanza.Insert(pCreateStanza.Length() - 152, aEncAccess);
	pCreateStanza.Insert(pCreateStanza.Length() - 103, *aEncTitle);
	pCreateStanza.Insert(pCreateStanza.Length() - 48, *aEncDescription);
	pCreateStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pCreateStanza, this, true);
	CleanupStack::PopAndDestroy(3); // aCreateStanza, aEncDescription, aEncTitle
	
	return aChannelItem->GetItemId();
}

void CBuddycloudLogic::SetMoodL(TDesC& aMood) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetMoodL"));
#endif

	if(iOwnItem) {
		HBufC8* aEncJid = iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()).AllocLC();
		TPtrC8 pEncJid(aEncJid->Des());
		
		TPtrC8 aEncodedMood(iTextUtilities->UnicodeToUtf8L(aMood));
	
		_LIT8(KMoodStanza, "<iq to='' type='set' id='%02d:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node='/user//mood'><item><mood xmlns='http://jabber.org/protocol/mood'><undefined/><text></text></mood></item></publish></pubsub></iq>\r\n");
		HBufC8* aMoodStanza = HBufC8::NewLC(KMoodStanza().Length() + KBuddycloudPubsubServer().Length() + pEncJid.Length() + aEncodedMood.Length());
		TPtr8 pMoodStanza(aMoodStanza->Des());
		pMoodStanza.AppendFormat(KMoodStanza, EXmppIdPublishMood, GetNewIdStamp());
		pMoodStanza.Insert(180, aEncodedMood);
		pMoodStanza.Insert(103, pEncJid);
		pMoodStanza.Insert(8, KBuddycloudPubsubServer);
	
		iXmppEngine->SendAndAcknowledgeXmppStanza(pMoodStanza, this, true);
		CleanupStack::PopAndDestroy(2); // aStanza, aEncJid
		
		// Set Activity Status
		if(iState == ELogicOnline) {
			SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
		}
	}
}

void CBuddycloudLogic::SetCurrentPlaceL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetCurrentPlaceL"));
#endif		
	RPointerArray<HBufC> aDynItems;
	RArray<TInt> aDynItemIds;
	
	// Location update
	iLocationEngine->TriggerEngine();
		
	// New place
	HBufC* aNewPlace = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_ITEM_NEWPLACE);
	TPtr pNewPlace(aNewPlace->Des());
	
	HBufC* aItem = HBufC::NewLC(4 + pNewPlace.Length());
	TPtr pItem(aItem->Des());
	pItem.Append(_L("4\t\t\t"));
	pItem.Insert(2, pNewPlace);
	
	aDynItems.Append(aItem);
	CleanupStack::Pop(); // aItem
	CleanupStack::PopAndDestroy(); // aNewPlace
	
	aDynItemIds.Append(0);
	
	if(iLocationEngine->GetLastPlaceId() > 0) {
		CBuddycloudExtendedPlace* aPlace =  static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(iLocationEngine->GetLastPlaceId()));
		
		if(aPlace) {
			HBufC* aItem = HBufC::NewLC(4 + aPlace->GetGeoloc()->GetString(EGeolocText).Length());
			TPtr pItem(aItem->Des());
			pItem.Append(_L("4\t\t\t"));
			pItem.Insert(2, aPlace->GetGeoloc()->GetString(EGeolocText));
			
			aDynItems.Append(aItem);
			aDynItemIds.Append(aPlace->GetItemId());
			CleanupStack::Pop(); // aItem
		}
	}
		
	// Search existing groups & contacts
	for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
		if(iNearbyPlaces[i]->GetPlaceId() != iLocationEngine->GetLastPlaceId()) {
			HBufC* aItem = HBufC::NewLC(4 + iNearbyPlaces[i]->GetPlaceName().Length());
			TPtr pItem(aItem->Des());
			pItem.Append(_L("4\t\t\t"));
			pItem.Insert(2, iNearbyPlaces[i]->GetPlaceName());
			
			aDynItems.Append(aItem);
			aDynItemIds.Append(iNearbyPlaces[i]->GetPlaceId());
			CleanupStack::Pop(); // aItem
		}
	}
	
	// Selection result
	TInt aSelectedItem = DisplaySingleLinePopupMenuL(aDynItems);
	
	if(aSelectedItem != KErrNotFound) {
		TInt aSelectedItemId = aDynItemIds[aSelectedItem];
		
		if(aSelectedItemId == 0) {
			// New place
			CBuddycloudExtendedPlace* aNewPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(KEditingNewPlaceId));
			
			if(aNewPlace == NULL) {
				aNewPlace = CBuddycloudExtendedPlace::NewLC();
				aNewPlace->SetItemId(KEditingNewPlaceId);
				aNewPlace->SetShared(false);
				
				iPlaceList->AddItem(aNewPlace);
				CleanupStack::Pop(); // aNewPlace
			}
			
			iPlaceList->SetEditedPlace(KEditingNewPlaceId);
						
			if(iOwnItem) {
				CGeolocData* aGeoloc = iOwnItem->GetGeolocItem(EGeolocItemCurrent);
				
				aNewPlace->GetGeoloc()->SetStringL(EGeolocStreet, aGeoloc->GetString(EGeolocStreet));
				aNewPlace->GetGeoloc()->SetStringL(EGeolocArea, aGeoloc->GetString(EGeolocArea));
				aNewPlace->GetGeoloc()->SetStringL(EGeolocLocality, aGeoloc->GetString(EGeolocLocality));
				aNewPlace->GetGeoloc()->SetStringL(EGeolocPostalcode, aGeoloc->GetString(EGeolocPostalcode));
				aNewPlace->GetGeoloc()->SetStringL(EGeolocRegion, aGeoloc->GetString(EGeolocRegion));
				aNewPlace->GetGeoloc()->SetStringL(EGeolocCountry, aGeoloc->GetString(EGeolocCountry));
			}
			
			NotifyNotificationObservers(ENotificationEditPlaceRequested, aNewPlace->GetItemId());
		}
		else if(aSelectedItemId > 0) {
			SetCurrentPlaceL(aSelectedItemId);
		}	
	}
		
	while(aDynItems.Count() > 0) {
		delete aDynItems[0];
		aDynItems.Remove(0);
	}
	
	aDynItems.Close();
	aDynItemIds.Close();
}

void CBuddycloudLogic::SetCurrentPlaceL(TInt aPlaceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetCurrentPlaceL"));
#endif

	// Set as current place
	if(aPlaceId > 0) {
		// Add to myplaces
		EditMyPlacesL(aPlaceId, true);
		
		// Send query
		SendPlaceQueryL(aPlaceId, EXmppIdSetCurrentPlace, true);
		
		// Set Activity Status
		if(iState == ELogicOnline) {
			SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
		}
	}
}

void CBuddycloudLogic::SetNextPlaceL(TDesC& aPlace, TInt aPlaceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetNextPlaceL"));
#endif

	// Place id 
	_LIT8(KPlaceIdElement, "<id>http://buddycloud.com/places/%d</id>");
	HBufC8* aPlaceIdElement = HBufC8::NewLC(KPlaceIdElement().Length() + 32);
	TPtr8 pPlaceIdElement(aPlaceIdElement->Des());
	
	if(aPlaceId != KErrNotFound) {
		pPlaceIdElement.Format(KPlaceIdElement, aPlaceId);
		
		// Add to 'my places'
		EditMyPlacesL(aPlaceId, true);
	}

	// Label
	TPtrC8 aEncodedPlace(iTextUtilities->UnicodeToUtf8L(aPlace));
	
	_LIT8(KNextStanza, "<iq to='butler.buddycloud.com' type='set' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/place#next'><place><name></name></place></query></iq>\r\n");
	HBufC8* aNextStanza = HBufC8::NewLC(KNextStanza().Length() + aEncodedPlace.Length() + pPlaceIdElement.Length());
	TPtr8 pNextStanza(aNextStanza->Des());
	pNextStanza.AppendFormat(KNextStanza, EXmppIdPublishFuturePlace, GetNewIdStamp());
	pNextStanza.Insert(130, pPlaceIdElement);
	pNextStanza.Insert(123, aEncodedPlace);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pNextStanza, this, true);
	CleanupStack::PopAndDestroy(2); // aNextStanza, aPlaceIdElement
	
	// Set Activity Status
	if(iState == ELogicOnline) {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
	}
}

TInt CBuddycloudLogic::GetBubbleToPosition(CFollowingItem* aBubblingItem) {
	//  BUBBLE LOGIC
	// ---------------------------------------------------------------------
	// OWN           <- Own Item always on top
	// NOTICE        <- Notices bubble to top (below Own Item)
	// NOTICE
	// ROSTER     (1)<- PM's have message priority
	// CHANNEL    (1)<- Unread Channel/Roster Item bubbles below Notices
	// ROSTER        <- Other Items bubble below unread Items
	// CHANNEL
	// FEED
	// CONTACT
	// ---------------------------------------------------------------------

	if(aBubblingItem->GetItemType() == EItemNotice) {
		return 1;
	}
	else if(aBubblingItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aBubblingChannelItem = static_cast <CFollowingChannelItem*> (aBubblingItem);		
		TInt aBubblingChannelUnread = aBubblingChannelItem->GetUnread();
		TInt aBubblingChannelReplies = aBubblingChannelItem->GetReplies();
		TInt aBubblingPrivateUnread = 0;
		
		if(aBubblingItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aBubblingRosterItem = static_cast <CFollowingRosterItem*> (aBubblingItem);			
			aBubblingPrivateUnread = aBubblingRosterItem->GetUnread();				
		}

		// Loop through other items
		for(TInt i = 1; i < iFollowingList->Count(); i++) {
			CFollowingItem* aListItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

			if(aListItem->GetItemType() >= EItemRoster) {
				CFollowingChannelItem* aListChannelItem = static_cast <CFollowingChannelItem*> (aListItem);
				TInt aListChannelUnread = aListChannelItem->GetUnread();
				TInt aListChannelReplies = aListChannelItem->GetReplies();
				TInt aListPrivateUnread = 0;
				
				if(aListItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aListRosterItem = static_cast <CFollowingRosterItem*> (aListItem);			
					aListPrivateUnread = aListRosterItem->GetUnread();
				}
				
				// Bubble on activity (not channel post sorted)
				if(aBubblingPrivateUnread > aListPrivateUnread || 
						(aBubblingPrivateUnread >= aListPrivateUnread && aBubblingChannelReplies > aListChannelReplies) ||
						(aBubblingPrivateUnread == aListPrivateUnread && aBubblingChannelReplies == aListChannelReplies && 
								(aBubblingChannelUnread > 0 || aBubblingChannelUnread == aListChannelUnread))) {
					
					return i;
				}
			}
		}
	}
	
	return iFollowingList->Count();
}

void CBuddycloudLogic::DiscussionRead(TDesC& /*aDiscussionId*/, TInt aItemId) {
	CFollowingItem* aFollowingItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
	
	if(iOwnItem && aFollowingItem) {
		if(iOwnItem->GetItemId() != aFollowingItem->GetItemId()) {
			iFollowingList->RemoveItemById(aFollowingItem->GetItemId());
			iFollowingList->InsertItem(GetBubbleToPosition(aFollowingItem), aFollowingItem);
			
			iSettingsSaveNeeded = true;
			NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
		}
	}
}

#ifdef _DEBUG
void CBuddycloudLogic::DiscussionDebug(const TDesC8& aMessage) {
	WriteToLog(aMessage);
}
#endif

/*
----------------------------------------------------------------------------
--
-- Reading/Writing state to file
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::LoadSettingsAndItems() {
	TRAPD(aErr, LoadSettingsAndItemsL());

	if(aErr != KErrNone) {
		TBuf8<256> aLog;
		aLog.Format(_L8("LOGIC CBuddycloudLogic::LoadSettingsAndItems Trap: %d"), aErr);
		WriteToLog(aLog);
	}
}

void CBuddycloudLogic::LoadSettingsAndItemsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::LoadSettingsAndItemsL"));
#endif

	RFs aSession = CCoeEnv::Static()->FsSession();	
	
	RFile aFile;
	TInt aFileSize;

	TFileName aFilePath(_L("LogicStateV2.xml"));
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);

	if(aFile.Open(aSession, aFilePath, EFileStreamText|EFileRead) == KErrNone) {
		CleanupClosePushL(aFile);
		aFile.Size(aFileSize);

		// Creat buffer & read file
		HBufC8* aBuf = HBufC8::NewLC(aFileSize);
		TPtr8 pBuf(aBuf->Des());
		aFile.Read(pBuf);
		aFile.Close();

		WriteToLog(_L8("BL    Internalize Logic State -- Start"));

		CXmlParser* aXmlParser = CXmlParser::NewLC(*aBuf);
		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();

		if(aXmlParser->MoveToElement(_L8("logicstate"))) {
			// App version
			HBufC* aAppVersion = CCoeEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
			TPtrC aEncVersion(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("version"))));
			
			if(aEncVersion.Compare(*aAppVersion) != 0) {
				iSettingNewInstall = true;
			}
			
			WriteToLog(iTextUtilities->UnicodeToUtf8L(*aAppVersion));
			CleanupStack::PopAndDestroy(); // aAppVersion
			
			// Last node received
			SetLastNodeIdReceived(aXmlParser->GetStringAttribute(_L8("lastnodeid")));

			if(aXmlParser->MoveToElement(_L8("settings"))) {
				iSettingConnectionMode = aXmlParser->GetIntAttribute(_L8("apmode"));
				iSettingConnectionModeId = aXmlParser->GetIntAttribute(_L8("apid"));
				
				HBufC* aApName = iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("apname"))).AllocLC();
				
				if(aXmlParser->MoveToElement(_L8("account"))) {
					iSettingFullName.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("fullname"))));	
					iSettingUsername.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("username"))));
					iSettingPassword.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("password"))));										
					iSettingXmppServer.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("server"))));										
					
					if(iSettingUsername.Length() > 0 && iSettingUsername.Locate('@') == KErrNotFound) {
						iSettingUsername.Append(_L("@buddycloud.com"));
						iSettingXmppServer.Copy(_L("cirrus.buddycloud.com:443"));
					}
				}
				
				if(aXmlParser->MoveToElement(_L8("preferences"))) {
					iSettingPreferredLanguage = aXmlParser->GetIntAttribute(_L8("language"));
					
					// Check language validity
					if(iSettingPreferredLanguage != -1 && 
							// Check in language range
							iSettingPreferredLanguage != ELangEnglish && 
							iSettingPreferredLanguage != ELangFrench && 
							iSettingPreferredLanguage != ELangGerman && 
							iSettingPreferredLanguage != ELangSpanish && 
							iSettingPreferredLanguage != ELangItalian && 
							iSettingPreferredLanguage != ELangFinnish && 
							iSettingPreferredLanguage != ELangPortuguese && 
							iSettingPreferredLanguage != ELangTurkish && 
							iSettingPreferredLanguage != ELangRussian && 
							iSettingPreferredLanguage != ELangHungarian && 
							iSettingPreferredLanguage != ELangDutch && 
							iSettingPreferredLanguage != ELangCzech && 
							iSettingPreferredLanguage != ELangPolish &&
							iSettingPreferredLanguage != ELangPrcChinese && 
							iSettingPreferredLanguage != ELangThai && 
							iSettingPreferredLanguage != ELangArabic && 
							iSettingPreferredLanguage != ELangEstonian && 
							iSettingPreferredLanguage != ELangGreek && 
							iSettingPreferredLanguage != ELangHebrew && 
							iSettingPreferredLanguage != ELangIndonesian && 
							iSettingPreferredLanguage != ELangKorean &&
							iSettingPreferredLanguage != ELangRomanian && 
							// Custom languages: Pirate, Boarisch, LOLspeak
							iSettingPreferredLanguage != ELangPirate && 
							iSettingPreferredLanguage != ELangBoarisch && 
							iSettingPreferredLanguage != ELangLOLspeak) {
						
						iSettingPreferredLanguage = -1;
					}
					
					iSettingAutoStart = aXmlParser->GetBoolAttribute(_L8("autostart"));
					iSettingNotifyChannelsFollowing = aXmlParser->GetIntAttribute(_L8("notifyfollowing"));
					iSettingNotifyChannelsModerating = aXmlParser->GetIntAttribute(_L8("notifymoderating"), 1);
					iSettingMessageBlocking = aXmlParser->GetBoolAttribute(_L8("messageblocking"), true);
					iSettingNotifyReplyTo = aXmlParser->GetBoolAttribute(_L8("replyto"), true);
					iSettingAccessPoint = aXmlParser->GetBoolAttribute(_L8("accesspoint"));
					
					// Reset access point
					if(iSettingAccessPoint) {
						iSettingConnectionMode = 0;
						iSettingConnectionModeId = 0;
					}					
				}
				
				iXmppEngine->SetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId, *aApName);
				CleanupStack::PopAndDestroy(); // aApName
				
				if(aXmlParser->MoveToElement(_L8("notifications"))) {	
			        iSettingPrivateMessageTone = aXmlParser->GetIntAttribute(_L8("pmt_id"));
			        iSettingChannelPostTone = aXmlParser->GetIntAttribute(_L8("cmt_id"));
			        iSettingDirectReplyTone = aXmlParser->GetIntAttribute(_L8("drt_id"));
			        iSettingPrivateMessageToneFile.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("pmt_file"))));
			        iSettingChannelPostToneFile.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("cmt_file"))));
			        iSettingDirectReplyToneFile.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("drt_file"))));
				}
				
				if(aXmlParser->MoveToElement(_L8("beacons"))) {					
					iSettingCellOn = aXmlParser->GetBoolAttribute(_L8("cell"));
					iSettingWlanOn = aXmlParser->GetBoolAttribute(_L8("wlan"));
					iSettingBtOn = aXmlParser->GetBoolAttribute(_L8("bt"));
					iSettingGpsOn = aXmlParser->GetBoolAttribute(_L8("gps"));
				}
				
				if(aXmlParser->MoveToElement(_L8("communities"))) {
					if(aXmlParser->MoveToElement(_L8("twitter"))) {
						iSettingTwitterUsername.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("username"))));
						iSettingTwitterPassword.Copy(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("password"))));
					}
				}
			}

			if(aXmlParser->MoveToElement(_L8("unsent"))) {
				TPtrC8 aDataUnsentMessages(aXmlParser->GetStringData());
				TInt aSearchResult = KErrNotFound;

				while((aSearchResult = aDataUnsentMessages.Find(_L8("\r\n"))) != KErrNotFound) {
					aSearchResult += 2;

					TPtrC8 aUnsentMessage = aDataUnsentMessages.Left(aSearchResult);
					iXmppEngine->SendAndForgetXmppStanza(aUnsentMessage, true);
					
					aDataUnsentMessages.Set(aDataUnsentMessages.Mid(aSearchResult));
				}
			}

			while(aXmlParser->MoveToElement(_L8("item"))) {
				TPtrC8 pTimeAttribute = aXmlParser->GetStringAttribute(_L8("last"));
				TInt aIconId = aXmlParser->GetIntAttribute(_L8("iconid"));
				
				CXmlParser* aItemXml = CXmlParser::NewLC(aXmlParser->GetStringData());

				if(aItemXml->MoveToElement(_L8("notice"))) {
					CFollowingItem* aNoticeItem = CFollowingItem::NewLC();
					aNoticeItem->SetItemId(iNextItemId++);
					aNoticeItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), TIdType(aItemXml->GetIntAttribute(_L8("type"))));
					aNoticeItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aNoticeItem->SetDescriptionL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					
					iFollowingList->AddItem(aNoticeItem);
					CleanupStack::Pop(); // aNoticeItem
				}
				else if(aItemXml->MoveToElement(_L8("topicchannel"))) {
					CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
					aChannelItem->SetItemId(iNextItemId++);
					aChannelItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aChannelItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("title"))));
					aChannelItem->SetSubTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("subtitle"))));
					aChannelItem->SetDescriptionL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));

					if(aIconId != 0) {
						aChannelItem->SetIconId(aIconId);
					}
					
					if(pTimeAttribute.Length() > 0) {
						aChannelItem->SetLastUpdated(aTimeUtilities->DecodeL(pTimeAttribute));
					}

					CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
					aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());		
					aDiscussion->SetUnreadData(aItemXml->GetIntAttribute(_L8("unreadentries")), aItemXml->GetIntAttribute(_L8("unreadreplies")));
					
					aChannelItem->SetUnreadData(aDiscussion);

					iFollowingList->AddItem(aChannelItem);
					CleanupStack::Pop(); // aChannelItem
				}
				else if(aItemXml->MoveToElement(_L8("roster"))) {
					CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
					aRosterItem->SetItemId(iNextItemId++);
					aRosterItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aRosterItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("title"))));
					aRosterItem->SetSubTitleL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("subtitle"))));
					aRosterItem->SetDescriptionL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					aRosterItem->SetOwnItem(aItemXml->GetBoolAttribute(_L8("own")));
					aRosterItem->SetSubscription(aRosterItem->OwnItem() ? EPresenceSubscriptionBoth : EPresenceSubscriptionUnknown);

					if(aIconId != 0) {
						aRosterItem->SetIconId(aIconId);
					}
					
					if(aRosterItem->GetTitle().Length() == 0) {
						aRosterItem->SetTitleL(aRosterItem->GetId());
						aRosterItem->SetSubTitleL(KNullDesC);
						
						TInt aLocate = aRosterItem->GetId().Locate('@');
						
						if(aLocate != KErrNotFound) {
							aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
							aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
						}
					}
					
					CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());
					aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());			
					aDiscussion->SetUnreadData(aItemXml->GetIntAttribute(_L8("unreadprivate")));
					
					aRosterItem->SetUnreadData(aDiscussion);
					
					// Channel
					if(aItemXml->MoveToElement(_L8("channel"))) {
						aRosterItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), EIdChannel);
						
						aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId(EIdChannel));
						aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());			
						aDiscussion->SetUnreadData(aItemXml->GetIntAttribute(_L8("unreadentries")), aItemXml->GetIntAttribute(_L8("unreadreplies")));
						
						aRosterItem->SetUnreadData(aDiscussion, EIdChannel);
					}

					// Location
					while(aItemXml->MoveToElement(_L8("location"))) {
						TGeolocItemType aGeolocItem = TGeolocItemType(aItemXml->GetIntAttribute(_L8("type")));
						
						if(aGeolocItem < EGeolocItemBroad) {
							CGeolocData* aGeoloc = aRosterItem->GetGeolocItem(aGeolocItem);
							aGeoloc->SetStringL(EGeolocText, iTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("text"))));
						}
					}

					if(aRosterItem->OwnItem()) {
						iOwnItem = aRosterItem;

						iFollowingList->InsertItem(0, aRosterItem);

						if(iStatusObserver) {
							iStatusObserver->JumpToItem(aRosterItem->GetItemId());
						}
					}
					else {
						iFollowingList->AddItem(aRosterItem);
					}

					CleanupStack::Pop();
				}

				CleanupStack::PopAndDestroy(); // aItemXml
			}
		}
		
		WriteToLog(_L8("BL    Internalize Logic State -- End"));

		CleanupStack::PopAndDestroy(3); // aTimeUtilities, aXmlParser, aBuf
		CleanupStack::PopAndDestroy(&aFile);
	}
	else {
		iSettingNewInstall = true;
	}
}

void CBuddycloudLogic::SaveSettingsAndItemsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SaveSettingsAndItemsL"));
#endif

	RFs aSession = CCoeEnv::Static()->FsSession();	
	RFileWriteStream aFile;
	TBuf8<256> aBuf;

	TFileName aFilePath(_L("LogicStateV2.xml"));
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);

	if(aFile.Replace(aSession, aFilePath, EFileStreamText|EFileWrite) == KErrNone) {
		CleanupClosePushL(aFile);

		WriteToLog(_L8("BL    Externalize Logic State -- Start"));

		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
		TFormattedTimeDesc aTime;

		aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>\r\n"));
		aFile.WriteL(_L8("\t<logicstate version='"));
		
		HBufC* aVersion = CCoeEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
		aFile.WriteL(iTextUtilities->UnicodeToUtf8L(*aVersion));
		CleanupStack::PopAndDestroy(); // aVersion
		
		aFile.WriteL(_L8("' lastnodeid='"));
		aFile.WriteL(*iLastNodeIdReceived);
		aFile.WriteL(_L8("'>\r\n"));

		// Settings
		aBuf.Format(_L8("\t\t<settings apmode='%d' apid='%d' apname='"), iSettingConnectionMode, iSettingConnectionModeId);
		aFile.WriteL(aBuf);
		aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iXmppEngine->GetConnectionName()));
		aFile.WriteL(_L8("'>\r\n\t\t\t<account"));

		if(iSettingFullName.Length() > 0) {
			aFile.WriteL(_L8(" fullname='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingFullName));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingUsername.Length() > 0) {
			aFile.WriteL(_L8(" username='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingUsername));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingPassword.Length() > 0) {
			aFile.WriteL(_L8(" password='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingPassword));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingXmppServer.Length() > 0) {
			aFile.WriteL(_L8(" server='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingXmppServer));
			aFile.WriteL(_L8("'"));
		}
		
		// Preferences
		aBuf.Format(_L8("/>\r\n\t\t\t<preferences language='%d' autostart='%d' notifyfollowing='%d' notifymoderating='%d' accesspoint='%d' replyto='%d' messageblocking='%d'/>\r\n"), iSettingPreferredLanguage, iSettingAutoStart, iSettingNotifyChannelsFollowing, iSettingNotifyChannelsModerating, iSettingAccessPoint, iSettingNotifyReplyTo, iSettingMessageBlocking);
		aFile.WriteL(aBuf);
		
		// Notifications
		aBuf.Format(_L8("\t\t\t<notifications pmt_id='%d' cmt_id='%d' drt_id='%d' pmt_file='"), iSettingPrivateMessageTone, iSettingChannelPostTone, iSettingDirectReplyTone);
		aFile.WriteL(aBuf);
		aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingPrivateMessageToneFile));
		aFile.WriteL(_L8("' cmt_file='"));
		aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingChannelPostToneFile));
		aFile.WriteL(_L8("' drt_file='"));
		aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingDirectReplyToneFile));
		
		// Beacons
		aBuf.Format(_L8("'/>\r\n\t\t\t<beacons cell='%d' wlan='%d' bt='%d' gps='%d'/>\r\n\t\t\t<communities>\r\n"), iSettingCellOn, iSettingWlanOn, iSettingBtOn, iSettingGpsOn);
		aFile.WriteL(aBuf);
		
		// Communities
		if(iSettingTwitterUsername.Length() > 0) {
			aFile.WriteL(_L8("\t\t\t\t<twitter username='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingTwitterUsername));
			aFile.WriteL(_L8("' password='"));
			aFile.WriteL(iTextUtilities->UnicodeToUtf8L(iSettingTwitterPassword));
			aFile.WriteL(_L8("'/>\r\n"));
		}
		
		aFile.WriteL(_L8("\t\t\t</communities>\r\n\t\t</settings>\r\n"));

		// Write Unsent Messages
		CXmppOutbox* aUnsentOutbox = iXmppEngine->GetMessageOutbox();

		if(aUnsentOutbox->Count() > 0) {
			aFile.WriteL(_L8("\t\t<unsent>"));

			for(TInt i = 0; i < aUnsentOutbox->Count(); i++) {
				CXmppOutboxMessage* aMessage = aUnsentOutbox->GetMessage(i);
				
				if(aMessage->GetPersistance()) {
					aFile.WriteL(_L8("\t\t\t"));
					aFile.WriteL(aMessage->GetStanza());
				}
			}

			aFile.WriteL(_L8("\t\t</unsent>\r\n"));
		}

		for(TInt i = 0; i < iFollowingList->Count();i++) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
			
			aFile.WriteL(_L8("\t\t<item last='"));
			aTimeUtilities->EncodeL(aItem->GetLastUpdated(), aTime);
			aFile.WriteL(aTime);
			aBuf.Format(_L8("' iconid='%d'>\r\n"), aItem->GetIconId());
			aFile.WriteL(aBuf);		

			if(aItem->GetItemType() == EItemNotice) {
				aBuf.Format(_L8("\t\t\t<notice type='%d' jid='"), aItem->GetIdType());
				aFile.WriteL(aBuf);

				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aItem->GetId()));
				aFile.WriteL(_L8("'"));

				if(aItem->GetTitle().Length() > 0) {
					aFile.WriteL(_L8(" name='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aItem->GetTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aItem->GetDescription().Length() > 0) {
					aFile.WriteL(_L8(" description='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aItem->GetDescription()));
					aFile.WriteL(_L8("'"));
				}
				
				aFile.WriteL(_L8("/>\r\n"));
			}
			else if(aItem->GetItemType() == EItemChannel) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);

				aFile.WriteL(_L8("\t\t\t<topicchannel"));

				if(aChannelItem->GetId().Length() > 0) {
					aFile.WriteL(_L8(" jid='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()));
					aFile.WriteL(_L8("'"));
				}

				if(aChannelItem->GetTitle().Length() > 0) {
					aFile.WriteL(_L8(" title='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aChannelItem->GetSubTitle().Length() > 0) {
					aFile.WriteL(_L8(" subtitle='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aChannelItem->GetSubTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aChannelItem->GetDescription().Length() > 0) {
					aFile.WriteL(_L8(" description='"));
					aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aChannelItem->GetDescription()));
					aFile.WriteL(_L8("'"));
				}
				
				CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());

				if(aDiscussion->GetUnreadEntries() > 0) {
					aBuf.Format(_L8(" unreadentries='%d' unreadreplies='%d'"), aDiscussion->GetUnreadEntries(), aDiscussion->GetUnreadReplies());
					aFile.WriteL(aBuf);
				}

				aFile.WriteL(_L8("/>\r\n"));
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(!iRosterSynchronized || aRosterItem->GetSubscription() > EPresenceSubscriptionNone) {
					aFile.WriteL(_L8("\t\t\t<roster"));

					if(aRosterItem->GetId().Length() > 0) {
						aFile.WriteL(_L8(" jid='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));
						aFile.WriteL(_L8("'"));
					}
					
					if(aRosterItem->GetTitle().Length() > 0) {
						aFile.WriteL(_L8(" title='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetTitle()));
						aFile.WriteL(_L8("'"));
					}
					
					if(aRosterItem->GetSubTitle().Length() > 0) {
						aFile.WriteL(_L8(" subtitle='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetSubTitle()));
						aFile.WriteL(_L8("'"));
					}
					
					if(aRosterItem->GetDescription().Length() > 0) {
						aFile.WriteL(_L8(" description='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetDescription()));
						aFile.WriteL(_L8("'"));
					}

					aBuf.Format(_L8(" own='%d'"), aRosterItem->OwnItem());
					aFile.WriteL(aBuf);
					
					CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());

					if(aDiscussion->GetUnreadEntries() > 0) {
						aBuf.Format(_L8(" unreadprivate='%d'"), aDiscussion->GetUnreadEntries());
						aFile.WriteL(aBuf);
					}

					aFile.WriteL(_L8(">\r\n"));

					if(aRosterItem->GetId(EIdChannel).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<channel jid='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId(EIdChannel)));
						aFile.WriteL(_L8("'"));
						
						aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId(EIdChannel));

						if(aDiscussion->GetUnreadEntries() > 0) {
							aBuf.Format(_L8(" unreadentries='%d' unreadreplies='%d'"), aDiscussion->GetUnreadEntries(), aDiscussion->GetUnreadReplies());
							aFile.WriteL(aBuf);
						}
						
						aFile.WriteL(_L8("/>\r\n"));
					}

					// Place
					if(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<location type='0' text='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					if(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<location type='1' text='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<location type='2' text='"));
						aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					aFile.WriteL(_L8("\t\t\t</roster>\r\n"));
				}
			}
			
			aFile.WriteL(_L8("\t\t</item>\r\n"));
		}

		aFile.WriteL(_L8("\t</logicstate>\r\n</?xml?>"));
		
		WriteToLog(_L8("BL    Externalize Logic State -- End"));

		CleanupStack::PopAndDestroy(); // aTimeUtilities
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::LoadPlaceItems() {
	TRAPD(aErr, LoadPlaceItemsL());

	if(aErr != KErrNone) {
		TBuf8<256> aLog;
		aLog.Format(_L8("LOGIC CBuddycloudLogic::LoadPlaceItems Trap: %d"), aErr);
		WriteToLog(aLog);
	}
}

void CBuddycloudLogic::LoadPlaceItemsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::LoadPlaceItemsL"));
#endif

	RFs aSession = CCoeEnv::Static()->FsSession();	
	RFile aFile;
	TInt aFileSize;
		
	TFileName aFilePath(_L("PlaceStoreItems.xml"));
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);

	if(aFile.Open(aSession, aFilePath, EFileStreamText|EFileRead) == KErrNone) {
		CleanupClosePushL(aFile);
		aFile.Size(aFileSize);

		// Create buffer & read file
		HBufC8* aBuf = HBufC8::NewLC(aFileSize);
		TPtr8 pBuf(aBuf->Des());
		aFile.Read(pBuf);

		WriteToLog(_L8("BL    Internalize Place Store"));

		CXmlParser* aXmlParser = CXmlParser::NewLC(*aBuf);
		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();

		if(aXmlParser->MoveToElement(_L8("places"))) {
			while(aXmlParser->MoveToElement(_L8("place"))) {
				CBuddycloudExtendedPlace* aPlace = CBuddycloudExtendedPlace::NewLC();

				aPlace->SetItemId(aXmlParser->GetIntAttribute(_L8("id")));
				aPlace->SetRevision(aXmlParser->GetIntAttribute(_L8("rev"), -1));
				aPlace->SetShared(aXmlParser->GetBoolAttribute(_L8("public")));

				if(aXmlParser->MoveToElement(_L8("geoloc"))) {
					CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
					CGeolocData* aGeoloc = aGeolocParser->XmlToGeolocLC(aXmlParser->GetStringData());
					
					aPlace->CopyGeolocL(aGeoloc);
					aPlace->UpdateFromGeolocL();
					
					CleanupStack::PopAndDestroy(2); // aGeoloc, aGeolocParser
				}

				if(aXmlParser->MoveToElement(_L8("visit"))) {
					aPlace->SetVisits(aXmlParser->GetIntAttribute(_L8("count")));
					aPlace->SetPlaceSeen(TBuddycloudPlaceSeen(aXmlParser->GetIntAttribute(_L8("type"))));
					aPlace->SetLastSeen(aTimeUtilities->DecodeL(aXmlParser->GetStringAttribute(_L8("lasttime"))));
					aPlace->SetVisitSeconds(aXmlParser->GetIntAttribute(_L8("lastsecs")));
					aPlace->SetTotalSeconds(aXmlParser->GetIntAttribute(_L8("totalsecs")));
				}

				iPlaceList->AddItem(aPlace);
				CleanupStack::Pop(); // aPlace
			}
		}

		CleanupStack::PopAndDestroy(3); // aTimeUtilities, aXmlParser, aBuf
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::SavePlaceItemsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SavePlaceItemsL"));
#endif

	RFs aSession = CCoeEnv::Static()->FsSession();	
	RFileWriteStream aFile;
	TBuf8<256> aBuf;

	TFileName aFilePath(_L("PlaceStoreItems.xml"));
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);

	if(aFile.Replace(aSession, aFilePath, EFileStreamText|EFileWrite) == KErrNone) {
		CleanupClosePushL(aFile);

		WriteToLog(_L8("BL    Externalize Place Store"));

		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
		TFormattedTimeDesc aTime;

		aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>\r\n\t<places>\r\n"));

		TTime aTimeNow;
		aTimeNow.UniversalTime();

		for(TInt i = 0;i < iPlaceList->Count();i++) {
			CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemByIndex(i));

			if(aPlace && aPlace->GetItemId() > 0) {
				// Place
				aBuf.Format(_L8("\t\t<place id='%d' rev='%d' public='%d'>\r\n"), aPlace->GetItemId(), aPlace->GetRevision(), aPlace->Shared());
				aFile.WriteL(aBuf);

				// Geoloc
				aFile.WriteL(_L8("\t\t\t<geoloc xmlns='http://jabber.org/protocol/geoloc'>\r\n\t\t\t\t<uri>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocUri)));
				aFile.WriteL(_L8("</uri>\r\n\t\t\t\t<text>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocText)));
				aFile.WriteL(_L8("</text>\r\n\t\t\t\t<street>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocStreet)));
				aFile.WriteL(_L8("</street>\r\n\t\t\t\t<area>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocArea)));
				aFile.WriteL(_L8("</area>\r\n\t\t\t\t<locality>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocLocality)));
				aFile.WriteL(_L8("</locality>\r\n\t\t\t\t<postalcode>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocPostalcode)));
				aFile.WriteL(_L8("</postalcode>\r\n\t\t\t\t<region>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocRegion)));
				aFile.WriteL(_L8("</region>\r\n\t\t\t\t<country>"));
				aFile.WriteL(iTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocCountry)));
				aBuf.Format(_L8("</country>\r\n\t\t\t\t<lat>%.6f</lat>\r\n\t\t\t\t<lon>%.6f</lon>\r\n\t\t\t\t<accuracy>%.3f</accuracy>\r\n\t\t\t</geoloc>\r\n"), aPlace->GetGeoloc()->GetReal(EGeolocLatitude), aPlace->GetGeoloc()->GetReal(EGeolocLongitude), aPlace->GetGeoloc()->GetReal(EGeolocAccuracy));
				aFile.WriteL(aBuf);

				// Visit
				aBuf.Format(_L8("\t\t\t<visit count='%d'"), aPlace->GetVisits());
				aFile.WriteL(aBuf);

				if(aPlace->GetPlaceSeen() == EPlaceHere || aPlace->GetPlaceSeen() == EPlaceFlux) {
					aBuf.Format(_L8(" type='%d'"), EPlaceRecent);
				}
				else {
					aBuf.Format(_L8(" type='%d'"), aPlace->GetPlaceSeen());
				}
				aFile.WriteL(aBuf);

				aFile.WriteL(_L8(" lasttime='"));
				aTimeUtilities->EncodeL(aPlace->GetLastSeen(), aTime);
				aFile.WriteL(aTime);

				aBuf.Format(_L8("' lastsecs='%d' totalsecs='%d'/>\r\n\t\t</place>\r\n"), aPlace->GetVisitSeconds(), aPlace->GetTotalSeconds());
				aFile.WriteL(aBuf);
			}
		}

		aFile.WriteL(_L8("\t</places>\r\n</?xml?>"));

		CleanupStack::PopAndDestroy(); // aTimeUtilities
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::BackupOldLog() {
	RFs aSession = CCoeEnv::Static()->FsSession();	
	
	_LIT(KLogFile, "C:\\Logs\\Buddycloud\\LogicLog.txt");
	_LIT(KBackupFile, "C:\\Logs\\Buddycloud\\LogicLog.bak.txt");
	
	// Delete old backup files
	aSession.Delete(KBackupFile);
	
	if(aSession.Rename(KLogFile, KBackupFile) != KErrNone) {
		// Create path
		aSession.MkDirAll(KLogFile);
	}
}

void CBuddycloudLogic::WriteToLog(const TDesC8& aText) {
#ifdef _DEBUG
	if(iLog.Handle()) {
		TInt aSearchResult, aSearchTemp;
		TPtrC8 aBufferedText(aText);
		
		if(aText.Length() > 128) {
			// Add line breaks around multiline log
			iLog.Write(KNullDesC);
		}

		while(aBufferedText.Length() > 128) {
			TPtrC8 aTextLine(aBufferedText.Left(128));

			aSearchResult = aTextLine.LocateReverse(' ');
			aSearchTemp = aTextLine.LocateReverse('>');

			if(aSearchResult == KErrNotFound || (aSearchTemp != KErrNotFound && aSearchTemp > aSearchResult)) {
				aSearchResult = aSearchTemp;
			}

			if(aSearchResult != KErrNotFound) {
				aTextLine.Set(aTextLine.Left(aSearchResult + 1));
			}

			aBufferedText.Set(aBufferedText.Mid(aTextLine.Length()));

			iLog.Write(aTextLine);
		}

		iLog.Write(aBufferedText);
	}
#endif
}

/*
----------------------------------------------------------------------------
--
-- Date Stamp
--
----------------------------------------------------------------------------
*/

TTime CBuddycloudLogic::TimeStamp() {
	TTime aNow;
	aNow.UniversalTime();
	aNow -= iServerOffset;

	return aNow;
}

/*
----------------------------------------------------------------------------
--
-- Observers
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::AddNotificationObserver(MBuddycloudLogicNotificationObserver* aNotificationObserver) {
	iNotificationObservers.Append(aNotificationObserver);
}

void CBuddycloudLogic::RemoveNotificationObserver(MBuddycloudLogicNotificationObserver* aNotificationObserver) {
	for(TInt i = 0; i < iNotificationObservers.Count(); i++) {
		if(iNotificationObservers[i] == aNotificationObserver) {
			iNotificationObservers.Remove(i);
			break;
		}
	}
}

void CBuddycloudLogic::NotifyNotificationObservers(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	for(TInt i = 0; i < iNotificationObservers.Count(); i++) {
		iNotificationObservers[i]->NotificationEvent(aEvent, aId);
	}
}

void CBuddycloudLogic::AddStatusObserver(MBuddycloudLogicStatusObserver* aStatusObserver) {
	iStatusObserver = aStatusObserver;
}

void CBuddycloudLogic::RemoveStatusObserver() {
	iStatusObserver = NULL;
}

/*
----------------------------------------------------------------------------
--
-- Items
--
----------------------------------------------------------------------------
*/

CBuddycloudFollowingStore* CBuddycloudLogic::GetFollowingStore() {
	return iFollowingList;
}

CFollowingRosterItem* CBuddycloudLogic::GetOwnItem() {
	return iOwnItem;
}

void CBuddycloudLogic::UnfollowItemL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnfollowItemL"));
#endif

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));

	if(aItem) {
		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(!aRosterItem->OwnItem() && aRosterItem->GetSubscription() > EPresenceSubscriptionRemove) {
				// Remove roster subscription
				TPtrC8 aEncJid(iTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));

				_LIT8(KStanza, "<iq type='set' id='remove1'><query xmlns='jabber:iq:roster'><item subscription='remove' jid=''/></query></iq>\r\n");
				HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aEncJid.Length());
				TPtr8 pStanza(aStanza->Des());
				pStanza.Append(KStanza);
				pStanza.Insert(93, aEncJid);
				
				iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
				CleanupStack::PopAndDestroy(); // aStanza
			}
		}
		else if(aItem->GetItemType() == EItemChannel) {
			UnfollowChannelL(aItemId);
		}		
	}
}

/*
----------------------------------------------------------------------------
--
-- Messaging
--
----------------------------------------------------------------------------
*/

TInt CBuddycloudLogic::IsSubscribedTo(const TDesC& aId, TInt aItemOptions) {
	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if((aItem->GetItemType() == EItemNotice && aItemOptions & EItemNotice) ||
				(aItem->GetItemType() >= EItemRoster && aItemOptions & EItemChannel)) {
			
			if(aItem->GetId().CompareF(aId) == 0) {
				// Check id
				return aItem->GetItemId();
			}		
		}
		
		if(aItem->GetItemType() == EItemRoster && aItemOptions & EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(aRosterItem->GetId().CompareF(aId) == 0) {
				// Check roster jid
				if(aRosterItem->GetSubscription() >= EPresenceSubscriptionTo) {
					return aItem->GetItemId();
				}
				else {
					return false;
				}
			}
		}
	}
	
	return false;
}

void CBuddycloudLogic::RespondToNoticeL(TInt aItemId, TNoticeResponse aResponse) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RespondToNoticeL"));
#endif
	
	if(iOwnItem) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
		
		if(aItem && aItem->GetItemType() == EItemNotice) {
			if(aItem->GetIdType() == EIdRoster) {
				TPtrC8 aEncId = iTextUtilities->UnicodeToUtf8L(aItem->GetId());
				
				if(aResponse <= ENoticeAcceptAndFollow) {
					// Accept request
					SendPresenceSubscriptionL(aEncId, _L8("subscribed"));
					
					if(aResponse == ENoticeAcceptAndFollow) {
						// Follow requester back
						FollowContactL(aItem->GetId());
					}
				}
				else {
					// Unsubscribe to presence
					SendPresenceSubscriptionL(aEncId, _L8("unsubscribed"));
				}
			}
			
			iFollowingList->DeleteItemById(aItemId);
			
			NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);
		}
	}
}

void CBuddycloudLogic::MediaPostRequestL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::MediaPostRequestL"));
#endif

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
	
	if(iOwnItem && aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		if(aChannelItem->GetId().Length() > 0) {		
			CGeolocData* aGeoloc = iOwnItem->GetGeolocItem(EGeolocItemCurrent);
			
			HBufC8* aEncId = iTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()).AllocLC();
			TPtr8 pEncId(aEncId->Des());
			
			HBufC* aLangCode = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE);
			TPtrC8 aEncLangCode(iTextUtilities->UnicodeToUtf8L(*aLangCode));
			
			_LIT8(KMediaStanza, "<iq to='media.buddycloud.com' type='set' id='%02d:%02d:%d' xml:lang='$LANG'><media xmlns='http://buddycloud.com/media#request'><placeid>%d</placeid><lat>%.6f</lat><lon>%.6f</lon><node></node></media></iq>\r\n");
			HBufC8* aMediaStanza = HBufC8::NewLC(KMediaStanza().Length() + aEncLangCode.Length() + pEncId.Length() + 8 + 10 + 14);
			TPtr8 pMediaStanza(aMediaStanza->Des());
			pMediaStanza.AppendFormat(KMediaStanza, EXmppIdRequestMediaPost, GetNewIdStamp(), aItem->GetItemId(), iStablePlaceId, aGeoloc->GetReal(EGeolocLatitude),  aGeoloc->GetReal(EGeolocLongitude));
			pMediaStanza.Insert(pMediaStanza.Length() - 22, pEncId);
			pMediaStanza.Replace(pMediaStanza.Find(_L8("$LANG")), 5, aEncLangCode);
			
			iXmppEngine->SendAndAcknowledgeXmppStanza(pMediaStanza, this, true);
			CleanupStack::PopAndDestroy(3); // aMediaStanza, aLangCode, aEncId
		}
	}
}

CDiscussion* CBuddycloudLogic::GetDiscussion(const TDesC& aJid) {
	return iDiscussionManager->GetDiscussionL(aJid);			
}

void CBuddycloudLogic::PostMessageL(TInt aItemId, TDesC& aId, TDesC& aContent, const TDesC8& aReferenceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::PostMessageL"));
#endif
	
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
	
	if(iOwnItem && aItem && aItem->GetItemType() >= EItemRoster) {
		HBufC8* aEncId = iTextUtilities->UnicodeToUtf8L(aId).AllocLC();
		TPtr8 pEncId(aEncId->Des());
				
		if(aItem->GetItemType() == EItemRoster && aItem->GetId().CompareF(aId) != 0) {	
			// Send private message
			TFormattedTimeDesc aThreadData;
			TTime aCurrentTime = TimeStamp();
			
			CTimeUtilities::EncodeL(aCurrentTime, aThreadData);
			
			HBufC8* aEncContent = iTextUtilities->UnicodeToUtf8L(aContent).AllocLC();
			TPtr8 pEncContent(aEncContent->Des());
			
			_LIT8(KThreadParent, " parent=''");
			_LIT8(KMessageStanza, "<message type='chat' to='' id='message:%02d'><body></body><thread></thread></message>\r\n");
			HBufC8* aMessageStanza = HBufC8::NewLC(KMessageStanza().Length() + pEncId.Length() + pEncContent.Length() + KThreadParent().Length() + aReferenceId.Length() + aThreadData.Length());
			TPtr8 pMessageStanza(aMessageStanza->Des());
			pMessageStanza.AppendFormat(KMessageStanza, GetNewIdStamp());
			pMessageStanza.Insert(64, aThreadData);
			
			if(aReferenceId.Length() > 0) {
				pMessageStanza.Insert(63, KThreadParent);
				pMessageStanza.Insert(72, aReferenceId);				
			}
			
			pMessageStanza.Insert(49, pEncContent);
			pMessageStanza.Insert(25, pEncId);
			
			iXmppEngine->SendAndForgetXmppStanza(pMessageStanza, true, EXmppPriorityHigh);			
			CleanupStack::PopAndDestroy(2); // aMessageStanza, aEncContent
			
			// Store private message
			CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
			aAtomEntry->SetIdL(aThreadData);
			aAtomEntry->SetPublishTime(aCurrentTime);
			aAtomEntry->SetAuthorNameL(iOwnItem->GetId());	
			aAtomEntry->SetAuthorJidL(iOwnItem->GetId());	
			aAtomEntry->GetLocation()->SetStringL(EGeolocText, iOwnItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText));			
			aAtomEntry->SetContentL(aContent);			
			aAtomEntry->SetIconId(iOwnItem->GetIconId());	
			aAtomEntry->SetPrivate(true);
			aAtomEntry->SetRead(true);
			
			if(aItem->GetId().CompareF(iOwnItem->GetId()) == 0) {
				aAtomEntry->SetAuthorAffiliation(EPubsubAffiliationOwner);
			}
			
			// Add to discussion
			CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aId);
			if(aDiscussion->AddEntryOrCommentLD(aAtomEntry, aReferenceId)) {
				if(iFollowingList->GetIndexById(aItem->GetItemId()) > 0) {
					// Bubble item
					iFollowingList->RemoveItemById(aItem->GetItemId());
					iFollowingList->InsertItem(GetBubbleToPosition(aItem), aItem);
					
					NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aItem->GetItemId());
				}
				
				aItem->SetLastUpdated(TimeStamp());
			}
		}
		else {
			// Channel post			
			CGeolocData* aGeoloc = iOwnItem->GetGeolocItem(EGeolocItemCurrent);
			
			CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
			aAtomEntry->GetLocation()->SetStringL(EGeolocText, *iBroadLocationText);			
			aAtomEntry->GetLocation()->SetStringL(EGeolocLocality, aGeoloc->GetString(EGeolocLocality));			
			aAtomEntry->GetLocation()->SetStringL(EGeolocCountry, aGeoloc->GetString(EGeolocCountry));			
			aAtomEntry->SetAuthorNameL(iSettingFullName);	
			aAtomEntry->SetAuthorJidL(iOwnItem->GetId());	
			aAtomEntry->SetContentL(aContent, EEntryUnprocessed);			

			CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();
			TPtrC8 aAtomEntryXml(aAtomEntryParser->AtomEntryToXmlL(aAtomEntry, aReferenceId));
			
			_LIT8(KPublishStanza, "<iq to='' type='set' id='%02d:%02d:%d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node=''><item></item></publish></pubsub></iq>\r\n");
			HBufC8* aPublishStanza = HBufC8::NewLC(KPublishStanza().Length() + KBuddycloudPubsubServer().Length() + pEncId.Length() + aAtomEntryXml.Length() + 8);
			TPtr8 pPublishStanza(aPublishStanza->Des());
			pPublishStanza.AppendFormat(KPublishStanza, EXmppIdPublishChannelPost, GetNewIdStamp(), aItemId);
			pPublishStanza.Insert(pPublishStanza.Length() - 41, pEncId);
			pPublishStanza.Insert(pPublishStanza.Length() - 33, aAtomEntryXml);
			pPublishStanza.Insert(8, KBuddycloudPubsubServer);
			
			iXmppEngine->SendAndAcknowledgeXmppStanza(pPublishStanza, this, true, EXmppPriorityHigh);			
			CleanupStack::PopAndDestroy(3); // aPublishStanza, aAtomEntryXml, aPublishStanza
		}
		
		if(iState == ELogicOffline) {
			// Send message when online dialog
			ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_MESSAGEQUEUED);
		}	
		
		CleanupStack::PopAndDestroy(); // aEncId
	}	
}

/*
----------------------------------------------------------------------------
--
-- Communities
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::SendCommunityCredentials(TCommunityItems aCommunity) {
	TBuf8<32> aCommunityName;
	TPtrC pCommunityUsername;
	TPtrC pCommunityPassword;
	
	if(aCommunity == ECommunityTwitter) {
		aCommunityName.Append(_L8("twitter"));
		pCommunityUsername.Set(iSettingTwitterUsername);
		pCommunityPassword.Set(iSettingTwitterPassword);
	}
	
	if(aCommunityName.Length() > 0) {
		HBufC8* aEncUsername = iTextUtilities->UnicodeToUtf8L(pCommunityUsername).AllocLC();
		TPtr8 pEncUsername(aEncUsername->Des());
		
		HBufC8* aEncPassword = iTextUtilities->UnicodeToUtf8L(pCommunityPassword).AllocLC();
		TPtr8 pEncPassword(aEncPassword->Des());

		_LIT8(KStanza, "<iq to='communities.buddycloud.com' type='set' id='%02d:%02d'><credentials application='' username='' password=''/></iq>\r\n");
		HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aCommunityName.Length() + pEncUsername.Length() + pEncPassword.Length());
		TPtr8 pStanza = aStanza->Des();
		pStanza.AppendFormat(KStanza, EXmppIdSetCredentials, GetNewIdStamp());
		pStanza.Insert(108, pEncPassword);
		pStanza.Insert(96, pEncUsername);
		pStanza.Insert(84, aCommunityName);

		iXmppEngine->SendAndAcknowledgeXmppStanza(pStanza, this, true);
		CleanupStack::PopAndDestroy(2); // aEncPassword, aEncUsername
	}
}

/*
----------------------------------------------------------------------------
--
-- Avatars
--
----------------------------------------------------------------------------
*/

CAvatarRepository* CBuddycloudLogic::GetImageRepository() {
	return iAvatarRepository;
}

/*
----------------------------------------------------------------------------
--
-- Location engine
--
----------------------------------------------------------------------------
*/

MLocationEngineDataInterface* CBuddycloudLogic::GetLocationInterface() {
	return (MLocationEngineDataInterface*)iLocationEngine;
}

/*
----------------------------------------------------------------------------
--
-- Places
--
----------------------------------------------------------------------------
*/

CBuddycloudPlaceStore* CBuddycloudLogic::GetPlaceStore() {
	return iPlaceList;
}

void CBuddycloudLogic::SendEditedPlaceDetailsL(TBool aSearchOnly) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendEditedPlaceDetailsL"));
#endif
	
	iPlacesSaveNeeded = true;
	
	CBuddycloudExtendedPlace* aEditingPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetEditedPlace());
	
	// Send New/Updated place to server
	if(aEditingPlace) {
		CGeolocData* aGeoloc = aEditingPlace->GetGeoloc();

		// -----------------------------------------------------------------
		// Get data from edit
		// -----------------------------------------------------------------
		// Name
		HBufC8* aPlaceName = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocText)).AllocLC();
		TPtr8 pPlaceName(aPlaceName->Des());

		// Street
		HBufC8* aPlaceStreet = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocStreet)).AllocLC();
		TPtr8 pPlaceStreet(aPlaceStreet->Des());

		// Area
		HBufC8* aPlaceArea = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocArea)).AllocLC();
		TPtr8 pPlaceArea(aPlaceArea->Des());

		// City
		HBufC8* aPlaceCity = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocLocality)).AllocLC();
		TPtr8 pPlaceCity(aPlaceCity->Des());

		// Postcode
		HBufC8* aPlacePostcode = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocPostalcode)).AllocLC();
		TPtr8 pPlacePostcode(aPlacePostcode->Des());

		// Region
		HBufC8* aPlaceRegion = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocRegion)).AllocLC();
		TPtr8 pPlaceRegion(aPlaceRegion->Des());

		// Country
		HBufC8* aPlaceCountry = iTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocCountry)).AllocLC();
		TPtr8 pPlaceCountry(aPlaceCountry->Des());
		
		if(aEditingPlace->GetItemId() < 0) {
			if(aGeoloc->GetReal(EGeolocLatitude) == 0.0 && aGeoloc->GetReal(EGeolocLongitude) == 0.0) {
				TReal aLatitude, aLongitude;
				
				iLocationEngine->GetGpsPosition(aLatitude, aLongitude);
				aGeoloc->SetRealL(EGeolocLatitude, aLatitude);
				aGeoloc->SetRealL(EGeolocLongitude, aLongitude);
			}
		}
		
		// Latitude
		TBuf8<16> aPlaceLatitude;
		aPlaceLatitude.Format(_L8("%.6f"), aGeoloc->GetReal(EGeolocLatitude));

		// Longitude
		TBuf8<16> aPlaceLongitude;
		aPlaceLongitude.Format(_L8("%.6f"), aGeoloc->GetReal(EGeolocLongitude));
		
		// Build core form stanza
		_LIT8(KLatitudeElement, "<lat></lat>");
		_LIT8(KLongitudeElement, "<lon></lon>");			
		_LIT8(KCoreDetailsStanza, "<iq to='butler.buddycloud.com' type='' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/place#'><place><name></name><street></street><area></area><city></city><region></region><country></country><postalcode></postalcode></place></query></iq>\r\n");
		
		HBufC8* aCoreStanza = HBufC8::NewLC(KCoreDetailsStanza().Length() + pPlaceName.Length() + pPlaceStreet.Length() + 
				pPlaceArea.Length() + pPlaceCity.Length() + pPlacePostcode.Length() + pPlaceRegion.Length() + 
				pPlaceCountry.Length() + KLatitudeElement().Length() + aPlaceLatitude.Length() + KLongitudeElement().Length() + aPlaceLongitude.Length());		
		TPtr8 pCoreStanza(aCoreStanza->Des());
		pCoreStanza.Append(KCoreDetailsStanza);
		pCoreStanza.Insert(218, pPlacePostcode);
		pCoreStanza.Insert(196, pPlaceCountry);
		pCoreStanza.Insert(178, pPlaceRegion);
		pCoreStanza.Insert(163, pPlaceCity);
		pCoreStanza.Insert(150, pPlaceArea);
		pCoreStanza.Insert(135, pPlaceStreet);
		pCoreStanza.Insert(120, pPlaceName);
	
		if(aGeoloc->GetReal(EGeolocLatitude) != 0.0 || aGeoloc->GetReal(EGeolocLongitude) != 0.0) {
			pCoreStanza.Insert(pCoreStanza.Length() - 23, KLongitudeElement);
			pCoreStanza.Insert(pCoreStanza.Length() - 29, aPlaceLongitude);
			pCoreStanza.Insert(pCoreStanza.Length() - 23, KLatitudeElement);
			pCoreStanza.Insert(pCoreStanza.Length() - 29, aPlaceLatitude);
		}

		if(aSearchOnly) {
			HBufC8* aSearchStanza = HBufC8::NewLC(pCoreStanza.Length() + KPlaceSearch().Length() + KIqTypeGet().Length());
			TPtr8 pSearchStanza(aSearchStanza->Des());							
			pSearchStanza.AppendFormat(pCoreStanza, EXmppIdSearchForPlace, GetNewIdStamp());
			pSearchStanza.Insert(101, KPlaceSearch);
			pSearchStanza.Insert(37, KIqTypeGet);
			
			iXmppEngine->SendAndAcknowledgeXmppStanza(pSearchStanza, this);
			CleanupStack::PopAndDestroy(); // aSearchStanza
		}
		else {		
			TBuf8<20> aSharedElement;
			aSharedElement.Format(_L8("<shared>%d</shared>"), aEditingPlace->Shared());
			
			if(aEditingPlace->GetItemId() < 0) {
				HBufC8* aCreateStanza = HBufC8::NewLC(pCoreStanza.Length() + KPlaceCreate().Length() + aSharedElement.Length() + KIqTypeSet().Length());
				TPtr8 pCreateStanza(aCreateStanza->Des());							
				pCreateStanza.AppendFormat(pCoreStanza, EXmppIdCreatePlace, GetNewIdStamp());
				pCreateStanza.Insert(110, aSharedElement);
				pCreateStanza.Insert(101, KPlaceCreate);
				pCreateStanza.Insert(37, KIqTypeSet);
				
				iXmppEngine->SendAndAcknowledgeXmppStanza(pCreateStanza, this, true);
				CleanupStack::PopAndDestroy(); // aCreateStanza
	
				// Delete New Place (will be recollected)
				iPlaceList->DeleteItemById(aEditingPlace->GetItemId());
				
				// Set Activity Status
				if(iState == ELogicOnline) {
					SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
				}
			}
			else {
				TBuf8<64> aIdElement;
				aIdElement.Format(_L8("<id>http://buddycloud.com/places/%d</id>"), aEditingPlace->GetItemId());
				
				HBufC8* aEditStanza = HBufC8::NewLC(pCoreStanza.Length() + KPlaceEdit().Length() + aSharedElement.Length() + aIdElement.Length() + KIqTypeSet().Length());
				TPtr8 pEditStanza(aEditStanza->Des());							
				pEditStanza.AppendFormat(pCoreStanza, EXmppEditPlaceDetails, GetNewIdStamp());
				pEditStanza.Insert(110, aSharedElement);
				pEditStanza.Insert(110, aIdElement);
				pEditStanza.Insert(101, KPlaceEdit);
				pEditStanza.Insert(37, KIqTypeSet);
	
				iXmppEngine->SendAndAcknowledgeXmppStanza(pEditStanza, this, true);
				CleanupStack::PopAndDestroy(); // aEditStanza
				
				aEditingPlace->UpdateFromGeolocL();
				
				iPlaceList->SetEditedPlace(KErrNotFound);
			}
		}

		CleanupStack::PopAndDestroy(8); // aCoreStanza, aPlaceCountry, aPlaceRegion, aPlacePostcode, aPlaceCity, aPlaceArea, aPlaceStreet, aPlaceName
	}
}

void CBuddycloudLogic::HandlePlaceQueryResultL(const TDesC8& aStanza, TBuddycloudXmppIdEnumerations aType) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePlaceQueryResultL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aXmlParser->MoveToElement(_L8("query"))) {
		CBuddycloudPlaceStore* aPlaceStore = CBuddycloudPlaceStore::NewLC();
		
		TPtrC8 aAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));
		
		while(aXmlParser->MoveToElement(_L8("place"))) {
			TPtrC8 aPlaceData(aXmlParser->GetStringData());
			
			CBuddycloudExtendedPlace* aPlace = CBuddycloudExtendedPlace::NewLC();
			
			// Parse generic geoloc data for place
			CXmppGeolocParser* aXmppGeolocParser = CXmppGeolocParser::NewLC();
			aPlace->CopyGeolocL(aXmppGeolocParser->XmlToGeolocLC(aPlaceData));
			CleanupStack::PopAndDestroy(2); // aGeoloc, aXmppGeolocParser
			
			// Parse place specific data
			CXmlParser* aPlaceXmlParser = CXmlParser::NewLC(aPlaceData);
			
			do {
				TPtrC8 aElementName = aPlaceXmlParser->GetElementName();
				TPtrC aElementData = iTextUtilities->Utf8ToUnicodeL(aPlaceXmlParser->GetStringData());
				
				if(aElementName.Compare(_L8("revision")) == 0) {
					aPlace->SetRevision(aPlaceXmlParser->GetIntData());
				}
				else if(aElementName.Compare(_L8("population")) == 0) {
					aPlace->SetPopulation(aPlaceXmlParser->GetIntData());
				}
				else if(aElementName.Compare(_L8("shared")) == 0) {
					aPlace->SetShared(aPlaceXmlParser->GetBoolData());
				}
			} while(aPlaceXmlParser->MoveToNextElement());
			
			CleanupStack::PopAndDestroy(); // aPlaceXmlParser
			
			aPlaceStore->AddItem(aPlace);
			CleanupStack::Pop(); // aPlace			
		}
		
		if(aType == EXmppIdGetPlaceSubscriptions) {
			ProcessPlaceSubscriptionsL(aPlaceStore);
		}
		else if(aType == EXmppIdSearchForPlace) {
			ProcessPlaceSearchL(aPlaceStore);
		}
		else if(aPlaceStore->Count() > 0) {
			CBuddycloudExtendedPlace* aResultPlace = static_cast <CBuddycloudExtendedPlace*> (aPlaceStore->GetItemByIndex(0));
						
			if(aResultPlace->GetItemId() > 0) {				
				CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aResultPlace->GetItemId()));

				if(aAttributeType.Compare(_L8("error")) != 0) {
					// Valid result returned, process					
					if(aPlace == NULL) {
						// Add to place to 'my place' list
						iPlaceList->AddItem(aResultPlace);
						aPlaceStore->RemoveItemByIndex(0);
					}
					else if(aType == EXmppIdGetPlaceDetails) {
						// New place details received, copy to existing place
						aPlace->CopyGeolocL(aResultPlace->GetGeoloc());
						aPlace->UpdateFromGeolocL();
						
						// Extra data
						aPlace->SetShared(aResultPlace->Shared());
						aPlace->SetRevision(aResultPlace->GetRevision());
						aPlace->SetPopulation(aResultPlace->GetPopulation());
					}
					
					// Current place set by placeid
					if(aType == EXmppIdSetCurrentPlace) {
						HandleLocationServerResult(EMotionStationary, 100, aResultPlace->GetItemId());
						SetActivityStatus(KNullDesC);
					}
					else if(aType == EXmppIdCreatePlace) {
						SetCurrentPlaceL(aResultPlace->GetItemId());
					}
				
					// Bubble place to top if it's our current location
					if(aResultPlace->GetItemId() == iStablePlaceId) {
						iPlaceList->MoveItemById(iStablePlaceId, 0);
					}
					
					NotifyNotificationObservers(ENotificationPlaceItemsUpdated, aResultPlace->GetItemId());
				}
				else {
					// Result returned error condition
					if(aType == EXmppIdGetPlaceDetails && aXmlParser->MoveToElement(_L8("not-authorized"))) {
						// User is no longer authorized to see this place, remove
						if(aPlace) {
							aPlace->SetShared(true);
							EditMyPlacesL(aResultPlace->GetItemId(), false);
						}
						
						NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
					}
				}				
			}
		}
		
		CleanupStack::PopAndDestroy(); // aPlaceStore
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::GetPlaceDetailsL(TInt aPlaceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::GetPlaceDetailsL"));
#endif
	
	CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aPlaceId));

	if(aPlace) {
		// Set minimal revision to avoid any detail re-requests
		aPlace->SetRevision(0);
	}

	// Send request
	_LIT8(KPlaceDetailsStanza, "<iq to='butler.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/place'><place><id>http://buddycloud.com/places/%d</id></place></query></iq>\r\n");
	HBufC8* aPlaceDetailsStanza = HBufC8::NewLC(KPlaceDetailsStanza().Length() + 32);
	TPtr8 pPlaceDetailsStanza(aPlaceDetailsStanza->Des());
	pPlaceDetailsStanza.AppendFormat(KPlaceDetailsStanza, EXmppIdGetPlaceDetails, GetNewIdStamp(), aPlaceId);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pPlaceDetailsStanza, this, true);
	CleanupStack::PopAndDestroy(); // aPlaceDetailsStanza
}

void CBuddycloudLogic::EditMyPlacesL(TInt aPlaceId, TBool aAddPlace) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::EditMyPlacesL"));
#endif
	
	CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aPlaceId));

	if(aAddPlace && aPlace == NULL) {
		// Add place to 'my places'
		SendPlaceQueryL(aPlaceId, EXmppIdAddPlaceSubscription, true);
	}
	else if(!aAddPlace && aPlace) {
		if(aPlace->Shared()) {
			// Remove place from 'my places'
			SendPlaceQueryL(aPlaceId, EXmppIdRemovePlaceSubscription);
		}
		else {
			// Delete private place
			SendPlaceQueryL(aPlaceId, EXmppIdDeletePlace);
		}
		
		// Remove place
		iPlaceList->DeleteItemById(aPlaceId);
	}
	
	NotifyNotificationObservers(ENotificationPlaceItemsUpdated, aPlaceId);
}

void CBuddycloudLogic::CollectPlaceSubscriptionsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectPlaceSubscriptionsL"));
#endif
	
	_LIT8(KSubscribedPlacesStanza, "<iq to='butler.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/place#myplaces'><options><feature var='id'/><feature var='revision'/><feature var='population'/></options></query></iq>\r\n");
	HBufC8* aSubscribedPlacesStanza = HBufC8::NewLC(KSubscribedPlacesStanza().Length());
	TPtr8 pSubscribedPlacesStanza(aSubscribedPlacesStanza->Des());
	pSubscribedPlacesStanza.AppendFormat(KSubscribedPlacesStanza, EXmppIdGetPlaceSubscriptions, GetNewIdStamp());
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pSubscribedPlacesStanza, this, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aSubscribedPlacesStanza
}

void CBuddycloudLogic::ProcessPlaceSubscriptionsL(CBuddycloudPlaceStore* aPlaceStore) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPlaceSubscriptionsL"));
#endif
	
	for(TInt i = (iPlaceList->Count() - 1); i >= 0; i--) {
		CBuddycloudExtendedPlace* aLocalPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemByIndex(i));
		CBuddycloudExtendedPlace* aStorePlace = static_cast <CBuddycloudExtendedPlace*> (aPlaceStore->GetItemById(aLocalPlace->GetItemId()));
		
		if(aLocalPlace->GetItemId() > 0) {
			if(aStorePlace) {
				// Place in 'my places'
				aLocalPlace->SetPopulation(aStorePlace->GetPopulation());
				
				if(aLocalPlace->GetRevision() != KErrNotFound && 
						aLocalPlace->GetRevision() < aStorePlace->GetRevision()) {
					
					// Revision out of date, recollect
					GetPlaceDetailsL(aLocalPlace->GetItemId());
				}
				
				aPlaceStore->DeleteItemById(aStorePlace->GetItemId());
			}
			else {
				// Place removed from 'my places'
				iPlaceList->DeleteItemByIndex(i);
			}
		}
	}
	
	while(aPlaceStore->Count() > 0) {
		// Place added to 'my places'
		CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (aPlaceStore->GetItemByIndex(0));
		aPlace->SetRevision(KErrNotFound);
		
		iPlaceList->AddItem(aPlace);
		aPlaceStore->RemoveItemByIndex(0);
	}
	
	iPlacesSaveNeeded = true;

	NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
}

void CBuddycloudLogic::ProcessPlaceSearchL(CBuddycloudPlaceStore* aPlaceStore) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPlaceSearchL"));
#endif

	SetActivityStatus(KNullDesC);
	
	CBuddycloudExtendedPlace* aEditingPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetEditedPlace());
	
	if(aEditingPlace) {	
		TInt aEditingPlaceId = aEditingPlace->GetItemId();
		
		if(aPlaceStore->Count() > 0) {
			// Multiple results
			for(TInt i = 0; i < aPlaceStore->Count(); i++) {
				CBuddycloudExtendedPlace* aResultPlace = static_cast <CBuddycloudExtendedPlace*> (aPlaceStore->GetItemByIndex(i));
	
				if(aResultPlace) {
					CGeolocData* aGeoloc = aResultPlace->GetGeoloc();
					
					_LIT(KResultText, "Result %d of %d:\n\n");
					
					// Prepare message
					HBufC* aMessage = HBufC::NewLC(KResultText().Length() + aGeoloc->GetString(EGeolocText).Length() + aGeoloc->GetString(EGeolocStreet).Length() + aGeoloc->GetString(EGeolocLocality).Length() + aGeoloc->GetString(EGeolocPostalcode).Length() + 5);
					TPtr pMessage(aMessage->Des());
					
					pMessage.AppendFormat(KResultText, (i + 1), aPlaceStore->Count());
					iTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocText), KNullDesC);
					iTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocStreet), _L(",\n"));
					iTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocLocality), _L(",\n"));
					iTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocPostalcode), _L(" "));
						
					// Prepare dialog
					CSearchResultMessageQueryDialog* iDidYouMeanDialog = new (ELeave) CSearchResultMessageQueryDialog();
					iDidYouMeanDialog->PrepareLC(R_DIDYOUMEAN_DIALOG);
					iDidYouMeanDialog->HasPreviousResult((i > 0));
					iDidYouMeanDialog->HasNextResult((i < aPlaceStore->Count() - 1));					
					iDidYouMeanDialog->SetMessageTextL(pMessage);
					
					TInt aDialogResult = iDidYouMeanDialog->RunLD();
					CleanupStack::PopAndDestroy(); // aMessage										
				
					if(aDialogResult == 0) {
						// Cancel
						return;
					}
					else if(aDialogResult == EMenuSelectCommand) {
						// Select place						
						aEditingPlace->CopyGeolocL(aGeoloc);
						aEditingPlace->UpdateFromGeolocL();
						
						if(aEditingPlaceId < 0 && aResultPlace->GetItemId() > 0) {
							// Existing place (has id) selected
							iPlaceList->DeleteItemById(aEditingPlaceId);
							
							// Set place as current
							SetCurrentPlaceL(aResultPlace->GetItemId());
							
							NotifyNotificationObservers(ENotificationEditPlaceCompleted);
						}
						else {
							NotifyNotificationObservers(ENotificationEditPlaceRequested);
						}
	
						break;
					}
					else if(aDialogResult == EMenuNewSearchCommand) {
						// New search
						NotifyNotificationObservers(ENotificationEditPlaceRequested);
						break;
					}
					else if(aDialogResult == EMenuPreviousCommand) {
						i -= 2;
					}
				}
			}
		}
		else {
			// No results
			ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);
		}
	}
}

void CBuddycloudLogic::SendPlaceQueryL(TInt aPlaceId, TBuddycloudXmppIdEnumerations aType, TBool aAcknowledge) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPlaceQueryL"));
#endif
	
	if(aType >= EXmppIdAddPlaceSubscription && aType <= EXmppIdSetCurrentPlace) {
		_LIT8(KPlaceQueryStanza, "<iq to='butler.buddycloud.com' type='set' id='%02d:%02d'><query xmlns='http://buddycloud.com/protocol/place#'><place><id>http://buddycloud.com/places/%d</id></place></query></iq>\r\n");
		HBufC8* aPlaceQueryStanza = HBufC8::NewLC(KPlaceQueryStanza().Length() + KPlaceCurrent().Length() + 32);
		TPtr8 pPlaceQueryStanza(aPlaceQueryStanza->Des());
		pPlaceQueryStanza.AppendFormat(KPlaceQueryStanza, aType, GetNewIdStamp(), aPlaceId);
		
		if(aType == EXmppIdAddPlaceSubscription) {
			pPlaceQueryStanza.Insert(104, KPlaceAdd);
		}
		else if(aType == EXmppIdRemovePlaceSubscription) {
			pPlaceQueryStanza.Insert(104, KPlaceRemove);
		}
		else if(aType == EXmppIdDeletePlace) {
			pPlaceQueryStanza.Insert(104, KPlaceDelete);
		}
		else {
			pPlaceQueryStanza.Insert(104, KPlaceCurrent);
		}
		
		if(aAcknowledge) {
			iXmppEngine->SendAndAcknowledgeXmppStanza(pPlaceQueryStanza, this, true);
		}
		else {
			iXmppEngine->SendAndForgetXmppStanza(pPlaceQueryStanza, true);
		}
		
		CleanupStack::PopAndDestroy(); // aPlaceQueryStanza	
	}
}

/*
----------------------------------------------------------------------------
--
-- MTimeoutNotification
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::TimerExpired(TInt /*aExpiryId*/) {
	if(iTimerState == ETimeoutStartConnection) {
		// Startup connection
		ConnectL();
	}
	else {
		if(iTimerState == ETimeoutConnected) {
			// Clear online activity
			SetActivityStatus(KNullDesC);
			
			iTimerState = ETimeoutSaveSettings;
		}
		else if(iTimerState == ETimeoutSaveSettings) {
			// Save settings
			if(iSettingsSaveNeeded) {
				SaveSettingsAndItemsL();
				
				iSettingsSaveNeeded = false;
			}
	
			iTimerState = ETimeoutSavePlaces;
		}
		else if(iTimerState == ETimeoutSavePlaces) {
			// Save places
			if(iPlacesSaveNeeded) {
				SavePlaceItemsL();	
				
				iPlacesSaveNeeded = false;
			}
			
			iTimerState = ETimeoutSaveSettings;
		}
		
		iTimer->After(150000000);
	}
}

/*
----------------------------------------------------------------------------
--
-- MAvatarRepositoryObserver
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::AvatarLoaded() {
	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
	NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
}

/*
----------------------------------------------------------------------------
--
-- MLocationEngineNotification
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::HandleLocationServerResult(TLocationMotionState aMotionState, TInt /*aPatternQuality*/, TInt aPlaceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleLocationServerResult"));
#endif
	
	TBool aNearbyRefreshNeeded = false;

	// Get places
	CBuddycloudExtendedPlace* aLastPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(iStablePlaceId));
	CBuddycloudExtendedPlace* aCurrentPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aPlaceId));
	
	// Check motion state
	if(aMotionState < iLastMotionState) {
		aNearbyRefreshNeeded = true;
	}
	
	iLastMotionState = aMotionState;

	if(aPlaceId > 0) {
		if(aCurrentPlace) {
			// Bubble place to top
			iPlaceList->MoveItemById(aPlaceId, 0);
		}
		else {
			// Collect unlisted place
			GetPlaceDetailsL(aPlaceId);
		}
		
		// Near or at a place
		if(aPlaceId != iStablePlaceId) {
			// Remove top placeholder
			if(iStablePlaceId <= 0) {
				iPlaceList->DeleteItemById(iStablePlaceId);
				aLastPlace = NULL;
			}

			// Set current place to flux place
			if(aLastPlace) {
				aLastPlace->SetPlaceSeen(EPlaceFlux);
			}

			if(aMotionState == EMotionStationary) {
				// Set current place to recent place
				if(aLastPlace) {
					aLastPlace->SetPlaceSeen(EPlaceRecent);
				}

				// At new place
				iStablePlaceId = aPlaceId;
				aNearbyRefreshNeeded = true;

				if(aCurrentPlace) {
					// Set a current place
					aCurrentPlace->SetPlaceSeen(EPlaceHere);
					
					if(aCurrentPlace->GetRevision() == KErrNotFound) {
						GetPlaceDetailsL(iStablePlaceId);
					}
				}
				
				iPlacesSaveNeeded = true;
			}
		}
		else {
			if(aMotionState == EMotionStationary) {
				if(aLastPlace != NULL) {
					aLastPlace->SetLastSeen(TimeStamp());
					aLastPlace->SetPlaceSeen(EPlaceHere);
				}
			}
		}
	}
	else {
		// On the road or non placed location
		if(aCurrentPlace == NULL) {
			// Add placeholder
			aCurrentPlace = CBuddycloudExtendedPlace::NewL();
			aCurrentPlace->SetShared(true);
			aCurrentPlace->SetRevision(0);
			
			iPlaceList->InsertItem(0, aCurrentPlace);
		}
		
		// Set current place to recent place
		if(aPlaceId != iStablePlaceId) {
			iStablePlaceId = aPlaceId;
	
			if(aLastPlace != NULL) {
				aLastPlace->SetPlaceSeen(EPlaceRecent);
			}
	
			aNearbyRefreshNeeded = true;
		}
	}
	
	// Refresh nearby list
	if(aNearbyRefreshNeeded) {
		TPtrC8 aReferenceJid = iTextUtilities->UnicodeToUtf8L(iOwnItem->GetId());
		
		_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='%02d:%02d'><query xmlns='urn:oslo:nearbyobjects'><reference type='person' id=''/><options limit='10'/><request var='place'/></query></iq>\r\n");
		HBufC8* aNearbyStanza = HBufC8::NewLC(KNearbyStanza().Length() + aReferenceJid.Length());
		TPtr8 pNearbyStanza(aNearbyStanza->Des());
		pNearbyStanza.AppendFormat(KNearbyStanza, EXmppIdGetNearbyPlaces, GetNewIdStamp());
		pNearbyStanza.Insert(120, aReferenceJid);
		
		iXmppEngine->SendAndAcknowledgeXmppStanza(pNearbyStanza, this);	
		CleanupStack::PopAndDestroy(); // aNearbyStanza
	}

	NotifyNotificationObservers(ENotificationLocationUpdated, aPlaceId);
}

void CBuddycloudLogic::LocationShutdownComplete() {
	if(!iLocationEngineReady) {
		iLocationEngineReady = true;
	
		if(iLocationEngineReady && iXmppEngineReady) {
			iOwnerObserver->ShutdownComplete();
		}
	}
}

/*
----------------------------------------------------------------------------
--
-- MTimeInterface
--
----------------------------------------------------------------------------
*/

TTime CBuddycloudLogic::GetTime() {
	return TimeStamp();
}

/*
----------------------------------------------------------------------------
--
-- MXmppEngineObserver
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::XmppStanzaRead(const TDesC8& aMessage) {
	WriteToLog(aMessage);
}

void CBuddycloudLogic::XmppStanzaWritten(const TDesC8& aMessage) {
	WriteToLog(aMessage);
}

void CBuddycloudLogic::XmppStateChanged(TXmppEngineState aState) {
#ifdef _DEBUG
	TBuf8<64> aPrint;
	aPrint.Format(_L8("BL    CBuddycloudLogic::XmppStateChanged: %d"), aState);
	WriteToLog(aPrint);
#endif
	
	if(aState == EXmppOnline) {
		// XMPP state changes to an Online state
		iState = ELogicOnline;
		iLastMotionState = EMotionUnknown;
		iOwnerObserver->StateChanged(iState);
		
		iXmppEngine->GetServerDetails(iSettingXmppServer);
		iXmppEngine->GetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId);
		
		// Sent presence
		SendPresenceL();

		// Set activity status
		iTimerState = ETimeoutConnected;
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_ONLINE);

		if(iConnectionCold) {
			if(iPubsubSubscribedTo) {
				// Collect pubsub subscriptions
				CollectPubsubSubscriptionsL();
				
				// Recollect own /geoloc/current
				CollectUserPubsubNodeL(iOwnItem->GetId(), _L("/geo/current"));
				
				// Re-start Location Engine
				iLocationEngine->TriggerEngine();
			}
			
			// Collect my places
			CollectPlaceSubscriptionsL();
		}
		else {
			// Send presence to pubsub
			SendPresenceToPubsubL(*iLastNodeIdReceived, EXmppPriorityHigh);
			
			// Send queued stanzas
			iXmppEngine->SendQueuedXmppStanzas();
			
			// TODO: Workaround for google
			SendPresenceToPubsubL(KNullDesC8);
						
			// Enable google presence queuing
			if(iSettingXmppServer.Match(_L("*google.com*")) != KErrNotFound) {
				iXmppEngine->SendAndForgetXmppStanza(_L8("<iq type='set'><query xmlns='google:queue'><enable/></query></iq>"), false);
			}
		}
		
		// Set timer
		iTimer->After(3000000);
		
		NotifyNotificationObservers(ENotificationConnectivityChanged);
	}
	else if(aState == EXmppConnecting || aState == EXmppReconnect) {
		// XMPP state changes to (re)connecting
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
		
		if(aState == EXmppReconnect) {
			// Set offline
			iOffsetReceived = false;
			
			if(iState != ELogicConnecting) {
				// Notify observers
				iOwnerObserver->StateChanged(ELogicConnecting);
				NotifyNotificationObservers(ENotificationConnectivityChanged);
				
				// Save state
				iDiscussionManager->CompressDiscussionL(true);
				
				SaveSettingsAndItemsL();
				SavePlaceItemsL();
			}
			
			// XMPP state changes to a Reconnection state
			iState = ELogicConnecting;
		}
	}
	else if(aState == EXmppDisconnected) {
		// XMPP state changes to an Offline state
		iTimer->Stop();
		iTimerState = ETimeoutNone;
		iOffsetReceived = false;
		iPubsubSubscribedTo = false;
				
		if(iSettingAccessPoint) {
			iSettingConnectionMode = 0;
			iSettingConnectionModeId = 0;
			iXmppEngine->SetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId, KNullDesC);
		}
		
		// Stop LocationEngine
		iLocationEngine->StopEngine();

		// Set Activity Status
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_OFFLINE);

		if(iState == ELogicOnline) {			
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

				if(aItem && aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);	
					aRosterItem->SetPresence(EPresenceOffline);
				}
			}
		}

		// Store state
        iDiscussionManager->CompressDiscussionL(true);
        
		SaveSettingsAndItemsL();
		SavePlaceItemsL();

		iState = ELogicOffline;
		iOwnerObserver->StateChanged(iState);
		NotifyNotificationObservers(ENotificationConnectivityChanged);
	}
	else {
		// XMPP state changes to a In Connection state
		iState = ELogicConnecting;
		iOwnerObserver->StateChanged(iState);
		NotifyNotificationObservers(ENotificationConnectivityChanged);
	}
}

void CBuddycloudLogic::XmppUnhandledStanza(const TDesC8& aStanza) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	TPtrC8 aAttributeFrom = aXmlParser->GetStringAttribute(_L8("from"));
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	TPtrC8 aAttributeId = aXmlParser->GetStringAttribute(_L8("id"));

	if(aXmlParser->MoveToElement(_L8("iq"))) {
		if(aXmlParser->MoveToElement(_L8("event")) || aXmlParser->MoveToElement(_L8("pubsub"))) {
			if(aAttributeFrom.Compare(KBuddycloudPubsubServer) == 0) {				
				// Handle pubsub event
				HandlePubsubEventL(aStanza);
					
				if(aAttributeType.Compare(_L8("set")) == 0) {	
					AcknowledgePubsubEventL(aAttributeId);
				}
			}
		}
		else if(aXmlParser->MoveToElement(_L8("time"))) {
			if(aAttributeType.Compare(_L8("result")) == 0) {			
				if(aXmlParser->MoveToElement(_L8("utc"))) {
					if( !iOffsetReceived ) {
						iOffsetReceived = true;
						
						// XEP-0090: Server Time
						CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
		
						// Server time
						TTime aServerTime = aTimeUtilities->DecodeL(aXmlParser->GetStringData());
		
						// Utc Phone time
						TTime aPhoneTime;
						aPhoneTime.UniversalTime();
		
						// Calculate offset
						aPhoneTime.SecondsFrom(aServerTime, iServerOffset);
		
						// Calculate BT beacon sync
						aPhoneTime = TimeStamp();
						TTime aBeaconTime = aPhoneTime;
						aBeaconTime.RoundUpToNextMinute();
						TDateTime aDateTime = aBeaconTime.DateTime();
		
						if(aDateTime.Minute() % 5 > 0) {
							TTimeIntervalMinutes aAddMinutes(5 - (aDateTime.Minute() % 5));
							aBeaconTime += aAddMinutes;
						}
		
						TTimeIntervalSeconds aNextBeaconSeconds;
						aBeaconTime.SecondsFrom(aPhoneTime, aNextBeaconSeconds);
						iLocationEngine->SetBtLaunch(aNextBeaconSeconds.Int());
		
#ifdef _DEBUG
						TBuf8<256> aLog;
						aLog.Format(_L8("LOGIC Server/Phone offset is %d seconds. BT launch in %d seconds."), iServerOffset.Int(), aNextBeaconSeconds.Int());
						WriteToLog(aLog);
#endif
		
						CleanupStack::PopAndDestroy(); // aTimeUtilities
					}
				}
			}
		}
		else if(aXmlParser->MoveToElement(_L8("query"))) {			
			if(aAttributeType.Compare(_L8("set")) == 0) {
				TPtrC8 aAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));

				if(aAttributeXmlns.Compare(_L8("jabber:iq:roster")) == 0) {
					// Roster Push (Update)
					HandleRosterItemPushL(aStanza);
				}			
			}
		}
		else if(aXmlParser->MoveToElement(_L8("place"))) {
			// Population push
			if(aAttributeType.Compare(_L8("set")) == 0) {			
				if(aXmlParser->MoveToElement(_L8("id"))) {
					TInt aPlaceId = aXmlParser->GetIntData();
					
					if(aPlaceId > 0 && aXmlParser->MoveToElement(_L8("population"))) {
						CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aPlaceId));
	
						if(aPlace) {
							aPlace->SetPopulation(aXmlParser->GetIntData());
	
							NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
						}
					}
				}
			}
		}
	}
	else if(aXmlParser->MoveToElement(_L8("message"))) {
		if(aAttributeType.Compare(_L8("error")) != 0 && aAttributeType.Compare(_L8("groupchat")) != 0) {
			// Pass new message to correct handler
			if(aAttributeFrom.Compare(KBuddycloudPubsubServer) == 0) {				
				if(aXmlParser->MoveToElement(_L8("event"))) {
					// TODO: Remove message event handler
					HandlePubsubEventL(aStanza);
				}
				else if(aXmlParser->MoveToElement(_L8("x"))) {
					// Handle subscription request message
					HandlePubsubRequestL(aStanza);
				}				
			}
			else if(!aXmlParser->MoveToElement(_L8("invite"))) {
				HandleIncomingMessageL(aStanza);
			}
		}
	}
	else if(aXmlParser->MoveToElement(_L8("presence"))) {
		if(aAttributeType.Compare(_L8("subscribe")) == 0) {
			if(aAttributeFrom.Compare(KBuddycloudPubsubServer) == 0) {				
				// Accept pubsub subscription
				SendPresenceSubscriptionL(KBuddycloudPubsubServer, _L8("subscribed"));
			}
			else {
				ProcessPresenceSubscriptionL(aStanza);
			}
		}
		else {
			HandleIncomingPresenceL(aStanza);
		}
	}

	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::XmppError(TXmppEngineError aError) {
	if(aError == EXmppBadAuthorization) {
		// Send registration
		HBufC8* aEncUsername = iTextUtilities->UnicodeToUtf8L(iSettingUsername).AllocLC();
		TPtrC8 pEncUsername(aEncUsername->Des());
		TInt aLocate = pEncUsername.Locate('@');
		
		if(aLocate != KErrNotFound) {
			pEncUsername.Set(pEncUsername.Left(aLocate));
		}
		
		HBufC8* aEncPassword = iTextUtilities->UnicodeToUtf8L(iSettingPassword).AllocLC();
		TPtrC8 pEncPassword(aEncPassword->Des());
		
		HBufC8* aEncFullName = iTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtrC8 pEncFirstName(aEncFullName->Des());
		TPtrC8 pEncLastName;
		
		if((aLocate = pEncFirstName.Locate(' ')) != KErrNotFound) {
			pEncLastName.Set(pEncFirstName.Mid(aLocate + 1));
			pEncFirstName.Set(pEncFirstName.Left(aLocate));
		}
		
		_LIT8(KFirstNameElement, "<firstname></firstname>");
		_LIT8(KLastNameElement, "<lastname></lastname>");
		
		_LIT8(KRegisterStanza, "<iq type='set' id='%02d:%02d'><query xmlns='jabber:iq:register'><username></username><password></password><email></email></query></iq>\r\n");
		HBufC8* aRegisterStanza = HBufC8::NewLC(KRegisterStanza().Length() + pEncUsername.Length() + pEncPassword.Length() + KFirstNameElement().Length() + pEncFirstName.Length() + KLastNameElement().Length() + pEncLastName.Length());
		TPtr8 pRegisterStanza(aRegisterStanza->Des());
		pRegisterStanza.AppendFormat(KRegisterStanza, EXmppIdRegistration, GetNewIdStamp());
		
		// First name
		if(pEncFirstName.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KFirstNameElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 12, pEncFirstName);
		}
		
		// Last name
		if(pEncLastName.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KLastNameElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 11, pEncLastName);
		}
		
		pRegisterStanza.Insert(91, pEncPassword);
		pRegisterStanza.Insert(70, pEncUsername);

		iXmppEngine->SendAndAcknowledgeXmppStanza(pRegisterStanza, this, false, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(4); // aRegisterStanza, aEncFullName, aEncPassword, aEncUsername
	}
	else if(aError == EXmppServerUnresolved) {	
		// Server could not be resolved
		HBufC* aMessageTitle = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SERVERRESOLVEERROR_TITLE);
		HBufC* aMessageText = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SERVERRESOLVEERROR_TEXT);		
		
		CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
		aDialog->PrepareLC(R_AUTHERROR_DIALOG);
		aDialog->SetHeaderTextL(*aMessageTitle);
		aDialog->SetMessageTextL(*aMessageText);
		aDialog->RunLD();
		CleanupStack::PopAndDestroy(2); // aMessageText, aMessageTitle
		
		iXmppEngine->Disconnect();
		
		NotifyNotificationObservers(ENotificationServerResolveFailed);
	}
	else if(aError != EXmppAlreadyConnected) {	
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_OFFLINE);
	}
}

void CBuddycloudLogic::XmppShutdownComplete() {
	if(!iXmppEngineReady) {
		iXmppEngineReady = true;
	
		if(iLocationEngineReady && iXmppEngineReady) {
			iOwnerObserver->ShutdownComplete();
		}
	}
}

void CBuddycloudLogic::XmppDebug(const TDesC8& aMessage) {
	WriteToLog(aMessage);
}

/*
----------------------------------------------------------------------------
--
-- MXmppRosterObserver
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::XmppRosterL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::XmppRosterL"));
#endif
	
	CBuddycloudListStore* aRosterItemStore = CBuddycloudListStore::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	// Configure users item
	HandleUserItemConfigurationL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("to"))));
		
	// Collect roster items	
	while(aXmlParser->MoveToElement(_L8("item"))) {
		TPtrC8 aJid(aXmlParser->GetStringAttribute(_L8("jid")));
		TPresenceSubscription aSubscription = CXmppEnumerationConverter::PresenceSubscription(aXmlParser->GetStringAttribute(_L8("subscription")));

		if(aJid.Compare(KBuddycloudPubsubServer) == 0) {	
			// Handle pubsub item
			if(aSubscription == EPresenceSubscriptionBoth) {			
				// Pubsub is subscribed to
				iPubsubSubscribedTo = true;
			}
		}
		else {
			CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
			aRosterItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aJid));
			aRosterItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("name"))));	
			aRosterItem->SetSubscription(aSubscription);
			
			aRosterItemStore->AddItem(aRosterItem);
			CleanupStack::Pop(); // aRosterItem
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
	
	if(!iPubsubSubscribedTo) {
		// Subscribe to pubsub server	
		AddRosterManagementItemL(KBuddycloudPubsubServer);
		SendPresenceSubscriptionL(KBuddycloudPubsubServer, _L8("subscribe"));
	}
	
	// Synchronize local roster to received roster
	for(TInt i = (iFollowingList->Count() - 1); i >= 1; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem) {
			TInt aItemId = aItem->GetItemId();

			if(aItem->GetItemType() == EItemNotice) {
				// Remove notice item (will be re-added)
				iFollowingList->DeleteItemByIndex(i);					
				
				NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);				
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aLocalItem = static_cast <CFollowingRosterItem*> (aItem);
				TBool aItemFound = false;
				
				for(TInt x = (aRosterItemStore->Count() - 1); x >= 0; x--) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aRosterItemStore->GetItemByIndex(x));
					
					if(aRosterItem->GetId().CompareF(aLocalItem->GetId()) == 0) {
						// Handle roster item						
						if(aRosterItem->GetSubscription() > EPresenceSubscriptionNone) {						
							// Update
							aLocalItem->SetSubscription(aRosterItem->GetSubscription());
							
							aItemFound = true;
						}
						
						aRosterItemStore->DeleteItemByIndex(x);
						
						break;
					}
				}
				
				if(!aItemFound) {
					// Remove no longer subscribed presence		
					UnfollowChannelL(aItemId);
					
					iDiscussionManager->DeleteDiscussionL(aLocalItem->GetId());
					iFollowingList->DeleteItemByIndex(i);		
					
					NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);
				}	
			}
		}
	}
	
	// Add new roster items
	while(aRosterItemStore->Count() > 0) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aRosterItemStore->GetItemByIndex(0));
		
		if(aRosterItem->GetSubscription() > EPresenceSubscriptionNone) {
			// Add item to following list
			aRosterItem->SetItemId(iNextItemId++);			
				
			if(aRosterItem->GetTitle().Length() == 0) {
				aRosterItem->SetTitleL(aRosterItem->GetId());
				
				TInt aLocate = aRosterItem->GetId().Locate('@');

				if(aLocate != KErrNotFound) {
					aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
					aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
				}
			}
			else {
				aRosterItem->SetSubTitleL(aRosterItem->GetId());
			}
			
			// Add to discussion manager
			CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());
			aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
			
			aRosterItem->SetUnreadData(aDiscussion);
	
			iFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);		
			aRosterItemStore->RemoveItemByIndex(0);	
		}
		else {	
			aRosterItemStore->DeleteItemByIndex(0);	
		}
	}
	
	iRosterSynchronized = true;
	iSettingsSaveNeeded = true;

	CleanupStack::PopAndDestroy(); // aRosterItemStore
	
	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

void CBuddycloudLogic::HandleUserItemConfigurationL(const TDesC& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleUserItemConfigurationL"));
#endif

	if(iOwnItem) {
		iOwnItem->SetOwnItem(false);
	}
	
	// Remove resource from jid
	TPtrC pJid(aJid);
	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
	}
	
	CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (iFollowingList->GetItemById(IsSubscribedTo(pJid, EItemRoster)));
	
	if(aRosterItem == NULL) {
		aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetItemId(iNextItemId++);
		aRosterItem->SetIdL(pJid);
		aRosterItem->SetTitleL(iSettingFullName);
			
		if(aRosterItem->GetTitle().Length() == 0) {
			aRosterItem->SetTitleL(aRosterItem->GetId());
			
			TInt aLocate = aRosterItem->GetId().Locate('@');

			if(aLocate != KErrNotFound) {
				aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
				aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
			}
		}
		else {
			aRosterItem->SetSubTitleL(aRosterItem->GetId());
		}
	
		CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(pJid);
		aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
		
		aRosterItem->SetUnreadData(aDiscussion);
		
		iFollowingList->InsertItem(0, aRosterItem);
		CleanupStack::Pop();
	}
	
	aRosterItem->SetOwnItem(true);
	aRosterItem->SetSubscription(EPresenceSubscriptionBoth);
	
	iOwnItem = aRosterItem;	
	
	if(iStatusObserver) {
		iStatusObserver->JumpToItem(aRosterItem->GetItemId());
	}
}

void CBuddycloudLogic::HandleRosterItemPushL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleRosterItemPushL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	if(aXmlParser->MoveToElement(_L8("item"))) {
		TPtrC8 aJid(aXmlParser->GetStringAttribute(_L8("jid")));		
		TPresenceSubscription aSubscription = CXmppEnumerationConverter::PresenceSubscription(aXmlParser->GetStringAttribute(_L8("subscription")));
		
		if(aJid.Compare(KBuddycloudPubsubServer) == 0) {					
			if(!iPubsubSubscribedTo && aSubscription == EPresenceSubscriptionBoth) {			
				// Collect pubsub subscriptions
				CollectPubsubSubscriptionsL();
					
				// Trigger location engine
				iLocationEngine->TriggerEngine();
				
				// Pubsub is subscribed to
				iPubsubSubscribedTo = true;
			}
		}
		else {
			TPtrC aEncJid(iTextUtilities->Utf8ToUnicodeL(aJid));
			TInt aItemId = KErrNotFound;
				
			if((aSubscription < EPresenceSubscriptionNone || aSubscription > EPresenceSubscriptionTo) && 
					(aItemId = IsSubscribedTo(aEncJid, EItemNotice)) > 0) {
				
				// Remove notice item
				iFollowingList->DeleteItemById(aItemId);					
				
				NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);	
			}
			
			// Manage roster item
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (iFollowingList->GetItemById(IsSubscribedTo(aEncJid, EItemRoster)));
			
			if(aRosterItem) {
				aItemId = aRosterItem->GetItemId();
				
				// Update items pubsub affiliation & subscription
				if(iPubsubSubscribedTo) {
					if(aRosterItem->GetSubscription() < EPresenceSubscriptionFrom && 
							aSubscription >= EPresenceSubscriptionFrom) {
						
						// Add user to my node subscribers
						SetPubsubNodeAffiliationL(aRosterItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubAffiliationPublisher);							
					}
					else if(aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom && 
							aSubscription < EPresenceSubscriptionFrom) {
						
						// Remove user from my node subscribers
						SetPubsubNodeSubscriptionL(aRosterItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubSubscriptionNone);
						
						if(aRosterItem->GetPubsubAffiliation() != EPubsubAffiliationNone && 
								aRosterItem->GetPubsubSubscription() > EPubsubSubscriptionNone) {
							
							// Remove me from users node subscribers
							SetPubsubNodeSubscriptionL(iOwnItem->GetId(), aRosterItem->GetId(EIdChannel), EPubsubSubscriptionNone);
						}
					}
				}
					
				if(aSubscription > EPresenceSubscriptionNone) {						
					// Update item
					aRosterItem->SetSubscription(aSubscription);
				}
				else {
					// Remove (no subscription)
					UnfollowChannelL(aItemId);
					
					iDiscussionManager->DeleteDiscussionL(aRosterItem->GetId());
					iFollowingList->DeleteItemById(aItemId);
					
					NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);
				}	
			}
			else if(aSubscription > EPresenceSubscriptionNone) {
				// Add item to following list
				aRosterItem = CFollowingRosterItem::NewLC();
				aRosterItem->SetIdL(aEncJid);
				aRosterItem->SetItemId(iNextItemId++);	
				aRosterItem->SetSubscription(aSubscription);
				aRosterItem->SetTitleL(aRosterItem->GetId());
					
				TInt aLocate = aRosterItem->GetId().Locate('@');

				if(aLocate != KErrNotFound) {
					aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
					aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
				}
				
				if(iPubsubSubscribedTo && aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom) {
					// Add user to my node subscribers
					SetPubsubNodeAffiliationL(aRosterItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubAffiliationPublisher);
				}
				
				// Add to discussion manager
				CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());
				aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
				
				aRosterItem->SetUnreadData(aDiscussion);
		
				iFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);		
				CleanupStack::Pop(); // aRosterItem
				
				NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aRosterItem->GetItemId());
			}
		}	
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

/*
----------------------------------------------------------------------------
--
-- MXmppStanzaObserver
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::XmppStanzaAcknowledgedL"));
#endif
	
	if(aId.Locate(':') != KErrNotFound) {
		TLex8 aIdLex(aId.Left(aId.Locate(':')));
		TInt aIdEnum = KErrNotFound;
		
		if(aIdLex.Val(aIdEnum) != KErrNotFound) {
			CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
			TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
			
			if(aAttributeType.Compare(_L8("result")) == 0) {
				if(aIdEnum == EXmppIdRegistration) {
					// Registration success
					iXmppEngine->SetAccountDetailsL(iSettingUsername, iSettingPassword);
					iXmppEngine->HandlePlainAuthorizationL();
					
					// Save settings
					SaveSettingsAndItemsL();
				}
				else if(aIdEnum == EXmppIdSetCredentials || aIdEnum == EXmppIdChangeChannelAffiliation) {
					// Communities update or Affiliation change success
					ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_UPDATESUCCESS);
				}
				else if(aIdEnum == EXmppIdGetNearbyPlaces) {
					// Delete old nearby places
					for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
						delete iNearbyPlaces[i];
					}
					
					iNearbyPlaces.Reset();

					// Get new nearby places
					while(aXmlParser->MoveToElement(_L8("item"))) {
						TPtrC8 pAttributeId = aXmlParser->GetStringAttribute(_L8("id"));
						TPtrC8 pAttributeVar = aXmlParser->GetStringAttribute(_L8("var"));
						TInt aIntValue = pAttributeId.LocateReverse('/');
						
						if(aIntValue != KErrNotFound && pAttributeVar.Compare(_L8("place")) == 0) {
							CXmlParser* aItemXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
							
							// Convert place id
							TLex8 aLexValue(pAttributeId.Mid(aIntValue + 1));
							if(aLexValue.Val(aIntValue) != KErrNone) {
								aIntValue = 0;
							}
							
							if(aIntValue > 0 && aItemXmlParser->MoveToElement(_L8("name"))) {
								// Add Place to Nearby List
								CBuddycloudNearbyPlace* aNewPlace = new (ELeave) CBuddycloudNearbyPlace();
								CleanupStack::PushL(aNewPlace);

								aNewPlace->SetPlaceId(aIntValue);
								aNewPlace->SetPlaceNameL(iTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData()));
								
								iNearbyPlaces.Append(aNewPlace);
								CleanupStack::Pop();
							}
							
							CleanupStack::PopAndDestroy(); // aItemXmlParser
						}
					}
				}
				else if(aIdEnum >= EXmppIdGetPlaceSubscriptions && aIdEnum <= EXmppIdGetPlaceDetails) {
					// Handle places query result
					HandlePlaceQueryResultL(aStanza, (TBuddycloudXmppIdEnumerations)aIdEnum);
				}
				else if(aIdEnum == EXmppIdGetPubsubSubscriptions || aIdEnum == EXmppIdGetUsersPubsubNodeAffiliations) {
					// Handle pubsub subscriptions or users pubsub node affiliations
					TInt aListSize = iGenericList->Count();
					TPtrC8 aLastItem;
					
					if(aIdEnum == EXmppIdGetPubsubSubscriptions) {
						// Handle subscription items
						while(aXmlParser->MoveToElement(_L8("subscription"))) {
							aLastItem.Set(aXmlParser->GetStringAttribute(_L8("node")));
							
							CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
							aChannelItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aLastItem));
							aChannelItem->SetPubsubSubscription(CXmppEnumerationConverter::PubsubSubscription(aXmlParser->GetStringAttribute(_L8("subscription"))));
							aChannelItem->SetPubsubAffiliation(CXmppEnumerationConverter::PubsubAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation"))));
							
							iGenericList->AddItem(aChannelItem);
							CleanupStack::Pop(); // aChannelItem
						}
					}
					else {
						// Handle affiliation items
						while(aXmlParser->MoveToElement(_L8("affiliation"))) {
							aLastItem.Set(aXmlParser->GetStringAttribute(_L8("jid")));
							
							CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
							aRosterItem->SetIdL(iTextUtilities->Utf8ToUnicodeL(aLastItem));
							
							iGenericList->AddItem(aRosterItem);
							CleanupStack::Pop(); // aRosterItem
						}
					}
					
					// Process result set management data
					TInt aTotalCount = iGenericList->Count();
					
					if(aXmlParser->MoveToElement(_L8("set")) && aXmlParser->MoveToElement(_L8("count"))) {
						// Get total count
						aTotalCount = aXmlParser->GetIntData();
					}
					
					if(aIdEnum == EXmppIdGetPubsubSubscriptions) {
						// Handle pubsub subscriptions
						if(aListSize < iGenericList->Count() && iGenericList->Count() < aTotalCount) {	
							// Request more items	
							CollectPubsubSubscriptionsL(aLastItem);
						}
						else {
							// Begin processing pubsub subscriptions
							ProcessPubsubSubscriptionsL();
						}
					}
					else {
						// Handle users pubsub node affiliations
						if(aListSize < iGenericList->Count() && iGenericList->Count() < aTotalCount) {	
							// Request more items	
							CollectUsersPubsubNodeAffiliationsL(aLastItem);
						}
						else {
							// Begin processing node affiliation data
							ProcessUsersPubsubNodeAffiliationsL();
						}
					}
				}
				else if(aIdEnum == EXmppIdPublishMood || aIdEnum == EXmppIdPublishFuturePlace) {
					// Publish mood or future place result
					SetActivityStatus(KNullDesC);
				}
				else if(aIdEnum == EXmppIdGetChannelMetadata) {
					// Handle channel metadata result
					ProcessChannelMetadataL(aStanza);
				}
				else if(aIdEnum == EXmppIdRequestMediaPost) {
					// Media post request
					if(aXmlParser->MoveToElement(_L8("uploaduri"))) {
						TPtrC8 aDataUploaduri(aXmlParser->GetStringData());
						TLex8 aItemLex(aId.Mid(aId.LocateReverse(':') + 1));
						TInt aItemId = KErrNotFound;
						
						if(aItemLex.Val(aItemId) == KErrNone) {
							CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
							
							if(aItem && aItem->GetItemType() >= EItemRoster) {						
								// Create discussion entry
								CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
								aAtomEntry->SetIdL(aDataUploaduri);
								aAtomEntry->SetPrivate(true);
								aAtomEntry->SetDirectReply(true);
								aAtomEntry->SetIconId(KIconChannel);							
								aAtomEntry->SetPublishTime(TimeStamp());	
								aAtomEntry->SetAuthorNameL(_L("Media Uploader"));
									
								HBufC* aResourceText = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_MEDIAUPLOADREADY);					
								TPtrC aEncUploaduri(iTextUtilities->Utf8ToUnicodeL(aDataUploaduri));
								
								HBufC* aMessageText = HBufC::NewLC(aResourceText->Des().Length() + aEncUploaduri.Length());
								TPtr pMessageText(aMessageText->Des());
								pMessageText.Append(*aResourceText);
								pMessageText.Replace(pMessageText.Find(_L("$LINK")), 5, aEncUploaduri);							
								
								aAtomEntry->SetContentL(pMessageText);
								CleanupStack::PopAndDestroy(2); // aMessageText, aResourceText
			
								// Add to discussion
								CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aItem->GetId());
								
								if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {									
									NotifyNotificationObservers(ENotificationNotifiedMessageEvent, aItem->GetItemId());
								}
							}
						}
					}
				}
			}
			else if(aAttributeType.Compare(_L8("error")) == 0) {
				if(aIdEnum == EXmppIdRegistration) {
					// Registration failed			
					CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
					dlg->ExecuteLD(R_AUTHERROR_DIALOG);
					
					iXmppEngine->Disconnect();
					
					NotifyNotificationObservers(ENotificationAuthenticationFailed);
				}
				else if(aIdEnum == EXmppIdSetCredentials) {
					// Communities update failed
					HBufC* aMessage = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CREDENTIALS_FAIL);
					CAknErrorNote* aDialog = new (ELeave) CAknErrorNote();	
					aDialog->SetTimeout(CAknNoteDialog::ENoTimeout);
					aDialog->ExecuteLD(*aMessage);
					CleanupStack::PopAndDestroy(); // aMessage	
				}
				else if(aIdEnum == EXmppIdChangeChannelAffiliation) {
					// Affiliation change failed
					HBufC* aMessage = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_UPDATEFAILED);
					CAknErrorNote* aDialog = new (ELeave) CAknErrorNote();	
					aDialog->ExecuteLD(*aMessage);
					CleanupStack::PopAndDestroy(); // aMessage	
				}
				else if(aIdEnum == EXmppIdCreatePlace || aIdEnum == EXmppIdSetCurrentPlace || aIdEnum == EXmppEditPlaceDetails) {
					// Handle place query error
					SetActivityStatus(KNullDesC);
					
					if(aXmlParser->MoveToElement(_L8("text"))) {
						HBufC* aMessageTitle = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_UPDATEFAILED);
						
						CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
						aDialog->PrepareLC(R_AUTHERROR_DIALOG);
						aDialog->SetHeaderTextL(*aMessageTitle);
						aDialog->SetMessageTextL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
						aDialog->RunLD();
						
						CleanupStack::PopAndDestroy(); // aMessageTitle
					}
				}
				else if(aIdEnum == EXmppIdSearchForPlace) {
					// Handle place search error
					ShowInformationDialogL(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);
				}
				else if(aIdEnum == EXmppIdGetPlaceDetails) {
					// Handle place details error
					HandlePlaceQueryResultL(aStanza, (TBuddycloudXmppIdEnumerations)aIdEnum);
				}
				else if(aIdEnum == EXmppIdPublishMood || aIdEnum == EXmppIdPublishFuturePlace) {
					// Set mood or future place error
					SetActivityStatus(KNullDesC);
				}
				else if(aIdEnum == EXmppIdPublishChannelPost) {
					// Publishing error
					if(aXmlParser->MoveToElement(_L8("error"))) {
						if(aXmlParser->MoveToElement(_L8("text"))) {
							TLex8 aItemLex(aId.Mid(aId.LocateReverse(':') + 1));
							TInt aItemId = KErrNotFound;
							
							if(aItemLex.Val(aItemId) == KErrNone) {
								CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
								
								if(aItem && aItem->GetItemType() >= EItemRoster) {	
									CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
									aAtomEntry->SetPrivate(true);
									aAtomEntry->SetDirectReply(true);
									aAtomEntry->SetIconId(KIconChannel);							
									aAtomEntry->SetPublishTime(TimeStamp());	
									aAtomEntry->SetAuthorNameL(_L("Broadcaster"));							
									aAtomEntry->SetContentL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
	
									// Add to discussion
									CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aItem->GetId());
									
									if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {									
										NotifyNotificationObservers(ENotificationNotifiedMessageEvent, aItem->GetItemId());
									}
								}
							}
						}
					}
				}
			}
			
			CleanupStack::PopAndDestroy(); // aXmlParser
		}
	}
}

void CBuddycloudLogic::HandleIncomingPresenceL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingPresenceL"));
#endif

	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	HBufC* aFrom = iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("from"))).AllocLC();
	TPtrC aJid(aFrom->Des());
	TPtrC aResource(aFrom->Des());

	TInt aLocate = aJid.Locate('/');
	if(aLocate != KErrNotFound) {
		aJid.Set(aJid.Left(aLocate));
		aResource.Set(aResource.Mid(aLocate + 1));
	}
	
	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(aRosterItem->GetId().CompareF(aJid) == 0) {
				TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
				
				if(aAttributeType.Length() == 0) {
					// Users presence changes
					if(aRosterItem->GetPresence() != EPresenceOnline) {
						aRosterItem->SetPresence(EPresenceOnline);
					}
					
					// Presence has nick
					if(aXmlParser->MoveToElement(_L8("nick"))) {
						aRosterItem->SetTitleL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
						
						if(aRosterItem->GetTitle().Length() == 0) {
							aRosterItem->SetTitleL(aRosterItem->GetId());
							aRosterItem->SetSubTitleL(KNullDesC);
							
							if((aLocate = aRosterItem->GetId().Locate('@')) != KErrNotFound) {
								aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
								aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
							}
						}
						else {
							aRosterItem->SetSubTitleL(aRosterItem->GetId());
						}
					}
					
					// TODO: Handle possible avatar hash
				}
				else if(aAttributeType.Compare(_L8("unavailable")) == 0) {
					// User goes offline
					if(aXmlParser->MoveToElement(_L8("status")) && 
							aXmlParser->GetStringData().Compare(_L8("Replaced by new connection")) != 0) {
					
						// User is not 'replaced by new connection'
						aRosterItem->SetPresence(EPresenceOffline);
					}
				}
				
				break;
			}
		}
	}
	
	CleanupStack::PopAndDestroy(2); // aFrom, aXmlParser
}

void CBuddycloudLogic::ProcessPresenceSubscriptionL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPresenceSubscriptionL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);	
	TPtrC8 aJid(aXmlParser->GetStringAttribute(_L8("from")));
	TPtrC aEncJid(iTextUtilities->Utf8ToUnicodeL(aJid));

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(IsSubscribedTo(aEncJid, EItemNotice)));
	
	if(aItem == NULL) {
		TPtrC8 aName, aLocation;
		TInt aLocate = KErrNotFound;
		
		CFollowingItem* aNoticeItem = CFollowingItem::NewLC();
		aNoticeItem->SetItemId(iNextItemId++);
		aNoticeItem->SetIdL(aEncJid, EIdRoster);
		aNoticeItem->SetSubTitleL(aEncJid);
		
		// Get name
		if(aXmlParser->MoveToElement(_L8("nick"))) {
			aName.Set(aXmlParser->GetStringData());
			aXmlParser->ResetElementPointer();
		}
		else if((aLocate = aJid.Locate('@')) != KErrNotFound) {
			aName.Set(aJid.Left(aLocate));
		}
		else {
			aName.Set(aJid);
		}
		
		// Get broad location		
		if(aXmlParser->MoveToElement(_L8("geoloc"))) {
			if(aXmlParser->MoveToElement(_L8("text"))) {
				aLocation.Set(aXmlParser->GetStringData());
			}
		}
		
		// Prepare user info
		HBufC* aEncName = iTextUtilities->Utf8ToUnicodeL(aName).AllocLC();
		HBufC* aEncLocation = iTextUtilities->Utf8ToUnicodeL(aLocation).AllocLC();
		TPtr pEncLocation(aEncLocation->Des());
	
		HBufC* aUserInfo = HBufC::NewLC(aEncName->Des().Length() + 5 + pEncLocation.Length());
		TPtr pUserInfo(aUserInfo->Des());
		pUserInfo.Append(*aEncName);
		
		if(pEncLocation.Length() > 0) {
			pUserInfo.Append(_L(" ~ "));
			pUserInfo.Append(pEncLocation);
			pUserInfo.Append(_L(" :"));
		}		
		
		// Prepare item title & description
		HBufC* aTitle = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_FOLLOWINGREQUEST_TITLE);
		HBufC* aDescriptionResource = CCoeEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_FOLLOWINGREQUEST_TEXT);
		HBufC* aDescription = HBufC::NewLC(aDescriptionResource->Des().Length() + pUserInfo.Length());
		TPtr pDescription(aDescription->Des());
		pDescription.Append(*aDescriptionResource);
		
		if((aLocate = pDescription.Find(_L("$USER"))) != KErrNotFound) {
			pDescription.Replace(aLocate, 5, pUserInfo);
		}
		
		aNoticeItem->SetTitleL(*aTitle);
		aNoticeItem->SetDescriptionL(*aDescription);
		
		iFollowingList->InsertItem(GetBubbleToPosition(aNoticeItem), aNoticeItem);
		
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
		NotifyNotificationObservers(ENotificationNotifiedMessageEvent);
		CleanupStack::PopAndDestroy(6); // aDescription, aDescriptionResource, aTitle, aUserInfo, aEncLocation, aEncName
		CleanupStack::Pop(); // aNoticeItem
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::HandleIncomingMessageL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingMessageL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	TPtrC8 aRawJid(aXmlParser->GetStringAttribute(_L8("from")));
	TPtrC aJid(iTextUtilities->Utf8ToUnicodeL(aRawJid));
	TInt aLocate = aJid.Locate('/');
	
	if(aLocate != KErrNotFound) {
		aJid.Set(aJid.Left(aLocate));
	}
		
	// Get discussion
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aJid);
	CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (iFollowingList->GetItemById(aDiscussion->GetItemId()));
	
	if(aRosterItem == NULL && (!iSettingMessageBlocking || aJid.Locate('@') == KErrNotFound)) {
		// Add sender to roster
		aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetItemId(iNextItemId++);
		aRosterItem->SetIdL(aJid);
		aRosterItem->SetTitleL(aJid);
		aRosterItem->SetUnreadData(aDiscussion);
		
		TInt aLocate = aRosterItem->GetId().Locate('@');
		
		if(aLocate != KErrNotFound) {
			aRosterItem->SetTitleL(aRosterItem->GetId().Left(aLocate));
			aRosterItem->SetSubTitleL(aRosterItem->GetId().Mid(aLocate));
		}
		
		aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());

		iFollowingList->AddItem(aRosterItem);
		CleanupStack::Pop();			
	}
	
	if(aRosterItem) {	
		// Create atom entry
		CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
		aAtomEntry->SetPublishTime(TimeStamp());
        aAtomEntry->SetAuthorAffiliation(EPubsubAffiliationOwner);
		aAtomEntry->SetAuthorNameL(aJid);
		aAtomEntry->SetAuthorJidL(aJid);
		aAtomEntry->SetPrivate(true);

		TBuf8<32> aReferenceId;
		
		do {
			TPtrC8 aElementName = aXmlParser->GetElementName();
			
			if(aElementName.Compare(_L8("body")) == 0 && aAtomEntry->GetContent().Length() == 0) {
				// Get first body data only, avoid recollecting html data
				aAtomEntry->SetContentL(iTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
			}
			else if(aElementName.Compare(_L8("thread")) == 0) {
				aReferenceId.Copy(aXmlParser->GetStringAttribute(_L8("parent")).Left(32));
				
				aAtomEntry->SetIdL(aXmlParser->GetStringData());
			}
			else if(aElementName.Compare(_L8("x")) == 0 || aElementName.Compare(_L8("delay")) == 0) {
				TPtrC8 aAttributeStamp = aXmlParser->GetStringAttribute(_L8("stamp"));
				
				if(aAttributeStamp.Length() > 0) {
					aAtomEntry->SetPublishTime(CTimeUtilities::DecodeL(aAttributeStamp));
				}
			}
		} while(aXmlParser->MoveToNextElement());
		
		// Set geoloc data
		CGeolocData* aGeoloc = aRosterItem->GetGeolocItem(EGeolocItemCurrent);
		
		aAtomEntry->GetLocation()->SetStringL(EGeolocText, aGeoloc->GetString(EGeolocText));
		aAtomEntry->GetLocation()->SetStringL(EGeolocLocality, aGeoloc->GetString(EGeolocLocality));
		aAtomEntry->GetLocation()->SetStringL(EGeolocCountry, aGeoloc->GetString(EGeolocCountry));
		
		// Add to discussion
		if(aDiscussion->AddEntryOrCommentLD(aAtomEntry, aReferenceId)) {
			if(!aRosterItem->OwnItem()) {
				// Bubble item
				iFollowingList->RemoveItemByIndex(iFollowingList->GetIndexById(aRosterItem->GetItemId()));
				iFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);
				
				NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aRosterItem->GetItemId());
			}
			
			// Notify arrival
			if(aDiscussion->Notify() && !CPhoneUtilities::InCall()) {
				NotifyNotificationObservers(ENotificationNotifiedMessageEvent, aRosterItem->GetItemId());
				
				iNotificationEngine->NotifyL(ESettingItemPrivateMessageTone);
			}
		}
	}
	else {
		// Send error back
		_LIT8(KErrorMessageStanza, "<message to='' type='error'><error type='cancel'><service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/></error></message>");
		HBufC8* aErrorMessage = HBufC8::NewLC(KErrorMessageStanza().Length() + aRawJid.Length());
		TPtr8 pErrorMessage(aErrorMessage->Des());
		pErrorMessage.Append(KErrorMessageStanza);
		pErrorMessage.Insert(13, aRawJid);
		
		iXmppEngine->SendAndForgetXmppStanza(pErrorMessage, false);
		CleanupStack::PopAndDestroy(); // aErrorMessage
	}	
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}
