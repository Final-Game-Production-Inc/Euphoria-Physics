// 
// system/userlist.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_USERLIST_H
#define SYSTEM_USERLIST_H

#include "atl/delegate.h"
#include "diag/channel.h"

namespace rage
{

RAGE_DECLARE_CHANNEL(UserList)

#define userlistAssert(cond)					RAGE_ASSERT(UserList,cond)
#define userlistAssertf(cond,fmt,...)			RAGE_ASSERTF(UserList,cond,fmt,##__VA_ARGS__)
#define userlistFatalAssertf(cond,fmt,...)		RAGE_FATALASSERTF(UserList,cond,fmt,##__VA_ARGS__)
#define userlistVerify(cond)					RAGE_VERIFY(UserList,cond)
#define userlistVerifyf(cond,fmt,...)			RAGE_VERIFYF(UserList,cond,fmt,##__VA_ARGS__)
#define userlistErrorf(fmt,...)					RAGE_ERRORF(UserList,fmt,##__VA_ARGS__)
#define userlistWarningf(fmt,...)				RAGE_WARNINGF(UserList,fmt,##__VA_ARGS__)
#define userlistDisplayf(fmt,...)				RAGE_DISPLAYF(UserList,fmt,##__VA_ARGS__)
#define userlistDebugf1(fmt,...)				RAGE_DEBUGF1(UserList,fmt,##__VA_ARGS__)
#define userlistDebugf2(fmt,...)				RAGE_DEBUGF2(UserList,fmt,##__VA_ARGS__)
#define userlistDebugf3(fmt,...)				RAGE_DEBUGF3(UserList,fmt,##__VA_ARGS__)
#define userlistLogf(severity,fmt,...)			RAGE_LOGF(UserList,severity,fmt,##__VA_ARGS__)

// PURPOSE: Base class for platform user information.
class sysUserInfo
{
public:
	virtual ~sysUserInfo(){}

	// PURPOSE: Retrieves the player index.
	virtual s32 GetUserIndex() const = 0;

	// PURPOSE: Retrieves the player id.
	virtual u64 GetUserId() const = 0;

	// PRUPOSE: Indicates if the user is signed in.
	virtual bool IsSignedIn() const = 0;

	// PURPOSE:	Indicates an invalid user id.
	const static u64 INVALID_USER_ID = 0;

	// PURPOSE:	A constant representing an invalid user index.
	const static s32 INVALID_USER_INDEX = -1;

protected:
	// PURPOSE: Ensures a user info object is created through a concrete implementation.
	sysUserInfo() {}
};

// PURPOSE:	Maps a user to player number.
// NOTES:	Some platforms do not have a concept of player number whereas
//			rage references a user by player number. This class maps a User 
//			to a player number and provides some accessors and helper functions.
class sysUserList
{
public:
	// PURPOSE: Destructor.
	virtual ~sysUserList() {}

	// PURPOSE:	Delegator for changes to the user list.
	// NOTES:	The delegate parameter takes a const reference to the changed sysUserInfo.
	typedef atDelegator<void (const sysUserInfo&)> Delegator;

	// PURPOSE:	Delegate for changes to the user list.
	typedef Delegator::Delegate Delegate;

	// PURPOSE: Retrieves an instance to the singleton object.
	static sysUserList& GetInstance();

	// PURPOSE:	Init function to setup the singleton instance.
	virtual void Init() = 0;

	// PURPOSE: Shutdown function to shutdown the instance.
	virtual void Shutdown() = 0;

	// PURPOSE:	Updates the user list object.
	// NOTES:	Should be called once every frame.
	virtual void Update() = 0;
	
	// PURPOSE: Retrieves the user info for the user with the specified index.
	// PARAMS:	userIndex - the player number/index to retrieve.
	// RETURNS:	The user info object or NULL if there is no user assigned to the index.
	virtual const sysUserInfo* GetUserInfo(u32 userIndex) const = 0;

	// PURPOSE: Retrieves the user info for the user with the specified user id.
	// PARAMS:	userId - the user's unique id.
	// RETURNS:	The user info object or NULL if there is no user assigned to the id.
	virtual const sysUserInfo* GetUserInfoById(u64 userId) const = 0;

	// PURPOSE: Retrieves the maximum number of logged in users.
	virtual s32 GetMaxUsers() const = 0;

	// PURPOSE:	Resets the user list.
	virtual void Reset() = 0;

	// PURPOSE: Updates a user's info.
	// PARAMS:	userIndex - the player number/index to update.
	// NOTES:	The behavior of this is platform specific. It will update any invalid user info.
	virtual void UpdateUserInfo(u32 userIndex) = 0;

	// PURPOSE:	Retrieves the number of users that has been detected (e.g. with Kinect).
	// RETURNS:	The number of users detected users in the room or 0 on platforms that do not support this.
	// NOTES:	WARNING - On some platforms this functions can be slow! It polls the live
	//			body controller count at this instance in time. This could take a few ms in worst cases.
	//			This does not include guests.
	virtual u32 GetNumberOfDetectedUsersInRoom() const = 0;

	// PURPOSE: Retrieves the number of users that are logged into the console.
	// RETURNS:	The number of users logged in on the console itself.
	// NOTES:	WARNING - This does not represent the number of players logged in on the game. This can be
	//			exceptionally slow on some platforms.
	virtual u32 GetNumberOfUsersLoggedInOnConsole() const = 0;

	// PURPOSE:	Adds a delegate to be called whenever a change to the user list is called.
	// PARAMS:	dlgt - the delegate to be added.
	void AddDelegate(Delegate* dlgt);

	// PURPOSE: Removes a delegate that has previously been added.
	// PARAMS:	dlgt - the delegate to b added.
	void RemoveDelegate(Delegate* dlgt);

protected:
	// PURPOSE: Ensures an instance can only be created through a concrete implementation.
	sysUserList() {}

	// PURPOSE: Calls all delegates that have been added.
	void CallDelegates(const sysUserInfo& userInfo);

protected:
	// PURPOSE: Non-copyable.
	sysUserList(const sysUserList&);
	sysUserList& operator=(const sysUserList&);

	// PURPOSE: Delegator to call delagates when a change occurs to the user list.
	Delegator m_Delegator;
};

}

#endif // SYSTEM_USERLIST_H