/*
* ============================================================================
*  Name        : btengconnman.h
*  Part of     : Bluetooth Engine / Bluetooth Connection Manager
*  Description : Bluetooth Engine API for connection management functionality.
*  Version     : %version: 1.2.2 % 
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

#ifndef BTENGCONNMAN_H
#define BTENGCONNMAN_H

#include <bt_sock.h>
#include <btengconstants.h>

class CBTEngConnHandler;
class CBTEngPairingHandler;


/**
 *  Class MBTEngConnObserver
 *
 *  Callback class for receiving CBTEngConnMan 
 *  connect/disconnect complete events.
 *
 *  @lib btengdevman.dll
 *  @since S60 V3.0
 */
class MBTEngConnObserver
    {

public:

    /**
     * Indicates to the caller that a service-level connection has completed.
     * This function is called for both incoming and outgoing connections. 
     * This function is also called when an outgoing connection request fails, 
     * e.g. with error code KErrCouldNotConnect.
     * When this function is called, new commands can be issued to the 
     * CBTEngConnMan API immediately.
     *
     * @since  S60 v3.2
     * @param  aAddr The address of the remote device.
     * @param  aErr Status information of the connection. KErrNone if the
     *              connection succeeded, otherwise the error code with 
     *              which the outgoing connection failed. KErrAlreadyExists 
     *              is returned if there already is an existing connection 
     *              for the selected profile(s), or otherwise e.g. 
     *              KErrCouldNotConnect or KErrDisconnected for indicating 
     *              connection problems.
     * @param  aConflicts If there already is a connection for the selected 
     *                    profile(s) of an outgoing connection request (the 
     *                    selection is performed by BTEng), then this array 
     *                    contains the bluetooth device addresses of the 
     *                    remote devices for those connections.
     */
    virtual void ConnectComplete( TBTDevAddr& aAddr, TInt aErr, 
                                   RBTDevAddrArray* aConflicts = NULL ) = 0;

    /**
     * Indicates to the caller that a service-level connection has disconnected.
     * When this function is called, new commands can be issued to the 
     * CBTEngConnMan API immediately.
     *
     * @since  S60 v3.2
     * @param  aAddr The address of the remote device.
     * @param  aErr The error code with which the disconnection occured. 
     *              KErrNone for a normal disconnection, 
     *              or e.g. KErrDisconnected if the connection was lost.
     */
    virtual void DisconnectComplete( TBTDevAddr& aAddr, TInt aErr ) = 0;

    /**
     * Indicates to the caller that a pairing procedure has been completed.
     * When this function is called, new commands can be issued to the 
     * CBTEngConnMan API immediately.
     * A default implementation (that does nothing) of this method exists.
     *
     * @since  S60 v3.2
     * @param  aAddr The address of the remote device.
     * @param  aErr The error code indicating the result of the pairing 
     *              procedure; KErrNone for a normal disconnection, or 
     *              e.g. KErrCancel if the user cancelled the pairing dialog.
     */
    virtual void PairingComplete( TBTDevAddr& aAddr, TInt aErr );

    };


/**
 *  Class CBTEngConnMan
 *
 *  This class provides functionality for Bluetooth connection management 
 *  on profile-level. The class mainly forwards the commands to BTEng server 
 *  side, which passes the commands to its plug-ins which handle the 
 *  profile-level (service-level) connection management.
 *
 *  Connection management commands are asynchronous, and the result is passed 
 *  back to the caller through the MBTEngConnObserver callback interface.
 *  Multiple commands can be outstanding simultaneously.
 *
 *  @lib btengconnman.lib
 *  @since S60 v3.2
 */
