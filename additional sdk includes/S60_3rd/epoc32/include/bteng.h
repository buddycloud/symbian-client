/*
* ============================================================================
*  Name     : BTEng.h from BTEng.h
*  Part of  : BTEng
*
*  Description:
*     The public Bluetooth UI engine API.
*  Version: 45
*
*  Copyright (C) 2002 Nokia Corporation.
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

#ifndef BTENGAPI_H
#define BTENGAPI_H

#include <btmanclient.h>
#include <btdevice.h>
#include <bt_sock.h>
#include <btsdp.h>
#include <bttypes.h>

// Enumeration for BT discoverability mode setting
// This must always match BTServer's TBTServerScanMode enum
enum TBTDiscoverabilityMode
    {
    // This values are defined in the BT Core specs
    EBTDiscModeHidden  = 0x02,   // Page enabled, scan disabled
    EBTDiscModeGeneral = 0x03,   // Page enabled, scan enabled

    // This value is used to set the device to EBTServerScanModeGeneral temporarily
    EBTDiscModeTemporary = 0x100 // First value after reserved range
    };

// Enumeration for BT search mode setting.
enum TBTSearchMode
    {
    EBTSearchModeGeneral,
    EBTSearchModeLimited
    };

enum TBTSapMode
    {
    EBTSapDisabled,
    EBTSapEnabled
    };

// SDP UUID Constants - Short form
// Taken from Bluetooth Profile specification v1.1
// These are used when registering the service to
// local SDP database and when searching the service
// information from remote device.
const TUint KBTSdpDun                   = 0x1103;
const TUint KBTSdpGenericTelephony      = 0x1204;
const TUint KBTSdpFax                   = 0x1111;
const TUint KBTSdpObjectPush            = 0x1105;
const TUint KBTSdpFileTransfer          = 0x1106;
const TUint KBTSdpHeadSet               = 0x1108;
const TUint KBTSdpHandsfree             = 0x111e;
const TUint KBTSdpHeadsetAudioGateway   = 0x1112;
const TUint KBTSdpHandsfreeAudioGateway = 0x111f;
const TUint KBTSdpGenericAudio          = 0x1203;
const TUint KBTSdpGenericNetworking     = 0x1201;
const TUint KBTSdpBasicImaging          = 0x111b;
const TUint KBTSdpBasicPrinting         = 0x1120;
const TUint KBTSdpSap                   = 0x112d;
const TUint KBTSdpAudioSource           = 0x110a;

// Security manager Uids
// These are used when registering/deregistering
// service to Security Manager.
// (These are identical to SDP constants but
// the meaning is not the same.)

const TUint KBTSecurityUidFax                   = 0x1111;
const TUint KBTSecurityUidDun                   = 0x1103;
const TUint KBTSecurityUidObjectPush            = 0x1105;
const TUint KBTSecurityUidFileTransfer          = 0x1106;
const TUint KBTSecurityUidHandsfreeAudioGateway = 0x111F;
const TUint KBTSecurityUidHeadsetAudioGateway   = 0x1112;

//HF attribute ID
const TUint KBTHFSupportedFeatures        = 0x0311;
const TUint KBTHSRemoteAudioVolumeControl = 0x0302;

//HW specific attributes
const TUint KBTHFDeviceModelNumber              = 0x0300;
const TUint KBTHFHWVersionNumber	            = 0x0301;
const TUint KBTHFSWVersionNumber	            = 0x0302;

// FORWARD DECLARATIONS
class MBTMCMSettingsCB;
class MBTSdpSearchObserver;
class CBTSdpDbHandler;
class MBTBondingObserver;
class CBTSdpResultReceiver;
class CSdpAgent;
class CSdpSearchPattern;
class CBTBondingHandler;
class CBTAsyncOp;

class CBTEngCenRepWatcher;
class CBTEngPubSubWatcher;

// ---------------------------------------------------------------------
// Class CBTMCMSettings
// This class provides the tools for setting states in local BT MCM.
// Users of this class have to implement mixin class MBTMCMSettingsCB.
// ---------------------------------------------------------------------
NONSHARABLE_CLASS(CBTMCMSettings) : public CBase
    {
    public:

        /**
        * Two-phased constructor.
        */
        IMPORT_C static CBTMCMSettings* NewL(MBTMCMSettingsCB* aCallBack = NULL);

        /**
        * Two-phased constructor.
        */
        IMPORT_C static CBTMCMSettings* NewLC(MBTMCMSettingsCB* aCallBack = NULL);


        /**
        * Destructor
        */
        virtual ~CBTMCMSettings();

        /**
        * Getter for all BT MCM settings.
        * @param  aPowerState Reference to boolean value where power state is updated.
        * @param  aDiscoverabilityMode Reference to discoverability mode where current state is fetched.
        * @param  aSearchMode Reference to search mode where current state is fetched.
        * @param  aLocalName Reference to descriptor where local name is fetched.
        * @param  aNameModifiedFlag Reference to boolean value where local name modification flag is updated.
        * @return TInt KErrNone if successful otherwise one of system wide error codes
        */
        IMPORT_C static TInt GetAllSettings(TBool& aPowerState,
                                            TBTDiscoverabilityMode& aDiscoverabilityMode,
                                            TBTSearchMode& aSearchMode,
                                            TDes& aLocalName,
                                            TBool& aNameModifiedFlag);

        /**
        * Getter for all BT MCM settings.
        * @param  aPowerState Reference to boolean value where power state is updated.
        * @param  aDiscoverabilityMode Reference to discoverability mode where current state is fetched.
        * @param  aSearchMode Reference to search mode where current state is fetched.
        * @param  aLocalName Reference to descriptor where local name is fetched.
        * @param  aNameModifiedFlag Reference to boolean value where local name modification flag is updated.
        * @param  aBTSapEnabled Reference where the current BTSap state is saved
        * @return TInt KErrNone if successful otherwise one of system wide error codes
        */
        IMPORT_C static TInt GetAllSettings(TBool& aPowerState,
                                            TBTDiscoverabilityMode& aDiscoverabilityMode,
                                            TBTSearchMode& aSearchMode,
                                            TDes& aLocalName,
                                            TBool& aNameModifiedFlag,
                                            TBTSapMode& aBTSapEnabled);

        /**
        * Gets the status of Local name modification flag
        * from Shared Data.
        * Parameters:
        * @param aStatus  EFalse = System default name only exists in MCM.
        *                 ETrue  = User has given a new local device name.
        * @return TInt    Error indicating the success of call.
        */
        IMPORT_C static TInt IsLocalNameModified(TBool& aStatus);

        /**
        * Setter for local BT name.
        * MCM then responds to remote name requests with this name.
        * Parameters:
        * @param aName    Reference to descriptor holding the BT name.
        * @return TInt    Error indicating the success of call.
        */
        IMPORT_C TInt SetLocalBTName(const TDesC& aName);

        /**
        * Getter for local BT name.
        * This method reads the local name from Shared Data
        * and returns it to user. If name is not found from Shared
        * Data, then it is fetched from BT MCM.
        * Parameters:
        * @param aName     BT Name from Shared Data.
        * @return TInt     Error value from Shared Data call.
        */
        IMPORT_C static TInt GetLocalBTName(TDes& aName);


        /**
        * Opens L2CAP socket and sets Discoverability Mode for MCM
        * according to the mapping table:
        *
        * aMode                    | Page | Inquiry | Limited |
        * =====================================================
        * EDiscModeHidden          | OFF  | OFF     | N/A     |
        * EDiscModeLimited         | ON   | ON      | ON      |
        * EDiscModeGeneral         | ON   | ON      | OFF     |
        * =====================================================
        * Parameters:
        * @param aMode     Discoverability mode to be set to MCM
        *                  and Shared Data.
        * @param aServer   Boolean indicating if BT server has made
        *                  the call (means that no session with BT
        *                  server is enabled.
        *                  Default value is EFalse --> server is not caller.
        * @param aTime     time during which the phone remains in visible state (in microsends)
        *                  only used with EBTDiscModeTemporary (otherwise ignored)
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SetDiscoverabilityModeL(TBTDiscoverabilityMode aMode, TInt aTime = 0);

        /**
        * Gets Discoverability Mode setting from Shared Data
        * and returns that setting via referenced parameter aMode.
        * Parameters:
        * @param aMode     Functions returns discoverability mode via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C static TInt GetDiscoverabilityMode(TBTDiscoverabilityMode& aMode);

        /**
        * Setter for BT search mode.
        * This setting is stored only on Shared Data.
        * Search mode can be general or limited.
        * Parameters:
        * @param aMode     Search mode to be set to shared data.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SetSearchMode(TBTSearchMode aMode);

        /**
        * Getter for BT search mode.
        * Function reads the setting from Shared Data.
        * Search mode can be general or limited.
        * Parameters:
        * @param aMode     Search mode to be read from shared data and put into this reference.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C static TInt GetSearchMode(TBTSearchMode& aMode);

        /**
        * Setter for BT MCM power mode
        * With this function power is turned on/off
        * and BT services are enabled/disabled. Also if power is turned off
        * and there are active baseband connections, those will be disconnected.
        * Parameters:
        * @param aMode     Power mode to be set.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SetPowerState(TBool aState);

        /*
        * Getter for BT MCM power mode
        * Returns the Power value from Shared data.
        * Parameters:
        * @param aMode     Functions returns power mode via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C static TInt GetPowerState(TBool& aValue);

        /*
        * Setter for BT SAP enable state
        * Parameters:
        * @param aEnable     enable state to be set
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt EnableBTSap(TBTSapMode aEnable);

        /*
        * Getter for BT SAP enable state
        * Returns the enable state value from Shared data.
        * Parameters:
        * @param aBTSapEnabled     Functions returns enable state via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C static TInt GetBTSapEnableState(TBTSapMode& aBTSapEnabled);

        /*
        * Getter for shared data BDAddress
        * Returns the BDAddress of the BT HF Accessory
        * from shared data.
        * Parameters:
        * @param aAddr     Function returns BDAddress via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetAccessoryAddr(TBTDevAddr& aAddr);
        
        /* 
        * Getter for connection status
        * Returns the connection status from shared data.
        * Parameters:
        * @param aAddr     Function returns connection status via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetConnectionStatus(TBool& aStatus);
        
        /* 
        * Getter for  wired accessory connection status
        * Returns the status from shared data.
        * Parameters:
        * @param aAddr     Function returns connection status via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt IsWiredAccConnected(TBool& aAnswer);
        
        /* 
        * Getter for call status
        * Returns the call status from shared data.
        * Parameters:
        * @param aAddr     Function returns call status via this reference
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt IsCallOngoing(TBool& aAnswer);

        /**
        * Sets default values to Shared Data and MCM.
        * Parameters:
        * @param none
        * @return TInt       KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SetDefaultValuesL();

    private:

        /**
        * C++ default constructor.
        */
        CBTMCMSettings(MBTMCMSettingsCB* aCallBack);

        /**
        * By default Symbian OS constructor is private.
        */
        void ConstructL();
        
       /**
        * Writes local BT name to chip
        * @param aName the new local BT name
        * @return KErrNone if OK, else value indicating error situation.
        */
        TInt SetLocalBTNameToMCM();
        
    private:

        MBTMCMSettingsCB*   iCallBack;             // Reference to parent
        CBTEngCenRepWatcher* iCenRepWatcher;
        CBTEngPubSubWatcher* iPubSubWatcher;
    };

