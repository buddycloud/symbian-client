/*
============================================================================
 Name        : BuddycloudAppUi.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : CBuddycloudAppUi implementation
============================================================================
*/

// INCLUDE FILES
#include <akncontext.h> 
#include <akndef.h>
#include <aknsutils.h>
#include <apgtask.h>
#include <aknmessagequerydialog.h>
#include <avkon.hrh>
#include <bacline.h>
#include <bautils.h>
#include "BrowserLauncher.h"
#include <Buddycloud.rsg>
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"
#include "Buddycloud.pan"
#include "BuddycloudAccountSettingsView.h"
#include "BuddycloudAppUi.h"
#include "BuddycloudBeaconSettingsView.h"
#include "BuddycloudChannelInfoView.h"
#include "BuddycloudCommunitiesView.h"
#include "BuddycloudConstants.h"
#include "BuddycloudEditChannelView.h"
#include "BuddycloudEditPlaceView.h"
#include "BuddycloudExplorerView.h"
#include "BuddycloudFollowing.h"
#include "BuddycloudFollowingView.h"
#include "BuddycloudIconIds.h"
#include "BuddycloudMessagingView.h"
#include "BuddycloudNotificationsSettingsView.h"
#include "BuddycloudPlacesView.h"
#include "BuddycloudPreferencesSettingsView.h"
#include "BuddycloudSetupView.h"
#include "FileUtilities.h"
#include "PhoneUtilities.h"

#define KEnableSkinFlag 0x1000
#define KLayoutAwareFlag 0x08

void CBuddycloudAppUi::ConstructL() {
	TInt aErr;	
#ifdef __SERIES60_3X__
	TRAP(aErr, BaseConstructL(EAknEnableSkin | EAppOrientationAutomatic));
#else
	TRAP(aErr, BaseConstructL(KEnableSkinFlag | KLayoutAwareFlag));
#endif
	PanicIfError(_L("AUI-BC"), aErr);
	
	iLanguageOffset = 0;
	
	AknsUtils::InitSkinSupportL();
		
	iBuddycloudLogic = CBuddycloudLogic::NewL(this);	
	iBuddycloudLogic->AddNotificationObserver(this);
	
	CreateViewsL();
	StateChanged(ELogicOffline);
}

CBuddycloudAppUi::~CBuddycloudAppUi() {
	if(iBuddycloudLogic) {
		delete iBuddycloudLogic;
		iBuddycloudLogic = NULL;
	}
	
	if(iTimer)
		delete iTimer;
	
	if(iLanguageOffset) {
		iCoeEnv->DeleteResourceFile(iLanguageOffset);
	}
}

