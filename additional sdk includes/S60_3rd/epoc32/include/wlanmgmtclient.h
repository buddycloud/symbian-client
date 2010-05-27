/*
* ==============================================================================
*  Name        : wlanmgmtclient.h
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Wrapper class for instantiating an implementation of
*                MWlanMgmtInterface via ECom framework.
*  Version     : %Version: %
*
*  Copyright © 2002-2005 Nokia. All rights reserved.
*  This material, including documentation and any related 
*  computer programs, is protected by copyright controlled by 
*  Nokia. All rights are reserved. Copying, including 
*  reproducing, storing, adapting or translating, any 
*  or all of this material requires the prior written consent of 
*  Nokia. This material also contains confidential 
*  information which may not be disclosed to others without the 
*  prior written consent of Nokia.
* ==============================================================================
*/

#ifndef WLANMGMTCLIENT_H
#define WLANMGMTCLIENT_H

// INCLUDES
#include <ecom/ecom.h>
#include "wlanmgmtinterface.h"

// CLASS DECLARATION
/**
* @brief Class for instantiating an implementation of MWlanMgmtInterface via ECom.
* @lib wlanmgmtimpl.dll
* @since Series 60 3.0
*/
class CWlanMgmtClient : public CBase, public MWlanMgmtInterface
    {
    public:  // Methods

       // Constructors and destructor
        
        /**
        * Static constructor.
        * @return Pointer to the constructed object.
        */
        inline static CWlanMgmtClient* NewL();
        
        /**
        * Destructor.
        */
        inline virtual ~CWlanMgmtClient();
        
    private: // Data

        /**
         * Identifies the instance of an implementation created by
         * the ECOM framework.
         */
        TUid iInstanceIdentifier;
    };

#include "wlanmgmtclient.inl"

#endif // WLANMGMTCLIENT_H
            
// End of File
