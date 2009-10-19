/*
============================================================================
 Name        : 	SearchResultMessageQueryDialog.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Message Query Dialog for Search Results
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#import "SearchResultMessageQueryDialog.h"
#include <Buddycloud_lang.rsg>
#include "Buddycloud.hrh"

void CSearchResultMessageQueryDialog::HasPreviousResult(TBool aValue) {
	iHasPrevious = aValue;
}

void CSearchResultMessageQueryDialog::HasNextResult(TBool aValue) {
	iHasNext = aValue;
}

void CSearchResultMessageQueryDialog::DynInitMenuPaneL(TInt aResourceId, CEikMenuPane *aMenuPane) {
	if(aResourceId == R_SEARCH_OPTIONS_MENU) {
		if( !iHasPrevious ) {
			aMenuPane->SetItemDimmed(EMenuPreviousCommand, true);
		}
	}
}

void CSearchResultMessageQueryDialog::ProcessCommandL(TInt aCommandId) {
	CAknDialog::HideMenu();
	TryExitL(aCommandId);
}

void CSearchResultMessageQueryDialog::PostLayoutDynInitL() {
	iMenuBar->SetMenuTitleResourceId(R_SEARCH_OPTIONS_MENUBAR);
	
	if( !iHasNext ) {
		CEikButtonGroupContainer& aCba = ButtonGroupContainer();
		aCba.SetCommandSetL(R_AVKON_SOFTKEYS_OPTIONS_CANCEL);
	}
}

TBool CSearchResultMessageQueryDialog::OkToExitL(TInt aButtonId) {
	if(aButtonId == EAknSoftkeyOptions) {
		CAknDialog::DisplayMenuL();
		return false;
	}
	
	return true;
}
