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

#include <aknutils.h>
#include <bacline.h>
#include "FileUtilities.h"

TInt CFileUtilities::CompleteWithApplicationPath(RFs aSession, TFileName& aFileName) {
	TInt aResult = KErrNone;
	
#ifndef __SERIES60_3X__
	aResult = CompleteWithAppPath(aFileName);
#else
	TFileName aCopyFileName(aFileName);
	
	if((aResult = aSession.PrivatePath(aFileName)) == KErrNone) {
		aFileName.Insert(0, _L(":"));
		
#ifndef __WINSCW__		
		CCommandLineArguments* aArguments = CCommandLineArguments::NewLC();
		TFileName drive;
		
		if (aArguments->Count() > 0) {
			drive.Append(aArguments->Arg(0)[0]);
			aFileName.Insert(0, drive);
		}
		
		CleanupStack::PopAndDestroy(); // aArguments
#else
		aFileName.Insert(0, _L("c"));
#endif

		aFileName.Append(aCopyFileName);
	}
#endif
	
	return aResult;
}

void CFileUtilities::CompleteWithDrive(RFs aSession, TFileName& aFileName) {
	if(aFileName.Length() > 0 && aFileName[0] != TInt(':')) {
		aFileName.Insert(0, _L(":"));
	}
	
#ifndef __WINSCW__
	CCommandLineArguments* aArguments = CCommandLineArguments::NewLC();
		
	TFileName aDrive;
		
	if(aArguments->Count() > 0) {
		aDrive.Append(aArguments->Arg(0)[0]);
		aFileName.Insert(0, aDrive);
	}
		
	CleanupStack::PopAndDestroy();
#else
	aFileName.Insert(0, _L("z"));
#endif
}
