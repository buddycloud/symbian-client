/*
============================================================================
 Name        : BuddycloudChannelInfoView.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2010
 Description : Declares Channel Info View
============================================================================
*/

#ifndef BUDDYCLOUDCHANNELINFOVIEW_H_
#define BUDDYCLOUDCHANNELINFOVIEW_H_

// INCLUDES
#include <aknview.h>
#include <coecobs.h>
#include "BuddycloudConstants.h"
#include "BuddycloudChannelInfoContainer.h"
#include "BuddycloudLogic.h"
#include "ViewReference.h"

// CLASS DECLARATION
class CBuddycloudChannelInfoView : public CAknView {
	public: // Constructors and destructor
		void ConstructL(CBuddycloudLogic* aBuddycloudLogic);
		~CBuddycloudChannelInfoView();

	public: // Functions from base classes
		TUid Id() const;
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	private:
		void DoActivateL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
		void DoDeactivate();

	private: // Data
		CBuddycloudChannelInfoContainer* iContainer;		
		CBuddycloudLogic* iBuddycloudLogic;
		
		TUid iPrevViewId;
		TViewData iPrevViewData;
};

#endif /* BUDDYCLOUDCHANNELINFOVIEW_H_ */
