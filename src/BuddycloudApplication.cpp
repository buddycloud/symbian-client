/*
============================================================================
 Name        : BuddycloudApplication.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Main application class
============================================================================
*/

// INCLUDE FILES
#include "Buddycloud.hrh"
#include "BuddycloudDocument.h"
#include "BuddycloudApplication.h"

// ============================ MEMBER FUNCTIONS ===============================

// UID for the application;
// this should correspond to the uid defined in the mmp file
const TUid KUidBuddycloudApp = { APPUID };

// -----------------------------------------------------------------------------
// CBuddycloudApplication::CreateDocumentL()
// Creates CApaDocument object
// -----------------------------------------------------------------------------
//
CApaDocument* CBuddycloudApplication::CreateDocumentL()
    {
    // Create an Buddycloud document, and return a pointer to it
    return (static_cast<CApaDocument*>
                    ( CBuddycloudDocument::NewL( *this ) ) );
    }

// -----------------------------------------------------------------------------
// CBuddycloudApplication::AppDllUid()
// Returns application UID
// -----------------------------------------------------------------------------
//
TUid CBuddycloudApplication::AppDllUid() const
    {
    // Return the UID for the Buddycloud application
    return KUidBuddycloudApp;
    }

// End of File
