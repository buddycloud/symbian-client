/*
* ============================================================================
*  Name        : BTEngDevMan.h
*  Part of     : Bluetooth Engine / Bluetooth Device Manager
*  Description : The Bluetooth Engine Device Management API
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

#ifndef BTENGDEVMAN_H
#define BTENGDEVMAN_H

#include <btmanclient.h>


/**
 * Class MBTEngDevManObserver
 *
 * Mix-in class for calling back to CBTEngDevMan observer when 
 * device handling has completed.
 *
 *  @lib btengdevman.dll
 *  @since S60 V3.0
 */
class MBTEngDevManObserver
    {

public:

    /**
     * Indicates to the caller that adding, deleting or modifying a device 
     * has completed.
     * When this function is called, new commands can be issued to the 
     * CBTEngDevMan API immediately.
     *
     * @since  S60 v3.2
     * @param  aErr Status information, if there is an error.
     */
    IMPORT_C virtual void HandleDevManComplete( TInt aErr );

    /**
     * Indicates to the caller that getting an array of devices has completed.
     * When this function is called, new commands can be issued to the 
     * CBTEngDevMan API immediately.
     *
     * @since S60 v3.0
     * @param aErr Status information, if there is an error.
     * @param aDeviceArray Array of devices that match the given criteria 
     *                     (the array provided by the calller).
     */
    IMPORT_C virtual void HandleGetDevicesComplete( TInt aErr, 
                                                     CBTDeviceArray* aDeviceArray );

    };


/**
 *  Class CBTEngDevMan
 *
 *  This is a helper class that simplifies the usage 
 *  of Symbian's BT Device Registry interface.
 *
 *  @lib btengdevman.dll
 *  @since S60 v3.0
 */
class CBTEngDevMan : public CActive
    {

public:

    /** States enumeration */
    enum TState
        {
        EStateIdle,
        EStateAddDevice,
        EStateGetDevices,
        EStateModifyDevice,
        EStateDeleteDevices
        };

    /** Substates enumeration for modifying device parameters */
    enum TModifyState
        {
        ECheckNone,
        ECheckPairing,
        ECheckDevName,
        ECheckFriendlyName,
        ECheckNameless,
        ECheckFinal
        };

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface for informing 
     *                  the result of the operation. If NULL, the GetDevices
     *                  operation will operate syncrhonously; the other 
     *                  operations are then not available.
     * @return Pointer to the constructed CBTEngDevMan object.
     */
    IMPORT_C static CBTEngDevMan* NewL( MBTEngDevManObserver* aObserver );

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface for informing 
     *                  the result of the operation. If NULL, the GetDevices
     *                  operation will operate syncrhonously; the other 
     *                  operations are then not available.
     * @return Pointer to the constructed CBTEngDevMan object.
     */
    IMPORT_C static CBTEngDevMan* NewLC( MBTEngDevManObserver* aObserver );

    /**
     * Destructor
     */
    virtual ~CBTEngDevMan();

    /**
     * Add a device into the Bluetooth device registry.
     *
     * @since S60 v3.0
     * @param aDevice Device to be added.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt AddDevice( const CBTDevice& aDevice );

    /**
     * Get an array of devices that match the search pattern 
     * from the registry.
     * If no callback interface has been passed during construction, 
     * this function will complete synchronously.
     *
     * @since S60 v3.0
     * @param aCriteria Search criteria for the devices to be returned.
     * @param aDevice Array owned by the client in which the result 
     *                will be returned. Ownership is not transferred, 
     *                so his pointer must remain valid until the callback 
     *                is made (for the asyncronous version). If no callback
     *                interface has been supplied, the array will contain the
     *                results, if any.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt GetDevices( const TBTRegistrySearch& aCriteria, 
                               CBTDeviceArray* aResultArray );

    /**
     * Delete one or more devices that match the search pattern from the
     * registry.
     *
     * @since S60 v3.0
     * @param aCriteria Search criteria for the devices to be deleted.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt DeleteDevices( const TBTRegistrySearch& aCriteria );

    /**
     * Modify a device in the Bluetooth device registry.
     *
     * @since S60 v3.2
     * @param aDevice Device to be modifed.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt ModifyDevice( const CBTDevice& aDevice );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v3.0
     * @param aObserver Pointer to callback interface for informing 
     *                  the result of the operation.
     */
    CBTEngDevMan( MBTEngDevManObserver* aObserver );

    /**
     * Symbian 2nd-phase constructor
     *
     * @since S60 v3.0
     */
    void ConstructL();

// from base class CActive

    /**
     * When the Active Object completes succesfully, the callback is called
     * with the return value.
     *
     * @since S60 v3.0
     */
    void RunL();

    /**
     * If an error occurs, the callback is called with the error code.
     *
     * @since S60 v3.0
     */
    TInt RunError( TInt aError );

    /**
     * Cancel current outstanding operation, if any.
     *
     * @since S60 v3.0
     */
    void DoCancel();

    /**
     * Processes the result of a registry search.
     *
     * @since S60 v3.0
     */
    void DoModifyDeviceL();

private: // data

    /**
     * Keeps track of the current state.
     */
    TState iState;

    /**
     * Keeps track of sequence of modifications.
     */
    TModifyState iModifyState;

    /**
     * Used to mark when a view is being created.
     */
    TBool iCreatingView;

    /**
     * Registry access session.
     */
    RBTRegServ iRegServ;

    /**
     * Subsession on the BT Registry Server.
     */
    RBTRegistry iRegistry;

    /**
     * Holds the response from the registry query.
     * Own.
     */
    CBTRegistryResponse* iResponse;

    /**
     * Reference to modified device to be stored in registry.
     * own.
     */
    CBTDevice* iModifiedDevice;

    /**
     * Nested active scheduler loop for synchronous GetDevice operation.
     * own.
     */
    CActiveSchedulerWait* iAsyncWaiter;

    /**
     * Reference to the device array in which the results will be stored.
     * Not own.
     */
    CBTDeviceArray* iResultArray;

    /**
     * Observer class which implements the method that is called when the
     * request is completed.
     * Not own.
     */
    MBTEngDevManObserver* iObserver;

    };

// BTENGDEVMAN_H
#endif
