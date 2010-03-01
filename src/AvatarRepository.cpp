/*
============================================================================
 Name        : 	AvatarRepository.cpp
 Author      : 	Ross Savage
 Copyright   : 	2008 Buddycloud
 Description : 	Management of Avatar Images
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

// INCLUDE FILES
#include <coemain.h>
#include <s32file.h>
#include <bacline.h>
#include <gdi.h>
#include "AvatarRepository.h"
#include "FileUtilities.h"
#include "XmlParser.h"

/*
----------------------------------------------------------------------------
--
-- CAvatarImage
--
----------------------------------------------------------------------------
*/

CAvatarImage::~CAvatarImage() {
	if(iAvatar)
		delete iAvatar;

	if(iMask)
		delete iMask;
}

/*
----------------------------------------------------------------------------
--
-- CAvatarItem
--
----------------------------------------------------------------------------
*/

CAvatarItem::CAvatarItem() {
	iReady = false;
}

CAvatarItem::~CAvatarItem() {
	for(TInt i = 0; i < iImages.Count(); i++) {
		delete iImages[i];
	}
	
	iImages.Close();
}

/*
----------------------------------------------------------------------------
--
-- CAvatarRepository
--
----------------------------------------------------------------------------
*/

CAvatarRepository::CAvatarRepository() : CActive(EPriorityHigh) {
}

CAvatarRepository* CAvatarRepository::NewL(MAvatarRepositoryObserver* aObserver) {
	CAvatarRepository* self = new (ELeave) CAvatarRepository();
	CleanupStack::PushL(self);
	self->ConstructL(aObserver);
	CleanupStack::Pop();
	return self;
}

CAvatarRepository::~CAvatarRepository() {
	Cancel();

	ExternalizeL();

	if(iImageDecoder)
		delete iImageDecoder;

	// Delete list
	for(TInt i = 0; i < iAvatarList.Count();i++) {
		delete iAvatarList[i];
	}

	iAvatarList.Close();
	
	if(iNullImage)
		delete iNullImage;
}

void CAvatarRepository::ConstructL(MAvatarRepositoryObserver* aObserver) {
	iObserver = aObserver;
	
	iNullImage = new (ELeave) CFbsBitmap();
	iNullImage->Create(TSize(1, 1), EGray2);

	InternalizeL();

	CActiveScheduler::Add(this);

	iRepositoryAction = ERepositoryIdle;

	iImageDecoder = CBufferedImageDecoder::NewL(CCoeEnv::Static()->FsSession());

	// Start loading avatars
	iNextAvatarToLoad = 0;
	LoadAvatarItemL(iNextAvatarToLoad);
}

void CAvatarRepository::RemoveL(TInt aId) {
	for(TInt i = 0; i < iAvatarList.Count(); i++) {
		if(iAvatarList[i]->iId == aId) {
			if(iAvatarList[i]->iType == ETypeAvatar) {
				// If stored, delete file
				if(iAvatarList[i]->iStored) {
					DeleteAvatarItemL(iAvatarList[i]->iFilename);
				}

				// Remove Avatar from Repository
				delete iAvatarList[i];
				iAvatarList.Remove(i);
			}
		}
	}
}

void CAvatarRepository::LoadAvatarItemL(TInt aIndex) {
	if(aIndex >= 0 && aIndex < iAvatarList.Count()) {
		RFs aSession = CCoeEnv::Static()->FsSession();
		TFileName aFilename(iAvatarList[aIndex]->iFilename);
		RFile aFile;

		CFileUtilities::CompleteWithApplicationPath(aSession, aFilename);

		// Load data into buffer
		if(aFile.Open(aSession, aFilename, EFileStreamText|EFileRead) == KErrNone) {
			TInt aFileSize;
			aFile.Size(aFileSize);

			// Create buffer & read file
			HBufC8* aAvatarData = HBufC8::NewLC(aFileSize);
			TPtr8 pAvatarData = aAvatarData->Des();
			aFile.Read(pAvatarData);
			aFile.Close();

			// Pass data to image decoder
			iImageDecoder->OpenL(pAvatarData);

			if(iImageDecoder->ValidDecoder()) {
				// Get frame information
				TFrameInfo aFrameInfo = iImageDecoder->FrameInfo();

				// Create avatar bitmap
				CAvatarImage* aNewImage = new (ELeave) CAvatarImage();
				iAvatarList[aIndex]->iImages.Append(aNewImage);
									
				aNewImage->iAvatar = new (ELeave) CFbsBitmap();
				aNewImage->iSize = aFrameInfo.iOverallSizeInPixels;

				if(aNewImage->iAvatar->Create(aFrameInfo.iOverallSizeInPixels, aFrameInfo.iFrameDisplayMode) == KErrNone) {
					TDisplayMode aMaskMode = EGray2;

					// Select correct mask mode
					if(aFrameInfo.iFlags & TFrameInfo::EAlphaChannel) {
						aMaskMode = EGray256;
					}

					// Create mask bitmap
					aNewImage->iMask = new (ELeave) CFbsBitmap();

					if(aNewImage->iMask->Create(aFrameInfo.iOverallSizeInPixels, aMaskMode) == KErrNone) {
						// Creation of avatar & mask successful
						// Decode image
						iRepositoryAction = ERepositoryConverting;
						iImageDecoder->Convert(&iStatus, *aNewImage->iAvatar, *aNewImage->iMask, 0);
						SetActive();
					}
				}
			}

			CleanupStack::PopAndDestroy(); // aAvatarData
		}
		else {
			// File was not found
			// Delete avatar from repository
			delete iAvatarList[aIndex];
			iAvatarList.Remove(aIndex);

			LoadAvatarItemL(aIndex);
		}
	}
}

