/*
* ==============================================================================
*  Name        : wlanscaninfo.inl
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Inline functions of CWlanScanInfo class.
*  Version     : %version: 4 %
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

#ifndef WLANSCANINFO_INL
#define WLANSCANINFO_INL

// Static constructor.
inline CWlanScanInfo* CWlanScanInfo::NewL()
    {
    const TUid KCWlanScanInfoUid = { 0x101f8f01 };

    TAny* interface = REComSession::CreateImplementationL(
        KCWlanScanInfoUid,
        _FOFF( CWlanScanInfo,
        iInstanceIdentifier ) );
    return reinterpret_cast<CWlanScanInfo*>( interface );
    }
    
// Destructor
inline CWlanScanInfo::~CWlanScanInfo()
    {
    REComSession::DestroyedImplementation( iInstanceIdentifier );
    }

// WLANSCANINFO_INL
#endif
            
// End of File
