/*
============================================================================
 Name        : 	NotificationEngine.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Notification Engine to alert the user via vibration
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef NOTIFICATIONENGINE_H_
#define NOTIFICATIONENGINE_H_

// INCLUDES
#include <e32base.h>
#include <centralrepository.h>
#include "AudioPlayer.h"

#ifdef __SERIES60_3X__
#include <hwrmvibra.h> // hwrmvibraclient.lib
#else
#include <vibractrl.h> // vibractrl.lib
#endif

/*
----------------------------------------------------------------------------
--
-- CNotificationEngine
--
----------------------------------------------------------------------------
*/

class CNotificationEngine : public CActive, MAudioPlayerObserver {
	public:
		static CNotificationEngine* NewL();
		static CNotificationEngine* NewLC();
		~CNotificationEngine();

	private:
		CNotificationEngine();
		void ConstructL();
		
	public:
		void AddAudioItemL(TInt aAudioId, const TDesC& aAudioFile, TBool aVibrate = true);
		void NotifyL(TInt aAudioId);
		void VibrateL(TInt aMilliseconds);
		
	public: // From CActive
		void DoCancel();
		void RunL();
		
	public: // From MAudioPlayerObserver
		void AudioPlayed(TInt aAudioId);
		
	private:
		void UpdateFromProfile();

	private:
		// Settings
		CRepository* iRepository;
		TInt iPlayVolume;
		
		// Vibration controller
#ifdef __SERIES60_3X__
		CHWRMVibra* iVibra;
#else
		CVibraControl* iVibra;
#endif
		
		// Audio items
		RPointerArray<CAudioPlayer> iAudioItems;
};

#endif // NOTIFICATIONENGINE_H_