void CAvatarRepository::DeleteAvatarItemL(TFileName aAvatarFile) {
	RFs aSession = CCoeEnv::Static()->FsSession();

	CFileUtilities::CompleteWithApplicationPath(aSession, aAvatarFile);

	aSession.Delete(aAvatarFile);
}

HBufC8* CAvatarRepository::LoadAvatarListFromFileLC(TFileName aListFile) {
	RFs aSession = CCoeEnv::Static()->FsSession();
	HBufC8* aFileData = NULL;
	RFile aFile;

	CFileUtilities::CompleteWithApplicationPath(aSession, aListFile);

	if(aFile.Open(aSession, aListFile, EFileStreamText|EFileRead) == KErrNone) {
		TInt aFileSize;
		aFile.Size(aFileSize);

		// Create buffer
		aFileData = HBufC8::NewLC(aFileSize);
		TPtr8 pFileData(aFileData->Des());

		// Read data
		aFile.Read(pFileData);
		aFile.Close();
	}

	if(aFileData) {
		return aFileData;
	}
	
	// Empty string
	return HBufC8::NewLC(0);
}

void CAvatarRepository::LoadAvatarItemsFromListL(const TDesC8& aXml) {
	CXmlParser* aXmlParser = CXmlParser::NewLC(aXml);

	if(aXmlParser->MoveToElement(_L8("avatars"))) {
		while(aXmlParser->MoveToElement(_L8("avatar"))) {
			// Create item
			CAvatarItem* aAvatarItem = new CAvatarItem();
			CleanupStack::PushL(aAvatarItem);

			// Set data
			aAvatarItem->iStored = true;
			aAvatarItem->iId = aXmlParser->GetIntAttribute(_L8("id"));
			aAvatarItem->iType = TRepositoryType(aXmlParser->GetIntAttribute(_L8("type")));
			aAvatarItem->iLevels = aXmlParser->GetIntAttribute(_L8("levels"));
			aAvatarItem->iFilename.Copy(aXmlParser->GetStringAttribute(_L8("file")));

			// Add to list
			iAvatarList.Append(aAvatarItem);
			CleanupStack::Pop(); // aAvatarItem
		}
	}

	CleanupStack::PopAndDestroy(); // aXmlParser
}

void CAvatarRepository::RenderMipmapImageL(CFbsBitmap* aSource, CFbsBitmap* aDestination, TRect aSourceRect) {
	CFbsBitmapDevice* aBufferDevice = CFbsBitmapDevice::NewL(aDestination);
	CleanupStack::PushL(aBufferDevice);
	
	if(aBufferDevice) {
		CFbsBitGc* aBufferGc;
		aBufferDevice->CreateContext(aBufferGc);
		CleanupStack::PushL(aBufferGc);
		
		aBufferGc->BitBlt(TPoint(0, 0), aSource, aSourceRect);
		CleanupStack::PopAndDestroy(); // aBufferGc
	}
	
	CleanupStack::PopAndDestroy(); // aBufferDevice
}

