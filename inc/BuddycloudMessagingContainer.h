/*
============================================================================
 Name        : BuddycloudMessagingContainer.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Declares Messaging Container
============================================================================
*/

#ifndef BUDDYCLOUDMESSAGINGCONTAINER_H_
#define BUDDYCLOUDMESSAGINGCONTAINER_H_

// INCLUDES
#include "BuddycloudListComponent.h"
#include "MessagingParticipants.h"

enum TMoveDirection {
	EMoveNone, EMoveUp, EMoveDown
};

/*
----------------------------------------------------------------------------
--
-- TMessagingViewObject
--
----------------------------------------------------------------------------
*/
	
class TMessagingViewObject {
	public:
		TInt iFollowerId;
		TBuf<128> iTitle;
		TBuf<128> iJid;
};

typedef TPckg<TMessagingViewObject> TMessagingViewObjectPckg;

/*
----------------------------------------------------------------------------
--
-- CFormattedBody
--
----------------------------------------------------------------------------
*/

class CFormattedBody : public CBase {
	public:
		CFormattedBody();
		~CFormattedBody();

	public:
		RPointerArray<HBufC> iLines;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudMessagingContainer
--
----------------------------------------------------------------------------
*/

class CBuddycloudMessagingContainer : public CBuddycloudListComponent, MDiscussionMessagingObserver {
	public: // Constructors and destructor
		CBuddycloudMessagingContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TMessagingViewObject aObject);
        ~CBuddycloudMessagingContainer();
        
	private:
		void InitializeMessageDataL();

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From MDiscussionMessagingObserver
		void TopicChanged(TDesC& aTopic);
		void MessageDeleted(TInt aPosition);
		void MessageReceived(CMessage* aMessageItem, TInt aPosition);

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	private:
		void ComposeNewMessageL(const TDesC& aPretext, CMessage* aReferenceMessage = NULL);
		TBool OpenMessageLinkL();
		void FormatWordWrap(CMessage* aMessage, TInt aPosition);
		void RenderSelectedText(TInt& aStartPos);
#ifdef __SERIES60_40__
		void ResetItemLinks();
#endif
	
	private:
		TInt CalculateMessageSize(TInt aIndex, TBool aSelected);
		void RenderScreen();
	
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
		
	public: // From CCoeControl
		void GetHelpContext(TCoeHelpContext& aContext) const;
		
	private: // From CCoeControl
		void SizeChanged();
 		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

#ifdef __SERIES60_40__
	private:
 		void HandlePointerEventL(const TPointerEvent &aPointerEvent);
#endif

	private: // Variables
		TBool iRendering;

		TMessagingViewObject iMessagingObject;
 		
		TBool iIsPersonalChannel;
 		TBool iIsChannel;
 		
 		TBool iJumpToUnreadMessage;
 		
		// Messages
		RPointerArray<CFormattedBody> iMessages;
		CDiscussion* iDiscussion;

		TInt iSelectedLink;		
		TRect iSelectedItemBox;
		
#ifdef __SERIES60_40__
		RArray<RRegion> iItemLinks;
#endif
};

#endif /*BUDDYCLOUDMESSAGINGCONTAINER_H_*/

// End of File
