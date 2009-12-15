/*
============================================================================
 Name        : 	BuddycloudLogic.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
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
#include "SearchResultMessageQueryDialog.h"
#include "TextUtilities.h"
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

	if(iContactStreamer)
		delete iContactStreamer;

	if(iContactSearcher)
		delete iContactSearcher;

	// Contact Database
	if(iContactDbNotifier)
		delete iContactDbNotifier;

	if(iContactDatabase)
		delete iContactDatabase;

	// Avatars & text
	if(iAvatarRepository)
		delete iAvatarRepository;

	if(iServerActivityText)
		delete iServerActivityText;
	
	if(iBroadLocationText)
		delete iBroadLocationText;
	
	if(iLastNodeIdReceived) 
		delete iLastNodeIdReceived;

	if(iContactFilter)
		delete iContactFilter;

	// Engines
	if(iTelephonyEngine)
		delete iTelephonyEngine;
	
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
	
	// Nearby places
	for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
		delete iNearbyPlaces[i];
	}
	
	iNearbyPlaces.Close();

	// Log
	if(iLog.Handle()) {
		iLog.CloseLog();
	}

	iLog.Close();
	
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
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CAvatarRepository"));
#endif

	iAvatarRepository = CAvatarRepository::NewL(this);

	iNextItemId = 1;
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CContactDatabase"));
#endif

	iContactDatabase = CContactDatabase::OpenL();
	iContactDatabase->SetDbViewContactType(KUidContactCard);
	iContactDbNotifier = CContactChangeNotifier::NewL(*iContactDatabase, this);
	iContactNameOrder = ENameOrderFirstLast;
	iContactsLoaded = false;

	// Preferences defaults
	iSettingPreferredLanguage = -1;
	iSettingNotifyChannelsModerating = 1;
	iSettingNotifyReplyTo = true;
	iSettingShowContacts = false;
	
	// Positioning defaults
	iSettingCellOn = true;
	iSettingWlanOn = true;
	iSettingGpsOn = false;
	iSettingBtOn = false;
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CBuddycloudFollowingStore"));
#endif

	iFollowingList = CBuddycloudListStore::NewL();

 	iContactFilter = HBufC::NewL(0);
 	iFilteringContacts = false;
	
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
 	
	NotificationSettingChanged(ESettingItemPrivateMessageTone);
	NotificationSettingChanged(ESettingItemChannelPostTone);
	NotificationSettingChanged(ESettingItemDirectReplyTone);
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CTelephonyEngine"));
#endif

	iTelephonyEngine = CTelephonyEngine::NewL(this);
}

void CBuddycloudLogic::Startup() {	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CLocationEngine"));
#endif

	// Initialize Location Engine
	iLocationEngineReady = false;
	iLocationEngine = CLocationEngine::NewL(this);
	iLocationEngine->SetXmppWriteInterface((MXmppWriteInterface*)iXmppEngine);
	iLocationEngine->SetTimeInterface(this);
	
	HBufC* aLangCode = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_LANGCODE);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	iLocationEngine->SetLanguageCodeL(aTextUtilities->UnicodeToUtf8L(*aLangCode));
	CleanupStack::PopAndDestroy(2); // aLangCode, aTextUtilities
	
	iLocationEngine->SetCellActive(iSettingCellOn);
	iLocationEngine->SetWlanActive(iSettingWlanOn);
	iLocationEngine->SetBtActive(iSettingBtOn);
	iLocationEngine->SetGpsActive(iSettingGpsOn);
	
	// Load places & addressbook
	LoadPlaceItems();
	
	if(iSettingShowContacts) {
		LoadAddressBookContacts();
	}
	
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
		// Get login details
		CAknMultiLineDataQueryDialog* dlg = CAknMultiLineDataQueryDialog::NewL(iSettingUsername, iSettingPassword);

		if(dlg->ExecuteLD(R_CREDENTIALS_DIALOG) != 0) {
			ConnectToXmppServerL();
		}
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

void CBuddycloudLogic::GetConnectionStatistics(TInt& aDataSent, TInt& aDataReceived) {
	iXmppEngine->GetConnectionStatistics(aDataSent, aDataReceived);
}

void CBuddycloudLogic::ConnectToXmppServerL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ConnectToXmppServerL"));
#endif

	iConnectionCold = true;

	SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
	
	switch(iSettingServerId) {
		case 1:
			iXmppEngine->SetServerDetailsL(_L("beta.buddycloud.com"), 5222);
			break;
		case 2:
			iXmppEngine->SetServerDetailsL(_L("talk.google.com"), 5222);
			break;
		default:
			iXmppEngine->SetServerDetailsL(_L("cirrus.buddycloud.com"), 443);
			break;
	}
	
	iXmppEngine->SetAuthorizationDetailsL(iSettingUsername, iSettingPassword);
	iXmppEngine->ConnectL(iConnectionCold);
}

void CBuddycloudLogic::SendPresenceL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	// Nick
	TPtrC8 aEncText(aTextUtilities->UnicodeToUtf8L(iSettingFullName));
	_LIT8(KNickContainer, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
	
	HBufC8* aEncNick = HBufC8::NewLC(KNickContainer().Length() + aEncText.Length());
	TPtr8 pEncNick(aEncNick->Des());
	
	if(aEncText.Length() > 0) {
		pEncNick.Append(KNickContainer);
		pEncNick.Insert(46, aEncText);
	}
	
	// Broad location
	aEncText.Set(aTextUtilities->UnicodeToUtf8L(*iBroadLocationText));
	
	_LIT8(KPresenceStanza, "<presence><priority>10</priority><status></status><c xmlns='http://jabber.org/protocol/caps' node='http://buddycloud.com/caps' ver='s60-0.5.04'/></presence>\r\n");
	HBufC8* aPresenceStanza = HBufC8::NewLC(KPresenceStanza().Length() + aEncText.Length() + pEncNick.Length());
	TPtr8 pPresenceStanza(aPresenceStanza->Des());
	pPresenceStanza.Copy(KPresenceStanza);
	pPresenceStanza.Insert(145, pEncNick);
	pPresenceStanza.Insert(41, aEncText);
	
	iXmppEngine->SendAndForgetXmppStanza(pPresenceStanza, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(3); // aPresenceStanza, aEncNick, aTextUtilities
}

void CBuddycloudLogic::SendPresenceSubscriptionL(const TDesC8& aTo, const TDesC8& aType, const TDesC8& aOptionalData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceSubscriptionL"));
#endif
	
	// Add roster item
	if(aType.Compare(_L8("subscribe")) == 0) {
		_LIT8(KRosterItemStanza, "<iq type='set' id='additem1'><query xmlns='jabber:iq:roster'><item jid=''/></query></iq>\r\n");
		HBufC8* aRosterItemStanza = HBufC8::NewLC(KRosterItemStanza().Length() + aTo.Length());
		TPtr8 pRosterItemStanza(aRosterItemStanza->Des());
		pRosterItemStanza.Append(KRosterItemStanza);
		pRosterItemStanza.Insert(72, aTo);
		
		iXmppEngine->SendAndForgetXmppStanza(pRosterItemStanza, true, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(); // aRosterItemStanza
	}
	
	// Send presence subscription
	_LIT8(KSubscriptionStanza, "<presence to='' type=''/>\r\n");
	_LIT8(KExtendedSubscriptionStanza, "<presence to='' type=''></presence>\r\n");
	HBufC8* aSubscribeStanza = HBufC8::NewLC(KExtendedSubscriptionStanza().Length() + aTo.Length() + aType.Length() + aOptionalData.Length());
	TPtr8 pSubscribeStanza(aSubscribeStanza->Des());
	
	if(aOptionalData.Length() > 0) {
		pSubscribeStanza.Copy(KExtendedSubscriptionStanza);
		pSubscribeStanza.Insert(24, aOptionalData);
	}
	else {
		pSubscribeStanza.Copy(KSubscriptionStanza);
	}

	pSubscribeStanza.Insert(22, aType);
	pSubscribeStanza.Insert(14, aTo);
	
	iXmppEngine->SendAndForgetXmppStanza(pSubscribeStanza, true, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aSubscribeStanza
}

TInt CBuddycloudLogic::GetNewIdStamp() {
	return (++iIdStamp) % 100;
}

/*
----------------------------------------------------------------------------
--
-- Settings
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::ResetStoredDataL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ResetStoredDataL"));
#endif

	iOwnItem = NULL;
		
	// Remove People items
	for(TInt i = iFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			// Unpair and remove item
			if(aRosterItem->GetAddressbookId() != KErrNotFound) {
				UnpairItemsL(aRosterItem->GetItemId());
			}
				
			iDiscussionManager->DeleteDiscussionL(aRosterItem->GetId(EIdRoster));
			iDiscussionManager->DeleteDiscussionL(aRosterItem->GetId(EIdChannel));
			iFollowingList->DeleteItemByIndex(i);
		}
		else if(aItem->GetItemType() == EItemChannel) {
			iDiscussionManager->DeleteDiscussionL(aItem->GetId());
			iFollowingList->DeleteItemByIndex(i);
		}
	}	
	
	// Remove Place items
	while(iPlaceList->Count() > 0) {
		iPlaceList->DeleteItemByIndex(0);
	}
	
	// Remove Nearby items
	for(TInt i = 0; i < iNearbyPlaces.Count(); i++) {
		delete iNearbyPlaces[i];
	}
	
	iNearbyPlaces.Reset();
}

void CBuddycloudLogic::ResetConnectionSettings() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ResetConnectionSettings"));
#endif

	// Perform Reset/Reconnect
	iSettingConnectionMode = 0;
	iSettingConnectionModeId = 0;
	iXmppEngine->SetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId, _L(""));

	if(iState > ELogicOffline) {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
	}

	// If offline start new connection
	if(iState == ELogicOffline) {
		ConnectL();
	}
}

void CBuddycloudLogic::SetLocationResource(TBuddycloudLocationResource aResource, TBool aEnable) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetLocationResource"));
#endif
	
	iSettingsSaveNeeded = true;
	
	switch(aResource) {
		case EResourceCell:
			iLocationEngine->SetCellActive(aEnable);
			break;
		case EResourceWlan:
			iLocationEngine->SetWlanActive(aEnable);
			break;
		case EResourceBt:
			iLocationEngine->SetBtActive(aEnable);
			break;
		case EResourceGps:
			iLocationEngine->SetGpsActive(aEnable);
			break;
	}
}

TBool CBuddycloudLogic::GetLocationResourceDataAvailable(TBuddycloudLocationResource aResource) {
	switch(aResource) {
		case EResourceCell:
			return iLocationEngine->CellDataAvailable();
		case EResourceWlan:
			return iLocationEngine->WlanDataAvailable();
		case EResourceBt:
			return iLocationEngine->BtDataAvailable();
		case EResourceGps:
			return iLocationEngine->GpsDataAvailable();
	}
	
	return false;
}

void CBuddycloudLogic::ValidateUsername() {
	iSettingUsername.LowerCase();
	
	for(TInt i = iSettingUsername.Length() - 1; i >= 0; i--) {
		if(iSettingUsername[i] <= 45 || iSettingUsername[i] == 47 || (iSettingUsername[i] >= 58 && iSettingUsername[i] <= 63) ||
				(iSettingUsername[i] >= 91 && iSettingUsername[i] <= 96) || iSettingUsername[i] >= 123) {
			
			iSettingUsername.Delete(i, 1);
		}
	}
}

void CBuddycloudLogic::LanguageSettingChanged() {
	if(iOwnerObserver) {
		iOwnerObserver->LanguageChanged(iSettingPreferredLanguage);
	}
}

void CBuddycloudLogic::NotificationSettingChanged(TIntSettingItems aItem) {
	if(aItem == ESettingItemPrivateMessageTone) {
		if(iSettingPrivateMessageTone == EToneNone) {
			iSettingPrivateMessageToneFile.Zero();
		}
		else if(iSettingPrivateMessageTone == EToneDefault) {
			iSettingPrivateMessageToneFile.Copy(_L("pm.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingPrivateMessageToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemPrivateMessageTone, iSettingPrivateMessageToneFile);
	}
	else if(aItem == ESettingItemChannelPostTone) {
		if(iSettingChannelPostTone == EToneNone) {
			iSettingChannelPostToneFile.Zero();
		}
		else if(iSettingChannelPostTone == EToneDefault) {
			iSettingChannelPostToneFile.Copy(_L("cp.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingChannelPostToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemChannelPostTone, iSettingChannelPostToneFile);
	}
	else if(aItem == ESettingItemDirectReplyTone) {
		if(iSettingDirectReplyTone == EToneNone) {
			iSettingDirectReplyToneFile.Zero();
		}
		else if(iSettingDirectReplyTone == EToneDefault) {
			iSettingDirectReplyToneFile.Copy(_L("dr.wav"));

			CFileUtilities::CompleteWithApplicationPath(CCoeEnv::Static()->FsSession(), iSettingDirectReplyToneFile);
		}
		
		iNotificationEngine->AddAudioItemL(ESettingItemDirectReplyTone, iSettingDirectReplyToneFile);
	}
}

TDes& CBuddycloudLogic::GetDescSetting(TDescSettingItems aItem) {
	switch(aItem) {
		case ESettingItemEmailAddress:
			return iSettingEmailAddress;
		case ESettingItemUsername:
			return iSettingUsername;
		case ESettingItemPassword:
			return iSettingPassword;			
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
		case ESettingItemServerId:
			return iSettingServerId;
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
		case ESettingItemShowContacts:
			return iSettingShowContacts;
		case ESettingItemContactsLoaded:
			return iContactsLoaded;
		case ESettingItemShowName:
			return iSettingShowName;
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

void CBuddycloudLogic::SetActivityStatus(TInt aResource) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetActivityStatus(TInt aResource)"));
#endif
	
	if(aResource == R_LOCALIZED_STRING_NOTE_CONNECTING) {
		TPtrC pConnectionName = iXmppEngine->GetConnectionName();
		
		if(pConnectionName.Length() > 0) {		
			HBufC* aNoteText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CONNECTINGWITH);
			TPtr pNoteText(aNoteText->Des());
			
			HBufC* aConnectText = HBufC::NewLC(pNoteText.Length() + pConnectionName.Length());
			TPtr pConnectText(aConnectText->Des());
			pConnectText.Append(pNoteText);
			pConnectText.Replace(pConnectText.Find(_L("$APN")), 4, pConnectionName);
			
			SetActivityStatus(pConnectText);
			CleanupStack::PopAndDestroy(2); // aConnectText, aNoteText
		}
		else {
			HBufC* aNoteText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CONNECTING);
			SetActivityStatus(*aNoteText);
			CleanupStack::PopAndDestroy(); // aNoteText
		}	
	}
	else {
		HBufC* aNoteText = CEikonEnv::Static()->AllocReadResourceLC(aResource);
		SetActivityStatus(*aNoteText);
		CleanupStack::PopAndDestroy(); // aNoteText
	}
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

void CBuddycloudLogic::SendSmsOrEmailL(TDesC& aAddress, TDesC& aSubject, TDesC& aBody) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendSmsOrEmailL"));
#endif

	// Create sender & message
	CSendUi* aSendUi = CSendUi::NewLC();
	TUid aServiceUid = KSenduiMtmSmsUid;
	CMessageData* aMessage = CMessageData::NewLC();
	aMessage->AppendToAddressL(aAddress);
	
	if(aAddress.Locate('@') != KErrNotFound) {
		// Email
		aServiceUid = KSenduiMtmSmtpUid;
		aMessage->SetSubjectL(&aSubject);
	}
	
	// Create Body
	CParaFormatLayer* aParaFormatLayer = CParaFormatLayer::NewL();
	CleanupStack::PushL(aParaFormatLayer);
	CCharFormatLayer* aCharFormatLayer = CCharFormatLayer::NewL();
	CleanupStack::PushL(aCharFormatLayer);
	CRichText* aRichText = CRichText::NewL(aParaFormatLayer, aCharFormatLayer);
	CleanupStack::PushL(aRichText);
	aRichText->InsertL(0, aBody);
	aMessage->SetBodyTextL(aRichText);

	// Send
	aSendUi->CreateAndSendMessageL(aServiceUid, aMessage, KNullUid, false);
	CleanupStack::PopAndDestroy(5); // aRichText, aCharFormatLayer, aParaFormatLayer, aMessage, aSendUi
}

/*
----------------------------------------------------------------------------
--
-- Set Status
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::SendInviteL(TInt aFollowerId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendInviteL"));
#endif

	// Maitre'd
	_LIT8(KInviteStanza, "<iq to='maitred.buddycloud.com' type='set' id='invite1'><invite xmlns='http://buddycloud.com/protocol/invite'><details></details></invite></iq>\r\n");		
	_LIT8(KDetailElement, "<detail></detail>");
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	// Maitre'd
	HBufC8* aInviteStanza = HBufC8::NewL(KInviteStanza().Length());
	TPtr8 pInviteStanza(aInviteStanza->Des());
	pInviteStanza.Append(KInviteStanza);

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));

	if(aItem && aItem->GetItemType() == EItemContact) {
		CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);

		// Select contact
		CContactItem* aContact = GetContactDetailsLC(aContactItem->GetAddressbookId());
		
		if(aContact != NULL) {
			CContactItemFieldSet& aContactFieldSet = aContact->CardFields();
			RPointerArray<HBufC> aDynItems;				
			
			// List phone numbers & email addresses
			for(TInt x = 0; x < aContactFieldSet.Count(); x++) {
				if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber) ||
						aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
					
					// Add detail
					HBufC* aDetailLine = HBufC::NewLC(2 + aContactFieldSet[x].Label().Length() + 1 + aContactFieldSet[x].TextStorage()->Text().Length());
					TPtr pDetailLine(aDetailLine->Des());
					pDetailLine.Append(aContactFieldSet[x].TextStorage()->Text());				
					pDetailLine.Insert(0, _L("\t"));
					pDetailLine.Insert(0, aContactFieldSet[x].Label());
					
					if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber)) {
						pDetailLine.Insert(0, _L("0\t"));
					}
					else {
						pDetailLine.Insert(0, _L("1\t"));
					}
					
					aDynItems.Append(aDetailLine);
					CleanupStack::Pop();
					
					// ---------------------------------------------------------
					// Maitre'd
					// ---------------------------------------------------------
					TPtrC8 aEncDetail(aTextUtilities->UnicodeToUtf8L(aContactFieldSet[x].TextStorage()->Text()));

					// Package detail
					HBufC8* aDetailElement = HBufC8::NewLC(KDetailElement().Length() + aEncDetail.Length());
					TPtr8 pDetailElement(aDetailElement->Des());
					pDetailElement.Append(KDetailElement);
					pDetailElement.Insert(8, aEncDetail);
					
					// Add to stanza
					aInviteStanza = aInviteStanza->ReAlloc(pInviteStanza.Length()+pDetailElement.Length());
					pInviteStanza.Set(aInviteStanza->Des());
					pInviteStanza.Insert(pInviteStanza.Length() - 26, pDetailElement);

					CleanupStack::PopAndDestroy(); // aDetailElement
				}
			}
			
			if(aDynItems.Count() == 0) {
				aDynItems.Append(_L("\t").AllocL());
			}
			
			// Selection result
			TInt aSelectedItem = DisplayDoubleLinePopupMenuL(aDynItems);
			
			if(aSelectedItem != KErrNotFound) {
				TPtr pSelectedAddress(aDynItems[aSelectedItem]->Des());
				pSelectedAddress.Delete(0, pSelectedAddress.LocateReverse('\t') + 1);
				
				// Send invite to contact
				HBufC* aSubject = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_INVITEMESSAGE_TITLE);
				HBufC* aBody = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_INVITEMESSAGE_TEXT);					
				SendSmsOrEmailL(pSelectedAddress, *aSubject, *aBody);
				CleanupStack::PopAndDestroy(2); // aBody, aSubject
				
				// Send invite to maitre'd
				iXmppEngine->SendAndForgetXmppStanza(pInviteStanza, true);
			}
				
			while(aDynItems.Count() > 0) {
				delete aDynItems[0];
				aDynItems.Remove(0);
			}
			
			aDynItems.Close();		
			CleanupStack::PopAndDestroy(); // aContact
		}			
	}
	
	if(aInviteStanza) {
		delete aInviteStanza;
	}
	
	CleanupStack::PopAndDestroy(); // aTextUtilities
}

void CBuddycloudLogic::SendPlaceL(TInt aFollowerId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPlaceL"));
#endif

	if(iLocationEngine->GetLastPlaceId() > 0) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));
	
		if(aItem && (aItem->GetItemType() == EItemContact || aItem->GetItemType() == EItemRoster)) {
			CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);

			if(aContactItem->GetAddressbookId() > KErrNotFound) {
				// Select contact
				CContactItem* aContact = GetContactDetailsLC(aContactItem->GetAddressbookId());
				
				if(aContact != NULL) {
					CContactItemFieldSet& aContactFieldSet = aContact->CardFields();
					RPointerArray<HBufC> aDynItems;				
					
					// List phone numbers & email addresses
					for(TInt x = 0; x < aContactFieldSet.Count(); x++) {
						if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber) ||
								aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
							
							// Add detail
							HBufC* aDetailLine = HBufC::NewLC(2 + aContactFieldSet[x].Label().Length() + 1 + aContactFieldSet[x].TextStorage()->Text().Length());
							TPtr pDetailLine(aDetailLine->Des());
							pDetailLine.Append(aContactFieldSet[x].TextStorage()->Text());				
							pDetailLine.Insert(0, _L("\t"));
							pDetailLine.Insert(0, aContactFieldSet[x].Label());
							
							if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber)) {
								pDetailLine.Insert(0, _L("0\t"));
							}
							else {
								pDetailLine.Insert(0, _L("1\t"));
							}
						
							aDynItems.Append(aDetailLine);
							CleanupStack::Pop();
						}
					}
					
					if(aDynItems.Count() == 0) {
						aDynItems.Append(_L("\t").AllocL());
					}
					
					// Selection result
					TInt aSelectedItem = DisplayDoubleLinePopupMenuL(aDynItems);
					
					if(aSelectedItem != KErrNotFound) {
						TPtr pSelectedAddress(aDynItems[aSelectedItem]->Des());
						pSelectedAddress.Delete(0, pSelectedAddress.LocateReverse('\t') + 1);
						
						// Construct body
						TBuf<32> aId;
						aId.Format(_L("%d"), iLocationEngine->GetLastPlaceId());
						HBufC* aSendPlaceText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SENDPLACEMESSAGE_TEXT);
						TPtr pSendPlaceText(aSendPlaceText->Des());
						HBufC* aBody = HBufC::NewLC(pSendPlaceText.Length() + aId.Length() + iOwnItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length());
						
						TPtr pBody(aBody->Des());
						pBody.Copy(pSendPlaceText);
						pBody.Replace(pBody.Find(_L("$PLACE")), 6, iOwnItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText));
						pBody.Replace(pBody.Find(_L("$ID")), 3, aId);
						
						HBufC* aSubject = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_SENDPLACEMESSAGE_TITLE);			
						
						SendSmsOrEmailL(pSelectedAddress, *aSubject, pBody);
						CleanupStack::PopAndDestroy(3); // aSubject, aBody, aSendPlaceText
					}
						
					while(aDynItems.Count() > 0) {
						delete aDynItems[0];
						aDynItems.Remove(0);
					}
					
					aDynItems.Close();		
					CleanupStack::PopAndDestroy(); // aContact
				}	
			}
		}
	}
}

void CBuddycloudLogic::FollowContactL(const TDesC& aContact) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowContactL"));
#endif

	if(aContact.Locate('@') != KErrNotFound) {	
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(IsSubscribedTo(aContact, EItemRoster)));
		
		if(aItem == NULL) {
			// Prepare data
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			
			_LIT8(KNickElement, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
			HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
			TPtr8 pEncFullName(aEncFullName->Des());
			
			_LIT8(KGeolocElement, "<geoloc xmlns='http://jabber.org/protocol/geoloc'><text></text></geoloc>");
			HBufC8* aEncLocation = aTextUtilities->UnicodeToUtf8L(*iBroadLocationText).AllocLC();
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
			
			SendPresenceSubscriptionL(aTextUtilities->UnicodeToUtf8L(aContact), _L8("subscribe"), pOptionalData);
			CleanupStack::PopAndDestroy(4); // aOptionalData, aEncLocation, aEncFullName, aTextUtilities
					
			// Acknowledge request sent
			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_REQUESTSENT);
			CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage
		}
	}
}

void CBuddycloudLogic::RecollectFollowerDetailsL(TInt aFollowerId) {
	if(iState == ELogicOnline) {	
//#ifdef _DEBUG
//		WriteToLog(_L8("BL    CBuddycloudLogic::RecollectFollowerDetailsL"));
//#endif

		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));
		
		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(!aRosterItem->PubsubCollected() && aRosterItem->GetPubsubAffiliation() > EPubsubAffiliationNone &&
					aRosterItem->GetPubsubSubscription() == EPubsubSubscriptionSubscribed) {
				
//				// Collect nodes
//				CollectUserPubsubNodeL(aRosterItem->GetId(), _L8("/mood"));
//				CollectUserPubsubNodeL(aRosterItem->GetId(), _L8("/geo/previous"));
//				CollectUserPubsubNodeL(aRosterItem->GetId(), _L8("/geo/current"));
//				CollectUserPubsubNodeL(aRosterItem->GetId(), _L8("/geo/future"));				
			}
			
			aRosterItem->SetPubsubCollected(true);
		}
	}	
}

void CBuddycloudLogic::SendPresenceToPubsubL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceToPubsubL"));
#endif
	TPtrC8 aNodeItemId(iLastNodeIdReceived->Des());
	
		// Send direct presence with last node item id
	_LIT8(KPubsubPresence, "<presence to=''><set xmlns='http://jabber.org/protocol/rsm'><after></after></set></presence>\r\n");
	HBufC8* aPubsubPresence = HBufC8::NewLC(KPubsubPresence().Length() + KBuddycloudPubsubServer().Length() + aNodeItemId.Length());
	TPtr8 pPubsubPresence(aPubsubPresence->Des());
	pPubsubPresence.Append(KPubsubPresence);
	pPubsubPresence.Insert(67, aNodeItemId);
	pPubsubPresence.Insert(14, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndForgetXmppStanza(pPubsubPresence, false, EXmppPriorityHigh);
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

void CBuddycloudLogic::CollectPubsubSubscriptionsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectPubsubSubscriptionsL"));
#endif

	// Collect users pubsub subscriptions
	_LIT8(KPubsubSubscriptions, "<iq to='' type='get' id='pubsubsubscriptions'><pubsub xmlns='http://jabber.org/protocol/pubsub'><subscriptions/></pubsub></iq>\r\n");
	HBufC8* aPubsubSubscriptions = HBufC8::NewLC(KPubsubSubscriptions().Length() + KBuddycloudPubsubServer().Length());
	TPtr8 pPubsubSubscriptions(aPubsubSubscriptions->Des());
	pPubsubSubscriptions.Append(KPubsubSubscriptions);
	pPubsubSubscriptions.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pPubsubSubscriptions, this, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(); // aPubsubSubscriptions	
}

void CBuddycloudLogic::ProcessPubsubSubscriptionsL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPubsubSubscriptionsL"));
#endif

	CBuddycloudListStore* aChannelItemStore = CBuddycloudListStore::NewLC();
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	// Collect subscriptions
	while(aXmlParser->MoveToElement(_L8("subscription"))) {
		TPtrC8 aAttributeNode(aXmlParser->GetStringAttribute(_L8("node")));
		
		CXmppPubsubNodeParser* aNodeParser = CXmppPubsubNodeParser::NewLC(aAttributeNode);
		TXmppPubsubSubscription aSubscription = CXmppEnumerationConverter::PubsubSubscription(aXmlParser->GetStringAttribute(_L8("subscription")));
		TXmppPubsubAffiliation aAffiliation = CXmppEnumerationConverter::PubsubAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation")));
		
		if(aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
			// Handle topic channel node
			CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
			aChannelItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aAttributeNode));
			aChannelItem->SetPubsubSubscription(aSubscription);
			aChannelItem->SetPubsubAffiliation(aAffiliation);
			
			aChannelItemStore->AddItem(aChannelItem);
			CleanupStack::Pop(); // aChannelItem
		}
		else if(aNodeParser->GetNode(2).Compare(_L8("channel")) == 0) {
			// Handle user channel node
			TPtrC aJid(aTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)));
			
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

				if(aItem && aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
					if(aRosterItem->GetId().Compare(aJid) == 0) {
						TPtrC aEncNode(aTextUtilities->Utf8ToUnicodeL(aAttributeNode));
						
						if(aRosterItem->GetId(EIdChannel).Compare(aEncNode) != 0) {
							// Get old discussion
							CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId(EIdChannel));							
							
							// Change channel id
							aRosterItem->SetIdL(aEncNode, EIdChannel);
							aRosterItem->SetUnreadData(aDiscussion, EIdChannel);
							
							// Update discuss with new id
							aDiscussion->SetDiscussionIdL(aRosterItem->GetId(EIdChannel));
							aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
							
							// Collect node items
							CollectLastPubsubNodeItemsL(aRosterItem->GetId(EIdChannel), _L8("1"));
							
							iSettingsSaveNeeded = true;
						}

						aRosterItem->SetPubsubSubscription(aSubscription);
						aRosterItem->SetPubsubAffiliation(aAffiliation);
						
						break;
					}
				}				
			}
		}
		
		CleanupStack::PopAndDestroy(); // aNodeParser
	}
	
	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
	
	// Synchronize local roster to received roster
	for(TInt i = (iFollowingList->Count() - 1); i >= 1; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem && aItem->GetItemType() == EItemChannel) {
			CFollowingChannelItem* aLocalItem = static_cast <CFollowingChannelItem*> (aItem);
			
			TBool aItemFound = false;
			
			for(TInt x = 0; x < aChannelItemStore->Count(); x++) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aChannelItemStore->GetItemByIndex(x));
				
				if(aLocalItem->GetId().Compare(aChannelItem->GetId()) == 0) {				
					// Update
					aLocalItem->SetPubsubSubscription(aChannelItem->GetPubsubSubscription());
					aLocalItem->SetPubsubAffiliation(aChannelItem->GetPubsubAffiliation());
					
					aChannelItemStore->DeleteItemByIndex(x);
					
					aItemFound = true;
					break;
				}
			}
			
			if(!aItemFound) {
				// Remove no longer subscribed to channel
				iDiscussionManager->DeleteDiscussionL(aLocalItem->GetId());
				iFollowingList->DeleteItemByIndex(i);
				
				iSettingsSaveNeeded = true;
			}
		}
	}
	
	// Add new channel items
	while(aChannelItemStore->Count() > 0) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aChannelItemStore->GetItemByIndex(0));
		CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
				
		// Set temporary channel name
		TPtrC aTitle(aChannelItem->GetId());
		TInt aLocate = aTitle.LocateReverse('/');		
		
		if(aLocate != KErrNotFound) {
			aTitle.Set(aTitle.Mid(aLocate + 1));
		}
		
		aChannelItem->SetItemId(iNextItemId++);
		aChannelItem->SetTitleL(aTitle);
		aChannelItem->SetUnreadData(aDiscussion);
					
		// Add to discussion manager
		aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());		
	
		iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);		
		aChannelItemStore->RemoveItemByIndex(0);	
		
		// Collect metadata & items
		CollectChannelMetadataL(aChannelItem->GetId());
		CollectLastPubsubNodeItemsL(aChannelItem->GetId(), _L8("1"));
			
		iSettingsSaveNeeded = true;
	}

	CleanupStack::PopAndDestroy(); // aChannelItemStore
	
	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
	
	// Send presence to pubsub
	SendPresenceToPubsubL();
	
	// Collect users pubsub node subscribers
	CollectUsersPubsubNodeSubscribersL();
}

void CBuddycloudLogic::CollectUsersPubsubNodeSubscribersL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectUsersPubsubNodeSubscribersL"));
#endif
	
	if(iOwnItem) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		TPtrC8 aEncId = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId(EIdChannel));
		
		// Collect users pubsub node subscribers
		_LIT8(KNodeSubscribers, "<iq to='' type='get' id='pubsubnodesubscribers'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><subscriptions node=''/></pubsub></iq>\r\n");
		HBufC8* aNodeSubscribers = HBufC8::NewLC(KNodeSubscribers().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length());
		TPtr8 pNodeSubscribers(aNodeSubscribers->Des());
		pNodeSubscribers.Append(KNodeSubscribers);
		pNodeSubscribers.Insert(125, aEncId);
		pNodeSubscribers.Insert(8, KBuddycloudPubsubServer);
	
		iXmppEngine->SendAndAcknowledgeXmppStanza(pNodeSubscribers, this, false, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(2); // aPubsubSubscriptions, aTextUtilities
	}
}

void CBuddycloudLogic::ProcessUsersPubsubNodeSubscribersL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessUsersPubsubNodeSubscribersL"));
#endif
	
	// Synchronize users node subscribers to my roster
	CBuddycloudListStore* aUserNodeSubscribers = CBuddycloudListStore::NewLC();
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	// Users node id
	HBufC8* aEncId = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId(EIdChannel)).AllocLC();
	TPtrC8 pEncId(aEncId->Des());
	
	// Parse personal channel members
	while(aXmlParser->MoveToElement(_L8("subscription"))) {	
		CFollowingItem* aItem = CFollowingItem::NewLC();
		aItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
		
		aUserNodeSubscribers->AddItem(aItem);
		CleanupStack::Pop(); // aItem
	}
	
	_LIT8(KAffiliationItem, "<affiliation jid='' affiliation=''/>");
	HBufC8* aDeltaList = HBufC8::NewLC(0);
	TPtr8 pDeltaList(aDeltaList->Des());
	
	// Syncronization of roster items & subscribers
	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom) {
				TBool aUserFound = false;
					
				for(TInt x = 0; x < aUserNodeSubscribers->Count(); x++) {
					CFollowingItem* aSubscriberItem = static_cast <CFollowingItem*> (aUserNodeSubscribers->GetItemByIndex(x));
					
					if(aRosterItem->GetId().Compare(aSubscriberItem->GetId()) == 0) {						
						// Delete subscriber from list
						aUserNodeSubscribers->DeleteItemByIndex(x);
						
						aUserFound = true;
						break;
					}
				}
				
				if(!aUserFound) {
					// Add user to subscribers
					TPtrC8 aEncJid = aTextUtilities->UnicodeToUtf8L(aRosterItem->GetId());
					
					CleanupStack::Pop(aDeltaList);
					aDeltaList = aDeltaList->ReAlloc(pDeltaList.Length() + KAffiliationItem().Length() + aEncJid.Length() + KPubsubAffiliationPublisher().Length());
					CleanupStack::PushL(aDeltaList);
					
					pDeltaList.Set(aDeltaList->Des());
					pDeltaList.Append(KAffiliationItem);
					pDeltaList.Insert(pDeltaList.Length() - 18, aEncJid);
					pDeltaList.Insert(pDeltaList.Length() - 3, KPubsubAffiliationPublisher);					
				}
			}
		}
	}
	
	// Send affiliation delta
	if(pDeltaList.Length() > 0) {
		_LIT8(KAffiliationStanza, "<iq to='' type='set' id='affiliation:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''></affiliations></pubsub></iq>\r\n");	
		HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + KBuddycloudPubsubServer().Length() + pEncId.Length() + pDeltaList.Length());
		TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
		pAffiliationStanza.AppendFormat(KAffiliationStanza, GetNewIdStamp());
		pAffiliationStanza.Insert(119, pDeltaList);
		pAffiliationStanza.Insert(117, pEncId);
		pAffiliationStanza.Insert(8, KBuddycloudPubsubServer);

		iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
		CleanupStack::PopAndDestroy(); // aAffiliationStanza
	}
	
	// Remove users from subscribers
	if(aUserNodeSubscribers->Count() > 0) {
		_LIT8(KSubscriptionStanza, "<iq to='' type='set' id='subscription:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><subscriptions node=''></subscriptions></pubsub></iq>\r\n");	
		_LIT8(KSubscriptionItem, "<subscription jid='' subscription=''/>");

		CleanupStack::PopAndDestroy(aDeltaList);
		aDeltaList = HBufC8::NewLC(0);
		TPtr8 pDeltaList(aDeltaList->Des());
		
		while(aUserNodeSubscribers->Count() > 0) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (aUserNodeSubscribers->GetItemByIndex(0));		
			TPtrC8 aEncJid = aTextUtilities->UnicodeToUtf8L(aItem->GetId());
			
			CleanupStack::Pop();
			aDeltaList = aDeltaList->ReAlloc(pDeltaList.Length() + KSubscriptionItem().Length() + aEncJid.Length() + KPubsubSubscriptionNone().Length());
			CleanupStack::PushL(aDeltaList);

			pDeltaList.Set(aDeltaList->Des());
			pDeltaList.Append(KSubscriptionItem);
			pDeltaList.Insert(pDeltaList.Length() - 19, aEncJid);
			pDeltaList.Insert(pDeltaList.Length() - 3, KPubsubSubscriptionNone);					

			aUserNodeSubscribers->DeleteItemByIndex(0);
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

	CleanupStack::PopAndDestroy(5); // aDeltaList, aEncId, aXmlParser, aTextUtilities, aUserNodeSubscribers		
}

void CBuddycloudLogic::CollectLastPubsubNodeItemsL(const TDesC& aNode, const TDesC8& aLastIdReceived) {
	if(aNode.Length() > 0) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		TPtrC8 aEncNode(aTextUtilities->UnicodeToUtf8L(aNode));
	
		_LIT8(KNodeItemsStanza, "<iq to='' type='get' id='nodeitems:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node=''><set xmlns='http://jabber.org/protocol/rsm'><after></after></set></items></pubsub></iq>\r\n");
		HBufC8* aNodeItemsStanza = HBufC8::NewLC(KNodeItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + aLastIdReceived.Length());
		TPtr8 pNodeItemsStanza(aNodeItemsStanza->Des());
		pNodeItemsStanza.AppendFormat(KNodeItemsStanza, GetNewIdStamp());
		pNodeItemsStanza.Insert(155, aLastIdReceived);
		pNodeItemsStanza.Insert(102, aEncNode);
		pNodeItemsStanza.Insert(8, KBuddycloudPubsubServer);
		
		iXmppEngine->SendAndForgetXmppStanza(pNodeItemsStanza, false);
		CleanupStack::PopAndDestroy(2); // aNodeItemsStanza, aTextUtilities
	}
}

void CBuddycloudLogic::CollectUserPubsubNodeL(const TDesC& aJid, const TDesC& aNodeLeaf) {
	if(aJid.Locate('@') != KErrNotFound && aNodeLeaf.Length() > 0) {
		_LIT(KNodeRoot, "/user/");
		HBufC* aNode = HBufC::NewLC(KNodeRoot().Length() + aJid.Length() + 1 + aNodeLeaf.Length());
		TPtr pNode(aNode->Des());
		pNode.Append(KNodeRoot);
		pNode.Append(aJid);
		
		if(aNodeLeaf.Locate('/') != 0) {
			pNode.Append(_L("/"));
		}
		
		pNode.Append(aNodeLeaf);

		CollectLastPubsubNodeItemsL(pNode, _L8("1"));
		CleanupStack::PopAndDestroy(); // aNode
	}
}

void CBuddycloudLogic::CollectChannelMetadataL(const TDesC& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectChannelMetadataL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	TPtrC8 aEncId = aTextUtilities->UnicodeToUtf8L(aNode);
	
	_LIT8(KDiscoItemsStanza, "<iq to='' type='get' id='metadata:%02d'><query xmlns='http://jabber.org/protocol/disco#info' node=''/></iq>\r\n");
	HBufC8* aDiscoItemsStanza = HBufC8::NewLC(KDiscoItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length());
	TPtr8 pDiscoItemsStanza(aDiscoItemsStanza->Des());
	pDiscoItemsStanza.AppendFormat(KDiscoItemsStanza, GetNewIdStamp());
	pDiscoItemsStanza.Insert(97, aEncId);
	pDiscoItemsStanza.Insert(8, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pDiscoItemsStanza, this, true);
	CleanupStack::PopAndDestroy(2); // aDiscoItemsStanza, aTextUtilities	
}

void CBuddycloudLogic::ProcessChannelMetadataL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessChannelMetadataL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);

	if(aXmlParser->MoveToElement(_L8("query"))) {
		TPtrC aEncAttributeNode(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("node"))));
		
		if(aEncAttributeNode.Length() > 0) {
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
				
				if(aItem && aItem->GetItemType() == EItemChannel) {
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
					
					if(aChannelItem->GetId().Compare(aEncAttributeNode) == 0) {
						while(aXmlParser->MoveToElement(_L8("field"))) {
							TPtrC8 aAttributeVar(aXmlParser->GetStringAttribute(_L8("var")));
							
							if(aXmlParser->MoveToElement(_L8("value"))) {
								TPtrC aEncDataValue(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));							
								
								if(aAttributeVar.Compare(_L8("pubsub#title")) == 0) {
									aChannelItem->SetTitleL(aEncDataValue);
								}
								else if(aAttributeVar.Compare(_L8("pubsub#description")) == 0) {
									aChannelItem->SetDescriptionL(aEncDataValue);
								}
								else if(aAttributeVar.Compare(_L8("pubsub#access_model")) == 0) {
									aChannelItem->SetAccessModel(CXmppEnumerationConverter::PubsubAccessModel(aXmlParser->GetStringData()));
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
	
	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
}

void CBuddycloudLogic::SetPubsubNodeAffiliationL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubAffiliation aAffiliation) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetPubsubNodeAffiliationL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();	
	TPtrC8 pEncJid(aEncJid->Des());	
	
	TPtrC8 aEncNode(aTextUtilities->UnicodeToUtf8L(aNode));			
	TPtrC8 aAffiliationText(CXmppEnumerationConverter::PubsubAffiliation(aAffiliation));
	
	_LIT8(KAffiliationStanza, "<iq to='' type='set' id='affiliation:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><affiliations node=''><affiliation jid='' affiliation=''/></affiliations></pubsub></iq>\r\n");	
	HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + pEncJid.Length() + aAffiliationText.Length());
	TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
	pAffiliationStanza.AppendFormat(KAffiliationStanza, GetNewIdStamp());
	pAffiliationStanza.Insert(152, aAffiliationText);
	pAffiliationStanza.Insert(137, pEncJid);
	pAffiliationStanza.Insert(117, aEncNode);
	pAffiliationStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
	CleanupStack::PopAndDestroy(3); // aAffiliationStanza, aEncJid, aTextUtilities
}

void CBuddycloudLogic::SetPubsubNodeSubscriptionL(const TDesC& aJid, const TDesC& aNode, TXmppPubsubSubscription aSubscription) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetPubsubNodeSubscriptionL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	
	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();	
	TPtrC8 pEncJid(aEncJid->Des());	
	
	TPtrC8 aEncNode(aTextUtilities->UnicodeToUtf8L(aNode));			
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
	CleanupStack::PopAndDestroy(3); // aSubscriptionStanza, aEncJid, aTextUtilities
}

void CBuddycloudLogic::RequestPubsubNodeAffiliationL(const TDesC& aNode, TXmppPubsubAffiliation aAffiliation, const TDesC& aText) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RequestPubsubNodeAffiliationL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	HBufC8* aEncText = aTextUtilities->UnicodeToUtf8L(aText).AllocLC();
	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()).AllocLC();
	
	TPtrC8 aEncNode(aTextUtilities->UnicodeToUtf8L(aNode));			
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
	CleanupStack::PopAndDestroy(4); // aAffiliationStanza, aEncJid, aEncText, aTextUtilities
}

void CBuddycloudLogic::RetractPubsubNodeItemL(const TDesC& aNode, const TDesC8& aNodeItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RetractPubsubNodeItemL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	TPtrC8 aEncNode(aTextUtilities->UnicodeToUtf8L(aNode));			
	
	_LIT8(KRetractStanza, "<iq to='' type='set' id='retractitem:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><retract node='' notify='1'><item id=''/></retract></pubsub></iq>\r\n");	
	HBufC8* aRetractStanza = HBufC8::NewLC(KRetractStanza().Length() + KBuddycloudPubsubServer().Length() + aEncNode.Length() + aNodeItemId.Length());
	TPtr8 pRetractStanza(aRetractStanza->Des());
	pRetractStanza.AppendFormat(KRetractStanza, GetNewIdStamp());
	pRetractStanza.Insert(129, aNodeItemId);
	pRetractStanza.Insert(106, aEncNode);
	pRetractStanza.Insert(8, KBuddycloudPubsubServer);

	iXmppEngine->SendAndForgetXmppStanza(pRetractStanza, true);
	CleanupStack::PopAndDestroy(2); // aRetractStanza, aTextUtilities
}

void CBuddycloudLogic::HandlePubsubEventL(const TDesC8& aStanza, TBool aNewEvent) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePubsubEventL"));
#endif
	
	TBool aBubbleEvent = false;
	TInt aNotifyId = KErrNotFound;
	
	TXmppPubsubEventType aPubsubEventType;
	
	// Initialize xml parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
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
			HBufC* aUserJid = aTextUtilities->Utf8ToUnicodeL(aNodeParser->GetNode(1)).AllocLC();
			TPtrC pUserJid(aUserJid->Des());
			
			TPtrC aNodeId(aTextUtilities->Utf8ToUnicodeL(aAttributeNode));		
			
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
				
				if(aItem && aItem->GetItemType() >= EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
					if((aItem->GetItemType() == EItemRoster && aRosterItem->GetId().Compare(pUserJid) == 0) || 
							aItem->GetId().CompareF(aNodeId) == 0) {
						
						aNotifyId = aItem->GetItemId();
						
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
											TPtrC aMood(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
											
											if(!aRosterItem->OwnItem() && aRosterItem->GetDescription().CompareF(aMood) != 0) {
												aBubbleEvent = true;
											}
											
											aRosterItem->SetDescriptionL(aMood);
										}
										
										// Set last node id received
										if(aNewEvent) {
											SetLastNodeIdReceived(aItemId);
										}
									}
									else if(aElement.Compare(_L8("retract")) == 0) {
										// Deleted item
										aRosterItem->SetDescriptionL(_L(""));
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
														
														CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
														aTextUtilities->AppendToString(aBroadLocation, aGeoloc->GetString(EGeolocLocality), _L(""));
														aTextUtilities->AppendToString(aBroadLocation, aGeoloc->GetString(EGeolocCountry), _L(", "));
														CleanupStack::PopAndDestroy(); // aTextUtilities
														
														// Send presence
														SendPresenceL();
													}
												}
												else if(aGeolocItemType == EGeolocItemFuture) {
													if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).FindF(aGeoloc->GetString(EGeolocText)) != KErrNotFound) {
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
											if(aNewEvent) {
												SetLastNodeIdReceived(aItemId);
											}
										}
										else if(aElement.Compare(_L8("retract")) == 0) {
											// Deleted item
											aRosterItem->SetGeolocItemL(aGeolocItemType, NULL);
										}
									}
								}
								else if(aNodeParser->GetNode(2).Compare(_L8("channel")) == 0 || aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
									CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
									CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
									
									if(aElement.Compare(_L8("item")) == 0 && aXmlParser->MoveToElement(_L8("entry"))) {
										// Channel post
										TBool aNotify = aDiscussion->Notify();
										TInt aAudioId = ESettingItemChannelPostTone;
										TBuf8<32> aReferenceId;
										
										CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();						
										CAtomEntryData* aAtomEntry = aAtomEntryParser->XmlToAtomEntryLC(aXmlParser->GetStringData(), aReferenceId);									
										aAtomEntry->SetIdL(aItemId);
										
										if(aAtomEntry->GetAuthorJid().Compare(iOwnItem->GetId()) == 0) {
											// Own post
											aAtomEntry->SetRead(true);
											aNotify = false;
										}
										else if(aReferenceId.Length() > 0) {
											CThreadedEntry* aReferenceEntry = aDiscussion->GetThreadedEntryById(aReferenceId);
											
											if(aReferenceEntry && aReferenceEntry->GetEntry()->GetAuthorJid().Compare(iOwnItem->GetId()) == 0) {
												// Comment to own post
												aAtomEntry->SetDirectReply(true);
											}
										}
										
										// Notification
										if(aAtomEntry->DirectReply() && iSettingNotifyReplyTo) {
											aAudioId = ESettingItemDirectReplyTone;
											aNotify = true;
										}
										else if(aNotify && 
												((aChannelItem->GetPubsubAffiliation() >= EPubsubAffiliationModerator && 
														(iSettingNotifyChannelsModerating == 1 || 
														(iSettingNotifyChannelsModerating == 0 && aDiscussion->GetUnreadEntries() == 0))) || 
												((aChannelItem->GetPubsubAffiliation() < EPubsubAffiliationModerator && 
														(iSettingNotifyChannelsFollowing == 1 || 
														(iSettingNotifyChannelsFollowing == 0 && aDiscussion->GetUnreadEntries() == 0)))))) {
											
											// Channel notification on
											aNotify = true;									
										}
										else {
											aNotify = false;									
										}
										
										if(aDiscussion->AddEntryOrCommentLD(aAtomEntry, aReferenceId)) {
											if(iFollowingList->GetIndexById(aNotifyId) > 0) {
												aBubbleEvent = true;
												aNewEvent = true;
											}
											
											if(aNotify && !iTelephonyEngine->IsTelephonyBusy()) {
												NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aNotifyId);
												
												iNotificationEngine->NotifyL(aAudioId);
											}
											else {
												NotifyNotificationObservers(ENotificationMessageSilentEvent, aNotifyId);
											}
										}
										
										CleanupStack::PopAndDestroy(); // aAtomEntryParser
										
										// Set last node id received
										if(aNewEvent) {
											SetLastNodeIdReceived(aItemId);
										}
									}
									else if(aElement.Compare(_L8("retract")) == 0) {
										// Channel post deleted
										if(aDiscussion->DeleteEntryById(aItemId)) {
											if(iFollowingList->GetIndexById(aNotifyId) > 0) {
												aBubbleEvent = true;
											}
											
											NotifyNotificationObservers(ENotificationMessageSilentEvent, aNotifyId);
										}	
									}
								}
							}
						}
						else if(aNodeParser->GetNode(2).Compare(_L8("channel")) == 0 || aNodeParser->GetNode(0).Compare(_L8("channel")) == 0) {
							// Handle other event types for channels
							CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
							CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
							TInt aResourceNotificationId = KErrNotFound;
							
							// User node changes
							if(aChannelItem->GetItemType() == EItemRoster && aChannelItem->GetId().Compare(aNodeId) != 0) {
								// Set new node id
								aChannelItem->SetIdL(aNodeId);
								aChannelItem->SetUnreadData(aDiscussion);
								
								// Set discussion id
								aDiscussion->SetDiscussionIdL(aNodeId);								
								aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());
																
								// TODO: Collect metadata
								// CollectChannelMetadataL(aChannelItem->GetId());
								
								// Collect node items
								CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/channel"));
								
								iSettingsSaveNeeded = true;
							}
							
							if(aPubsubEventType == EPubsubEventAffiliation) {
								// Handle affiliation event
								TPtrC aAttributeJid(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
								
								if(iOwnItem->GetId().Compare(aAttributeJid) == 0) {
									// Own affiliation to channel change
									TXmppPubsubAffiliation aAffiliation = CXmppEnumerationConverter::PubsubAffiliation(aXmlParser->GetStringAttribute(_L8("affiliation")));
									
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
								TPtrC aSubscriptionJid(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
								
								if(iOwnItem->GetId().Compare(aSubscriptionJid) == 0) {
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
								iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());
								iFollowingList->DeleteItemById(aChannelItem->GetItemId());
							}							
							
							// Add notification to channel
							if(aResourceNotificationId != KErrNotFound) {
								HBufC* aResourceText = CEikonEnv::Static()->AllocReadResourceLC(aResourceNotificationId);
								
								CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
								aAtomEntry->SetPublishTime(TimeStamp());
								aAtomEntry->SetContentL(*aResourceText, EEntryContentAction);			
								aAtomEntry->SetIconId(KIconChannel);	
								aAtomEntry->SetAuthorAffiliation(EPubsubAffiliationOwner);
								aAtomEntry->SetDirectReply(true);
								aAtomEntry->SetPrivate(true);
								
								// Add to discussion
								if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {
									if(iFollowingList->GetIndexById(aNotifyId) > 0) {
										aBubbleEvent = true;
										aNewEvent = true;
									}
									
									NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aNotifyId);
									
									iNotificationEngine->NotifyL(ESettingItemDirectReplyTone);
								}
								
								CleanupStack::PopAndDestroy(); // aResourceText
							}
						}
						
						if(aBubbleEvent && aNewEvent) {
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
	
	CleanupStack::PopAndDestroy(2); // aTextUtilities, aItemsXmlParser
	
	if(aNotifyId != KErrNotFound) {
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aNotifyId);
	}
}

void CBuddycloudLogic::HandlePubsubRequestL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePubsubRequestL"));
#endif
	
	TPtrC8 aNode;	

	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
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
				HBufC* aTitle = CEikonEnv::Static()->AllocReadResourceLC(aTitleResourceId);
				aAtomEntry->SetAuthorNameL(*aTitle);
				aAtomEntry->SetAuthorJidL(aTextUtilities->Utf8ToUnicodeL(aDataValue), false);
				
				// Set text
				HBufC* aText = CEikonEnv::Static()->AllocReadResourceLC(aTextResourceId);
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
				TPtrC aEncData(aTextUtilities->Utf8ToUnicodeL(aDataValue));
				
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
	
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aTextUtilities->Utf8ToUnicodeL(aNode));
	CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (iFollowingList->GetItemById(aDiscussion->GetItemId()));
	
	if(aChannelItem && aChannelItem->GetItemType() >= EItemRoster) {		
		// Set content
		TPtrC aEncJid = aTextUtilities->Utf8ToUnicodeL(aAtomEntry->GetId());
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
			NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aChannelItem->GetItemId());
			
			iNotificationEngine->NotifyL(ESettingItemChannelPostTone);
		}		
	}
	else {
		CleanupStack::PopAndDestroy(); // aAtomEntry		
	}
	
	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
}

void CBuddycloudLogic::FollowChannelL(const TDesC& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowChannelL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	_LIT(KRootNode, "/channel/");
	HBufC* aNodeId = HBufC::NewLC(aNode.Length() + KRootNode().Length());
	TPtr pNodeId(aNodeId->Des());
	pNodeId.Append(aNode);
	pNodeId.LowerCase();
	
	if(pNodeId.Find(KRootNode) != 0) {
		// Add node root if not found
		pNodeId.Insert(0, KRootNode);		
	}	
	
	// Create channel
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
	
	TPtrC8 aEncId = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId());
	
	_LIT8(KDiscoItemsStanza, "<iq to='' type='get' id='followchannel:%d'><query xmlns='http://jabber.org/protocol/disco#info' node=''/></iq>\r\n");
	HBufC8* aDiscoItemsStanza = HBufC8::NewLC(KDiscoItemsStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length() + 32);
	TPtr8 pDiscoItemsStanza(aDiscoItemsStanza->Des());
	pDiscoItemsStanza.AppendFormat(KDiscoItemsStanza, aChannelItem->GetItemId());
	pDiscoItemsStanza.Insert(pDiscoItemsStanza.Length() - 10, aEncId);
	pDiscoItemsStanza.Insert(8, KBuddycloudPubsubServer);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pDiscoItemsStanza, this, true);
	CleanupStack::PopAndDestroy(3); // aDiscoItemsStanza, aNodeId, aTextUtilities
}

void CBuddycloudLogic::UnfollowChannelL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnfollowChannelL"));
#endif
	
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));

	if(aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		if(aChannelItem->GetId().Length() > 0) {
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();			
			HBufC8* aEncNode = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()).AllocLC();
			
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
				TPtrC8 aEncId(aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()));

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
			
			CleanupStack::PopAndDestroy(2); // aEncNode, aTextUtilities
			
			// Delete discussion
			iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());
		}
		
		if(aItem->GetItemType() == EItemChannel) {
			iFollowingList->DeleteItemById(aItemId);
			
			NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);		
			
			iSettingsSaveNeeded = true;
		}
	}
}

void CBuddycloudLogic::PublishChannelMetadataL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::PublishChannelMetadataL"));
#endif
	
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
	
	if(aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		if(aChannelItem->GetPubsubAffiliation() == EPubsubAffiliationOwner) {		
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			
			HBufC8* aEncTitle = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()).AllocLC();
			HBufC8* aEncDescription = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetDescription()).AllocLC();
			TPtrC8 aEncId(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()));
			TPtrC8 aAccess = CXmppEnumerationConverter::PubsubAccessModel(aChannelItem->GetAccessModel());
			
			// Send stanza
			_LIT8(KEditStanza, "<iq to='' type='set' id='editchannel:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub#owner'><configure node=''><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE' type='hidden'><value>http://jabber.org/protocol/pubsub#node_config</value></field><field var='pubsub#access_model'><value></value></field><field var='pubsub#title'><value></value></field><field var='pubsub#description'><value></value></field></x></configure></pubsub></iq>\r\n");
			HBufC8* aEditStanza = HBufC8::NewLC(KEditStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length() + aAccess.Length() + aEncTitle->Des().Length() + aEncDescription->Des().Length());
			TPtr8 pEditStanza(aEditStanza->Des());
			pEditStanza.AppendFormat(KEditStanza, GetNewIdStamp());
			pEditStanza.Insert(403, *aEncDescription);
			pEditStanza.Insert(349, *aEncTitle);
			pEditStanza.Insert(300, aAccess);
			pEditStanza.Insert(116, aEncId);
			pEditStanza.Insert(8, KBuddycloudPubsubServer);
		
			iXmppEngine->SendAndForgetXmppStanza(pEditStanza, true);
			CleanupStack::PopAndDestroy(4); // aEditStanza, aEncDescription, aEncTitle, aTextUtilities
		}
	}
}

TInt CBuddycloudLogic::CreateChannelL(CFollowingChannelItem* aChannelItem) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CreateChannelL"));
#endif
	
	_LIT(KRootNode, "/channel/");	

	if(aChannelItem->GetItemId() <= 0 && aChannelItem->GetId().Length() > 0) {
		if(iFollowingList->GetItemById(aChannelItem->GetItemId()) == NULL) {
			iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
		}
		
		aChannelItem->SetItemId(iNextItemId++);
		aChannelItem->SetPubsubAffiliation(EPubsubAffiliationOwner);
		aChannelItem->SetPubsubSubscription(EPubsubSubscriptionSubscribed);
		
		// Create correct node id
		if(aChannelItem->GetId().Find(KRootNode) != 0) {
			HBufC* aNodeId = HBufC::NewLC(KRootNode().Length() + aChannelItem->GetId().Length());
			TPtr pNodeId(aNodeId->Des());
			pNodeId.Append(KRootNode);
			pNodeId.Append(aChannelItem->GetId());
			
			aChannelItem->SetIdL(pNodeId);
			CleanupStack::PopAndDestroy(); // aNodeId
		}
		
		// Add to discussion
		CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
		aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());
		
		aChannelItem->SetUnreadData(aDiscussion);
			
		// Get data
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncTitle = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()).AllocLC();
		HBufC8* aEncDescription = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetDescription()).AllocLC();
		TPtrC8 aEncId(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()));
		TPtrC8 aAccess = CXmppEnumerationConverter::PubsubAccessModel(aChannelItem->GetAccessModel());
		
		// Send stanza
		_LIT8(KCreateStanza, "<iq to='' type='set' id='createchannel:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><create node=''/><configure><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE' type='hidden'><value>http://jabber.org/protocol/pubsub#node_config</value></field><field var='pubsub#access_model'><value></value></field><field var='pubsub#title'><value></value></field><field var='pubsub#description'><value></value></field></x></configure></pubsub></iq>\r\n");
		HBufC8* aCreateStanza = HBufC8::NewLC(KCreateStanza().Length() + KBuddycloudPubsubServer().Length() + aEncId.Length() + aAccess.Length() + aEncTitle->Des().Length() + aEncDescription->Des().Length());
		TPtr8 pCreateStanza(aCreateStanza->Des());
		pCreateStanza.AppendFormat(KCreateStanza, GetNewIdStamp());
		pCreateStanza.Insert(409, *aEncDescription);
		pCreateStanza.Insert(354, *aEncTitle);
		pCreateStanza.Insert(305, aAccess);
		pCreateStanza.Insert(107, aEncId);
		pCreateStanza.Insert(8, KBuddycloudPubsubServer);
	
		iXmppEngine->SendAndForgetXmppStanza(pCreateStanza, true);
		CleanupStack::PopAndDestroy(4); // aCreateStanza, aEncDescription, aEncTitle, aTextUtilities
	}
	
	return aChannelItem->GetItemId();
}

void CBuddycloudLogic::SetMoodL(TDesC& aMood) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetMoodL"));
#endif

	if(iOwnItem) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()).AllocLC();
		TPtrC8 pEncJid(aEncJid->Des());
		
		TPtrC8 aEncodedMood(aTextUtilities->UnicodeToUtf8L(aMood));
	
		_LIT8(KMoodStanza, "<iq to='' type='set' id='setmood:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node='/user//mood'><item><mood xmlns='http://jabber.org/protocol/mood'><undefined/><text></text></mood></item></publish></pubsub></iq>\r\n");
		HBufC8* aMoodStanza = HBufC8::NewLC(KMoodStanza().Length() + KBuddycloudPubsubServer().Length() + pEncJid.Length() + aEncodedMood.Length());
		TPtr8 pMoodStanza(aMoodStanza->Des());
		pMoodStanza.AppendFormat(KMoodStanza, GetNewIdStamp());
		pMoodStanza.Insert(185, aEncodedMood);
		pMoodStanza.Insert(108, pEncJid);
		pMoodStanza.Insert(8, KBuddycloudPubsubServer);
	
		iXmppEngine->SendAndAcknowledgeXmppStanza(pMoodStanza, this, true);
		CleanupStack::PopAndDestroy(3); // aStanza, aEncJid, aTextUtilities
		
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
	HBufC* aNewPlace = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_ITEM_NEWPLACE);
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
		SendPlaceQueryL(aPlaceId, KPlaceCurrent, true);
		
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
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	TPtrC8 aEncodedPlace(aTextUtilities->UnicodeToUtf8L(aPlace));
	
	_LIT8(KNextStanza, "<iq to='butler.buddycloud.com' type='set' id='setnext1'><query xmlns='http://buddycloud.com/protocol/place#next'><place><name></name></place></query></iq>\r\n");
	HBufC8* aNextStanza = HBufC8::NewLC(KNextStanza().Length() + aEncodedPlace.Length() + pPlaceIdElement.Length());
	TPtr8 pNextStanza(aNextStanza->Des());
	pNextStanza.Append(KNextStanza);
	pNextStanza.Insert(133, pPlaceIdElement);
	pNextStanza.Insert(126, aEncodedPlace);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pNextStanza, this, true);
	CleanupStack::PopAndDestroy(3); // aNextStanza, aTextUtilities, aPlaceIdElement
	
	// Set Activity Status
	if(iState == ELogicOnline) {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
	}
}

TInt CBuddycloudLogic::GetBubbleToPosition(CFollowingItem* aBubblingItem) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::GetBubbleToPosition"));
#endif

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
			else if(aListItem->GetItemType() == EItemContact) {
				return i;
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

/*
----------------------------------------------------------------------------
--
-- Contact Services
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::PairItemIdsL(TInt aPairItemId1, TInt aPairItemId2) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::PairItemIdsL"));
#endif

	
	CFollowingItem* aPair1 = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aPairItemId1));
	CFollowingItem* aPair2 = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aPairItemId2));

	if(aPair1 && aPair2) {
		if(aPair1->GetItemType() >= EItemContact && aPair2->GetItemType() >= EItemContact) {
			if(aPair1->GetItemType() != aPair2->GetItemType()) {
				CFollowingContactItem* aContactItem;
				CFollowingRosterItem* aRosterItem;

				// Define ContactItem & RosterItem
				if(aPair1->GetItemType() == EItemRoster) {
					// Swap over to ease copying
					aContactItem = static_cast <CFollowingContactItem*> (aPair2);
					aRosterItem = static_cast <CFollowingRosterItem*> (aPair1);
				}
				else {
					aContactItem = static_cast <CFollowingContactItem*> (aPair1);
					aRosterItem = static_cast <CFollowingRosterItem*> (aPair2);
				}

				if(aRosterItem->GetAddressbookId() >= 0 && iSettingShowContacts) {
					CFollowingContactItem* aBuddycloudContactItem = CFollowingContactItem::NewLC();
					aBuddycloudContactItem->SetItemId(iNextItemId++);
					aBuddycloudContactItem->SetAddressbookId(aRosterItem->GetAddressbookId());
					iFollowingList->InsertItem(iFollowingList->GetIndexById(aContactItem->GetItemId()), aBuddycloudContactItem);
					CleanupStack::Pop();
				}

				// Copy data
				aRosterItem->SetLastUpdated(TimeStamp());
				aRosterItem->SetAddressbookId(aContactItem->GetAddressbookId());

				// Delete contact item
				iFollowingList->DeleteItemById(aContactItem->GetItemId());
			}
		}
	}
}

void CBuddycloudLogic::UnpairItemsL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnpairItemsL"));
#endif

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));

	if(aItem && aItem->GetItemType() == EItemRoster) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

		if(aRosterItem->GetAddressbookId() >= 0) {
			if(iSettingShowContacts) {
				// Create new ContactItem
				CFollowingContactItem* aContactItem = CFollowingContactItem::NewLC();
				aContactItem->SetItemId(iNextItemId++);
				aContactItem->SetAddressbookId(aRosterItem->GetAddressbookId());
				
				iFollowingList->AddItem(aContactItem);
				CleanupStack::Pop();
			}

			// Clean existing RosterItem
			aRosterItem->SetAddressbookId(KErrNotFound);
		}
	}
}

void CBuddycloudLogic::LoadAddressBookContacts() {
	if( !iContactsLoaded ) {
		iContactsLoaded = true;

		TRAPD(aErr, LoadAddressBookContactsL());

		if(aErr != KErrNone) {
			TBuf8<256> aLog;
			aLog.Format(_L8("LOGIC CBuddycloudLogic::LoadAddressBookContacts Trap: %d"), aErr);
			WriteToLog(aLog);
		}
	}
}

void CBuddycloudLogic::RemoveAddressBookContacts() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RemoveAddressBookContactsL"));
#endif
	
	if(iContactStreamer) {
		delete iContactStreamer;
		iContactStreamer = NULL;
	}
	
	for(TInt i = iFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem && aItem->GetItemType() == EItemContact) {
			iFollowingList->DeleteItemByIndex(i);
		}
	}
	
	iContactsLoaded = false;
}

void CBuddycloudLogic::LoadAddressBookContactsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::LoadAddressBookContactsL"));
#endif

	// ---------------------------------------------------------------------
	// Get Sort Preference
	// ---------------------------------------------------------------------
	CPbkContactEngine* aPhonebookEngine = CPbkContactEngine::NewL();
	CleanupStack::PushL(aPhonebookEngine);

	if(aPhonebookEngine->NameDisplayOrderL() == CPbkContactEngine::EPbkNameOrderLastNameFirstName) {
		iContactNameOrder = ENameOrderLastFirst;
	}

	CleanupStack::PopAndDestroy(); // aPhonebookEngine

	// ---------------------------------------------------------------------
	// Sort and Start Contact Streamer
	// ---------------------------------------------------------------------
	CArrayFixFlat<TSortPref>* aSortOrder = new (ELeave) CArrayFixFlat<TSortPref>(4);
	CleanupStack::PushL(aSortOrder);

	if(iContactNameOrder == ENameOrderFirstLast) {
		aSortOrder->AppendL(TSortPref(KUidContactFieldGivenName, TSortPref::EAsc));
		aSortOrder->AppendL(TSortPref(KUidContactFieldFamilyName, TSortPref::EAsc));
	}
	else {
		aSortOrder->AppendL(TSortPref(KUidContactFieldFamilyName, TSortPref::EAsc));
		aSortOrder->AppendL(TSortPref(KUidContactFieldGivenName, TSortPref::EAsc));
	}
	
	aSortOrder->AppendL(TSortPref(KUidContactFieldPhoneNumber, TSortPref::EAsc));
	aSortOrder->AppendL(TSortPref(KUidContactFieldEMail, TSortPref::EAsc));

#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudContactStreamer::NewL"));
#endif
	iContactStreamer = CBuddycloudContactStreamer::NewL(iContactDatabase, this);
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudContactStreamer::SortAndStartL"));
#endif
	iContactStreamer->SortAndStartL(aSortOrder);
	CleanupStack::Pop(); // database owns aSortOrder
}

void CBuddycloudLogic::CallL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CallL"));
#endif

	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
	
	if(aItem && (aItem->GetItemType() == EItemContact || aItem->GetItemType() == EItemRoster)) {
		CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
		
		if(aContactItem->GetAddressbookId() > -1) {
			CContactItem* aContactDetails = GetContactDetailsLC(aContactItem->GetAddressbookId());

			if(aContactDetails != NULL) {
				CContactItemFieldSet& aContactDetailsFieldSet = aContactDetails->CardFields();
				
				// Get list of callers phone numbers
				RPointerArray<HBufC> aDynItems;				
				TInt aPosition = aContactDetailsFieldSet.FindNext(KUidContactFieldPhoneNumber);
				
				while(aPosition != KErrNotFound) {
					HBufC* aNumberLine = HBufC::NewLC(2 + aContactDetailsFieldSet[aPosition].Label().Length() + 1 + aContactDetailsFieldSet[aPosition].TextStorage()->Text().Length());
					TPtr pNumberLine(aNumberLine->Des());
					pNumberLine.Append(aContactDetailsFieldSet[aPosition].TextStorage()->Text());
					
					// Remove ' '
					TInt aSearch = KErrNotFound;
					while((aSearch = pNumberLine.Locate(' ')) != KErrNotFound) {
						pNumberLine.Delete(aSearch, 1);
					}
					
					// Remove '-'
					while((aSearch = pNumberLine.Locate('-')) != KErrNotFound) {
						pNumberLine.Delete(aSearch, 1);
					}
					
					pNumberLine.Insert(0, _L("\t"));
					pNumberLine.Insert(0, aContactDetailsFieldSet[aPosition].Label());
					pNumberLine.Insert(0, _L("0\t"));
					
					if(aContactDetailsFieldSet[aPosition].ContentType().ContainsFieldType(KUidContactFieldVCardMapPREF)) {
						aDynItems.Insert(aNumberLine, 0);
					}
					else {
						aDynItems.Append(aNumberLine);
					}
					
					CleanupStack::Pop();
					
					aPosition = aContactDetailsFieldSet.FindNext(KUidContactFieldPhoneNumber, aPosition+1);
				}
				
				// Get callers name
				iTelephonyEngine->SetCallerDetailsL(aItemId, aContactItem->GetTitle());
				
				// Selection result
				TInt aSelectedItem = DisplayDoubleLinePopupMenuL(aDynItems);
				
				if(aSelectedItem != KErrNotFound) {
					TPtr pSelectedText(aDynItems[aSelectedItem]->Des());
					pSelectedText.Delete(0, pSelectedText.LocateReverse('\t') + 1);
					
					iTelephonyEngine->DialNumber(pSelectedText);
				}
					
				while(aDynItems.Count() > 0) {
					delete aDynItems[0];
					aDynItems.Remove(0);
				}
					
				aDynItems.Close();	
				CleanupStack::PopAndDestroy(); // aContactDetails
			}
		}
	}
}

CTelephonyEngine* CBuddycloudLogic::GetCall() {
	return iTelephonyEngine;
}

TDesC& CBuddycloudLogic::GetContactFilter() {
	return *iContactFilter;
}

void CBuddycloudLogic::SetContactFilterL(const TDesC& aFilter) {
	TPtr pContactFilter(iContactFilter->Des());

	if(pContactFilter.Compare(aFilter) != 0) {
		if(iContactFilter) {
			delete iContactFilter;
			iContactFilter = NULL;
		}

		iContactFilter = aFilter.AllocL();

		TRAPD(aErr, FilterContactsL());

		if(aErr != KErrNone) {
			TBuf8<256> aLog;
			aLog.Format(_L8("LOGIC CBuddycloudLogic::FilterContacts Trap: %d"), aErr);
			WriteToLog(aLog);
		}
	}	
}

void CBuddycloudLogic::FilterContactsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FilterContactsL"));
#endif

	TPtr pContactFilter(iContactFilter->Des());
	
	if(iContactSearcher == NULL) {
		iContactSearcher = CBuddycloudContactSearcher::NewL(iContactDatabase, this);	
	}
	
	iContactSearcher->StopSearch();

	if(pContactFilter.Length() == 0) {
		iFilteringContacts = false;

		// Reset Filter
		for(TInt i = 0; i < iFollowingList->Count(); i++) {
			iFollowingList->GetItemByIndex(i)->SetFiltered(true);
		}
	}
	else {
		TBool aToFilter;

		iFilteringContacts = true;

		for(TInt i = 0; i < iFollowingList->Count(); i++) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

			aToFilter = false;

			if(aItem->GetItemType() != EItemContact) {
				if(pContactFilter.Length() <= aItem->GetTitle().Length()) {
					// Item title
					if(aItem->GetTitle().FindF(pContactFilter) != KErrNotFound) {
						aToFilter = true;
					}
				}
				
				if(!aToFilter && aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					TPtrC aPlaceName(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText));
					
					// Place
					if(pContactFilter.Length() <= aPlaceName.Length()) {
						if(aPlaceName.FindF(pContactFilter) != KErrNotFound) {
							aToFilter = true;
						}
					}
				}
			}

			aItem->SetFiltered(aToFilter);
		}
		
		if(iSettingShowContacts) {
			iContactSearcher->StartSearchL(pContactFilter);
		}
	}

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

CContactItem* CBuddycloudLogic::GetContactDetailsLC(TInt aContactId) {
	CContactItem* aContact = NULL;
	
	TRAPD(aErr, aContact = iContactDatabase->ReadContactL(aContactId));
	
	if(aErr == KErrNotFound) {
		// Contact not found so clean the roster item
		for(TInt i = 0;i < iFollowingList->Count();i++) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

			if(aItem && aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetAddressbookId() == aContactId) {
					// Clean the roster item
					aRosterItem->SetAddressbookId(KErrNotFound);					
					break;
				}
			}
		}
	
		return NULL;
	}
	else {
		CleanupStack::PushL(aContact);
		return aContact;
	}
}

void CBuddycloudLogic::CollectContactDetailsL(TInt aFollowerId) {
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aFollowerId));

	if(aItem && aItem->GetItemType() == EItemContact) {
		CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
		
		CContactItem* aContact = iContactDatabase->ReadContactLC(aContactItem->GetAddressbookId());
		
		if(aContact) {
			TBuf<128> aTextBuffer;
			
			// Get title
			CContactTextDef* aContactTextDef = CContactTextDef::NewLC();
			if(iContactNameOrder == ENameOrderFirstLast) {
				aContactTextDef->AppendL(TContactTextDefItem(KUidContactFieldGivenName, _L(" ")));
				aContactTextDef->AppendL(TContactTextDefItem(KUidContactFieldFamilyName));
			}
			else {
				aContactTextDef->AppendL(TContactTextDefItem(KUidContactFieldFamilyName, _L(", ")));
				aContactTextDef->AppendL(TContactTextDefItem(KUidContactFieldGivenName));
			}
			
			aContactTextDef->SetFallbackField(KUidContactFieldCompanyName);
			iContactDatabase->ReadContactTextDefL(*aContact, aTextBuffer, aContactTextDef);
			aContactItem->SetTitleL(aTextBuffer);
			
			// Reset
			aContactTextDef->Reset();
			aTextBuffer.Zero();
			
			// Get description
			aContactTextDef->AppendL(TContactTextDefItem(KUidContactFieldPhoneNumber));
			aContactTextDef->SetFallbackField(KUidContactFieldEMail);
			iContactDatabase->ReadContactTextDefL(*aContact, aTextBuffer, aContactTextDef);
			aContactItem->SetDescriptionL(aTextBuffer);
			
			CleanupStack::PopAndDestroy(); // aContactTextDef
		}
		else {
			aContactItem->SetTitleL(_L("Empty"));
		}
		
		CleanupStack::PopAndDestroy(); // aContact
	}
}

void CBuddycloudLogic::HandleStreamingContactL(TInt aContactId, CContactItem* aContact) {
	if(aContact) {
		TBool aFound = false;
		
		for(TInt i = 0;i < iFollowingList->Count();i++) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetAddressbookId() == aContactId) {
					aFound = true;
					break;
				}
			}
		}

		// Not paired. Add as contact and index
		if(!aFound) {
			CFollowingContactItem* aContactItem = CFollowingContactItem::NewLC();
			aContactItem->SetItemId(iNextItemId++);
			aContactItem->SetAddressbookId(aContactId);
			aContactItem->SetFiltered(!iFilteringContacts);

			// Insert
			iFollowingList->AddItem(aContactItem);
			CleanupStack::Pop();
		}
	}
}

void CBuddycloudLogic::FinishedStreamingCycle() {
	if(iFilteringContacts) {
		FilterContactsL();
	}

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

void CBuddycloudLogic::FinishedContactSearch() {
	CContactIdArray* aContactArray = iContactSearcher->SearchResults();
	
	if(aContactArray) {
		CleanupStack::PushL(aContactArray);
		
		while(aContactArray->Count() > 0) {
			for(TInt i = 0;i < iFollowingList->Count();i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

				if(aItem && aItem->GetItemType() >= EItemContact) {
					CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);
					
					if(aContactItem->GetAddressbookId() > KErrNotFound) {
						if((*aContactArray)[0] == aContactItem->GetAddressbookId()) {
							// Contact Ids match
							aContactItem->SetFiltered(true);
							break;
						}
					}
				}
			}
			
			aContactArray->Remove(0);
		}
		
		CleanupStack::PopAndDestroy(); // aContactArray
	}

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

void CBuddycloudLogic::HandleDatabaseEventL(TContactDbObserverEvent aEvent) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleDatabaseEventL"));
#endif

	if(aEvent.iConnectionId != iContactDatabase->ConnectionId()) {
		if(aEvent.iType == EContactDbObserverEventContactAdded) {
			if(iSettingShowContacts) {
				CFollowingContactItem* aNewItem = CFollowingContactItem::NewLC();
				aNewItem->SetItemId(iNextItemId++);
				aNewItem->SetAddressbookId(aEvent.iContactId);
				
				iFollowingList->InsertItem(GetBubbleToPosition(aNewItem), aNewItem);
				CleanupStack::Pop();
			}
		}
		else if(aEvent.iType == EContactDbObserverEventContactDeleted) {
			for(TInt i = 0; i < iFollowingList->Count(); i++) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

				if(aItem && aItem->GetItemType() >= EItemContact) {
					CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);

					if(aContactItem->GetAddressbookId() == aEvent.iContactId) {
						if(aItem->GetItemType() == EItemContact) {
							iFollowingList->DeleteItemByIndex(i);
						}
						else if(aItem->GetItemType() == EItemRoster) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
							aRosterItem->SetAddressbookId(KErrNotFound);

							TPtrC aName(aRosterItem->GetId());
							TInt aSearchResult = aName.Locate('@');
							
							if(aSearchResult != KErrNotFound) {
								aName.Set(aName.Left(aSearchResult));
							}

							aRosterItem->SetTitleL(aName);
						}

						break;
					}
				}
			}
		}

		NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
	}
}

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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();

		if(aXmlParser->MoveToElement(_L8("logicstate"))) {
			// App version
			HBufC* aAppVersion = CEikonEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
			TPtrC aEncVersion(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("version"))));
			
			if(aEncVersion.Compare(*aAppVersion) != 0) {
				iSettingNewInstall = true;
			}
			
			WriteToLog(aTextUtilities->UnicodeToUtf8L(*aAppVersion));
			CleanupStack::PopAndDestroy(); // aAppVersion
			
			// Last node received
			SetLastNodeIdReceived(aXmlParser->GetStringAttribute(_L8("lastnodeid")));

			if(aXmlParser->MoveToElement(_L8("settings"))) {
				iSettingConnectionMode = aXmlParser->GetIntAttribute(_L8("apmode"));
				iSettingConnectionModeId = aXmlParser->GetIntAttribute(_L8("apid"));
				
				HBufC* aApName = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("apname"))).AllocLC();
				
				if(aXmlParser->MoveToElement(_L8("account"))) {
					iSettingFullName.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("fullname"))));	
					iSettingEmailAddress.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("email"))));
					iSettingUsername.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("username"))));
					iSettingPassword.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("password"))));										
					
					iSettingServerId = aXmlParser->GetIntAttribute(_L8("serverid"));

#ifndef _DEBUG
					if(iSettingServerId == 1) {
						// Reset beta.buddycloud.com to jabber.buddycloud.com
						iSettingServerId = 0;
					}
#endif
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
							// Custom languages: Pirate, Boarisch
							iSettingPreferredLanguage != 512 && 
							iSettingPreferredLanguage != 513) {
						
						iSettingPreferredLanguage = -1;
					}
					
					iSettingAutoStart = aXmlParser->GetBoolAttribute(_L8("autostart"));
					iSettingNotifyChannelsFollowing = aXmlParser->GetIntAttribute(_L8("notifyfollowing"));
					iSettingNotifyChannelsModerating = aXmlParser->GetIntAttribute(_L8("notifymoderating"), 1);
					iSettingNotifyReplyTo = aXmlParser->GetBoolAttribute(_L8("replyto"));
					iSettingShowName = aXmlParser->GetBoolAttribute(_L8("showname"));
					iSettingShowContacts = aXmlParser->GetBoolAttribute(_L8("showcontacts"));
					iSettingMessageBlocking = aXmlParser->GetBoolAttribute(_L8("messageblocking"));
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
			        iSettingPrivateMessageToneFile.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("pmt_file"))));
			        iSettingChannelPostToneFile.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("cmt_file"))));
			        iSettingDirectReplyToneFile.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("drt_file"))));
				}
				
				if(aXmlParser->MoveToElement(_L8("beacons"))) {					
					iSettingCellOn = aXmlParser->GetBoolAttribute(_L8("cell"));
					iSettingWlanOn = aXmlParser->GetBoolAttribute(_L8("wlan"));
					iSettingBtOn = aXmlParser->GetBoolAttribute(_L8("bt"));
					iSettingGpsOn = aXmlParser->GetBoolAttribute(_L8("gps"));
				}
				
				if(aXmlParser->MoveToElement(_L8("communities"))) {
					if(aXmlParser->MoveToElement(_L8("twitter"))) {
						iSettingTwitterUsername.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("username"))));
						iSettingTwitterPassword.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("password"))));
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
					aNoticeItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), TIdType(aItemXml->GetIntAttribute(_L8("type"))));
					aNoticeItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aNoticeItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					
					iFollowingList->AddItem(aNoticeItem);
					CleanupStack::Pop(); // aNoticeItem
				}
				else if(aItemXml->MoveToElement(_L8("group"))) {
					CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
					aChannelItem->SetItemId(iNextItemId++);
					aChannelItem->SetRank(aItemXml->GetIntAttribute(_L8("rank")), aItemXml->GetIntAttribute(_L8("shift")));						
					aChannelItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aChannelItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aChannelItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					aChannelItem->SetAccessModel((TXmppPubsubAccessModel)aItemXml->GetIntAttribute(_L8("access")));

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
					aRosterItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aRosterItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aRosterItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					aRosterItem->SetOwnItem(aItemXml->GetBoolAttribute(_L8("own")));

					if(aIconId != 0) {
						aRosterItem->SetIconId(aIconId);
					}
					
					// Addressbook Id
					TPtrC8 pAttributeAbid = aItemXml->GetStringAttribute(_L8("abid"));
					if(pAttributeAbid.Length() > 0) {
						aRosterItem->SetAddressbookId(aItemXml->GetIntAttribute(_L8("abid")));
					}
					
					CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());
					aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());			
					aDiscussion->SetUnreadData(aItemXml->GetIntAttribute(_L8("unreadprivate")));
					
					aRosterItem->SetUnreadData(aDiscussion);
					
					// Channel
					if(aItemXml->MoveToElement(_L8("channel"))) {
						aRosterItem->SetRank(aItemXml->GetIntAttribute(_L8("rank")), aItemXml->GetIntAttribute(_L8("shift")));						
						aRosterItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), EIdChannel);
						
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
							aGeoloc->SetStringL(EGeolocText, aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("text"))));
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

		CleanupStack::PopAndDestroy(4); // aTimeUtilities, aTextUtilities, aXmlParser, aBuf
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

		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		CTimeUtilities* aTimeUtilities = CTimeUtilities::NewLC();
		TFormattedTimeDesc aTime;

		aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>\r\n"));
		aFile.WriteL(_L8("\t<logicstate version='"));
		
		HBufC* aVersion = CEikonEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(*aVersion));
		CleanupStack::PopAndDestroy(); // aVersion
		
		aFile.WriteL(_L8("' lastnodeid='"));
		aFile.WriteL(*iLastNodeIdReceived);
		aFile.WriteL(_L8("'>\r\n"));

		// Settings
		aBuf.Format(_L8("\t\t<settings apmode='%d' apid='%d' apname='"), iSettingConnectionMode, iSettingConnectionModeId);
		aFile.WriteL(aBuf);
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iXmppEngine->GetConnectionName()));
		aFile.WriteL(_L8("'>\r\n\t\t\t<account"));

		if(iSettingFullName.Length() > 0) {
			aFile.WriteL(_L8(" fullname='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingFullName));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingEmailAddress.Length() > 0) {
			aFile.WriteL(_L8(" email='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingEmailAddress));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingUsername.Length() > 0) {
			aFile.WriteL(_L8(" username='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingUsername));
			aFile.WriteL(_L8("'"));
		}

		if(iSettingPassword.Length() > 0) {
			aFile.WriteL(_L8(" password='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingPassword));
			aFile.WriteL(_L8("'"));
		}
		
		// Preferences
		aBuf.Format(_L8(" serverid='%d'/>\r\n\t\t\t<preferences language='%d' autostart='%d' notifyfollowing='%d' notifymoderating='%d' accesspoint='%d' replyto='%d' showname='%d' showcontacts='%d' messageblocking='%d'/>\r\n"), iSettingServerId, iSettingPreferredLanguage, iSettingAutoStart, iSettingNotifyChannelsFollowing, iSettingNotifyChannelsModerating, iSettingAccessPoint, iSettingNotifyReplyTo, iSettingShowName, iSettingShowContacts, iSettingMessageBlocking);
		aFile.WriteL(aBuf);
		
		// Notifications
		aBuf.Format(_L8("\t\t\t<notifications pmt_id='%d' cmt_id='%d' drt_id='%d' pmt_file='"), iSettingPrivateMessageTone, iSettingChannelPostTone, iSettingDirectReplyTone);
		aFile.WriteL(aBuf);
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingPrivateMessageToneFile));
		aFile.WriteL(_L8("' cmt_file='"));
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingChannelPostToneFile));
		aFile.WriteL(_L8("' drt_file='"));
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingDirectReplyToneFile));
		
		// Beacons
		aBuf.Format(_L8("'/>\r\n\t\t\t<beacons cell='%d' wlan='%d' bt='%d' gps='%d'/>\r\n\t\t\t<communities>\r\n"), iSettingCellOn, iSettingWlanOn, iSettingBtOn, iSettingGpsOn);
		aFile.WriteL(aBuf);
		
		// Communities
		if(iSettingTwitterUsername.Length() > 0) {
			aFile.WriteL(_L8("\t\t\t\t<twitter username='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingTwitterUsername));
			aFile.WriteL(_L8("' password='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingTwitterPassword));
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

				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aItem->GetId()));
				aFile.WriteL(_L8("'"));

				if(aItem->GetTitle().Length() > 0) {
					aFile.WriteL(_L8(" name='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aItem->GetTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aItem->GetDescription().Length() > 0) {
					aFile.WriteL(_L8(" description='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aItem->GetDescription()));
					aFile.WriteL(_L8("'"));
				}
				
				aFile.WriteL(_L8("/>\r\n"));
			}
			else if(aItem->GetItemType() == EItemChannel) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);

				aBuf.Format(_L8("\t\t\t<group rank='%d' shift='%d' access='%d'"), aChannelItem->GetRank(), aChannelItem->GetRankShift(), aChannelItem->GetAccessModel());
				aFile.WriteL(aBuf);

				if(aChannelItem->GetId().Length() > 0) {
					aFile.WriteL(_L8(" jid='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()));
					aFile.WriteL(_L8("'"));
				}

				if(aChannelItem->GetTitle().Length() > 0) {
					aFile.WriteL(_L8(" name='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aChannelItem->GetDescription().Length() > 0) {
					aFile.WriteL(_L8(" description='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetDescription()));
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
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));
						aFile.WriteL(_L8("'"));
					}
					
					if(aRosterItem->GetTitle().Length() > 0) {
						aFile.WriteL(_L8(" name='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetTitle()));
						aFile.WriteL(_L8("'"));
					}
					
					if(aRosterItem->GetDescription().Length() > 0) {
						aFile.WriteL(_L8(" description='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetDescription()));
						aFile.WriteL(_L8("'"));
					}

					aBuf.Format(_L8(" own='%d'"), aRosterItem->OwnItem());
					aFile.WriteL(aBuf);

					if(aRosterItem->GetAddressbookId() != -1) {
						aBuf.Format(_L8(" abid='%d'"), aRosterItem->GetAddressbookId());
						aFile.WriteL(aBuf);
					}
					
					CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());

					if(aDiscussion->GetUnreadEntries() > 0) {
						aBuf.Format(_L8(" unreadprivate='%d'"), aDiscussion->GetUnreadEntries());
						aFile.WriteL(aBuf);
					}

					aFile.WriteL(_L8(">\r\n"));

					if(aRosterItem->GetId(EIdChannel).Length() > 0) {
						aBuf.Format(_L8("\t\t\t\t<channel rank='%d' shift='%d' jid='"), aRosterItem->GetRank(), aRosterItem->GetRankShift());
						aFile.WriteL(aBuf);
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetId(EIdChannel)));
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
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemPrevious)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					if(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<location type='1' text='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemCurrent)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					if(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText).Length() > 0) {
						aFile.WriteL(_L8("\t\t\t\t<location type='2' text='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetGeolocItem(EGeolocItemFuture)->GetString(EGeolocText)));
						aFile.WriteL(_L8("'/>\r\n"));
					}

					aFile.WriteL(_L8("\t\t\t</roster>\r\n"));
				}
			}
			
			aFile.WriteL(_L8("\t\t</item>\r\n"));
		}

		aFile.WriteL(_L8("\t</logicstate>\r\n</?xml?>"));
		
		WriteToLog(_L8("BL    Externalize Logic State -- End"));

		CleanupStack::PopAndDestroy(2); // aTimeUtilities, aTextUtilities
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::AddChannelItemL(TPtrC aId, TPtrC aName, TPtrC aTopic, TInt aRank) {
	if(!IsSubscribedTo(aId, EItemChannel)) {
		CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
		aChannelItem->SetItemId(iNextItemId++);
		aChannelItem->SetIdL(aId);
		aChannelItem->SetTitleL(aName);
		aChannelItem->SetDescriptionL(aTopic);
		aChannelItem->SetRank(aRank);
		
		CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aChannelItem->GetId());
		aDiscussion->SetDiscussionReadObserver(this, aChannelItem->GetItemId());			
		
		aChannelItem->SetUnreadData(aDiscussion);
	
		iFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
		CleanupStack::Pop(); // aChannelItem
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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
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

		CleanupStack::PopAndDestroy(4); // aTimeUtilities, aTextUtilities, aXmlParser, aBuf
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

		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
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
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocUri)));
				aFile.WriteL(_L8("</uri>\r\n\t\t\t\t<text>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocText)));
				aFile.WriteL(_L8("</text>\r\n\t\t\t\t<street>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocStreet)));
				aFile.WriteL(_L8("</street>\r\n\t\t\t\t<area>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocArea)));
				aFile.WriteL(_L8("</area>\r\n\t\t\t\t<locality>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocLocality)));
				aFile.WriteL(_L8("</locality>\r\n\t\t\t\t<postalcode>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocPostalcode)));
				aFile.WriteL(_L8("</postalcode>\r\n\t\t\t\t<region>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocRegion)));
				aFile.WriteL(_L8("</region>\r\n\t\t\t\t<country>"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetGeoloc()->GetString(EGeolocCountry)));
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

		CleanupStack::PopAndDestroy(2); // aTimeUtilities, aTextUtilities
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
			iLog.Write(_L(""));
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

CBuddycloudListStore* CBuddycloudLogic::GetFollowingStore() {
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
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
				TPtrC8 aEncJid(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetId()));

				_LIT8(KStanza, "<iq type='set' id='remove1'><query xmlns='jabber:iq:roster'><item subscription='remove' jid=''/></query></iq>\r\n");
				HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aEncJid.Length());
				TPtr8 pStanza(aStanza->Des());
				pStanza.Append(KStanza);
				pStanza.Insert(93, aEncJid);
				
				iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
				CleanupStack::PopAndDestroy(2); // aStanza, aTextUtilities
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
				(aItem->GetItemType() == EItemChannel && aItemOptions & EItemChannel) ||
				(aItem->GetItemType() == EItemRoster && aItemOptions & EItemRoster)) {
			
			if(aItem->GetItemType() != EItemRoster) {
				if(aItem->GetId().CompareF(aId) == 0) {
					// Check id
					return aItem->GetItemId();
				}		
			}
			else {
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
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
				
				if(aResponse <= ENoticeAcceptAndFollow) {
					// Accept request
					SendPresenceSubscriptionL(aTextUtilities->UnicodeToUtf8L(aItem->GetId()), _L8("subscribed"));
					
					if(aResponse == ENoticeAcceptAndFollow) {
						// Follow requester back
						FollowContactL(aItem->GetId());
					}
				}
				else {
					// Unsubscribe to presence
					SendPresenceSubscriptionL(aTextUtilities->UnicodeToUtf8L(aItem->GetId()), _L8("unsubscribed"));
				}
				
				CleanupStack::PopAndDestroy(); // aTextUtilities
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
	
	if(iOwnItem && aItem && aItem->GetItemId() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		if(aChannelItem->GetId().Length() > 0) {		
			CGeolocData* aGeoloc = iOwnItem->GetGeolocItem(EGeolocItemCurrent);
			
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
					
			HBufC8* aEncId = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetId()).AllocLC();
			TPtr8 pEncId(aEncId->Des());
			
			_LIT8(KMediaStanza, "<iq to='media.buddycloud.com' type='set' id='mediareq:%d'><media xmlns='http://buddycloud.com/media#request'><placeid>%d</placeid><lat>%.6f</lat><lon>%.6f</lon><node></node></media></iq>\r\n");
			HBufC8* aMediaStanza = HBufC8::NewLC(KMediaStanza().Length() + pEncId.Length() + 32 + 22);
			TPtr8 pMediaStanza(aMediaStanza->Des());
			pMediaStanza.Format(KMediaStanza, aItem->GetItemId(), iStablePlaceId, aGeoloc->GetReal(EGeolocLatitude),  aGeoloc->GetReal(EGeolocLongitude));
			pMediaStanza.Insert(pMediaStanza.Length() - 22, pEncId);
		
			iXmppEngine->SendAndAcknowledgeXmppStanza(pMediaStanza, this, true);
			CleanupStack::PopAndDestroy(3); // aMediaStanza, aEncId, aTextUtilities
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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
		
		HBufC8* aEncId = aTextUtilities->UnicodeToUtf8L(aId).AllocLC();
		TPtr8 pEncId(aEncId->Des());
				
		if(aItem->GetItemType() == EItemRoster && aItem->GetId().Compare(aId) != 0) {	
			// Send private message
			TFormattedTimeDesc aThreadData;
			TTime aCurrentTime = TimeStamp();
			
			CTimeUtilities::EncodeL(aCurrentTime, aThreadData);
			
			HBufC8* aEncContent = aTextUtilities->UnicodeToUtf8L(aContent).AllocLC();
			TPtr8 pEncContent(aEncContent->Des());
			
			CXmppGeolocParser* aGeolocParser = CXmppGeolocParser::NewLC();
			TPtrC8 aGeolocXml(aGeolocParser->GeolocToXmlL(iOwnItem->GetGeolocItem(EGeolocItemCurrent)));
			
			_LIT8(KThreadParent, " parent=''");
			_LIT8(KMessageStanza, "<message type='chat' to='' id='message'><body></body><thread></thread></message>\r\n");
			HBufC8* aMessageStanza = HBufC8::NewLC(KMessageStanza().Length() + pEncId.Length() + pEncContent.Length() + KThreadParent().Length() + aReferenceId.Length() + aThreadData.Length() + aGeolocXml.Length());
			TPtr8 pMessageStanza(aMessageStanza->Des());
			pMessageStanza.Append(KMessageStanza);
			pMessageStanza.Insert(70, aGeolocXml);
			pMessageStanza.Insert(61, aThreadData);
			
			if(aReferenceId.Length() > 0) {
				pMessageStanza.Insert(60, KThreadParent);
				pMessageStanza.Insert(69, aReferenceId);				
			}
			
			pMessageStanza.Insert(46, pEncContent);
			pMessageStanza.Insert(25, pEncId);
			
			iXmppEngine->SendAndForgetXmppStanza(pMessageStanza, true, EXmppPriorityHigh);			
			CleanupStack::PopAndDestroy(3); // aMessageStanza, aGeolocParser, aEncContent
			
			// Store private message
			CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
			aAtomEntry->SetIdL(aThreadData);
			aAtomEntry->SetPublishTime(aCurrentTime);
			aAtomEntry->SetAuthorJidL(iOwnItem->GetId(), false);	
			aAtomEntry->GetLocation()->SetStringL(EGeolocText, *iBroadLocationText);			
			aAtomEntry->SetContentL(aContent);			
			aAtomEntry->SetIconId(iOwnItem->GetIconId());	
			aAtomEntry->SetPrivate(true);
			aAtomEntry->SetRead(true);
			
			if(aItem->GetId().Compare(iOwnItem->GetId()) == 0) {
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
			aAtomEntry->SetAuthorJidL(iOwnItem->GetId(), (aItem->GetItemType() != EItemRoster));	
			aAtomEntry->SetContentL(aContent, EEntryUnprocessed);			

			CXmppAtomEntryParser* aAtomEntryParser = CXmppAtomEntryParser::NewLC();
			TPtrC8 aAtomEntryXml(aAtomEntryParser->AtomEntryToXmlL(aAtomEntry, aReferenceId));
			
			_LIT8(KPublishStanza, "<iq to='' type='set' id='publish:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node=''><item></item></publish></pubsub></iq>\r\n");
			HBufC8* aPublishStanza = HBufC8::NewLC(KPublishStanza().Length() + KBuddycloudPubsubServer().Length() + pEncId.Length() + aAtomEntryXml.Length());
			TPtr8 pPublishStanza(aPublishStanza->Des());
			pPublishStanza.AppendFormat(KPublishStanza, GetNewIdStamp());
			pPublishStanza.Insert(110, aAtomEntryXml);
			pPublishStanza.Insert(102, pEncId);
			pPublishStanza.Insert(8, KBuddycloudPubsubServer);
			
			iXmppEngine->SendAndForgetXmppStanza(pPublishStanza, true, EXmppPriorityHigh);			
			CleanupStack::PopAndDestroy(3); // aPublishStanza, aAtomEntryXml, aPublishStanza
		}
		
		CleanupStack::PopAndDestroy(2); // aEncId, aTextUtilities
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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncUsername = aTextUtilities->UnicodeToUtf8L(pCommunityUsername).AllocLC();
		TPtr8 pEncUsername(aEncUsername->Des());
		
		HBufC8* aEncPassword = aTextUtilities->UnicodeToUtf8L(pCommunityPassword).AllocLC();
		TPtr8 pEncPassword(aEncPassword->Des());

		_LIT8(KStanza, "<iq to='communities.buddycloud.com' type='set' id='credentials1'><credentials application='' username='' password=''/></iq>\r\n");
		HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aCommunityName.Length() + pEncUsername.Length() + pEncPassword.Length());
		TPtr8 pStanza = aStanza->Des();
		pStanza.Append(KStanza);
		pStanza.Insert(115, pEncPassword);
		pStanza.Insert(103, pEncUsername);
		pStanza.Insert(91, aCommunityName);

		iXmppEngine->SendAndAcknowledgeXmppStanza(pStanza, this, true);
		CleanupStack::PopAndDestroy(3); // aEncPassword, aEncUsername, aTextUtilities
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
-- Places
--
----------------------------------------------------------------------------
*/

