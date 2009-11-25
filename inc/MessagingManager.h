/*
============================================================================
 Name        : 	MessagingManager.h
 Author      : 	Ross Savage
 Copyright   : 	Buddycloud 2009
 Description : 	Management of message discussions
 History     : 	1.0

				1.0	Initial Development
============================================================================
*/

#include <e32base.h>
#include <e32cmn.h>
#include "BuddycloudFollowing.h"
#include "MessagingParticipants.h"
#include "Timer.h"

#ifndef MESSAGINGMANAGER_H_
#define MESSAGINGMANAGER_H_

/*
----------------------------------------------------------------------------
--
-- Enumerations
--
----------------------------------------------------------------------------
*/

enum TMessageContentType {
	EMessageTextual, EMessageAdvertisment, EMessageUserAction, EMessageUserEvent
};

enum TMessageJidType {
	EMessageJidRoster, EMessageJidRoom
};

enum TMessageLinkType {
	ELinkWebsite, ELinkUsername, ELinkChannel
};

/*
----------------------------------------------------------------------------
--
-- CMessage
--
----------------------------------------------------------------------------
*/

class CMessageLinkPosition {
	public:
		CMessageLinkPosition(TInt aStart, TInt aLength, TMessageLinkType aType = ELinkWebsite);
	
	public:
		TInt iStart;
		TInt iLength;
		
		TMessageLinkType iType;
};

class CMessage : public CBase {
	public:
		static CMessage* NewL();
		static CMessage* NewLC();
		~CMessage();
	
	private:
		CMessage();
		void ConstructL();
	
	public:
		TMessageContentType GetMessageType();

		TTime GetReceivedAt();
		void SetReceivedAt(TTime aTime);

		TInt GetAvatarId();
		void SetAvatarId(TInt aAvatarId);

		TBool GetRead();
		void SetRead(TBool aRead);

		TBool GetDirectReply();
		void SetDirectReply(TBool aDirectReply);

		TBool GetPrivate();
		void SetPrivate(TBool aPrivate);

		TBool GetOwn();
		void SetOwn(TBool aOwn);

		TDesC& GetJid();
		TMessageJidType GetJidType();
		void SetJidL(TDesC& aJid, TMessageJidType aType);

		TDesC& GetName();
		void SetNameL(const TDesC& aName);

		TDesC& GetBody();
		void SetBodyL(TDesC& aBody, const TDesC& aUsername, TMessageContentType aMessageType = EMessageTextual);

		TDesC8& GetThread();
		void SetThreadL(const TDesC8& aThread);
		
		TDesC& GetLocation();
		void SetLocationL(TDesC& aLocation, TBool aAppend = false);
		
		TInt GetLinkCount();
		CMessageLinkPosition GetLink(TInt aIndex);

		TBool Equals(CMessage* aMessage);

	protected:
		TMessageContentType iMessageType;
		TTime iReceivedAt;

		TInt iAvatarId;

		HBufC* iJid;
		TMessageJidType iJidType;
		
		HBufC* iName;
		HBufC* iBody;
		HBufC* iLocation;
		HBufC8* iThread;

		TBool iRead;
		TBool iDirectReply;
		TBool iPrivate;
		TBool iOwn;
		
		RArray<CMessageLinkPosition> iLinks;
};

/*
----------------------------------------------------------------------------
--
-- CDiscussion
--
----------------------------------------------------------------------------
*/

class MDiscussionMessagingObserver {
	public:
		virtual void TopicChanged(TDesC& aTopic) = 0;
		virtual void MessageDeleted(TInt aPosition) = 0;
		virtual void MessageReceived(CMessage* aMessage, TInt aPosition) = 0;
};

class MDiscussionReadObserver {
	public:
		virtual void AllMessagesRead(CFollowingItem* aItem) = 0;
};

class CDiscussion : public CBase {
	public:
		static CDiscussion* NewL(const TDesC& aJid);
		static CDiscussion* NewLC(const TDesC& aJid);
		~CDiscussion();
	
	private:
		CDiscussion();
		void ConstructL(const TDesC& aJid);

	public:
		TDesC& GetJid();
		void SetJidL(const TDesC& aJid);
		
		CFollowingItem* GetFollowingItem();

	public: // Observers
		void AddMessageReadObserver(MMessageReadObserver* aObserver);
		
		void AddDiscussionReadObserver(MDiscussionReadObserver* aObserver, CFollowingItem* aFollowingItem);
		
		void AddMessagingObserver(MDiscussionMessagingObserver* aObserver);
		void RemoveMessagingObserver();
		
	public:
		void CompressL();
		void CleanL();

	public: // Messages & settings
		TInt GetTotalMessages();
		TInt GetUnreadMessages();
		TInt GetUnreadReplies();
		void SetUnreadMessages(TInt aMessages, TInt aReplies = 0);

		void SetMessageRead(TInt aIndex);
		void SetMaxMessagesStored(TInt aLimit);

		void DeleteAll();
		void DeleteMessage(TInt aIndex);
		
		TBool GetNotificationOn();
		void SetNotificationOn(TBool aNotificationOn);
		
		CMessage* GetMessage(TInt aIndex);
		TBool AddMessageLD(CMessage* aMessage, TBool aDelayed = false);

	private:
		void InsertMessage(CMessage* aMessage, TInt aIndex);

	public: // Participants		
		CMessagingParticipantstore* GetParticipants();
				
	public: // Role & Affiliation
		TMucAffiliation GetMyAffiliation();
		TMucRole GetMyRole();
		void SetMyRoleAndAffiliation(TMucRole aRole, TMucAffiliation aAffiliation);
		
	private: // File interfaces
		void LoadDiscussionFromFileL();
		void SaveDiscussionToFileL();
		void DeleteDiscussionFileL();
		
	private:
		TFileName GetFileName(RFs& aSession);

	protected:
		// Discussion jid
		HBufC* iJid;

		// Observers
		MMessageReadObserver* iMessageReadObserver;
		MDiscussionReadObserver* iDiscussionReadObserver;
		MDiscussionMessagingObserver* iMessagingObserver;
		
		CFollowingItem* iFollowingItem;

		// Settings
		TInt iMessageLimit;
		TInt iUnreadMessages;
		TInt iUnreadReplies;
		
		TBool iNotificationOn;

		// Messages
		RPointerArray<CMessage> iMessages;
		
		// Participants
		CMessagingParticipantstore* iParticipantStore;
		
		// Role & Affiliation
		TMucRole iRole;
		TMucAffiliation iAffiliation;
		
		// Message compression control
		TBool iMessagesCached;
		TTime iLastUpdate;
};

/*
----------------------------------------------------------------------------
--
-- CMessagingManager
--
----------------------------------------------------------------------------
*/

class CMessagingManager : public CBase, MTimeoutNotification {
	public:
		static CMessagingManager* NewL();
		static CMessagingManager* NewLC();
		~CMessagingManager();
	
	private:
		void ConstructL();
	
	public:
		void CompressMessagesL();
		
	public:
		CDiscussion* GetDiscussionL(const TDesC& aJid);
		void DeleteDiscussionL(const TDesC& aJid);
		
	public: // From MTimeoutNotification
		void TimerExpired(TInt aExpiryId);
		
	protected:
		RPointerArray<CDiscussion> iDiscussions;
		
		CCustomTimer* iTimer;
};

#endif /* MESSAGINGMANAGER_H_ */
