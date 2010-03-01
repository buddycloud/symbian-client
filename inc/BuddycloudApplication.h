/*
============================================================================
 Name        : BuddycloudApplication.h
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
 Description : Declares main application class.
============================================================================
*/

#ifndef __BUDDYCLOUDAPPLICATION_H__
#define __BUDDYCLOUDAPPLICATION_H__

// INCLUDES
#include <aknapp.h>

// CLASS DECLARATION

/**
* CBuddycloudApplication application class.
* Provides factory to create concrete document object.
* An instance of CBuddycloudApplication is the application part of the
* AVKON application framework for the Buddycloud example application.
*/
class CBuddycloudApplication : public CAknApplication
    {
    public: // Functions from base classes

        /**
        * From CApaApplication, AppDllUid.
        * @return Application's UID (KUidBuddycloudApp).
        */
        TUid AppDllUid() const;

    protected: // Functions from base classes

        /**
        * From CApaApplication, CreateDocumentL.
        * Creates CBuddycloudDocument document object. The returned
        * pointer in not owned by the CBuddycloudApplication object.
        * @return A pointer to the created document object.
        */
        CApaDocument* CreateDocumentL();
    };

#endif // __BUDDYCLOUDAPPLICATION_H__

// End of File
