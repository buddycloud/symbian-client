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
#include "TimeCoder.h"
#include "XmlParser.h"

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
	if(iMasterFollowingList)
		delete iMasterFollowingList;
	
	if(iMasterPlaceList)
		delete iMasterPlaceList;
	
	if(iMessagingManager)
		delete iMessagingManager;
	
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

	iMasterFollowingList = CBuddycloudFollowingStore::NewL();

 	iContactFilter = HBufC::NewL(0);
 	iFilteringContacts = false;
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CXmppEngine"));
#endif

	iXmppEngineReady = false;
	iXmppEngine = CXmppEngine::NewL(this);
	iXmppEngine->AddRosterObserver(this);
 	iXmppEngine->SetXmppServerL(_L8("buddycloud.com"));
 	iXmppEngine->SetResourceL(_L8("buddycloud"));
	
#ifdef _DEBUG
	WriteToLog(_L8("BL    Initialize CBuddycloudPlaceStore/CBuddycloudNearbyStore"));
#endif

 	iMasterPlaceList = CBuddycloudPlaceStore::NewL();

 	iLastReceivedAt = TimeStamp();
	
	// Messaging manager
	iMessagingManager = CMessagingManager::NewL();
	
	// Notification engine
	iNotificationEngine = CNotificationEngine::NewL();

 	LoadSettingsAndItems();
 	
 	if(iOwnerObserver) {
 		iOwnerObserver->LanguageChanged(iSettingPreferredLanguage);
 	}
 	
	NotificationChanged(ESettingItemPrivateMessageTone);
	NotificationChanged(ESettingItemChannelPostTone);
	NotificationChanged(ESettingItemDirectReplyTone);
	
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
		if(iMessagingManager) {
			delete iMessagingManager;
			iMessagingManager = NULL;
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

void CBuddycloudLogic::SendXmppStanzaL(const TDesC8& aStanza, MXmppStanzaObserver* aObserver) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendXmppStanzaL"));
#endif
	
	if(aObserver) {
		iXmppEngine->SendAndAcknowledgeXmppStanza(aStanza, aObserver, true);
	}
	else {
		iXmppEngine->SendAndForgetXmppStanza(aStanza, true);
	}
}

void CBuddycloudLogic::CancelXmppStanzaObservation(MXmppStanzaObserver* aObserver) {
	if(iXmppEngine) {
		iXmppEngine->CancelXmppStanzaObservation(aObserver);
	}
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

	// Launch Server
	iXmppEngine->SetAuthorizationDetailsL(iSettingUsername, iSettingPassword);
	
	if(iSettingTestServer) {
		iXmppEngine->SetHostServerL(_L("beta.buddycloud.com"), 5222);
	}
	else {
		iXmppEngine->SetHostServerL(_L("cirrus.buddycloud.com"), 443);
	}
	
	iXmppEngine->ConnectL(iConnectionCold);
}

void CBuddycloudLogic::SendPresenceL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendPresenceL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	TPtrC8 aEncBroadLocationText(aTextUtilities->UnicodeToUtf8L(*iBroadLocationText));
	
	_LIT8(KPresenceStanza, "<presence><priority>10</priority><status></status><c xmlns='http://jabber.org/protocol/caps' node='http://buddycloud.com/caps' ver='s60-0.5.00'/></presence>\r\n");
	HBufC8* aPresenceStanza = HBufC8::NewLC(KPresenceStanza().Length() + aEncBroadLocationText.Length());
	TPtr8 pPresenceStanza(aPresenceStanza->Des());
	pPresenceStanza.Copy(KPresenceStanza);
	pPresenceStanza.Insert(41, aEncBroadLocationText);
	
	iXmppEngine->SendAndForgetXmppStanza(pPresenceStanza, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(2); // aPresenceStanza, aTextUtilities
}

/*
----------------------------------------------------------------------------
--
-- Settings
--
----------------------------------------------------------------------------
*/
void CBuddycloudLogic::MasterResetL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::MasterResetL"));
#endif

	// Remove own
	iOwnItem = NULL;
	
	// Remove People items
	for(TInt i = iMasterFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);
		
		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			// Unpair and remove item
			if(aRosterItem->GetAddressbookId() != KErrNotFound) {
				UnpairItemsL(aRosterItem->GetItemId());
			}
				
			iMessagingManager->DeleteDiscussionL(aRosterItem->GetJid(EJidRoster));
			iMessagingManager->DeleteDiscussionL(aRosterItem->GetJid(EJidChannel));
			iMasterFollowingList->DeleteItemByIndex(i);
		}
		else if(aItem->GetItemType() == EItemChannel) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
			
			iMessagingManager->DeleteDiscussionL(aChannelItem->GetJid());
			iMasterFollowingList->DeleteItemByIndex(i);
		}
	}	
	
	// Remove Place items
	while(iMasterPlaceList->CountPlaces() > 0) {
		iMasterPlaceList->DeletePlaceByIndex(0);
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
		if(iSettingUsername[i] <= 47 || (iSettingUsername[i] >= 58 && iSettingUsername[i] <= 64) ||
				(iSettingUsername[i] >= 91 && iSettingUsername[i] <= 96) || iSettingUsername[i] >= 123) {
			
			iSettingUsername.Delete(i, 1);
		}
	}
}

void CBuddycloudLogic::LookupUserByEmailL(TBool aForcePublish) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::LookupUserByEmailL"));
#endif
	
	if(iSettingEmailAddress.Length() > 0 && iSettingEmailAddress.Locate('@') != KErrNotFound) {
		// Contact text definition
		CContactItemFieldDef* aFieldDef = new (ELeave) CContactItemFieldDef();
		CleanupStack::PushL(aFieldDef);
		aFieldDef->AppendL(KUidContactFieldEMail);
		
		CContactIdArray* aContactIdArray = iContactDatabase->FindLC(iSettingEmailAddress, aFieldDef);	
		
		if(aContactIdArray->Count() == 1) {
			TRAPD(aErr, PublishUserDataL(aContactIdArray, aForcePublish));	
		}
		
		CleanupStack::PopAndDestroy(2); // aContactIdArray, aTextDef
	}
}

void CBuddycloudLogic::LookupUserByNumberL(TBool aForcePublish) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::LookupUserByNumberL"));
#endif

	CContactIdArray* aContactIdArray = iContactDatabase->MatchPhoneNumberL(iSettingPhoneNumber, 9);
	CleanupStack::PushL(aContactIdArray);
	
	TRAPD(aErr, PublishUserDataL(aContactIdArray, aForcePublish));	
	
	CleanupStack::PopAndDestroy(); // aContactIdArray
}

void CBuddycloudLogic::PublishUserDataL(CContactIdArray* aContactIdArray, TBool aForcePublish) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::PublishUserDataL"));
#endif
	
	iSettingsSaveNeeded = true;
		
	// User profile
	_LIT8(KProfileStanza, "<iq type='set' id='profile1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node='http://www.xmpp.org/extensions/xep-0154.html#ns'><item id='current'><profile xmlns='http://www.xmpp.org/extensions/xep-0154.html#ns'><x xmlns='jabber:x:data' type='result'><field var='detail'></field></x></profile></item></publish></pubsub></iq>\r\n");		
	_LIT8(KNameElement, "<field var='full_name'><value></value></field>");
	_LIT8(KEmailElement, "<field var='email'><value></value></field>");
	_LIT8(KValueElement, "<value></value>");
	
	TInt aAddedDetails = 0;
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	HBufC8* aEncName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
	TPtr8 pEncName(aEncName->Des());
	
	HBufC8* aEncEmail = aTextUtilities->UnicodeToUtf8L(iSettingEmailAddress).AllocLC();
	TPtr8 pEncEmail(aEncEmail->Des());
	
	// User profile
	HBufC8* aProfileStanza = HBufC8::NewL(KProfileStanza().Length() + KNameElement().Length() + pEncName.Length() + KEmailElement().Length() + pEncEmail.Length());
	TPtr8 pProfileStanza(aProfileStanza->Des());
	pProfileStanza.Append(KProfileStanza);
	
	if(pEncEmail.Length() > 0) {
		pProfileStanza.Insert(266, KEmailElement);
		pProfileStanza.Insert(266 + 26, pEncEmail);
	}
	
	pProfileStanza.Insert(266, KNameElement);
	pProfileStanza.Insert(266 + 30, pEncName);
	
	if(aContactIdArray) {
		for(TInt i = 0; i < aContactIdArray->Count(); i++) {
			TContactItemId aContactId = (*aContactIdArray)[i];
			CContactItem* aContact = iContactDatabase->ReadContactLC(aContactId);
			CContactItemFieldSet& aContactFieldSet = aContact->CardFields();
			
			// List phone numbers & email addresses
			for(TInt x = 0; x < aContactFieldSet.Count(); x++) {
				if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber) ||
						aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
					
					if(iSettingPhoneNumber.Length() == 0) {
						if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldPhoneNumber)) {
							iSettingPhoneNumber.Append(aContactFieldSet[x].TextStorage()->Text());
						}
					}
					
					if(iSettingEmailAddress.Length() == 0) {
						if(aContactFieldSet[x].ContentType().ContainsFieldType(KUidContactFieldEMail)) {
							iSettingEmailAddress.Append(aContactFieldSet[x].TextStorage()->Text());
						}
					}
					
					// Encode detail
					TPtrC8 aEncDetail(aTextUtilities->UnicodeToUtf8L(aContactFieldSet[x].TextStorage()->Text()));
					
					// Package detail
					HBufC8* aValueElement = HBufC8::NewLC(KValueElement().Length() + aEncDetail.Length());
					TPtr8 pValueElement(aValueElement->Des());
					pValueElement.Append(KValueElement);
					pValueElement.Insert(7, aEncDetail);
					
					// Add to stanza
					if((pProfileStanza.MaxLength() - pProfileStanza.Length()) < pValueElement.Length()) {
						aProfileStanza = aProfileStanza->ReAlloc(pProfileStanza.Length() + pValueElement.Length());
						pProfileStanza.Set(aProfileStanza->Des());
					}
					
					pProfileStanza.Insert(pProfileStanza.Length() - 55, pValueElement);
					
					aAddedDetails++;
									
					CleanupStack::PopAndDestroy(); // aValueElement
				}
			}
			
			CleanupStack::PopAndDestroy(); // aContact
		}
	}
	
	if(aAddedDetails == 0 && (iSettingPhoneNumber.Length() > 0 || iSettingEmailAddress.Length() > 0)) {
		// No contact database items found
		TPtrC8 aEncDetail;
		
		// Encode & package detail
		if(iSettingPhoneNumber.Length() > 0) {
			aEncDetail.Set(aTextUtilities->UnicodeToUtf8L(iSettingPhoneNumber));
		}
		else {
			aEncDetail.Set(aTextUtilities->UnicodeToUtf8L(iSettingEmailAddress));
		}
		
		// Package detail
		HBufC8* aValueElement = HBufC8::NewLC(KValueElement().Length() + aEncDetail.Length());
		TPtr8 pValueElement(aValueElement->Des());
		pValueElement.Append(KValueElement);
		pValueElement.Insert(7, aEncDetail);
		
		// Add to stanza
		if((pProfileStanza.MaxLength() - pProfileStanza.Length()) < pValueElement.Length()) {
			aProfileStanza = aProfileStanza->ReAlloc(pProfileStanza.Length() + pValueElement.Length());
			pProfileStanza.Set(aProfileStanza->Des());
		}
		
		pProfileStanza.Insert(pProfileStanza.Length() - 55, pValueElement);
		
		aAddedDetails++;
						
		CleanupStack::PopAndDestroy(); // aValueElement
	}
	
	if(aAddedDetails > 0 || aForcePublish) {
		iXmppEngine->SendAndForgetXmppStanza(pProfileStanza, true);
	}
	
	if(aProfileStanza) {
		delete aProfileStanza;
	}
	
	CleanupStack::PopAndDestroy(3); // aEncEmail, aEncName, aTextUtilities
}

void CBuddycloudLogic::LanguageChanged() {
	if(iOwnerObserver) {
		iOwnerObserver->LanguageChanged(iSettingPreferredLanguage);
	}
}

void CBuddycloudLogic::NotificationChanged(TIntSettingItems aItem) {
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
		case ESettingItemPhoneNumber:
			return iSettingPhoneNumber;
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
		case ESettingItemGroupNotification:
			return iSettingGroupNotification;
		case ESettingItemLanguage:
			return iSettingPreferredLanguage;
		default:
			return iSettingGroupNotification;
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
		case ESettingItemTestServer:
			return iSettingTestServer;
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

	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);

	if(aItem != NULL) {
		if(aItem->GetItemType() == EItemContact) {
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
		CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);
	
		if(aItem != NULL) {
			if(aItem->GetItemType() == EItemContact || aItem->GetItemType() == EItemRoster) {
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
							HBufC* aBody = HBufC::NewLC(pSendPlaceText.Length() + aId.Length() + iOwnItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length());
							
							TPtr pBody(aBody->Des());
							pBody.Copy(pSendPlaceText);
							pBody.Replace(pBody.Find(_L("$PLACE")), 6, iOwnItem->GetPlace(EPlaceCurrent)->GetPlaceName());
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
}

void CBuddycloudLogic::FollowContactL(const TDesC& aContact) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowContactL"));
#endif

	HBufC* aJid = HBufC::NewLC(aContact.Length() + KBuddycloudRosterServer().Length());
	TPtr pJid(aJid->Des());
	pJid.Copy(aContact);
	pJid.LowerCase();

	// Check for server
	TInt aSearchResult = pJid.Locate('@');
	if(aSearchResult == KErrNotFound) {
		pJid.Append(KBuddycloudRosterServer);
	}
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(IsSubscribedTo(pJid, EItemRoster));
	
	if(aItem == NULL) {
		// Prepare data
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(pJid).AllocLC();
		TPtr8 pEncJid(aEncJid->Des());
		
		_LIT8(KNickElement, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
		HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtr8 pEncFullName(aEncFullName->Des());
		
		_LIT8(KGeolocElement, "<geoloc xmlns='http://jabber.org/protocol/geoloc'><text></text></geoloc>");
		HBufC8* aEncLocation = aTextUtilities->UnicodeToUtf8L(*iBroadLocationText).AllocLC();
		TPtr8 pEncLocation(aEncLocation->Des());
		
		// Build & send request
		_LIT8(KStanza, "<presence to='' type='subscribe'></presence>\r\n");
		HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncJid.Length() + KNickElement().Length() + pEncFullName.Length() + KGeolocElement().Length() + pEncLocation.Length());
		TPtr8 pStanza(aStanza->Des());
		pStanza.Append(KStanza);
		
		if(pEncLocation.Length() > 0) {
			pStanza.Insert(33, KGeolocElement);
			pStanza.Insert(33 + 56, pEncLocation);
		}
		
		if(pEncFullName.Length() > 0) {
			pStanza.Insert(33, KNickElement);
			pStanza.Insert(33 + 46, pEncFullName);
		}
		pStanza.Insert(14, pEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
		CleanupStack::PopAndDestroy(5); // aStanza, aEncLocation, aEncFullName, aEncJid, aTextUtilities
				
		// Acknowledge request sent
		HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_REQUESTSENT);
		CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
		aDialog->ExecuteLD(*aMessage);
		CleanupStack::PopAndDestroy(); // aMessage
	}

	CleanupStack::PopAndDestroy(); // aJid	
}

void CBuddycloudLogic::GetFriendDetailsL(TInt aFollowerId) {
	if(iState == ELogicOnline) {	
#ifdef _DEBUG
		WriteToLog(_L8("BL    CBuddycloudLogic::GetFriendDetailsL"));
#endif

		CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);
		
		if(aItem && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
			
			if(!aRosterItem->GetPubsubCollected() && aRosterItem->GetSubscriptionType() >= ESubscriptionTo) {
				CollectPubsubNodesL(aRosterItem->GetJid());
			}
			
			aRosterItem->SetPubsubCollected(true);
		}
	}	
}

TInt CBuddycloudLogic::FollowTopicChannelL(TDesC& aChannelJid, TDesC& aTitle, const TDesC& aDescription) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowTopicChannelL"));
#endif

	TInt aChannelItemId = 0;
	
	if(aChannelJid.Locate('@') != KErrNotFound) {
		if((aChannelItemId = IsSubscribedTo(aChannelJid, EItemChannel)) == 0) {
			// Add channel Item to list
			CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
			aChannelItemId = iNextItemId;
			aChannelItem->SetItemId(iNextItemId++);
			aChannelItem->SetJidL(aChannelJid);
			aChannelItem->SetTitleL(aTitle);
			aChannelItem->SetDescriptionL(aDescription);
			
			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelJid);
			aDiscussion->AddMessageReadObserver(aChannelItem);
			aDiscussion->AddDiscussionReadObserver(this, aChannelItem);

			iSettingsSaveNeeded = true;			
			iMasterFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
			CleanupStack::Pop(); // aGroupItem
		}

		// Send initial presence to channel
		SendChannelPresenceL(aChannelJid, _L8("<history maxstanzas='30'/>"));
	}
	
	return aChannelItemId;
}

void CBuddycloudLogic::FollowPersonalChannelL(TInt aFollowerId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::FollowPersonalChannelL"));
#endif
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);

	if(aItem && aItem->GetItemType() == EItemRoster) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
		
		if(aRosterItem->GetSubscriptionType() == ESubscriptionTo || aRosterItem->GetSubscriptionType() == ESubscriptionBoth) {	
			if(aRosterItem->GetJid(EJidChannel).Length() == 0) {
				HBufC* aChannelJid = HBufC::NewLC(aRosterItem->GetJid().Length() + KBuddycloudChannelsServer().Length());
				TPtr pChannelJid(aChannelJid->Des());
				pChannelJid.Append(aRosterItem->GetJid());
				
				TInt aLocateResult = pChannelJid.Locate('@');
				
				if(aLocateResult != KErrNotFound) {
					pChannelJid.Replace(aLocateResult, 1, _L("%"));
					pChannelJid.Append(KBuddycloudChannelsServer);
					
					aRosterItem->SetJidL(pChannelJid, EJidChannel);
					
					CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pChannelJid);
					aDiscussion->AddMessageReadObserver(aRosterItem);
					aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
									
					GetChannelInfoL(pChannelJid);
				}
				
				CleanupStack::PopAndDestroy(); // aChannelJid			
			}
		}
	}
}

void CBuddycloudLogic::UnfollowChannelL(TInt aFollowerId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnfollowChannelL"));
#endif
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);

	if(aItem && (aItem->GetItemType() == EItemRoster || aItem->GetItemType() == EItemChannel)) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		TPtrC aChannelJid(aChannelItem->GetJid());
		
		if(aChannelJid.Length() > 0) {
			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelJid);
			
			if(aItem->GetItemType() == EItemChannel) {
				if(aDiscussion->GetMyAffiliation() == EAffiliationOwner) {
					// Is owner, delete the channel
					CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
					TPtrC8 aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aChannelJid);
					
					_LIT8(KDeleteChannelsStanza, "<iq to='' type='set' id='deletechannel1'><query xmlns='http://jabber.org/protocol/muc#owner'><destroy/></query></iq>\r\n");
					HBufC8* aDeleteChannelsStanza = HBufC8::NewLC(KDeleteChannelsStanza().Length() + aEncChannelJid.Length());
					TPtr8 pDeleteChannelsStanza(aDeleteChannelsStanza->Des());
					pDeleteChannelsStanza.Append(KDeleteChannelsStanza);
					pDeleteChannelsStanza.Insert(8, aEncChannelJid);

					iXmppEngine->SendAndForgetXmppStanza(pDeleteChannelsStanza, true);
					CleanupStack::PopAndDestroy(2); // aDeleteChannelsStanza, aTextUtilities
				}
				else {
					// Unfollow channel (change own affiliation to none)
					ChangeUsersChannelAffiliationL(iOwnItem->GetJid(), aChannelItem->GetJid(), EAffiliationNone);
				}
				
				// Send unavailable to channel
				SendChannelUnavailableL(aChannelJid);
				
				// A topic channel, delete the item
				iMessagingManager->DeleteDiscussionL(aChannelJid);
				iMasterFollowingList->DeleteItemById(aFollowerId);
			}
			else {
				// Unfollow channel (change own affiliation to none)
				ChangeUsersChannelAffiliationL(iOwnItem->GetJid(), aChannelJid, EAffiliationNone);
				
				// Send unavailable to channel
				SendChannelUnavailableL(aChannelJid);
								
				// Delete discussion
				iMessagingManager->DeleteDiscussionL(aChannelJid);
				
				// Set personal channel jid to empty
				aChannelItem->SetJidL(_L(""));
			}
			
			NotifyNotificationObservers(ENotificationFollowingItemDeleted, aFollowerId);
			
			iSettingsSaveNeeded = true;
		}
	}
}

TInt CBuddycloudLogic::CreateChannelL(const TDesC& aChannelTitleOrJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CreateChannelL"));
#endif

	TInt aChannelItemId = 0;
	
	CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
	aChannelItemId = iNextItemId;
	aChannelItem->SetItemId(iNextItemId++);
	
	HBufC* aJid = HBufC::NewLC(aChannelTitleOrJid.Length() + KBuddycloudChannelsServer().Length());
	TPtr pJid(aJid->Des());
	pJid.Append(aChannelTitleOrJid);
	pJid.LowerCase();
	
	if(aChannelTitleOrJid.Locate('@') == KErrNotFound) {
		// Validate jid by removing special characters
		for(TInt i = pJid.Length() - 1; i >= 0; i--) {
			if(pJid[i] <= 47 || (pJid[i] >= 58 && pJid[i] <= 64) ||
					(pJid[i] >= 91 && pJid[i] <= 96) || pJid[i] >= 123) {
				
				pJid.Delete(i, 1);
			}
		}
		
		if(pJid.Length() == 0) {
			// Jid is empty because it contains non-ascii characters
			pJid.Append(aChannelTitleOrJid);
			
			for(TInt i = 0; i < pJid.Length(); i++) {
				pJid[i] = (pJid[i] % 26) + 97;
			}
		}
		
		pJid.Append(KBuddycloudChannelsServer);
	}
	else {
		aChannelItem->SetExternal(true);
	}
	
	aChannelItem->SetJidL(pJid);
	aChannelItem->SetTitleL(aChannelTitleOrJid);	
	CleanupStack::PopAndDestroy(); // aJid
	
	CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());
	CFollowingItem* aItem = aDiscussion->GetFollowingItem();
	
	if(aItem) {
		// Already exists
		aChannelItemId = aItem->GetItemId();
		CleanupStack::PopAndDestroy(); // aChannelItem
	}
	else {
		// Does not exist in list
		aDiscussion->AddMessageReadObserver(aChannelItem);
		aDiscussion->AddDiscussionReadObserver(this, aChannelItem);
	
		iSettingsSaveNeeded = true;			
		iMasterFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
		CleanupStack::Pop(); // aChannelItem
			
		GetChannelInfoL(aChannelItem->GetJid());
	}
	
	return aChannelItemId;
}

