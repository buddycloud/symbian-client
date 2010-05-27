/*
* ==============================================================================
*  Name        : wlanmgmtcommon.h
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Contains common data structures used by WLAN management service.
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

#ifndef WLANMGMTCOMMON_H
#define WLANMGMTCOMMON_H

// INCLUDES
#include <e32std.h>

// LOCAL CONSTANTS
// The maximum SSID length.
const TUint KWlanMaxSsidLength = 32;

// The maximum BSSID length.
const TUint KWlanMaxBssidLength = 6;

// DATA TYPES
// Data structure for storing the SSID of a WLAN network.
typedef TBuf8<KWlanMaxSsidLength> TWlanSsid;

// Data structure for storing the BSSID of a BSS.
typedef TBuf8<KWlanMaxBssidLength> TWlanBssid;

// Values for possible WLAN connection states.
enum TWlanConnectionMode
    {
    // No connection is active.
    EWlanConnectionModeNotConnected,
    // Connection to an infrastrucutre network is active.
    EWlanConnectionModeInfrastructure,    
    // Connection to an ad-hoc network is active.
    EWlanConnectionModeAdhoc,
    // Connection to a secure infrastructure network is active.
    EWlanConnectionModeSecureInfra,
    // Searching for an access point. No data flow.
    EWlanConnectionModeSearching
    };

// Values for possible WLAN connection security modes.
enum TWlanConnectionSecurityMode
    {
    // Security mode open, i.e. no security
    EWlanConnectionSecurityOpen,
    // Security mode WEP
    EWlanConnectionSecurityWep,
    // Security mode 802d1x
    EWlanConnectionSecurity802d1x,
    // Security mode WPA
    EWlanConnectionSecurityWpa,
    // Security mode WPA PSK
    EWlanConnectionSecurityWpaPsk
    };

// Data values for RSS classes.
enum TWlanRssClass
    {
    // Received signal level is 'normal'
    EWlanRssClassNormal,
    // Received signal level is 'weak'
    EWlanRssClassWeak
    };

// CLASS DECLARATION
/** 
* @brief Callback interface for WLAN management notifications.
*
* These virtual methods should be inherited and implemented by the
* client wanting to observe WLAN management events.
*
* The client has to enable notifications by calling the appropriate
* method from the management interface.
* @see MWlanMgmtInterface::ActivateNotificationsL.
* @lib wlanmgmtimpl.dll
* @since S60 3.0
*/
class MWlanMgmtNotifications
    {
    public:

        /**
        * Connection status has changed.
        */
        virtual void ConnectionStateChanged( TWlanConnectionMode /* aNewState */ ) {};
    
        /**
        * BSSID has changed (i.e. AP handover).
        */
        virtual void BssidChanged( TWlanBssid& /* aNewBSSID */ ) {};

        /**
        * Connection has been lost.
        */
        virtual void BssLost() {};

        /**
        * Connection has been regained.
        */
        virtual void BssRegained() {};

        /**
        * New networks have been detected during scan.
        */
        virtual void NewNetworksDetected() {};

        /**
        * One or more networks have been lost since the last scan.
        */
        virtual void OldNetworksLost() {};

        /**
        * The used transmit power has been changed.
        * @param aPower The transmit power in mW.
        */
        virtual void TransmitPowerChanged( TUint /* aPower */ ) {};
        
        /**
        * Received signal strength level has been changed.
        * @param aRssClass specifies the current class of the received signal
        * @param aRss RSS level in absolute dBm values.
        */
        virtual void RssChanged(
            TWlanRssClass /* aRssClass */,
            TUint /* aRss */ ) {};

    };

#endif // WLANMGMTCOMMON_H
            
// End of File
