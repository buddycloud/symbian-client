/*
* ==============================================================================
*  Name        : wlanscaninfo.inl
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Inline functions of CWlanScanInfo class.
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

#ifndef WLANSCANINFO_INL
#define WLANSCANINFO_INL

// C++ default constructor can NOT contain any code, that
// might leave.
//
inline CWlanScanInfo::CWlanScanInfo()
    {
    }

// Static constructor.
inline CWlanScanInfo* CWlanScanInfo::NewL()
    {
    const TUid KCWlanScanInfoErin = { 0x101f8f01 };

    TAny* interface = REComSession::CreateImplementationL( KCWlanScanInfoErin,
        _FOFF( CWlanScanInfo, iInstanceIdentifier ) );
    return reinterpret_cast<CWlanScanInfo*>( interface );
    }
    
// Destructor
inline CWlanScanInfo::~CWlanScanInfo()
    {
    REComSession::DestroyedImplementation( iInstanceIdentifier );
    }

#endif // WLANSCANINFO_INL
            
// End of File