TInt CBuddycloudLogic::GetMyPlaceId() {
	return iLocationEngine->GetLastPlaceId();
}

TLocationMotionState CBuddycloudLogic::GetMyMotionState() {
	return iLocationEngine->GetLastMotionState();
}

TInt CBuddycloudLogic::GetMyPatternQuality() {
	return iLocationEngine->GetLastPatternQuality();
}

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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		CGeolocData* aGeoloc = aEditingPlace->GetGeoloc();

		// -----------------------------------------------------------------
		// Get data from edit
		// -----------------------------------------------------------------
		// Name
		HBufC8* aPlaceName = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocText)).AllocLC();
		TPtr8 pPlaceName(aPlaceName->Des());

		// Street
		HBufC8* aPlaceStreet = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocStreet)).AllocLC();
		TPtr8 pPlaceStreet(aPlaceStreet->Des());

		// Area
		HBufC8* aPlaceArea = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocArea)).AllocLC();
		TPtr8 pPlaceArea(aPlaceArea->Des());

		// City
		HBufC8* aPlaceCity = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocLocality)).AllocLC();
		TPtr8 pPlaceCity(aPlaceCity->Des());

		// Postcode
		HBufC8* aPlacePostcode = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocPostalcode)).AllocLC();
		TPtr8 pPlacePostcode(aPlacePostcode->Des());

		// Region
		HBufC8* aPlaceRegion = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocRegion)).AllocLC();
		TPtr8 pPlaceRegion(aPlaceRegion->Des());

		// Country
		HBufC8* aPlaceCountry = aTextUtilities->UnicodeToUtf8L(aGeoloc->GetString(EGeolocCountry)).AllocLC();
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
		_LIT8(KCoreDetailsStanza, "<iq to='butler.buddycloud.com' type='' id='placequery1'><query xmlns='http://buddycloud.com/protocol/place#'><place><name></name><street></street><area></area><city></city><region></region><country></country><postalcode></postalcode></place></query></iq>\r\n");
		
		HBufC8* aCoreStanza = HBufC8::NewLC(KCoreDetailsStanza().Length() + pPlaceName.Length() + pPlaceStreet.Length() + 
				pPlaceArea.Length() + pPlaceCity.Length() + pPlacePostcode.Length() + pPlaceRegion.Length() + 
				pPlaceCountry.Length() + KLatitudeElement().Length() + aPlaceLatitude.Length() + KLongitudeElement().Length() + aPlaceLongitude.Length());		
		TPtr8 pCoreStanza(aCoreStanza->Des());
		pCoreStanza.Append(KCoreDetailsStanza);
		pCoreStanza.Insert(220, pPlacePostcode);
		pCoreStanza.Insert(198, pPlaceCountry);
		pCoreStanza.Insert(180, pPlaceRegion);
		pCoreStanza.Insert(165, pPlaceCity);
		pCoreStanza.Insert(152, pPlaceArea);
		pCoreStanza.Insert(137, pPlaceStreet);
		pCoreStanza.Insert(122, pPlaceName);
	
		if(aGeoloc->GetReal(EGeolocLatitude) != 0.0 || aGeoloc->GetReal(EGeolocLongitude) != 0.0) {
			pCoreStanza.Insert(pCoreStanza.Length() - 23, KLongitudeElement);
			pCoreStanza.Insert(pCoreStanza.Length() - 29, aPlaceLongitude);
			pCoreStanza.Insert(pCoreStanza.Length() - 23, KLatitudeElement);
			pCoreStanza.Insert(pCoreStanza.Length() - 29, aPlaceLatitude);
		}

		if(aSearchOnly) {
			HBufC8* aSearchStanza = HBufC8::NewLC(pCoreStanza.Length() + (KPlaceSearch().Length() * 2) + KIqTypeGet().Length());
			TPtr8 pSearchStanza(aSearchStanza->Des());							
			pSearchStanza.Append(pCoreStanza);
			pSearchStanza.Insert(107, KPlaceSearch);
			pSearchStanza.Insert(43, KPlaceSearch);
			pSearchStanza.Insert(37, KIqTypeGet);
			
			iXmppEngine->SendAndAcknowledgeXmppStanza(pSearchStanza, this);
			CleanupStack::PopAndDestroy(); // aSearchStanza
		}
		else {		
			TBuf8<20> aSharedElement;
			aSharedElement.Format(_L8("<shared>%d</shared>"), aEditingPlace->Shared());
			
			if(aEditingPlace->GetItemId() < 0) {
				HBufC8* aCreateStanza = HBufC8::NewLC(pCoreStanza.Length() + (KPlaceCreate().Length() * 2) + aSharedElement.Length() + KIqTypeSet().Length());
				TPtr8 pCreateStanza(aCreateStanza->Des());							
				pCreateStanza.Append(pCoreStanza);
				pCreateStanza.Insert(116, aSharedElement);
				pCreateStanza.Insert(107, KPlaceCreate);
				pCreateStanza.Insert(43, KPlaceCreate);
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
				
				HBufC8* aEditStanza = HBufC8::NewLC(pCoreStanza.Length() + (KPlaceEdit().Length() * 2) + aSharedElement.Length() + aIdElement.Length() + KIqTypeSet().Length());
				TPtr8 pEditStanza(aEditStanza->Des());							
				pEditStanza.Append(pCoreStanza);
				pEditStanza.Insert(116, aSharedElement);
				pEditStanza.Insert(116, aIdElement);
				pEditStanza.Insert(107, KPlaceEdit);
				pEditStanza.Insert(43, KPlaceEdit);
				pEditStanza.Insert(37, KIqTypeSet);
	
				iXmppEngine->SendAndAcknowledgeXmppStanza(pEditStanza, this, true);
				CleanupStack::PopAndDestroy(); // aEditStanza
				
				iPlaceList->SetEditedPlace(KErrNotFound);
			}
		}

		CleanupStack::PopAndDestroy(9); // aCoreStanza, aPlaceCountry, aPlaceRegion, aPlacePostcode, aPlaceCity, aPlaceArea, aPlaceStreet, aPlaceName, aTextUtilities
	}
}

