/*
============================================================================
 Name        : 	CompressionEngine.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Compression Engine for inflating & deflating zlib streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "CompressionEngine.h"

CCompressionEngine* CCompressionEngine::NewL(MCompressionObserver* aObserver) {
	CCompressionEngine* self = NewLC(aObserver);
	CleanupStack::Pop(self);
	return self;

}

CCompressionEngine* CCompressionEngine::NewLC(MCompressionObserver* aObserver) {
	CCompressionEngine* self = new (ELeave) CCompressionEngine();
	CleanupStack::PushL(self);
	self->ConstructL(aObserver);
	return self;
}

CCompressionEngine::~CCompressionEngine() {
	if(iCompressor)
		delete iCompressor;

	if(iDecompressor)
		delete iDecompressor;
}

CCompressionEngine::CCompressionEngine() {
}

void CCompressionEngine::ConstructL(MCompressionObserver* aObserver) {
	iObserver = aObserver;

	iCompressor = CEZStreamCompressor::NewL();
	iDecompressor = CEZStreamDecompressor::NewL();
}

void CCompressionEngine::ResetL() {
	iOutgoingRaw = 0;
	iOutgoingZip = 0;
	iIncomingRaw = 0;
	iIncomingZip = 0;

	iCompressor->ResetL();
	iDecompressor->ResetL();
}

void CCompressionEngine::DeflateL(const TDesC8& aData) {
	TInt aFlush = Z_NO_FLUSH;
	TPtrC8 pDataBuffer(aData);
	
	iOutgoingRaw += pDataBuffer.Length(); 
	
	TBuf8<256> aOutput;
	TInt aAvailableOutput = aOutput.MaxLength();	
	iCompressor->SetOutput(aOutput);	
		
	do {
		TPtrC8 pChunk(pDataBuffer);
		
		if( iCompressor->AvailIn() == 0 ) {
			// Get Next Chunk to process
			if(pDataBuffer.Length() > 32) {
				pChunk.Set(pChunk.Left(32));
			}
			else {
				aFlush = Z_SYNC_FLUSH;
			}
			
			// Set Input
			if(pChunk.Length() > 0) {
				pDataBuffer.Set(pDataBuffer.Mid(pChunk.Length()));
				iCompressor->SetInput(pChunk);
			}
		}
		
		do {
			// Compress
			iCompressor->DeflateL(aFlush);
			
			aAvailableOutput = iCompressor->AvailOut();
			
			if( aAvailableOutput < aOutput.MaxLength() ) {
				// Copy output
				TPtrC8 pOutput = iCompressor->OutputDescriptor();
				iOutgoingZip += pOutput.Length(); 
				
				iObserver->DataDeflated(pOutput);
				
				// Set Output
				aOutput.Zero();
				iCompressor->SetOutput(aOutput);
			}
		} while( aAvailableOutput == 0 );

	} while( aFlush == Z_NO_FLUSH );
}

void CCompressionEngine::InflateL(const TDesC8& aData) {
	iIncomingZip += aData.Length(); 

	TBuf8<256> aOutput;
	TInt aAvailableOutput = aOutput.MaxLength();

	iDecompressor->SetInput(aData);
	iDecompressor->SetOutput(aOutput);	

	do {
		// Inflate
		iDecompressor->InflateL(Z_FULL_FLUSH);
		
		aAvailableOutput = iDecompressor->AvailOut();
		
		if( aAvailableOutput < aOutput.MaxLength() ) {
			// Copy output to stream
			TPtrC8 pOutput = iDecompressor->OutputDescriptor();
			iIncomingRaw += pOutput.Length(); 
		
			iObserver->DataInflated(pOutput);
			
			if( aAvailableOutput == 0 ) {
				// Set Output
				aOutput.Zero();
				iDecompressor->SetOutput(aOutput);
			}
		}
	} while( aAvailableOutput == 0 );
}

void CCompressionEngine::WriteDebugL() {
#ifdef _DEBUG
	if(iOutgoingRaw > 0 && iIncomingRaw > 0) {
		iObserver->CompressionDebug(_L8("CE    ------------------ Compression Statistics -------------------"));
		
		TReal aOutgoing = (100.0 / iOutgoingRaw)*iOutgoingZip;
		TReal aIncoming = (100.0 / iIncomingRaw)*iIncomingZip;
		
		TBuf8<256> aMessage;
		aMessage.Format(_L8("CE    - Outgoing: %d bytes, compressed %d bytes (%2.1f%%)"), iOutgoingRaw, iOutgoingZip, aOutgoing);
		iObserver->CompressionDebug(aMessage);
		aMessage.Format(_L8("CE    - Incoming: %d bytes, decompressed %d bytes (%2.1f%%)"), iIncomingZip, iIncomingRaw, aIncoming);
		iObserver->CompressionDebug(aMessage);
		iObserver->CompressionDebug(_L8("CE    -------------------------------------------------------------"));
	}
#endif
}
