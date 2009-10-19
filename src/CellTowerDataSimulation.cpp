/*
============================================================================
 Name        : 	CellTowerDataSimulation.cpp
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Simulation of data from Cell Towers
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <f32file.h>
#include <s32file.h>
#include <bacline.h>
#include "CellTowerDataSimulation.h"

CCellTowerDataSimulation::CCellTowerDataSimulation(MCellTowerNotification* aObserver) {
	iObserver = aObserver;
}

CCellTowerDataSimulation* CCellTowerDataSimulation::NewL(MCellTowerNotification* aObserver) {
	CCellTowerDataSimulation* self = new (ELeave) CCellTowerDataSimulation(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

CCellTowerDataSimulation::~CCellTowerDataSimulation() {
	if(iTimer)
		delete iTimer;
	
	if(iCellBuffer)
		delete iCellBuffer;
}

void CCellTowerDataSimulation::ConstructL() {
	iTimer = CCustomTimer::NewL(this);
	
	InternalizeL();	
}

void CCellTowerDataSimulation::StartL() {	
	TPtr8 pCellBuffer(iCellBuffer->Des());

	if(pCellBuffer.Length() > 0) {
		TimerExpired(0);
	}
	else {
		iObserver->CellTowerError(ECellTowerReadError);
	}
}

void CCellTowerDataSimulation::InternalizeL() {
	RFs aSession;
	RFile aFile;
	TFileName aFileName;
	TInt aFileSize;

	if(aSession.Connect() == KErrNone) {
		CleanupClosePushL(aSession);

#ifndef __SERIES60_3X__
		aFileName = TFileName(_L("CellTowerSimData.txt"));
		CompleteWithAppPath(aFileName);
#else
		if(aSession.PrivatePath(aFileName) == KErrNone) {
#ifndef __WINSCW__
			aFileName.Insert(0, _L(":"));
			CCommandLineArguments* pArguments;
			TFileName drive;
			pArguments = CCommandLineArguments::NewL();
			if (pArguments->Count() > 0) {
				drive.Append(pArguments->Arg(0)[0]);
				aFileName.Insert(0, drive);
				delete pArguments;
			}
#endif

			aFileName.Append(_L("CellTowerSimData.txt"));

#ifdef __WINSCW__
			aFileName.Insert(0, _L("z:"));
#endif
		}
#endif

		if(aFile.Open(aSession, aFileName, EFileStreamText|EFileRead) == KErrNone) {
			CleanupClosePushL(aFile);
			aFile.Size(aFileSize);

			// Creat buffer & read file
			iCellBuffer = HBufC8::NewL(aFileSize);
			TPtr8 pCellBuffer(iCellBuffer->Des());
			aFile.Read(pCellBuffer);
			aFile.Close();
			
			CleanupStack::PopAndDestroy(&aFile);
		}
		
		CleanupStack::PopAndDestroy(&aSession);
	}
}

void CCellTowerDataSimulation::TimerExpired(TInt /*aExpiryId*/) {
	TPtr8 pCellBuffer(iCellBuffer->Des());
	TInt aSearch = pCellBuffer.Locate(';');
	
	TBuf<4> aCountryCode;
	TBuf<8> aNetworkIdentity;
	TUint aLocationAreaCode;
	TUint aCellId;
	TInt aTimeout;
	
	if(aSearch != KErrNotFound) {
		HBufC* aCellData = HBufC::NewLC(pCellBuffer.Length());
		TPtr pCellData(aCellData->Des());
		pCellData.Copy(pCellBuffer);
		pCellData.Delete(aSearch, pCellData.Length());
		pCellBuffer.Delete(0, aSearch+1);
		
		// Mcc
		aSearch = pCellData.Locate(',');
		aCountryCode.Copy(pCellData.Ptr(), aSearch);
		pCellData.Delete(0, aSearch+1);
		
		// Mnc
		aSearch = pCellData.Locate(',');
		aNetworkIdentity.Copy(pCellData.Ptr(), aSearch);
		pCellData.Delete(0, aSearch+1);
		
		// Lac
		aSearch = pCellData.Locate(',');
		HBufC* aLac = HBufC::NewLC(aSearch);
		TPtr pLac(aLac->Des());
		pLac.Copy(pCellData.Ptr(), aSearch);
		pCellData.Delete(0, aSearch+1);
		TLex aLacLex(pLac);
		aLacLex.Val(aLocationAreaCode);
		CleanupStack::PopAndDestroy(); // aLac
		
		// Cell
		aSearch = pCellData.Locate(',');
		HBufC* aCell = HBufC::NewLC(aSearch);
		TPtr pCell(aCell->Des());
		pCell.Copy(pCellData.Ptr(), aSearch);
		pCellData.Delete(0, aSearch+1);
		TLex aCellLex(pCell);
		aCellLex.Val(aCellId);
		CleanupStack::PopAndDestroy(); // aCell
		
		iObserver->CellTowerData(aCountryCode, aNetworkIdentity, aLocationAreaCode, aCellId);
		
		// Timeout
		TLex aTimeoutLex(pCellData);
		aTimeoutLex.Val(aTimeout);
		
		if(aTimeout > 0) {
			iTimer->After(aTimeout * 1000000);
		}
	
		CleanupStack::PopAndDestroy(); // aCellData
	}
}