void CBuddycloudLogic::HandlePlaceQueryResultL(const TDesC8& aStanza, const TDesC8& aId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandlePlaceQueryResultL"));
#endif
	
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aXmlParser->MoveToElement(_L8("query"))) {
		CBuddycloudPlaceStore* aPlaceStore = CBuddycloudPlaceStore::NewLC();
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
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
				TPtrC aElementData = aTextUtilities->Utf8ToUnicodeL(aPlaceXmlParser->GetStringData());
				
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
		
		if(aId.Compare(_L8("myplaces1")) == 0) {
			ProcessMyPlacesL(aPlaceStore);
		}
		else if(aId.Compare(_L8("searchplacequery1")) == 0) {
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
					else if(aId.Compare(_L8("placedetails1")) == 0) {
						// New place details received, copy to existing place
						aPlace->CopyGeolocL(aResultPlace->GetGeoloc());
						aPlace->UpdateFromGeolocL();
						
						// Extra data
						aPlace->SetShared(aResultPlace->Shared());
						aPlace->SetRevision(aResultPlace->GetRevision());
						aPlace->SetPopulation(aResultPlace->GetPopulation());
					}
					
					// Current place set by placeid
					if(aId.Compare(_L8("currentplacequery1")) == 0) {
						HandleLocationServerResult(EMotionStationary, 100, aResultPlace->GetItemId());
						SetActivityStatus(_L(""));
					}
					else if(aId.Compare(_L8("createplacequery1")) == 0) {
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
					if(aId.Compare(_L8("placedetails1")) == 0 && aXmlParser->MoveToElement(_L8("not-authorized"))) {
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
		
		CleanupStack::PopAndDestroy(2); // aTextUtilities, aPlaceStore
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
	_LIT8(KPlaceDetailsStanza, "<iq to='butler.buddycloud.com' type='get' id='placedetails1'><query xmlns='http://buddycloud.com/protocol/place'><place><id>http://buddycloud.com/places/%d</id></place></query></iq>\r\n");
	HBufC8* aPlaceDetailsStanza = HBufC8::NewLC(KPlaceDetailsStanza().Length() + 32);
	TPtr8 pPlaceDetailsStanza(aPlaceDetailsStanza->Des());
	pPlaceDetailsStanza.Format(KPlaceDetailsStanza, aPlaceId);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pPlaceDetailsStanza, this, true);
	CleanupStack::PopAndDestroy();
}

void CBuddycloudLogic::EditMyPlacesL(TInt aPlaceId, TBool aAddPlace) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::EditMyPlacesL"));
#endif
	
	CBuddycloudExtendedPlace* aPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemById(aPlaceId));

	if(aAddPlace && aPlace == NULL) {
		// Add place to 'my places'
		SendPlaceQueryL(aPlaceId, KPlaceAdd, true);
	}
	else if(!aAddPlace && aPlace) {
		if(aPlace->Shared()) {
			// Remove place from 'my places'
			SendPlaceQueryL(aPlaceId, KPlaceRemove);
		}
		else {
			// Delete private place
			SendPlaceQueryL(aPlaceId, KPlaceDelete);
		}
		
		// Remove place
		iPlaceList->DeleteItemById(aPlaceId);
	}
	
	NotifyNotificationObservers(ENotificationPlaceItemsUpdated, aPlaceId);
}

