/*
============================================================================
 Name        : BuddycloudAppUi.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares UI class for application.
============================================================================
*/

#ifndef BUDDYCLOUDAPPUI_H
#define BUDDYCLOUDAPPUI_H

// INCLUDES
#include <aknviewappui.h>
#include "BuddycloudLogic.h"

class CBuddycloudAppUi : public CAknViewAppUi, MTimeoutNotification, 
									MBuddycloudLogicOwnerObserver, MBuddycloudLogicNotificationObserver {
	public:
		void ConstructL();
		~CBuddycloudAppUi();

	private: 
		void CreateViewsL();	
		
	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	public: // From MBuddycloudLogicOwnerObserver 
		void StateChanged(TBuddycloudLogicState aState);
		void LanguageChanged(TInt aLanguage);
		void ShutdownComplete();
		
	public: // From CAknViewAppUi 
		TBool ProcessCommandParametersL(CApaCommandLine &aCommandLine);
		
	protected: // From CAknViewAppUi
		void HandleCommandL(TInt aCommand);
		void HandleForegroundEventL(TBool aForeground);
		void HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination);

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	private: // Variables
		CBuddycloudLogic* iBuddycloudLogic;
		CCustomTimer* iTimer;
		
		TBool iAllowFocus;
		TBool iExitDialogShowing;
		
		TInt iLanguageOffset;
};

#endif


