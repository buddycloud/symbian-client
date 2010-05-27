/*
* ============================================================================
*  Name        : BTEngDevMan.h
*  Part of     : Bluetooth Engine / BTEng
*  Description : The public Bluetooth Engine Device Management API
*  Version     : %version: 3 % << Don't touch! Updated by Synergy at check-out.
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

#ifndef BTENG_DEVMAN_API_H
#define BTENG_DEVMAN_API_H

#include <btmanclient.h>

/**
 * Class MBTEngDevManObserver
 *
 * Mix-in class for calling back to CBTEngDevMan observer when device handling
 * complete.
 *
 *  @lib bteng.dll
 *  @since S60 V3.0
 */
class MBTEngDevManObserver
    {
public:

    /**
     * Indicates to the caller that adding a device has completed.
     *
     * @since  S60 v3.0
     * @param  aErr      Status information, if there is error.
     * @return none
     */
    IMPORT_C virtual void HandleAddDeviceComplete(TInt aErr);

    /**
     * Indicates to the caller that getting an array of devices has completed.
     *
     * @since  S60 v3.0
     * @param  aErr          Status information, if there is error.
     * @param  aDeviceArray  Array of devices that match the given criteria (empty if an error occurred)
     * @return none
     */
    IMPORT_C virtual void HandleGetDevicesComplete(TInt aErr, const RBTDeviceArray& aDeviceArray);

    /**
     * Indicates to the caller that deleting devices has completed.
     *
     * @since  S60 v3.0
     * @param  aErr      Status information, if there is error.
     * @return none
     */
    IMPORT_C virtual void HandleDeleteDevicesComplete(TInt aErr);
    };

/**
 *  Class CBTEngDevMan
 *
 *  This is a helper class that simplifies the usage of Symbian's BT Device
 *  Registry interface
 *
 *  @lib bteng.dll
 *  @since S60 v3.0
 */
NONSHARABLE_CLASS(CBTEngDevMan) : public CActive
    {
public:

    /** States enumeration */
    enum TState
        {
        EStateIdle,
        EStateAddDevice,
        EStateGetDevices,
        EStateDeleteDevices
        };

    IMPORT_C static CBTEngDevMan* NewL(MBTEngDevManObserver* aObserver);
    IMPORT_C static CBTEngDevMan* NewLC(MBTEngDevManObserver* aObserver);

    virtual ~CBTEngDevMan();

    /**
     * Add a device into the Bluetooth device registry
     *
     * @since S60 v3.0
     * @param aDevice Device to be added.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt AddDevice(const CBTDevice& aDevice);

    /**
     * Get an array of devices that match the search pattern from the
     * registry.
     *
     * @since S60 v3.0
     * @param aDevice   An array of devices where the information will be returned.
     *                  This pointer must remain valid until the callback is made.
     * @param aCriteria Search criteria for the devices to be returned.
     * @return KErrNone if the request was succesful,
     *         KErrBusy if there is already an ongoing request or
     *         another error code in case of failure.
     */
    IMPORT_C TInt GetDevices(const TBTRegistrySearch& aCriteria);

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
    IMPORT_C TInt DeleteDevices(const TBTRegistrySearch& aCriteria);

protected:

// from base class CActive

    /**
     * When the Active Object completes succesfully, the callback is called
     * with the return value.
     *
     * @since S60 v3.0
     * @param none
     * @return none
     */
    void RunL();

    /**
     * If an error occurs, the callback is called with the error code.
     *
     * @since S60 v3.0
     * @param none
     * @return none
     */
    TInt RunError(TInt aError);

    /**
     * Cancel current outstanding operation, if any.
     *
     * @since S60 v3.0
     * @param none
     * @return none
     */
    void DoCancel();

private:

    CBTEngDevMan();

    void ConstructL(MBTEngDevManObserver* aObserver);

private: // data

    /**
     * Registry access session
     */
    RBTRegServ iRegServ;

    /**
     * Subsession on the BT Registry Server
     */
    RBTRegistry iRegistry;

    /**
     * Keeps track of the current state
     */
    TState iState;

    /**
     * Used to mark when a view is being created
     */
    TBool iView;

    /**
     * Holds the response from the registry query
     */
    CBTRegistryResponse* iResponse;

    /**
     * Observer class which implements the method that is called when the
     * request is completed.
     * Not own.
     */
    MBTEngDevManObserver* iObserver;

    };

#endif // BTENG_DEVMAN_API_H
