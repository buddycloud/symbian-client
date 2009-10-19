/*
============================================================================
 Name        : 	TimeCoder.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Encode and Decode formatted time
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef TIMECODER_H_
#define TIMECODER_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- CTimeCoder
--
----------------------------------------------------------------------------
*/

typedef TBuf8<20> TFormattedTimeDesc;

class CTimeCoder : public CBase {
	public:
		static CTimeCoder* NewL();
		static CTimeCoder* NewLC();
		~CTimeCoder();

	private:
		CTimeCoder();
		void ConstructL();

	public:
		static TTime DecodeL(const TDesC8& aFormattedTime);
		static void EncodeL(TTime aTime, TFormattedTimeDesc& aFormatedTime);
};

#endif /*CHARACTERCODER_H_*/
