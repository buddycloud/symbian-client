/*
============================================================================
 Name        : 	GeolocData.cpp
 Author      : 	Ross Savage
 Copyright   : 	2009 Buddycloud
 Description : 	Geoloc data store
 History     : 	1.0
============================================================================
*/

#include "GeolocData.h"

CGeolocData* CGeolocData::NewL() {
	CGeolocData* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CGeolocData* CGeolocData::NewLC() {
	CGeolocData* self = new (ELeave) CGeolocData();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CGeolocData::~CGeolocData() {	
	for(TInt i = 0; i < iStringData.Count(); i++) {
		delete iStringData[i];
	}
	
	iStringData.Close();
	iRealData.Close();
}

void CGeolocData::ConstructL() {
	for(TInt i = 0; i < KMaxStringItems; i++) {
		iStringData.Append(NULL);
	}
	
	for(TInt i = 0; i < KMaxRealItems; i++) {
		iRealData.Append(0.0);
	}
}

TDesC& CGeolocData::GetString(TGeolocString aId) {
	if(iStringData[aId] == NULL) {
		return iNullString;
	}
	
	return *iStringData[aId];
}

void CGeolocData::SetStringL(TGeolocString aId, const TDesC& aString) {
	if(iStringData[aId]) {
		delete iStringData[aId];
		iStringData[aId] = NULL;
	}
	
	if(aString.Length() > 0) {
		iStringData[aId] = aString.AllocL();
	}
}

TReal CGeolocData::GetReal(TGeolocReal aId) {
	return iRealData[aId];
}

void CGeolocData::SetRealL(TGeolocReal aId, TReal aReal) {
	iRealData[aId] = aReal;
}