void CBuddycloudAppUi::CreateViewsL() {
	CBuddycloudSetupView* aViewSetup = new (ELeave) CBuddycloudSetupView;
	CleanupStack::PushL(aViewSetup);
	aViewSetup->ConstructL(iBuddycloudLogic);
	AddViewL(aViewSetup);
	CleanupStack::Pop();

	CBuddycloudAccountSettingsView* aViewAccountSettings = new (ELeave) CBuddycloudAccountSettingsView;
	CleanupStack::PushL(aViewAccountSettings);
	aViewAccountSettings->ConstructL(iBuddycloudLogic);
	AddViewL(aViewAccountSettings);
	CleanupStack::Pop();

	CBuddycloudPreferencesSettingsView* aViewPreferencesSettings = new (ELeave) CBuddycloudPreferencesSettingsView;
	CleanupStack::PushL(aViewPreferencesSettings);
	aViewPreferencesSettings->ConstructL(iBuddycloudLogic);
	AddViewL(aViewPreferencesSettings);
	CleanupStack::Pop();

	CBuddycloudNotificationsSettingsView* aViewNotificationsSettings = new (ELeave) CBuddycloudNotificationsSettingsView;
	CleanupStack::PushL(aViewNotificationsSettings);
	aViewNotificationsSettings->ConstructL(iBuddycloudLogic);
	AddViewL(aViewNotificationsSettings);
	CleanupStack::Pop();

	CBuddycloudBeaconSettingsView* aViewBeaconSettings = new (ELeave) CBuddycloudBeaconSettingsView;
	CleanupStack::PushL(aViewBeaconSettings);
	aViewBeaconSettings->ConstructL(iBuddycloudLogic);
	AddViewL(aViewBeaconSettings);
	CleanupStack::Pop();

	CBuddycloudCommunitiesView* aViewCommunities = new (ELeave) CBuddycloudCommunitiesView;
	CleanupStack::PushL(aViewCommunities);
	aViewCommunities->ConstructL(iBuddycloudLogic);
	AddViewL(aViewCommunities);
	CleanupStack::Pop();

	CBuddycloudFollowingView* aViewFollowing = new (ELeave) CBuddycloudFollowingView;
	CleanupStack::PushL(aViewFollowing);
	aViewFollowing->ConstructL(iBuddycloudLogic);
	AddViewL(aViewFollowing);
	CleanupStack::Pop();

	CBuddycloudMessagingView* aViewMessaging = new (ELeave) CBuddycloudMessagingView;
	CleanupStack::PushL(aViewMessaging);
	aViewMessaging->ConstructL(iBuddycloudLogic);
	AddViewL(aViewMessaging);
	CleanupStack::Pop();

	CBuddycloudPlacesView* aViewPlaces = new (ELeave) CBuddycloudPlacesView;
	CleanupStack::PushL(aViewPlaces);
	aViewPlaces->ConstructL(iBuddycloudLogic);
	AddViewL(aViewPlaces);
	CleanupStack::Pop();

	CBuddycloudExplorerView* aViewExplorer = new (ELeave) CBuddycloudExplorerView;
	CleanupStack::PushL(aViewExplorer);
	aViewExplorer->ConstructL(iBuddycloudLogic);
	AddViewL(aViewExplorer);
	CleanupStack::Pop();

	CBuddycloudEditPlaceView* aViewEditPlace = new (ELeave) CBuddycloudEditPlaceView;
	CleanupStack::PushL(aViewEditPlace);
	aViewEditPlace->ConstructL(iBuddycloudLogic);
	AddViewL(aViewEditPlace);
	CleanupStack::Pop();

	CBuddycloudEditChannelView* aViewEditChannel = new (ELeave) CBuddycloudEditChannelView;
	CleanupStack::PushL(aViewEditChannel);
	aViewEditChannel->ConstructL(iBuddycloudLogic);
	AddViewL(aViewEditChannel);
	CleanupStack::Pop();

	CBuddycloudChannelInfoView* aViewChannelInfo = new (ELeave) CBuddycloudChannelInfoView;
	CleanupStack::PushL(aViewChannelInfo);
	aViewChannelInfo->ConstructL(iBuddycloudLogic);
	AddViewL(aViewChannelInfo);
	CleanupStack::Pop();

	if(iBuddycloudLogic->GetDescSetting(ESettingItemUsername).Length() > 0) {
		if(iBuddycloudLogic->GetFollowingStore()->Count() > 1) {
			ActivateLocalViewL(KFollowingViewId);
		}
		else {
			ActivateLocalViewL(KExplorerViewId);
		}
	}
	else {
		ActivateLocalViewL(KSetupViewId);
	}
}

void CBuddycloudAppUi::TimerExpired(TInt /*aExpiryId*/) {
	ShutdownComplete();
}

void CBuddycloudAppUi::StateChanged(TBuddycloudLogicState aState) {
	CAknContextPane* aContextPane = (CAknContextPane*)StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidContext));
	
	if(aContextPane) {
		TFileName aFileName(_L("\\resource\\apps\\Buddycloud.mif"));
		
		CFileUtilities::CompleteWithDrive(iCoeEnv->FsSession(), aFileName);

		if(aState == ELogicOnline) {
			aContextPane->SetPictureFromFileL(aFileName, EMbmBuddycloudiconidsBuddycloud, EMbmBuddycloudiconidsBuddycloud);  
		}
		else {
			aContextPane->SetPictureFromFileL(aFileName, EMbmBuddycloudiconidsBuddycloud_grey, EMbmBuddycloudiconidsBuddycloud_grey);  
		}
	}
}

