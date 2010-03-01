/*
============================================================================
 Name        : 	TimeUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Encode and Decode formatted time
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef TIMEUTILITIES_H_
#define TIMEUTILITIES_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- Time Interfaces
--
----------------------------------------------------------------------------
*/

class MTimeInterface {
	public:
		virtual TTime GetTime() = 0;
};

/*
----------------------------------------------------------------------------
--
-- CTimeUtilities
--
----------------------------------------------------------------------------
*/

typedef TBuf8<20> TFormattedTimeDesc;

class CTimeUtilities : public CBase {
	public:
		static CTimeUtilities* NewL();
		static CTimeUtilities* NewLC();

	public:
		static TTime DecodeL(const TDesC8& aFormattedTime);
		static void EncodeL(TTime aTime, TFormattedTimeDesc& aFormatedTime);
};

#endif /*TIMEUTILITIES_H_*/