void CBuddycloudLogic::InviteToChannelL(TInt aChannelId) {
	HBufC* aEmptyText = HBufC::NewLC(0);
	InviteToChannelL(*aEmptyText, aChannelId);
	CleanupStack::PopAndDestroy();
}

void CBuddycloudLogic::InviteToChannelL(TDesC& aJid, TInt aChannelId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::InviteToChannelL"));
#endif
	
	RPointerArray<HBufC> aDynItems;
	RArray<TInt> aDynItemIds;
		
	// Search existing Channels & contacts
	for(TInt i = 1; i < iMasterFollowingList->Count(); i++) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem->GetItemType() == EItemChannel || aItem->GetItemType() == EItemRoster) {			
			if(aItem->GetItemType() == EItemChannel) {
				if(aJid.Length() > 0 && aChannelId != aItem->GetItemId()) {
					HBufC* aItemText = HBufC::NewLC(4 + aItem->GetTitle().Length());
					TPtr pItemText(aItemText->Des());
					pItemText.Append(_L("3\t\t\t"));
					pItemText.Insert(2, aItem->GetTitle());
					
					aDynItems.Insert(aItemText, 0);
					aDynItemIds.Insert(aItem->GetItemId(), 0);
					CleanupStack::Pop(); // aItem
				}
			}
			else {
				if( !aItem->GetIsOwn() && aJid.Length() == 0 ) {
					HBufC* aItemText = HBufC::NewLC(4 + aItem->GetTitle().Length());
					TPtr pItemText(aItemText->Des());
					pItemText.Append(_L("2\t\t\t"));
					pItemText.Insert(2, aItem->GetTitle());
					
					aDynItems.Append(aItemText);
					aDynItemIds.Append(aItem->GetItemId());
					CleanupStack::Pop(); // aItem
				}
			}
		}
	}
		
	if(aJid.Length() > 0) {		
		// New Channel
		HBufC* aNewChannel = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_ITEM_NEWGROUP);
		TPtr pNewChannel(aNewChannel->Des());
		
		HBufC* aItem = HBufC::NewLC(4 + pNewChannel.Length());
		TPtr pItem(aItem->Des());
		pItem.Append(_L("3\t\t\t"));
		pItem.Insert(2, pNewChannel);
		
		aDynItems.Insert(aItem, 0);
		aDynItemIds.Insert(0, 0);
		CleanupStack::Pop(); // aItem
		CleanupStack::PopAndDestroy(); // aNewChannel	
	}
	
	// Selection result
	TInt aSelectedItem = DisplaySingleLinePopupMenuL(aDynItems);
	
	if(aSelectedItem != KErrNotFound) {
		TInt aSelectedItemId = aDynItemIds[aSelectedItem];
		
		if(aSelectedItemId == 0) {
			// New Channel
			TBuf<64> aChannelNameOrJid;

			CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(aChannelNameOrJid);
			aDialog->SetPredictiveTextInputPermitted(true);

			if(aDialog->ExecuteLD(R_CREATECHANNEL_DIALOG) != 0) {
				CreateChannelL(aChannelNameOrJid);
			}
		}
		
		if(aSelectedItemId > 0) {
			CFollowingItem* aItem = static_cast <CFollowingItem*> (iMasterFollowingList->GetItemById(aSelectedItemId));
	
			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aChosenItem = static_cast <CFollowingRosterItem*> (aItem);
				
				SendChannelInviteL(aChosenItem->GetJid(), aChannelId);
			}
			else if(aItem->GetItemType() == EItemChannel) {
				SendChannelInviteL(aJid, aSelectedItemId);
			}
			
			HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_REQUESTSENT);
			CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
			
			aDialog->ExecuteLD(*aMessage);
			CleanupStack::PopAndDestroy(); // aMessage
		}
	}
	
	while(aDynItems.Count() > 0) {
		delete aDynItems[0];
		aDynItems.Remove(0);
	}
	
	aDynItems.Close();
	aDynItemIds.Close();
}

void CBuddycloudLogic::DeclineChannelInviteL(TDesC& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::DeclineChannelInviteL"));
#endif

	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	TPtrC8 aEncJid(aTextUtilities->UnicodeToUtf8L(aJid));

	_LIT8(KStanza, "<message to=''><x xmlns='http://jabber.org/protocol/muc#user'><decline/></x></message>\r\n");	
	HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aEncJid.Length());
	TPtr8 pStanza(aStanza->Des());
	pStanza.Copy(KStanza);
	pStanza.Insert(13, aEncJid);

	iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
	CleanupStack::PopAndDestroy(2); // aStanza, aTextUtilities
}

void CBuddycloudLogic::CollectMyChannelsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::CollectMyChannelsL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
	// Collect my channels
	TPtrC8 aEncOwnJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetJid());

	_LIT8(KMyChannelsStanza, "<iq to='maitred.buddycloud.com' type='get' id='mychannels1'><query xmlns='http://buddycloud.com/protocol/channels'><subject></subject></query></iq>\r\n");
	HBufC8* aMyChannelsStanza = HBufC8::NewLC(KMyChannelsStanza().Length() + aEncOwnJid.Length());
	TPtr8 pMyChannelsStanza(aMyChannelsStanza->Des());
	pMyChannelsStanza.Append(KMyChannelsStanza);
	pMyChannelsStanza.Insert(124, aEncOwnJid);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pMyChannelsStanza, this, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(2); // aMyChannelsStanza, aTextUtilities
}

void CBuddycloudLogic::ProcessMyChannelsL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessMyChannelsL"));
#endif
	
	// Synchronize local channels to server stored channels
	CBuddycloudFollowingStore* aStoredChannels = CBuddycloudFollowingStore::NewLC();
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	TBool aChannelFound = false;
	TInt aLocateResult;
	
	// Parse my channels
	while(aXmlParser->MoveToElement(_L8("channel"))) {	
		CFollowingChannelItem* aChannel = CFollowingChannelItem::NewLC();
		CXmlParser* aChannelXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
		
		if(aChannelXmlParser->MoveToElement(_L8("jid"))) {
			aChannel->SetJidL(aTextUtilities->Utf8ToUnicodeL(aChannelXmlParser->GetStringData()));
		}
		
		if(aChannelXmlParser->MoveToElement(_L8("title"))) {
			aChannel->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aChannelXmlParser->GetStringData()));
		}
		
		if(aChannelXmlParser->MoveToElement(_L8("rank"))) {
			aChannel->SetRank(aChannelXmlParser->GetIntData());
		}
		
		CleanupStack::PopAndDestroy(); // aChannelXmlParser
		
		// Check if channel is a personal one
		aLocateResult = aChannel->GetJid().Locate('%');
		
		if(aLocateResult != KErrNotFound) {
			// Is a personal channel
			HBufC* aChannelJid = aChannel->GetJid().AllocLC();
			TPtr pChannelJid(aChannelJid->Des());
			pChannelJid.Replace(aLocateResult, 1, _L("@"));
			
			if((aLocateResult = pChannelJid.LocateReverse('@')) != KErrNotFound) {
				pChannelJid = pChannelJid.Left(aLocateResult);
			}
			
			// Get discussion & item of ROSTER (not channel)
			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pChannelJid);
			CFollowingItem* aItem = aDiscussion->GetFollowingItem();
			
			if(aItem && aItem->GetItemType() == EItemRoster) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
				
				aChannelItem->SetRank(aChannel->GetRank());
				
				// Set channel jid if not exists
				if(aChannelItem->GetJid().Length() == 0) {
					aChannelItem->SetJidL(aChannel->GetJid());
				}
				
				// Now get CHANNEL discussion & setup observers
				aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());
				aDiscussion->AddMessageReadObserver(aChannelItem);
				aDiscussion->AddDiscussionReadObserver(this, aChannelItem);
				
				// Change discussion jid if different
				if(aChannel->GetJid().Compare(aChannelItem->GetJid()) != 0) {
					aChannelItem->SetJidL(aChannel->GetJid());
					aDiscussion->SetJidL(aChannel->GetJid());
				}
			}
			
			CleanupStack::PopAndDestroy(2); // aChannelJid, aChannel
		}
		else {		
			// Add to stored channels
			aStoredChannels->AddItem(aChannel);
			CleanupStack::Pop(); // aChannel
		}
	}
	
	CleanupStack::PopAndDestroy(); // aXmlParser
	
	// Syncronization of topic channels
	for(TInt i = iMasterFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem && aItem->GetItemType() == EItemChannel) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);

			aChannelFound = false;
				
			if(!aChannelItem->IsExternal()) {
				for(TInt x = 0; x < aStoredChannels->Count(); x++) {
					CFollowingChannelItem* aStoredChannel = static_cast <CFollowingChannelItem*> (aStoredChannels->GetItemByIndex(x));
					
					if(aChannelItem->GetJid().Compare(aStoredChannel->GetJid()) == 0) {						
						CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);						
						aChannelItem->SetTitleL(aStoredChannel->GetTitle());
						aChannelItem->SetRank(aStoredChannel->GetRank());
						
						// Delete stored channel from list
						aStoredChannels->DeleteItemByIndex(x);
						
						aChannelFound = true;
						break;
					}
				}
				
				if(!aChannelFound) {
					// Delete channel					
					iMessagingManager->DeleteDiscussionL(aChannelItem->GetJid());
					iMasterFollowingList->DeleteItemByIndex(i);
										
					NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItem->GetItemId());
				}
				
				if(aStoredChannels->Count() == 0) {
					break;
				}
			}
		}
	}
	
	// Syncronize - add stored
	while(aStoredChannels->Count() > 0) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aStoredChannels->GetItemByIndex(0));
		
		AddChannelItemL(aChannelItem->GetJid(), aChannelItem->GetTitle(), aChannelItem->GetDescription(), aChannelItem->GetRank());
		aStoredChannels->DeleteItemByIndex(0);
	}
	
	iSettingsSaveNeeded = true;
	
	// Send presence to channels
	iTimerState |= ETimeoutSendPresence;
	TimerExpired(0);
	
	if(iOwnItem->GetJid(EJidChannel).Length() == 0) {
		// No personal channel
		FollowPersonalChannelL(iOwnItem->GetItemId());
	}

	CleanupStack::PopAndDestroy(2); // aTextUtilities, aStoredChannels
}

void CBuddycloudLogic::SynchronizePersonalChannelMembersL(const TDesC8& aStanza) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SynchronizePersonalChannelMembersL"));
#endif
	
	// Synchronize personal channel members to my roster
	CBuddycloudFollowingStore* aChannelMembers = CBuddycloudFollowingStore::NewLC();
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aStanza);
	
	// Get personal channel jid
	TPtrC aOwnChannelJid(iOwnItem->GetJid(EJidChannel));
	
	// Parse personal channel members
	while(aXmlParser->MoveToElement(_L8("item"))) {	
		CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetJidL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
		
		aChannelMembers->AddItem(aRosterItem);
		CleanupStack::Pop(); // aRosterItem
	}
	
	_LIT8(KAffiliationItem, "<item jid='' affiliation=''/>");
	HBufC8* aAffiliationDeltaList = HBufC8::NewL(0);
	TPtr8 pAffiliationDeltaList(aAffiliationDeltaList->Des());
	
	TBool aUserFound = false;
	
	// Syncronization of roster & members
	for(TInt i = iMasterFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem && !aItem->GetIsOwn() && aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetSubscriptionType() == ESubscriptionFrom || aRosterItem->GetSubscriptionType() == ESubscriptionBoth) {
				aUserFound = false;
					
				for(TInt x = 0; x < aChannelMembers->Count(); x++) {
					CFollowingRosterItem* aMemberItem = static_cast <CFollowingRosterItem*> (aChannelMembers->GetItemByIndex(x));
					
					if(aRosterItem->GetJid().Compare(aMemberItem->GetJid()) == 0) {						
						// Delete stored channel from list
						aChannelMembers->DeleteItemByIndex(x);
						
						aUserFound = true;
						break;
					}
				}
				
				if(!aUserFound) {
					// Add roster user to membership
					TPtrC8 aEncJid = aTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid());
					
					aAffiliationDeltaList = aAffiliationDeltaList->ReAlloc(pAffiliationDeltaList.Length() + KAffiliationItem().Length() + aEncJid.Length() + KAffiliationMember().Length());
					pAffiliationDeltaList.Set(aAffiliationDeltaList->Des());
					pAffiliationDeltaList.Append(KAffiliationItem);
					pAffiliationDeltaList.Insert(pAffiliationDeltaList.Length() - 18, aEncJid);
					pAffiliationDeltaList.Insert(pAffiliationDeltaList.Length() - 3, KAffiliationMember);					
				}
				
				if(aChannelMembers->Count() == 0) {
					break;
				}
			}
		}
	}
	
	// Remove others from personal channel membership
	while(aChannelMembers->Count() > 0) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aChannelMembers->GetItemByIndex(0));		
		TPtrC8 aEncJid = aTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid());
		
		aAffiliationDeltaList = aAffiliationDeltaList->ReAlloc(pAffiliationDeltaList.Length() + KAffiliationItem().Length() + aEncJid.Length() + KAffiliationNone().Length());
		pAffiliationDeltaList.Set(aAffiliationDeltaList->Des());
		pAffiliationDeltaList.Append(KAffiliationItem);
		pAffiliationDeltaList.Insert(pAffiliationDeltaList.Length() - 18, aEncJid);
		pAffiliationDeltaList.Insert(pAffiliationDeltaList.Length() - 3, KAffiliationNone);					

		aChannelMembers->DeleteItemByIndex(0);
	}
	
	if(pAffiliationDeltaList.Length() > 0) {
		// Send affiliation delta
		TPtrC8 aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aOwnChannelJid);
		
		_LIT8(KAffiliationStanza, "<iq to='' type='set' id='affiliation1'><query xmlns='http://jabber.org/protocol/muc#admin'></query></iq>\r\n");	
		HBufC8* aAffiliationStanza = HBufC8::NewLC(KAffiliationStanza().Length() + aEncChannelJid.Length() + pAffiliationDeltaList.Length());
		TPtr8 pAffiliationStanza(aAffiliationStanza->Des());
		pAffiliationStanza.Append(KAffiliationStanza);
		pAffiliationStanza.Insert(91, pAffiliationDeltaList);
		pAffiliationStanza.Insert(8, aEncChannelJid);

		iXmppEngine->SendAndForgetXmppStanza(pAffiliationStanza, true);
		CleanupStack::PopAndDestroy(); // aAffiliationStanza
	}
	
	if(aAffiliationDeltaList) {
		delete aAffiliationDeltaList;
	}

	CleanupStack::PopAndDestroy(3); // aXmlParser, aTextUtilities, aChannelMembers	
}

void CBuddycloudLogic::SendChannelPresenceL(TDesC& aJid, const TDesC8& aHistory) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendChannelPresenceL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();
	TPtr8 pEncJid(aEncJid->Des());
	
	HBufC8* aEncUsername = aTextUtilities->UnicodeToUtf8L(iSettingUsername).AllocLC();
	TPtr8 pEncUsername(aEncUsername->Des());
	
	HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
	TPtr8 pEncFullName(aEncFullName->Des());
	
	// Nick
	_LIT8(KNickElement, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
	HBufC8* aNickElement = HBufC8::NewLC(KNickElement().Length() + pEncFullName.Length());
	TPtr8 pNickElement(aNickElement->Des());
	
	if(pEncFullName.Length() > 0 && iSettingShowName) {
		pNickElement.Copy(KNickElement);
		pNickElement.Insert(46, pEncFullName);
	}
	
	_LIT8(KStanza, "<presence to='/'><c xmlns='http://jabber.org/protocol/caps' node='http://buddycloud.com/caps' ver='s60-0.5.00'/><x xmlns='http://jabber.org/protocol/muc'></x></presence>\r\n");
	HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncUsername.Length() + pEncJid.Length() + pNickElement.Length() + aHistory.Length());
	TPtr8 pStanza(aStanza->Des());
	pStanza.Copy(KStanza);
	pStanza.Insert(158, pNickElement);
	pStanza.Insert(154, aHistory);
	pStanza.Insert(15, pEncUsername);
	pStanza.Insert(14, pEncJid);

	iXmppEngine->SendAndForgetXmppStanza(pStanza, false, EXmppPriorityHigh);
	CleanupStack::PopAndDestroy(6); // aStanza, aNickElement, aEncFullName, aEncUsername, aEncJid, aTextUtilities
}

void CBuddycloudLogic::SendChannelUnavailableL(TDesC& aChannelJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendChannelUnavailableL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aChannelJid).AllocLC();
	TPtrC8 pEncJid(aEncJid->Des());
	
	HBufC8* aEncUsername = aTextUtilities->UnicodeToUtf8L(iSettingUsername).AllocLC();
	TPtrC8 pEncUsername(aEncUsername->Des());

	// Send leave group stanza
	_LIT8(KUnavailableStanza, "<presence to='/' type='unavailable'/>\r\n");
	HBufC8* aUnavailableStanza = HBufC8::NewLC(KUnavailableStanza().Length() + pEncUsername.Length() + pEncJid.Length());
	TPtr8 pUnavailableStanza = aUnavailableStanza->Des();
	pUnavailableStanza.Append(KUnavailableStanza);
	pUnavailableStanza.Insert(15, pEncUsername);
	pUnavailableStanza.Insert(14, pEncJid);

	iXmppEngine->SendAndForgetXmppStanza(pUnavailableStanza, false);
	CleanupStack::PopAndDestroy(4); // aUnavailableStanza, aEncUsername, aEncJid, aTextUtilities
}

void CBuddycloudLogic::SendChannelConfigurationL(TInt aFollowerId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendChannelConfigurationL"));
#endif
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);
	
	if(aItem->GetItemType() == EItemChannel) {		
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		HBufC8* aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetJid()).AllocLC();
		TPtrC8 pEncChannelJid(aEncChannelJid->Des());
		
		HBufC8* aEncChannelTitle = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()).AllocLC();
		TPtrC8 pEncChannelTitle(aEncChannelTitle->Des());
		
		HBufC8* aEncChannelDescription = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetDescription()).AllocLC();
		TPtrC8 pEncChannelDescription(aEncChannelDescription->Des());
		
		// Send topic channel config								
		_LIT8(KConfigStanza, "<iq to='' type='set' id='channelconfig1'><query xmlns='http://jabber.org/protocol/muc#owner'><x xmlns='jabber:x:data' type='submit'><field var='FORM_TYPE'><value>http://jabber.org/protocol/muc#roomconfig</value></field><field var='muc#roomconfig_roomname'><value></value></field><field var='muc#roomconfig_roomdesc'><value></value></field><field var='muc#roomconfig_maxusers'><value>0</value></field><field var='muc#roomconfig_publicroom'><value>1</value></field><field var='muc#roomconfig_persistentroom'><value>1</value></field><field var='muc#roomconfig_moderatedroom'><value>1</value></field></x></query></iq>\r\n");
		HBufC8* aConfigStanza = HBufC8::NewLC(KConfigStanza().Length() + pEncChannelJid.Length() + pEncChannelTitle.Length() + pEncChannelDescription.Length());
		TPtr8 pConfigStanza(aConfigStanza->Des());
		pConfigStanza.Append(KConfigStanza);
		pConfigStanza.Insert(323, pEncChannelDescription);
		pConfigStanza.Insert(263, pEncChannelTitle);
		pConfigStanza.Insert(8, pEncChannelJid);
		
		iXmppEngine->SendAndForgetXmppStanza(pConfigStanza, true);
		CleanupStack::PopAndDestroy(5); // aConfigStanza, aEncChannelDescription, aEncChannelTitle, aEncChannelJid, aTextUtilities						
	}
}

void CBuddycloudLogic::GetChannelInfoL(TDesC& aChannelJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::GetChannelInfoL"));
#endif
	
	// Get channel info
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	TPtrC8 pEncJid(aTextUtilities->UnicodeToUtf8L(aChannelJid));
	
	_LIT8(KInfoStanza, "<iq to='' type='get' id='channelinfo1'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>");
	HBufC8* aInfoStanza = HBufC8::NewLC(KInfoStanza().Length() + pEncJid.Length());
	TPtr8 pInfoStanza(aInfoStanza->Des());
	pInfoStanza.Append(KInfoStanza);
	pInfoStanza.Insert(8, pEncJid);
	
	iXmppEngine->SendAndAcknowledgeXmppStanza(pInfoStanza, this, true);
	CleanupStack::PopAndDestroy(2); // aInfoStanza, aTextUtilities

}

void CBuddycloudLogic::SendChannelInviteL(TDesC& aJid, TInt aChannelId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendChannelInviteL"));
#endif
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aChannelId);
	
	if(aItem->GetItemType() == EItemChannel) {		
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		HBufC8* aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetJid()).AllocLC();
		TPtr8 pEncChannelJid(aEncChannelJid->Des());
		
		HBufC8* aEncInviteeJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();
		TPtr8 pEncInviteeJid(aEncInviteeJid->Des());
		
		HBufC8* aEncUsername = aTextUtilities->UnicodeToUtf8L(iSettingUsername).AllocLC();
		TPtr8 pEncUsername(aEncUsername->Des());
		
		HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtr8 pEncFullName(aEncFullName->Des());
		
		HBufC8* aEncChannelTitle = aTextUtilities->UnicodeToUtf8L(aChannelItem->GetTitle()).AllocLC();
		TPtr8 pEncChannelTitle(aEncChannelTitle->Des());
		
		HBufC* aInviteText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_GROUPINVITE_TEXT);
		HBufC8* aEncInviteText = aTextUtilities->UnicodeToUtf8L(*aInviteText).AllocLC();
		TPtr8 pEncInviteText(aEncInviteText->Des());
	
		_LIT8(KName, " ()");
		_LIT8(KStanza, "<message to=''><x xmlns='http://jabber.org/protocol/muc#user'><invite to=''><reason></reason></invite></x></message>\r\n");	
		HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncInviteeJid.Length() + KName().Length() + pEncFullName.Length() + pEncUsername.Length() + pEncChannelJid.Length() + pEncInviteText.Length() + pEncChannelTitle.Length());
		TPtr8 pStanza(aStanza->Des());
		pStanza.Copy(KStanza);
		pStanza.Insert(84, pEncChannelTitle);
		pStanza.Insert(84, pEncInviteText);
		
		if(pEncFullName.Length() > 0) {
			pStanza.Insert(84, KName);
			pStanza.Insert(86, pEncUsername);
			pStanza.Insert(84, pEncFullName);
		}
		else {
			pStanza.Insert(84, pEncUsername);
		}
		
		pStanza.Insert(74, pEncInviteeJid);
		pStanza.Insert(13, pEncChannelJid);
	
		iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
		CleanupStack::PopAndDestroy(9); // aStanza, aEncInviteText, aInviteText, aEncChannelTitle, aEncChannelJid, aEncFullName, aEncUsername, aEncInviteeJid, aTextUtilities
	}
}