void CBuddycloudAppUi::LanguageChanged(TInt aLanguage) {
	_LIT(KLanguageFile, "\\resource\\apps\\Buddycloud_lang.r");
	
	TFileName aLanguageFile(KLanguageFile);
	
	// Get collect file name for resource file
	if(aLanguage == KErrNotFound) {
		aLanguage = User::Language();
	}
	
	if(aLanguage > 9) {
		aLanguageFile.AppendFormat(_L("%d"), aLanguage);
	}
	else {
		aLanguageFile.AppendFormat(_L("%02d"), aLanguage);
	}
	
	CFileUtilities::CompleteWithDrive(iCoeEnv->FsSession(), aLanguageFile);
	
	if(!BaflUtils::FileExists(iCoeEnv->FsSession(), aLanguageFile)) {
		// Ideal or selected language doesn't exist, load English as default
		aLanguageFile.Copy(KLanguageFile);
		aLanguageFile.Append(_L("01"));	
		
		CFileUtilities::CompleteWithDrive(iCoeEnv->FsSession(), aLanguageFile);
	}
	
	if(iLanguageOffset) {
		iCoeEnv->DeleteResourceFile(iLanguageOffset);
	}
	
	iLanguageOffset = iCoeEnv->AddResourceFileL(aLanguageFile);
}

void CBuddycloudAppUi::ShutdownComplete() {
	Exit();
}

// #define _DEVICEDEBUG

TBool CBuddycloudAppUi::ProcessCommandParametersL(CApaCommandLine &aCommandLine) {	
	if(aCommandLine.OpaqueData().Length() > 0) {
		// User started
		iAllowFocus = true;
	}
	else {		
		if(iBuddycloudLogic->GetBoolSetting(ESettingItemAutoStart) || 
				iBuddycloudLogic->GetBoolSetting(ESettingItemNewInstall)) {
			
			// Auto start enabled
			iAllowFocus = true;
		}
		else {
#ifdef _DEVICEDEBUG
			iAllowFocus = true;
#else			
			TApaTask aTask(iEikonEnv->WsSession());
			aTask.SetWgId(iEikonEnv->RootWin().Identifier());
			aTask.SendToBackground();

			// Auto start disabled
			iTimer = CCustomTimer::NewL(this);
			iTimer->After(5000000);
#endif
		}
	}
	
	if(iAllowFocus) {
		// Continue startup
		iBuddycloudLogic->Startup();
	}
  
	return false;
}

TInt AboutDialogCallback(TAny* /*aAny*/) {
	CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
	aLauncher->LaunchBrowserWithLinkL(_L("http://www.buddycloud.com"));
	CleanupStack::PopAndDestroy(); // aLauncher

	return EFalse;
}

