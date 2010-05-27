/*
* ==============================================================================
*  Name        : wlanscaninfo.h
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Wrapper class for instantiating an implementation of
*                MWlanScanInfoBase and MWlanScanInfoIteratorBase
*                via ECom framework.
*  Version     : %version: 7 %
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

#ifndef WLANSCANINFO_H
#define WLANSCANINFO_H

// INCLUDES
#include <ecom/ecom.h>
#include "wlanscaninfointerface.h"

// CLASS DECLARATION
/**
 * @brief Class for instantiating an implementation of MWlanScanInfoBase
 * and MWlanScanInfoIteratorBase via ECom.
 *
 * This class encapsulates both the WLAN scan results and methods
 * needed for parsing and iterating through them.
 *
 * Scan results consist of a list of all the BSSs that the scan discovered
 * and their parameters. Methods from MWlanScanInfoIteratorBase are used
 * to iterate through list of BSSs. Parameters of an individual BSS are
 * parsed with methods from MWlanScanInfoBase.
 *
 * An usage example: 
 * @code
 * // scanInfo is a pointer to a CScanInfo object that contains results 
 * // from a scan.
 *
 * // Information Element ID for SSID as specified in 802.11.
 * const TUint8 KWlan802Dot11SsidIE = 0;
 *
 * // Iterate through scan results.
 * for( scanInfo->First(); !scanInfo->IsDone(); scanInfo->Next() )
 *     {
 *     // Parse through BSS parameters.
 *
 *     TWlanBssid bssid;
 *     scanInfo->Bssid( bssid );
 *     // BSSID of the BSS is now stored in bssid.
 *
 *     TUint8 ieLen( 0 );
 *     const TUint8* ieData( NULL );
 *     TInt ret = scanInfo->InformationElement( KWlan802Dot11SsidIE, ieLen, &ieData );
 *     if ( ret == KErrNone )
 *         {
 *         // ieData now points to the beginning of the 802.11 SSID payload data
 *         // (i.e. the header is bypassed). ieLen contains the length of payload
 *         // data.
 *         }
 *     }
 * @endcode
 * @see MWlanMgmtInterface::GetScanResults
 * @since S60 3.0
 */
class CWlanScanInfo : public CBase, public MWlanScanInfoBase,
                      public MWlanScanInfoIteratorBase
    {
    public:  // Methods

       // Constructors and destructor
        
        /**
         * Static constructor.
         * @return Pointer to the constructed object.
         */
        inline static CWlanScanInfo* NewL();
        
        /**
         * Destructor.
         */
        inline virtual ~CWlanScanInfo();
        
    private: // Data

        /**
         * Identifies the instance of an implementation created by
         * the ECOM framework.
         */
        TUid iInstanceIdentifier;
    };

#include "wlanscaninfo.inl"

// WLANSCANINFO_H 
#endif
            
// End of File
