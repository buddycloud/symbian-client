/*
* ============================================================================
*  Name     : BTServerInternalPSKeys.h
*  Part of  : BTServer
*
*  Description:
*     BT Server P&S key definitions
*  Version: %version: 7 %
*
*  Copyright (C) 2002-2006 Nokia Corporation.
*  This material, including documentation and any related 
*  computer programs, is protected by copyright controlled by 
*  Nokia Corporation. All rights are reserved. Copying, 
*  including reproducing, storing,  adapting or translating, any 
*  or all of this material requires the prior written consent of 
*  Nokia Corporation. This material also contains confidential 
*  information which may not be disclosed to others without the 
*  prior written consent of Nokia Corporation.
*
* ============================================================================
*/

#ifndef BTSERVER_INTERNAL_PS_KEYS_H
#define BTSERVER_INTERNAL_PS_KEYS_H

/**
* PubSub Uid of Bluetooth DUT mode
*/
const TUid KPSUidBluetoothDutMode = {0x101FFE48};

/**
* Bluetooth device under test mode
*
* Possible integer values:
* 0 (EBTDutOff) Device not in test mode
* 1 (EBTDutOn)  Device in test mode
*
* EBTDutOff if this key is not defined
*
* (shared data key: KDutMode)
*/
const TUint KBTDutEnabled = 0x01;

// Enumeration for BT Test mode values
enum TBTDutModeValue
	{
	EBTDutOff,
	EBTDutOn
	};

/**
* PubSub Uid of Bluetooth Chip state
*/
const TUid KPSUidBluetoothChipState = {0x101FFE47};

/**
* Bluetooth chip mode
*
* Possible integer values:
* 0 (EBTChipNormal)
* 1 (EBTChipError)
*
* EBTChipNormal(chip is working properly, or no error deteced so far) if this key is not defined
* Setting the value of this key to EBTChipError will trigger btserver to turn bt off.
*/
const TUint KBTChipState = 0x01;

// Enumeration for BT chip state values
enum TBTChipStateValue
	{
	EBTChipNormal,
	EBTChipError
	};

// PubSub Uid of BTAAC state
const TUid KPSUidBluetoothAudioAccessoryState = { 0x101FFE49 };

// Key1 : Bluetooth HW address of the connected BTAA (string value)
// No connected BT audio accessory if this key is not defined or the value is "",
// otherwise, it contains the readable BT address of audio accessory
// 
const TUint KBtaaAddress = 0x00000001;

// Key2 : Is a BTAA (BT Audio Accessory) connected and does it support remote volume control.
// No connected BT audio accessory if this key is not defined.
const TUint KBtaacConnected = 0x00000002;

// Possible integer values for key KBtaacConnected
enum TBtaaConnectionStatus
	{
	EBTaacNoAudioAccConnected,
	EBTaacRemoteVolCtrlCapableAccConnected,
	EBTaacRemoteVolCtrlIncapableAccConnected,
	EBTaacAccTemporarilyDisconnected,
	EBTaacAccTemporarilyUnavailable 		
	};

// Indicates state of Connected device capturing COD bit
const TUint KBtaacDRMSecureAccessory = 0x00000005;

enum TBTDRMSecureAccessory
	{
	EBTAccessoryCapturing,
	EBTAccessoryNotCapturing
	};

/**
* BTServer internal PubSub Category
*/
const TUid KPSUidBTServerInternal = {0x10005950}; // BTServer SID

/**
* BT Mono Audio Control Connection
*
* Possible integer values:
* 0 (ENoControlConnection) No BT accessory connected
* 1 (EHandsfreeServiceLevelFailed) Service Level establishment failed
* 2 (EHandsfreeConnected) Handsfree connected (Service Level established)
* 3 (EHeadsetConnected) Headset connected
*
* 0 if this key is not defined
*/
const TUint KBTMonoAudioControlConnection = 0x20;

enum TBTMonoAudioControlConnectionState
    {
    ENoControlConnection,
    EHandsfreeServiceLevelFailed,
    EHandsfreeServiceLevelConnected,
    EHeadsetConnected
    };

// Bluetooth HW address of the connected BTAA (string value)
// No connected BT audio accessory if this key is not defined or the value is "",
// otherwise, it contains the readable BT address of audio accessory
//
const TUint KBTMonoAudioControlConnectionAddr = 0x21;

const TUint KBTStereoAudioConnectionState = 0x22;

enum TBTStereoAudioConnectionState
    {
    EBTStereoAudioDisconnected,
    EBTStereoAudioConnected
    };

// Bluetooth HW address of the connected Stereo headset
// No connected BT audio accessory if this key is not defined or the value is "",
// otherwise, it contains the readable BT address of audio accessory
//
const TUint KBTStereoAudioConnectionAddr = 0x23;

// This key is used to determine Voice Dial states related to BT audio connections
// We use BTServer's UID, but we keep the old name used by BTAAC
const TUid KPSUidBluetoothVoiceDial = {0x101FFE4A};

// Indicates the state of Voice Dialer
const TUint KNameDialerMode = 0x00000003;

enum TBTVoiceDialDialerMode
	{
	EBTaacVoiceDialerIdle,
	EBTaacVoiceDialerRecording,
	EBTaacVoiceDialerPlaying,
	EBTaacVoiceDialerRecognizing
	};

#endif // BTSERVER_INTERNAL_PS_KEYS_H

