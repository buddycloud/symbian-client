/*
============================================================================
 Name        : BuddycloudPlacesView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Places View
============================================================================
*/

#ifndef BUDDYCLOUDPLACESVIEW_H_
#define BUDDYCLOUDPLACESVIEW_H_

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
class CBuddycloudPlacesContainer;

// CLASS DECLARATION
#ifndef __SERIES60_40__
class CBuddycloudPlacesView : public CAknView, MViewAccessorObserver {
#else
class CBuddycloudPlacesView : public CAknView, MViewAccessorObserver, MAknToolbarObserver {
#endif
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudPlacesView();

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
		CBuddycloudPlacesContainer* iContainer;
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif /*BUDDYCLOUDPLACESVIEW_H_*/