void CBuddycloudLogic::CollectMyPlacesL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectMyPlacesL"));
#endif
	
	_LIT8(KMyPlacesStanza, "<iq to='butler.buddycloud.com' type='get' id='myplaces1'><query xmlns='http://buddycloud.com/protocol/place#myplaces'><options><feature var='id'/><feature var='revision'/><feature var='population'/></options></query></iq>\r\n");
	iXmppEngine->SendAndAcknowledgeXmppStanza(KMyPlacesStanza, this, false, EXmppPriorityHigh);
}

void CBuddycloudLogic::ProcessMyPlacesL(CBuddycloudPlaceStore* aPlaceStore) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessMyPlacesL"));
#endif
	
	for(TInt i = (iPlaceList->Count() - 1); i >= 0; i--) {
		CBuddycloudExtendedPlace* aLocalPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetItemByIndex(i));
		CBuddycloudExtendedPlace* aStorePlace = static_cast <CBuddycloudExtendedPlace*> (aPlaceStore->GetItemById(aLocalPlace->GetItemId()));
		
		if(aLocalPlace->GetItemId() != 0) {
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

	SetActivityStatus(_L(""));
	
	CBuddycloudExtendedPlace* aEditingPlace = static_cast <CBuddycloudExtendedPlace*> (iPlaceList->GetEditedPlace());
	
	if(aEditingPlace) {	
		TInt aEditingPlaceId = aEditingPlace->GetItemId();
		
		if(aPlaceStore->Count() > 0) {
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			
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
					aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocText), _L(""));
					aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocStreet), _L(",\n"));
					aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocLocality), _L(",\n"));
					aTextUtilities->AppendToString(pMessage, aGeoloc->GetString(EGeolocPostalcode), _L(" "));
						
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
			
			CleanupStack::PopAndDestroy(); // aTextUtilities
		}
		else {
			// No results
			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);
			CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage
		}
	}
}