void CBuddycloudLogic::ChangeUsersChannelAffiliationL(const TDesC& aJid, TDesC& aChannelJid, TMucAffiliation aAffiliation) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ChangeUsersChannelAffiliationL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	HBufC8* aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aChannelJid).AllocLC();
	TPtr8 pEncChannelJid(aEncChannelJid->Des());
	
	HBufC8* aEncUserJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();
	TPtr8 pEncUserJid(aEncUserJid->Des());
	
	_LIT8(KStanza, "<iq to='' type='set' id='affiliation1'><query xmlns='http://jabber.org/protocol/muc#admin'><item jid='' affiliation=''/></query></iq>\r\n");	
	HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncUserJid.Length() + pEncChannelJid.Length() + KAffiliationOutcast().Length());
	TPtr8 pStanza(aStanza->Des());
	pStanza.Copy(KStanza);
	
	if(aAffiliation == EAffiliationOwner) {
		pStanza.Insert(117, KAffiliationOwner);
	}
	else if(aAffiliation == EAffiliationOutcast) {
		pStanza.Insert(117, KAffiliationOutcast);
	}
	else if(aAffiliation == EAffiliationMember) {
		pStanza.Insert(117, KAffiliationMember);
	}
	else if(aAffiliation == EAffiliationAdmin) {
		pStanza.Insert(117, KAffiliationAdmin);
	}
	else {
		pStanza.Insert(117, KAffiliationNone);
	}
	
	pStanza.Insert(102, pEncUserJid);
	pStanza.Insert(8, pEncChannelJid);

	iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
	CleanupStack::PopAndDestroy(4); // aStanza, aEncChannelJid, aEncUserJid, aTextUtilities	
}

void CBuddycloudLogic::ChangeUsersChannelRoleL(const TDesC& aJidOrNick, TDesC& aChannelJid, TMucRole aRole) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ChangeUsersChannelRoleL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	HBufC8* aEncChannelJid = aTextUtilities->UnicodeToUtf8L(aChannelJid).AllocLC();
	TPtr8 pEncChannelJid(aEncChannelJid->Des());
	
	HBufC8* aEncUserJidOrNick = aTextUtilities->UnicodeToUtf8L(aJidOrNick).AllocLC();
	TPtr8 pEncUserJidOrNick(aEncUserJidOrNick->Des());
	
	_LIT8(KItemJid, " jid=''");
	_LIT8(KItemNick, " nick=''");
	_LIT8(KRoleStanza, "<iq to='' type='set' id='role1'><query xmlns='http://jabber.org/protocol/muc#admin'><item role=''/></query></iq>\r\n");	
	HBufC8* aRoleStanza = HBufC8::NewLC(KRoleStanza().Length() + KItemNick().Length() + pEncUserJidOrNick.Length() + pEncChannelJid.Length() + KRoleParticipant().Length());
	TPtr8 pRoleStanza(aRoleStanza->Des());
	pRoleStanza.Copy(KRoleStanza);

	if(aRole == ERoleModerator) {
		pRoleStanza.Insert(96, KRoleModerator);
	}
	else if(aRole == ERoleParticipant) {
		pRoleStanza.Insert(96, KRoleParticipant);
	}
	else if(aRole == ERoleVisitor) {
		pRoleStanza.Insert(96, KRoleVisitor);
	}
	else {
		pRoleStanza.Insert(96, KRoleNone);
	}
	
	if(pEncUserJidOrNick.Locate('@') == KErrNotFound) {
		pRoleStanza.Insert(89, KItemNick);
		pRoleStanza.Insert(96, pEncUserJidOrNick);
	}
	else {
		pRoleStanza.Insert(89, KItemJid);
		pRoleStanza.Insert(95, pEncUserJidOrNick);
	}
	
	pRoleStanza.Insert(8, pEncChannelJid);

	iXmppEngine->SendAndForgetXmppStanza(pRoleStanza, true);
	CleanupStack::PopAndDestroy(4); // aRoleStanza, aEncChannelJid, aEncUserJid, aTextUtilities	
}

void CBuddycloudLogic::SetMoodL(TDesC& aMood) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetMoodL"));
#endif

	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	TPtrC8 aEncodedMood(aTextUtilities->UnicodeToUtf8L(aMood));

	_LIT8(KStanza, "<iq type='set' id='setmood1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><publish node='http://jabber.org/protocol/mood'><item id='current'><mood xmlns='http://jabber.org/protocol/mood'><text></text></mood></item></publish></pubsub></iq>\r\n");
	HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aEncodedMood.Length());
	TPtr8 pStanza = aStanza->Des();
	pStanza.Append(KStanza);
	pStanza.Insert(198, aEncodedMood);

	iXmppEngine->SendAndAcknowledgeXmppStanza(pStanza, this, true);
	CleanupStack::PopAndDestroy(2); // aStanza, aTextUtilities
	
	// Set Activity Status
	if(iState == ELogicOnline) {
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
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
		CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceById(iLocationEngine->GetLastPlaceId());
		
		if(aPlace != NULL) {
			HBufC* aItem = HBufC::NewLC(4 + aPlace->GetPlaceName().Length());
			TPtr pItem(aItem->Des());
			pItem.Append(_L("4\t\t\t"));
			pItem.Insert(2, aPlace->GetPlaceName());
			
			aDynItems.Append(aItem);
			aDynItemIds.Append(aPlace->GetPlaceId());
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
			CBuddycloudExtendedPlace* aNewPlace = iMasterPlaceList->GetPlaceById(KEditingNewPlaceId);
			
			if(aNewPlace == NULL) {
				aNewPlace = CBuddycloudExtendedPlace::NewLC();
				aNewPlace->SetPlaceId(KEditingNewPlaceId);
				aNewPlace->SetShared(false);
				
				iMasterPlaceList->AddPlace(aNewPlace);
				CleanupStack::Pop(); // aNewPlace
			}
			
			iMasterPlaceList->SetEditedPlace(KEditingNewPlaceId);
						
			if(iOwnItem) {
				CBuddycloudBasicPlace* aOwnPlace = iOwnItem->GetPlace(EPlaceCurrent);
				
				if(aOwnPlace) {
					aNewPlace->SetPlaceStreetL(aOwnPlace->GetPlaceStreet());
					aNewPlace->SetPlaceAreaL(aOwnPlace->GetPlaceArea());
					aNewPlace->SetPlaceCityL(aOwnPlace->GetPlaceCity());
					aNewPlace->SetPlacePostcodeL(aOwnPlace->GetPlacePostcode());
					aNewPlace->SetPlaceRegionL(aOwnPlace->GetPlaceRegion());
					aNewPlace->SetPlaceCountryL(aOwnPlace->GetPlaceCountry());
				}
			}
			
			NotifyNotificationObservers(ENotificationEditPlaceRequested, aNewPlace->GetPlaceId());
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

void CBuddycloudLogic::CollectPubsubNodesL(TDesC& aJid) {
	if(aJid.Locate('@') != KErrNotFound) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		TPtrC8 aEncJid(aTextUtilities->UnicodeToUtf8L(aJid));

		// Collect mood item
		_LIT8(KPubStanzaMood, "<iq to='' type='get' id='items1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node='http://jabber.org/protocol/mood'/></pubsub></iq>\r\n");
		HBufC8* aMoodStanza = HBufC8::NewLC(aEncJid.Length() + KPubStanzaMood().Length());
		TPtr8 pMoodStanza = aMoodStanza->Des();
		pMoodStanza.Append(KPubStanzaMood);
		pMoodStanza.Insert(8, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pMoodStanza, false);
		CleanupStack::PopAndDestroy(); // aMoodStanza

		// Collect geoloc-prev item
		_LIT8(KPubStanzaGeoPrev, "<iq to='' type='get' id='items1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node='http://jabber.org/protocol/geoloc-prev'/></pubsub></iq>\r\n");
		HBufC8* aGeoPrevStanza = HBufC8::NewLC(aEncJid.Length() + KPubStanzaGeoPrev().Length());
		TPtr8 pGeoPrevStanza = aGeoPrevStanza->Des();
		pGeoPrevStanza.Append(KPubStanzaGeoPrev);
		pGeoPrevStanza.Insert(8, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pGeoPrevStanza, false);
		CleanupStack::PopAndDestroy(); // aGeoPrevStanza

		// Collect geoloc item
		_LIT8(KPubStanzaGeo, "<iq to='' type='get' id='items1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node='http://jabber.org/protocol/geoloc'/></pubsub></iq>\r\n");
		HBufC8* aGeoStanza = HBufC8::NewLC(aEncJid.Length() + KPubStanzaGeo().Length());
		TPtr8 pGeoStanza = aGeoStanza->Des();
		pGeoStanza.Append(KPubStanzaGeo);
		pGeoStanza.Insert(8, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pGeoStanza, false);
		CleanupStack::PopAndDestroy(); // aGeoStanza

		// Collect geoloc-next item
		_LIT8(KPubStanzaGeoNext, "<iq to='' type='get' id='items1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node='http://jabber.org/protocol/geoloc-next'/></pubsub></iq>\r\n");
		HBufC8* aGeoNextStanza = HBufC8::NewLC(aEncJid.Length() + KPubStanzaGeoNext().Length());
		TPtr8 pGeoNextStanza = aGeoNextStanza->Des();
		pGeoNextStanza.Append(KPubStanzaGeoNext);
		pGeoNextStanza.Insert(8, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pGeoNextStanza, false);
		CleanupStack::PopAndDestroy(); // aGeoNextStanza

		// Collect profile item
		_LIT8(KPubStanzaProfile, "<iq to='' type='get' id='items1'><pubsub xmlns='http://jabber.org/protocol/pubsub'><items node='http://www.xmpp.org/extensions/xep-0154.html#ns'/></pubsub></iq>\r\n");
		HBufC8* aProfileStanza = HBufC8::NewLC(aEncJid.Length() + KPubStanzaProfile().Length());
		TPtr8 pProfileStanza = aProfileStanza->Des();
		pProfileStanza.Append(KPubStanzaProfile);
		pProfileStanza.Insert(8, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pProfileStanza, false);
		CleanupStack::PopAndDestroy(2); // aProfileStanza, aTextUtilities
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
		for(TInt i = 1; i < iMasterFollowingList->Count(); i++) {
			CFollowingItem* aListItem = iMasterFollowingList->GetItemByIndex(i);

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
				
//				if(aBubblingPrivateUnread > aListPrivateUnread || 
//						(aBubblingPrivateUnread == aListPrivateUnread && 
//								(aBubblingChannelUnread > aListChannelUnread || aBubblingChannelUnread == aListChannelUnread))) {
//					// If more unread pm's or (equal pm's and (more unread cm's or equal cm's))
//					
//					return i;
//				}
			}
			else if(aListItem->GetItemType() == EItemContact) {
				return i;
			}
		}
	}
	
	return iMasterFollowingList->Count();
}

void CBuddycloudLogic::AllMessagesRead(CFollowingItem* aItem) {
	if(iOwnItem && aItem) {
		if(iOwnItem->GetItemId() != aItem->GetItemId()) {
			// Move Item
			iMasterFollowingList->RemoveItemById(aItem->GetItemId());
			iMasterFollowingList->InsertItem(GetBubbleToPosition(aItem), aItem);
		}
	}
	
	iSettingsSaveNeeded = true;

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
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

	CFollowingItem* aPair1 = iMasterFollowingList->GetItemById(aPairItemId1);
	CFollowingItem* aPair2 = iMasterFollowingList->GetItemById(aPairItemId2);

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
					iMasterFollowingList->InsertItem(iMasterFollowingList->GetIndexById(aContactItem->GetItemId()), aBuddycloudContactItem);
					CleanupStack::Pop();
				}

				// Copy data
				aRosterItem->SetLastUpdated(TimeStamp());
				aRosterItem->SetAddressbookId(aContactItem->GetAddressbookId());

				// Delete contact item
				iMasterFollowingList->DeleteItemById(aContactItem->GetItemId());
			}
		}
	}
}

