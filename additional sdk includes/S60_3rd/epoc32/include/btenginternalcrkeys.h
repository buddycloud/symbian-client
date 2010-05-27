/*
* ============================================================================
*  Name     : BTEngInternalCRKeys.h
*  Part of  : BTEng
*
*  Description:
*     BT Engine internal CenRep key definitions
*  Version:
*
*  Copyright (C) 2002-2004 Nokia Corporation.
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

#ifndef BTENG_INTERNAL_CR_KEYS_H
#define BTENG_INTERNAL_CR_KEYS_H

/**
* Bluetooth Local Name UID
*/
const TUid KCRUidBluetoothLocalNameSettings = {0x10204DAC};

/**
* CenRep keys for storing BT local name settings:
* Local bluetooth device name, string
*/
const TUint32 KBTLocalName = 0x00000001;

/**
* CenRep keys for storing BT local name settings:
* Bluetooth local name status (if user has changed local BT name)
*
* Possible integer values:
* 0 (EBTLocalNameDefault) user has not changed local BT name
* 1 (EBTLocalNameSet) user has set the local BT name and 
*    the name has been written to chip
* 2 (EBTLocalNamePending) user has set the local BT name but the 
*    name has not been written to chip
*
* Default value: 0 (EBTLocalNameDefault)
*
* (Shared data key: KBTDefaultNameFlag)
*/
const TUint32 KBTLocalNameChanged = 0x00000002;

// Enumeration for LocalNameChangedStatus value
enum TBTLocalNameChangedStatus
    {
    EBTLocalNameDefault,
    EBTLocalNameSet,
    EBTLocalNamePending
    };

/**
* Bluetooth Search Mode UID
*/
const TUid KCRUidBluetoothSearchMode = {0x10204DAD};

/**
* CenRep Keys for BT search mode settings
*
* Bluetooth search mode
*
* Possible integer values:
* 0 (EBTSearchModeGeneral) General inquiry using GIAC
* 1 (EBTSearchModeLimited) LimitedLimited inquiry using LIAC
*
* Default value: 0(EBTSearchModeGeneral)
*/
const TUint32 KBTSearchMode	= 0x00000001;

#endif // BTENG_INTERNAL_CR_KEYS_H

