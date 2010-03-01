/*
============================================================================
 Name        : Buddycloud.cpp
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
 Description : Main application class
============================================================================
*/

// INCLUDE FILES
#include <eikstart.h>
#include "BuddycloudApplication.h"


LOCAL_C CApaApplication* NewApplication()
	{
	return new CBuddycloudApplication;
	}

GLDEF_C TInt E32Main()
	{
	return EikStart::RunApplication( NewApplication );
	}