void CBuddycloudLogic::UnpairItemsL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::UnpairItemsL"));
#endif

	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aItemId);

	if(aItem && aItem->GetItemType() == EItemRoster) {
		CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

		if(aRosterItem->GetAddressbookId() >= 0) {
			if(iSettingShowContacts) {
				// Create new ContactItem
				CFollowingContactItem* aContactItem = CFollowingContactItem::NewLC();
				aContactItem->SetItemId(iNextItemId++);
				aContactItem->SetAddressbookId(aRosterItem->GetAddressbookId());
				
				iMasterFollowingList->AddItem(aContactItem);
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
	
	for(TInt i = iMasterFollowingList->Count() - 1; i >= 0; i--) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);
		
		if(aItem->GetItemType() == EItemContact) {
			iMasterFollowingList->DeleteItemByIndex(i);
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

	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aItemId);
	
	if(aItem != NULL) {
		if(aItem->GetItemType() == EItemContact || aItem->GetItemType() == EItemRoster) {
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
		for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
			iMasterFollowingList->GetItemByIndex(i)->SetFiltered(true);
		}
	}
	else {
		TBool aToFilter;

		iFilteringContacts = true;

		for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
			CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

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
					TPtrC aPlaceName(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName());
					
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
		for(TInt i = 0;i < iMasterFollowingList->Count();i++) {
			CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

			if(aItem->GetItemType() == EItemRoster) {
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
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aFollowerId);

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
	if(aContact != NULL) {
		TBool aFound = false;
		
		for(TInt i = 0;i < iMasterFollowingList->Count();i++) {
			CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

			if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetAddressbookId() == aContactId) {
					aFound = true;
					break;
				}
			}
		}

		// Not paired. Add as contact and index
		if( !aFound ) {
			CFollowingContactItem* aContactItem = CFollowingContactItem::NewLC();
			aContactItem->SetItemId(iNextItemId++);
			aContactItem->SetAddressbookId(aContactId);
			aContactItem->SetFiltered(!iFilteringContacts);

			// Insert
			iMasterFollowingList->AddItem(aContactItem);
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
			for(TInt i = 0;i < iMasterFollowingList->Count();i++) {
				CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

				if(aItem->GetItemType() >= EItemContact) {
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
				
				iMasterFollowingList->InsertItem(GetBubbleToPosition(aNewItem), aNewItem);
				CleanupStack::Pop();
			}
		}
		else if(aEvent.iType == EContactDbObserverEventContactDeleted) {
			for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
				CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

				if(aItem->GetItemType() >= EItemContact) {
					CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aItem);

					if(aContactItem->GetAddressbookId() == aEvent.iContactId) {
						if(aItem->GetItemType() == EItemContact) {
							iMasterFollowingList->DeleteItemByIndex(i);
						}
						else if(aItem->GetItemType() == EItemRoster) {
							CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
							aRosterItem->SetAddressbookId(KErrNotFound);

							TPtrC aName(aRosterItem->GetJid());
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
		CTimeCoder* aTimeCoder = CTimeCoder::NewLC();

		if(aXmlParser->MoveToElement(_L8("logicstate"))) {
			// Last Received At
			TPtrC8 pAttributeLast = aXmlParser->GetStringAttribute(_L8("last"));

			if(pAttributeLast.Length() > 0) {
				iLastReceivedAt = aTimeCoder->DecodeL(pAttributeLast);
			}
			
			HBufC* aAppVersion = CEikonEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
			TPtrC aEncVersion(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("version"))));
			
			if(aEncVersion.Compare(*aAppVersion) != 0) {
				iSettingNewInstall = true;
			}
			
			WriteToLog(aTextUtilities->UnicodeToUtf8L(*aAppVersion));
			CleanupStack::PopAndDestroy(); // aAppVersion

			if(aXmlParser->MoveToElement(_L8("settings"))) {
				iSettingConnectionMode = aXmlParser->GetIntAttribute(_L8("apmode"));
				iSettingConnectionModeId = aXmlParser->GetIntAttribute(_L8("apid"));
				
				HBufC* aApName = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("apname"))).AllocLC();
				
#ifdef _DEBUG
				iSettingTestServer = aXmlParser->GetBoolAttribute(_L8("test"));
#endif
				
				if(aXmlParser->MoveToElement(_L8("account"))) {
					iSettingFullName.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("fullname"))));	
					iSettingEmailAddress.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("email"))));
					iSettingPhoneNumber.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("phonenum"))));
					iSettingUsername.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("username"))));
					iSettingPassword.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("password"))));
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
					iSettingGroupNotification = aXmlParser->GetIntAttribute(_L8("groupnotify"));
					iSettingAccessPoint = aXmlParser->GetBoolAttribute(_L8("accesspoint"));
					
					// Reset access point
					if(iSettingAccessPoint) {
						iSettingConnectionMode = 0;
						iSettingConnectionModeId = 0;
					}
					
					TPtrC8 pAttributeNotifyReplyTo = aXmlParser->GetStringAttribute(_L8("replyto"));
					if(pAttributeNotifyReplyTo.Length() > 0) {
						iSettingNotifyReplyTo = aXmlParser->GetBoolAttribute(_L8("replyto"));
					}
					
					TPtrC8 pAttributeShowName = aXmlParser->GetStringAttribute(_L8("showname"));
					if(pAttributeShowName.Length() > 0) {
						iSettingShowName = aXmlParser->GetBoolAttribute(_L8("showname"));
					}
					
					TPtrC8 pAttributeShowContacts = aXmlParser->GetStringAttribute(_L8("showcontacts"));
					if(pAttributeShowContacts.Length() > 0) {
						iSettingShowContacts = aXmlParser->GetBoolAttribute(_L8("showcontacts"));
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
				TInt aAvatarId = aXmlParser->GetIntAttribute(_L8("avatar_id"));
				
				CXmlParser* aItemXml = CXmlParser::NewLC(aXmlParser->GetStringData());

				if(aItemXml->MoveToElement(_L8("notice"))) {
					CFollowingNoticeItem* aNoticeItem = CFollowingNoticeItem::NewLC();
					aNoticeItem->SetItemId(iNextItemId++);
					aNoticeItem->SetJidL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), TJidType(aItemXml->GetIntAttribute(_L8("type"))));
					aNoticeItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aNoticeItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					
					iMasterFollowingList->AddItem(aNoticeItem);
					CleanupStack::Pop(); // aNoticeItem
				}
				else if(aItemXml->MoveToElement(_L8("group"))) {
					CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
					aChannelItem->SetItemId(iNextItemId++);
					aChannelItem->SetExternal(aItemXml->GetBoolAttribute(_L8("external")));						
					aChannelItem->SetRank(aItemXml->GetIntAttribute(_L8("rank")), aItemXml->GetIntAttribute(_L8("shift")));						
					aChannelItem->SetJidL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aChannelItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aChannelItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));

					if(aAvatarId != 0) {
						aChannelItem->SetAvatarId(aAvatarId);
					}
					
					if(pTimeAttribute.Length() > 0) {
						aChannelItem->SetLastUpdated(aTimeCoder->DecodeL(pTimeAttribute));
					}

					CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());
					aDiscussion->AddMessageReadObserver(aChannelItem);
					aDiscussion->AddDiscussionReadObserver(this, aChannelItem);
					aDiscussion->SetUnreadMessages(aItemXml->GetIntAttribute(_L8("unread")), aItemXml->GetIntAttribute(_L8("replies")));

					iMasterFollowingList->AddItem(aChannelItem);
					CleanupStack::Pop(); // aChannelItem
				}
				else if(aItemXml->MoveToElement(_L8("roster"))) {
					CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
					aRosterItem->SetItemId(iNextItemId++);
					aRosterItem->SetJidL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))));
					aRosterItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("name"))));
					aRosterItem->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("description"))));
					aRosterItem->SetIsOwn(aItemXml->GetBoolAttribute(_L8("own")));

					if(aAvatarId != 0) {
						aRosterItem->SetAvatarId(aAvatarId);
					}
					
					// Addressbook Id
					TPtrC8 pAttributeAbid = aItemXml->GetStringAttribute(_L8("abid"));
					if(pAttributeAbid.Length() > 0) {
						aRosterItem->SetAddressbookId(aItemXml->GetIntAttribute(_L8("abid")));
					}
					
					CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid());
					aDiscussion->AddMessageReadObserver(aRosterItem);
					aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
					aDiscussion->SetUnreadMessages(aItemXml->GetIntAttribute(_L8("unread")));
					
					// Channel
					if(aItemXml->MoveToElement(_L8("channel"))) {
						aRosterItem->SetRank(aItemXml->GetIntAttribute(_L8("rank")), aItemXml->GetIntAttribute(_L8("shift")));						
						aRosterItem->SetJidL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("jid"))), EJidChannel);
						
						aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid(EJidChannel));
						aDiscussion->AddMessageReadObserver(aRosterItem);
						aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
						aDiscussion->SetUnreadMessages(aItemXml->GetIntAttribute(_L8("unread")), aItemXml->GetIntAttribute(_L8("replies")));							
					}

					// Location
					while(aItemXml->MoveToElement(_L8("location"))) {
						// Type
						CBuddycloudBasicPlace* aNewPlace = CBuddycloudBasicPlace::NewLC();
						TInt aPlaceType = aItemXml->GetIntAttribute(_L8("type"));
						aNewPlace->SetPlaceNameL(aTextUtilities->Utf8ToUnicodeL(aItemXml->GetStringAttribute(_L8("text"))));
						aRosterItem->SetPlaceL(TBuddycloudPlaceType(aPlaceType), aNewPlace);
						CleanupStack::Pop(); // aNewPlace
					}

					if(aRosterItem->GetIsOwn()) {
						iOwnItem = aRosterItem;

						iMasterFollowingList->InsertItem(0, aRosterItem);

						if(iStatusObserver) {
							iStatusObserver->JumpToItem(aRosterItem->GetItemId());
						}
					}
					else {
						iMasterFollowingList->AddItem(aRosterItem);
					}

					CleanupStack::Pop();
				}

				CleanupStack::PopAndDestroy(); // aItemXml
			}
		}
		
		WriteToLog(_L8("BL    Internalize Logic State -- End"));

		CleanupStack::PopAndDestroy(4); // aTimeCoder, aTextUtilities, aXmlParser, aBuf
		CleanupStack::PopAndDestroy(&aFile);
	}
	else {
		iSettingNewInstall = true;
		iLastReceivedAt -= TTimeIntervalMinutes(30);
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
		CTimeCoder* aTimeCoder = CTimeCoder::NewLC();
		TFormattedTimeDesc aTime;

		aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?>"));
		aFile.WriteL(_L8("<logicstate last='"));
		aTimeCoder->EncodeL(iLastReceivedAt, aTime);
		aFile.WriteL(aTime);

		WriteToLog(aTime);
		
		HBufC* aVersion = CEikonEnv::Static()->AllocReadResourceLC(R_STRING_APPVERSION);
		aFile.WriteL(_L8("' version='"));
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(*aVersion));
		aFile.WriteL(_L8("'>"));
		CleanupStack::PopAndDestroy(); // aVersion

		// Settings
		aBuf.Format(_L8("<settings apmode='%d' apid='%d' apname='"), iSettingConnectionMode, iSettingConnectionModeId);
		aFile.WriteL(aBuf);
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iXmppEngine->GetConnectionName()));
		aBuf.Format(_L8("' test='%d'><account"), iSettingTestServer);
		aFile.WriteL(aBuf);

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

		if(iSettingPhoneNumber.Length() > 0) {
			aFile.WriteL(_L8(" phonenum='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingPhoneNumber));
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
		aBuf.Format(_L8("/><preferences language='%d' autostart='%d' groupnotify='%d' accesspoint='%d' replyto='%d' showname='%d' showcontacts='%d'/>"), iSettingPreferredLanguage, iSettingAutoStart, iSettingGroupNotification, iSettingAccessPoint, iSettingNotifyReplyTo, iSettingShowName, iSettingShowContacts);
		aFile.WriteL(aBuf);
		
		// Notifications
		aBuf.Format(_L8("<notifications pmt_id='%d' cmt_id='%d' drt_id='%d' pmt_file='"), iSettingPrivateMessageTone, iSettingChannelPostTone, iSettingDirectReplyTone);
		aFile.WriteL(aBuf);
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingPrivateMessageToneFile));
		aFile.WriteL(_L8("' cmt_file='"));
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingChannelPostToneFile));
		aFile.WriteL(_L8("' drt_file='"));
		aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingDirectReplyToneFile));
		
		// Beacons
		aBuf.Format(_L8("'/><beacons cell='%d' wlan='%d' bt='%d' gps='%d'/><communities>"), iSettingCellOn, iSettingWlanOn, iSettingBtOn, iSettingGpsOn);
		aFile.WriteL(aBuf);
		
		// Communities
		if(iSettingTwitterUsername.Length() > 0) {
			aFile.WriteL(_L8("<twitter username='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingTwitterUsername));
			aFile.WriteL(_L8("' password='"));
			aFile.WriteL(aTextUtilities->UnicodeToUtf8L(iSettingTwitterPassword));
			aFile.WriteL(_L8("'/>"));
		}
		
		aFile.WriteL(_L8("</communities></settings>"));

		// Write Unsent Messages
		CXmppOutbox* aUnsentOutbox = iXmppEngine->GetMessageOutbox();

		if(aUnsentOutbox->Count() > 0) {
			aFile.WriteL(_L8("<unsent>"));

			for(TInt i = 0; i < aUnsentOutbox->Count(); i++) {
				CXmppOutboxMessage* aMessage = aUnsentOutbox->GetMessage(i);
				
				if(aMessage->GetPersistance()) {
					aFile.WriteL(aMessage->GetStanza());
				}
			}

			aFile.WriteL(_L8("</unsent>"));
		}

		for(TInt i = 0; i < iMasterFollowingList->Count();i++) {
			CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

			if(aItem->GetItemType() == EItemNotice) {
				CFollowingNoticeItem* aNoticeItem = static_cast <CFollowingNoticeItem*> (aItem);
				
				aFile.WriteL(_L8("<item last='"));
				aTimeCoder->EncodeL(aItem->GetLastUpdated(), aTime);
				aFile.WriteL(aTime);
				aBuf.Format(_L8("'><notice type='%d' jid='"), aNoticeItem->GetJidType());
				aFile.WriteL(aBuf);

				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aNoticeItem->GetJid()));
				aFile.WriteL(_L8("'"));

				if(aNoticeItem->GetTitle().Length() > 0) {
					aFile.WriteL(_L8(" name='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aNoticeItem->GetTitle()));
					aFile.WriteL(_L8("'"));
				}

				if(aNoticeItem->GetDescription().Length() > 0) {
					aFile.WriteL(_L8(" description='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aNoticeItem->GetDescription()));
					aFile.WriteL(_L8("'"));
				}
				
				aFile.WriteL(_L8("/></item>"));
			}
			else if(aItem->GetItemType() == EItemChannel) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);

				aFile.WriteL(_L8("<item last='"));
				aTimeCoder->EncodeL(aItem->GetLastUpdated(), aTime);
				aFile.WriteL(aTime);
				aBuf.Format(_L8("' avatar_id='%d'><group external='%d' rank='%d' shift='%d'"), aChannelItem->GetAvatarId(), aChannelItem->IsExternal(), aChannelItem->GetRank(), aChannelItem->GetRankShift());
				aFile.WriteL(aBuf);

				if(aChannelItem->GetJid().Length() > 0) {
					aFile.WriteL(_L8(" jid='"));
					aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aChannelItem->GetJid()));
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
				
				CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());

				if(aDiscussion->GetUnreadMessages() > 0) {
					aBuf.Format(_L8(" unread='%d' replies='%d'"), aDiscussion->GetUnreadMessages(), aDiscussion->GetUnreadReplies());
					aFile.WriteL(aBuf);
				}

				aFile.WriteL(_L8("/></item>"));
			}
			else if(aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(!iRosterSynchronized || aRosterItem->GetSubscriptionType() > ESubscriptionNone) {
					aFile.WriteL(_L8("<item last='"));
					aTimeCoder->EncodeL(aItem->GetLastUpdated(), aTime);
					aFile.WriteL(aTime);
					aBuf.Format(_L8("' avatar_id='%d'><roster"), aRosterItem->GetAvatarId());
					aFile.WriteL(aBuf);

					if(aRosterItem->GetJid().Length() > 0) {
						aFile.WriteL(_L8(" jid='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid()));
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

					aBuf.Format(_L8(" own='%d'"), aRosterItem->GetIsOwn());
					aFile.WriteL(aBuf);

					if(aRosterItem->GetAddressbookId() != -1) {
						aBuf.Format(_L8(" abid='%d'"), aRosterItem->GetAddressbookId());
						aFile.WriteL(aBuf);
					}
					
					CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid());

					if(aDiscussion->GetUnreadMessages() > 0) {
						aBuf.Format(_L8(" unread='%d'"), aDiscussion->GetUnreadMessages());
						aFile.WriteL(aBuf);
					}

					aFile.WriteL(_L8(">"));

					if(aRosterItem->GetJid(EJidChannel).Length() > 0) {
						aBuf.Format(_L8("<channel rank='%d' shift='%d' jid='"), aRosterItem->GetRank(), aRosterItem->GetRankShift());
						aFile.WriteL(aBuf);
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid(EJidChannel)));
						aFile.WriteL(_L8("'"));
						
						aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid(EJidChannel));

						if(aDiscussion->GetUnreadMessages() > 0) {
							aBuf.Format(_L8(" unread='%d' replies='%d'"), aDiscussion->GetUnreadMessages(), aDiscussion->GetUnreadReplies());
							aFile.WriteL(aBuf);
						}
						
						aFile.WriteL(_L8("/>"));
					}

					// Place
					if(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName().Length() > 0) {
						aFile.WriteL(_L8("<location type='0' text='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName()));
						aFile.WriteL(_L8("'/>"));
					}

					if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
						aFile.WriteL(_L8("<location type='1' text='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName()));
						aFile.WriteL(_L8("'/>"));
					}

					if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
						aFile.WriteL(_L8("<location type='2' text='"));
						aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName()));
						aFile.WriteL(_L8("'/>"));
					}

					aFile.WriteL(_L8("</roster></item>"));
				}
			}
		}

		aFile.WriteL(_L8("</logicstate></?xml?>"));
		
		WriteToLog(_L8("BL    Externalize Logic State -- End"));

		CleanupStack::PopAndDestroy(2); // aTimecoder, aTextUtilities
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::AddChannelItemL(TPtrC aJid, TPtrC aName, TPtrC aTopic, TInt aRank) {
	if( !IsSubscribedTo(aJid, EItemChannel) ) {
		CFollowingChannelItem* aChannelItem = CFollowingChannelItem::NewLC();
		aChannelItem->SetItemId(iNextItemId++);
		aChannelItem->SetJidL(aJid);
		aChannelItem->SetTitleL(aName);
		aChannelItem->SetDescriptionL(aTopic);
		aChannelItem->SetRank(aRank);
		
		CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());
		aDiscussion->AddMessageReadObserver(aChannelItem);
		aDiscussion->AddDiscussionReadObserver(this, aChannelItem);
	
		iMasterFollowingList->InsertItem(GetBubbleToPosition(aChannelItem), aChannelItem);
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
		CTimeCoder* aTimeCoder = CTimeCoder::NewLC();

		if(aXmlParser->MoveToElement(_L8("places"))) {
			while(aXmlParser->MoveToElement(_L8("place"))) {
				CBuddycloudExtendedPlace* aPlace = CBuddycloudExtendedPlace::NewLC();

				aPlace->SetPlaceId(aXmlParser->GetIntAttribute(_L8("id")));
				aPlace->SetRevision(aXmlParser->GetIntAttribute(_L8("revision"), -1));
				aPlace->SetPlaceNameL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("name"))));
				aPlace->SetShared(aXmlParser->GetBoolAttribute(_L8("public")));

				if(aXmlParser->MoveToElement(_L8("address"))) {
					aPlace->SetPlaceStreetL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("street"))));
					aPlace->SetPlaceAreaL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("area"))));
					aPlace->SetPlaceCityL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("city"))));
					aPlace->SetPlacePostcodeL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("postcode"))));
					aPlace->SetPlaceRegionL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("region"))));
					aPlace->SetPlaceCountryL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("country"))));
				}
				
				if(aXmlParser->MoveToElement(_L8("extended"))) {
					aPlace->SetDescriptionL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("description"))));
					aPlace->SetWebsiteL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("site"))));
					aPlace->SetWikiL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("wiki"))));
				}

				if(aXmlParser->MoveToElement(_L8("visit"))) {
					aPlace->SetVisits(aXmlParser->GetIntAttribute(_L8("count")));
					aPlace->SetPlaceSeen(TBuddycloudPlaceSeen(aXmlParser->GetIntAttribute(_L8("type"))));
					aPlace->SetLastSeen(aTimeCoder->DecodeL(aXmlParser->GetStringAttribute(_L8("lasttime"))));
					aPlace->SetVisitSeconds(aXmlParser->GetIntAttribute(_L8("lastsecs")));
					aPlace->SetTotalSeconds(aXmlParser->GetIntAttribute(_L8("totalsecs")));
				}

				if(aXmlParser->MoveToElement(_L8("position"))) {
					// Latitude
					TPtrC8 pAttributeLatitude = aXmlParser->GetStringAttribute(_L8("latitude"));
					if(pAttributeLatitude.Length() > 0) {
						TLex8 aPlaceLatitude(pAttributeLatitude);
						TReal aLatitude;
						aPlaceLatitude.Val(aLatitude);
						aPlace->SetPlaceLatitude(aLatitude);
					}

					// Longitude
					TPtrC8 pAttributeLongitude = aXmlParser->GetStringAttribute(_L8("longitude"));
					if(pAttributeLongitude.Length() > 0) {
						TLex8 aPlaceLongitude(pAttributeLongitude);
						TReal aLongitude;
						aPlaceLongitude.Val(aLongitude);
						aPlace->SetPlaceLongitude(aLongitude);
					}
				}

				iMasterPlaceList->AddPlace(aPlace);
				CleanupStack::Pop(); // aPlace
			}
		}

		CleanupStack::PopAndDestroy(4); // aTimeCoder, aTextUtilities, aXmlParser, aBuf
		CleanupStack::PopAndDestroy(&aFile);
	}
}

