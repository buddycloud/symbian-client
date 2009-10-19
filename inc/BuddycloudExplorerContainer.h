/*
============================================================================
 Name        : BuddycloudExplorerContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Explorer Container
============================================================================
*/

#ifndef BUDDYCLOUDEXPLORERCONTAINER_H_
#define BUDDYCLOUDEXPLORERCONTAINER_H_

// INCLUDES
#include <aknnavi.h>
#include <akntabgrp.h>
#include <akntabobserver.h>
#include "BuddycloudListComponent.h"
#include "BuddycloudExplorer.h"
#include "XmppObservers.h"

/*
----------------------------------------------------------------------------
--
-- CBuddycloudExplorerContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudExplorerContainer : public CBuddycloudListComponent, public MAknTabObserver, MXmppStanzaObserver {
	
	public: // Constructors and destructor
		CBuddycloudExplorerContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TExplorerQuery aQuery);
        ~CBuddycloudExplorerContainer();
        
	public:
		void SetPrevView(const TVwsViewId& aViewId, TUid aViewMsgId);

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);
			
	private: // Xmpp handling
		void ParseAndSendXmppStanzasL(const TDesC8& aStanza);
		
	private: // Level query/result management
		void PushLevelL(const TDesC& aTitle, const TDesC8& aStanza);
		void PopLevelL();
		void RefreshLevelL();

	private: // From CBuddycloudListComponent
		void RenderWrappedText(TInt aIndex);
		TInt CalculateItemSize(TInt aIndex);
		void RenderListItems();
		void RepositionItems(TBool aSnapToItem);
		void HandleItemSelection(TInt aItemId);

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;		

	protected: // From CCoeControl
		void SizeChanged();
 		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
  		
	public: // From MAknTabObserver
		void TabChangedL(TInt aIndex);
		
    public: // From MXmppStanzaObserver
		void XmppStanzaNotificationL(const TDesC8& aStanza, const TDesC8& aId);

	protected: // Variables
		// Tabs
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
		CAknTabGroup* iTabGroup;
		
		// Strings
		HBufC* iLocalizedRank;
		
		// Previous view
		TVwsViewId iPrevViewId;
		TUid iPrevViewMsgId;
		
		// Query/result management
		TExplorerState iExplorerState;
		
		TInt iQueryResultSize;
		TInt iQueryResultCount;	
		
		RPointerArray<CExplorerQueryLevel> iExplorerLevels;
};

#endif /*BUDDYCLOUDEXPLORERCONTAINER_H_*/
