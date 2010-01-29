/*
============================================================================
 Name        : BuddycloudExplorerView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Explorer View
============================================================================
*/

#ifndef BUDDYCLOUDEXPLORERVIEW_H_
#define BUDDYCLOUDEXPLORERVIEW_H_

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
class CBuddycloudExplorerContainer;

// CLASS DECLARATION
#ifndef __SERIES60_40__
class CBuddycloudExplorerView : public CAknView, MViewAccessorObserver {
#else
class CBuddycloudExplorerView : public CAknView, MViewAccessorObserver, MAknToolbarObserver {
#endif
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudExplorerView();

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
		CBuddycloudExplorerContainer* iContainer;
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif /*BUDDYCLOUDEXPLORERVIEW_H_*/
