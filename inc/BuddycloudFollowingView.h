/*
============================================================================
 Name        : BuddycloudFollowingView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Declares Following View
============================================================================
*/

#ifndef BUDDYCLOUDFOLLOWINGVIEW_H
#define BUDDYCLOUDFOLLOWINGVIEW_H

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"
#include "ViewAccessorObserver.h"

#ifdef __SERIES60_40__
#include <akntoolbar.h>
#include <akntoolbarobserver.h>
#endif

// FORWARD DECLARATIONS
class CBuddycloudFollowingContainer;

// CLASS DECLARATION
#ifndef __SERIES60_40__
class CBuddycloudFollowingView : public CAknView, MViewAccessorObserver {
#else
class CBuddycloudFollowingView : public CAknView, MViewAccessorObserver, MAknToolbarObserver {
#endif
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudFollowingView();

	public: // From CAknView
		TUid Id() const;
		void ProcessCommandL(TInt aCommand);
		void HandleCommandL(TInt aCommand);
	
	public: // From MEikMenuObserver
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		
#ifdef __SERIES60_40__
	public: // From MAknToolbarObserver
		void OfferToolbarEventL(TInt aCommandId);
#endif

	protected:
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
		CBuddycloudFollowingContainer* iContainer;
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
