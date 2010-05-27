/*
* ==============================================================================
*  Name        : wlanscaninfointerface.h
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Interface definition for WLAN scan results.
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

#ifndef WLANSCANINFOINTERFACE_H
#define WLANSCANINFOINTERFACE_H

// INCLUDES
#include "wlanmgmtcommon.h"

// DATA TYPES
typedef TUint8 TWlanScanInfoFrame;

// CLASS DECLARATION
/**
* @brief ECom interface class for WLAN scan results iterator.
*
* This class contains the methods for iterating through
* WLAN scan results.
* @lib wlanmgmtimpl.dll
* @since S60 3.0
*/
class MWlanScanInfoIteratorBase
    {
    public:  // Methods

        /**
        * Return the size of the scan info. The size includes Status Info,
        * MAC header and Frame Body.
        * @return The size of the scan info in bytes.
        */
        virtual TUint16 Size() const = 0;

        /**
        * Find the data of the first access point.
        * @return Pointer at the beginning of the first access point stored 
        *         in the scan list. NULL if not any.
        */
        virtual const TWlanScanInfoFrame* First() = 0;

        /**
        * Find the data of the next access point.
        * @return Pointer at the beginning of the next access point stored
        *         in the scan list. NULL if not any.
        */
        virtual const TWlanScanInfoFrame* Next() = 0;

        /**
        * Find the data of the current access point.
        * @return Pointer at the beginning of the current access point stored 
        *         in the scan list. NULL if not any.
        */
        virtual const TWlanScanInfoFrame* Current() const = 0;

        /**
        * Find is there any more unhandled access points.
        * @return EFalse if there is access points in the list left, 
        *         ETrue if not.
        */
        virtual TBool IsDone() const = 0;
        
    };

/**
* @brief ECom interface class for WLAN scan results.
*
* This class contains the methods for parsing the scan results
* of a WLAN network.
* @lib wlanmgmtimpl.dll
* @since S60 3.0
*/
class MWlanScanInfoBase
    {
    public:  // Methods

        /**
        * Return RX level of the BSS.
        * @return RX level.
        */
        virtual TUint8 RXLevel() const = 0;

        /**
        * Return BSSID of the BSS.
        * @param  aBssid ID of the access point or IBSS network.
        * @return Pointer to the beginning of the BSSID. Length is always 6 bytes.
        */
        virtual void Bssid( TWlanBssid& aBssid ) const = 0;

        /**
        * Get beacon interval of the BSS.
        * @return the beacon interval.
        */
        virtual TUint16 BeaconInterval() const = 0;

        /**
        * Get capability of the BSS (see IEEE 802.11 section 7.3.1.4.
        * @return The capability information.
        */
        virtual TUint16 Capability() const = 0;

        /**
        * Get security mode of the BSS.
        * @return security mode.
        */
        virtual TWlanConnectionSecurityMode SecurityMode() const = 0;

        /**
        * Return requested information element.
        * @param aIE        Id of the requested IE data.
        * @param aLength    Length of the IE. Zero if IE not found.
        * @param aData      Pointer to the beginning of the IE data. NULL if IE not found.
        * @return           General error message.
        */
        virtual TInt InformationElement( TUint8 aIE, 
                                         TUint8& aLength, 
                                         const TUint8** aData ) = 0;
        /**
        * Return WPA information element.
        * @param aLength    Length of the IE. Zero if IE not found.
        * @param aData      Pointer to the beginning of the IE data. NULL if IE not found.
        * @return           General error message.
        */
        virtual TInt WpaIE( TUint8& aLength, 
                            const TUint8** aData ) = 0;

        /**
        * Return the first information element.
        * @param aIE        Id of the IE. See IEEE 802.11 section 7.3.2.
        * @param aLength    Length of the IE. Zero if IE not found.
        * @param aData      Pointer to the beginning of the IE data. NULL if IE not found.
        * @return           General error message.
        */
        virtual TInt FirstIE( TUint8& aIE, 
                              TUint8& aLength, 
                              const TUint8** aData ) = 0;
        /**
        * Return next information element.
        * @param aIE        Id of the IE. See IEEE 802.11 section 7.3.2.
        * @param aLength    Length of the IE. Zero if IE not found.
        * @param aData      Pointer to the beginning of the IE data. NULL if IE not found.
        * @return           General error message.
        */
        virtual TInt NextIE( TUint8& aIE, 
                             TUint8& aLength, 
                             const TUint8** aData ) = 0;
    };

#endif // WLANSCANINFOINTERFACE_H 
            
// End of File