// ---------------------------------------------------------------------
// Class MBTMCMSettingsCB
// This is the mixin-class to provide way for CBTMCMSettings to make
// callbacks to parent.
// User of the CBTMCMSettings must implement this class to enable
// callbacks.
// ---------------------------------------------------------------------

class MBTMCMSettingsCB
    {
    public:
        /**
        * Pure virtual function to provide a way to notify parent instance
        * of Discoverability Mode changes.
        * Parameters:
        * @param aMode     Current discoverability mode.
        * @return none
        */
        virtual void DiscoverabilityModeChangedL(TBTDiscoverabilityMode aMode) = 0;

        /**
        * Pure virtual function to provide a way to notify parent instance
        * of Power Mode changes.
        * Parameters:
        * @param aState    Current power mode state
        * @return none
        */
        virtual void PowerModeChangedL(TBool aState) = 0;

        /**
        * Pure virtual function to provide a way to notify parent instance
        * of Audio accessory changes.
        * Parameters:
        * @param aAddr      Bluetooth address
        * @return none
        */
        virtual void BTAAccessoryChangedL (TBTDevAddr& aAddr) = 0; 

        /**
        * Pure virtual function to provide a way to notify parent instance
        * of Audio connection changes.
        * Parameters:
        * @param aStatus        ETrue if we have audio connection
        * @return none
        */
        virtual void BTAAConnectionStatusChangedL(TBool& aStatus) = 0; 
    };

