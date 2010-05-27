/*
* ==============================================================================
*  Name        : wlanmgmtclient.inl
*  Part of     : WLAN Engine / Adaptation
*  Interface   : 
*  Description : Inline functions of CWlanMgmtClient class.
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

#ifndef WLANMGMTCLIENT_INL
#define WLANMGMTCLIENT_INL

// ---------------------------------------------------------
// CWlanScanRequest::NewL
// ---------------------------------------------------------
//
inline CWlanMgmtClient* CWlanMgmtClient::NewL()
    {
    const TUid KCWlanMgmtClientUid = { 0x101f8eff };

    TAny* interface = REComSession::CreateImplementationL(
        KCWlanMgmtClientUid,
        _FOFF( CWlanMgmtClient,
        iInstanceIdentifier ) );
    return reinterpret_cast<CWlanMgmtClient*>( interface );
    }
    
// ---------------------------------------------------------
// CWlanScanRequest::~CWlanMgmtClient
// ---------------------------------------------------------
//
inline CWlanMgmtClient::~CWlanMgmtClient()
    {
    REComSession::DestroyedImplementation( iInstanceIdentifier );
    }

#endif // WLANMGMTCLIENT_INL
            
// End of File
