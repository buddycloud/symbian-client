/*
* ============================================================================
*  Name        : btengdiscovery.h
*  Part of     : Bluetooth Engine / Bluetooth Discovery
*  Description : Bluetooth Engine API for remote device and service discovery 
*                and local SDP database functionality.
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

#ifndef BTENGDISCOVERY_H
#define BTENGDISCOVERY_H

#include <btsdp.h>
#include <btdevice.h>

class CBTEngSdpDbHandler;
class CBTEngSdpQuery;
class CBTEngDeviceSearch;
class TBTEngSdpAttrValue;

/**  Array definition for storing SDP record handles */
typedef RArray<TSdpServRecordHandle> RSdpRecHandleArray;

/**  Array definition for storing SDP attribute values */
typedef RArray<TBTEngSdpAttrValue> RSdpResultArray;


/**
 *  class TBTEngSdpResult
 *
 *  Data structure for passing SDP results to the client.
 *
 *  @lib btengdiscovery.lib
 *  @since S60 v3.2
 */
class TBTEngSdpAttrValue
    {

public:

    /**
     * Data structure containing the SDP attribute value.
     * Note: the member varaible containing the data is dependent 
     * on the data type attribute of the parent class.
     */
    class TSdpAttrValue
        {

    public:

        /**
         * Numeric value (0 if the attribute is non-numeric).
         */
        TInt iValNumeric;

        /**
         * String value (empty if the attribute is numeric).
         */
        TPtrC8 iValString;

        };

public: // data

    /**
     * Attribute identifier of this attribute.
     */
    TSdpAttributeID iAttrId;

    /**
     * Attribute type of this attribute.
     */
    TSdpElementType iAttrType;

    /**
     * Attribute value of this attribute.
     */
    TSdpAttrValue iAttrValue;

    };


/**
 *  Class CBTEngDiscovery
 *
 *  Callback interface for receiving results from queries 
 *  of remote SDP databases.
 *  Clients that make SDP queries through CBTEngDiscovery 
 *  must implement this interface to handle the results.
 *
 *  @lib btengdiscovery.lib
 *  @since S60 v3.2
 */
class MBTEngSdpResultReceiver
    {

public:

    /**
     * Provides notification of the result of a service search that matches 
     * the requested UUID (through CBTEngDiscovery::RemoteSdpQuery).
     * This method indicates that the search has completed, and returns 
     * all the results to the caller at once.
     *
     * @since S60 v3.2
     * @param aResult Array of record handles that match the requested UUID.
     *                Note: the array will not be available anymore after 
     *                this method returns.
     * @param aTotalRecordsCount The total number of records returned.
     * @param aErr Error code of the service search operation; KErrNone if 
     *             sucessful, KErrEof if no record matched the requested UUID, 
     *             KErrCouldNotConnect and KErrCouldDisconnected in case of 
     *             Bluetooth connection errors; otherwise one of the 
     *             system-wide error codes.
     */
    virtual void ServiceSearchComplete( const RSdpRecHandleArray& aResult, 
                                         TUint aTotalRecordsCount, TInt aErr ) = 0;

    /**
     * Provides notification of the result of an attribute search that matches 
     * the requested attribute (through CBTEngDiscovery::RemoteSdpQuery).
     * This method indicates that the search has completed, and returns 
     * all the results to the caller at once.
     *
     * @since S60 v3.2
     * @param aHandle Record handle of the service record containing the result.
     * @param aAttr Array containing the attribute that matches the 
     *              requested attribute.
     *              Note: the array will not be available anymore after 
     *              this method returns.
     * @param aErr Error code of the service search operation; KErrNone if 
     *             sucessful, KErrEof if the requested attribute was not 
     *             contained in the specified service record, 
     *             KErrCouldNotConnect and KErrCouldDisconnected in case of 
     *             Bluetooth connection errors; otherwise one of the 
     *             system-wide error codes.
     */
    virtual void AttributeSearchComplete( TSdpServRecordHandle aHandle, 
                                           const RSdpResultArray& aAttr, 
                                           TInt aErr ) = 0;

    /**
     * Provides notification of the result of an combination of a service 
     * and attribute search (through CBTEngDiscovery::RemoteSdpQuery).
     * This method is called for each service and attribute combination for 
     * which a match was found. The last result (which could be empty if no 
     * match was found) contain error code KErrEof to indicate that the 
     * search has completed.
     *
     * @since S60 v3.2
     * @param aHandle Record handle of the service record containing the result.
     * @param aAttr Array containing the attribute that matches the 
     *              requested attribute.
     *              Note: the array will not be available anymore after 
     *              this method returns.
     * @param aErr Error code of the service search operation; KErrNone if 
     *             sucessful and more results follow, KErrEof indicates that 
     *             this is the last result (which could be empty if no match 
     *             was found), KErrCouldNotConnect and KErrCouldDisconnected 
     *             in case of Bluetooth connection errors; otherwise one of 
     *             the system-wide error codes.
     */
    virtual void ServiceAttributeSearchComplete( TSdpServRecordHandle aHandle, 
                                                  const RSdpResultArray& aAttr, 
                                                  TInt aErr ) = 0;

    /**
     * Provides notification of the result of the discovery of nearby 
     * Bluetooth devices.
     *
     * @since S60 v3.2
     * @param aDevice The data structure encapsulates all information 
     *                about the selected device. Ownership of the data 
     *                structure has not been transfered and is still with
     *                the API client.
     * @param aErr Error code of the device search operation; KErrNone if 
     *             sucessful, KErrCancel if the user cancelled the 
     *             dialog, KErrAbort if CBTEngDiscovery::CancelSearchRemoteDevice
     *             was called; otherwise one of the system-wide error codes.
     */
    virtual void DeviceSearchComplete( CBTDevice* aDevice, TInt aErr ) = 0;

    };


