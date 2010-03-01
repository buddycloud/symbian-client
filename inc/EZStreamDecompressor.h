/*
============================================================================
 Name        : 	EZStreamDecompressor.h
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Compression Engine for inflating streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef EZSTREAMDECOMPRESSOR_H_
#define EZSTREAMDECOMPRESSOR_H_

// INCLUDES
#include <e32base.h>
#include <EZStream.h>
#include <EZLib.h>

/*
----------------------------------------------------------------------------
--
-- CCompressionEngine
--
----------------------------------------------------------------------------
*/

class CEZStreamDecompressor : public CEZZStream {
	public:
		static CEZStreamDecompressor* NewL();
		static CEZStreamDecompressor* NewLC();
		~CEZStreamDecompressor();

	private:
		void ConstructL();

	public:
		void ResetL();
		TInt InflateL(TInt aFlush);
};

#endif /*EZSTREAMDECOMPRESSOR_H_*/
