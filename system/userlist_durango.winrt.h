// 
// system/userlist_durango.winrt.h
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTE_USERLIST_DURANGO_WINRT_H
#define SYSTE_USERLIST_DURANGO_WINRT_H

#if RSG_DURANGO

#include "userlist.h"

// Rage headers.
#include "atl/array.h"
#include "system/criticalsection.h"
#include "system/service.h"

namespace rage
{
class sysDurangoUserList;

// PURPOSE:	User information specific to Durango.
class sysDurangoUserInfo : public sysUserInfo
{
public:
	// PURPOSE: Only sysDurangoUserList can create and edit a sysDurangoUserInfo.
	friend class sysDurangoUserList;
	friend class atArray<sysDurangoUserInfo>;

	// PURPOSE: Destructor.
	virtual ~sysDurangoUserInfo() {}

	// PURPOSE: Retrieves the player index.
	virtual s32 GetUserIndex() const;

	// PURPOSE: Retrieves the player id.
	virtual u64 GetUserId() const;

	// PRUPOSE: Indicates if the user is signed in.
	virtual bool IsSignedIn() const;

	// PURPOSE: Retrieves the Platform specific user.
	Windows::Xbox::System::User^ GetPlatformUser() const;

	// PURPOSE: Retrieves the Platform specific pad.
	Windows::Xbox::Input::IGamepad^ GetPlatformGamepad() const;

private:
	// PURPOSE: Stops creation of instances except for sysDurangoUserList which is a friend.
	sysDurangoUserInfo();

	// PURPOSE: Non-copyable.
	sysDurangoUserInfo(const sysDurangoUserInfo*);
	sysDurangoUserInfo* operator=(const sysDurangoUserInfo*);

	// PURPOSE: Resets a user info object.
	void Reset(u32 userIndex);

	// PURPOSE: Sets a users pad.
	void SetGamepad(Windows::Xbox::Input::IGamepad^ gamepad);

	// PURPOSE: Sets a user.
	void SetUser(Windows::Xbox::System::User^ user);

	// PURPOSE: The user.
	Windows::Xbox::System::User^ m_User;

	// PURPOSE: The user's gamepad.
	Windows::Xbox::Input::IGamepad^ m_Gamepad;

	// PURPOSE:	The cached unique user id.
	// NOTES:	This is cached from m_User as it is a string.
	u64 m_UserId;

	// PURPOSE: The user player index.
	s32 m_UserIndex;
};


// PURPOSE: Platform specific user list object.
class sysDurangoUserList : public sysUserList
{
public:
	// Purpose: Destructor.
	virtual ~sysDurangoUserList();

	// PURPOSE: Retrieves the singleton instance.
	static sysDurangoUserList& GetPlatformInstance();

	// PURPOSE:	Init function to setup the singleton instance.
	virtual void Init();

	// PURPOSE: Shutdown function to shutdown the instance.
	virtual void Shutdown();

	// PURPOSE:	Updates the user list object.
	// NOTES:	Should be called once every frame.
	virtual void Update();

	// PURPOSE: Retrieves the user info for the user with the specified index.
	// PARAMS:	userIndex - the player number/index to retrieve.
	// RETURNS:	The user info object or NULL if there is no user assigned to the index.
	virtual const sysUserInfo* GetUserInfo(u32 userIndex) const;

	// PURPOSE: Retrieves the user info for the user with the specified user id.
	// PARAMS:	userId - the user's unique id.
	// RETURNS:	The user info object or NULL if there is no user assigned to the id.
	virtual const sysUserInfo* GetUserInfoById(u64 userId) const;

	// PURPOSE: Retrieves the maximum number of logged in users.
	virtual s32 GetMaxUsers() const;

	// PURPOSE:	Resets the user list.
	virtual void Reset();

	// PURPOSE: Updates a user's info.
	// PARAMS:	userIndex - the player number/index to update.
	// NOTES:	On Durango, this will scan for a pad with input to re-pair to the user.
	virtual void UpdateUserInfo(u32 userIndex);

	// PURPOSE: Retrieves the platform user info for the user with the specified index.
	// PARAMS:	userIndex - the player number/index to retrieve.
	// RETURNS:	The user info object or NULL if there is no user assigned to the id.
	const sysDurangoUserInfo* GetPlatformUserInfo(u32 userIndex) const;

	// PURPOSE: Retrieves the user info for a specified User object.
	// PARAMS:	user - the user's object.
	// RETURNS:	The user info object or NULL if there is no user with this.
	// NOTES:	A User^ object can become 'stale'/invalid when a user logs out and a different User^
	//			object will be created upon them re-logging in. It *IS* safe to pass the invalid 'stale'
	//			User^ object here to retrieve the new valid User^ object.
	const sysDurangoUserInfo* GetPlatformUserInfo(Windows::Xbox::System::User^ user) const;

