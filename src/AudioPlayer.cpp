/*
 ============================================================================
 Name		 : AudioPlayer.cpp
 Author	     : Ross Savage
 Copyright   : 2009 Buddycloud
 Description : Loads & plays an audio clip
 ============================================================================
 */

#include "AudioPlayer.h"

CAudioPlayer* CAudioPlayer::NewL(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate) {
	CAudioPlayer* self = NewLC(aAudioId, aAudioFile, aVibrate);
	CleanupStack::Pop(self);
	
	return self;
}

CAudioPlayer* CAudioPlayer::NewLC(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate) {
	CAudioPlayer* self = new (ELeave) CAudioPlayer(aAudioId, aVibrate);
	CleanupStack::PushL(self);
	self->ConstructL(aAudioFile);
	
	return self;
}

CAudioPlayer::~CAudioPlayer() {
	if(iMdaPlayer) {
		iMdaPlayer->Stop();
		iMdaPlayer->Close();
		
		delete iMdaPlayer;
	}
}

CAudioPlayer::CAudioPlayer(TInt aAudioId, TBool aVibrate) {
	iId = aAudioId;
	iVibrate = aVibrate;
	
	iAudioState = TAudioUninitialized;
}

void CAudioPlayer::ConstructL(const TDesC& aAudioFile) {
	iObserver = NULL;
	
	// Setup player
	iMdaPlayer = CMdaAudioPlayerUtility::NewL(*this);
	iMdaPlayer->OpenFileL(aAudioFile);
}

TInt CAudioPlayer::GetId() {
	return iId;
}

TBool CAudioPlayer::GetVibrate() {
	return iVibrate;
}

void CAudioPlayer::SetObserver(MAudioPlayerObserver* aObserver) {
	iObserver = aObserver;
}

void CAudioPlayer::ChangeFileL(const TDesC& aAudioFile) {
	Stop();
	
	if(iMdaPlayer) {
		if(iAudioState == TAudioReady) {
			iMdaPlayer->Close();
			iAudioState = TAudioUninitialized;
		}
		
		iMdaPlayer->OpenFileL(aAudioFile);
	}
}

void CAudioPlayer::SetVolume(TInt aVolume) {
	iVolume = aVolume;
	
	if(iAudioState > TAudioUninitialized && iMdaPlayer) {
		iMdaPlayer->SetVolume((iMdaPlayer->MaxVolume() / 10) * aVolume);
	}
}

void CAudioPlayer::Play() {
	if(iAudioState > TAudioUninitialized && iVolume > 1 && iMdaPlayer) {
		if(iAudioState == TAudioReady) {
			iAudioState = TAudioPlaying;
			
			if(iObserver) {
				iObserver->AudioPlayed(iId);
			}
			
			iMdaPlayer->Play();
		}
	}
	else if(iVibrate && iObserver) {
		iObserver->AudioPlayed(iId);
	}
}

void CAudioPlayer::Stop() {
	if(iAudioState > TAudioUninitialized && iMdaPlayer) {
		iMdaPlayer->Stop();
		iAudioState = TAudioReady;
	}
}

void CAudioPlayer::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& /*aDuration*/) {	
	if(aError == KErrNone) {
		iAudioState = TAudioReady;
		
		iMdaPlayer->SetVolume((iMdaPlayer->MaxVolume() / 10) * iVolume);
	}
}

void CAudioPlayer::MapcPlayComplete(TInt /*aError*/) {
	iAudioState = TAudioReady;
}
