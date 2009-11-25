/*
============================================================================
 Name        : 	CompressionEngine.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Compression Engine for inflating & deflating zlib streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef COMPRESSIONENGINE_H_
#define COMPRESSIONENGINE_H_

// INCLUDES
#include <e32base.h>
#include <EZLib.h>
#include "EZStreamDecompressor.h"
#include "EZStreamCompressor.h"

class MCompressionObserver {
	public:
		virtual void DataDeflated(const TDesC8& aDeflatedData) = 0;
		virtual void DataInflated(const TDesC8& aInflatedData) = 0;
		virtual void CompressionDebug(const TDesC8& aDebug) = 0;
};

/*
----------------------------------------------------------------------------
--
-- CCompressionEngine
--
----------------------------------------------------------------------------
*/

class CCompressionEngine : public CBase {
	public:
		static CCompressionEngine* NewL(MCompressionObserver* aObserver);
		static CCompressionEngine* NewLC(MCompressionObserver* aObserver);
		~CCompressionEngine();

	private:
		CCompressionEngine();
		void ConstructL(MCompressionObserver* aObserver);

	public:
		void ResetL();
			
		void DeflateL(const TDesC8& aData);
		void InflateL(const TDesC8& aData);
		
	public:
		void WriteDebugL();

	private:
		MCompressionObserver* iObserver;

		CEZStreamCompressor* iCompressor;
		CEZStreamDecompressor* iDecompressor;
		
		TInt iOutgoingRaw;
		TInt iOutgoingZip;
		TInt iIncomingRaw;
		TInt iIncomingZip;
};

#endif // COMPRESSIONENGINE_H_
