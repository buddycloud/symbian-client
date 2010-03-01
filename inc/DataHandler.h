/*
============================================================================
 Name        : 	DataHandler.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Handle data for a Resource
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef DATAHANDLER_H_
#define DATAHANDLER_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- CDataHandler
--
----------------------------------------------------------------------------
*/

class CDataHandler : public CActive {
	public:
		CDataHandler();
	
	public:
		void SetWaiting(TBool aWaiting);
		TBool IsWaiting();

	public:
		virtual void StartL() = 0;
		virtual void Stop() = 0;
		virtual TBool IsConnected() = 0;

	public: // From CActive
		virtual void RunL() = 0;
		virtual TInt RunError(TInt aError) = 0;
		virtual void DoCancel() = 0;

	protected:
		TBool iWaiting;

};

#endif /*DATAHANDLER_H_*/