	// PURPOSE: Retrieves the user info for a specified gamepad object.
	// PARAMS:	user - the user's object.
	// RETURNS:	The user info object or NULL if there is no user with this.
	const sysDurangoUserInfo* GetPlatformUserInfo(Windows::Xbox::Input::IGamepad^ gamepad) const;

	// PURPOSE: Retrieves the user info for the user with the specified user id.
	// PARAMS:	userId - the user's unique id.
	// RETURNS:	The user info object or NULL if there is no user assigned to the id.
	const sysDurangoUserInfo* GetPlatformUserInfoById(u64 userId) const;

	// PURPOSE: Retrieves the user info for the user with the specified user session id.
	// PARAMS:	sessionId - the user's unique id.
	// RETURNS:	The user info object or NULL if there is no user assigned to the session id.
	const sysDurangoUserInfo* GetPlatformUserInfoBySessionId(u32 sessionId) const;

	// PURPOSE: Retrieves the platform user info for the signed in user index
	// PARAMS:	signedInUserIndex : the signed in user Index
	// NOTES:	Does not match the normal userIndex we use for internal mapping, this includes inactive profiles
	const sysDurangoUserInfo* GetSignedInPlatformUserInfo(u32 signedInUserIndex) const;

	// PURPOSE: Retrieves the platform user info for the signed in user index
	// PARAMS:	userId - xuid to lookup
	// NOTES:	Does not match the normal userIndex we use for internal mapping, this includes inactive profiles
	const sysDurangoUserInfo* GetSignedInPlatformUserInfo(u64 userId) const;

	// PURPOSE: Retrieves the platform user info for the signed in user index
	// PARAMS:	userId - xuid to lookup
	// NOTES:	Does not match the normal userIndex we use for internal mapping, this includes inactive profiles
	const sysDurangoUserInfo* GetSignedInPlatformUserInfo(Platform::String^ userHash) const;

	// PURPOSE:	Retrieves the number of users that Kinect has detected.
	// RETURNS:	The number of users Kinect has detected.
	// NOTES:	WARNING - This functions can be slow! It polls the live body controller count at
	//			this instant in time. This could take a few ms in worst cases. This does not include guests.
	virtual u32 GetNumberOfDetectedUsersInRoom() const;

	// PURPOSE: Retrieves the number of users that are logged into the console.
	// RETURNS:	The number of users logged in on the console itself.
	// NOTES:	WARNING - This does not represent the number of players logged in on the game. This can be
	//			exceptionally slow on some platforms.
	virtual u32 GetNumberOfUsersLoggedInOnConsole() const;

	// PURPOSE: Retrieves the Platform specific controller.
	Windows::Xbox::Input::IGamepad^ GetPlatformGamepad() const;

	// PURPOSE: Sets the user and gamepad for a given slot.
	// PARAMS:	userIndex - the player number of the user to update.
	//			user - the user object to set in the slot (pass nullptr to remove a user).
	//			gamepad - the gamepad to assing to the user (pass nullptr to remove the gamepad).
	//			moveCurrentUserToFreeSlot - will move the current logged in user to another free slot.
	void SetUserInfo(s32 userIndex, Windows::Xbox::System::User^ user, Windows::Xbox::Input::IGamepad^ gamepad, bool moveCurrentUserToFreeSlot);

	// PURPOSE:
	//	Returns TRUE if m_LiveUserListUpdated is true for any index (i.e. a callback is fired, we're waiting to process)
	bool IsWaitingForLiveUserListUpdate();

private:
	// PURPOSE: Creates singleton instance.
	sysDurangoUserList();

	// PURPOSE: Non-copyable.
	sysDurangoUserList(const sysDurangoUserList&);
	sysDurangoUserList& operator=(const sysDurangoUserList&);

	// PURPOSE: Builds the list of live users.
	void CreateLiveUsersList();

	// PURPOSE:	Validates the user's positions in the live list upon a resume.
	// NOTES:	The live list is recreated upon resume using CreateLiveUsersList(). It is possible, however, that
	//			users move position (index) in the live list. This function puts users back into the same positions (index)
	//			as the current list (e.g. User A could be player 1 before resume but player 2 after resume, this will fix this
	//			up so users remain the same player number).
	void ValidateUsers();

	// PURPOSE: Resets the user lists.
	void ResetLists();

	// PURPOSE: Swaps two live users.
	void SwapLiveUsers(u32 indexA, u32 indexB);