void CBuddycloudAppUi::HandleCommandL(TInt aCommand) {
	if(aCommand == EEikCmdExit) {
		TApaTask aTask(iEikonEnv->WsSession());
		aTask.SetWgId(iEikonEnv->RootWin().Identifier());
		aTask.SendToBackground();

		iBuddycloudLogic->PrepareShutdown();
	}
	else if(aCommand == EApplicationExit || aCommand == EMenuExitCommand) {
		TApaTask aTask(iEikonEnv->WsSession());
		aTask.SetWgId(iEikonEnv->RootWin().Identifier());
		aTask.BringToForeground();

		if(!iExitDialogShowing) {
			iExitDialogShowing = true;
			
			CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
	
			if(dlg->ExecuteLD(R_EXIT_DIALOG) != 0) {
				aTask.SendToBackground();
	
				iBuddycloudLogic->PrepareShutdown();
			}
		}
		
		iExitDialogShowing = false;
	}
	else if(aCommand == EMenuUserSettingsCommand) {
		ActivateLocalViewL(KAccountSettingsViewId);
	}
	else if(aCommand == EMenuCommunitySettingsCommand) {
		ActivateLocalViewL(KCommunitiesViewId);
	}
	else if(aCommand == EMenuClearConnectionCommand) {
		iBuddycloudLogic->ResetConnectionSettings();
	}
	else if(aCommand == EMenuHelpCommand) {
		CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
		aLauncher->LaunchBrowserWithLinkL(_L("http://buddycloud.com/help"));
		CleanupStack::PopAndDestroy(); // aLauncher
	}
	else if(aCommand == EMenuUpdateCommand) {
		HBufC* aVersion = iEikonEnv->AllocReadResourceLC(R_STRING_APPVERSION);
		TPtrC pVersion(aVersion->Des());
		
		TBuf<128> aPhoneModel;
		TInt aLocateResult = KErrNotFound;
		CPhoneUtilities::GetPhoneModelL(aPhoneModel);
		
		while((aLocateResult = aPhoneModel.Locate(' ')) != KErrNotFound) {
			aPhoneModel.Replace(aLocateResult, 1, _L("%20"));
		}
		
		_LIT(KUrlLink, "http://m.buddycloud.com/index.php?device=&build=");
		HBufC* aUrlLink = HBufC::NewLC(KUrlLink().Length() + pVersion.Length() + aPhoneModel.Length());
		TPtr pUrlLink(aUrlLink->Des());
		pUrlLink.Append(KUrlLink);
		pUrlLink.Append(pVersion);
		pUrlLink.Insert(41, aPhoneModel);
	
		CBrowserLauncher* aLauncher = CBrowserLauncher::NewLC();
		aLauncher->LaunchBrowserWithLinkL(pUrlLink);
		CleanupStack::PopAndDestroy(3); // aLauncher, aUrlLink, aVersion
	}
	else if(aCommand == EMenuAboutCommand) {
		TInt aDataSent;
		TInt aDataReceived;
		TBuf<256> aTextBuffer;
		
		// Get statistics
		iBuddycloudLogic->GetConnectionStatistics(aDataSent, aDataReceived);
		
		// Calculate kilobytes
		if(aDataSent > 0) {
			aDataSent = (aDataSent / 1024);
		}
		
		if(aDataReceived > 0) {
			aDataReceived = (aDataReceived / 1024);
		}
		
		// About text
		HBufC* aAboutText = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_ABOUT_TEXT);
		aTextBuffer.Format(*aAboutText, aDataSent, aDataReceived);
		CleanupStack::PopAndDestroy(); // aAboutText
		
		// Translated by text
		HBufC* aTranslatedText = iEikonEnv->AllocReadResourceLC(R_LOCALIZED_STRING_TRANSLATEDBY);
		TPtr pTranslatedText(aTranslatedText->Des());
		
		if(pTranslatedText.Length() > 0) {
			aTextBuffer.Insert(33, pTranslatedText);
			aTextBuffer.Insert(33, _L("\n"));
		}
		
		CleanupStack::PopAndDestroy(); // aTranslatedText
		
		// Show dialog		
		CAknMessageQueryDialog* aMessageDialog = new (ELeave) CAknMessageQueryDialog();
		aMessageDialog->PrepareLC(R_ABOUT_DIALOG);
		aMessageDialog->SetMessageTextL(aTextBuffer);
		
		TCallBack aCallback(AboutDialogCallback);
		aMessageDialog->SetLink(aCallback);
		aMessageDialog->SetLinkTextL(_L("http://buddycloud.com"));
		
		aMessageDialog->RunLD();
	}
}

void CBuddycloudAppUi::HandleForegroundEventL(TBool aForeground) {
	if( aForeground && !iAllowFocus ) {
		TApaTask aTask(iEikonEnv->WsSession());
		aTask.SetWgId(iEikonEnv->RootWin().Identifier());
		aTask.SendToBackground();
	}
}

void CBuddycloudAppUi::HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination) {
	switch(aEvent.Type()) {
		case KAknUidValueEndKeyCloseEvent:
			HandleCommandL(EApplicationExit);
			break;
		default:
			CAknAppUi::HandleWsEventL(aEvent, aDestination);
			break;
	}	
}

void CBuddycloudAppUi::NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId) {
	if(aEvent == ENotificationNotifiedMessageEvent) {
		User::ResetInactivityTime();
	}
	else if(aEvent == ENotificationEditPlaceRequested) {
		ActivateLocalViewL(KEditPlaceViewId, TUid::Uid(aId), KNullDesC8);
	}
	else if(aEvent == ENotificationAuthenticationFailed) {
		ActivateLocalViewL(KAccountSettingsViewId, TUid::Uid(1), KNullDesC8);
	}
	else if(aEvent == ENotificationServerResolveFailed) {
		ActivateLocalViewL(KAccountSettingsViewId, TUid::Uid(3), KNullDesC8);
	}
}
