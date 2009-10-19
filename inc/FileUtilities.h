/*
============================================================================
 Name        : 	FileUtilities.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	File based utilities
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef FILEUTILITIES_H_
#define FILEUTILITIES_H_

#include <e32cmn.h>
#include <f32file.h>

class CFileUtilities {
	public:
		static TInt CompleteWithApplicationPath(RFs aSession, TFileName& aFileName);
		static void CompleteWithDrive(RFs aSession, TFileName& aFileName);
};

#endif /*FILEUTILITIES_H_*/