	// PURPOSE:	Helper function to see if a gamepad has input form one of its face buttons.
	// PARAMS:	The gamepad to check.
	// NOTES:	This is used when finding a gamepad a user trying to use.
	static bool DoesGamepadHaveInput(Windows::Xbox::Input::IGamepad^ gamepad);

	// PURPOSE: The maximum number of logged in users.
	const static s32 MAX_USERS = 4;

	// PURPOSE: The length of time to wait to re-validate the user lists.
	const static u32 VALIDATE_WAIT_TIME = 500u;

	// PURPOSE:	Array of live users.
	// NOTES:	These are used internally and contain live user state. These can change
	//			mid frame.
	sysDurangoUserInfo m_LiveUsers[MAX_USERS];

	// PURPOSE: Array of users accessible through this user list.
	// NOTES:	These users are guaranteed to persist for the frame. Any user additions or
	//			removals will not update until Update() is called.
	sysDurangoUserInfo m_CurrentUsers[MAX_USERS];

	// PURPOPSE: Array of users logged into the console
	// NOTES: Do not query this array for anything other than size. 
	//		  Use m_CurrentUsers for the gamepad mapped users
	atArray<sysDurangoUserInfo> m_ConsoleSignedInUsers;

	// PURPOSE:	Flag to indicate the live user list has changed.
	bool m_LiveUserListUpdated[MAX_USERS];

	// PURPOSE: Delegate for suspend/resume callback.
	ServiceDelegate m_ServiceDelegate;

	// PURPOSE:	The time to start a validate chec.
	// NOTES:	According to Microsoft, we should leave the game a small amount of time after
	//			a resume to re-establish users and gamepads.
	u32 m_ValidateStartTime;

	// PUPORSE: Ensure thread safety.
	mutable sysCriticalSectionToken m_Cs;

	// PURPOSE: Singleton instance.
	static sysDurangoUserList sm_Instance;

	// PURPOSE: Durango gamepad added event handler.
	void OnGamepadAdded(Windows::Xbox::Input::IGamepad^ gamepad);

	// PURPOSE: Durango gamepad removed event hander.
	void OnGamepadRemoved(Windows::Xbox::Input::IGamepad^ gamepad);

	// PURPOSE: Durango gamepad pairing changed event hander.
	void OnGamepadPairingChanged(Windows::Xbox::System::User^ user, Windows::Xbox::Input::IGamepad^ gamepad);

	// PURPOSE: Durango user signed in event handler.
	void OnUserSignedIn(Windows::Xbox::System::User^ user);

	// PURPOSE: Durango user signed out event handler.
	void OnUserSignedOut(Windows::Xbox::System::User^ user);

	// PURPOSE: Service event handler.
	// NOTES:	This is used to recieve the resume events.
	void OnServiceEvent(sysServiceEvent* evnt);

	// PURPOSE: Token of the gamepad added event hander.
	Windows::Foundation::EventRegistrationToken m_GamepadAddedToken;

	// PURPOSE: Token of the gamepad removed event hander.
	Windows::Foundation::EventRegistrationToken m_GamepadRemovedToken;

	// PURPOSE: Token of the gamepad pairing changed event hander.
	Windows::Foundation::EventRegistrationToken m_GamepadPairingChangedToken;

	// PURPOSE: Token of the user signed out event hander.
	Windows::Foundation::EventRegistrationToken m_UserSignedOutToken;

	// PURPOSE: Token of the user signed in event hander.
	Windows::Foundation::EventRegistrationToken m_UserSignedInToken;
};

inline sysDurangoUserInfo::sysDurangoUserInfo()
	: sysUserInfo()
	, m_User(nullptr)
	, m_Gamepad(nullptr)
	, m_UserId(INVALID_USER_ID)
	, m_UserIndex(INVALID_USER_INDEX)
{
}

inline Windows::Xbox::System::User^ sysDurangoUserInfo::GetPlatformUser() const
{
	return m_User;
}

inline Windows::Xbox::Input::IGamepad^ sysDurangoUserInfo::GetPlatformGamepad() const
{
	return m_Gamepad;
}

inline void sysDurangoUserInfo::SetGamepad(Windows::Xbox::Input::IGamepad^ gamepad)
{
	m_Gamepad = gamepad;
}

inline void sysDurangoUserInfo::Reset(u32 userIndex)
{
	m_UserIndex = userIndex;
	m_User = nullptr;
	m_Gamepad = nullptr;
}

inline sysDurangoUserList& sysDurangoUserList::GetPlatformInstance()
{
	return sm_Instance;
}

}

#endif // RSG_DURANGO

#endif // SYSTE_USERLIST_DURANGO_WINRT_H
