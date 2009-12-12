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
#include "DiscussionManager.h"

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
		TBuf<256> iId;
};

typedef TPckg<TMessagingViewObject> TMessagingViewObjectPckg;

/*
----------------------------------------------------------------------------
--
-- CTextWrappedEntry
--
----------------------------------------------------------------------------
*/

class CTextWrappedEntry : public CBase {
	public:
		static CTextWrappedEntry* NewLC(CAtomEntryData* aEntry, TBool aComment);
	
	public:
		CTextWrappedEntry(CAtomEntryData* aEntry, TBool aComment);
		~CTextWrappedEntry();
		
	public:
		CAtomEntryData* GetEntry();
		TBool Comment();
		
		TBool Read();
		void SetRead(TBool aRead);
		
	public:
		TInt LineCount();
		TDesC& GetLine(TInt aIndex);
		void ResetLines();

	protected:
		CAtomEntryData* iEntry;
		TBool iComment;
		TBool iRead;
		
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

class CBuddycloudMessagingContainer : public CBuddycloudListComponent, MDiscussionUpdateObserver {
	public: // Constructors and destructor
		CBuddycloudMessagingContainer(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TMessagingViewObject aObject);
        ~CBuddycloudMessagingContainer();
        
	private:
		void InitializeMessageDataL();

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From MDiscussionUpdateObserver
		void EntryAdded(CAtomEntryData* aAtomEntry);
		void EntryDeleted(CAtomEntryData* aAtomEntry);

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	private:
		void ComposeNewCommentL(const TDesC& aContent);
		void ComposeNewPostL(const TDesC& aContent, const TDesC8& aReferenceId = KNullDesC8);
		TBool OpenPostedLinkL();
		
	private:
		void TextWrapEntry(TInt aIndex);
		
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

	protected: // Variables
		TMessagingViewObject iMessagingObject;
		TInt iFollowingItemIndex;
		
		// Flags
		TBool iRendering;
		TBool iIsChannel;		
 		TBool iJumpToUnreadPost;
 		
		// Messages
		RPointerArray<CTextWrappedEntry> iEntries;
		CDiscussion* iDiscussion;
		CFollowingItem* iItem;

		TInt iSelectedLink;		
		TRect iSelectedItemBox;
		
#ifdef __SERIES60_40__
		RArray<RRegion> iItemLinks;
#endif
};

#endif /*BUDDYCLOUDMESSAGINGCONTAINER_H_*/

// End of File
