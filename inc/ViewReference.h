/*
============================================================================
 Name        : ViewReference.h
 Author      : Ross Savage
 Copyright   : 2010 Buddycloud
 Description : Packaging buffer for view-to-view referencing
============================================================================
*/

#ifndef VIEWREFERENCE_H_
#define VIEWREFERENCE_H_

#include <e32cmn.h>

class TViewData {
	public:
		TInt iId;
		TBuf<128> iTitle;
		TBuf8<512> iData;
};
	
class TViewReference {
	public:
		TViewReference() {
			iCallbackRequested = false;
			iCallbackViewId.iUid = 0;
			iOldViewData.iId = 0;
			iNewViewData.iId = 0;
		}
	
	public: // Deactivated view (caller reference)
		TBool iCallbackRequested;
		
	public:
		TUid iCallbackViewId;
		TViewData iOldViewData;
		
	public: // Activated view (callee reference)
		TViewData iNewViewData;
};

typedef TPckgBuf<TViewReference> TViewReferenceBuf;

#endif /* VIEWREFERENCE_H_ */
