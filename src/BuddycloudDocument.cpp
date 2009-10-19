/*
============================================================================
 Name        : BuddycloudDocument.cpp
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : CBuddycloudDocument implementation
============================================================================
*/


// INCLUDE FILES
#include "BuddycloudAppUi.h"
#include "BuddycloudDocument.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CBuddycloudDocument::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CBuddycloudDocument* CBuddycloudDocument::NewL( CEikApplication&
                                                          aApp )
    {
    CBuddycloudDocument* self = NewLC( aApp );
    CleanupStack::Pop( self );
    return self;
    }

// -----------------------------------------------------------------------------
// CBuddycloudDocument::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CBuddycloudDocument* CBuddycloudDocument::NewLC( CEikApplication&
                                                           aApp )
    {
    CBuddycloudDocument* self =
        new ( ELeave ) CBuddycloudDocument( aApp );

    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// -----------------------------------------------------------------------------
// CBuddycloudDocument::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CBuddycloudDocument::ConstructL()
    {
    // No implementation required
    }

// -----------------------------------------------------------------------------
// CBuddycloudDocument::CBuddycloudDocument()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CBuddycloudDocument::CBuddycloudDocument( CEikApplication& aApp )
    : CAknDocument( aApp )
    {
    // No implementation required
    }

// ---------------------------------------------------------------------------
// CBuddycloudDocument::~CBuddycloudDocument()
// Destructor.
// ---------------------------------------------------------------------------
//
CBuddycloudDocument::~CBuddycloudDocument()
    {
    // No implementation required
    }

// ---------------------------------------------------------------------------
// CBuddycloudDocument::CreateAppUiL()
// Constructs CreateAppUi.
// ---------------------------------------------------------------------------
//
CEikAppUi* CBuddycloudDocument::CreateAppUiL()
    {
    // Create the application user interface, and return a pointer to it;
    // the framework takes ownership of this object
    return ( static_cast <CEikAppUi*> ( new ( ELeave )
                                        CBuddycloudAppUi ) );
    }

// End of File
