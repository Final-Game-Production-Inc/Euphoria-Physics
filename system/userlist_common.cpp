// 
// system/userlist_common.cpp
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "userlist.h"

namespace rage
{

RAGE_DEFINE_CHANNEL(UserList)

#if !RSG_DURANGO
// PURPOSE:	Maps a user to player number.
// NOTES:	Some platforms do not have a concept of player number whereas
//			rage references a user by player number. This class maps a User 
//			to a player number and provides some accessors and helper functions.
class sysEmptyUserList : public sysUserList
{
public:
	// PURPOSE: Destructor.
	virtual ~sysEmptyUserList() {}
	
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

	// PURPOSE:	Init function to setup the singleton instance.
	virtual void Init();

	// PURPOSE: Shutdown function to shutdown the instance.
	virtual void Shutdown();

	// PURPOSE:	Updates the user list object.
	// NOTES:	Should be called once every frame.
	virtual void Update();

	// PURPOSE:	Resets the user list.
	virtual void Reset();

	// PURPOSE: Updates a user's info.
	// PARAMS:	userIndex - the player number/index to update.
	// NOTES:	This implementation does not do anything.
	virtual void UpdateUserInfo(u32 userIndex);

	// PURPOSE:	Retrieves the number of users that has been detected (e.g. with Kinect).
	// RETURNS:	The number of users detected users in the room or 0 on platforms that do not support this.
	//			body controller count at this instance in time. This could take a few ms in worst cases.
	//			This does not include guests.
	virtual u32 GetNumberOfDetectedUsersInRoom() const;

	// PURPOSE: Retrieves the number of users that are logged into the console.
	// RETURNS:	The number of users logged in on the console itself.
	// NOTES:	WARNING - This does not represent the number of players logged in on the game. This can be
	//			exceptionally slow on some platforms.
	virtual u32 GetNumberOfUsersLoggedInOnConsole() const;

	// PURPOSE: Retrieves the singleton instance.
	static sysEmptyUserList& GetPlatformInstance();

private:
	// PURPOSE:	Can only be constructed by sysEmptyUserList()
	sysEmptyUserList();

	// PURPOSE:	Static instance.
	static sysEmptyUserList sm_Instance;

	// PURPOSE: Non-copyable.
	sysEmptyUserList(const sysUserList&);
	sysEmptyUserList& operator=(const sysUserList&);
};

sysEmptyUserList sysEmptyUserList::sm_Instance;

sysEmptyUserList::sysEmptyUserList()
	: sysUserList()
{
}

sysUserList& sysUserList::GetInstance()
{
	return sysEmptyUserList::GetPlatformInstance();
}

const sysUserInfo* sysEmptyUserList::GetUserInfo( u32 ) const
{
	return NULL;
}

const sysUserInfo* sysEmptyUserList::GetUserInfoById( u64 ) const
{
	return NULL;
}

s32 sysEmptyUserList::GetMaxUsers() const
{
	return 0;
}

void sysEmptyUserList::Init()
{
}

void sysEmptyUserList::Shutdown()
{
}

void sysEmptyUserList::Update()
{
}

void sysEmptyUserList::Reset()
{

}

void sysEmptyUserList::UpdateUserInfo( u32 )
{
}

sysEmptyUserList& sysEmptyUserList::GetPlatformInstance()
{
	return sm_Instance;
}

u32 sysEmptyUserList::GetNumberOfDetectedUsersInRoom() const
{
	return 0;
}

u32 sysEmptyUserList::GetNumberOfUsersLoggedInOnConsole() const
{
	return 0;
}

#endif // !RSG_DURANGO


void sysUserList::AddDelegate(sysUserList::Delegate* dlgt)
{
	m_Delegator.AddDelegate(dlgt);
}

void sysUserList::RemoveDelegate(sysUserList::Delegate* dlgt)
{
	m_Delegator.RemoveDelegate(dlgt);
}

void sysUserList::CallDelegates(const sysUserInfo& userInfo)
{
	m_Delegator.Dispatch(userInfo);
}

}
