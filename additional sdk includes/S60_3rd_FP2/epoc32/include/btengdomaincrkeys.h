/*
* ============================================================================
*  Name        : btengdomaincrkeys.h
*  Part of     : Bluetooth Engine / Bluetooth Engine
*  Description : Bluetooth Engine domain central repository key definitions.
*  Version     : %version: 1.1.3 % 
*
*  Copyright © 2006 Nokia.  All rights reserved.
*  This material, including documentation and any related computer
*  programs, is protected by copyright controlled by Nokia.  All
*  rights are reserved.  Copying, including reproducing, storing,
*  adapting or translating, any or all of this material requires the
*  prior written consent of Nokia.  This material also contains
*  confidential information which may not be disclosed to others
*  without the prior written consent of Nokia.
* ============================================================================
* Template version: 4.1
*/

#ifndef BTENG_DOMAIN_CR_KEYS_H
#define BTENG_DOMAIN_CR_KEYS_H


#include <btserversdkcrkeys.h>


/**  Bluetooth Engine CenRep Uid */
const TUid KCRUidBluetoothEngine = { 0x10204DAB };


/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if Bluetooth Headset Profile is supported or not.
 *
 * Possible integer values:
 * 0 Headset Profile not supported
 * 1 Headset Profile supported
 *
 * Default value: 1 
 */
const TUint32 KBTHspSupported = 0x00000001;


/**  Enumeration for Headset profile support */
enum TBTHspSupported
    {
    EBTHspNotSupported = 0,
    EBTHspSupported
    };


/**
 * CenRep key for storing Bluetooth feature settings.
 * Product specific settings for activating BT in offline mode.
 *
 * Possible integer values:
 * 0 BT activation disabled in offline mode
 * 1 BT activation enabled in offline mode
 *
 * Default value: 1
 */
const TUint32 KBTEnabledInOffline = 0x00000002;


/**  Enumeration for Bluetooth activation in offline mode */
enum TBTEnabledInOfflineMode
    {
    EBTDisabledInOfflineMode = 0,
    EBTEnabledInOfflineMode
    };


/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if eSCO is supported.
 *
 * Possible integer values:
 * 0 eSCO not supported
 * 1 eSCO not supported
 *
 * Default value: 0
 */
const TUint32 KBTEScoSupportedLV = 0x00000003;


/**  Enumeration for eSCO support */
enum TBTEScoSupported
    {
    EBTEScoNotSupported = 0,
    EBTEScoSupported
    };


/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if device selection/passkey setting by means 
 * other than user input is enabled.
 *
 * Possible integer values:
 * 0 Out-of-band setting is disabled
 * 1 Out-of-band setting is enabled
 *
 * Default value: 0
 */
const TUint32 KBTOutbandDeviceSelectionLV = 0x00000004;


/**  Enumeration for out-of-band selection mode */
enum TBTOutbandSelection
    {
    EBTOutbandDisabled = 0,
    EBTOutbandEnabled
    };


/**
 * CenRep key for storing Bluetooth feature settings.
 * Stores the Bluetooth Vendor ID.
 *
 * The integer value is specified by the Bluetooth SIG, and used for the 
 * Device Identification Profile. It needs to be pre-set by each product.
 */
const TUint32 KBTVendorID = 0x00000005;


/**
 * CenRep key for storing Bluetooth feature settings.
 * Stores the Bluetooth Product ID.
 *
 * The integer value is used for the Device Identification Profile. It is 
 * product-specific, and the values are managed by the company 
 * (as identified by the Vendor ID) It needs to be pre-set by each product.
 */
const TUint32 KBTProductID = 0x00000006;

/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if supports remote volume control over AVRCP Controller.
 *
 * Possible integer values:
 * 0  supported
 * 1  supported
 *
 * Default value: 1
 */
const TUint32 KBTAvrcpVolCTLV = 0x00000007;

/**  Enumeration for remote volume control AVRCP Controller support */
enum TBTAvrcpVolCTSupported
    {
    EBTAvrcpVolCTNotSupported = 0,
    EBTAvrcpVolCTSupported
    };
    
/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if the auto-pairing feature for audio devices is supported.
 *
 * Possible integer values:
 * 0  supported
 * 1  supported
 *
 * Default value: 0
 */
const TUint32 KBTAutoPairingLV = 0x00000008;


/**  Enumeration for remote volume control AVRCP Controller support */
enum TBTAutoPairingSupported
    {
    EBTAutoPairingNotSupported = 0,
    EBTAutoPairingSupported
    };

/**
 * CenRep key for storing Bluetooth feature settings.
 * Indicates if the Bluetooth/IrDa Receiving Indicator feature is supported.
 *
 * Possible integer values:
 * 0  supported
 * 1  supported
 *
 * Default value: 0
 */
const TUint32 KBTBTIRReceiveIndicatorLV = 0x00000009;

/**  Enumeration for remote volume control AVRCP Controller support */
enum TBTReceiveIndicatorSupported
    {
    EBTIRReceiveIndicatorNotSupported = 0,
    EBTIRReceiveIndicatorSupported
    };
/**  Bluetooth Local Device Address CenRep UID */
const TUid KCRUidBluetoothLocalDeviceAddress = { 0x10204DAA };

/**
 * CenRep key for storing the Bluetooth local device address.
 *
 * Default value (in string format): ""
 */
const TUint32 KBTLocalDeviceAddress = 0x00000001;


// BTENG_DOMAIN_CR_KEYS_H
#endif
