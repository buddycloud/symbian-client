/*
============================================================================
 Name        : 	SearchResultMessageQueryDialog.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Message Query Dialog for Search Results
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef SEARCHRESULTMESSAGEQUERYDIALOG_H_
#define SEARCHRESULTMESSAGEQUERYDIALOG_H_

#include <aknmessagequerydialog.h>

class CSearchResultMessageQueryDialog : public CAknMessageQueryDialog {
	public:
		void HasPreviousResult(TBool aValue);
		void HasNextResult(TBool aValue);
		
	public: // From CAknMessageQueryDialog
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane *aMenuPane);
		void ProcessCommandL(TInt aCommandId);
		
	protected: // From CAknMessageQueryDialog
		void PostLayoutDynInitL();
		TBool OkToExitL(TInt aButtonId);
		
	protected:
		TBool iHasPrevious;
		TBool iHasNext;
};

#endif /*SEARCHRESULTMESSAGEQUERYDIALOG_H_*/
