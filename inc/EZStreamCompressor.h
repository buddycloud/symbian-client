/*
============================================================================
 Name        : 	EZStreamCompressor.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Compression Engine for deflating streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef EZSTREAMCOMPRESSOR_H_
#define EZSTREAMCOMPRESSOR_H_

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

class CEZStreamCompressor : public CEZZStream {
	public:
		static CEZStreamCompressor* NewL();
		static CEZStreamCompressor* NewLC();
		~CEZStreamCompressor();

	private:
		void ConstructL();

	public:
		void ResetL();
		TInt DeflateL(TInt aFlush);
};

#endif /*EZSTREAMCOMPRESSOR_H_*/