/**
 *  Class CBTEngDiscovery
 *
 *  This is a helper class that simplifies the usage of Symbian's 
 *  BluetoothT SDP interface, for registration of SDP records in 
 *  the local database and for queries of remote SDP databases.
 *  Additionally, functionality for searching for and pairing with 
 *  remote Bluetooth devices.
 *
 *  @lib btengdiscovery.lib
 *  @since S60 v3.2
 */
class CBTEngDiscovery : public CBase
    {

public:
    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aNotifier Pointer to callback interface that receives 
     *                  the SDP query results.
     * @return Pointer to the constructed CBTEngDiscovery object.
     */
    IMPORT_C static CBTEngDiscovery* NewL( MBTEngSdpResultReceiver* aNotifier = NULL );

    /**
     * Two-phase constructor
     *
     * @since S60 v3.2
     * @param aNotifier Pointer to callback interface that receives 
     *                  the SDP query results.
     * @return Pointer to the constructed CBTEngDiscovery object.
     */
    IMPORT_C static CBTEngDiscovery* NewLC( MBTEngSdpResultReceiver* aNotifier = NULL );

    /**
     * Destructor
     */
    virtual ~CBTEngDiscovery();

    /**
     * Launches to notifier for discovering nearby Bluetooth devices and 
     * user selection of one device.
     * When completed, the selected device is passed back to the client 
     * through the callback interface 
     * method MBTEngSdpResultReceiver::DeviceSearchComplete().
     *
     * @since S60 v3.2
     * @param aDevice The data structure in which the result will be stored 
     *                (passed back thorugh DeviceSearchComplete()).
     *                Ownership of the data structure remains with the caller 
     *                of this method.
     * @param aServiceClass Filter for device search; this will filter the 
     *                      results according to the specified major service 
     *                      class field of the CoD. The default value 
     *                      is zero and will not apply any filter.
     * @return KErrNone if sucessful, otherwise the error code 
     *         indicating the error situation.
     */
    IMPORT_C TInt SearchRemoteDevice( CBTDevice* aDevice, TUint aServiceClass = 0 );

    /**
     * Cancels an ongoing search for nearby Bluetooth devices.
     * This results in a call to the callback interface 
     * method MBTEngSdpResultReceiver::ServiceSearchComplete() with error
     * code KErrAbort.
     *
     * @since S60 v3.2
     */
    IMPORT_C void CancelSearchRemoteDevice();

    /**
     * Registers an service record in the local SDP database, based on 
     * a known service as defined in Bluetooth Engine resource file.
     *
     * @since S60 v3.2
     * @param aService UUID of the service to be registered.
     * @param aChannel The value of the channel of the ProtocolDescriptorList 
                       attribute (e.g. the RFCOMM channel), if applicable.
     * @param aHandle On return, contains the service record handle 
     *                identifying the created service record.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt RegisterSdpRecord( const TUUID& aService, const TUint aChannel, 
                                      TSdpServRecordHandle& aHandle );

    /**
     * Deletes a service record from the local SDP database that was 
     * previously registered by through this instance of CBTEngDiscovery.
     *
     * @since S60 v3.2
     * @param aHandle Handle to the SDP record to be deleted. Note that 
     *                this has to be a valid (existing) SDP record.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt DeleteSdpRecord( const TSdpServRecordHandle aHandle );

    /**
     * Sets the callback class for receiving SDP query results. This will
     * replace the current callback interface used to pass back the 
     * SDP query results.
     *
     * @since S60 v3.2
     * @param aNotifier Callback class to receive the SDP results.
     */
    IMPORT_C void SetNotifier( MBTEngSdpResultReceiver* aNotifier );

    /**
     * Starts an SDP query. The remote SDP database is searched 
     * for service records containing the specified UUID, the 
     * equivalent of an SDP ServiceSearch transaction.
     * When completed, record handles for all matching service records 
     * are passed back to the client through the callback interface 
     * method MBTEngSdpResultReceiver::ServiceSearchComplete().
     *
     * @since S60 v3.2
     * @param aAddr Target Bluetooth device address for the SDP query.
     * @param aService The UUID to search for.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt RemoteSdpQuery( const TBTDevAddr& aAddr, const TUUID& aService );

    /**
     * Starts an SDP query. A specific service record on the remote SDP 
     * database is queried for the value of a specified attribute, the 
     * equivalent of an SDP ServiceAttribute transaction.
     * When completed, the result is passed back to the client 
     * through the callback interface method 
     * MBTEngSdpResultReceiver::AttributeSearchComplete().
     *
     * @since S60 v3.2
     * @param aAddr Target Bluetooth device address for the SDP query.
     * @param aHandle The service record handle identifying the 
     *                service record to search the attribute from.
     * @param aAttrId The service attribute to search for.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt RemoteSdpQuery( const TBTDevAddr& aAddr, 
                                   const TSdpServRecordHandle aHandle, 
                                   const TSdpAttributeID aAttrId );

    /**
     * Starts an SDP query. The remote SDP database is searched for 
     * the value of a specified attribute in a service record 
     * containing the specified UUID, the equivalent of an 
     * SDP ServiceAttributeSearch transaction.
     * When completed, the results are passed back to the client 
     * through the callback interface method 
     * MBTEngSdpResultReceiver::ServiceAttributeSearchComplete().
     *
     * @since S60 v3.2
     * @param aAddr Target Bluetooth device address for the SDP query.
     * @param aService The UUID to search for.
     * @param aAttrId The service attribute to search for.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt RemoteSdpQuery( const TBTDevAddr& aAddr, const TUUID& aService, 
                                   const TSdpAttributeID aAttrId );

    /**
     * Starts an SDP query. The remote SDP database is searched for the 
     * value of the ProtocolDescriptorList attribute in a service record 
     * containing the specified UUID, the equivalent of an SDP 
     * ServiceAttributeSearch transaction with ProtocolDescriptorList as 
     * attribute. This can e.g. be used to search the remote RFCOMM channel.
     * When completed, the results are passed back to the client 
     * through the callback interface method 
     * MBTEngSdpResultReceiver::AttributeSearchComplete().
     *
     * @since S60 v3.2
     * @param aAddr Target Bluetooth device address for the SDP query.
     * @param aService The UUID to search for.
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    IMPORT_C TInt RemoteProtocolChannelQuery( const TBTDevAddr& aAddr, 
                                               const TUUID& aService );

    /**
     * Cancels an ongoing SDP query.
     *
     * @since S60 v3.2
     */
    IMPORT_C void CancelRemoteSdpQuery();

    /**
     * Static helper function to return the SDP element type 
     * of an indexed element in a list (DES or DEA).
     *
     * @param aResultArray The array containing the SDP attribute.
     * @param aIndex The position of the element to be parsed, relative 
     *               to zero (i.e. zero is the first element).
     * @param aType On return, contains the element type 
     *              of the element at position aIndex.
     * @return KErrNone if sucessful, 
     *         KErrEof if the index is greater than the number of items in the array.
     * @since S60 v3.2
     */
    IMPORT_C static TInt ParseNextSdpAttrValueType( RSdpResultArray& aResultArray, 
                                                     TInt aIndex, TSdpElementType& aType );

    /**
     * Static helper function to return the protocol channel number from the 
     * result array of a ServiceAttributeSearch. This value can be used to 
     * obtain e.g. the remote RFCOMM channel after a RemoteProtocolChannelQuery 
     * has completed.
     *
     * @param aResultArray The array containing the SDP attribute.
     * @param aType On return, contains the channel number.
     * @return KErrNone if sucessful, KErrArgument if the SDP attribute is not 
     *         of type ProtocolDescriptorList.
     * @since S60 v3.2
     */
    IMPORT_C static TInt ParseRfcommChannel( RSdpResultArray& aResultArray, 
                                              TInt& aChannel );

private:

    /**
     * C++ default constructor
     *
     * @since S60 v3.2
     * @param aNotifier Pointer to callback interface that receives 
     *                  the SDP query results.
     */
    CBTEngDiscovery( MBTEngSdpResultReceiver* aNotifier );

    /**
     * Symbian 2nd-phase constructor
     *
     * @since S60 v3.2
     */
    void ConstructL();

    /**
     * Helper function to check that a valid session with 
     * the local SDP database exists.
     *
     * @since S60 v3.2
     * @return KErrNone if sucessful, otherwise the error code 
     *                  indicating the error situation.
     */
    TInt CheckSdpDbHandler();

    /**
     * Helper function to check that a valid handler of 
     * remote SDP queries exists.
     *
     * @since S60 v3.2
     * @return KErrNone if sucessful, 
     *         KErrNotReady if no SDP result notifier has been set,
     *          otherwise the error code indicating the error situation.
     */
    TInt CheckSdpQueryHandler();

private: // data

    /**
     * Reference to SDP DB handler.
     * Own.
     */
    CBTEngSdpDbHandler* iSdpDbHandler;

    /**
     * Reference to handler of remote SDP queries.
     * Own.
     */
    CBTEngSdpQuery* iSdpQueryHandler;

    /**
     * Reference to handler of remote device searches.
     * Own.
     */
    CBTEngDeviceSearch* iDevSearchHandler;

    /**
     * Reference to receiver of results.
     * Not own.
     */
    MBTEngSdpResultReceiver* iResultNotifier;

    };

// BTENGDISCOVERY_H
#endif
