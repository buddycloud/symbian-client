/*
============================================================================
 Name        : ViewAccessorObserver.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2007
 Description : Provides access from a Container to its View
============================================================================
*/

#ifndef VIEWACCESSOROBSERVER_H_
#define VIEWACCESSOROBSERVER_H_

#include <blddef.h>
#include <eikmenub.h>
#include <eikspane.h> 
#include <eikbtgpc.h>

#ifdef __SERIES60_40__
#include <akntoolbar.h>
#endif
		
class MViewAccessorObserver {
	public:
		virtual CEikMenuBar* ViewMenuBar() = 0;
		virtual CEikStatusPane* ViewStatusPane() = 0;
		virtual CEikButtonGroupContainer* ViewCba() = 0;
#ifdef __SERIES60_40__
		virtual CAknToolbar* ViewToolbar() = 0;
#endif
};

#endif /*VIEWACCESSOROBSERVER_H_*/
