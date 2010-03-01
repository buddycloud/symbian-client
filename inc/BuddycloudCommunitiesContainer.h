/*
============================================================================
 Name        : BuddycloudCommunitiesContainer.h
 Author      : Ross Savage
 Copyright   : 2008 Buddycloud
 Description : Declares Communities Container
============================================================================
*/

#ifndef BUDDYCLOUDCOMMUNITIESCONTAINER_H_
#define BUDDYCLOUDCOMMUNITIESCONTAINER_H_

// INCLUDES
#include <aknlists.h>
#include <Buddycloud.rsg>
#include "Buddycloud.hrh"
#include "BuddycloudLogic.h"

class CBuddycloudCommunitiesContainer : public CCoeControl {
	public:
		CBuddycloudCommunitiesContainer(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect);
		~CBuddycloudCommunitiesContainer();

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);
		
	private:
		void CreateListItemsL();
		void HandleItemSelectionL(TInt aIndex);
		
	public: // From CCoeControl
		CCoeControl* ComponentControl(TInt aIndex) const;
		TInt CountComponentControls() const;
		
	private: // From CCoeControl
		void SizeChanged();
		void HandleResourceChange(TInt aType);
  		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
		
	 private:
		CAknSingleLargeStyleListBox* iList;
		
		CBuddycloudLogic* iBuddycloudLogic;
};

#endif /*BUDDYCLOUDCOMMUNITIESCONTAINER_H_*/