// ---------------------------------------------------------------------
// Class RBTDeviceHandler
// This class encapsulates a use of BT registry to store BT device
// information.
// ---------------------------------------------------------------------

class RBTDeviceHandler
    {
    public:

        /**
        * Adds device into Bluetooth registry
        * Parameters:
        * @param aDevice   Device that will be added to registry.
        * Return value:
        * @ TInt       KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt AddDeviceIntoRegistryL(CBTDevice& aDevice);

        /**
        * Returns BT Device from registry.
        * Parameters:
        * @param aDevice  a device to be filled from BT Registry.
        * @param aAddr  the BT device address of the interested device.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetDeviceFromRegistryL(CBTDevice& aDevice, const TBTDevAddr& aAddr);

        /**
        * Returns BT Devices from registry.
        * Parameters:
        * @param aDevices  List of devices to be filled from BT Registry.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetDevicesFromRegistryL(CBTDeviceArray& aDevices);

        /**
        * Returns paired BT Devices from registry.
        * Parameters:
        * @param aDeviceArray  List of devices to be filled from BT Registry.
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetPairedDevicesL(CBTDeviceArray& aDeviceArray);

        /**
        * Deletes device from BT Registry.
        * Device to be deleted is found according to BT Address.
        * Parameters:
        * @param aDevice   Device which will be deleted from registry
        *                  (according to BD Address).
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt DeleteDeviceFromRegistryL(CBTDevice& aDevice);

        /**
        * Modifies device in registry.
        * Device to be modified is found according to BT address.
        * Parameters:
        * @param aDevice   Device which parameters will be modified
        *                  (according to BD Address).
        * @return TInt     KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt ModifyDeviceInRegistryL(CBTDevice& aDevice);

        /**
        * Sets device Global Security settings.
        * Device can be ordered to be authenticated (UI dialog,
        * authorized (UI dialog) or encrypted (non-UI)
        * Overwrites service level security settings.
        * If security parameter values are set to EFalse
        * service level security will be main security.
        * Parameters:
        * @param aDevice       Device which security issues will be modified.
        * @param aAuthenticate Need for authentication for the device connection.
        * @param aAuthorise    Need for authorisation for the device connection.
        * @param aEncrypt      Need for encryption for the device connection.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt SetDeviceSecurityL(CBTDevice& aDevice, TBool aAuthenticate, TBool aAuthorise, TBool aEncrypt);

        /**
        * Cleans the trust device flag from all the devices in BT Registry.
        * This is used when restore factory settings is done.
        * Parameters:
        * @param none
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt RemoveTrustFromDevicesL();

        /**
        * Deletes BT Devices from BT Registry which do not have
        * valid link key.
        * Parameters:
        * @param none
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt DeleteDevicesWithoutLinkKeyL();

        /**
        * Deletes all devices from BT Registry.
        * Parameters:
        * @param none
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt DeleteAllDevicesL();

        /**
        * Checks if the given device has baseband connection.
        * Parameters:
        * @param aDevice       Device to be checked.
        * @param aStatus       Boolean indicating the status of baseband connection.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt IsDeviceConnectedL(const CBTDevice& aDevice, TBool& aConnected);

        /**
        * Checks if there exists baseband connections.
        * Parameters:
        * @param aStatus       Boolean indicating the status of baseband connection.
        *                      ETrue if at least one baseband connection exists.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt AreExistingConnectionsL(TBool& aExisting);

        /**
        * Closes ACL Connection with given device.
        * Also closes SCO connection if one exists for the device.
        * Parameters:
        * @param aDevice       Device to be disconnected.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt CloseBasebandConnectionL(const CBTDevice& aDevice);

        /**
        * Closes all Baseband connections. After this device should
        * be connection-free.
        * Parameters:
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C  TInt CloseAllBasebandConnectionsL();

        /**
        * Returns an array of BT Devices (including only BD Addresses)
        * which are connected to this device.
        * If there are no connections, KErrNotFound is returned.
        * Parameters:
        * @param aDevices      List of devices (including BD Address) which are connected.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        *                      KErrNotFound if there are no connections.
        */
        IMPORT_C  TInt GetOpenBasebandConnectionsL(CBTDeviceArray& aDevices);

        IMPORT_C  void GetOpenBasebandConnectionsL(RBTDevAddrArray& aAddrArray);

       /**
        * Try to reform the current topology in preparation of a new connection.
        * @return   TInt indicating the success of call.
        */
        IMPORT_C  TInt OptimizeTopology();
        
    private:

        /**
        * Adds a device into device array into right place.
        * Place is defined by Friendly Name and if not set, Device Name
        * with CompareF method.
        * Parameters:
        * @param aDevice       Device to be inserted into array.
        * @param aDeviceArray  Device array where device is inserted to.
        * @return none
        */
         void SortDeviceArrayL(RBTDeviceArray& aSrcArray, CBTDeviceArray& aDesArray);
         
        /**
        * Closes ACL Connection with given device.
        * Also closes SCO connection if one exists for the device.
        * Parameters:
        * @param aBTDevAddr    The BT address of the device to be disconnected.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
         TInt CloseBasebandConnectionL(const TBTDevAddr& aBTDevAddr);
    };

/**
* Class MSDPSearchObserver
* Mixin-class for SDP search callbacks. Implemented by CBTConnection-class.
*/
class MBTSdpSearchObserver
    {
    public:

        /**
        * Indicates caller that SDP search has finished.
        * Parameters:
        * @param aStatus   Status information, if there is error.
        *                  KErrNone if channel info received correctly.
        * @return none
        */
        virtual void SearchComplete(TInt aStatus) = 0;

        /**
        * Returns a parsed attribute value to caller instance.
        * Parameters:
        * @param aResult   Attribute value
        * @param aStatus   Status information, if there is error.
        *                  KErrNone if channel info received correctly.
        * @return none
        */
        virtual void ReturnAttributeValue(TUint aResult, TInt aStatus) = 0;
    };

// ---------------------------------------------------------------------
// Class MBTBondingObserver
// Mixin-class for Bonding callback.
// ---------------------------------------------------------------------
class MBTBondingObserver
    {
    public:

        /**
        * Indicates that bonding procedure is complete.
        * Parameters:
        * @param aStatus   Status information, if there is error.
        *                  KErrNone if channel info received correctly.
        * @return none
        */
        virtual void BondingComplete(TInt aStatus) = 0;
    };

// ---------------------------------------------------------------------
// Class CBTConnection
// This class contains functionality to establish BT connections
// and receive BT connections. Includes functionality to search
// remote services (SDP) and register local services and security issues.
// ---------------------------------------------------------------------

NONSHARABLE_CLASS(CBTConnection) : public CBase, 
    public MBTSdpSearchObserver
    {
    public:

        /**
        * Two-phase constructor
        */
        IMPORT_C static CBTConnection* NewL();

        /**
        * Two-phase constructor
        */
        IMPORT_C static CBTConnection* NewLC();

        /**
        * Destructor.
        */
        virtual ~CBTConnection();

        /**
        * Method which invokes Search Dialog and returns CBTDevice from call.
        * Device search can be filtered (default filtering off)
        * with aServiceClass parameter.
        * Returned CBTDevice instance includes BDAddress, DeviceName and Class Of Device.
        * Parameters:
        * @param aDevice       Device, which is returned from device search.
        * @param aService      Service filter for device search.
        *                      can filter e.g. object transfer cabable
        *                      devices according to Major Service class.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SearchRemoteDeviceL(CBTDevice& aDevice,
                                           TBTMajorServiceClass aServiceClass);

        /**
        * Method which invokes Search Dialog and returns CBTDevice from call.
        * Returned CBTDevice instance includes BDAddress, DeviceName and Class of Device.
        * Parameters:
        * @param aDevice       Device, which is returned from device search.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SearchRemoteDeviceL(CBTDevice& aDevice);

        /**
        * Method which makes SDP query to remote device
        * and returns RFCOMM channel where required service is located.
        * Note! At this point this does only Protocol attribute search
        * to a given service.
        * Parameters:
        * @param aDevice       Device where SDP query is made to.
        * @param aService      Service which is searched.
        * @param aChannel      RFCOMM Channel to be returned.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SearchRemoteChannelL(const CBTDevice& aDevice,
                                            TUint aService,
                                            TUint& aChannel);

        /**
        * Method which makes SDP query to remote device
        * and returns RFCOMM channel where required service is located.
        * Note! At this point this does only Protocol attribute search
        * to a given service.
        * Parameters:
        * @param aDevice       Device where SDP query is made to.
        * @param aService      Service defined as DesC to be searched
        * @param aChannel      RFCOMM Channel to be returned.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
    	IMPORT_C TInt SearchRemoteChannelL(const CBTDevice& aDevice,
    										const TDesC8& aService,
    										TUint& aChannel);

        /**
        * Asynchronous version of the Service search.
        * Parameters:
        * @param aDevice       Device where SDP query is made to.
        * @param aService      Service which is searched.
        * @param aChannel      RFCOMM Channel to be returned.
        * @param aStatus       Reference to TRequestStatus.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt SearchRemoteChannelL(const CBTDevice& aDevice,
                                            TUint aService,
                                            TUint& aChannel,
                                            TRequestStatus& aStatus);

        /**
        * DEPRECATED!
        * 
        * Method which registers Security settings to Security Manager.
        * Returns value according to succession.
        * Protocols (according to bt_sock.h definitions)
        * HCI              KSolBtHCI
        * Link Manager     KSolBtLM
        * L2CAP            KSolBtL2CAP
        * RFCOMM           KSolBtRFCOMM
        *
        * Parameters:
        * @param aService      Service which is registered.
        * @param aProtool      Protocol where the service is based on.
        * @param aChannel      RFCOMM Channel to be registered.
        * @param aAuthenticate Authentication enabled TRUE/FALSE
        * @param aAuthorise    Authorisation enabled TRUE/FALSE
        * @param aEncrypt      Encryption enabled TRUE/FALSE
        * @return TInt         KErrNotSupported
        * @@deprecated
        */

        IMPORT_C TInt RegisterSecuritySettingsL(TUint aService,
                                                 TInt  aProtocol,
                                                 TUint  aChannel,
                                                 TBool aAuthenticate,
                                                 TBool aAuthorise,
                                                 TBool aEncrypt);

        /**
        * DEPRECATED!
        * 
        * Method which unregisters Security settings for given service
        * to Security Manager
        * Parameters:
        * @param aService      Service which is unregistered.
        * @return TInt         KErrNotSupported
        * @deprecated
        */
        IMPORT_C TInt UnregisterSecuritySettingsL(TUint aService);

        /**
        * Method which registers Service settings
        * to SDP NetDatabase to allow remote
        * clients search for service.
        * Parameters:
        * @param aService      Service which is registered.
        * @param aChannel      RFCOMM Channel to be registered.
        * @param aHandle       Handle to registered service record.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt RegisterSDPSettingsL(TUint aService,
                                            TUint aChannel,
                                            TSdpServRecordHandle& aHandle);

    	
    	/**
        * Method, which registers Service settings
        * to SDP NetDatabase to allow remote
        * clients search for service.
        * Parameters:
        * @param aService      Service which is registered. 128 bits format.
        * @param aChannel      RFCOMM Channel to be registered.
        * @param aHandle       Handle to registered service record.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
    	IMPORT_C TInt RegisterSDPSettingsL(const TDesC8& aService,
                                            TUint aChannel,
                                            TSdpServRecordHandle& aHandle);

        /**
        * Method which removes SDP registration from local SDP database.
        * Note! Unregistration can only be done with registered handle!
        * Parameters:
        * @param aHandle       Handle to service record to be unregistered.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt UnregisterSDPSettingsL(TSdpServRecordHandle aHandle);

        /**
        * Method which reserves local RFCOMM channel (from possible channels 1-30)
        * and returns it to client.
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aListenSocket Socket which is initiated for listening.
        * @param aChannel      RFCOMM channel which is reserved for listening.
        * @param aAuthenticate Authentication enabled TRUE/FALSE
        * @param aAuthorise    Authorisation enabled TRUE/FALSE
        * @param aEncrypt      Encryption enabled TRUE/FALSE
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ReserveLocalChannelL(RSocketServ& aSocketServer,
                                            RSocket& aListenSocket,
                                            TUint& aChannel,
                                            TBool aAuthenticate,
                                            TBool aAuthorise,
                                            TBool aEncrypt);

        /**
        * Method which reserves local RFCOMM channel (from possible channels 1-30)
        * and returns it to client.
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aListenSocket Socket which is initiated for listening.
        * @param aChannel      RFCOMM channel which is reserved for listening.
        * @param aAuthenticate Authentication enabled TRUE/FALSE
        * @param aAuthorise    Authorisation enabled TRUE/FALSE
        * @param aEncrypt      Encryption enabled TRUE/FALSE
        * @param aPasskeyLen   Minimum passkey length required on this channel
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ReserveLocalChannelL(RSocketServ& aSocketServ,
                                           RSocket& aListenSocket,
                                           TUint& aChannel,
                                           TBool aAuthenticate,
                                           TBool aAuthorise,
                                           TBool aEncrypt,
                                           TUint aPasskeyLen);

        /**
        * Method which reserves local RFCOMM channel (from possible channels 1-30)
        * and returns it to client.
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aListenSocket Socket which is initiated for listening.
        * @param aChannel      RFCOMM channel which is reserved for listening.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ReserveLocalChannelL(RSocketServ& aSocketServer,
                                            RSocket& aListenSocket,
                                            TUint& aChannel);

        /**
        * Method which releases listening socket.
        * Note! Socket has to be in listening state!
        * Parameters:
        * @param aListenSocket Socket which is initiated for listening.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ReleaseLocalChannelL(RSocket& aListenSocket);

        /**
        * Method which opens socket to SocketServer, which
        * reference is given to an instance and starts to listen to
        * that socket. After remote client makes connection request
        * to that socket, function returns to caller.
        * Error situations are handled with return value <> 0.
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aDataSocket   Socket where incoming connection is accepted.
        * @param aListenSocket Socket which is initiated for listening.
        * @param aChannel      RFCOMM channel which is reserved for listening.
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt PassiveConnectSocketL(RSocketServ& aSocketServer,
                                            RSocket& aDataSocket,
                                            RSocket& aListenSocket,
                                            TUint& aChannel,
                                            TRequestStatus& aStatus);

        /**
        * Method which cancels the outstanding accept operation of a socket
        * Parameters:
        * @param aListenSocket Socket which is outstanding accept operation
        * @return
        */
        IMPORT_C void CancelPassiveConnectSocketL(RSocket& aListenSocket);

        /**
        * Method which opens socket to SocketServer, which
        * reference is given to an instance and starts a RFCOMM connection
        * request to that socket. After this functiona returns to caller.
        * Error values are indicated with return value <> 0
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aSocket       Socket where connection is made
        * @param aDevice       Remote device where connection is made.
        * @param aChannel      RFCOMM channel where connection is made.
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ActiveConnectSocketL(RSocketServ& aSocketServer,
                                           RSocket& aSocket,
                                           CBTDevice& aDevice,
                                           TUint aChannel,
                                           TRequestStatus& aStatus);

        /**
        * Method which opens socket to SocketServer, which
        * reference is given to an instance and starts a defined protocol connection
        * request to that socket. After this functiona returns to caller.
        * Error values are indicated with return value <> 0
        * Protocols (according to bt_sock.h definitions)
        * L2CAP            KSolBtL2CAP
        * RFCOMM           KSolBtRFCOMM
        * HCI              KSolBtHCI        (returns KErrNotSupported)
        * Link Manager     KSolBtLM         (returns KErrNotSupported)
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aSocket       Socket where connection is made
        * @param aProtool      Protocol where the service is based on.
        * @param aDevice       Remote device where connection is made.
        * @param aChannel      RFCOMM channel where connection is made.
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ActiveConnectSocketL(RSocketServ& aSocketServer,
                                           RSocket& aSocket,
                                           TInt  aProtocol,
                                           CBTDevice& aDevice,
                                           TUint aChannel,
                                           TRequestStatus& aStatus);

        /**
        * Method which opens socket to SocketServer, which
        * reference is given to an instance and starts a defined protocol connection
        * request to that socket. After this functiona returns to caller.
        * Error values are indicated with return value <> 0
        * Protocols (according to bt_sock.h definitions)
        * L2CAP            KSolBtL2CAP
        * RFCOMM           KSolBtRFCOMM
        * HCI              KSolBtHCI        (returns KErrNotSupported)
        * Link Manager     KSolBtLM         (returns KErrNotSupported)
        * Parameters:
        * @param aSocketServer Handle to connected socket server.
        * @param aSocket       Socket where connection is made
        * @param aProtool      Protocol where the service is based on.
        * @param aDevice       Remote device where connection is made.
        * @param aChannel      RFCOMM channel where connection is made.
        * @param aSec          The security settings for this connection
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        * @param aProtocol     The protocol defined for this connection
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt ActiveConnectSocketL(RSocketServ& aSocketServer,
                                           RSocket& aSocket,
                                           CBTDevice& aDevice,
                                           TUint aChannel,
    									   TBTServiceSecurity aSec,
                                           TRequestStatus& aStatus,
                                           TUint aProtocol = KSolBtRFCOMM);


        /**
        * Method which gets remote device address from open RSocket,
        * searches information from BT Registry and returns found information
        * to given CBTDevice instance.
        * Error values are indicated with return value <> 0
        * Parameters:
        * @param aSocket       Socket which has active connection with remote device
        * @param aDevice       CBTDevice instance where information is updated.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt GetRemoteDeviceInfoL(RSocket& aSocket,
                                           CBTDevice& aDevice);


        /**
        * Method which disconnects an open RSocket-instance.
        * Also an option for shutdown can be passed. Default
        * value for this option is RSocket::ENormal.
        * Error values are indicated with return value <> 0
        * Parameters:
        * @param aSocket       Socket which has active connection with remote device
        * @param aHow          Information about the reason of shutdown.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt DisconnectSocketL(RSocket& asocket, RSocket::TShutdown aHow = RSocket::ENormal);

        /**
        * Method which authenticates an open L2CAP connection.
        * Parameters:
        * @param aSocket       Socket which has active L2CAP connection with remote device
        * @param aStatus       TRequestStatus instance for asynchronous processing.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C void AuthenticateConnection(RSocket& aSocket, TRequestStatus& aStatus);

        /**
        * Method which cancels ongoing authentication procedure.
        * Parameters:
        * @param aSocket       Socket which has active L2CAP connection with remote device
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C void CancelAuthentication(RSocket& aSocket);

        /**
        * Method which cancels ongoing connection establishment.
        * Parameters:
        * @param aSocket       Socket which is connecting to remote device
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C void CancelConnection(RSocket& aSocket);

        /**
        * Method which 1st creates L2CAP connection to remote device's signalling
        * CID and then requests authentication for that connection.
        * --> dedicated bonding procedure is done.
        * Parameters:
        * @param aDevice            Remote device information (only BDAddress needed).
        * @param aBondingObserver   Pointer to bonding observer instance.
        * @return TInt              KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt PerformBondingL(CBTDevice& aDevice, MBTBondingObserver* aBondingObserver);

        /**
        * Method which cancels ongoing bonding procedure.
        * Parameters:
        * @param none
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C void CancelBonding();

        // Audio features
        
        /**
        * Remote parameter search
        * Works only with attributes that return TUint
        * Parameters:
        * @param aDevice            Remote device information (only BDAddress needed).
        * @param aService           Service to be searched
        * @param TSdpAttributeID    Attribute value that we want to search
        * @param aResult            Value of the searched attribute
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        IMPORT_C TInt RemoteSdpQueryL(const CBTDevice& aDevice, TUUID aService, TUint16 TSdpAttributeID, TUint& aResult);

         /**
        * Cancels asynchronous SDP search to remote device
        * Parameters:
        * @param none
        * @return none
        */
        IMPORT_C void CancelSearch();

    private:

        /**
        * Method which makes SDP query to remote device
        * and returns RFCOMM channel where required service is located.
        * Note! At this point this does only Protocol attribute search
        * to a given service.
        * Parameters:
        * @param aDevice       Device where SDP query is made to.
        * @param aService      Service which is searched.
        * @param aChannel      RFCOMM Channel to be returned.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        TInt SearchRemoteChannelL(const CBTDevice& aDevice, TUUID aService, TUint& aChannel);

        /**
        * Method which instantiates instances performing SDP query
        * Parameters:
        * @param aDevice       Device where SDP query is made to.
        * @param aService      Service which is searched.
        * @param aAttributeID  The identifier of the attribute to be searched
        * @return none
        */
        void CreateSdpAgentL(const CBTDevice& aDevice, TUUID aService, TUint16 aAttributeID);

        /**
        * Method which destroys instances performing SDP query
        * Parameters:
        * @param none
        * @return none
        */
        void DeleteSdpAgent();

        /**
        * Method which gets remote device address from open RSocket,
        * searches information from BT Registry and returns found information
        * to given CBTDevice instance.
        * Error values are indicated with return value <> 0
        * Parameters:
        * @param aSocket       Socket which has active connection with remote device
        * @param aDevice       CBTDevice instance where information is updated.
        * @return none
        */
        void DoGetRemoteDeviceInfoL(RSocket& aSocket, CBTDevice& aDevice);

        /**
        * Method which will be called when there is incomming connection 
        * through a listening socket.
        * Parameters:
        * @param aThis       The pointer of 'this'
        * @return KErrNone if OK, else value indicating error situation.
        */
        static TInt PassiveConnectCallBack(TAny* aThis);

        /**
        * it is called when BT asynchronous request complete
        */
        void PassiveConnectComplete();

        /**
        * Method which invokes Search Dialog and returns CBTDevice from call.
        * Device search can be filtered (default filtering off)
        * with aServiceClass parameter.
        * Returned CBTDevice instance includes BDAddress, DeviceName and Class Of Device.
        * Parameters:
        * @param aDevice       Device, which is returned from device search.
        * @param aService      Service filter for device search.
        *                      can filter e.g. object transfer cabable
        *                      devices according to Major Service class.
        * @return TInt         KErrNone if OK, else value indicating error situation.
        */
        TInt SearchRemoteDeviceL(CBTDevice& aDevice, const TBTDeviceClass& aDeviceClass);

    private:    // From MBTSDPSearchObserver

        /**
        * Indicates caller that SDP search has finished.
        * Parameters:
        * @param aStatus   Status information, if there is error.
        *                  KErrNone if channel info received correctly.
        * @return none
        */
        void SearchComplete(TInt aStatus);

        /**
        * Returns a parsed attribute value to caller instance.
        * Parameters:
        * @param aResult   Attribute value
        * @param aStatus   Status information, if there is error.
        *                  KErrNone if channel info received correctly.
        * @return none
        */
        void ReturnAttributeValue(TUint aResult, TInt aStatus);

    private:

        /**
        * By default Symbian OS constructor is private.
        */
        void ConstructL();

        /**
        * C++ default constructor.
        */
        CBTConnection();

    private:
        CBTBondingHandler* iBondingHandler;  // Bonding procedure handler
        CBTSdpDbHandler* iBTSdpDbHandler;    // SDP Database handler

        CBTSdpResultReceiver* iSdpResultReceiver; // For SDP search results.
        CSdpAgent* iSdpAgent;          // For SDP Searh
        CSdpSearchPattern* iSdpSearchPattern;  // For SDP Search
        TUint iResult;   // For SDP search
        TInt iStatus;            // For SDP search status return

        // For asynchronous SDP search
        TRequestStatus* iRequestStatus;     // Pointer to client's remote TRequestStatus
        TBool iSearchActive;      // Boolean indicating if there's active search already
        TUint* iReturnChannel;     // Pointer to client's channel, where result is returned

        // For asynchronous passive connection
        CBTAsyncOp* iAsyncOp;  // provides a wrapper from asynchronous to synchronous
        TRequestStatus* iSocketAcceptStatus;     // Pointer to client's remote TRequestStatus
        TBool iSocketAcceptActive;      // Boolean indicating if there's active search already
        TCallBack iCallBack;
        TBTSockAddr iBTSockAddr; 

        RSocket* iListenSocket; // for backward compatible of RegisterSecuritySettings
        RSocketServ* iSocketServ;
    };

// ---------------------------------------------------------------------
// Class CBTRfs
// Provides API for RFS to init BT settings.
// ---------------------------------------------------------------------

NONSHARABLE_CLASS(CBTRfs) : public CBase
    {
    public:

        /**
        * Two-phase constructor
        * @param    aParent Callback reference to parent class.
        * @return   none
        */
        IMPORT_C static CBTRfs* NewL();

        /**
        * Two-phase constructor
        * @param    aParent Callback reference to parent class.
        * @return   none
        */
        IMPORT_C static CBTRfs* NewLC();

         /**
        * Destructor
        * @param    none
        * @return   none
        */
        virtual ~CBTRfs();

    public: // New API Functions

        /**
        * Provides API for RFS server for BT Factory settings.
        * @param    aUser   Boolean indicating type of restoring.
        *                   ETrue = User level restore (default).
        *                   EFalse = Dealer level restore.
        * @return   TInt    KErrNotSupported
        * @deprecated
        */
        IMPORT_C TInt RestoreFactorySettingsL(TBool aUser = ETrue);

    private:

        /**
        * Two-phase constructor.
        * @param    aParent     Pointer to parent class.
        * @return   none
        */
        void ConstructL();

        /**
        * Default constructor.
        * @param    none
        * @return   none
        */
        CBTRfs();
    };

// ---------------------------------------------------------------------
// General (global) definitions
// ---------------------------------------------------------------------
_LIT( KBTEngName, "BT Engine" );

enum TBTEngPanics
    {
    EBTEngPanicMemoryLeak = 10000,
    EBTEngPanicClassMemberVariableNotNull,
    EBTEngPanicClassMemberVariableIsNull,
    EBTEngPanicArgumentIsNull
    };

void Panic(TInt aPanic);

#endif    // BTENGAPI_H

// End of File