void CAvatarRepository::RenderMipmapImagesL(TInt aIndex) {
	CAvatarImage* aMasterImage = iAvatarList[aIndex]->iImages[0];
	TInt aIterationSize = aMasterImage->iSize.iHeight;
	TPoint aMasterCoord;
	
	for(TInt i = 0; i < iAvatarList[aIndex]->iLevels; i++) {
		CAvatarImage* aNewImage = new (ELeave) CAvatarImage();
		iAvatarList[aIndex]->iImages.Append(aNewImage);
		
		aNewImage->iAvatar = new (ELeave) CFbsBitmap();
		aNewImage->iSize = TSize(aIterationSize, aIterationSize);
		
		if(aNewImage->iAvatar->Create(aNewImage->iSize, aMasterImage->iAvatar->DisplayMode()) == KErrNone) {
			RenderMipmapImageL(aMasterImage->iAvatar, aNewImage->iAvatar, TRect(aMasterCoord.iX, aMasterCoord.iY, aMasterCoord.iX + aIterationSize, aMasterCoord.iY + aIterationSize));
		
			aNewImage->iMask = new (ELeave) CFbsBitmap();
			
			if(aNewImage->iMask->Create(aNewImage->iSize, aMasterImage->iMask->DisplayMode()) == KErrNone) {
				RenderMipmapImageL(aMasterImage->iMask, aNewImage->iMask, TRect(aMasterCoord.iX, aMasterCoord.iY, aMasterCoord.iX + aIterationSize, aMasterCoord.iY + aIterationSize));
			}			
		}
		
		if(aMasterCoord.iX == 0) {
			aMasterCoord.iX += aIterationSize;
		}
		else {
			aMasterCoord.iY += aIterationSize;
		}
		
		aIterationSize = aIterationSize / 2;
	}
	
	// Delete master mipmap image
	delete iAvatarList[aIndex]->iImages[0];
	iAvatarList[aIndex]->iImages.Remove(0);
}

void CAvatarRepository::InternalizeL() {
	// Load Icons
	HBufC8* aIconXml = LoadAvatarListFromFileLC(TFileName(_L("IconRepository.xml")));
	LoadAvatarItemsFromListL(*aIconXml);
	CleanupStack::PopAndDestroy(); // aIconXml

	// Load Avatars
	HBufC8* aAvatarXml = LoadAvatarListFromFileLC(TFileName(_L("AvatarRepository.xml")));
	LoadAvatarItemsFromListL(*aAvatarXml);
	CleanupStack::PopAndDestroy(); // aAvatarXml
}

void CAvatarRepository::ExternalizeL() {
}

CFbsBitmap* CAvatarRepository::GetImage(TInt aId, TBool aMask, TInt aMidmapLevel) {
	for(TInt i = 0; i < iAvatarList.Count(); i++) {
		if(iAvatarList[i]->iId == aId) {
			if(iAvatarList[i]->iReady) {
				if(aMidmapLevel < 0 || aMidmapLevel >= iAvatarList[i]->iImages.Count()) {
					aMidmapLevel = 0;
				}
				
				if(aMask) {
					return iAvatarList[i]->iImages[aMidmapLevel]->iMask;
				}
					
				return iAvatarList[i]->iImages[aMidmapLevel]->iAvatar;
			}
			
			break;
		}
	}

	return iNullImage;
}

void CAvatarRepository::RunL() {
	if(iRepositoryAction > ERepositoryIdle) {
		iRepositoryAction = ERepositoryIdle;

		if(iStatus == KErrNone) {
			// Get mip levels
			if(iAvatarList[iNextAvatarToLoad]->iLevels > 1) {
				RenderMipmapImagesL(iNextAvatarToLoad);
			}
			
			// Avatar is ready
			iAvatarList[iNextAvatarToLoad]->iReady = true;

			// Notify observer
			if(iAvatarList[iNextAvatarToLoad]->iType != ETypeOverlay) {
				iObserver->AvatarLoaded();
			}
		}

		// Reset
		iImageDecoder->Reset();

		// Load next avatar
		iNextAvatarToLoad++;

		while(iNextAvatarToLoad < iAvatarList.Count()) {
			if(iAvatarList[iNextAvatarToLoad]->iReady == false) {
				LoadAvatarItemL(iNextAvatarToLoad);
				break;
			}

			iNextAvatarToLoad++;
		}
	}
}

TInt CAvatarRepository::RunError(TInt /*aError*/) {
	return KErrNone;
}

void CAvatarRepository::DoCancel() {
	if(iRepositoryAction > ERepositoryIdle) {
		iRepositoryAction = ERepositoryIdle;
		iImageDecoder->Cancel();
	}
}

// End of File
