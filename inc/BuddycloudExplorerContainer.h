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
#include "ViewReference.h"
#include "XmppInterfaces.h"

#ifdef __SERIES60_40__
#include <akntoolbar.h>
#endif

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
		void ConstructL(const TRect& aRect, TViewReference aQueryReference);
        ~CBuddycloudExplorerContainer();

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);
			
	private: // Xmpp handling
		void ParseAndSendXmppStanzasL(const TDesC8& aStanza);
		
	private: // Level query/result management
		void PushLevelL(const TDesC& aTitle, const TDesC8& aStanza);
		void PopLevelL();
		void RefreshLevelL();
		
	private: // Request next result set management results
		void RequestMoreResultsL();

	private: // From CBuddycloudListComponent
		void RenderWrappedText(TInt aIndex);
		TInt CalculateItemSize(TInt aIndex);
		void RenderListItems();
		void RepositionItems(TBool aSnapToItem);
		void HandleItemSelection(TInt aItemId);

#ifdef __SERIES60_40__
	public:
		void DynInitToolbarL(TInt aResourceId, CAknToolbar* aToolbar);
#endif

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
		void HandleCommandL(TInt aCommand);

	protected: // From CCoeControl
		void SizeChanged();
 		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
  		
	public: // From MAknTabObserver
		void TabChangedL(TInt aIndex);
		
    public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);

	protected: // Variables
		MXmppWriteInterface* iXmppInterface;
		
		TViewReference iQueryReference;
		
		// Tabs
		CAknNavigationControlContainer* iNaviPane;
		CAknNavigationDecorator* iNaviDecorator;
		CAknTabGroup* iTabGroup;
		
		// Query/result management
		TExplorerState iExplorerState;
		
		TInt iQueryResultSize;
		TInt iQueryResultCount;	
		
		RPointerArray<CExplorerQueryLevel> iExplorerLevels;
};

#endif /*BUDDYCLOUDEXPLORERCONTAINER_H_*/
