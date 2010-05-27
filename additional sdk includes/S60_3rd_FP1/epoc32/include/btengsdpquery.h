/*
* ============================================================================
*  Name     : CBTSdpQuery from BTEngSdpQuery.h
*  Part of  : BTEng
*
*  Description:
*       CBTSdpQuery - class to handle remote SDP queries
*  Version: %version: 2.1.2 %
*
*  Copyright (C) 2005 Nokia Corporation.
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

#ifndef BTENGSDPQUERY_H
#define BTENGSDPQUERY_H

//  INCLUDES
#include <e32base.h>
#include <bttypes.h>

// FORWARD DECLARATIONS
class TBTDevAddr;
class CBTSdpQueryAlg;
class RBTPhysicalLinkAdapter;
class RSocketServ;

// CLASS DECLARATION
    
class TBTSupportedFeature
    {
    public:
        TUUID iUUID;
        TUint16 iSupportedFeature;
    };

class TBTSdpAttrInt
    {
    public:
        TUUID iUUID;   // The UUID of the service record which the attr belongs to.
                          // Must be specified by caller.
        TUint16 iAttrID;  // The Attribute ID. Must be specified by the caller.
        TUint iAttrValue; // The value of the attribute if this is found in the device.
    };

    
/**
* The wrapper class providing public interfaces for SDP queires
* @since A3.0
*/
NONSHARABLE_CLASS(CBTSdpQuery) : public CBase
    {
    public:   // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        IMPORT_C static CBTSdpQuery* NewL();

        /**
        * Destructor.
        */
        ~CBTSdpQuery();

    public: // New functions
    
        /**
        * Queries the supported BT services (profiles) in a remote BT device.
        * Only one query is allowed at a time.
        *
        * The completion status will be written to TRequestStatus. 
        * On a successful completion, aServiceArray contains the supported services only
        * and all not supported services will be removed from the array.
        * aServiceArray remains unchanged if the query fails.
        * 
        * Leave with KErrInUse if there is an ongoing query already,
        * or an SPD error from lower level
        *
        * Using CancelSdpQuery() to cancel this operation.
        *
        * Parameters:
        * @param aDevAddr      The remote device address
        * @param aServiceUUIDs Input: a list of service UUIDs to be queried
        *                      Output: supported BT service UUIDs
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        */
        IMPORT_C void QuerySupportedServicesL(
    		const TBTDevAddr& aDevAddr,
    		RArray<TUUID>& aServiceUUIDs,
    		TRequestStatus& aStatus);

        /**
        * Queries the BT services (profiles) in a remote BT device and 
        * the supported features.
        *
        * The completion status will be written to TRequestStatus. 
        * On a successful completion, aServiceArray contains the supported services and
        * the corresponding requested attribute values.
        * and all not supported services will be removed from the array.
        * aServiceArray remains unchanged if the query fails.
        * 
        * Leave with KErrInUse if there is an ongoing query already,
        * or an SPD error from lower level
        *
        * Using CancelSdpQuery() to cancel this operation.
        *
        * Parameters:
        * @param aDevAddr      The remote device address
        * @param aSupportedFeatures Input: a list of service UUIDs and supported features to be queried
        *                           Output: supported BT service UUIDs
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        */
        IMPORT_C void QueryServiceAndSupportedFeatureL(
    		const TBTDevAddr& aDevAddr,
    		RArray<TBTSupportedFeature>& aSupportedFeatures,
    		TRequestStatus& aStatus);
        
        /**
        * Queries a list of integer type attributes.
        *
        * The completion status will be written to TRequestStatus. 
        * On a successful completion, aAttrs contains the discovered services.
        * If the service record is found, but the specified attribute is not, the atrribute value is 0.
        * And any not found services are removed from the array.
        * 
        *
        * Leave with KErrInUse if there is an ongoing query already,
        * or an SPD error from lower level
        *
        * Using CancelSdpQuery() to cancel this operation.
        *
        * Parameters:
        * @param aAddr      The remote device address
        * @param aAttrs Input: a list of attributes to be queried
        *               Output: Discovered attributes and their values
        * @param aStatus       TRequestStatus instance for asynchronous connecting.
        */
        IMPORT_C void QueryServiceAndAttrIntL(
    		const TBTDevAddr& aAddr,
    		RArray<TBTSdpAttrInt>& aAttrs,
    		TRequestStatus& aStatus);
        /**
        * Cancels the query if there is any ongoing.
        */
    	IMPORT_C void CancelSdpQuery();

        /**
        * Called by CBTSdpQueryAlg. Cleanup memory
        * Parameters:
        * @return none
        */
        void SdpQueryCompleted(TInt aErr);
        
        /**
        * Unsniffs the BT link before staring the SDP. The function returns immediately and
        * unsniffing is done asynchronously.
        * Parameters:
        * @param aDevAdd    The device address related to the link to be unsniffed 
        * @return none
        */
        TInt ActivateLink(const TBTDevAddr& aDevAdd);

        /**
        * Cancels the unsniff request made by ActivateLink call
        * Parameters:
        * @return none
        */
        TInt CancelActivateLink();

    private:

        /**
        * 2nd phase constructor
        */
        void ConstructL();

        /**
        * Default constructor
        */
        CBTSdpQuery();

    private: // DATA
        CBTSdpQueryAlg* iActiveAlg;  // the instance for different SDP query strategies
        TRequestStatus* iRequest;
        
        RBTPhysicalLinkAdapter* iAdabter; 
        RSocketServ* iSocketServ;
    };

#endif      // BTENGSDPQUERY_H

// End of File
