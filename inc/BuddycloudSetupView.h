/*
============================================================================
 Name        : BuddycloudSetupView.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Setup View
============================================================================
*/

#ifndef BUDDYCLOUDSETUPVIEW_H_
#define BUDDYCLOUDSETUPVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudLogic.h"

// FORWARD DECLARATIONS
class CBuddycloudSetupContainer;

// CLASS DECLARATION
class CBuddycloudSetupView : public CAknView, MCoeControlObserver {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudSetupView();

	public: // From CAknView
		TUid Id() const;
		void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudSetupContainer* iContainer;
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif

// End of File