void CBuddycloudLogic::SendPlaceQueryL(TInt aPlaceId, const TDesC8& aAction, TBool aAcknowledge) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPlaceQueryL"));
#endif
	
	_LIT8(KEditMyPlacesStanza, "<iq to='butler.buddycloud.com' type='set' id='placequery1'><query xmlns='http://buddycloud.com/protocol/place#'><place><id>http://buddycloud.com/places/%d</id></place></query></iq>\r\n");
	HBufC8* aEditMyPlacesStanza = HBufC8::NewLC(KEditMyPlacesStanza().Length() + (aAction.Length() * 2) + 32);
	TPtr8 pEditMyPlacesStanza(aEditMyPlacesStanza->Des());
	pEditMyPlacesStanza.Format(KEditMyPlacesStanza, aPlaceId);
	pEditMyPlacesStanza.Insert(110, aAction);
	pEditMyPlacesStanza.Insert(46, aAction);
	
	if(aAcknowledge) {
		iXmppEngine->SendAndAcknowledgeXmppStanza(pEditMyPlacesStanza, this, true);
	}
	else {
		iXmppEngine->SendAndForgetXmppStanza(pEditMyPlacesStanza, true);
	}
	
	CleanupStack::PopAndDestroy(); // aEditMyPlacesStanza			
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
			SetActivityStatus(_L(""));
			
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
-- MTelephonyEngineNotification
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::TelephonyStateChanged(TTelephonyEngineState /*aState*/) {
	NotifyNotificationObservers(ENotificationTelephonyChanged);
}

