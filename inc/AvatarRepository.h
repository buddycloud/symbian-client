/*
============================================================================
 Name        : 	AvatarRepository.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2008
 Description : 	Management of Avatar Images
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#ifndef AVATARREPOSITORY_H_
#define AVATARREPOSITORY_H_

#include <e32base.h>
#include <fbs.h>
#include <f32file.h>
#include <imageconversion.h>
#include "AvatarConstants.h"

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TRepositoryAction {
	ERepositoryIdle, ERepositoryConverting
};

enum TRepositoryType {
	ETypeIcon, ETypeOverlay, ETypeAvatar
};

enum TMidmapLevel {
	ELevelLarge, ELevelNormal, ELevelSmall, ELevelTiny
};

/*
----------------------------------------------------------------------------
--
-- CAvatarItem
--
----------------------------------------------------------------------------
*/

class CAvatarImage : public CBase {
	public:
		~CAvatarImage();

	public:
		TSize iSize;

		CFbsBitmap* iAvatar;
		CFbsBitmap* iMask;
};

/*
----------------------------------------------------------------------------
--
-- CAvatarItem
--
----------------------------------------------------------------------------
*/

class CAvatarItem : public CBase {
	public:
		CAvatarItem();
		~CAvatarItem();

	public:
		TInt iId;
		TRepositoryType iType;
		TInt iLevels;
		TFileName iFilename;
		
		RPointerArray<CAvatarImage> iImages;

		TBool iReady;
		TBool iStored;
};

/*
----------------------------------------------------------------------------
--
-- Interfaces
--
----------------------------------------------------------------------------
*/

class MAvatarRepositoryObserver {
	public:
		virtual void AvatarLoaded() = 0;
};

/*
----------------------------------------------------------------------------
--
-- CAvatarRepository
--
----------------------------------------------------------------------------
*/

class CAvatarRepository : public CActive {
	public:
		CAvatarRepository();
		static CAvatarRepository* NewL(MAvatarRepositoryObserver* aObserver);
		~CAvatarRepository();

	private:
		void ConstructL(MAvatarRepositoryObserver* aObserver);

	public:
		void RemoveL(TInt aId);

	private:
		void LoadAvatarItemL(TInt aIndex);
		void DeleteAvatarItemL(TFileName aAvatarFile);

		HBufC8* LoadAvatarListFromFileLC(TFileName aListFile);
		void LoadAvatarItemsFromListL(const TDesC8& aXml);
		
	private:
		void RenderMipmapImageL(CFbsBitmap* aSource, CFbsBitmap* aDestination, TRect aSourceRect);
		void RenderMipmapImagesL(TInt aIndex);

	private: // Load & save repository state
		void InternalizeL();
		void ExternalizeL();

	public:
		CFbsBitmap* GetImage(TInt aId, TBool aMask = false, TInt aMidmapLevel = 0);

	public: // From CActive
		void RunL();
		TInt RunError(TInt aError);
		void DoCancel();

	private: // Variables
		MAvatarRepositoryObserver* iObserver;
		
		CFbsBitmap* iNullImage;

		RArray<CAvatarItem*> iAvatarList;

		TRepositoryAction iRepositoryAction;
		TInt iNextAvatarToLoad;

		CBufferedImageDecoder* iImageDecoder;
};

#endif /*AVATARREPOSITORY_H_*/