class CBTEngConnMan : public CBase
    {

public:

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  connection events (see also SetObserver below).
     * @return Pointer to the constructed CBTEngConnMan object.
     */
    IMPORT_C static CBTEngConnMan* NewL( MBTEngConnObserver* aObserver = NULL );

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  connection events (see also SetObserver below).
     * @return Pointer to the constructed CBTEngConnMan object.
     */
    IMPORT_C static CBTEngConnMan* NewLC( MBTEngConnObserver* aObserver = NULL );

    /**
     * Destructor
     */
    virtual ~CBTEngConnMan();

    /**
     * Create a service-level connection to the specified Bluetooth address.
     * The decision which profile to use for the service-level connection is 
     * taken by BTEng, based on the parameters that are part of the address 
     * structure passed as argument.
     *
     * This is an asynchronous operation; on completion, the caller is 
     * informed through MBTEngConnObserver::ConnectComplete with the 
     * appropriate error code.
     * 
     *
     * @since S60 v3.2
     * @param aAddr The Bluetooth address of the remote device.
     * @param aDeviceClass The device class of the remote device. This will 
     *                     be used to select the correct plug-in for creating 
     *                     the service-level connection.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt Connect( const TBTDevAddr& aAddr, 
                            const TBTDeviceClass& aDeviceClass );

    /**
     * Cancels the creation of a service-level connection to the specified 
     * Bluetooth address.
     *
     * This is part of an asynchronous operation; on completion, the caller 
     * is informed through MBTEngConnObserver::ConnectComplete with error 
     * code with KErrCancel if successfully canceled, or KErrNotFound if no 
     * service-level connection with the specified Bluetooth address existed.
     *
     * @since S60 v3.2
     * @param aAddr The Bluetooth address of the remote device.
     */
    IMPORT_C void CancelConnect( const TBTDevAddr& aAddr );

    /**
     * Disconnects the creation of a service-level connection with the 
     * specified Bluetooth address.
     *
     * This is an asynchronous operation; on completion, the caller is 
     * informed through  MBTEngConnObserver::DisconnectComplete with the 
     * appropriate error code.
     *
     * @since S60 v3.2
     * @param aAddr The Bluetooth address of the remote device.
     * @param aDiscType The type of disconnection; 
     *                  EGraceful for graceful (normal) disconnection, 
     *                  EImmediate for immediate (forced) disconnection.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt Disconnect( const TBTDevAddr& aAddr, TBTDisconnectType aDiscType );

    /**
     * Checks whether there is a service-level connection to the specified 
     * Bluetooth device.
     *
     * @since S60 v3.2
     * @param aAddr The Bluetooth address of the remote device.
     * @param aConnected On return, holds the connection status for the 
     *                   specified address; ENotConnected if no connection 
     *                   exists, EConnecting if a (service-level) connection 
     *                   is being established, EConnected if a service-level 
     *                   connection exists, and EDisconnecting if a 
     *                   service-level connection is being disconnected.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt IsConnected( const TBTDevAddr& aAddr, 
                                TBTEngConnectionStatus& aConnected );

    /**
     * Checks whether a service-level connection can be created to a device
     * that advertises the specified Class of Device.
     * Note that this only indicate that a suitable profile plug-in is 
     * currently loaded by BTEng, it does does not indicate that a connection 
     * would succeed (e.g. the device may refuse the connection or could be 
     * out of range).
     *
     * @since S60 v3.2
     * @param aConnectable On return, contains the value for indicating 
     *                     whether the device is connectable:
     *                     ETrue if the device is connectable, EFalse if not.
     * @param aDeviceClass The device class of the remote device. This will 
     *                     be used to determine whether the device is 
     *                     connectable or not.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt IsConnectable( const TBTDeviceClass& aDeviceClass, 
                                  TBool& aConnectable );

    /**
     * Subscribes to service-level connection events. This will replace any 
     * previoously registered callback interface object. The observer is 
     * informed of connection events through the MBTEngConnObserver 
     * callback interface.
     * Note: the observer is only notified of events related to service-level 
     * connections. Generic notifications about Bluetooth baseband events can 
     * be obtained through bluetooth.lib.
     *
     * An observer must be set before issueing any connection-related commands.
     *
     * @since S60 v3.2
     * @param aObserver The callback interface through which connection 
     *                  events are passed back.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt SetObserver( MBTEngConnObserver* aObserver );

    /**
     * Removes the subscription to service-level connection events.
     * Note: this functionality is implicit when destroying CBTConnMan.
     *
     * @since S60 v3.2
     */
    IMPORT_C void RemoveObserver();

    /**
     * Gets the remote addresses for all the open Bluetooth connections. 
     * These include all baseband connections, so also connections for 
     * which no service-level connection is handled by BTEng profile plug-ins.
     *
     * @since S60 v3.2
     * @param aAddrArray On return, holds the Bluetooth device addresses of 
     *                   all the connected Bluetooth devices.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt GetConnectedAddresses( RBTDevAddrArray& aAddrArray );

    /**
     * Gets the remote addresses for all the open Bluetooth connections 
     * for the specified profile.
     *
     * @since S60 v3.2
     * @param aAddrArray On return, holds the Bluetooth device addresses 
     *                   of the connected Bluetooth devices for the 
     *                   requested profile.
     * @param aConnectedProfile The profile for which the existing 
     *                          connections are requested.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt GetConnectedAddresses( RBTDevAddrArray& aAddrArray, 
                                       TBTProfile aConnectedProfile );

    /**
     * Start a pairing (a.k.a. bonding) operation with a remote 
     * Bluetooth device. This will launch the PIN query dialog 
     * as part of the pairing procedure. On completion, 
     *
     * This is an asynchronous operation; on completion, the caller is 
     * informed through  MBTEngConnObserver::PairingComplete with the 
     * appropriate error code.
     *
     * Note: this method is intended for operations that only include 
     * pairing, and no connection establishment. When requiring 
     * authentication/encryption on a Bluetooth link, the preferred method 
     * is to pass the security requirements through the TBTSockAddr structure.
     *
     * @since S60 v3.2
     * @param aDevice The address of the remote device to perform pairing with.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt PairDevice( const TBTDevAddr& aAddr );
    /**
     * Start a pairing (a.k.a. bonding) operation with a remote 
     * Bluetooth device. This will launch the PIN query dialog 
     * as part of the pairing procedure. Or in case of audio headset, and if
     * the auto-pairing feature is supported, the PIN query is not shown but
     * the default PIN (0000) is given automatically. If that fails the PIN query
     * is shown normally.
     *
     * This is an asynchronous operation; on completion, the caller is 
     * informed through  MBTEngConnObserver::PairingComplete with the 
     * appropriate error code.
     *
     * Note: this method is intended for operations that only include 
     * pairing, and no connection establishment. When requiring 
     * authentication/encryption on a Bluetooth link, the preferred method 
     * is to pass the security requirements through the TBTSockAddr structure.
     *
     * @since S60 v3.2
     * @param aDevice The address of the remote device to perform pairing with.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */    
    IMPORT_C TInt PairDevice( const TBTDevAddr& aAddr, TBTDeviceClass aDeviceClass );    

    /**
     * Cancels an ongoing search for nearby Bluetooth devices.
     *
     * @since S60 v3.2
     * @param aDevice The address of the remote device to perform pairing with.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C void CancelPairDevice();

// [SvV] Note: this method is only to be used by BTNotif
    /**
     * Listen to the result of a pairing operation, and launch 
     * the authorization notifier if successful.
     * Note: this method is for usage of BTNotif only. The authorization 
     * notifier is called independently i.e. there is no callback resulting 
     * from calling this function.
     * This function requires LocalServices and WriteDeviceData capabilities.
     *
     * @since S60 v3.2
     * @param aDevice The remote device with which pairing is performed.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt StartPairingObserver( const TBTDevAddr& aAddr );

// [SvV] Note: this method is only to be used by BTNotif
    /**
     * Prepares any existing Bluetooth connection topology for performing 
     * device discovery. As a result of this call, the server will request 
     * the master role on each existing connection.
     * Note: this method is for usage of BTNotif only. It attempts to 
     * optimize the existing topology so therefore there is no result to 
     * be informed. The functionality is only executed after this function 
     * has returned.
     *
     * @since S60 v3.2
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C void PrepareDiscovery();

    /**
     * Informs the connection manager that pairing has been completed, so
     * that we can clean up the resources.
     *
     * @since S60 v3.2
     */
    void PairingCompleted();
    
     /**
     * Tells whether the auto-pairing featurte for audio headsets is
     * currently enabled.
     *
     * @since S60 v3.2
     */
    TBool IsAutomatedPairingEnabled();    

private:

    /**
     * C++ default constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  connection events.
     */
    CBTEngConnMan( MBTEngConnObserver* aObserver );

    /**
     * Symbian 2nd-phase constructor
     *
     * @since S60 v3.2
     */
    void ConstructL();

private: // data

    /**
     * Handle to BTEng server side and listener to connection events.
     * Own.
     */
    CBTEngConnHandler* iConnHandler;

    /**
     * Handle to BTEng server side and listener to connection events.
     * Own.
     */
    CBTEngPairingHandler* iPairingHandler;

    /**
     * Reference to receiver of connection events.
     * Not own.
     */
    MBTEngConnObserver* iObserver;
     /**
     * Indicates whether the auto-pairing featurte for audio headsets is
     * currently enabled.
     */
    TBool iAutomatedPairingEnabled;

    };


// BTENGCONNMAN_H
#endif
