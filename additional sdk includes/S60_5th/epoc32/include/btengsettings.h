/*
* ============================================================================
*  Name        : btengsettings.h
*  Part of     : Bluetooth Engine / Bluetooth Settings
*  Description : Bluetooth Engine API for managing device settings.
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

#ifndef BTENGSETTINGS_H
#define BTENGSETTINGS_H

#include <btengdomaincrkeys.h>
#include <btengconstants.h>

class CBase;
class CBTEngSettingsNotify;


/**
 *  Class MBTEngSettingsObserver
 *
 *  Callback interface for receiving events related to changes 
 *  in Bluetooth power and discoverability mode settings.
 *
 *  @lib btengsettings.lib
 *  @since S60 v3.2
 */
class MBTEngSettingsObserver
    {

public:

    /**
     * Provides notification of changes in the power state 
     * of the Bluetooth hardware.
     *
     * @since S60 v3.2
     * @param aState EBTPowerOff if the BT hardware has been turned off, 
     *               EBTPowerOn if it has been turned off.
     */
    virtual void PowerStateChanged( TBTPowerStateValue aState ) = 0;

    /**
     * Provides notification of changes in the discoverability 
     * mode of the Bluetooth hardware.
     *
     * @since S60 v3.2
     * @param aState EBTDiscModeHidden if the BT hardware is in hidden mode, 
     *               EBTDiscModeGeneral if it is in visible mode.
     */
    virtual void VisibilityModeChanged( TBTVisibilityMode aState ) = 0;

    };


/**
 *  Class CBTEngSettings
 *
 *  This class provides functionality for 
 *  getting and setting Bluetooth settings.
 *
 *  @lib btengsettings.lib
 *  @since S60 v3.2
 */
class CBTEngSettings : public CBase
    {

public:

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  settings change events.
     * @return Pointer to the constructed CBTEngSettings object.
     */
    IMPORT_C static CBTEngSettings* NewL( MBTEngSettingsObserver* aObserver = NULL );

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  settings change events.
     * @return Pointer to the constructed CBTEngSettings object.
     */
    IMPORT_C static CBTEngSettings* NewLC( MBTEngSettingsObserver* aObserver = NULL );

    /**
     * Destructor
     */
    virtual ~CBTEngSettings();

    /**
     * Gets the Bluetooth power state (on or off).
     *
     * @since S60 v3.2
     * @param aState On return, contains the current power mode.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt GetPowerState( TBTPowerStateValue& aState );

    /**
     * Sets the Bluetooth power state (on or off). If a callback interface has 
     * been provided, the result is communicated through PowerStateChanged() 
     * when to power state change has been fully completed.
     *
     * @since S60 v3.2
     * @param aState Power mode to be set.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt SetPowerState( TBTPowerStateValue aState );

    /**
     * Gets Bluetooth visibility mode  (hidden, 
     * limited or general discoverable mode).
     *
     * @since S60 v3.2
     * @param aMode On return, contains the current visibility mode.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt GetVisibilityMode( TBTVisibilityMode& aMode );

    /**
     * Sets Bluetooth visibility mode  (hidden, 
     * limited or general discoverable mode).
     *
     * @since S60 v3.2
     * @param aMode Discoverability mode to be set to Central Repository.
     * @param aTime Time during which the phone remains in general 
     *              discoverable state  (in seconds), only used with 
     *              EBTDiscModeTemporary  (otherwise ignored).
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt SetVisibilityMode( TBTVisibilityMode aMode, TInt aTime = 0 );

    /**
     * Gets Bluetooth local name. If the name has not been set, a 
     * zero-lenght name is returned.
     *
     * @since S60 v3.2
     * @param aMode On return, contains the current local name.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt GetLocalName( TDes& aName );

    /**
     * Sets Bluetooth local name.
     *
     * @since S60 v3.2
     * @param aMode Local mode to be set to Central Repository.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    IMPORT_C TInt SetLocalName( const TDes& aName );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v3.2
     * @param aObserver Pointer to callback interface that receives 
     *                  settings change events.
     */
    CBTEngSettings( MBTEngSettingsObserver* aObserver );

    /**
     * Symbian 2nd-phase constructor
     *
     * @since S60 v3.2
     */
    void ConstructL();

    /**
     * Helper function to get the status from Central Repository of 
     * whether the local name has been modified.
     *
     * @since S60 v3.2
     * @param aStatus On return, contains the current status of the local name.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    TInt GetLocalNameModified( TBool& aStatus );

    /**
     * Helper function for reading a Central Repository key, and 
     * verifying that the value is within the appropriate range.
     *
     * @since S60 v3.2
     * @param aUid UID of the key to be read.
     * @param aKey The key to be read.
     * @param aValue Default value of the key; on return contains
     *               the value as read from CenRep.
     * @param aMinRange1 Lower limit of the first valid range of key values.
     * @param aMaxRange1 Upper limit of the first valid range of key values.
     * @param aMinRange2 Lower limit of the second valid range of key values.
     * @param aMaxRange2 Upper limit of the second valid range of key values.
     */
    void GetCenRepKeyL( const TUid aUid, const TInt aKey, TInt& aValue, 
                         const TInt aMinRange1, const TInt aMaxRange1,
                         const TInt aMinRange2 = 0, const TInt aMaxRange2 = 0 );

    /**
     * Helper function for getting/setting BT local name in BT Registry.
     *
     * @since S60 v3.2
     * @param aName The BT local device name to be read or written. If it 
     *              contains an empty string, it will hold the name on return,
     *              otherwise the contained string will be set to BT Registry.
     * @return KErrNone on success, otherwise a system wide error code.
     */
    TInt HandleBTRegistryNameSetting( TDes& aName );

private: // data

    /**
     * Timer for temporary discoverable mode.
     * Own.
     */
    CBTEngSettingsNotify* iSettingsWatcher;

    /**
     * Observer for changes in BT settings.
     * Not own.
     */
    MBTEngSettingsObserver* iObserver;

    };

// BTENGSETTINGS_H
#endif
