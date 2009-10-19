/*
============================================================================
 Name        : BuddycloudChannelsContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Channels Container
============================================================================
*/

#ifndef BUDDYCLOUDCHANNELSCONTAINER_H_
#define BUDDYCLOUDCHANNELSCONTAINER_H_

// INCLUDES
#include <akntabobserver.h> 
#include "BuddycloudExplorerContainer.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudChannelsContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudChannelsContainer : public CBuddycloudExplorerContainer {	
	public: // Constructors and destructor
		CBuddycloudChannelsContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect);
		
	private: // 
		void CreateDirectoryItemL(const TDesC& aId, TInt aTitleResource);
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;		

	private: // From CCoeControl
 		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
};

#endif /*BUDDYCLOUDCHANNELSCONTAINER_H_*/
