/*
* ==============================================================================
*  Name        : wlanmgmtcommon.h
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Contains common data structures used by WLAN management service.
*  Version     : %version: 5 %
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
/** The maximum SSID length. */
const TUint KWlanMaxSsidLength = 32;

/** The maximum BSSID length. */
const TUint KWlanMaxBssidLength = 6;

/** The maximum length of WPA preshared key data. */
const TUint KWlanWpaPskMaxLength = 63;

/** The maximum length of WEP key data. */
const TUint KWlanWepKeyMaxLength = 29;

// DATA TYPES
/** Data structure for storing the SSID of a WLAN network. */
typedef TBuf8<KWlanMaxSsidLength> TWlanSsid;

/** Data structure for storing the BSSID of a BSS. */
typedef TBuf8<KWlanMaxBssidLength> TWlanBssid;

/** Data structure for storing a WEP key. */
typedef TBuf8<KWlanWepKeyMaxLength> TWlanWepKey;

/** Data structure for storing a WPA preshared key. */
typedef TBuf8<KWlanWpaPskMaxLength> TWlanWpaPresharedKey;

/** Values for possible WLAN connection states. */
enum TWlanConnectionMode
    {
    /** No connection is active. */
    EWlanConnectionModeNotConnected,
    /** Connection to an infrastructure network is active. */
    EWlanConnectionModeInfrastructure,    
    /** Connection to an ad-hoc network is active. */
    EWlanConnectionModeAdhoc,
    /** Connection to a secure infrastructure network is active. */
    EWlanConnectionModeSecureInfra,
    // Searching for an access point. No data flow.
    EWlanConnectionModeSearching
    };

/** Values for possible WLAN operating modes. */
enum TWlanOperatingMode
    {
    /** Ad-hoc network. */
    EWlanOperatingModeAdhoc,
    /** Infrastructure network. */
    EWlanOperatingModeInfrastructure
    };

/** Values for possible WLAN connection security modes. */
enum TWlanConnectionSecurityMode
    {
    /** Security mode open, i.e. no security. */
    EWlanConnectionSecurityOpen,
    /** Security mode WEP. */
    EWlanConnectionSecurityWep,
    /** Security mode 802d1x. */
    EWlanConnectionSecurity802d1x,
    /** Security mode WPA. */
    EWlanConnectionSecurityWpa,
    /** Security mode WPA PSK. */
    EWlanConnectionSecurityWpaPsk
    };

/** Defines the possible values for IAP security mode. */
enum TWlanIapSecurityMode
    {
    /** No encryption used. */
    EWlanIapSecurityModeAllowUnsecure,
    /** Use WEP encryption with static keys. */
    EWlanIapSecurityModeWep,
    /** Use WEP/TKIP/CCMP encryption, keys are negotiated by EAPOL. */
    EWlanIapSecurityMode802d1x,
    /** Use TKIP/CCMP encryption, keys are negotiated by EAPOL. */
    EWlanIapSecurityModeWpa,
    /** Use CCMP encryption, keys are negotiated by EAPOL. */
    EWlanIapSecurityModeWpa2Only,
    };

/** Data values for RSS classes. */
enum TWlanRssClass
    {
    /** Received signal level is 'normal'. */
    EWlanRssClassNormal,
    /** Received signal level is 'weak'. */
    EWlanRssClassWeak
    };

/** Enumeration of the possible default WEP keys. */
enum TWlanDefaultWepKey
    {
    EWlanDefaultWepKey1,            ///< Key number 1
    EWlanDefaultWepKey2,            ///< Key number 2
    EWlanDefaultWepKey3,            ///< Key number 3
    EWlanDefaultWepKey4             ///< Key number 4
    };

/** Enumeration of the possible authentication modes. */
enum TWlanAuthenticationMode
    {
    EWlanAuthenticationModeOpen,    ///< Open authentication
    EWlanAuthenticationModeShared   ///< Shared authentication
    };

/** Data structure for storing a Protected Setup credential attribute. */
struct TWlanProtectedSetupCredentialAttribute
    {
    /** Operating mode of the network. */
    TWlanOperatingMode iOperatingMode;
    /** Authentication mode of the network. */
    TWlanAuthenticationMode iAuthenticationMode;
    /** Security mode of the network. */
    TWlanIapSecurityMode iSecurityMode;
    /** Name of the network. */
    TWlanSsid iSsid;
    /** WEP key number 1. */
    TWlanWepKey iWepKey1;
    /** WEP key number 2. */
    TWlanWepKey iWepKey2;
    /** WEP key number 3. */
    TWlanWepKey iWepKey3;
    /** WEP key number 4. */
    TWlanWepKey iWepKey4;
    /** The WEP key used by default. */
    TWlanDefaultWepKey iWepDefaultKey;
    /** WPA preshared key. */
    TWlanWpaPresharedKey iWpaPreSharedKey;
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
 * @since S60 3.0
 */
class MWlanMgmtNotifications
    {
    public:

        /**
         * Connection status has changed.
         */
        virtual void ConnectionStateChanged(
            TWlanConnectionMode /* aNewState */ ) {};

        /**
         * BSSID has changed (i.e. AP handover).
         */
        virtual void BssidChanged(
            TWlanBssid& /* aNewBSSID */ ) {};

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
        virtual void TransmitPowerChanged(
            TUint /* aPower */ ) {};
        
        /**
         * Received signal strength level has been changed.
         * @param aRssClass specifies the current class of the received signal
         * @param aRss RSS level in absolute dBm values.
         */
        virtual void RssChanged(
            TWlanRssClass /* aRssClass */,
            TUint /* aRss */ ) {};

    };

// WLANMGMTCOMMON_H
#endif
            
// End of File