void CBuddycloudLogic::SavePlaceItemsL() {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SavePlaceItemsL"));
#endif

	RFs aSession = CCoeEnv::Static()->FsSession();	
	RFileWriteStream aFile;
	TBuf8<128> aBuf;

	TFileName aFilePath(_L("PlaceStoreItems.xml"));
	CFileUtilities::CompleteWithApplicationPath(aSession, aFilePath);

	if(aFile.Replace(aSession, aFilePath, EFileStreamText|EFileWrite) == KErrNone) {
		CleanupClosePushL(aFile);

		WriteToLog(_L8("BL    Externalize Place Store"));

		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		CTimeCoder* aTimeCoder = CTimeCoder::NewLC();
		TFormattedTimeDesc aTime;

		aFile.WriteL(_L8("<?xml version='1.0' encoding='UTF-8'?><places>"));

		TTime aTimeNow;
		aTimeNow.UniversalTime();

		for(TInt i = 0;i < iMasterPlaceList->CountPlaces();i++) {
			CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceByIndex(i);

			if(aPlace && aPlace->GetPlaceId() > 0) {
				// Place
				aBuf.Format(_L8("<place id='%d' revision='%d' public='%d'"), aPlace->GetPlaceId(), aPlace->GetRevision(), aPlace->GetShared());
				aFile.WriteL(aBuf);

				aFile.WriteL(_L8(" name='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceName()));

				// Address
				aFile.WriteL(_L8("'><address street='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceStreet()));

				aFile.WriteL(_L8("' area='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceArea()));

				aFile.WriteL(_L8("' city='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceCity()));
				
				aFile.WriteL(_L8("' postcode='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlacePostcode()));

				aFile.WriteL(_L8("' region='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceRegion()));

				aFile.WriteL(_L8("' country='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceCountry()));
				
				// Extended
				aFile.WriteL(_L8("'/><extended description='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetDescription()));

				aFile.WriteL(_L8("' site='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetWebsite()));

				aFile.WriteL(_L8("' wiki='"));
				aFile.WriteL(aTextUtilities->UnicodeToUtf8L(aPlace->GetWiki()));

				// Visit
				aBuf.Format(_L8("'/><visit count='%d'"), aPlace->GetVisits());
				aFile.WriteL(aBuf);

				if(aPlace->GetPlaceSeen() == EPlaceHere || aPlace->GetPlaceSeen() == EPlaceFlux) {
					aBuf.Format(_L8(" type='%d'"), EPlaceRecent);
				}
				else {
					aBuf.Format(_L8(" type='%d'"), aPlace->GetPlaceSeen());
				}
				aFile.WriteL(aBuf);

				aFile.WriteL(_L8(" lasttime='"));
				aTimeCoder->EncodeL(aPlace->GetLastSeen(), aTime);
				aFile.WriteL(aTime);

				aBuf.Format(_L8("' lastsecs='%d' totalsecs='%d'/>"), aPlace->GetVisitSeconds(), aPlace->GetTotalSeconds());
				aFile.WriteL(aBuf);

				// Position
				aBuf.Format(_L8("<position latitude='%.6f' longitude='%.6f'/></place>"), aPlace->GetPlaceLatitude(), aPlace->GetPlaceLongitude());
				aFile.WriteL(aBuf);
			}
		}

		aFile.WriteL(_L8("</places></?xml?>"));

		CleanupStack::PopAndDestroy(2); // aTimeCoder, aTextUtilities
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

CBuddycloudFollowingStore* CBuddycloudLogic::GetFollowingStore() {
	return iMasterFollowingList;
}

CFollowingRosterItem* CBuddycloudLogic::GetOwnItem() {
	return iOwnItem;
}

void CBuddycloudLogic::DeleteItemL(TInt aItemId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::DeleteItemL"));
#endif

	CFollowingItem* aItem = iMasterFollowingList->GetItemById(aItemId);

	if(aItem) {
		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(!aRosterItem->GetIsOwn()) {
				if(aRosterItem->GetSubscriptionType() > ESubscriptionRemove) {
					// Create subscription remove stanza
					CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
					TPtrC8 aEncJid(aTextUtilities->UnicodeToUtf8L(aRosterItem->GetJid()));
	
					_LIT8(KStanza, "<iq type='set' id='remove1'><query xmlns='jabber:iq:roster'><item subscription='remove' jid=''/></query></iq>\r\n");
					HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + aEncJid.Length());
					TPtr8 pStanza(aStanza->Des());
					pStanza.Append(KStanza);
					pStanza.Insert(93, aEncJid);
					
					iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
					CleanupStack::PopAndDestroy(2); // aStanza, aTextUtilities
					
					// Remove from my personal channel membership
					ChangeUsersChannelAffiliationL(aRosterItem->GetJid(), iOwnItem->GetJid(EJidChannel), EAffiliationNone);
				}
				
				// Unfollow their personal channel
				UnfollowChannelL(aItemId);

				// Unpair and remove item
				if(aRosterItem->GetAddressbookId() != KErrNotFound) {
					UnpairItemsL(aItemId);
				}
			
				// Delete discussion and remove item
				iMessagingManager->DeleteDiscussionL(aRosterItem->GetJid());
				iMasterFollowingList->DeleteItemById(aItemId);
				
				NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);
				
				iSettingsSaveNeeded = true;
			}
		}
		else if(aItem->GetItemType() == EItemChannel) {
			UnfollowChannelL(aItemId);
		}		
		
		iSettingsSaveNeeded = true;
	}
}

/*
----------------------------------------------------------------------------
--
-- Messaging
--
----------------------------------------------------------------------------
*/

TPtrC CBuddycloudLogic::GetRawJid(TDesC& aJid) {
	TPtrC pJid(aJid);
	TInt aSearch = pJid.Locate('/');
	
	if(aSearch != KErrNotFound) {
		return pJid.Left(aSearch);
	}
	
	return pJid;
}

TInt CBuddycloudLogic::IsSubscribedTo(const TDesC& aJid, TInt aItemOptions) {
	for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);
		
		if(aItemOptions & EItemNotice && aItem->GetItemType() == EItemNotice) {
			CFollowingNoticeItem* aNoticeItem = static_cast <CFollowingNoticeItem*> (aItem);
			
			if(aNoticeItem->GetJid().CompareF(aJid) == 0) {
				// Check notice jid
				return aItem->GetItemId();
			}		
		}
		else if(aItem->GetItemType() >= EItemRoster) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
			
			if(aItemOptions & EItemChannel && aChannelItem->GetJid().CompareF(aJid) == 0) {
				// Check topic & personal channel jids
				return aItem->GetItemId();
			}
			else if(aItemOptions & EItemRoster && aItem->GetItemType() == EItemRoster) {
				CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
				
				if(aRosterItem->GetJid().CompareF(aJid) == 0) {
					// Check roster jid
					if(aRosterItem->GetSubscriptionType() >= ESubscriptionTo) {
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
		CFollowingItem* aItem = iMasterFollowingList->GetItemById(aItemId);
		
		if(aItem && aItem->GetItemType() == EItemNotice) {
			CFollowingNoticeItem* aNoticeItem = static_cast <CFollowingNoticeItem*> (aItem);
			
			if(aNoticeItem->GetJidType() == EJidChannel) {
				if(aResponse == ENoticeAccept) {
					TPtrC pDescription(aNoticeItem->GetDescription());
					TInt aSearchResult = pDescription.Locate(':');
					
					if(aSearchResult != KErrNotFound) {
						pDescription.Set(pDescription.Mid(aSearchResult + 2));
					}
					
					FollowTopicChannelL(aNoticeItem->GetJid(), pDescription);
				}
				else if(aResponse == ENoticeDecline) {
					DeclineChannelInviteL(aNoticeItem->GetJid());
				}
			}
			else if(aNoticeItem->GetJidType() == EJidRoster) {
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
				
				HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aNoticeItem->GetJid()).AllocLC();
				TPtr8 pEncJid(aEncJid->Des());
				
				if(aResponse <= ENoticeAcceptAndFollow) {
					// Update personal channel affiliation
					ChangeUsersChannelAffiliationL(aNoticeItem->GetJid(), iOwnItem->GetJid(EJidChannel), EAffiliationMember);		
									
					// Accept request
					_LIT8(KAcceptStanza, "<presence to='' type='subscribed'/>\r\n");
					HBufC8* aAcceptStanza = HBufC8::NewLC(KAcceptStanza().Length() + pEncJid.Length());
					TPtr8 pAcceptStanza(aAcceptStanza->Des());
					pAcceptStanza.Append(KAcceptStanza);
					pAcceptStanza.Insert(14, pEncJid);
	
					iXmppEngine->SendAndForgetXmppStanza(pAcceptStanza, true);
					CleanupStack::PopAndDestroy(); // aAcceptStanza				
					
					if(aResponse == ENoticeAcceptAndFollow) {
						// Follow requester back
						FollowContactL(aNoticeItem->GetJid());
					}
				}
				else {
					// Deny user to access personal channel
					ChangeUsersChannelAffiliationL(aNoticeItem->GetJid(), iOwnItem->GetJid(EJidChannel), EAffiliationNone);
	
					// Unsubscribe to presence
					_LIT8(KStanza, "<presence to='' type='unsubscribed'/>\r\n");
					HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncJid.Length());
					TPtr8 pStanza(aStanza->Des());
					pStanza.Append(KStanza);
					pStanza.Insert(14, pEncJid);
	
					iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
					CleanupStack::PopAndDestroy(); // aStanza
				}
				
				CleanupStack::PopAndDestroy(2); // aEncJid, aTextUtilities
			}
			
			iMasterFollowingList->DeleteItemById(aItemId);
			
			NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItemId);
		}
	}
}

void CBuddycloudLogic::MediaPostRequestL(const TDesC& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::MediaPostRequestL"));
#endif

	if(iOwnItem) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		
		HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aJid).AllocLC();
		TPtr8 pEncJid(aEncJid->Des());
		TInt aLocate = pEncJid.Locate('@');
		
		if(aLocate != KErrNotFound) {
			CBuddycloudBasicPlace* aPlace = iOwnItem->GetPlace(EPlaceCurrent);
		
			_LIT8(KMediaStanza, "<iq to='media.buddycloud.com' type='set' id='mediareq_'><media xmlns='http://buddycloud.com/media#request'><placeid>%d</placeid><lat>%.6f</lat><lon>%.6f</lon><channeljid></channeljid></media></iq>\r\n");
			HBufC8* aMediaStanza = HBufC8::NewLC(KMediaStanza().Length() + aLocate + pEncJid.Length() + 32 + 22);
			TPtr8 pMediaStanza(aMediaStanza->Des());
			pMediaStanza.Format(KMediaStanza, iStablePlaceId, aPlace->GetPlaceLatitude(), aPlace->GetPlaceLongitude());
			pMediaStanza.Insert(pMediaStanza.Length() - 28, pEncJid);
			pMediaStanza.Insert(54, pEncJid.Left(aLocate));
		
			iXmppEngine->SendAndAcknowledgeXmppStanza(pMediaStanza, this, true);
			CleanupStack::PopAndDestroy(); // aMediaStanza
		}
		CleanupStack::PopAndDestroy(2); // aEncJid, aTextUtilities
	}
}

CDiscussion* CBuddycloudLogic::GetDiscussion(const TDesC& aJid) {
	return iMessagingManager->GetDiscussionL(aJid);			
}

void CBuddycloudLogic::SetTopicL(TDesC& aTopic, TDesC& aToJid, TBool aIsChannel) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SetTopicL"));
#endif

	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	HBufC8* aEncTopic = aTextUtilities->UnicodeToUtf8L(aTopic).AllocLC();
	TPtr8 pEncTopic = aEncTopic->Des();

	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aToJid).AllocLC();
	TPtr8 pEncJid(aEncJid->Des());

	_LIT8(KStanza, "<message type='' to='' id='message'><subject></subject></message>\r\n");
	HBufC8* aStanza = HBufC8::NewLC(KStanza().Length() + pEncJid.Length() + pEncTopic.Length() + KMessageTypeGroupchat().Length());
	TPtr8 pStanza(aStanza->Des());
	pStanza.Append(KStanza);
	pStanza.Insert(45, pEncTopic);
	pStanza.Insert(21, pEncJid);

	if(aIsChannel) {
		pStanza.Insert(15, KMessageTypeGroupchat);
	}
	else {
		pStanza.Insert(15, KMessageTypeChat);
	}

	iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
	CleanupStack::PopAndDestroy(4); // aStanza, aEncJid, aEncTopic, aTextUtilities
}

void CBuddycloudLogic::BuildNewMessageL(TDesC& aToJid, TDesC& aMessageText, TBool aIsChannel, CMessage* aReferenceMessage) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::BuildNewMessageL"));
#endif
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	

	HBufC8* aEncJid = aTextUtilities->UnicodeToUtf8L(aToJid).AllocLC();
	TPtr8 pEncJid(aEncJid->Des());
	
	HBufC8* aEncMessage = aTextUtilities->UnicodeToUtf8L(aMessageText).AllocLC();
	TPtr8 pEncMessage(aEncMessage->Des());

	_LIT8(KMessageStanza, "<message type='' to='' id='message'><body></body></message>\r\n");
	HBufC8* aStanza = HBufC8::NewL(KMessageStanza().Length() + pEncJid.Length() + pEncMessage.Length() + KMessageTypeGroupchat().Length());
	TPtr8 pStanza(aStanza->Des());
	pStanza.Append(KMessageStanza);
	pStanza.Insert(42, pEncMessage);
	pStanza.Insert(21, pEncJid);
	
	if(aIsChannel) {
		pStanza.Insert(15, KMessageTypeGroupchat);
		
		// Thread
		_LIT8(KThreadElement, "<thread></thread>");

		if(aReferenceMessage && aReferenceMessage->GetThread().Length() > 0) {
			aStanza = aStanza->ReAlloc(pStanza.Length() + KThreadElement().Length() + aReferenceMessage->GetThread().Length());
			pStanza.Set(aStanza->Des());
			pStanza.Insert(pStanza.Length() - 12, KThreadElement);
			pStanza.Insert(pStanza.Length() - 12 - 9, aReferenceMessage->GetThread());
		}
		else {
			TPtrC8 aThread(GenerateNewThreadLC());
			
			aStanza = aStanza->ReAlloc(pStanza.Length() + KThreadElement().Length() + aThread.Length());
			pStanza.Set(aStanza->Des());
			pStanza.Insert(pStanza.Length() - 12, KThreadElement);
			pStanza.Insert(pStanza.Length() - 12 - 9, aThread);
					
			CleanupStack::PopAndDestroy();
		}
	}
	else {
		pStanza.Insert(15, KMessageTypeChat);
		
		// Insert message into discussion
		CMessage* aMessage = CMessage::NewLC();
		aMessage->SetReceivedAt(TimeStamp());
		aMessage->SetPrivate(true);
		aMessage->SetRead(true);
		aMessage->SetOwn(true);
		aMessage->SetJidL(iOwnItem->GetJid(), EMessageJidRoster);
		
		if(iSettingFullName.Length() > 0) {
			aMessage->SetNameL(iSettingFullName);
		}
		else {
			aMessage->SetNameL(iSettingUsername);
		}
		
		aMessage->SetBodyL(aMessageText, iSettingUsername);
		aMessage->SetLocationL(*iBroadLocationText);

		iMessagingManager->GetDiscussionL(aToJid)->AddMessageLD(aMessage);
	}
		
	// Geoloc
	if(iOwnItem) {
		CBuddycloudBasicPlace* aPlace = iOwnItem->GetPlace(EPlaceCurrent);
		
		if(aPlace->GetPlaceCity().Length() > 0 || aPlace->GetPlaceCountry().Length() > 0) {
			_LIT8(KGeolocStanza, "<geoloc xmlns='http://jabber.org/protocol/geoloc'><locality></locality><country></country></geoloc>");
			HBufC8* aEncCity = aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceCity()).AllocLC();
			TPtrC8 pEncCity(aEncCity->Des());
			
			HBufC8* aEncCountry = aTextUtilities->UnicodeToUtf8L(aPlace->GetPlaceCountry()).AllocLC();
			TPtrC8 pEncCountry(aEncCountry->Des());
			
			aStanza = aStanza->ReAlloc(pStanza.Length() + KGeolocStanza().Length() + pEncCity.Length() + pEncCountry.Length());
			pStanza.Set(aStanza->Des());
			pStanza.Insert((pStanza.Length() - 12), KGeolocStanza);
			pStanza.Insert((pStanza.Length() - 12 - 39), pEncCity);
			pStanza.Insert((pStanza.Length() - 12 - 19), pEncCountry);
			
			CleanupStack::PopAndDestroy(2); // aEncCountry, aEncCity
		}
	}
	
	// Chatstate
	if(!aIsChannel) {		
		_LIT8(KChatstateElement, "<active xmlns='http://jabber.org/protocol/chatstates'/>");
		aStanza = aStanza->ReAlloc(pStanza.Length() + KChatstateElement().Length());
		pStanza.Set(aStanza->Des());
		pStanza.Insert(pStanza.Length() - 12, KChatstateElement);
	}
	
	// Nick
	if(iSettingFullName.Length() > 0 && (iSettingShowName || !aIsChannel)) {
		_LIT8(KNickElement, "<nick xmlns='http://jabber.org/protocol/nick'></nick>");
		HBufC8* aEncFullName = aTextUtilities->UnicodeToUtf8L(iSettingFullName).AllocLC();
		TPtr8 pEncFullName(aEncFullName->Des());

		aStanza = aStanza->ReAlloc(pStanza.Length() + KNickElement().Length() + pEncFullName.Length());
		pStanza.Set(aStanza->Des());
		pStanza.Insert(pStanza.Length() - 12, KNickElement);
		pStanza.Insert(pStanza.Length() - 12 - 7, pEncFullName);
		
		CleanupStack::PopAndDestroy(); // aEncFullName
	}

	iXmppEngine->SendAndForgetXmppStanza(pStanza, true);
	
	if(aStanza) {
		delete aStanza;
	}
	
	CleanupStack::PopAndDestroy(3); // aEncJid, aEncMessage, aTextUtilities
	
	if(iState == ELogicOffline) {
		HBufC* aMessage = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_MESSAGEQUEUED);
		CAknInformationNote* aDialog = new (ELeave) CAknInformationNote();		
		aDialog->ExecuteLD(*aMessage);
		CleanupStack::PopAndDestroy(); // aMessage
	}
}

void CBuddycloudLogic::SetChatStateL(TMessageChatState aChatState, TDesC& aToJid) {
	if(iState == ELogicOnline) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
		TPtrC8 aEncJid(aTextUtilities->UnicodeToUtf8L(aToJid));

		_LIT8(KMessageStanza, "<message to='' type='chat' id='chatstate'>< xmlns='http://jabber.org/protocol/chatstates'/></message>\r\n");
		HBufC8* aStanza = HBufC8::NewLC(KMessageStanza().Length() + aEncJid.Length() + KChatstateComposing().Length() + KMessageTypeGroupchat().Length());
		TPtr8 pStanza(aStanza->Des());
		pStanza.Append(KMessageStanza);

		// Add chatstate
		if(aChatState == EChatActive) {
			pStanza.Insert(43, KChatstateActive);
		}
		else if(aChatState == EChatComposing) {
			pStanza.Insert(43, KChatstateComposing);
		}
		else if(aChatState == EChatInactive) {
			pStanza.Insert(43, KChatstateInactive);
		}
		else if(aChatState == EChatPaused) {
			pStanza.Insert(43, KChatstatePaused);
		}
		else {
			pStanza.Insert(43, KChatstateGone);
		}				

		pStanza.Insert(13, aEncJid);

		iXmppEngine->SendAndForgetXmppStanza(pStanza, false);
		CleanupStack::PopAndDestroy(2); // aStanza, aTextUtilities
	}
}

TDesC8& CBuddycloudLogic::GenerateNewThreadLC() {
	HBufC8* aDest = HBufC8::NewLC(64);
	TPtr8 pDest(aDest->Des());
	
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();	
	HBufC8* aSrc = HBufC8::NewLC(pDest.MaxLength() / 2);
	TPtr8 pSrc(aSrc->Des());
	
	pSrc.AppendNum(TimeStamp().Int64());
	pSrc.Insert((pSrc.Length() / 2), aTextUtilities->UnicodeToUtf8L(iSettingUsername).Left(pSrc.MaxLength() - pSrc.Length()));
	
	aTextUtilities->Base64Encode(pSrc, pDest);
	
	CleanupStack::PopAndDestroy(2); // aSrc, aTextUtilities
	
	return *aDest;
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
	return iMasterPlaceList;
}

void CBuddycloudLogic::SendEditedPlaceDetailsL(TBool aSearchOnly) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::SendEditedPlaceDetailsL"));
#endif
	
	iPlacesSaveNeeded = true;
	
	CBuddycloudExtendedPlace * aEditingPlace = iMasterPlaceList->GetEditedPlace();
	
	// Send New/Updated place to server
	if(aEditingPlace) {
		CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

		// -----------------------------------------------------------------
		// Get data from edit
		// -----------------------------------------------------------------
		// Name
		HBufC8* aPlaceName = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceName()).AllocLC();
		TPtr8 pPlaceName(aPlaceName->Des());

		// Street
		HBufC8* aPlaceStreet = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceStreet()).AllocLC();
		TPtr8 pPlaceStreet(aPlaceStreet->Des());

		// Area
		HBufC8* aPlaceArea = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceArea()).AllocLC();
		TPtr8 pPlaceArea(aPlaceArea->Des());

		// City
		HBufC8* aPlaceCity = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceCity()).AllocLC();
		TPtr8 pPlaceCity(aPlaceCity->Des());

		// Postcode
		HBufC8* aPlacePostcode = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlacePostcode()).AllocLC();
		TPtr8 pPlacePostcode(aPlacePostcode->Des());

		// Region
		HBufC8* aPlaceRegion = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceRegion()).AllocLC();
		TPtr8 pPlaceRegion(aPlaceRegion->Des());

		// Country
		HBufC8* aPlaceCountry = aTextUtilities->UnicodeToUtf8L(aEditingPlace->GetPlaceCountry()).AllocLC();
		TPtr8 pPlaceCountry(aPlaceCountry->Des());
		
		if(aEditingPlace->GetPlaceId() < 0) {
			if(aEditingPlace->GetPlaceLatitude() == 0.0 && aEditingPlace->GetPlaceLongitude() == 0.0) {
				TReal aLatitude, aLongitude;
				
				iLocationEngine->GetGpsPosition(aLatitude, aLongitude);
				aEditingPlace->SetPlaceLatitude(aLatitude);
				aEditingPlace->SetPlaceLongitude(aLongitude);
			}
		}
		
		// Latitude
		TBuf8<16> aPlaceLatitude;
		aPlaceLatitude.Format(_L8("%.6f"), aEditingPlace->GetPlaceLatitude());

		// Longitude
		TBuf8<16> aPlaceLongitude;
		aPlaceLongitude.Format(_L8("%.6f"), aEditingPlace->GetPlaceLongitude());
		
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
	
		if(aEditingPlace->GetPlaceLatitude() != 0.0 || aEditingPlace->GetPlaceLongitude() != 0.0) {
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
			aSharedElement.Format(_L8("<shared>%d</shared>"), aEditingPlace->GetShared());
			
			if(aEditingPlace->GetPlaceId() < 0) {
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
				iMasterPlaceList->DeletePlaceById(aEditingPlace->GetPlaceId());
				
				// Set Activity Status
				if(iState == ELogicOnline) {
					SetActivityStatus(R_LOCALIZED_STRING_NOTE_UPDATING);
				}
			}
			else {
				TBuf8<64> aIdElement;
				aIdElement.Format(_L8("<id>http://buddycloud.com/places/%d</id>"), aEditingPlace->GetPlaceId());
				
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
				
				iMasterPlaceList->SetEditedPlace(KErrNotFound);
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
			CBuddycloudExtendedPlace* aPlace = CBuddycloudExtendedPlace::NewLC();
			CXmlParser* aPlaceXmlParser = CXmlParser::NewLC(aXmlParser->GetStringData());
			
			do {
				TPtrC8 aElementName = aPlaceXmlParser->GetElementName();
				TPtrC aElementData = aTextUtilities->Utf8ToUnicodeL(aPlaceXmlParser->GetStringData());
				
				if(aElementName.Compare(_L8("id")) == 0) {
					TInt aResult = aElementData.LocateReverse('/');
					
					if(aResult != KErrNotFound) {
						aElementData.Set(aElementData.Mid(aResult + 1));
						
						TLex aLex(aElementData);
						
						if(aLex.Val(aResult) == KErrNone) {
							aPlace->SetPlaceId(aResult);
						}
					}
				}
				else if(aElementName.Compare(_L8("revision")) == 0) {
					aPlace->SetRevision(aPlaceXmlParser->GetIntData());
				}
				else if(aElementName.Compare(_L8("population")) == 0) {
					aPlace->SetPopulation(aPlaceXmlParser->GetIntData());
				}
				else if(aElementName.Compare(_L8("shared")) == 0) {
					aPlace->SetShared(aPlaceXmlParser->GetBoolData());
				}
				else if(aElementName.Compare(_L8("name")) == 0) {
					aPlace->SetPlaceNameL(aElementData);
				}
				else if(aElementName.Compare(_L8("description")) == 0) {
					aPlace->SetDescriptionL(aElementData);
				}
				else if(aElementName.Compare(_L8("lat")) == 0) {
					TLex8 aLatitudeLex(aPlaceXmlParser->GetStringData());
					TReal aLatitude = 0.0;

					if(aLatitudeLex.Val(aLatitude) == KErrNone) {
						aPlace->SetPlaceLatitude(aLatitude);
					}
				}
				else if(aElementName.Compare(_L8("lon")) == 0) {
					TLex8 aLongitudeLex(aPlaceXmlParser->GetStringData());
					TReal aLongitude = 0.0;

					if(aLongitudeLex.Val(aLongitude) == KErrNone) {
						aPlace->SetPlaceLongitude(aLongitude);
					}
				}
				else if(aElementName.Compare(_L8("street")) == 0) {
					aPlace->SetPlaceStreetL(aElementData);
				}
				else if(aElementName.Compare(_L8("area")) == 0) {
					aPlace->SetPlaceAreaL(aElementData);
				}
				else if(aElementName.Compare(_L8("city")) == 0) {
					aPlace->SetPlaceCityL(aElementData);
				}
				else if(aElementName.Compare(_L8("region")) == 0) {
					aPlace->SetPlaceRegionL(aElementData);
				}
				else if(aElementName.Compare(_L8("country")) == 0) {
					aPlace->SetPlaceCountryL(aElementData);
				}
				else if(aElementName.Compare(_L8("postalcode")) == 0) {
					aPlace->SetPlacePostcodeL(aElementData);
				}
				else if(aElementName.Compare(_L8("site")) == 0) {
					aPlace->SetWebsiteL(aElementData);
				}
				else if(aElementName.Compare(_L8("wiki")) == 0) {
					aPlace->SetWikiL(aElementData);
				}	
			} while(aPlaceXmlParser->MoveToNextElement());
			
			CleanupStack::PopAndDestroy(); // aPlaceXmlParser
			
			aPlaceStore->AddPlace(aPlace);
			CleanupStack::Pop(); // aPlace			
		}
		
		if(aId.Compare(_L8("myplaces1")) == 0) {
			ProcessMyPlacesL(aPlaceStore);
		}
		else if(aId.Compare(_L8("searchplacequery1")) == 0) {
			ProcessPlaceSearchL(aPlaceStore);
		}
		else if(aPlaceStore->CountPlaces() > 0) {
			CBuddycloudExtendedPlace* aResultPlace = aPlaceStore->GetPlaceByIndex(0);
						
			if(aResultPlace->GetPlaceId() > 0) {				
				CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceById(aResultPlace->GetPlaceId());

				if(aAttributeType.Compare(_L8("error")) != 0) {
					// Valid result returned, process					
					if(aPlace == NULL) {
						// Add to place to 'my place' list
						iMasterPlaceList->AddPlace(aResultPlace);
						aPlaceStore->RemovePlaceByIndex(0);
					}
					else if(aId.Compare(_L8("placedetails1")) == 0) {
						// New place details received, copy to existing place
						aPlace->CopyDetailsL(aResultPlace);
						
						// Extra data
						aPlace->SetShared(aResultPlace->GetShared());
						aPlace->SetRevision(aResultPlace->GetRevision());
						
						// Extended data
						aPlace->SetWikiL(aResultPlace->GetWiki());
						aPlace->SetWebsiteL(aResultPlace->GetWebsite());
						aPlace->SetDescriptionL(aResultPlace->GetDescription());
						aPlace->SetPopulation(aResultPlace->GetPopulation());
					}
					
					// Current place set by placeid
					if(aId.Compare(_L8("currentplacequery1")) == 0) {
						HandleLocationServerResult(EMotionStationary, 100, aResultPlace->GetPlaceId());
						SetActivityStatus(_L(""));
					}
					else if(aId.Compare(_L8("createplacequery1")) == 0) {
						SetCurrentPlaceL(aResultPlace->GetPlaceId());
					}
				
					// Bubble place to top if it's our current location
					if(aResultPlace->GetPlaceId() == iStablePlaceId) {
						iMasterPlaceList->MovePlaceById(iStablePlaceId, 0);
					}
					
					NotifyNotificationObservers(ENotificationPlaceItemsUpdated, aResultPlace->GetPlaceId());
				}
				else {
					// Result returned error condition
					if(aId.Compare(_L8("placedetails1")) == 0 && aXmlParser->MoveToElement(_L8("not-authorized"))) {
						// User is no longer authorized to see this place, remove
						if(aPlace) {
							aPlace->SetShared(true);
							EditMyPlacesL(aResultPlace->GetPlaceId(), false);
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
	
	CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceById(aPlaceId);

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
	
	CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceById(aPlaceId);

	if(aAddPlace && aPlace == NULL) {
		// Add place to 'my places'
		SendPlaceQueryL(aPlaceId, KPlaceAdd, true);
	}
	else if(!aAddPlace && aPlace) {
		if(aPlace->GetShared()) {
			// Remove place from 'my places'
			SendPlaceQueryL(aPlaceId, KPlaceRemove);
		}
		else {
			// Delete private place
			SendPlaceQueryL(aPlaceId, KPlaceDelete);
		}
		
		// Remove place
		iMasterPlaceList->DeletePlaceById(aPlaceId);
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
	
	for(TInt i = (iMasterPlaceList->CountPlaces() - 1); i >= 0; i--) {
		CBuddycloudExtendedPlace* aLocalPlace = iMasterPlaceList->GetPlaceByIndex(i);
		CBuddycloudExtendedPlace* aStorePlace = aPlaceStore->GetPlaceById(aLocalPlace->GetPlaceId());
		
		if(aLocalPlace->GetPlaceId() != 0) {
			if(aStorePlace) {
				// Place in 'my places'
				aLocalPlace->SetPopulation(aStorePlace->GetPopulation());
				
				if(aLocalPlace->GetRevision() != KErrNotFound && 
						aLocalPlace->GetRevision() < aStorePlace->GetRevision()) {
					
					// Revision out of date, recollect
					GetPlaceDetailsL(aLocalPlace->GetPlaceId());
				}
				
				aPlaceStore->DeletePlaceById(aStorePlace->GetPlaceId());
			}
			else {
				// Place removed from 'my places'
				iMasterPlaceList->DeletePlaceByIndex(i);
			}
		}
	}
	
	while(aPlaceStore->CountPlaces() > 0) {
		// Place added to 'my places'
		CBuddycloudExtendedPlace* aPlace = aPlaceStore->GetPlaceByIndex(0);
		aPlace->SetRevision(KErrNotFound);
		
		iMasterPlaceList->AddPlace(aPlace);
		aPlaceStore->RemovePlaceByIndex(0);
	}
	
	iPlacesSaveNeeded = true;

	NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
}

void CBuddycloudLogic::ProcessPlaceSearchL(CBuddycloudPlaceStore* aPlaceStore) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessPlaceSearchL"));
#endif

	SetActivityStatus(_L(""));
	
	CBuddycloudExtendedPlace* aEditingPlace = iMasterPlaceList->GetEditedPlace();
	
	if(aEditingPlace) {	
		if(aPlaceStore->CountPlaces() > 0) {
			// Multiple results
			for(TInt i = 0; i < aPlaceStore->CountPlaces(); i++) {
				CBuddycloudExtendedPlace* aResultPlace = aPlaceStore->GetPlaceByIndex(i);
	
				if(aResultPlace) {
					_LIT(KResultText, "Result %d of %d:\n\n");
					HBufC* aMessage = HBufC::NewLC(KResultText().Length() + aResultPlace->GetPlaceName().Length() + aResultPlace->GetPlaceStreet().Length() + aResultPlace->GetPlaceCity().Length() + aResultPlace->GetPlacePostcode().Length() + 5);
					TPtr pMessage(aMessage->Des());
					pMessage.AppendFormat(KResultText, (i + 1), aPlaceStore->CountPlaces());
					pMessage.Append(aResultPlace->GetPlaceName());
	
					if(pMessage.Length() > 0 && aResultPlace->GetPlaceStreet().Length() > 0) {
						pMessage.Append(_L(",\n"));
					}
	
					pMessage.Append(aResultPlace->GetPlaceStreet());
	
					if(pMessage.Length() > 0 && (aResultPlace->GetPlaceCity().Length() > 0 || aResultPlace->GetPlacePostcode().Length() > 0)) {
						pMessage.Append(_L(",\n"));						
					}
	
					pMessage.Append(aResultPlace->GetPlaceCity());
					
					if(pMessage.Length() > 0 && aResultPlace->GetPlaceCity().Length() > 0) {
						pMessage.Append(_L(" "));
					}
					
					pMessage.Append(aResultPlace->GetPlacePostcode());
	
					CSearchResultMessageQueryDialog* iDidYouMeanDialog = new (ELeave) CSearchResultMessageQueryDialog();
					iDidYouMeanDialog->PrepareLC(R_DIDYOUMEAN_DIALOG);
					iDidYouMeanDialog->HasPreviousResult((i > 0));
					iDidYouMeanDialog->HasNextResult((i < aPlaceStore->CountPlaces() - 1));					
					iDidYouMeanDialog->SetMessageTextL(pMessage);
					
					TInt aDialogResult = iDidYouMeanDialog->RunLD();
					CleanupStack::PopAndDestroy(); // aMessage										
				
					if(aDialogResult == 0) {
						// Cancel
						return;
					}
					else if(aDialogResult == EMenuSelectCommand) {
						// Select place						
						aEditingPlace->CopyDetailsL(aResultPlace);
						
						// Extended data
						aEditingPlace->SetWikiL(aResultPlace->GetWiki());
						aEditingPlace->SetWebsiteL(aResultPlace->GetWebsite());
						aEditingPlace->SetDescriptionL(aResultPlace->GetDescription());
						
						if(aEditingPlace->GetPlaceId() < 0 && aResultPlace->GetPlaceId() > 0) {
							// Existing place (has id) selected
							iMasterPlaceList->DeletePlaceById(aEditingPlace->GetPlaceId());
							
							// Set place as current
							SetCurrentPlaceL(aResultPlace->GetPlaceId());
							
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
	if(iTimerState & ETimeoutStartConnection) {
		// Startup connection
		ConnectL();
	}
	
	if(iTimerState & ETimeoutConnected) {
		// Clear online activity
		SetActivityStatus(_L(""));
		
		iTimerState -= ETimeoutConnected;
		iTimerState |= ETimeoutSaveSettings;
	}
	
	if(iTimerState & ETimeoutSendPresence) {
		// Send next presence
		TInt aSentCount = 0;
		
		// Get last update time
		TFormattedTimeDesc aFormattedTime;
		CTimeCoder::EncodeL(iHistoryFrom, aFormattedTime);
		
		_LIT8(KSinceElement, "<history since=''/>");
		HBufC8* aHistoryElement = HBufC8::NewLC(KSinceElement().Length() + aFormattedTime.Length());
		TPtr8 pHistoryElement(aHistoryElement->Des());
		pHistoryElement.Copy(KSinceElement);
		pHistoryElement.Insert(16, aFormattedTime);		
		
		for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
			CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

			if(aItem->GetItemType() >= EItemRoster) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
				
				if(aChannelItem->GetPresence() == EPresenceOffline && aChannelItem->GetJid().Length() > 0) {
					aChannelItem->SetPresence(EPresenceSent);
					
					aSentCount++;
					
					SendChannelPresenceL(aChannelItem->GetJid(), pHistoryElement);
					
					if(aSentCount == 5) {
						break;
					}
				}
			}
		}
		
		CleanupStack::PopAndDestroy(); // aHistoryElement
		
		if(aSentCount == 0) {
			iTimerState -= ETimeoutSendPresence;
			
			iXmppEngine->SendQueuedXmppStanzas();
		}
		else {
			iTimer->After(1000000);
		}
	}
	
	if(iTimerState == ETimeoutSaveSettings) {
		// Save settings
		if(iSettingsSaveNeeded) {
			SaveSettingsAndItemsL();			
		}
		
		iSettingsSaveNeeded = false;	

		iTimerState = ETimeoutSavePlaces;
		iTimer->After(300000000);
	}
	else if(iTimerState == ETimeoutSavePlaces) {
		// Save places
		if(iPlacesSaveNeeded) {
			SavePlaceItemsL();			
		}
		
		iPlacesSaveNeeded = false;
		iTimerState = ETimeoutSaveSettings;
		iTimer->After(300000000);
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

void CBuddycloudLogic::HandleXmppLocationStanza(const TDesC8& aStanza, MXmppStanzaObserver* aObserver) {
	iXmppEngine->SendAndAcknowledgeXmppStanza(aStanza, aObserver, false, EXmppPriorityHigh);
}

void CBuddycloudLogic::HandleLocationServerResult(TLocationMotionState aMotionState, TInt /*aPatternQuality*/, TInt aPlaceId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleLocationServerResult"));
#endif
	
	TBool aNearbyRefreshNeeded = false;

	// Get places
	CBuddycloudExtendedPlace* aLastPlace = iMasterPlaceList->GetPlaceById(iStablePlaceId);
	CBuddycloudExtendedPlace* aCurrentPlace = iMasterPlaceList->GetPlaceById(aPlaceId);
	
	// Check motion state
	if(aMotionState < iLastMotionState) {
		aNearbyRefreshNeeded = true;
	}
	
	iLastMotionState = aMotionState;

	if(aPlaceId > 0) {
		if(aCurrentPlace) {
			// Bubble place to top
			iMasterPlaceList->MovePlaceById(aPlaceId, 0);
		}
		else {
			// Collect unlisted place
			GetPlaceDetailsL(aPlaceId);
		}
		
		// Near or at a place
		if(aPlaceId != iStablePlaceId) {
			// Remove top placeholder
			if(iStablePlaceId <= 0) {
				iMasterPlaceList->DeletePlaceById(iStablePlaceId);
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
			
			iMasterPlaceList->AddPlace(aCurrentPlace);
			iMasterPlaceList->MovePlaceById(aPlaceId, 0);
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
		TPtrC8 aReferenceJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetJid());
		
		_LIT8(KNearbyStanza, "<iq to='butler.buddycloud.com' type='get' id='nearbyplaces1'><query xmlns='urn:oslo:nearbyobjects'><reference type='person' id=''/><options limit='10'/><request var='place'/></query></iq>\r\n");
		HBufC8* aNearbyStanza = HBufC8::NewLC(KNearbyStanza().Length() + aReferenceJid.Length());
		TPtr8 pNearbyStanza(aNearbyStanza->Des());
		pNearbyStanza.Append(KNearbyStanza);
		pNearbyStanza.Insert(128, aReferenceJid);
		
		iXmppEngine->SendAndAcknowledgeXmppStanza(pNearbyStanza, this);	
		CleanupStack::PopAndDestroy(2); // aNearbyStanza, aTextUtilities
	}

	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}

	NotifyNotificationObservers(ENotificationLocationUpdated, aPlaceId);
}

TTime CBuddycloudLogic::GetObserverTime() {
	return TimeStamp();
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
		iHistoryFrom = iLastReceivedAt;
		iXmppEngine->GetConnectionMode(iSettingConnectionMode, iSettingConnectionModeId);
		iLastMotionState = EMotionUnknown;
		
		// Sent presence
		SendPresenceL();

		// Set activity status
		iTimerState = ETimeoutConnected;
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_ONLINE);

		if(iConnectionCold) {		
			// Collect my channels & places
			CollectMyChannelsL();
			CollectMyPlacesL();
			
			iConnectionCold = false;
		}
		else {
			iTimerState |= ETimeoutSendPresence;
		}
		
		// Set timer
		iTimer->After(3000000);
		
		NotifyNotificationObservers(ENotificationConnectivityChanged);
	}
	else if(aState == EXmppConnecting || aState == EXmppReconnect) {
		// XMPP state changes to (re)connecting
		if(iTimerState & ETimeoutSendPresence) {
			iTimerState -= ETimeoutSendPresence;
		}
		
		SetActivityStatus(R_LOCALIZED_STRING_NOTE_CONNECTING);
		
		if(aState == EXmppReconnect) {
			// Set offline
			iOffsetReceived = false;
			
			// Reset channels (topic & personal)
			for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
				CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

				if(aItem->GetItemType() >= EItemRoster) {
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
					
					aChannelItem->SetPresence(EPresenceOffline);
				}
			}
			
			// XMPP state changes to a Reconnection state
			iState = ELogicConnecting;
			iOwnerObserver->StateChanged(iState);
			NotifyNotificationObservers(ENotificationConnectivityChanged);
		}
	}
	else if(aState == EXmppDisconnected) {
		// XMPP state changes to an Offline state
		iTimer->Stop();
		iTimerState = ETimeoutNone;
		iOffsetReceived = false;
		
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
			for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
				CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

				if(aItem->GetItemType() >= EItemRoster) {
					CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
					
					// Reset channels (topic & personal)
					aChannelItem->SetPresence(EPresenceOffline);

					if(aChannelItem->GetJid().Length() > 0) {
						CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelItem->GetJid());
						
						// Remove all participants from discussion
						aDiscussion->GetParticipants()->RemoveAllParticipants();
					}
					
					// Reset roster
					if(aItem->GetItemType() == EItemRoster) {
						CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);	
						CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid());

						// Reset presence & pubsub
						aRosterItem->SetPresence(EPresenceOffline);
						aRosterItem->SetPubsubCollected(false);
						
						// Remove all participants from discussion
						aDiscussion->GetParticipants()->RemoveAllParticipants();
					}
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
	
	TPtrC8 pAttributeFrom = aXmlParser->GetStringAttribute(_L8("from"));
	TPtrC8 pAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	TPtrC8 pAttributeId = aXmlParser->GetStringAttribute(_L8("id"));

	if(aXmlParser->MoveToElement(_L8("iq"))) {
		if(aXmlParser->MoveToElement(_L8("pubsub"))) {
			if(pAttributeType.Compare(_L8("result")) == 0) {
				// Pubsub query result
				RosterPubsubEventL(aTextUtilities->Utf8ToUnicodeL(pAttributeFrom), aXmlParser->GetStringData(), false);
			}
			else if(pAttributeType.Compare(_L8("error")) == 0) {
				// Pubsub query error
				if(aXmlParser->MoveToElement(_L8("items"))) {
					RosterPubsubDeleteL(aTextUtilities->Utf8ToUnicodeL(pAttributeFrom), aXmlParser->GetStringAttribute(_L8("node")));
				}
			}
		}
		else if(aXmlParser->MoveToElement(_L8("query"))) {
			if(pAttributeType.Compare(_L8("result")) == 0) {			
				if(aXmlParser->MoveToElement(_L8("utc"))) {
					if( !iOffsetReceived ) {
						iOffsetReceived = true;
						
						// XEP-0090: Server Time
						CTimeCoder* aTimeCoder = CTimeCoder::NewLC();
		
						// Server time
						TTime aServerTime = aTimeCoder->DecodeL(aXmlParser->GetStringData());
		
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
		
						CleanupStack::PopAndDestroy(); // aTimeCoder
					}
				}
			}
			else if(pAttributeType.Compare(_L8("set")) == 0) {
				TPtrC8 aAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));

				if(aAttributeXmlns.Compare(_L8("jabber:iq:roster")) == 0) {
					// Roster Push (Update)
					RosterItemsL(aXmlParser->GetStringData(), true);
				}			
			}
		}
		else if(aXmlParser->MoveToElement(_L8("place"))) {
			// Population push
			if(pAttributeType.Compare(_L8("set")) == 0) {			
				if(aXmlParser->MoveToElement(_L8("id"))) {
					TInt aPlaceId = aXmlParser->GetIntData();
					
					if(aPlaceId > 0 && aXmlParser->MoveToElement(_L8("population"))) {
						CBuddycloudExtendedPlace* aPlace = iMasterPlaceList->GetPlaceById(aPlaceId);
	
						if(aPlace != NULL) {
							aPlace->SetPopulation(aXmlParser->GetIntData());
	
							NotifyNotificationObservers(ENotificationPlaceItemsUpdated);
						}
					}
				}
			}
		}
	}
	else if(aXmlParser->MoveToElement(_L8("message"))) {
		if(pAttributeType.Compare(_L8("error")) != 0) {
			HandleIncomingMessageL(aTextUtilities->Utf8ToUnicodeL(pAttributeFrom), aStanza);
		}
	}
	else if(aXmlParser->MoveToElement(_L8("presence"))) {
		HandleIncomingPresenceL(aTextUtilities->Utf8ToUnicodeL(pAttributeFrom), aStanza);
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
		
		HBufC8* aEncPhoneNumber = aTextUtilities->UnicodeToUtf8L(iSettingPhoneNumber).AllocLC();
		TPtrC8 pEncPhoneNumber(aEncPhoneNumber->Des());
		
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
		_LIT8(KPhoneElement, "<mobilenum></mobilenum>");
		
		_LIT8(KRegisterStanza, "<iq type='set' id='registration1'><query xmlns='jabber:iq:register'><username></username><password></password></query></iq>\r\n");
		HBufC8* aRegisterStanza = HBufC8::NewLC(KRegisterStanza().Length() + pEncUsername.Length() + pEncPassword.Length() + KFirstNameElement().Length() + pEncFirstName.Length() + KLastNameElement().Length() + pEncLastName.Length() + KEmailElement().Length() + pEncEmailAddress.Length() + KPhoneElement().Length() + pEncPhoneNumber.Length());
		TPtr8 pRegisterStanza(aRegisterStanza->Des());
		pRegisterStanza.Copy(KRegisterStanza);
		
		// TODO: WA: This is a mod_register fix
		// First name
//		if(pEncFirstName.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KFirstNameElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 12, pEncFirstName);
//		}
		
		// Last name
//		if(pEncLastName.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KLastNameElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 11, pEncLastName);
//		}
		
		// Email
//		if(pEncEmailAddress.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KEmailElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 8, pEncEmailAddress);
//		}
		
		// Phone
//		if(pEncPhoneNumber.Length() > 0) {
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15, KPhoneElement);
			pRegisterStanza.Insert(pRegisterStanza.Length() - 15 - 12, pEncPhoneNumber);
//		}
		
		pRegisterStanza.Insert(99, pEncPassword);
		pRegisterStanza.Insert(78, pEncUsername);

		iXmppEngine->SendAndAcknowledgeXmppStanza(pRegisterStanza, this, false, EXmppPriorityHigh);
		CleanupStack::PopAndDestroy(7); // aRegisterStanza, aEncFullName, aEncPhoneNumber, aEncEmailAddress, aEncPassword, aEncUsername, aTextUtilities
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

void CBuddycloudLogic::RosterItemsL(const TDesC8& aItems, TBool aUpdate) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterItemsL"));
#endif

	// Initialize Parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aItems);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	while(aXmlParser->MoveToElement(_L8("item"))) {
		TBool aItemFound = false;

		// Collect attributes
		TPtrC aEncJid(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));

		TPtrC8 pAttributeSubscription = aXmlParser->GetStringAttribute(_L8("subscription"));
		TPtrC8 pAttributeAsk = aXmlParser->GetStringAttribute(_L8("ask"));
		TPtrC8 pAttributeName = aXmlParser->GetStringAttribute(_L8("name"));
		
		// Determine subscription type
		TSubscriptionType aSubscription;

		if(pAttributeSubscription.Compare(KRosterSubscriptionBoth) == 0) {
			aSubscription = ESubscriptionBoth;
		}
		else if(pAttributeSubscription.Compare(KRosterSubscriptionFrom) == 0) {
			aSubscription = ESubscriptionFrom;
		}
		else if(pAttributeSubscription.Compare(KRosterSubscriptionTo) == 0) {
			aSubscription = ESubscriptionTo;
		}
		else if(pAttributeSubscription.Compare(KRosterSubscriptionRemove) == 0) {
			aSubscription = ESubscriptionRemove;
		}
		else {
			aSubscription = ESubscriptionNone;
		}		
			
		if(pAttributeAsk.Length() == 0) {
			// Find existing Item
			for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
				CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);
	
				if(aItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
	
					if(aRosterItem->GetJid().CompareF(aEncJid) == 0) {
						if(aSubscription == ESubscriptionRemove || aSubscription == ESubscriptionNone) {							
							// Delete Item if no subscription or request exists
							DeleteItemL(aRosterItem->GetItemId());
						}
						else {
							// Mark item for pubsub recollection
							if(aRosterItem->GetSubscriptionType() < aSubscription) {
								aRosterItem->SetPubsubCollected(false);
							}
							
							aRosterItem->SetSubscriptionType(aSubscription);
												
							// Check if roster update shows user now following added user
							if(aUpdate && (aSubscription == ESubscriptionTo || aSubscription == ESubscriptionBoth)) {
								// Follow users personal channel
								FollowPersonalChannelL(aRosterItem->GetItemId());
							}
						}
	
						aItemFound = true;
						break;
					}
				}
			}
	
			if(!aItemFound && aSubscription > ESubscriptionNone) {
				// Add Item
				CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
				aRosterItem->SetItemId(iNextItemId++);
				aRosterItem->SetJidL(aEncJid);
				aRosterItem->SetSubscriptionType(aSubscription);
	
				if(pAttributeName.Length() > 0) {
					aRosterItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(pAttributeName));
				}
	
				CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aRosterItem->GetJid());
				aDiscussion->AddMessageReadObserver(aRosterItem);
				aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
	
				iMasterFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);
				CleanupStack::Pop();
				
				if(aUpdate) {
					if(aSubscription == ESubscriptionTo || aSubscription == ESubscriptionBoth) {
						// Follow users personal channel
						FollowPersonalChannelL(aRosterItem->GetItemId());
					}
					
					if(aSubscription == ESubscriptionFrom) {
						// Subscription requested			
						// Update personal channel affiliation
						ChangeUsersChannelAffiliationL(aEncJid, iOwnItem->GetJid(EJidChannel), EAffiliationMember);		
					}
				}
				
				iSettingsSaveNeeded = true;
			}
		}
		else if(aUpdate && aSubscription == ESubscriptionNone) {
			// Ask subscription to user from outside resource
			// Update personal channel affiliation
			ChangeUsersChannelAffiliationL(aEncJid, iOwnItem->GetJid(EJidChannel), EAffiliationMember);		
		}
	}
	
	iRosterSynchronized = true;

	CleanupStack::PopAndDestroy(2); // aTextUtilities, aXmlParser

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
}

