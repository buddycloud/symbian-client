/*
============================================================================
 Name        : BuddycloudList.h
 Author      : Ross Savage
 Copyright   : Buddycloud 2009
 Description : Generic list class & storage
============================================================================
*/

#ifndef BUDDYCLOUDLIST_H_
#define BUDDYCLOUDLIST_H_

#include <e32base.h>

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TSortByType {
	ESortByUnsorted, ESortByDistance, ESortByActivity, ESortByAlphabetically
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudListItem
--
----------------------------------------------------------------------------
*/

class CBuddycloudListItem : public CBase {
	public:
		static CBuddycloudListItem* NewL();
		static CBuddycloudListItem* NewLC();
		virtual ~CBuddycloudListItem();
	
	protected:
		CBuddycloudListItem();
		void ConstructL();

	public:
		TInt GetItemId();
		void SetItemId(TInt aItemId);
		
	public:
		TTime GetLastUpdated();
		void SetLastUpdated(TTime aTime);
		
		TInt GetDistance();
		void SetDistance(TInt aDistance);

	public:
		TBool Filtered();
		void SetFiltered(TBool aFilter);
		
	public:
		TInt GetIconId();
		void SetIconId(TInt aIconId);

	public:
		TDesC& GetTitle();
		void SetTitleL(const TDesC& aTitle);

		TDesC& GetSubTitle();
		void SetSubTitleL(const TDesC& aSubTitle);
		
		TDesC& GetDescription();
		void SetDescriptionL(const TDesC& aDescription);

	protected:
		TInt iItemId;
		
		TTime iLastUpdated;
		TInt iDistance;
		
		TBool iFiltered;
		
		TInt iIconId;	
		HBufC* iTitle;
		HBufC* iSubTitle;
		HBufC* iDescription;
};

/*
----------------------------------------------------------------------------
--
-- CBuddycloudListStore
--
----------------------------------------------------------------------------
*/

class CBuddycloudListStore : public CBase {
	public:
		static CBuddycloudListStore* NewL();
		static CBuddycloudListStore* NewLC();
		~CBuddycloudListStore();
		
	protected:
		void ConstructL();

	public: // Count & get items
		TInt Count();

		CBuddycloudListItem* GetItemByIndex(TInt aIndex);
		CBuddycloudListItem* GetItemById(TInt aItemId);
		
		TInt GetIdByIndex(TInt aIndex);
		TInt GetIndexById(TInt aItemId);

	public: // Move items
		void MoveItemByIndex(TInt aIndex, TInt aPosition);
		void MoveItemById(TInt aItemId, TInt aPosition);

	public: // Add & insert items
		void AddItem(CBuddycloudListItem* aItem);
		void InsertItem(TInt aIndex, CBuddycloudListItem* aItem);

	public: // Remove & delete items
		void RemoveItemByIndex(TInt aIndex);
		void RemoveItemById(TInt aItemId);
		void RemoveAll();

		void DeleteItemByIndex(TInt aIndex);
		void DeleteItemById(TInt aItemId);
		
	public: // Filter items
		TDesC& GetFilterText();
		TBool SetFilterTextL(const TDesC& aFilterText);
		
	protected: 
		virtual void FilterItemL(TInt aIndex);
		
	private:
		void FilterL();

	protected:
		RPointerArray<CBuddycloudListItem> iItemStore;
		
		HBufC* iFilterText;
};

#endif /* BUDDYCLOUDLIST_H_ */