void CBuddycloudLogic::TelephonyDebug(const TDesC8& aMessage) {
	WriteToLog(aMessage);
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
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		TPtrC8 aReferenceJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId());
		
		_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='nearbyplaces1'><query xmlns='urn:oslo:nearbyobjects'><reference type='person' id=''/><options limit='10'/><request var='place'/></query></iq>\r\n");
		HBufC8* aNearbyStanza = HBufC8::NewLC(KNearbyStanza().Length() + aReferenceJid.Length());
		TPtr8 pNearbyStanza(aNearbyStanza->Des());
		pNearbyStanza.Append(KNearbyStanza);
		pNearbyStanza.Insert(128, aReferenceJid);
		
		iXmppEngine->SendAndAcknowledgeXmppStanza(pNearbyStanza, this);	
		CleanupStack::PopAndDestroy(2); // aNearbyStanza, aTextUtilities
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
		iOwnerObserver->StateChanged(iState);
		iXmppEngine->GetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId);
		iLastMotionState = EMotionUnknown;
		
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
			}
			
			// Collect my places
			CollectMyPlacesL();
			
			iConnectionCold = false;
		}
		else {
			// Send presence to pubsub
			SendPresenceToPubsubL();
		}
		
		iXmppEngine->SendQueuedXmppStanzas();
		
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
			iXmppEngine->SetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId, _L(""));
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

					// Reset presence & pubsub
					aRosterItem->SetPresence(EPresenceOffline);
					aRosterItem->SetPubsubCollected(false);
				}
			}
		}

		// Store state
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
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	TPtrC8 aAttributeFrom = aXmlParser->GetStringAttribute(_L8("from"));
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	TPtrC8 aAttributeId = aXmlParser->GetStringAttribute(_L8("id"));

	if(aXmlParser->MoveToElement(_L8("iq"))) {
		if(aXmlParser->MoveToElement(_L8("pubsub"))) {
			if(aAttributeType.Compare(_L8("result")) == 0) {
				// Pubsub query result
				HandlePubsubEventL(aStanza, false);
			}
			else if(aAttributeType.Compare(_L8("error")) == 0) {		
				// TODO: Handle pubsub query error
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
	
						// Re-start Location Engine
						iLocationEngine->TriggerEngine();
		
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
					RosterItemsL(aXmlParser->GetStringData(), true);
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
					TPtrC8 aAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));
					
					if(aAttributeXmlns.Compare(_L8("http://jabber.org/protocol/pubsub#event")) == 0) {
						// Handle a pubsub event
						HandlePubsubEventL(aStanza, true);
					}
				}
				else if(aXmlParser->MoveToElement(_L8("x"))) {
					// Handle subscription request message
					HandlePubsubRequestL(aStanza);
				}
				
			}
			else {
				HandleIncomingMessageL(aTextUtilities->Utf8ToUnicodeL(aAttributeFrom), aStanza);
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
				ProcessPresenceSubscriptionL(aTextUtilities->Utf8ToUnicodeL(aAttributeFrom), aStanza);
			}
		}
		else {
			HandleIncomingPresenceL(aTextUtilities->Utf8ToUnicodeL(aAttributeFrom), aStanza);
		}
	}

	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
}