void CBuddycloudLogic::RosterOwnJidL(TDesC& aJid) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterOwnJidL"));
#endif

	TBool aItemFound = false;

	if(iOwnItem) {
		iOwnItem->SetIsOwn(false);
	}
	
	// Remove resource from jid
	TPtrC pJid(aJid);

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
	}

	for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetJid().Compare(pJid) == 0) {
				aRosterItem->SetIsOwn(true);
				aRosterItem->SetSubscriptionType(ESubscriptionBoth);

				iOwnItem = aRosterItem;

				aItemFound = true;
				break;
			}
		}
	}

	if( !aItemFound ) {
		CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
		aRosterItem->SetIsOwn(true);
		aRosterItem->SetSubscriptionType(ESubscriptionBoth);
		aRosterItem->SetItemId(iNextItemId++);
		aRosterItem->SetJidL(pJid);
		
		CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pJid);

		// Setting the Contact Name
		if(iSettingFullName.Length() > 0) {
			aRosterItem->SetTitleL(iSettingFullName);
		}

		aDiscussion->AddMessageReadObserver(aRosterItem);
		aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
		
		iOwnItem = aRosterItem;
		iMasterFollowingList->InsertItem(0, aRosterItem);
		CleanupStack::Pop();

		if(iStatusObserver) {
			iStatusObserver->JumpToItem(aRosterItem->GetItemId());
		}
	}
}

