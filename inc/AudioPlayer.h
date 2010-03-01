/*
 ============================================================================
 Name		 : AudioPlayer.h
 Author	     : Ross Savage
 Copyright   : 2009 Buddycloud
 Description : Loads & plays an audio clip
 ============================================================================
 */

#ifndef AUDIOPLAYER_H_
#define AUDIOPLAYER_H_

#include <e32base.h>
#include <mdaaudiosampleplayer.h>

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MAudioPlayerObserver {
	public:
		virtual void AudioPlayed(TInt aAudioId) = 0;
};

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TAudioState {
	TAudioUninitialized, TAudioReady, TAudioPlaying
};

/*
----------------------------------------------------------------------------
--
-- CAudioPlayer
--
----------------------------------------------------------------------------
*/

class CAudioPlayer : public CBase, MMdaAudioPlayerCallback {
	public:
		static CAudioPlayer* NewL(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate);
		static CAudioPlayer* NewLC(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate);
		~CAudioPlayer();
		
	private:
		CAudioPlayer(TInt aAudioId, TBool aVibrate);
		void ConstructL(const TDesC& aAudioFile);
		
	public:
		TInt GetId();
		TBool GetVibrate();
		
	public:
		void SetObserver(MAudioPlayerObserver* aObserver);
		void ChangeFileL(const TDesC& aAudioFile);
		
	public:
		void SetVolume(TInt aVolume);
		void Play();
		void Stop();
		
	public: // From MMdaAudioPlayerCallback
		void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
		void MapcPlayComplete(TInt aError);
		
	private:
		CMdaAudioPlayerUtility* iMdaPlayer;		
		MAudioPlayerObserver* iObserver;
		
		TAudioState iAudioState;
				
		TInt iId;
		TInt iVolume;
		TInt iVibrate;
};

#endif
