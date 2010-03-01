/*
============================================================================
 Name        : BuddycloudMessagingView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Messaging View
============================================================================
*/

#ifndef BUDDYCLOUDMESSAGINGVIEW_H_
#define BUDDYCLOUDMESSAGINGVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"
#include "BuddycloudMessagingContainer.h"
#include "ViewAccessorObserver.h"

#ifdef __SERIES60_40__
#include <akntoolbar.h>
#include <akntoolbarobserver.h>
#endif

/*
----------------------------------------------------------------------------
--
-- CBuddycloudMessagingView
--
----------------------------------------------------------------------------
*/

#ifndef __SERIES60_40__
class CBuddycloudMessagingView : public CAknView, MViewAccessorObserver {
#else
class CBuddycloudMessagingView : public CAknView, MViewAccessorObserver, MAknToolbarObserver {
#endif
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudMessagingView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);
		
#ifdef __SERIES60_40__
	public: // From MAknToolbarObserver
		void OfferToolbarEventL(TInt aCommandId);
#endif

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();
	
	public:
		CEikMenuBar* ViewMenuBar();
		CEikStatusPane* ViewStatusPane();
		CEikButtonGroupContainer* ViewCba();
#ifdef __SERIES60_40__
		CAknToolbar* ViewToolbar();
#endif

	private: // Data
		CBuddycloudMessagingContainer* iContainer;

		CBuddycloudLogic* iBuddycloudLogic;
};

#endif /*BUDDYCLOUDMESSAGINGVIEW_H_*/

// End of File
