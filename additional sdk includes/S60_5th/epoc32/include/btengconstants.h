/*
* ============================================================================
*  Name        : btengconstants.h
*  Part of     : Bluetooth Engine / Bluetooth Engine
*  Description : Definitions of Bluetooth Engine constant.
*  Version     : %version: 1 % 
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

#ifndef BTENCONSTANTS_H
#define BTENCONSTANTS_H


/**  Connect status of the service-level connection. */
enum TBTEngConnectionStatus
    {
    EBTEngNotConnected,
    EBTEngConnecting,
    EBTEngConnected,
    EBTEngDisconnecting
    };


/**  Type of disconnect operation for the service-level connection. */
enum TBTDisconnectType
    {
    EBTDiscImmediate,
    EBTDiscGraceful
    };


/**  Enumeration for Bluetooth visibility mode */
enum TBTVisibilityMode
    {
    EBTVisibilityModeHidden    = 0x02,
    EBTVisibilityModeGeneral   = 0x03,
    EBTVisibilityModeTemporary = 0x100
    };


/**  Enumeration for Bluetooth profiles */
enum TBTProfile
    {
    EBTProfileUndefined = 0x0,
    EBTProfileDUN       = 0x1103,
    EBTProfileOPP       = 0x1105,
    EBTProfileFTP       = 0x1106,
    EBTProfileHSP       = 0x1108,
    EBTProfileA2DP      = 0x110D,
    EBTProfileAVRCP     = 0x110E,
    EBTProfileFAX       = 0x1111,
    EBTProfilePANU      = 0x1115,
    EBTProfileNAP       = 0x1116,
    EBTProfileGN        = 0x1117,
    EBTProfileBIP       = 0x111A,
    EBTProfileHFP       = 0x111E,
    EBTProfileBPP       = 0x1122,
    EBTProfileHID       = 0x1124,
    EBTProfileSAP       = 0x112D,
    EBTProfilePBAP      = 0x1130,
    EBTProfileDI        = 0x1200
    };

// BTENCONSTANTS_H
#endif
