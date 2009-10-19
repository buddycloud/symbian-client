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

// FORWARD DECLARATIONS
class CBuddycloudPlacesContainer;

// CLASS DECLARATION
class CBuddycloudPlacesView : public CAknView, MViewAccessorObserver {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudPlacesView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

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
