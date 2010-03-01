/*
============================================================================
 Name        : 	EZStreamDecompressor.cpp
 Author      : 	Ross Savage
 Copyright   : 	2007 Buddycloud
 Description : 	Compression Engine for inflating streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "EZStreamDecompressor.h"

CEZStreamDecompressor* CEZStreamDecompressor::NewL() {
	CEZStreamDecompressor* self = NewLC();
	CleanupStack::Pop(self);
	return self;

}

CEZStreamDecompressor* CEZStreamDecompressor::NewLC() {
	CEZStreamDecompressor* self = new (ELeave) CEZStreamDecompressor();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CEZStreamDecompressor::~CEZStreamDecompressor() {
	inflateEnd(&iStream);
}

void CEZStreamDecompressor::ConstructL() {
	inflateInit(&iStream);
}

void CEZStreamDecompressor::ResetL() {
	inflateReset(&iStream);
}

TInt CEZStreamDecompressor::InflateL(TInt aFlush) {
	return inflate(&iStream, aFlush);
}