void CBuddycloudLogic::RosterPubsubEventL(TDesC& aFrom, const TDesC8& aData, TBool aNew) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterPubsubEventL"));
#endif
	
	TInt aNotifyId = KErrNotFound;

	// Remove resource from jid
	TPtrC pJid(aFrom);

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
	}

	// Initialize xml parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aData);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();

	for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetJid().CompareF(pJid) == 0) {
				aNotifyId = aItem->GetItemId();				

				if(aXmlParser->MoveToElement(_L8("items"))) {
					TPtrC8 pAttributeNode = aXmlParser->GetStringAttribute(_L8("node"));
					
					if(aXmlParser->MoveToElement(_L8("geoloc"))) {
						// XEP-0080: User Geoloc
						CBuddycloudBasicPlace* aNewPlace = CBuddycloudBasicPlace::NewLC();
						TBuddycloudPlaceType aPlaceType = EPlaceCurrent;
						
						if(pAttributeNode.Compare(_L8("http://jabber.org/protocol/geoloc-prev")) == 0) {
							aPlaceType = EPlacePrevious;
							aNew = false;
						}
						else if(pAttributeNode.Compare(_L8("http://jabber.org/protocol/geoloc-next")) == 0) {
							aPlaceType = EPlaceNext;
						}
						
						CBuddycloudBasicPlace* aOldPlace = aRosterItem->GetPlace(aPlaceType);
						
						do {
							TPtrC8 pElementName = aXmlParser->GetElementName();
							TPtrC8 pElementData = aXmlParser->GetStringData();
							
							if(pElementName.Compare(_L8("text")) == 0) {
								aNewPlace->SetPlaceNameL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("street")) == 0) {
								aNewPlace->SetPlaceStreetL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("area")) == 0) {
								aNewPlace->SetPlaceAreaL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("locality")) == 0) {
								aNewPlace->SetPlaceCityL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("postalcode")) == 0) {
								aNewPlace->SetPlacePostcodeL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("region")) == 0) {
								aNewPlace->SetPlaceRegionL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("country")) == 0) {
								aNewPlace->SetPlaceCountryL(aTextUtilities->Utf8ToUnicodeL(pElementData));
							}
							else if(pElementName.Compare(_L8("lat")) == 0) {
								TLex8 aLatitudeLex(pElementData);
								TReal aLatitude = 0.0;

								if(aLatitudeLex.Val(aLatitude) == KErrNone) {
									aNewPlace->SetPlaceLatitude(aLatitude);
								}
							}
							else if(pElementName.Compare(_L8("lon")) == 0) {
								TLex8 aLongitudeLex(pElementData);
								TReal aLongitude = 0.0;

								if(aLongitudeLex.Val(aLongitude) == KErrNone) {
									aNewPlace->SetPlaceLongitude(aLongitude);
								}
							}
						} while(aXmlParser->MoveToNextElement());
						
						if(aNew && aOldPlace->GetPlaceName().CompareF(aNewPlace->GetPlaceName()) == 0) {
							aNew = false;
						}
						
						if(aRosterItem->GetIsOwn() && aPlaceType == EPlaceCurrent) {
							// Broad location
							HBufC* aNewBroadLocation = HBufC::NewLC(aNewPlace->GetPlaceCity().Length() + 2 + aNewPlace->GetPlaceCountry().Length());
							TPtr pNewBroadLocation(aNewBroadLocation->Des());
							
							pNewBroadLocation.Append(aNewPlace->GetPlaceCity());
							
							if(pNewBroadLocation.Length() > 0 && aNewPlace->GetPlaceCountry().Length() > 0) {
								pNewBroadLocation.Append(_L(", "));
							}
							
							pNewBroadLocation.Append(aNewPlace->GetPlaceCountry());
							
							// Compair with old broad location
							if(pNewBroadLocation.Compare(*iBroadLocationText) != 0) {
								// Update
								if(iBroadLocationText)
									delete iBroadLocationText;
								
								iBroadLocationText = aNewBroadLocation;
								CleanupStack::Pop(); // aEncBroadLocation
								
								// Send presence
								SendPresenceL();
							}
							else {
								CleanupStack::PopAndDestroy(); // aEncBroadLocation
							}
							
							// Reset next
							if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().FindF(aNewPlace->GetPlaceName()) != KErrNotFound) {
								iXmppEngine->SendAndForgetXmppStanza(_L8("<iq to='butler.buddycloud.com' type='set' id='next2'><query xmlns='http://buddycloud.com/protocol/place#next'><place><name></name></place></query></iq>\r\n"), true);
							}
						}
						
						aRosterItem->SetPlaceL(aPlaceType, aNewPlace);						
						CleanupStack::Pop(); // aNewPlace
					}
					else if(aXmlParser->MoveToElement(_L8("mood"))) {
						// XEP-0107: User mood
						TPtrC aEncMood;
						
						if(aXmlParser->MoveToElement(_L8("text"))) {
							aEncMood.Set(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));						
						}
						
						if(aNew && aRosterItem->GetDescription().CompareF(aEncMood) == 0) {
							aNew = false;
						}
						
						aRosterItem->SetDescriptionL(aEncMood);
					}
					else if(aXmlParser->MoveToElement(_L8("profile"))) {
						// XEP-0154: User profile
						aNew = false;

						if(aXmlParser->MoveToElement(_L8("x"))) {
							while(aXmlParser->MoveToElement(_L8("field"))) {
								TPtrC8 pAttributeVar = aXmlParser->GetStringAttribute(_L8("var"));

								if(pAttributeVar.Compare(_L8("full_name")) == 0) {
									if(aXmlParser->MoveToElement(_L8("value"))) {
										TPtrC aEncFullName(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
										TPtrC aUsername(aRosterItem->GetJid());
										TInt aLocateResult = aUsername.Locate('@');
										
										// Remove '@domain.com'
										if(aLocateResult != KErrNotFound) {
											aUsername.Set(aUsername.Left(aLocateResult));
										}
																				
										if(aEncFullName.Length() == 0) {
											// No full_name set
											aRosterItem->SetTitleL(aUsername);
										}
										else {
											HBufC* aFormattedName = HBufC::NewLC(aEncFullName.Length() + aUsername.Length() + 3);
											TPtr pFormattedName(aFormattedName->Des());
											pFormattedName.Append(aEncFullName);
											pFormattedName.Trim();
											
											if(aUsername.Compare(pFormattedName) != 0) {
												pFormattedName.Append(_L(" ()"));
												pFormattedName.Insert(pFormattedName.Length() - 1, aUsername);
											}
											
											aRosterItem->SetTitleL(pFormattedName);
											CleanupStack::PopAndDestroy(); // aFormattedName
										}
									}
								}
								else if(pAttributeVar.Compare(_L8("detail")) == 0) {
									// Check if contact is already paired
									while(aXmlParser->MoveToElement(_L8("value"))) {
										if(aRosterItem->GetAddressbookId() < 0) {
											TPtrC aMobileNumber(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));

											CContactIdArray* aContactIdArray = iContactDatabase->MatchPhoneNumberL(aMobileNumber, 9);
											CleanupStack::PushL(aContactIdArray);

											// If match found in contact database
											if(aContactIdArray->Count() == 1) {
												TContactItemId aContactId = (*aContactIdArray)[0];
												TInt aMatchedContactItemId = KErrNotFound;

												for(TInt x = 0; x < iMasterFollowingList->Count(); x++) {
													CFollowingItem* aMatchingItem = iMasterFollowingList->GetItemByIndex(x);

													if(aMatchingItem->GetItemType() == EItemContact) {
														CFollowingContactItem* aContactItem = static_cast <CFollowingContactItem*> (aMatchingItem);

														// Pair items
														if(aContactItem->GetAddressbookId() == aContactId) {
															aMatchedContactItemId = aContactItem->GetItemId();
															break;
														}
													}
												}

												if(aMatchedContactItemId != KErrNotFound) {
													// Contact not in following list
													PairItemIdsL(aRosterItem->GetItemId(), aMatchedContactItemId);
												}
												else {
													// Pair without item id
													aRosterItem->SetAddressbookId(aContactId);
												}
											}

											CleanupStack::PopAndDestroy(); // aContactIdArray
										}
									}
								}
							}
						}
					}
					else if(aXmlParser->MoveToElement(_L8("metadata"))) {
						// XEP-0084: User Avatar - Metadata
					}
					else if(aXmlParser->MoveToElement(_L8("data"))) {
						// XEP-0084: User Avatar - Data
					}
				}

				if(aNew && !aRosterItem->GetIsOwn()) {
					aRosterItem->SetLastUpdated(TimeStamp());
					
					iMasterFollowingList->RemoveItemByIndex(i);
					iMasterFollowingList->InsertItem(GetBubbleToPosition(aRosterItem), aRosterItem);
				}

				break;
			}
		}
	}

	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities

	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}

	NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aNotifyId);
}

void CBuddycloudLogic::RosterPubsubDeleteL(TDesC& aFrom, const TDesC8& aNode) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterPubsubDeleteL"));
#endif

	TInt aNotifyId = KErrNotFound;
	TBool aNotifyObserver = false;

	// Remove resource from jid
	TPtrC pJid(aFrom);

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
	}

	for(TInt i = 0; i < iMasterFollowingList->Count(); i++) {
		CFollowingItem* aItem = iMasterFollowingList->GetItemByIndex(i);

		if(aItem->GetItemType() == EItemRoster) {
			CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);

			if(aRosterItem->GetJid().CompareF(pJid) == 0) {
				if(aNode.Compare(_L8("http://jabber.org/protocol/mood")) == 0) {
					if(aRosterItem->GetIsOwn() && iSettingNewInstall) {						
						// Initial status
						TBuf<256> aMood;
						CEikonEnv::Static()->ReadResource(aMood, R_LOCALIZED_STRING_DIALOG_MOODIS);
						SetMoodL(aMood);					
					}
					else if(aRosterItem->GetDescription().Length() > 0) {
						aNotifyObserver = true;
						aRosterItem->SetDescriptionL(_L(""));
					}
				}
				else if(aNode.Compare(_L8("http://jabber.org/protocol/geoloc-prev")) == 0) {
					if(aRosterItem->GetPlace(EPlacePrevious)->GetPlaceName().Length() > 0) {
						aNotifyObserver = true;
						aRosterItem->SetPlaceL(EPlacePrevious, CBuddycloudBasicPlace::NewL());
					}
				}
				else if(aNode.Compare(_L8("http://jabber.org/protocol/geoloc")) == 0) {
					if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
						aNotifyObserver = true;
						aRosterItem->SetPlaceL(EPlaceCurrent, CBuddycloudBasicPlace::NewL());
					}				
				}
				else if(aNode.Compare(_L8("http://jabber.org/protocol/geoloc-next")) == 0) {
					if(aRosterItem->GetPlace(EPlaceNext)->GetPlaceName().Length() > 0) {
						aNotifyObserver = true;
						aRosterItem->SetPlaceL(EPlaceNext, CBuddycloudBasicPlace::NewL());
					}
				}
				else if(aNode.Compare(_L8("http://www.xmpp.org/extensions/xep-0154.html#ns")) == 0) {
					if(aRosterItem->GetIsOwn()) {
						// Own profile no longer published
						PublishUserDataL(NULL, true);
					}
				}
				
				if(aNotifyObserver) {
					aNotifyId = aRosterItem->GetItemId();
					aRosterItem->SetLastUpdated(TimeStamp());
				}

				break;
			}
		}
	}

	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}

	if(aNotifyObserver) {
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aNotifyId);
	}
}

void CBuddycloudLogic::RosterSubscriptionL(TDesC& aJid, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::RosterSubscriptionL"));
#endif
	
	CFollowingItem* aItem = iMasterFollowingList->GetItemById(IsSubscribedTo(aJid, EItemNotice));
	
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
		
		CFollowingNoticeItem* aNoticeItem = CFollowingNoticeItem::NewLC();
		aNoticeItem->SetItemId(iNextItemId++);
		aNoticeItem->SetJidL(aJid, EJidRoster);
		aNoticeItem->SetTitleL(*aTitle);
		aNoticeItem->SetDescriptionL(*aDescription);
		
		iMasterFollowingList->InsertItem(GetBubbleToPosition(aNoticeItem), aNoticeItem);
		CleanupStack::Pop(); // aNoticeItem
		
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
		NotifyNotificationObservers(ENotificationMessageNotifiedEvent);
		CleanupStack::PopAndDestroy(8); // aDescription, aDescriptionResource, aTitle, aUserInfo, aEncLocation, aEncName, aXmlParser, aTextUtilities 
	}

	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}
}

void CBuddycloudLogic::XmppStanzaNotificationL(const TDesC8& aStanza, const TDesC8& aId) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::XmppStanzaNotificationL"));
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
		else if(aId.Compare(_L8("mychannelmembers1")) == 0) {
			// Handle personal channel members
			SynchronizePersonalChannelMembersL(aStanza);
		}
		else if(aId.Compare(_L8("mychannels1")) == 0) {
			// Handle my channels list
			ProcessMyChannelsL(aStanza);
		}
		else if(aId.Compare(_L8("myplaces1")) == 0 || aId.Compare(_L8("addplacequery1")) == 0 || 
				aId.Compare(_L8("createplacequery1")) == 0 || aId.Compare(_L8("currentplacequery1")) == 0 || 
				aId.Compare(_L8("searchplacequery1")) == 0 || aId.Compare(_L8("placedetails1")) == 0) {
			
			// Handle places query result
			HandlePlaceQueryResultL(aStanza, aId);
		}
		else if(aId.Compare(_L8("setmood1")) == 0 || aId.Compare(_L8("setnext1")) == 0) {
			// Set mood/next place result
			SetActivityStatus(_L(""));
		}
		else if(aId.Compare(_L8("channelinfo1")) == 0) {
			// Channel lookup success, confirm channel follow
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			TPtrC aChannelJid = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("from")));
			
			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelJid);
			CFollowingItem* aItem = aDiscussion->GetFollowingItem();
			
			if(aItem && aItem->GetItemType() >= EItemRoster) {
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);	
				
				if(aItem->GetItemType() == EItemRoster) {
					// Channel info for a personal channel
					// Send initial presence to channel
					iSettingsSaveNeeded = true;			
					SendChannelPresenceL(aChannelItem->GetJid(),  _L8("<history maxstanzas='30'/>"));
				}
				else if(aItem->GetItemType() == EItemChannel) {
					// Channel info for a topic channel
					if(aXmlParser->MoveToElement(_L8("identity"))) {
						// Get friendly name of channel
						TPtrC8 aChannelName = aXmlParser->GetStringAttribute(_L8("name"));
						
						aChannelItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aChannelName));
					}
					
					CAknMessageQueryDialog* aMessageDialog = new (ELeave) CAknMessageQueryDialog();
					aMessageDialog->PrepareLC(R_FOLLOWING_DIALOG);
					
					HBufC* aMessageResource = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_CHANNELFOLLOWING_TEXT);
					TPtrC pMessageResource(aMessageResource->Des());
					
					HBufC* aMessageBody = HBufC::NewLC(pMessageResource.Length() + aChannelItem->GetTitle().Length());
					TPtr pMessageBody(aMessageBody->Des());
					pMessageBody.Append(pMessageResource);
					
					TInt aFindResult = pMessageBody.Find(_L("$CHANNEL"));			
					if(aFindResult != KErrNotFound) {
						pMessageBody.Replace(aFindResult, 8, aChannelItem->GetTitle());
					}
					
					aMessageDialog->SetMessageText(pMessageBody);
					CleanupStack::PopAndDestroy(2); // aMessageBody, aMessageResource
					
					if(aMessageDialog->RunLD() != 0) {
						// Send initial presence to channel
						SendChannelPresenceL(aChannelItem->GetJid(), _L8("<history maxstanzas='30'/>"));
					}
					else {
						// Delete discussion & channel						
						iMessagingManager->DeleteDiscussionL(aChannelJid);
						iMasterFollowingList->DeleteItemById(aItem->GetItemId());
						
						NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItem->GetItemId());
						
						iSettingsSaveNeeded = true;			
					}
				}
			}
			else {
				// Delete discussion
				iMessagingManager->GetDiscussionL(aChannelJid);
			}
			
			CleanupStack::PopAndDestroy(); // aTextUtilities
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
		else if(aId.Find(_L8("mediareq_")) != KErrNotFound) {
			// Media post request
			if(aXmlParser->MoveToElement(_L8("uploaduri"))) {
				CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
				TPtrC aChannelRoomname = aTextUtilities->Utf8ToUnicodeL(aId.Mid(aId.Locate('_') + 1));
				
				// Format jid
				HBufC* aChannelJid = HBufC::NewLC(aChannelRoomname.Length() + KBuddycloudChannelsServer().Length());
				TPtr pChannelJid(aChannelJid->Des());
				pChannelJid.Append(aChannelRoomname);
				pChannelJid.Append(KBuddycloudChannelsServer);
				
				// Get discussion
				CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pChannelJid);
				CFollowingItem* aItem = aDiscussion->GetFollowingItem();
				
				if(aItem && aItem->GetItemType() >= EItemRoster) {
					// Create response message
					CMessage* aMessage = CMessage::NewLC();								
					aMessage->SetPrivate(true);
					aMessage->SetDirectReply(true);
					aMessage->SetAvatarId(KIconChannel);							
					aMessage->SetReceivedAt(TimeStamp());	
					aMessage->SetNameL(_L("Media Uploader"));

					HBufC* aResourceText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_MEDIAUPLOADREADY);
					TPtrC aDataUploaduri = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData());
					
					HBufC* aMessageText = HBufC::NewLC(2 + iSettingUsername.Length() + aResourceText->Des().Length() + aDataUploaduri.Length());
					TPtr pMessageText(aMessageText->Des());
					pMessageText.Append(_L("@ "));
					pMessageText.Insert(1, iSettingUsername);
					pMessageText.Append(*aResourceText);
					pMessageText.Replace(pMessageText.Find(_L("$LINK")), 5, aDataUploaduri);							
					aMessage->SetBodyL(pMessageText, iSettingUsername);
					CleanupStack::PopAndDestroy(2); // aMessageText, aResourceText

					aDiscussion->AddMessageLD(aMessage);					
					
					NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aItem->GetItemId());
				}
				
				CleanupStack::PopAndDestroy(2); // aChannelJid, aTextUtilities
			}
		}
	}
	else if(aAttributeType.Compare(_L8("error")) == 0) {
		if(aId.Compare(_L8("mychannels1")) == 0) {
			// Error collecting my channels
			// Send presence to channels
			iTimerState |= ETimeoutSendPresence;
			TimerExpired(0);
			
			if(iOwnItem->GetJid(EJidChannel).Length() == 0) {
				// No personal channel, create
				FollowPersonalChannelL(iOwnItem->GetItemId());
			}
		}		
		else if(aId.Compare(_L8("channelinfo1")) == 0) {
			// Channel item lookup failed, item does not exist
			CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
			TPtrC aChannelJid = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("from")));
			
			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(aChannelJid);
			CFollowingItem* aItem = aDiscussion->GetFollowingItem();
			
			if(aItem && aItem->GetItemType() >= EItemRoster) {						
				CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);			

				if(aChannelItem->GetItemType() == EItemRoster) {
					// Personal channel doesn't exist
					if(aChannelItem->GetIsOwn()) {
						// Send initial presence to channel (creating new channel)
						SendChannelPresenceL(aChannelItem->GetJid(), _L8("<history maxstanzas='30'/>"));
					}
					else {
						// Remove personal channel						
						iMessagingManager->DeleteDiscussionL(aChannelJid);
						aChannelItem->SetJidL(_L(""));
						
						NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItem->GetItemId());
					}
				}
				else if(aChannelItem->GetItemType() == EItemChannel) {
					// Topic channel doesn't exist
					if(aXmlParser->MoveToElement(_L8("item-not-found"))) {					
						// Send initial presence to channel (creating new channel)
						SendChannelPresenceL(aChannelItem->GetJid(), _L8("<history maxstanzas='30'/>"));
					}
					else {
						// Delete discussion & channel
						iMessagingManager->DeleteDiscussionL(aChannelJid);
						iMasterFollowingList->DeleteItemById(aItem->GetItemId());
						
						NotifyNotificationObservers(ENotificationFollowingItemDeleted, aItem->GetItemId());
						
						iSettingsSaveNeeded = true;			
					}
				}
			}
			else {
				// Item does not exist
				iMessagingManager->DeleteDiscussionL(aChannelJid);
			}
			
			CleanupStack::PopAndDestroy(); // aTextUtilities
		}
		else if(aId.Compare(_L8("createplacequery1")) == 0 || aId.Compare(_L8("currentplacequery1")) == 0 || 
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
		else if(aId.Compare(_L8("setmood1")) == 0 || aId.Compare(_L8("setnext1")) == 0) {
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

void CBuddycloudLogic::HandleIncomingPresenceL(TDesC& aFrom, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingPresenceL"));
#endif
	
	TBool aNotifyObserver = false;
//	TBool aBubbleItem = false;

	// Attribute aFrom contains both JID@SERVER/RESOURCE
	// It the case of a Group presence it will be ROOM@SERVER/JID
	TPtrC pJid(aFrom);
	TPtrC pResource(aFrom);

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pJid.Set(pJid.Left(aSearchResult));
		pResource.Set(pResource.Mid(aSearchResult + 1));
	}

	// Initialize xml parser
	CXmlParser* aXmlParser = CXmlParser::NewLC(aData);
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	
	TPtrC8 pAttributeType = aXmlParser->GetStringAttribute(_L8("type"));
	
	if(pAttributeType.Compare(_L8("subscribe")) == 0) {
		RosterSubscriptionL(aFrom, aXmlParser->GetStringData());					
	}
	else {
		CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pJid);
		CFollowingItem* aItem = aDiscussion->GetFollowingItem();
		
		if(aItem && aItem->GetItemType() >= EItemRoster) {
			CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);	
			
			if(pAttributeType.Compare(_L8("unavailable")) == 0) {
				// Remove participant from discussion
				TBool aIsReplaced = false;
				
				if(aXmlParser->MoveToElement(_L8("status"))) {
					TPtrC8 pStatusData = aXmlParser->GetStringData();
					
					if(pStatusData.Compare(_L8("Replaced by new connection")) == 0) {
						aIsReplaced = true;
					}
				}
				
				if( !aIsReplaced ) {						
					aDiscussion->GetParticipants()->RemoveParticipant(aFrom);
					
					if(aItem->GetItemType() == EItemRoster) {	
						CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
						
						aRosterItem->SetPresence(EPresenceOffline);
					}
				}
			}
			else if(pAttributeType.Length() == 0) {
				CMessagingParticipant* aParticipant = aDiscussion->GetParticipants()->AddParticipant(aFrom);
				
				if(pJid.Compare(aChannelItem->GetJid()) == 0) {
					// Is a channel presence
					aChannelItem->SetPresence(EPresenceOnline);
					
					if(aXmlParser->MoveToElement(_L8("item"))) {
						// Item's jid
						aParticipant->SetJidL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringAttribute(_L8("jid"))));
						
						// Item's affiliation
						TPtrC8 pAttributeAffiliation = aXmlParser->GetStringAttribute(_L8("affiliation"));
						TMucAffiliation aChannelAffiliation = EAffiliationOutcast;
						
						if(pAttributeAffiliation.Compare(KAffiliationNone) == 0) {
							aChannelAffiliation = EAffiliationNone;
						}
						else if(pAttributeAffiliation.Compare(KAffiliationMember) == 0) {
							aChannelAffiliation = EAffiliationMember;
						}
						else if(pAttributeAffiliation.Compare(KAffiliationAdmin) == 0) {
							aChannelAffiliation = EAffiliationAdmin;
						}
						else if(pAttributeAffiliation.Compare(KAffiliationOwner) == 0) {
							aChannelAffiliation = EAffiliationOwner;
						}
						
						// Item's role
						TPtrC8 pAttributeRole = aXmlParser->GetStringAttribute(_L8("role"));
						TMucRole aChannelRole = ERoleVisitor;
						
						if(pAttributeRole.Compare(KRoleNone) == 0) {
							aChannelRole = ERoleNone;
						}
						else if(pAttributeRole.Compare(KRoleParticipant) == 0) {
							aChannelRole = ERoleParticipant;
						}
						else if(pAttributeRole.Compare(KRoleModerator) == 0) {
							aChannelRole = ERoleModerator;
						}
						
						aParticipant->SetRole(aChannelRole);
						aParticipant->SetAffiliation(aChannelAffiliation);
						
						// Set users role & affiliation of the group
						if(pResource.Compare(iSettingUsername) == 0) {
							aDiscussion->SetMyRoleAndAffiliation(aChannelRole, aChannelAffiliation);
							
							if(aChannelItem->GetIsOwn()) {
								// Own presence in own personal channel
								// Collect personal channel members
								TPtrC8 aEncOwnChannelJid = aTextUtilities->UnicodeToUtf8L(iOwnItem->GetJid(EJidChannel));
									
								_LIT8(KChannelMembersStanza, "<iq to='' type='get' id='mychannelmembers1'><query xmlns='http://jabber.org/protocol/muc#admin'><item affiliation='member'/></query></iq>\r\n");
								HBufC8* aChannelMembersStanza = HBufC8::NewLC(KChannelMembersStanza().Length() + aEncOwnChannelJid.Length());
								TPtr8 pChannelMembersStanza(aChannelMembersStanza->Des());
								pChannelMembersStanza.Append(KChannelMembersStanza);
								pChannelMembersStanza.Insert(8, aEncOwnChannelJid);

								iXmppEngine->SendAndAcknowledgeXmppStanza(pChannelMembersStanza, this, false);
								CleanupStack::PopAndDestroy(); // aChannelMembersStanza
							}
						}						
					}
				}
				else if(aChannelItem->GetItemType() == EItemRoster) {
					CFollowingRosterItem* aRosterItem = static_cast <CFollowingRosterItem*> (aItem);
					
					if(aRosterItem->GetPresence() != EPresenceOnline) {
						aRosterItem->SetPresence(EPresenceOnline);
						
						aParticipant->SetJidL(aRosterItem->GetJid());
						aParticipant->SetAvatarId(aRosterItem->GetAvatarId());						
						
						// Collect Pubsub nodes
						if(!aRosterItem->GetPubsubCollected()) {
							aRosterItem->SetPubsubCollected(true);

							CollectPubsubNodesL(aRosterItem->GetJid());
						}
					
//						if(aRosterItem->GetPlace(EPlaceCurrent)->GetPlaceName().Length() > 0) {
//							// Only bubble a contact with geoloc set
//							aRosterItem->SetLastUpdated(TimeStamp());
//							aBubbleItem = true;
//						}
					}
				}
			}
			
			aXmlParser->ResetElementPointer();							
				
			while(aXmlParser->MoveToElement(_L8("status"))) {
				TInt aAttributeCode = aXmlParser->GetIntAttribute(_L8("code"));
				
				if(aAttributeCode == 201) {
					if(aItem->GetItemType() == EItemChannel) {
						// Topic channel created
						TBuf<256> aChannelDescription;

						CAknTextQueryDialog* aDialog = CAknTextQueryDialog::NewL(aChannelDescription);
						aDialog->PrepareLC(R_MOOD_DIALOG);
						aDialog->SetPredictiveTextInputPermitted(true);
						
						HBufC* aMessageText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_DIALOG_CREATECHANNELDESCRIPTION);
						aDialog->SetPromptL(*aMessageText);
						CleanupStack::PopAndDestroy(); // aMessageText
						
						if(aDialog->RunLD() != 0) {
							aItem->SetDescriptionL(aChannelDescription);
							
							// Configure new channel
							SendChannelConfigurationL(aItem->GetItemId());
							
							// Set new channel topic
							SetTopicL(aChannelDescription, pJid, true);
						}
						else {						
							// First have to configure before deleting
							SendChannelConfigurationL(aItem->GetItemId());
							
							// Destroy channel
							UnfollowChannelL(aItem->GetItemId());
						}
					}
					else {
						// Accept default configuration
						TPtrC8 aEncChannelJid = aTextUtilities->UnicodeToUtf8L(pJid);
						
						_LIT8(KConfigStanza, "<iq to='' type='set' id='channelconfig1'><query xmlns='http://jabber.org/protocol/muc#owner'><x xmlns='jabber:x:data' type='submit'/></query></iq>\r\n");
						HBufC8* aConfigStanza = HBufC8::NewLC(KConfigStanza().Length() + aEncChannelJid.Length());
						TPtr8 pConfigStanza(aConfigStanza->Des());
						pConfigStanza.Append(KConfigStanza);
						pConfigStanza.Insert(8, aEncChannelJid);
						
						iXmppEngine->SendAndForgetXmppStanza(pConfigStanza, true);
						CleanupStack::PopAndDestroy(); // aConfigStanza		
						
						// Add first message into personal channel
						CMessage* aMessage = CMessage::NewLC();								
						aMessage->SetPrivate(true);
						aMessage->SetAvatarId(KIconChannel);							
						aMessage->SetReceivedAt(TimeStamp());	
						aMessage->SetNameL(_L("Buddycloud Channels"));

						HBufC* aResourceText = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_NOTE_PERSONALCHANNELCREATED);
						HBufC* aMessageText = HBufC::NewLC(aResourceText->Des().Length() + iSettingUsername.Length());
						TPtr pMessageText(aMessageText->Des());
						pMessageText.Append(*aResourceText);
						pMessageText.Replace(pMessageText.Find(_L("$USER")), 5, iSettingUsername);							
						aMessage->SetBodyL(pMessageText, iSettingUsername);
						CleanupStack::PopAndDestroy(2); // aMessageText, aResourceText

						aDiscussion->AddMessageLD(aMessage);
					}
				}
				else if(aAttributeCode == 307 || aAttributeCode == 301) {
					TInt aResourceId = R_LOCALIZED_STRING_NOTE_USERBANNEDFROMGROUP;
					
					if(aAttributeCode == 307) {
						aResourceId = R_LOCALIZED_STRING_NOTE_USERKICKEDFROMGROUP;
					}

					CMessage* aMessage = CMessage::NewLC();
					aMessage->SetAvatarId(KIconChannel);
					aMessage->SetRead(true);
						
					HBufC* aResourceText = CEikonEnv::Static()->AllocReadResourceLC(aResourceId);
					HBufC* aMessageText = HBufC::NewLC(aResourceText->Des().Length() + pResource.Length());
					TPtr pMessageText(aMessageText->Des());
					pMessageText.Append(*aResourceText);
					pMessageText.Replace(pMessageText.Find(_L("$USER")), 5, pResource);
						
					aMessage->SetBodyL(pMessageText, iSettingUsername, EMessageUserAction);
					CleanupStack::PopAndDestroy(2); // aMessageText, aResourceText

					aDiscussion->AddMessageLD(aMessage);
					
					NotifyNotificationObservers(ENotificationMessageSilentEvent, aItem->GetItemId());
				}
			}	
			
