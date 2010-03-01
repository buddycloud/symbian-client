/*
============================================================================
 Name        : 	LocationInterfaces.h
 Author      : 	Ross Savage
 Copyright   : 	2010 Buddycloud
 Description : 	Location interfaces for access to location engine data
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef LOCATIONINTERFACES_H_
#define LOCATIONINTERFACES_H_

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TLocationMotionState {
	EMotionStationary, EMotionRestless, EMotionMoving, EMotionUnknown
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MLocationEngineNotification {
	public:
		virtual void HandleLocationServerResult(TLocationMotionState aMotionState, TInt aPatternQuality, TInt aPlaceId) = 0;

		virtual void LocationShutdownComplete() = 0;
};

class MLocationEngineDataInterface {
	public: // Data availability
		virtual TBool CellDataAvailable() = 0;
		virtual TBool GpsDataAvailable() = 0;
		virtual TBool WlanDataAvailable() = 0;
		virtual TBool BtDataAvailable() = 0;
		
	public: // Access to GPS position
		virtual void GetGpsPosition(TReal& aLatitude, TReal& aLongitude) = 0;
		
	public: // Access to location results
		virtual TLocationMotionState GetLastMotionState() = 0;
		virtual TInt GetLastPatternQuality() = 0;
		virtual TInt GetLastPlaceId() = 0;
};

#endif /* LOCATIONINTERFACES_H_ */
