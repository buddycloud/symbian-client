/*
============================================================================
 Name        : 	BrowserLauncher.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Get the platform version & launch web browser
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef BROWSERLAUNCHER_H_
#define BROWSERLAUNCHER_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- CBrowserLauncher
--
----------------------------------------------------------------------------
*/

class CBrowserLauncher : public CBase {
	public:
		static CBrowserLauncher* NewL();
		static CBrowserLauncher* NewLC();
		~CBrowserLauncher();

	private:
		CBrowserLauncher();
		void ConstructL();

	public:
		void GetPlatformVersionL(TUint& aMajor, TUint& aMinor);
		void LaunchBrowserWithLinkL(const TDesC& aLink);
};

#endif /*BROWSERLAUNCHER_H_*/
