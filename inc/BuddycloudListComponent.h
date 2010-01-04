/*
============================================================================
 Name        : BuddycloudListComponent.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2008
 Description : Custom List Component
============================================================================
*/

#ifndef BUDDYCLOUDLISTCOMPONENT_H_
#define BUDDYCLOUDLISTCOMPONENT_H_

// INCLUDES
#include <AknsBasicBackgroundControlContext.h>
#include <coecntrl.h>
#include <fbs.h>
#include <bitdev.h>
#include <Buddycloud.rsg>
#include "Buddycloud.hrh"
#include "Buddycloud.pan"
#include "AvatarRepository.h"
#include "BuddycloudLogic.h"
#include "BuddycloudConstants.h"
#include "TextUtilities.h"
#include "Timer.h"
#include "ViewAccessorObserver.h"

#ifdef __SERIES60_40__
#include <touchfeedback.h>
#endif

/*
----------------------------------------------------------------------------
--
-- CBuddycloudListComponent
--
----------------------------------------------------------------------------
*/
const TInt KTimeTimerId = 0;
const TInt KDragTimerId = 1;

class CBuddycloudListComponent : public CCoeControl, MBuddycloudLogicNotificationObserver, 
		MTimeoutNotification, MEikScrollBarObserver {
	
	public: // Constructors and destructor
		CBuddycloudListComponent(MViewAccessorObserver* aViewAccessor, CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL();
       ~CBuddycloudListComponent();

	public:
		void SetTitleL(const TDesC& aTitle);
		
	protected:
		void ConfigureFonts();
		void ReleaseFonts();
		void ConfigureSkin();

	public: // From MBuddycloudLogicNotificationObserver
		void NotificationEvent(TBuddycloudLogicNotificationType aEvent, TInt aId = KErrNotFound);

	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);
		
	protected:
		virtual void RenderWrappedText(TInt aIndex) = 0;
		void ClearWrappedText();

	protected:
		virtual TInt CalculateItemSize(TInt aIndex) = 0;
		void RenderItemFrame(TRect aFrame);		
		void RenderScreen();
	
	protected:
		virtual void RenderListItems() = 0;
		void RepositionItems(TBool aSnapToItem);
		
	protected:
		virtual void HandleItemSelection(TInt aItemId) = 0;

	protected:
		TTypeUid::Ptr MopSupplyObject(TTypeUid aId);
		void HandleResourceChange(TInt aType);
		void SizeChanged();

	private: // From CCoeControl
		void Draw(const TRect& aRect) const;		
  		
	public: // From MEikScrollBarObserver
		void HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType);
		
#ifdef __SERIES60_40__
	protected:
		void HandlePointerEventL(const TPointerEvent &aPointerEvent);
#endif

	protected: // Variables
		// View
		MViewAccessorObserver* iViewAccessor;

		// Background
		CAknsBasicBackgroundControlContext* iBgContext;

		TRect iRect;

		// Double Buffer
		CFbsBitmap* iBufferBitmap;
		CFbsBitmapDevice* iBufferDevice;
		CFbsBitGc* iBufferGc;

		// Fonts
 		CFont* i13BoldFont;
 		CFont* i10ItalicFont;
 		CFont* i10NormalFont;
 		CFont* i10BoldFont;
		
 		// Wrapped text
		RPointerArray<HBufC> iWrappedTextArray;
		
		CTextUtilities* iTextUtilities;
		
 		// Offsets
 		TInt iSelectedItemIconTextOffset;
 		TInt iUnselectedItemIconTextOffset;
 		TInt iItemIconSize;
 		TMidmapLevel iIconMidmapSize;

 		CBuddycloudLogic* iBuddycloudLogic;
		CAvatarRepository* iAvatarRepository;

		// Colours
		TRgb iColourHighlight;
		TRgb iColourHighlightBorder;
		TRgb iColourText;
 		TRgb iColourTextLink;
		TRgb iColourTextSelected;
		
		TRgb iColourHighlightTrans;
		TRgb iColourHighlightBorderTrans;
		TRgb iColourTextTrans;
		TRgb iColourTextSelectedTrans;

		// Current selected(highlighted) item
		TInt iSelectedItem;
		
		// Snap to same item on events
		TBool iSnapToItem;

		// Timer
		CCustomTimer* iTimer;
		
		// Scrollbar data
		CEikScrollBarFrame* iScrollBar;
		TAknDoubleSpanScrollBarModel iScrollBarVModel;
		TAknDoubleSpanScrollBarModel iScrollBarHModel;
		
		TInt iLeftBarSpacer;
		TInt iRightBarSpacer;
		TBool iLayoutMirrored;
		
		TInt iTotalListSize;
		TInt iScrollbarHandlePosition;
		
		// Mialog setting
		TBool iShowMialog;
		
#ifdef __SERIES60_40__
		TBool iDraggingAllowed;
		TReal iDragVelocity;
		TTime iLastDragTime;
		TInt iLastDragPosition;
		TInt iStartDragPosition;
		TInt iStartDragHandlePosition;
		CCustomTimer* iDragTimer;
		
		MTouchFeedback* iTouchFeedback;
		
		// List item and array
		class TListItem {
			public:
				inline TListItem(TInt aId, TRect aRect) {
					iId = aId;
					iRect = aRect;
				}
				
			public: // Variables
				TInt iId;
				TRect iRect;
		};
		
		RArray<TListItem> iListItems;
#endif
};

#endif /*BUDDYCLOUDLISTCOMPONENT_H_*/
