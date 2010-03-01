/*
============================================================================
 Name        : 	NotificationEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Notification Engine to alert the user via vibration
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include "NotificationEngine.h"
#include <ProfileEngineSDKCRKeys.h>

CNotificationEngine* CNotificationEngine::NewL() {
	CNotificationEngine* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CNotificationEngine* CNotificationEngine::NewLC() {
	CNotificationEngine* self = new (ELeave) CNotificationEngine();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CNotificationEngine::~CNotificationEngine() {
	// Repository
	Cancel();
	
	if(iRepository) {
		delete iRepository;
	}
	
	// Audio
	for(TInt i = 0; i < iAudioItems.Count(); i++) {
		delete iAudioItems[i];
	}
	
	iAudioItems.Close();
	
	// Vibration
	if(iVibra)
		delete iVibra;
}

CNotificationEngine::CNotificationEngine() : CActive(EPriorityNormal) {
	iPlayVolume = 1;
}

void CNotificationEngine::ConstructL() {
	CActiveScheduler::Add(this);
	
	// Profile state
	iRepository = CRepository::NewL(KCRUidProfileEngine);
	UpdateFromProfile();
	
#ifdef __SERIES60_3X__
	iVibra = CHWRMVibra::NewL();
#else
	iVibra = CVibraControl::NewL();
#endif
}

void CNotificationEngine::AddAudioItemL(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate) {
	for(TInt i = 0; i < iAudioItems.Count(); i++) {
		if(iAudioItems[i]->GetId() == aAudioId) {
			iAudioItems[i]->ChangeFileL(aAudioFile);
			
			return;
		}
	}
	
	// Add file
	CAudioPlayer* aAudioItem = CAudioPlayer::NewLC(aAudioId, aAudioFile, aVibrate);
	aAudioItem->SetObserver(this);
	aAudioItem->SetVolume(iPlayVolume);
	
	iAudioItems.Append(aAudioItem);
	CleanupStack::Pop(); // aAudioItem
}

void CNotificationEngine::NotifyL(TInt aAudioId) {
	for(TInt i = 0; i < iAudioItems.Count(); i++) {
		if(iAudioItems[i]->GetId() == aAudioId) {
			iAudioItems[i]->Play();			
			break;
		}
	}
}

void CNotificationEngine::VibrateL(TInt aMilliseconds) {
#ifdef __SERIES60_3X__
	if(iVibra->VibraSettings() == CHWRMVibra::EVibraModeON && 
			iVibra->VibraStatus() > CHWRMVibra::EVibraStatusNotAllowed) {
		
		TRAPD(aErr, iVibra->StartVibraL(aMilliseconds));
	}
#else
	if(iVibra->VibraSettings() == CVibraControl::EVibraModeON) {
		TRAPD(aErr, iVibra->StartVibraL(aMilliseconds));
	}
#endif
}

void CNotificationEngine::DoCancel() {
	if(iRepository) {
		iRepository->NotifyCancel(KProEngActiveRingingVolume);
	}
}

void CNotificationEngine::RunL() {
	UpdateFromProfile();
}

void CNotificationEngine::AudioPlayed(TInt aAudioId) {
	for(TInt i = 0; i < iAudioItems.Count(); i++) {
		if(iAudioItems[i]->GetId() == aAudioId && iAudioItems[i]->GetVibrate()) {
			VibrateL(750);
			break;
		}
	}
}

void CNotificationEngine::UpdateFromProfile() {
	TInt aValue;
	
	iPlayVolume = 1;
	
	if(iRepository && iRepository->Get(KProEngActiveRingingVolume, aValue) == KErrNone) {
		iPlayVolume = aValue;
		
		// Update volume
		for(TInt i = 0; i < iAudioItems.Count(); i++) {
			if(iPlayVolume == 1) {
				iAudioItems[i]->Stop();
			}
			
			iAudioItems[i]->SetVolume(aValue);
		}
		
		iRepository->NotifyRequest(KProEngActiveRingingVolume, iStatus);
		SetActive();
	}
}