//			// Bubble item
//			if(aBubbleItem && !aItem->GetIsOwn()) {
//				aNotifyObserver = true;
//				
//				iMasterFollowingList->RemoveItemByIndex(iMasterFollowingList->GetIndexById(aItem->GetItemId()));
//				iMasterFollowingList->InsertItem(GetBubbleToPosition(aItem), aItem);
//			}
		}
	}	
	
	CleanupStack::PopAndDestroy(2); // aTextUtilities, aXmlParser

	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}

	if(aNotifyObserver) {
		NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
	}
}

void CBuddycloudLogic::HandleIncomingMessageL(TDesC& aFrom, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::HandleIncomingMessageL"));
#endif

	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aData);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));	
	TBool aNonMessageEvent = false;

	// ---------------------------------------------------------------------
	// Handle event data
	// ---------------------------------------------------------------------
	if(aXmlParser->MoveToElement(_L8("event"))) {
		TPtrC8 pAttributeXmlns = aXmlParser->GetStringAttribute(_L8("xmlns"));

		if(pAttributeXmlns.Compare(_L8("http://jabber.org/protocol/pubsub#event")) == 0) {
			// Pubsub notification event received
			TPtrC8 pItemData = aXmlParser->GetStringData();
			
			aNonMessageEvent = true;			

			if(aXmlParser->MoveToElement(_L8("items"))) {
				TPtrC8 pAttributeNode = aXmlParser->GetStringAttribute(_L8("node"));

				if(aXmlParser->MoveToElement(_L8("retract"))) {
					RosterPubsubDeleteL(aFrom, pAttributeNode);
				}
				else  {
					RosterPubsubEventL(aFrom, pItemData, true);
				}
			}
		}
	}
	else if(aXmlParser->MoveToElement(_L8("x"))) {
		if(aXmlParser->MoveToElement(_L8("invite"))) {
			aNonMessageEvent = true;
			
			if( !IsSubscribedTo(aFrom) ) {
				TPtrC8 pDataReason;
				
				if(aXmlParser->MoveToElement(_L8("reason"))) {
					pDataReason.Set(aXmlParser->GetStringData());
				}
				
				// Get body if reason is empty
				if(pDataReason.Length() == 0) {
					aXmlParser->ResetElementPointer();
					
					if(aXmlParser->MoveToElement(_L8("body"))) {
						pDataReason.Set(aXmlParser->GetStringData());
					}
				}
				
				// Process invite
				if(pDataReason.Length() > 0) {
					HBufC* aTitle = CEikonEnv::Static()->AllocReadResourceLC(R_LOCALIZED_STRING_CHANNELINVITE_TITLE);
					HBufC* aDescription = aTextUtilities->Utf8ToUnicodeL(pDataReason).AllocLC();
					
					CFollowingNoticeItem* aNoticeItem = CFollowingNoticeItem::NewLC();
					aNoticeItem->SetItemId(iNextItemId++);
					aNoticeItem->SetJidL(aFrom, EJidChannel);
					aNoticeItem->SetTitleL(*aTitle);
					aNoticeItem->SetDescriptionL(*aDescription);
					
					iMasterFollowingList->InsertItem(GetBubbleToPosition(aNoticeItem), aNoticeItem);
					CleanupStack::Pop(); // aNoticeItem
									
					NotifyNotificationObservers(ENotificationFollowingItemsUpdated);
					NotifyNotificationObservers(ENotificationMessageNotifiedEvent);
					
					CleanupStack::PopAndDestroy(2); // aDescription, aTitle
				}
				else {
					DeclineChannelInviteL(aFrom);
				}
			}
			else {
				DeclineChannelInviteL(aFrom);
			}
		}
	}

	if( !aNonMessageEvent ) {
		// -----------------------------------------------------------------
		// Seperate resource from jid
		//
		// Attribute aFrom contains both JID@SERVER/RESOURCE
		// It the case of a Group presence it will be ROOM@SERVER/JID
		// -----------------------------------------------------------------
		TPtrC pJid(aFrom);

		TInt aSearchResult = pJid.Locate('/');
		if(aSearchResult != KErrNotFound) {
			pJid.Set(pJid.Left(aSearchResult));
		}
		
		CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pJid);
		CFollowingItem* aItem = aDiscussion->GetFollowingItem();
		
		if(aItem) {
			ProcessNewMessageL(aFrom, aData);
		}
		else if(aAttributeType.Compare(_L8("groupchat")) != 0 && aXmlParser->MoveToElement(_L8("body"))) {
			// Add tempmorary item
			CFollowingRosterItem* aRosterItem = CFollowingRosterItem::NewLC();
			aRosterItem->SetItemId(iNextItemId++);
			aRosterItem->SetJidL(pJid);

			if(aXmlParser->MoveToElement(_L8("nick"))) {
				aRosterItem->SetTitleL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
			}
			else {
				TPtrC aName(pJid);				
				TInt aSearchResult = aName.Find(_L("@"));
				
				if(aSearchResult != KErrNotFound) {
					aName.Set(aName.Left(aSearchResult));
				}
				
				aRosterItem->SetTitleL(aName);
			}

			CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pJid);
			aDiscussion->AddMessageReadObserver(aRosterItem);
			aDiscussion->AddDiscussionReadObserver(this, aRosterItem);
			
			CMessagingParticipant* aParticipant = aDiscussion->GetParticipants()->AddParticipant(iOwnItem->GetJid());
			aParticipant->SetJidL(iOwnItem->GetJid());
			aParticipant->SetAvatarId(iOwnItem->GetAvatarId());

			iMasterFollowingList->AddItem(aRosterItem);
			CleanupStack::Pop();			
			
			// Process message
			ProcessNewMessageL(aFrom, aData);
		}
	}

	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
}

void CBuddycloudLogic::ProcessNewMessageL(TDesC& aFrom, const TDesC8& aData) {
#ifdef _DEBUG
	WriteToLog(_L8("BL    CBuddycloudLogic::ProcessNewMessageL"));
#endif

	TInt aNotifyId = KErrNotFound;
	TInt aAudioId = KErrNotFound;
	
	TBool aNotifyArrival = false;
	TBool aBubbleArrival = false;
	TBool aIsChannel = false;
	
	// Initialize xml parser
	CTextUtilities* aTextUtilities = CTextUtilities::NewLC();
	CXmlParser* aXmlParser = CXmlParser::NewLC(aData);
	TPtrC8 aAttributeType = aXmlParser->GetStringAttribute(_L8("type"));

	// -----------------------------------------------------------------
	// Convert Delay time (if delayed)
	// -----------------------------------------------------------------
	TTime aTime = TimeStamp();
	TBool aDelayed = false;

	if(aXmlParser->MoveToElement(_L8("x"))) {
		TPtrC8 aAttributeStamp = aXmlParser->GetStringAttribute(_L8("stamp"));
		
		if(aAttributeStamp.Length() > 0) {
			// Delayed
			CTimeCoder* aTimeCoder = CTimeCoder::NewLC();
			aTime = aTimeCoder->DecodeL(aAttributeStamp);
			CleanupStack::PopAndDestroy(); // aTimeCoder
	
			aDelayed = true;	
		}
		
		aXmlParser->ResetElementPointer();
	}

	// -----------------------------------------------------------------
	// Seperate resource from jid
	//
	// Attribute aFrom contains both JID@SERVER/RESOURCE
	// It the case of a Group presence it will be ROOM@SERVER/JID
	// -----------------------------------------------------------------
	TPtrC pJid(aFrom);
	TPtrC pResource;

	TInt aSearchResult = pJid.Locate('/');
	if(aSearchResult != KErrNotFound) {
		pResource.Set(pJid.Mid(aSearchResult + 1));
		pJid.Set(pJid.Left(aSearchResult));
	}
	
	CDiscussion* aDiscussion = iMessagingManager->GetDiscussionL(pJid);
	CFollowingItem* aItem = aDiscussion->GetFollowingItem();
	
	if(aItem && aItem->GetItemType() >= EItemRoster) {
		CFollowingChannelItem* aChannelItem = static_cast <CFollowingChannelItem*> (aItem);
		CMessagingParticipant* aParticipant = aDiscussion->GetParticipants()->GetParticipant(aFrom);

		if(pJid.CompareF(aChannelItem->GetJid()) == 0) {
			aIsChannel = true;
		}
		
		aNotifyId = aItem->GetItemId();	
		aItem->SetLastUpdated(TimeStamp());

		// -----------------------------------------------------
		// Get senders name
		// -----------------------------------------------------
		TBuf<256> aContactName;
		
		if(iSettingShowName && aXmlParser->MoveToElement(_L8("nick"))) {
			// Get nick
			aContactName.Copy(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()).Left(aContactName.MaxLength()));
			
			aXmlParser->ResetElementPointer();
		}
		else {
			if(aIsChannel) {
				aContactName.Copy(pResource.Left(aContactName.MaxLength()));
				
				if((aSearchResult = aContactName.LocateReverse('/')) != KErrNotFound) {
					aContactName.Delete(0, aSearchResult + 1);
				}							
			}
			else {
				aContactName.Copy(pJid.Left(aContactName.MaxLength()));
				
				if((aSearchResult = aContactName.Locate('@')) != KErrNotFound) {
					aContactName.Delete(aSearchResult, aContactName.Length());
				}							
			}
		}

		// -----------------------------------------------------
		// Subject
		// -----------------------------------------------------
		if(aXmlParser->MoveToElement(_L8("subject"))) {
			// Set topic
			TPtrC aEncSubject(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
			
			if(aItem->GetItemType() == EItemChannel) {
				aItem->SetDescriptionL(aEncSubject);
			}

			if(pResource.Length() > 0) {
				// Only add non-delayed subject messages to discussion
				_LIT(KSubjectChange, " has changed the topic to ''");
				HBufC* aSubjectBody = HBufC::NewLC(aContactName.Length() + KSubjectChange().Length() + aEncSubject.Length());
				TPtr pSubjectBody(aSubjectBody->Des());

				pSubjectBody.Append(aContactName);
				pSubjectBody.Append(KSubjectChange);
				pSubjectBody.Insert(pSubjectBody.Length() - 1, aEncSubject);

				CMessage* aMessage = CMessage::NewLC();
				aMessage->SetReceivedAt(aTime);
				aMessage->SetAvatarId(KIconChannel);
				aMessage->SetBodyL(pSubjectBody, iSettingUsername, EMessageUserAction);
				aMessage->SetRead(true);

				aDiscussion->AddMessageLD(aMessage, aDelayed);
				CleanupStack::PopAndDestroy(); // aSubjectBody
			}
		}

		// -----------------------------------------------------
		// Body
		// -----------------------------------------------------
		if(pResource.Compare(_L("adserver")) == 0) {
			if(aXmlParser->MoveToElement(_L8("html"))) {
				if(aXmlParser->MoveToElement(_L8("body"))) {
					if(aXmlParser->MoveToElement(_L8("a"))) {
						CMessage* aMessage = CMessage::NewLC();
						aMessage->SetReceivedAt(aTime);
						aMessage->SetRead(true);

						// Link
						TPtrC8 aLink = aXmlParser->GetStringAttribute(_L8("href"));								
						TPtrC8 aBody = aXmlParser->GetStringData();

						if(aBody.Length() == 0 && aXmlParser->MoveToElement(_L8("img"))) {
							aBody.Set(aXmlParser->GetStringAttribute(_L8("alt")));
						}
						
						HBufC8* aMessageBody = HBufC8::NewLC(aBody.Length() + 1 + aLink.Length());
						TPtr8 pMessageBody(aMessageBody->Des());
						pMessageBody.Append(aBody);
						pMessageBody.Append(_L8("\n"));
						pMessageBody.Append(aLink);
						
						aMessage->SetBodyL(aTextUtilities->Utf8ToUnicodeL(pMessageBody), iSettingUsername, EMessageAdvertisment);
						CleanupStack::PopAndDestroy(); // aMessageBody

						aDiscussion->AddMessageLD(aMessage, aDelayed);
					}
				}
			}
		}
		else {
			if(aXmlParser->MoveToElement(_L8("body"))) {
				if(!aIsChannel || pResource.Length() > 0) {
					TPtrC aDataBody = aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData());
					
					if(aDataBody.Length() > 0) {
						CMessage* aMessage = CMessage::NewLC();								
						aMessage->SetReceivedAt(aTime);	
						aMessage->SetNameL(aContactName);
						aMessage->SetJidL(aFrom, EMessageJidRoom);
						
						// Set message privacy
						if(aAttributeType.Compare(_L8("groupchat")) != 0) {
							aMessage->SetPrivate(true);
						}
						
						// Set participants jid & avatar
						if(aParticipant) {
							if(aParticipant->GetJid().Length() > 0) {
								aMessage->SetJidL(aParticipant->GetJid(), EMessageJidRoster);
							}
							
							aMessage->SetAvatarId(aParticipant->GetAvatarId());							
						}
						
						aNotifyArrival = true;
						
						// Check if an event
						if(aIsChannel && pResource.Compare(_L("events@butler.buddycloud.com")) == 0) {
							aMessage->SetRead(true);
							aMessage->SetAvatarId(KIconPlace);
							aMessage->SetBodyL(aDataBody, iSettingUsername, EMessageUserEvent);
							
							aNotifyArrival = false;
						}
						else {
							aMessage->SetBodyL(aDataBody, iSettingUsername);
						}
													
						// Set read if own
						if(iSettingUsername.CompareF(pResource) == 0) {
							aMessage->SetRead(true);
							aMessage->SetOwn(true);
							aNotifyArrival = false;
						}
						
						// Thread data
						if(aXmlParser->MoveToElement(_L8("thread"))) {
							aMessage->SetThreadL(aXmlParser->GetStringData());
						}
						
						// Location data
						if(aXmlParser->MoveToElement(_L8("geoloc"))) {
							if(aXmlParser->MoveToElement(_L8("locality"))) {
								aMessage->SetLocationL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()));
							}
							
							if(aXmlParser->MoveToElement(_L8("country"))) {
								aMessage->SetLocationL(aTextUtilities->Utf8ToUnicodeL(aXmlParser->GetStringData()), true);
							}
						}
						
						// Check reply to & notification validation
						if(aNotifyArrival) {
							aNotifyArrival = aDiscussion->GetNotificationOn();
							
							// Notify if @username reply found or channel notification setting applies
							if(aIsChannel) {
								if(iSettingNotifyReplyTo && aMessage->GetDirectReply()) {									
									// @reply to user
									aNotifyArrival = true;									
									aAudioId = ESettingItemDirectReplyTone;
								}
								else if(aNotifyArrival && (iSettingGroupNotification == 1 || 
										(iSettingGroupNotification == 0 && aDiscussion->GetUnreadMessages() == 0))) {									
									
									// Channel notification on
									aNotifyArrival = true;									
									aAudioId = ESettingItemChannelPostTone;
								}
								else {
									// Channel notification off
									aNotifyArrival = false;
								}
							}
							else if(aNotifyArrival) {
								// Private message notification
								aAudioId = ESettingItemPrivateMessageTone;
							}
						}

						if(aDiscussion->AddMessageLD(aMessage, aDelayed)) {
							aBubbleArrival = true;
						}
						else {
							aNotifyArrival = false;
						}
					}
				}
			}
		}

		// -----------------------------------------------------
		// Chat State
		// -----------------------------------------------------
		if(!aIsChannel && aParticipant) {
			if(aXmlParser->MoveToElement(KChatstateActive) || aXmlParser->MoveToElement(KChatstateInactive) ||
					aXmlParser->MoveToElement(KChatstateComposing) || aXmlParser->MoveToElement(KChatstatePaused)) {
	
				TPtrC8 pChatState = aXmlParser->GetElementName();
				TMessageChatState aState = EChatGone;
	
				if(pChatState.Compare(KChatstateActive) == 0) {
					aState = EChatActive;
				}
				else if(pChatState.Compare(KChatstateInactive) == 0) {
					aState = EChatInactive;
				}
				else if(pChatState.Compare(KChatstateComposing) == 0) {
					aState = EChatComposing;
				}
				else if(pChatState.Compare(KChatstatePaused) == 0) {
					aState = EChatPaused;
				}
	
				aParticipant->SetChatState(aState);
			}
		}

		if(aBubbleArrival) {
			if(!aItem->GetIsOwn()) {
				iMasterFollowingList->RemoveItemByIndex(iMasterFollowingList->GetIndexById(aItem->GetItemId()));
				iMasterFollowingList->InsertItem(GetBubbleToPosition(aItem), aItem);
			}
			
			NotifyNotificationObservers(ENotificationFollowingItemsUpdated, aNotifyId);
		}
	}
	
	if(iState == ELogicOnline) {
		iLastReceivedAt = TimeStamp();
	}

	if(aNotifyArrival && !iTelephonyEngine->IsTelephonyBusy()) {
		NotifyNotificationObservers(ENotificationMessageNotifiedEvent, aNotifyId);
		
		iNotificationEngine->NotifyL(aAudioId);
	}
	else {
		NotifyNotificationObservers(ENotificationMessageSilentEvent, aNotifyId);
	}

	CleanupStack::PopAndDestroy(2); // aXmlParser, aTextUtilities
}

// End of File
