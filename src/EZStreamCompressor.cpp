/*
============================================================================
 Name        : 	EZStreamCompressor.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2007
 Description : 	Compression Engine for deflating streams
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include "EZStreamCompressor.h"

CEZZStream::CEZZStream() :iOutputPointer(NULL), iOutputBufferLength(0) {
}

CEZStreamCompressor* CEZStreamCompressor::NewL() {
	CEZStreamCompressor* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CEZStreamCompressor* CEZStreamCompressor::NewLC() {
	CEZStreamCompressor* self = new (ELeave) CEZStreamCompressor();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CEZStreamCompressor::~CEZStreamCompressor() {
	deflateEnd(&iStream);
}

void CEZStreamCompressor::ConstructL() {
	deflateInit(&iStream, Z_DEFAULT_COMPRESSION);
}

void CEZStreamCompressor::ResetL() {
	deflateReset(&iStream);
}

TInt CEZStreamCompressor::DeflateL(TInt aFlush) {
	return deflate(&iStream, aFlush);
}
