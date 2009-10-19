/*
============================================================================
 Name        : 	AutoSelectTextQueryDialog.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Text Query Dialog for Auto Selection of existing values
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef AUTOSELECTTEXTQUERYDIALOG_H_
#define AUTOSELECTTEXTQUERYDIALOG_H_

#include <aknquerydialog.h>
#include "Timer.h"

/*
----------------------------------------------------------------------------
--
-- CAutoSelectItem
--
----------------------------------------------------------------------------
*/

class CAutoSelectItem : public CBase {
	public:
		CAutoSelectItem(TInt aId, TDesC& aText);
		~CAutoSelectItem();

	public:
		TInt GetId();
		TDesC& GetText();

	public:
		TInt iId;
		HBufC* iText;
};

/*
----------------------------------------------------------------------------
--
-- CAutoSelectTextQueryDialog
--
----------------------------------------------------------------------------
*/

class CAutoSelectTextQueryDialog : public CAknTextQueryDialog, MTimeoutNotification {
	public:
		CAutoSelectTextQueryDialog(TInt &aSelectedId, TDes& aText);

	private:
		~CAutoSelectTextQueryDialog();

	public: // Custom Functions
		void AddAutoSelectTextL(TInt aId, TDesC& aText);

	private: // From CAknTextQueryDialog
		TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
		TBool OkToExitL(TInt aButtonId);

	private: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);

	private:
		RArray<CAutoSelectItem*> iAutoSelectList;
		TInt* iAutoSelectedId;

		CCustomTimer* iTimer;
};

#endif /*AUTOSELECTTEXTQUERYDIALOG_H_*/