void CBuddycloudLogic::XmppError(TXmppEngineError aError) {
	if(aError == EXmppBadAuthorization) {
		// Send registration
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncUsername = aTextUtilities->UnicodeToUtf8L(iSettingUsername).AllocLC();
		TPtrC8 pEncUsername(aEncUsername->Des());
		
		HBufC8* aEncPassword = aTextUtilities->UnicodeToUtf8L(iSettingPassword).AllocLC();
		TPtrC8 pEncPassword(aEncPassword->Des());
		
		HBufC8* aEncEmailAddress = aTextUtilities->UnicodeToUtf8L(iSettingEmailAddress).AllocLC();
		TPtrC8 pEncEmailAddress(aEncEmailAddress->Des());
		
		HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtrC8 pEncFirstName(aEncFullName->Des());
		TPtrC8 pEncLastName;
		TInt aLocateResult;
		
		if((aLocateResult = pEncFirstName.Locate(' ')) != KErrNotFound) {
			pEncLastName.Set(pEncFirstName.Mid(aLocateResult + 1));
			pEncFirstName.Set(pEncFirstName.Left(aLocateResult));
		}
		
		_LIT8(KFirstNameElement, "<firstname></firstname>");
		_LIT8(KLastNameElement, "<lastname></lastname>");
		_LIT8(KEmailElement, "<email></email>");
		
		_LIT8(KRegisterStanza, "<iq type='set' id='registration1'><query xmlns='jabber:iq:register'><username></username><password></password></query></iq>\r\n");
		HBufC8* aRegisterStanza = HBufC8::NewLC(KRegisterStanza().Length() + pEncUsername.Length() + pEncPassword.Length() + KFirstNameElement().Length() + pEncFirstName.Length() + KLastNameElement().Length() + pEncLastName.Length() + KEmailElement().Length() + pEncEmailAddress.Length());
		TPtr8 pRegisterStanza(aRegisterStanza->Des());
		pRegisterStanza.Copy(KRegisterStanza);
		
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
		
		// Email
		if(pEncEmailAddress.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KEmailElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 8, pEncEmailAddress);
		}
		
		pRegisterStanza.Insert(99, pEncPassword);
		pRegisterStanza.Insert(78, pEncUsername);

		iXmppEngine->SendAndAcknowledgeXmppStanza(pRegisterStanza, this, false, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(6); // aRegisterStanza, aEncFullName, aEncEmailAddress, aEncPassword, aEncUsername, aTextUtilities
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
-- MXmppItemsObserver
--
----------------------------------------------------------------------------
*/

void CBuddycloudLogic::RosterItemsL(const TDesC8& aItems, TBool aPush) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterItemsL"));
#endif
	
	TInt aNotifyId = KErrNotFound;
	
	CBuddycloudListStore* aRosterItemStore = CBuddycloudListStore::NewLC();
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aItems);
	
	// Collect roster items
	while(aXmlParser->MoveToElement(_L8("item"))) {
		TPtrC8 aJid(aXmlParser->GetStringAttribute(_L8("jid")));
		
		CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("name"))));	
		aRosterItem->SetIdL(aTextUtilities->Utf8ToUnicodeL(aJid));
		aRosterItem->SetSubscription(CXmppEnumerationConverter::PresenceSubscription(aXmlParser->GetStringAttribute(_L8("subscription"))));
		
		aRosterItemStore->AddItem(aRosterItem);
		CleanupStack::Pop(aRosterItem); 
		
		// Determine if is pubsub subscription
		if(aJid.Compare(KBuddycloudPubsubServer) == 0) {					
			if(!aPush || aRosterItem->GetSubscription() == EPresenceSubscriptionBoth) {			
				if(aPush && !iPubsubSubscribedTo) {
					// Collect pubsub subscriptions
					CollectPubsubSubscriptionsL();
					
					// Trigger location engine
					iLocationEngine->TriggerEngine();
				}
				
				// Pubsub is subscribed to
				iPubsubSubscribedTo = true;
			}
			
			aRosterItemStore->DeleteItemByIndex(aRosterItemStore->Count() - 1);
		}
	}
	
	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
	
	if(!aPush && !iPubsubSubscribedTo) {
		// Subscribe to pubsub server		
		SendPresenceSubscriptionL(KBuddycloudPubsubServer, _L8("subscribe"));
	}
	
	// Synchronize local roster to received roster
	for(TInt i = (iFollowingList->Count() - 1); i >= 1; i--) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aLocalItem = static_cast <CFollowingRosterItem*> (aItem);
			
			TBool aItemFound = false;
			
			for(TInt x = (aRosterItemStore->Count() - 1); x >= 0; x--) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aRosterItemStore->GetItemByIndex(x));
				
				if(aRosterItem->GetId().Compare(aLocalItem->GetId()) == 0) {
					aNotifyId = aItem->GetItemId();
					
					if(aPush && iPubsubSubscribedTo) {
						// Check user node subscription
						if(aLocalItem->GetSubscription() < EPresenceSubscriptionFrom && 
								aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom) {
							
							// Add user to my node subscribers
							SetPubsubNodeAffiliationL(aLocalItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubAffiliationPublisher);							
						}
						else if(aLocalItem->GetSubscription() >= EPresenceSubscriptionFrom && 
								aRosterItem->GetSubscription() < EPresenceSubscriptionFrom) {
							
							// Remove user from my node subscribers
							SetPubsubNodeSubscriptionL(aLocalItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubSubscriptionNone);
							
							if(aLocalItem->GetPubsubAffiliation() != EPubsubAffiliationNone && 
									aLocalItem->GetPubsubSubscription() > EPubsubSubscriptionNone) {
								
								// Remove me from users node subscribers
								SetPubsubNodeSubscriptionL(iOwnItem->GetId(), aLocalItem->GetId(EIdChannel), EPubsubSubscriptionNone);
							}
						}
					}
					
					if(aRosterItem->GetSubscription() > EPresenceSubscriptionNone) {						
						// Update
						aLocalItem->SetSubscription(aRosterItem->GetSubscription());
					}
					else {
						// Remove (no subscription)
						UnfollowChannelL(aLocalItem->GetItemId());
						
						iDiscussionManager->DeleteDiscussionL(aLocalItem->GetId());
						iFollowingList->DeleteItemByIndex(i);
						
						NotifyNotificationObservers(ENotificationFollowingItemDeleted, aNotifyId);
						
						iSettingsSaveNeeded = true;
					}
					
					aRosterItemStore->DeleteItemByIndex(x);
					
					aItemFound = true;
					break;
				}
			}
			
			if(!aPush && !aItemFound) {
				// Remove no longer subscribed presence (Non-push update only)					
				UnfollowChannelL(aLocalItem->GetItemId());
				
				iDiscussionManager->DeleteDiscussionL(aLocalItem->GetId());
				iFollowingList->DeleteItemByIndex(i);
				
				NotifyNotificationObservers(ENotificationFollowingItemDeleted, aNotifyId);
				
				iSettingsSaveNeeded = true;
			}
		}
	}
	
	// Add new roster items
	while(aRosterItemStore->Count() > 0) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aRosterItemStore->GetItemByIndex(0));
		
		if(aRosterItem->GetSubscription() > EPresenceSubscriptionNone) {
			// Add item to following list
			aRosterItem->SetItemId(iNextItemId++);
			
			if(aRosterItem->GetTitle().Compare(aRosterItem->GetId()) != 0) {
				// Set friendly name
				HBufC* aName = HBufC::NewLC(aRosterItem->GetTitle().Length() + 3 + aRosterItem->GetId().Length());
				TPtr pName(aName->Des());
				pName.Append(aRosterItem->GetId());
				pName.Append(_L(" ()"));
				pName.Insert((pName.Length() - 1), aRosterItem->GetTitle());
				
				aRosterItem->SetTitleL(pName);
				CleanupStack::PopAndDestroy(); // aName
			}
			
			if(aPush && iPubsubSubscribedTo && aRosterItem->GetSubscription() >= EPresenceSubscriptionFrom) {
				// Add user to my node subscribers
				SetPubsubNodeAffiliationL(aRosterItem->GetId(), iOwnItem->GetId(EIdChannel), EPubsubAffiliationPublisher);
			}
			
			// Add to discussion manager
			CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aRosterItem->GetId());
			aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
			
			aRosterItem->SetUnreadData(aDiscussion);
	
			iFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);		
			aRosterItemStore->RemoveItemByIndex(0);	
			
			iSettingsSaveNeeded = true;
		}
		else {	
			aRosterItemStore->DeleteItemByIndex(0);	
		}
	}
	
	iRosterSynchronized = true;

	CleanupStack::PopAndDestroy(); // aRosterItemStore
	
	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

