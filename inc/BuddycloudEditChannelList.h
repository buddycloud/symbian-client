/*
============================================================================
 Name        : BuddycloudEditChannelList.h
 Author      : Ross Savage
 Copyright   : 2009 Buddycloud
 Description : Declares Edit Channel List
============================================================================
*/

#ifndef BUDDYCLOUDEDITCHANNELLIST_H_
#define BUDDYCLOUDEDITCHANNELLIST_H_

// INCLUDES
#include <coecntrl.h>
#include <aknsettingitemlist.h>
#include "BuddycloudEditChannelList.h"
#include "BuddycloudLogic.h"
#include "TextUtilities.h"
#include "ViewReference.h"
#include "XmppInterfaces.h"

// CLASS DECLARATION
class CBuddycloudEditChannelList : public CAknSettingItemList, MXmppStanzaObserver {
	public: // Constructors and destructor
		CBuddycloudEditChannelList(CBuddycloudLogic* aBuddycloudLogic);
		void ConstructL(const TRect& aRect, TViewData aQueryData);
		~CBuddycloudEditChannelList();
		
	private:
		void SetTitleL(TInt aResourceId);
		
		void CollectChannelMetadataL(const TDesC& aNodeId);

	public:
		void EditCurrentItemL();
		void LoadChannelDataL();
		TInt SaveChannelDataL();

	public:
		void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);

	public: // From CAknSettingItemList
		void EditItemL(TInt aIndex, TBool aCalledFromMenu);

	private:
		CAknSettingItem* CreateSettingItemL (TInt aSettingId);

	public: // From MXmppStanzaObserver
		void XmppStanzaAcknowledgedL(const TDesC8& aStanza, const TDesC8& aId);

	private: // Data
		CBuddycloudLogic* iBuddycloudLogic;	
		MXmppWriteInterface* iXmppInterface;
		
		TViewData iQueryData;
		
		CTextUtilities* iTextUtilities;

		CFollowingChannelItem* iChannelItem;
		TBool iChannelSaveAllowed;
		TBool iChannelEditAllowed;
		
		TInt iTitleResourceId;
		
		TBuf<64> iTitle;
		TBuf<64> iId;
		TBuf<256> iDescription;
		TInt iAccess;
		TInt iAffiliation;
};

#endif

// End of File
