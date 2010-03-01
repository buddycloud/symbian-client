/*
============================================================================
 Name        : BuddycloudDocument.h
 Author      : Ross Savage
 Copyright   : 2007 Buddycloud
 Description : Declares document class for application.
============================================================================
*/

#ifndef __BUDDYCLOUDDOCUMENT_h__
#define __BUDDYCLOUDDOCUMENT_h__

// INCLUDES
#include <akndoc.h>

// FORWARD DECLARATIONS
class CBuddycloudAppUi;
class CEikApplication;


// CLASS DECLARATION

/**
* CBuddycloudDocument application class.
* An instance of class CBuddycloudDocument is the Document part of the
* AVKON application framework for the Buddycloud example application.
*/
class CBuddycloudDocument : public CAknDocument
    {
    public: // Constructors and destructor

        /**
        * NewL.
        * Two-phased constructor.
        * Construct a CBuddycloudDocument for the AVKON application aApp
        * using two phase construction, and return a pointer
        * to the created object.
        * @param aApp Application creating this document.
        * @return A pointer to the created instance of CBuddycloudDocument.
        */
        static CBuddycloudDocument* NewL( CEikApplication& aApp );

        /**
        * NewLC.
        * Two-phased constructor.
        * Construct a CBuddycloudDocument for the AVKON application aApp
        * using two phase construction, and return a pointer
        * to the created object.
        * @param aApp Application creating this document.
        * @return A pointer to the created instance of CBuddycloudDocument.
        */
        static CBuddycloudDocument* NewLC( CEikApplication& aApp );

        /**
        * ~CBuddycloudDocument
        * Virtual Destructor.
        */
        virtual ~CBuddycloudDocument();

    public: // Functions from base classes

        /**
        * CreateAppUiL
        * From CEikDocument, CreateAppUiL.
        * Create a CBuddycloudAppUi object and return a pointer to it.
        * The object returned is owned by the Uikon framework.
        * @return Pointer to created instance of AppUi.
        */
        CEikAppUi* CreateAppUiL();

    private: // Constructors

        /**
        * ConstructL
        * 2nd phase constructor.
        */
        void ConstructL();

        /**
        * CBuddycloudDocument.
        * C++ default constructor.
        * @param aApp Application creating this document.
        */
        CBuddycloudDocument( CEikApplication& aApp );

    };

#endif // __BUDDYCLOUDDOCUMENT_h__

// End of File