void CBuddycloudLogic::RosterOwnJidL(TDesC& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterOwnJidL"));
#endif

	TBool aItemFound = false;

	if(iOwnItem) {
		iOwnItem->SetOwnItem(false);
	}
	
	// Remove resource from jid
	TPtrC pJid(aJid);

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
	}

	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));

		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetId().Compare(pJid) == 0) {
				aRosterItem->SetOwnItem(true);
				aRosterItem->SetSubscription(EPresenceSubscriptionBoth);

				iOwnItem = aRosterItem;

				aItemFound = true;
				break;
			}
		}
	}

	if(!aItemFound) {
		CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetOwnItem(true);
		aRosterItem->SetSubscription(EPresenceSubscriptionBoth);
		aRosterItem->SetItemId(iNextItemId++);
		aRosterItem->SetIdL(pJid);
		
		// Setting the Contact Name
		if(iSettingFullName.Length() > 0) {
			aRosterItem->SetTitleL(iSettingFullName);
		}
		
		CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(pJid);
		aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());
		
		aRosterItem->SetUnreadData(aDiscussion);
		
		iOwnItem = aRosterItem;
		iFollowingList->InsertItem(0, aRosterItem);
		CleanupStack::Pop();

		if(iStatusObserver) {
			iStatusObserver->JumpToItem(aRosterItem->GetItemId());
		}
	}
}

void CBuddycloudLogic::XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::XmppStanzaAcknowledgedL"));
#endif

	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(aAttributeType.Compare(_L8("result")) == 0) {
		if(aId.Compare(_L8("nearbyplaces1")) == 0) {
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			
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
						aNewPlace->SetPlaceNameL(aTextUtilities->Utf8ToUnicodeL(aItemXmlParser->GetStringData()));
						
						iNearbyPlaces.Append(aNewPlace);
						CleanupStack::Pop();
					}
					
					CleanupStack::PopAndDestroy(); // aItemXmlParser
				}
			}
			
			CleanupStack::PopAndDestroy(); // aTextUtilities
		}
		else if(aId.Compare(_L8("pubsubsubscriptions")) == 0) {
			// Handle pubsub subscriptions
			ProcessPubsubSubscriptionsL(aStanza);
		}
		else if(aId.Compare(_L8("pubsubnodesubscribers")) == 0) {
			// Handle users pubsub node subscribers
			ProcessUsersPubsubNodeSubscribersL(aStanza);
		}
		else if(aId.Compare(_L8("myplaces1")) == 0 || aId.Compare(_L8("addplacequery1")) == 0 || 
				aId.Compare(_L8("createplacequery1")) == 0 || aId.Compare(_L8("currentplacequery1")) == 0 || 
				aId.Compare(_L8("searchplacequery1")) == 0 || aId.Compare(_L8("placedetails1")) == 0) {
			
			// Handle places query result
			HandlePlaceQueryResultL(aStanza, aId);
		}
		else if(aId.Find(_L8("setmood:")) != KErrNotFound || aId.Compare(_L8("setnext1")) == 0) {
			// Set mood/next place result
			SetActivityStatus(_L(""));
		}
		else if(aId.Find(_L8("followchannel:")) != KErrNotFound) {
			// Follow node result
			if(aXmlParser->MoveToElement(_L8("query"))) {
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
				
				TPtrC8 aAttributeNode(aXmlParser->GetStringAttribute(_L8("node")));
				TPtrC8 aEncId(aTextUtilities->UnicodeToUtf8L(iOwnItem->GetId()));
				
				// Send subscribe stanza
				_LIT8(KSubscribeStanza, "<iq to='' type='set' id='subscribe:%02d'><pubsub xmlns='http://jabber.org/protocol/pubsub'><subscribe node='' jid=''/></pubsub></iq>\r\n");
				HBufC8* aSubscribeStanza = HBufC8::NewLC(KSubscribeStanza().Length() + KBuddycloudPubsubServer().Length() + aAttributeNode.Length() + aEncId.Length());
				TPtr8 pSubscribeStanza(aSubscribeStanza->Des());
				pSubscribeStanza.AppendFormat(KSubscribeStanza, GetNewIdStamp());
				pSubscribeStanza.Insert(113, aEncId);
				pSubscribeStanza.Insert(106, aAttributeNode);
				pSubscribeStanza.Insert(8, KBuddycloudPubsubServer);
				
				iXmppEngine->SendAndForgetXmppStanza(pSubscribeStanza, true);
				CleanupStack::PopAndDestroy(2); // aSubscribeStanza, aTextUtilities
				
				// Process disco items
				ProcessChannelMetadataL(aStanza);					
			}
		}
		else if(aId.Find(_L8("metadata:")) != KErrNotFound) {
			// Handle (disco#info) metadata result
			ProcessChannelMetadataL(aStanza);
		}
		else if(aId.Compare(_L8("credentials1")) == 0) {
			// Communities update success
			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CREDENTIALS_SUCCESS);
			CAknConfirmationNote* aDialog = new (ELeave) CAknConfirmationNote();		
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage				
		}
		else if(aId.Compare(_L8("registration1")) == 0) {
			// Registration success
			iXmppEngine->SendAuthorization();
			
			// Save settings
			SaveSettingsAndItemsL();
		}
		else if(aId.Find(_L8("mediareq:")) != KErrNotFound) {
			// Media post request
			if(aXmlParser->MoveToElement(_L8("uploaduri"))) {
				TInt aItemId = KErrNotFound;
				TLex8 aLex(aId.Mid(aId.Locate(':') + 1));
				
				if(aLex.Val(aItemId) == KErrNone) {
					CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
					
					if(aItem && aItem->GetItemType() >= EItemRoster) {						
						// Create discussion entry
						CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
						aAtomEntry->SetPrivate(true);
						aAtomEntry->SetDirectReply(true);
						aAtomEntry->SetIconId(KIconChannel);							
						aAtomEntry->SetPublishTime(TimeStamp());	
						aAtomEntry->SetAuthorNameL(_L("Media Uploader"));
							
						HBufC* aResourceText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_MEDIAUPLOADREADY);
						
						CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
						TPtrC aDataUploaduri = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData());
						
						HBufC* aMessageText = HBufC::NewLC(aResourceText->Des().Length() + aDataUploaduri.Length());
						TPtr pMessageText(aMessageText->Des());
						pMessageText.Append(*aResourceText);
						pMessageText.Replace(pMessageText.Find(_L("$LINK")), 5, aDataUploaduri);							
						
						aAtomEntry->SetContentL(pMessageText);
						CleanupStack::PopAndDestroy(3); // aMessageText, aResourceText, aTextUtilities
	
						// Add to discussion
						CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aItem->GetId());
						
						if(aDiscussion->AddEntryOrCommentLD(aAtomEntry)) {									
							NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aItem->GetItemId());
						}
					}
				}
			}
		}
	}
	else if(aAttributeType.Compare(_L8("error")) == 0) {
		if(aId.Compare(_L8("createplacequery1")) == 0 || aId.Compare(_L8("currentplacequery1")) == 0 || 
				aId.Compare(_L8("editplacequery1")) == 0) {
			// Handle place query error
			SetActivityStatus(_L(""));
			
			if(aXmlParser->MoveToElement(_L8("text"))) {
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();							
				HBufC* aMessageTitle = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_UPDATEFAILED_TITLE);
				
				CAknMessageQueryDialog* aDialog = new (ELeave) CAknMessageQueryDialog();
				aDialog->PrepareLC(R_AUTHERROR_DIALOG);
				aDialog->SetHeaderTextL(*aMessageTitle);
				aDialog->SetMessageTextL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
				aDialog->RunLD();
				
				CleanupStack::PopAndDestroy(2); // aMessageTitle, aTextUtilities
			}
		}
		else if(aId.Find(_L8("setmood:")) != KErrNotFound || aId.Compare(_L8("setnext1")) == 0) {
			// Set mood/next place error
			SetActivityStatus(_L(""));
		}
		else if(aId.Compare(_L8("searchplacequery1")) == 0) {
			// No place search error
			SetActivityStatus(_L(""));

			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_NORESULTSFOUND);
			CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage
		}
		else if(aId.Compare(_L8("placedetails1")) == 0) {
			// Handle places query error
			HandlePlaceQueryResultL(aStanza, aId);
		}
		else if(aId.Find(_L8("followchannel:")) != KErrNotFound) {
			// Follow node error
			TInt aItemId = KErrNotFound;
			TLex8 aLex(aId.Mid(aId.Locate(':') + 1));
			
			if(aLex.Val(aItemId) == KErrNone) {
				CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(aItemId));
				
				if(aItem && aItem->GetItemType() == EItemChannel) {	
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
					
					if(aXmlParser->MoveToElement(_L8("item-not-found"))) {
						// Channel doesn't exist
						aChannelItem->SetIdL(aChannelItem->GetTitle());
						aChannelItem->SetItemId(KEditingNewChannelId);
						
						NotifyNotificationObservers(ENotificationEditChannelRequested, KEditingNewChannelId);
					}
					else {
						// Delete discussion & channel
						iDiscussionManager->DeleteDiscussionL(aChannelItem->GetId());
						iFollowingList->DeleteItemById(aItemId);
					}
				}
			}
		}
		else if(aId.Compare(_L8("credentials1")) == 0) {
			// Communities update failed
			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_CREDENTIALS_FAIL);
			CAknErrorNote* aDialog = new (ELeave) CAknErrorNote();	
			aDialog->SetTimeout(CAknNoteDialog::ENoTimeout);
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage	
		}
		else if(aId.Compare(_L8("registration1")) == 0) {
			// Registration failed			
			CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
			dlg->ExecuteLD(R_AUTHERROR_DIALOG);
			
			iXmppEngine->Disconnect();
			
			NotifyNotificationObservers(ENotificationAuthenticationFailed);
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::HandleIncomingPresenceL(TDesC& aFrom, const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingPresenceL"));
#endif

	TPtrC aJid(aFrom);
	TPtrC aResource(aFrom);

	TInt aSearchResult = aJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		aJid.Set(aJid.Left(aSearchResult));
		aResource.Set(aResource.Mid(aSearchResult + 1));
	}

	// Initialize xml parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	for(TInt i = 0; i < iFollowingList->Count(); i++) {
		CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemByIndex(i));
		
		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(aRosterItem->GetId().Compare(aJid) == 0) {
				TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
				
				if(aAttributeType.Length() == 0) {
					// Users presence changes
					if(aRosterItem->GetPresence() != EPresenceOnline) {
						aRosterItem->SetPresence(EPresenceOnline);
							
//						// Collect pubsub nodes
//						if(!aRosterItem->PubsubCollected()) {
//							aRosterItem->SetPubsubCollected(true);
//		
//							CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/mood"));
//							CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/previous"));
//							CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/current"));
//							CollectUserPubsubNodeL(aRosterItem->GetId(), _L("/geo/future"));				
//						}
					}
					
					// Presence has nick
					if(aXmlParser->MoveToElement(_L8("nick"))) {
						CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
						TPtrC aDataNick(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
						
						if(aDataNick.Length() > 0) {
							HBufC* aTitle = HBufC::NewLC(aRosterItem->GetId().Length() + aDataNick.Length() + 3);
							TPtr pTitle(aTitle->Des());
							pTitle.Append(aRosterItem->GetId());
							pTitle.Append(_L(" ()"));
							pTitle.Insert((pTitle.Length() - 1), aDataNick);
							
							aRosterItem->SetTitleL(pTitle);
							CleanupStack::PopAndDestroy(); // aTitle
						}
						
						CleanupStack::PopAndDestroy(); // aTextUtilities
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
	
	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CBuddycloudLogic::ProcessPresenceSubscriptionL(TDesC& aJid, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPresenceSubscriptionL"));
#endif
	
	CFollowingItem* aItem = static_cast <CFollowingItem*> (iFollowingList->GetItemById(IsSubscribedTo(aJid, EItemNotice)));
	
	if(aItem == NULL) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();				
		CXmlParser* aXmlParser = CXmlParser::NewLC(aData);

		// Get name
		TPtrC8 aName;
		
		if(aXmlParser->MoveToElement(_L8("nick"))) {
			aName.Set(aXmlParser->GetStringData());
			aXmlParser->ResetElementPointer();
		}
		
		HBufC* aEncName = aTextUtilities->Utf8ToUnicodeL(aName).AllocLC();
		TPtr pEncName(aEncName->Des());
		
		// Get broad location		
		TPtrC8 aLocation;		

		if(aXmlParser->MoveToElement(_L8("geoloc"))) {
			if(aXmlParser->MoveToElement(_L8("text"))) {
				aLocation.Set(aXmlParser->GetStringData());
			}
		}
		
		HBufC* aEncLocation = aTextUtilities->Utf8ToUnicodeL(aLocation).AllocLC();
		TPtr pEncLocation(aEncLocation->Des());
	
		_LIT(KBrackets, " ()");
		HBufC* aUserInfo = HBufC::NewLC(pEncName.Length() + KBrackets().Length() + aJid.Length() + 4 + pEncLocation.Length());
		TPtr pUserInfo(aUserInfo->Des());
		
		if(pEncName.Length() > 0) {
			pUserInfo.Append(KBrackets);
			pUserInfo.Insert(2, aJid);
			pUserInfo.Insert(0, pEncName);
		}
		else {
			pUserInfo.Append(aJid);
		}
		
		if(pEncLocation.Length() > 0) {
			pUserInfo.Append(_L(" ~ "));
			pUserInfo.Append(pEncLocation);
			pUserInfo.Append(_L(":"));
		}		
		
		// Prepare message
		HBufC* aTitle = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_FOLLOWINGREQUEST_TITLE);
		HBufC* aDescriptionResource = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_FOLLOWINGREQUEST_TEXT);
		TPtrC pDescriptionResource(aDescriptionResource->Des());
		
		HBufC* aDescription = HBufC::NewLC(pDescriptionResource.Length() + pUserInfo.Length());
		TPtr pDescription(aDescription->Des());
		pDescription.Append(pDescriptionResource);
		
		TInt aResult = pDescription.Find(_L("$USER"));			
		if(aResult != KErrNotFound) {
			pDescription.Replace(aResult, 5, pUserInfo);
		}
		
		CFollowingItem* aNoticeItem = CFollowingItem::NewLC();
		aNoticeItem->SetItemId(iNextItemId++);
		aNoticeItem->SetIdL(aJid, EIdRoster);
		aNoticeItem->SetTitleL(*aTitle);
		aNoticeItem->SetDescriptionL(*aDescription);
		
		iFollowingList->InsertItem(GetBubbleToPosition(aNoticeItem), aNoticeItem);
		CleanupStack::Pop(); // aNoticeItem
		
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
		NotifyNotificationObservers(ENotificationMessageNotifiedEvent);
		CleanupStack::PopAndDestroy(8); // aDescription, aDescriptionResource, aTitle, aUserInfo, aEncLocation, aEncName, aXmlParser, aTextUtilities 
	}
}

void CBuddycloudLogic::HandleIncomingMessageL(TDesC& aFrom, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingMessageL"));
#endif
	
	TBuf8<32> aReferenceId;

	TPtrC aJid(aFrom);
	TInt aSearchResult = aJid.Locate('/');
	
	if(aSearchResult != KErrNotFound) {
		aJid.Set(aJid.Left(aSearchResult));
	}
	
	CAtomEntryData* aAtomEntry = CAtomEntryData::NewLC();
	aAtomEntry->SetPublishTime(TimeStamp());
	aAtomEntry->SetAuthorJidL(aJid, false);
	aAtomEntry->SetPrivate(true);
	
	// Parse message
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aData);
	
	do {
		TPtrC8 aElementName = aXmlParser->GetElementName();
		
		if(aElementName.Compare(_L8("body")) == 0) {
			aAtomEntry->SetContentL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
		}
		else if(aElementName.Compare(_L8("thread")) == 0) {
			aReferenceId.Copy(aXmlParser->GetStringAttribute(_L8("parent")).Left(32));
			
			aAtomEntry->SetIdL(aXmlParser->GetStringData());
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
		else if(aElementName.Compare(_L8("x")) == 0 || aElementName.Compare(_L8("delay")) == 0) {
			TPtrC8 aAttributeStamp = aXmlParser->GetStringAttribute(_L8("stamp"));
			
			if(aAttributeStamp.Length() > 0) {
				aAtomEntry->SetPublishTime(CTimeUtilities::DecodeL(aAttributeStamp));
			}
		}
	} while(aXmlParser->MoveToNextElement());
	
	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
	
	// Add to discussion
	CDiscussion* aDiscussion = iDiscussionManager->GetDiscussionL(aJid);
	CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (iFollowingList->GetItemById(aDiscussion->GetItemId()));
	
	if(aRosterItem == NULL && (!iSettingMessageBlocking || aJid.Locate('@') == KErrNotFound) && 
			aAtomEntry->GetContent().Length() > 0) {

		// Add sender to roster
		aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetItemId(iNextItemId++);
		aRosterItem->SetIdL(aJid);
		aRosterItem->SetUnreadData(aDiscussion);
		
		aDiscussion->SetDiscussionReadObserver(this, aRosterItem->GetItemId());

		iFollowingList->AddItem(aRosterItem);
		CleanupStack::Pop();			
	}
	
	if(aRosterItem) {		
		if(aRosterItem->GetId().Compare(aJid) == 0) {
			aAtomEntry->SetAuthorAffiliation(EPubsubAffiliationOwner);
		}
		
		if(aDiscussion->AddEntryOrCommentLD(aAtomEntry, aReferenceId)) {
			if(!aRosterItem->OwnItem()) {
				// Bubble item
				iFollowingList->RemoveItemByIndex(iFollowingList->GetIndexById(aRosterItem->GetItemId()));
				iFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);
				
				NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aRosterItem->GetItemId());
			}
			
			// Notify arrival
			if(aDiscussion->Notify() && !iTelephonyEngine->IsTelephonyBusy()) {
				NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aRosterItem->GetItemId());
				
				iNotificationEngine->NotifyL(ESettingItemPrivateMessageTone);
			}
			else {
				NotifyNotificationObservers(ENotificationMessageSilentEvent, aRosterItem->GetItemId());
			}
		}
	}
	else {
		// Dispose of message
		CleanupStack::PopAndDestroy(); // aAtomEntry
	}
}
