/*
============================================================================
 Name        : 	CellTowerDataSimulation.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Simulation of data from Cell Towers
 History     : 	1.0

		1.0	Initial Development
============================================================================
*/

#ifndef CELLTOWERDATASIMULATION_H_
#define CELLTOWERDATASIMULATION_H_

#include "CellTowerDataHandler.h"
#include "Timer.h"

class CCellTowerDataSimulation : public MTimeoutNotification {
	public:
		CCellTowerDataSimulation(MCellTowerNotification* aObserver);
		static CCellTowerDataSimulation* NewL(MCellTowerNotification* aObserver);
		~CCellTowerDataSimulation();

	private:
		void ConstructL();
		
	public: 
		void StartL();

	private:
		void InternalizeL();

	public: // From CCustomTimer
		void TimerExpired(TInt aExpiryId);
		
	private:
		MCellTowerNotification* iObserver;

		CCustomTimer* iTimer;
		
		HBufC8* iCellBuffer;
};

#endif /*CELLTOWERDATASIMULATION_H_*/
