/*
============================================================================
 Name        : 	AutoSelectTextQueryDialog.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Text Query Dialog for Auto Selection of existing values
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "AutoSelectTextQueryDialog.h"

/*
----------------------------------------------------------------------------
--
-- CAutoSelectItem
--
----------------------------------------------------------------------------
*/

CAutoSelectItem::CAutoSelectItem(TInt aId, TDesC& aText) {
	iId = aId;
	iText = aText.AllocL();
}

CAutoSelectItem::~CAutoSelectItem() {
	if(iText)
		delete iText;
}

TInt CAutoSelectItem::GetId() {
	return iId;
}

TDesC& CAutoSelectItem::GetText() {
	return *iText;
}

/*
----------------------------------------------------------------------------
--
-- CAutoSelectTextQueryDialog
--
----------------------------------------------------------------------------
*/

CAutoSelectTextQueryDialog::CAutoSelectTextQueryDialog(TInt& aSelectedId, TDes& aText) : CAknTextQueryDialog(aText) {
	iAutoSelectedId = &aSelectedId;
	*iAutoSelectedId = KErrNotFound;

	iTimer = CCustomTimer::NewL(this);
}

CAutoSelectTextQueryDialog::~CAutoSelectTextQueryDialog() {
	if(iTimer)
		delete iTimer;

	for(TInt i = 0;i < iAutoSelectList.Count();i++) {
		delete iAutoSelectList[i];
	}

	iAutoSelectList.Close();
}

void CAutoSelectTextQueryDialog::AddAutoSelectTextL(TInt aId, TDesC& aText) {
	CAutoSelectItem* aItem = new (ELeave) CAutoSelectItem(aId, aText);

	iAutoSelectList.Append(aItem);
}

TKeyResponse CAutoSelectTextQueryDialog::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType) {
	TKeyResponse aResult = EKeyWasNotConsumed;

	iTimer->Stop();

	if(aType == EEventKey) {
		if(aKeyEvent.iCode == EKeyUpArrow) {
			aResult = EKeyWasConsumed;
		}
		else if(aKeyEvent.iCode == EKeyDownArrow) {
			aResult = EKeyWasConsumed;
		}
	}

	if(aResult == EKeyWasNotConsumed) {
		if(aKeyEvent.iScanCode != EStdKeyLeftArrow && aKeyEvent.iScanCode != EStdKeyRightArrow &&
				aKeyEvent.iScanCode != 164 && aKeyEvent.iScanCode != 165 && aKeyEvent.iScanCode != EStdKeyBackspace) {

			CAknQueryControl* aQueryControl = QueryControl();

			if(aQueryControl != NULL) {
				CEikEdwin* aControlEdwin = (CEikEdwin*)aQueryControl->ControlByLayoutOrNull(EDataLayout);

				if(aControlEdwin != NULL) {
					if(aControlEdwin->SelectionLength() > 0) {
						HBufC* aControlText = HBufC::NewLC(aControlEdwin->TextLength());
						TPtr pControlText(aControlText->Des());

						aControlEdwin->GetText(pControlText);
						pControlText.Delete(aControlEdwin->CursorPos(), pControlText.Length());
						aControlEdwin->SetTextL(aControlText);
						aControlEdwin->HandleTextChangedL();

						CleanupStack::PopAndDestroy(); // aControlText

						aControlEdwin->ClearSelectionL();
						aControlEdwin->HandleTextChangedL();
					}
				}
			}

			iTimer->After(1000000);
		}

		CAknTextQueryDialog::OfferKeyEventL(aKeyEvent, aType);
	}

	return EKeyWasConsumed;
}

TBool CAutoSelectTextQueryDialog::OkToExitL(TInt /*aButtonId*/) {
	CAknQueryControl* aQueryControl = QueryControl();

	if(aQueryControl != NULL) {
		CEikEdwin* aControlEdwin = (CEikEdwin*)aQueryControl->ControlByLayoutOrNull(EDataLayout);

		if(aControlEdwin != NULL) {
			TInt aTextLength = aControlEdwin->TextLength();

			if(aTextLength > 0) {
				HBufC* aControlText = HBufC::NewLC(aTextLength);
				TPtr pControlText(aControlText->Des());
				TInt aFindCount = 0;

				aControlEdwin->GetText(pControlText);

				for(TInt i = 0;i < iAutoSelectList.Count();i++) {					
					if(pControlText.FindF(iAutoSelectList[i]->GetText()) == 0) {
						*iAutoSelectedId = iAutoSelectList[i]->GetId();
						aFindCount++;
					}
				}
				
				if(aFindCount > 1) {
					*iAutoSelectedId = KErrNotFound;
				}
				
				aControlEdwin->GetText(iDataText);
				
				CleanupStack::PopAndDestroy(); // aControlText
			}
		}
	}

	return true;
}

void CAutoSelectTextQueryDialog::TimerExpired(TInt /*aExpiryId*/) {
	CAknQueryControl* aQueryControl = QueryControl();

	if(aQueryControl != NULL) {
		CEikEdwin* aControlEdwin = (CEikEdwin*)aQueryControl->ControlByLayoutOrNull(EDataLayout);

		if(aControlEdwin != NULL) {
			TInt aTextLength = aControlEdwin->TextLength();

			if(aTextLength > 0) {
				HBufC* aControlText = HBufC::NewLC(aTextLength);
				TPtr pControlText(aControlText->Des());

				aControlEdwin->GetText(pControlText);
				pControlText.LowerCase();

				for(TInt i = 0;i < iAutoSelectList.Count();i++) {
					HBufC* aCompareText = iAutoSelectList[i]->GetText().AllocLC();
					TPtr pCompareText(aCompareText->Des());
					pCompareText.LowerCase();

					if(pCompareText.Length() > aTextLength) {
						pCompareText.Delete(aTextLength, pCompareText.Length());
					}

					if(pControlText.Compare(pCompareText) == 0) {
						aControlEdwin->SetTextL(iAutoSelectList[i]->iText);
						aControlEdwin->HandleTextChangedL();
						aControlEdwin->SetSelectionL(aTextLength, iAutoSelectList[i]->GetText().Length());

						CleanupStack::PopAndDestroy(); // aCompareText
						break;
					}
					else {
						CleanupStack::PopAndDestroy(); // aCompareText
					}
				}

				CleanupStack::PopAndDestroy(); // aControlText
			}
		}
	}
}
