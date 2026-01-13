// 
// system/userlist_durango.winrt.cpp
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#if RSG_DURANGO

#include "new_winrt.h"
#include "userlist_durango.winrt.h"

// Rage headers.
#include "system/timer.h"
#include "system/stack.h"

namespace rage
{
namespace dns
{
	using Windows::Foundation::Collections::IVectorView;
	using Windows::Foundation::EventHandler;
	using Windows::Xbox::Input::Controller;
	using Windows::Xbox::Input::ControllerPairingChangedEventArgs;
	using Windows::Xbox::Input::BodyController;
	using Windows::Xbox::Input::Gamepad;
	using Windows::Xbox::Input::GamepadAddedEventArgs;
	using Windows::Xbox::Input::GamepadButtons;
	using Windows::Xbox::Input::GamepadRemovedEventArgs;
	using Windows::Xbox::Input::IBodyController;
	using Windows::Xbox::Input::IGamepad;
	using Windows::Xbox::Input::RawGamepadReading;
	using Windows::Xbox::System::SignOutCompletedEventArgs;
	using Windows::Xbox::System::SignInCompletedEventArgs;
	using Windows::Xbox::System::User;
	using Windows::Xbox::System::UserDisplayInfo;
}

sysDurangoUserList sysDurangoUserList::sm_Instance;

sysUserList& sysUserList::GetInstance()
{
	return sysDurangoUserList::GetPlatformInstance();
}

s32 sysDurangoUserInfo::GetUserIndex() const
{
	return m_UserIndex;
}

u64 sysDurangoUserInfo::GetUserId() const
{
	return m_UserId;
}

bool sysDurangoUserInfo::IsSignedIn() const
{
	return m_User != nullptr;
}

void sysDurangoUserInfo::SetUser(Windows::Xbox::System::User^ user)
{
	m_User = user;
	if(user != nullptr)
	{
		// This is how the network code extracted the user id.
		userlistVerify(swscanf_s(m_User->XboxUserId->Data(), L"%" LI64FMT L"u", &m_UserId) == 1);
	}
	else
	{
		m_UserId = INVALID_USER_ID;
	}
}

sysDurangoUserList::sysDurangoUserList()
	: sysUserList()
	, m_GamepadAddedToken()
	, m_GamepadRemovedToken()
	, m_GamepadPairingChangedToken()
	, m_ServiceDelegate()
	, m_ValidateStartTime(0u)
	, m_Cs()
{
	try
	{
		// Cache the console's list of users
		auto users = Windows::Xbox::System::User::Users;
		for (unsigned i = 0; i < users->Size; ++i )
		{
			auto pUser = users->GetAt( i );
			OnUserSignedIn(pUser);
		}  

		m_GamepadAddedToken = dns::Gamepad::GamepadAdded += ref new dns::EventHandler<dns::GamepadAddedEventArgs^>( [=](Platform::Object^, dns::GamepadAddedEventArgs^ args)
		{
			sysCriticalSection lock(m_Cs);
			try
			{
				OnGamepadAdded(args->Gamepad);
			}
			catch(Platform::Exception^)
			{
				userlistAssertf(false, "Error in GampadAdded event handler!");
			}
		} );

		m_GamepadRemovedToken = dns::Gamepad::GamepadRemoved += ref new dns::EventHandler<dns::GamepadRemovedEventArgs^>( [=](Platform::Object^, dns::GamepadRemovedEventArgs^ args)
		{
			sysCriticalSection lock(m_Cs);
			try
			{
				OnGamepadRemoved(args->Gamepad);
			}
			catch(Platform::Exception^)
			{
				userlistAssertf(false, "Error in GampadRemoved event handler!");
			}
		} );

		m_GamepadPairingChangedToken = dns::Controller::ControllerPairingChanged += ref new dns::EventHandler<dns::ControllerPairingChangedEventArgs^>( [=](Platform::Object^, dns::ControllerPairingChangedEventArgs^ args)
		{
			sysCriticalSection lock(m_Cs);
			try
			{
				if(args != nullptr && args->Controller->Type == Platform::StringReference(L"Windows.Xbox.Input.Gamepad"))
				{
					OnGamepadPairingChanged(args->User, static_cast<dns::IGamepad^>(args->Controller));
				}
			}
			catch(Platform::Exception^)
			{
				userlistAssertf(false, "Error in ControllerPairingChanged event handler!");
			}
		} );

		m_UserSignedOutToken = dns::User::SignOutCompleted += ref new dns::EventHandler<dns::SignOutCompletedEventArgs^>( [=](Platform::Object^, dns::SignOutCompletedEventArgs^ args)
		{
			sysCriticalSection lock(m_Cs);
			try
			{
				OnUserSignedOut(args->User);
			}
			catch(Platform::Exception^)
			{
				userlistAssertf(false, "Error in SignOutCompleted event handler!");
			}
		} );

		m_UserSignedInToken = dns::User::SignInCompleted += ref new dns::EventHandler<dns::SignInCompletedEventArgs^ >( [=]( Platform::Object^ , dns::SignInCompletedEventArgs ^ args )
		{
			sysCriticalSection lock(m_Cs);
			try
			{
				OnUserSignedIn(args->User);
			}
			catch(Platform::Exception^)
			{
				userlistAssertf(false, "Error in SignInCompleted event handler!");
			}
		} );

		sysCriticalSection lock(m_Cs);
		ResetLists();
		CreateLiveUsersList();

		m_ServiceDelegate.Bind(this, &sysDurangoUserList::OnServiceEvent);
		g_SysService.AddDelegate(&m_ServiceDelegate);
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error initialzing user list!");
	}
}

sysDurangoUserList::~sysDurangoUserList()
{
	try
	{
		sysCriticalSection lock(m_Cs);
		g_SysService.RemoveDelegate(&m_ServiceDelegate);
		m_ServiceDelegate.Unbind();
		dns::Gamepad::GamepadAdded -= m_GamepadAddedToken;
		dns::Gamepad::GamepadRemoved -= m_GamepadRemovedToken;
		dns::Controller::ControllerPairingChanged -= m_GamepadPairingChangedToken;
		dns::User::SignOutCompleted -= m_UserSignedOutToken;
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error removing callbacks!");
	}
}

void sysDurangoUserList::CreateLiveUsersList()
{
	try
	{
		userlistDebugf1("Creating live user list.");

		// This is faster than accessing Gamepad::Gamepads directly which is very slow according to MS.
		dns::IVectorView<dns::IGamepad^>^ slowGamepads = dns::Gamepad::Gamepads;
		Platform::Array<dns::IGamepad^>^ gamepads = ref new Platform::Array<dns::IGamepad^>(slowGamepads->Size);
		const u32 numGamepads = slowGamepads->GetMany(0, gamepads);

		u32 padIndex = 0;
		for (u32 i = 0;  i < numGamepads && padIndex < MAX_USERS; ++i)
		{
			dns::IGamepad^ gamepad = gamepads[i];
			dns::User^ user = gamepad->User;

			if(user != nullptr)
			{
				// It turns out that a user can be paired to more than one controller. If they are paired to more than one at the start we will use the first
				// controller as their active controller. This is to ensure a user is not entered into the list more than once.
				bool userAlreadyPaired = false;
				for(u32 pairedIndex = 0; (userAlreadyPaired == false && pairedIndex < padIndex); ++pairedIndex)
				{
					if(m_LiveUsers[pairedIndex].GetPlatformUser()->Id == user->Id)
					{
						userAlreadyPaired = true;
					}
				}

				if(userAlreadyPaired == false)
				{
					m_LiveUsers[padIndex].SetGamepad(gamepad);
					m_LiveUsers[padIndex].SetUser(user);
					m_LiveUserListUpdated[padIndex] = true;
					userlistDebugf1("Init User: Gamepad %u, index %d, user %" I64FMT "u", gamepad->Id, padIndex, m_LiveUsers[padIndex].m_UserId);
					++padIndex;
				}
			}
		}

		// Now add the pads without a user.
		for (u32 i = 0;  i < numGamepads && padIndex < MAX_USERS; ++i)
		{
			dns::IGamepad^ gamepad = gamepads[i];

			if(gamepad->User == nullptr)
			{
				m_LiveUsers[padIndex].SetGamepad(gamepad);
				m_LiveUsers[padIndex].SetUser(nullptr); // Clear the user incase there was one previously.
				userlistDebugf1("Init Pad: Gamepad %u, index %d", gamepad->Id, padIndex);
				m_LiveUserListUpdated[padIndex] = true;
				++padIndex;
			}
		}

		// Ensure the remaining entries are cleared.
		for(u32 i = padIndex; i < MAX_USERS; ++i)
		{
			m_LiveUsers[i].SetGamepad(nullptr);
			m_LiveUsers[i].SetUser(nullptr);
			m_LiveUserListUpdated[i] = true;
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error resetting user list!");
	}
}

void sysDurangoUserList::Reset()
{
	sysCriticalSection lock(m_Cs);

	// Re-validate users!
	m_ValidateStartTime = sysTimer::GetSystemMsTime() + VALIDATE_WAIT_TIME;
	userlistDisplayf("User list reset requested from the following call stack");
	sysStack::PrintStackTrace();
}

void sysDurangoUserList::UpdateUserInfo(u32 userIndex)
{
	if(userlistVerifyf(userIndex < MAX_USERS, "Invalid user index %u!", userIndex))
	{
		userlistDebugf3("Updating user info for user index %u!", userIndex);

		sysCriticalSection lock(m_Cs);

		try
		{
			if(m_LiveUsers[userIndex].GetPlatformGamepad() == nullptr)
			{
				userlistDebugf1("Updating gamepad for user index %u!", userIndex);

				// NOTE: This is slow getting a list of gamepads but the gamepad might not be paired so we need
				// to check all gamepads.
				dns::IVectorView<dns::IGamepad^>^ allGamepads = dns::Gamepad::Gamepads;

				// This is faster than using IVectorView according to Microsoft.
				const u32 numGamepads = allGamepads->Size;
				Platform::Array<dns::IGamepad^>^ gamepads = ref new Platform::Array<dns::IGamepad^>(numGamepads);
				allGamepads->GetMany(0, gamepads);

				for (dns::IGamepad^ gamepad : gamepads)
				{
					const sysDurangoUserInfo* oldPadInfo = GetPlatformUserInfo(gamepad);

					// Only scan gamepads that are not used/paired already.
					if((oldPadInfo == nullptr || oldPadInfo->GetPlatformUser() == nullptr) && DoesGamepadHaveInput(gamepad))
					{
						// If the gamepad is already in used (but not paired).
						if(oldPadInfo != nullptr)
						{
							userlistDebugf1( "Removing Gamepad with id  %" I64FMT " in user index %u as we are moving it to user index %u.",
								gamepad->Id,
								oldPadInfo->m_UserIndex,
								userIndex );

							m_LiveUsers[oldPadInfo->m_UserIndex].SetGamepad(nullptr);
							m_LiveUserListUpdated[oldPadInfo->m_UserIndex] = true;
						}

						userlistDebugf1( "Setting Gamepad with id  %" I64FMT " to user index %u.",
							gamepad->Id,
							userIndex );

						m_LiveUsers[userIndex].SetGamepad(gamepad);
						m_LiveUserListUpdated[userIndex] = true;

						return;
					}
				}
			}
		}
		catch(Platform::Exception^)
		{
			userlistAssertf(false, "Failed to update user info for user %u!", userIndex);
		}
	}
}

void sysDurangoUserList::ResetLists()
{
	for(s32 i = 0; i < MAX_USERS; ++i)
	{
		m_LiveUsers[i].Reset(i);
		m_CurrentUsers[i].Reset(i);

		m_LiveUserListUpdated[i] = true;
	}
}

void sysDurangoUserList::Init()
{
}

void sysDurangoUserList::Shutdown()
{
}

void sysDurangoUserList::Update()
{
	sysCriticalSection lock(m_Cs);
	if(m_ValidateStartTime != 0u)
	{
		if(m_ValidateStartTime <= sysTimer::GetSystemMsTime())
		{
			m_ValidateStartTime = 0u;

			ValidateUsers();
		}
	}
	
	if(m_ValidateStartTime == 0u)
	{
		for(u32 i = 0; i < MAX_USERS; ++i)
		{
			if(m_LiveUserListUpdated[i])
			{
				m_CurrentUsers[i] = m_LiveUsers[i];
				m_LiveUserListUpdated[i] = false;

				userlistDebugf1("List index %u updated, calling delegates.", i);
				CallDelegates(m_CurrentUsers[i]);
			}
		}
	}
}

const sysUserInfo* sysDurangoUserList::GetUserInfo(u32 userIndex) const
{
	return GetPlatformUserInfo(userIndex);
}

const sysUserInfo* sysDurangoUserList::GetUserInfoById(u64 userId) const
{
	return GetPlatformUserInfoById(userId);
}

s32 sysDurangoUserList::GetMaxUsers() const
{
	return MAX_USERS;
}

const sysDurangoUserInfo* sysDurangoUserList::GetPlatformUserInfo(u32 userIndex) const
{
	sysCriticalSection lock(m_Cs);
	if( userlistVerifyf(userIndex < MAX_USERS, "Invalid user index passed in, check calling code!") &&
		(m_CurrentUsers[userIndex].GetPlatformUser() != nullptr || m_CurrentUsers[userIndex].GetPlatformGamepad() != nullptr) )
	{
		return &m_CurrentUsers[userIndex];
	}
	return nullptr;
}

const sysDurangoUserInfo* sysDurangoUserList::GetPlatformUserInfo(Windows::Xbox::System::User^ user) const
{
	// We could have searched all of m_Users for the object with the same User^ but a User^ object becomes 'stale' when the user
	// logs out and a new one is created when they re-login. Doing it this way means we can find the new valid User^ object given
	// an invalid 'stale' object.
	if(user != nullptr)
	{
		try
		{
			return GetPlatformUserInfoBySessionId(user->Id);
		}
		catch(Platform::Exception^)
		{
			userlistAssertf(false, "Failed to get user by user object!");
		}
	}
		
	return nullptr;
}

const sysDurangoUserInfo* sysDurangoUserList::GetPlatformUserInfo( Windows::Xbox::Input::IGamepad^ gamepad ) const
{
	try
	{
		if(gamepad != nullptr)
		{
			sysCriticalSection lock(m_Cs);
			for(s32 i = 0; i < MAX_USERS; ++i)
			{
				if(m_CurrentUsers[i].GetPlatformGamepad() != nullptr && m_CurrentUsers[i].GetPlatformGamepad()->Id == gamepad->Id)
				{
					return &m_CurrentUsers[i];
				}
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Failed to get user by gamepad object!");
	}

	return nullptr;
}

const sysDurangoUserInfo* sysDurangoUserList::GetPlatformUserInfoById(u64 userId) const
{
	try
	{
		sysCriticalSection lock(m_Cs);
		for(s32 i = 0; i < MAX_USERS; ++i)
		{
			if(m_CurrentUsers[i].GetPlatformUser() != nullptr && m_CurrentUsers[i].m_UserId == userId)
			{
				return &m_CurrentUsers[i];
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Failed to get user by user id %" I64FMT "u!", userId);
	}
	
	return nullptr;
}

const sysDurangoUserInfo* sysDurangoUserList::GetPlatformUserInfoBySessionId(u32 sessionId) const
{
	try
	{
		for(const sysDurangoUserInfo& user : m_CurrentUsers)
		{
			if(user.m_User != nullptr && user.m_User->Id == sessionId)
			{
				return &user;
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Failed to get user by user session id %u!", sessionId);
	}

	return nullptr;
}


const sysDurangoUserInfo* sysDurangoUserList::GetSignedInPlatformUserInfo(u32 signedInUserIndex) const
{
	sysCriticalSection lock(m_Cs);
	if(userlistVerifyf((int)signedInUserIndex < m_ConsoleSignedInUsers.GetCount(), "Invalid user index passed in, check calling code!") &&
		(m_ConsoleSignedInUsers[signedInUserIndex].GetPlatformUser() != nullptr))
	{
		return &m_ConsoleSignedInUsers[signedInUserIndex];
	}

	return NULL;
}

const sysDurangoUserInfo* sysDurangoUserList::GetSignedInPlatformUserInfo(u64 userId) const
{
	try
	{
		sysCriticalSection lock(m_Cs);
		for(s32 i = 0; i < m_ConsoleSignedInUsers.GetCount(); ++i)
		{
			if(m_ConsoleSignedInUsers[i].GetPlatformUser() != nullptr && m_ConsoleSignedInUsers[i].m_UserId == userId)
			{
				return &m_ConsoleSignedInUsers[i];
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Failed to get signed in user by user id %" I64FMT "u!", userId);
	}

	return NULL;
}

const sysDurangoUserInfo* sysDurangoUserList::GetSignedInPlatformUserInfo(Platform::String^ userHash) const
{
	try
	{
		sysCriticalSection lock(m_Cs);
		for(s32 i = 0; i < m_ConsoleSignedInUsers.GetCount(); ++i)
		{
			if(m_ConsoleSignedInUsers[i].GetPlatformUser() != nullptr && 
				(wcscmp (m_ConsoleSignedInUsers[i].GetPlatformUser()->XboxUserHash->Data(), userHash->Data()) == 0 ))
			{
				return &m_ConsoleSignedInUsers[i];
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Failed to get signed in user by user hash %ls!", userHash->Data());
	}

	return NULL;
}

u32 sysDurangoUserList::GetNumberOfDetectedUsersInRoom() const
{
	try
	{
		dns::IVectorView<dns::IBodyController^>^ bodyControllers = dns::BodyController::BodyControllers;

		if(bodyControllers != nullptr)
		{
			// This is faster than accessing the IVectorView directly which is very slow according to MS.
			Platform::Array<dns::IBodyController^>^ userControllers = ref new Platform::Array<dns::IBodyController^>(bodyControllers->Size);
			bodyControllers->GetMany(0, userControllers);

			// A known user is a user that is not a guest.
			u32 numKnownUsers = 0;
			for(dns::IBodyController^ userController : userControllers)
			{
				// If userController->User is nullptr then the user is a guest/unknown user so ignore them.
				if(userController != nullptr && userController->User != nullptr)
				{
					++numKnownUsers;
				}
			}

			return numKnownUsers;
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error retrieving Kinect detected user count!");
	}

	return 0;
}

u32 sysDurangoUserList::GetNumberOfUsersLoggedInOnConsole() const
{
	sysCriticalSection lock(m_Cs);
	return m_ConsoleSignedInUsers.GetCount();
}


void sysDurangoUserList::OnGamepadAdded( Windows::Xbox::Input::IGamepad^ gamepad )
{
	try
	{
		sysCriticalSection lock(m_Cs);

		s32 firstFreeSlot = sysUserInfo::INVALID_USER_INDEX;
		bool hasLoggedInUser = false;

		// If the pad is paired to a user find the user.
		Windows::Xbox::System::User^ padUser = gamepad->User;
		for(s32 i = 0; i < MAX_USERS; ++i)
		{
			// We could have compared the User^ pointers directly but this way will work with stale user objects.
			if(padUser != nullptr && m_LiveUsers[i].GetPlatformUser() != nullptr && m_LiveUsers[i].GetPlatformUser()->Id == padUser->Id)
			{
				// Only update the user info if they don't already have a a pad.
				if(m_LiveUsers[i].GetPlatformGamepad() == nullptr)
				{
					m_LiveUsers[i].SetGamepad(gamepad);
					// Also update the user in case it is stale.
					m_LiveUsers[i].SetUser(padUser);
					m_LiveUserListUpdated[i] = true;
					userlistDebugf1( "Updated Gamepad %u paired with User %" I64FMT "u, index %d",
						gamepad->Id,
						padUser->Id,
						i );
				}

				return;
			}

			if(m_LiveUsers[i].GetPlatformUser() != nullptr)
			{
				hasLoggedInUser = true;
			}			
			else if(firstFreeSlot == sysUserInfo::INVALID_USER_INDEX && m_LiveUsers[i].GetPlatformGamepad() == nullptr)
			{
				firstFreeSlot = i;
			}
		}

		// Don't add pads once we have a logged in user, instead add then with pairing events. We don't add the pads because we could
		// end up adding a pad that is later paired with the logged in user. This would mean a pad changes index.
		if(hasLoggedInUser == false && userlistVerifyf(firstFreeSlot != sysUserInfo::INVALID_USER_INDEX, "No free gamepad slots to add gamepad!"))
		{
			m_LiveUsers[firstFreeSlot].SetGamepad(gamepad);
			// Also update the user in case it is stale.
			m_LiveUsers[firstFreeSlot].SetUser(padUser);
			m_LiveUserListUpdated[firstFreeSlot] = true;
			userlistDebugf1( "Added: Gamepad %u, User %" I64FMT "u, index %d",
				gamepad->Id,
				(padUser ? padUser->Id : 0),
				firstFreeSlot );
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error adding gamepad!");
	}
}

void sysDurangoUserList::OnGamepadRemoved( Windows::Xbox::Input::IGamepad^ gamepad )
{
	if(gamepad != nullptr)
	{
		try
		{
			sysCriticalSection lock(m_Cs);

			// Find a free space for the gamepad that is not being used!
			for(sysDurangoUserInfo& userInfo : m_LiveUsers)
			{
				if(userInfo.GetPlatformGamepad() != nullptr && userInfo.GetPlatformGamepad()->Id == gamepad->Id)
				{
					userInfo.SetGamepad(nullptr);
					m_LiveUserListUpdated[userInfo.m_UserIndex] = true;
					userlistDebugf1( "Removed: Gamepad %u, index %d, user %" I64FMT "u",
										gamepad->Id,
										userInfo.m_UserIndex,
										userInfo.m_UserId );

					break;
				}
			}
		}
		catch(Platform::Exception^)
		{
			userlistAssertf(false, "Error removing gamepad!");
		}
	}
}

void sysDurangoUserList::OnGamepadPairingChanged( Windows::Xbox::System::User^ user, Windows::Xbox::Input::IGamepad^ gamepad )
{
	try
	{
		// NOTE: We are only interested in a player being paired when there is no user object or no pad!
		if(gamepad != nullptr)
		{
			sysCriticalSection lock(m_Cs);
			sysDurangoUserInfo* newPairingInfo = nullptr;

			for(sysDurangoUserInfo& userInfo : m_LiveUsers)
			{
				// If there is a pad then check to see if it is the pad we are looking for.
				if(userInfo.GetPlatformGamepad() != nullptr)
				{
					// If this is the pad we are looking for then stop.
					if(userInfo.GetPlatformGamepad()->Id == gamepad->Id)
					{
						// Only update if there is no user.
						if(userInfo.GetPlatformUser() == nullptr)
						{
							userInfo.SetUser(user);
							m_LiveUserListUpdated[userInfo.m_UserIndex] = true;
							userlistDebugf1( "Pairing to gamepad: Gamepad %u paired to user %" I64FMT "u, index %d",
								gamepad->Id,
								userInfo.m_UserId,
								userInfo.m_UserIndex );
						}

						return;
					}
				}
				// If there is no pad but there is a user and it is the user we are pairing to then update.
				else if(user != nullptr && userInfo.GetPlatformUser() != nullptr && user->Id == userInfo.GetPlatformUser()->Id)
				{
					userInfo.SetUser(user);
					userInfo.SetGamepad(gamepad);
					m_LiveUserListUpdated[userInfo.m_UserIndex] = true;
					userlistDebugf1( "Pairing to user: Gamepad %u paired to user %" I64FMT "u, index %d",
						gamepad->Id,
						userInfo.m_UserId,
						userInfo.m_UserIndex );

					return;
				}
				// if both user and gamepad are null then save the position as a possible new pairing.
				else if(newPairingInfo == nullptr)
				{
					newPairingInfo = &userInfo;
				}
			}

			// Add new user and pad but only if the user is not already in our list (this means they are using another gamepad).
			if(newPairingInfo != nullptr && GetPlatformUserInfo(gamepad->User) == nullptr)
			{
				newPairingInfo->SetUser(user);
				newPairingInfo->SetGamepad(gamepad);
				m_LiveUserListUpdated[newPairingInfo->m_UserIndex] = true;
				userlistDebugf1( "New pairing: Gamepad %u paired to user %" I64FMT "u, index %d",
					gamepad->Id,
					newPairingInfo->m_UserId,
					newPairingInfo->m_UserIndex );
			}
		}
	}
	catch(Platform::Exception^)
	{
		userlistAssertf(false, "Error pairing gamepad with user.");
	}
}

void sysDurangoUserList::OnUserSignedOut( Windows::Xbox::System::User^ user )
{
	if(user != nullptr)
	{
		try
		{
			sysCriticalSection lock(m_Cs);

#if !__NO_OUTPUT
			auto users = Windows::Xbox::System::User::Users;
			userlistDebugf1("OnUserSignedOut : user: %ls, user is %s, user list size is %u", user->XboxUserId->Data(), user->IsSignedIn ? "SIGNED IN" : "SIGNED OUT", users->Size);
			userlistAssertf(!user->IsSignedIn, "OnUserSignedOut : signed out user is still signed in.");

			for (unsigned i = 0; i < users->Size; ++i )
			{
				Windows::Xbox::System::User^ pUser = users->GetAt( i );
				userlistDebugf1("OnUserSignedOut : user at index %u is : %ls", i, pUser->XboxUserId->Data());
				userlistAssertf(wcscmp(pUser->XboxUserId->Data(), user->XboxUserId->Data()) != 0, "OnUserSignedOut : signed out user still exists in User::Users list, Please attach logs to B* 2007406. Thanks!");
			}
#endif

			for(sysDurangoUserInfo& userInfo : m_LiveUsers)
			{
				if(userInfo.GetPlatformUser() == user)
				{
					userlistDebugf1( "Sign-Out: Gamepad %u, index %d, user %" I64FMT "u", 
						(userInfo.GetPlatformGamepad() != nullptr) ? userInfo.GetPlatformGamepad()->Id : 0,
						userInfo.m_UserIndex,
						userInfo.m_UserId );
					userInfo.SetUser(nullptr);
					m_LiveUserListUpdated[userInfo.m_UserIndex] = true;

					break;
				}
			}

			// maintain our list of signed in users
			u64 xuid;
			if (userlistVerify(swscanf_s(user->XboxUserId->Data(), L"%" LI64FMT L"u", &xuid) == 1))
			{
				int count = m_ConsoleSignedInUsers.GetCount();
				for (int i = count-1; i >= 0 ; i--)
				{
					// Parse u64 and check to see if it matches our current index
					if (xuid == m_ConsoleSignedInUsers[i].GetUserId())
					{
						userlistDebugf1("Removing user: %" I64FMT "u", m_ConsoleSignedInUsers[i].GetUserId());
						m_ConsoleSignedInUsers.Delete(i);
						break;
					}
				}
			}
		}
		catch(Platform::Exception^)
		{
			userlistAssertf(false, "Error signing user out!");
		}
	}
}

void sysDurangoUserList::OnUserSignedIn(Windows::Xbox::System::User^ user)
{
	try
	{
		sysDurangoUserInfo userInfo;
		userInfo.SetUser(user);
		userInfo.SetGamepad(nullptr);

		u64 xuid;
		if (userlistVerify(swscanf_s(user->XboxUserId->Data(), L"%" LI64FMT L"u", &xuid) == 1))
		{
			for (int i = 0; i < m_ConsoleSignedInUsers.GetCount(); i++)
			{
				// Xuid must parse successfully, and check if user is already registered on console (this would indicate an error)
				if (!userlistVerify(xuid != m_ConsoleSignedInUsers[i].GetUserId()))
				{
					return;
				}
			}

			userlistDebugf1("Added user: %" I64FMT "u", userInfo.GetUserId());
			m_ConsoleSignedInUsers.PushAndGrow(userInfo);
		}
	}
	catch (Platform::Exception^ ex)
	{
		userlistAssertf(false, "Error adding user!");
	}
}

void sysDurangoUserList::OnServiceEvent(sysServiceEvent* evnt)
{
	if(evnt != nullptr && (evnt->GetType() == sysServiceEvent::RESUMING))
	{
		Reset();
	}
}

void sysDurangoUserList::ValidateUsers()
{
	// Copy the live list so we can try and place users in the same order/player index.
	sysDurangoUserInfo oldLiveList[MAX_USERS];
	for(u32 i = 0; i < MAX_USERS; ++i)
	{
		oldLiveList[i] = m_LiveUsers[i];
	}

	// CreateLiveUsersList will trash our live list and re-build it.
	CreateLiveUsersList();

	// However, we want users to remain in their previous positions/user indexes so we sort them to
	// be the same order as the current list.
	userlistDebugf1("Validating live user list indexes (player numbers).");
	for(u32 oldIndex = 0; oldIndex < MAX_USERS; ++oldIndex)
	{
		// If the current user list had a user, find it in the live list.
		if(oldLiveList[oldIndex].GetPlatformUser() != nullptr)
		{
			userlistDebugf1( "User %" I64FMT "u was set as index %u before reset. Looking for user index after reset.",
				oldLiveList[oldIndex].m_UserId,
				oldIndex );

			OUTPUT_ONLY(bool found = false);

			for(u32 liveIndex = 0; liveIndex < MAX_USERS; ++liveIndex)
			{
				if( m_LiveUsers[liveIndex].GetPlatformUser() != nullptr &&
					m_LiveUsers[liveIndex].m_UserId == oldLiveList[oldIndex].m_UserId )
				{
					userlistDebugf1( "User %" I64FMT "u new index %u after reset.",
						m_LiveUsers[liveIndex].m_UserId,
						liveIndex );

					if(oldIndex != liveIndex)
					{
						userlistDebugf1( "Swapping current User %" I64FMT "u index %u with User %" I64FMT "u index %u.",
							m_LiveUsers[liveIndex].m_UserId,
							liveIndex,
							m_LiveUsers[oldIndex].m_UserId,
							oldIndex );

						SwapLiveUsers(oldIndex, liveIndex);
					}

					OUTPUT_ONLY(found = true);

					break;
				}
			}

// There appears to no OUTPUT define.
#if !__NO_OUTPUT
			if(found == false)
			{
				userlistDebugf1("User %" I64FMT "u is no longer logged in.", m_CurrentUsers[oldIndex].m_UserId);
			}
#endif // !__NO_OUTPUT
		}
		else
		{
			userlistDebugf1("There was no user in index %u before reset.", oldIndex);
		}
	}
}

void sysDurangoUserList::SwapLiveUsers(u32 indexA, u32 indexB)
{
	if(userlistVerifyf(indexA < MAX_USERS && indexB < MAX_USERS, "Invalid index!"))
	{
		sysDurangoUserInfo swapUser	= m_LiveUsers[indexA];
		m_LiveUsers[indexA] = m_LiveUsers[indexB];
		m_LiveUsers[indexB] = swapUser;

		// Ensure that the indexes are put back.
		m_LiveUsers[indexB].m_UserIndex = indexB;
		m_LiveUsers[indexA].m_UserIndex = indexA;

		m_LiveUserListUpdated[indexB] = true;
		m_LiveUserListUpdated[indexA] = true;
	}
}

bool sysDurangoUserList::DoesGamepadHaveInput(dns::IGamepad^ gamepad)
{
	dns::RawGamepadReading reading = gamepad->GetRawCurrentReading();
	return (reading.Buttons & dns::GamepadButtons::A) == dns::GamepadButtons::A ||
		   (reading.Buttons & dns::GamepadButtons::B) == dns::GamepadButtons::B ||
		   (reading.Buttons & dns::GamepadButtons::X) == dns::GamepadButtons::X ||
		   (reading.Buttons & dns::GamepadButtons::Y) == dns::GamepadButtons::Y ||
		   (reading.Buttons & dns::GamepadButtons::Menu) == dns::GamepadButtons::Menu ||
		   (reading.Buttons & dns::GamepadButtons::View) == dns::GamepadButtons::View;
}

void sysDurangoUserList::SetUserInfo(s32 userIndex, Windows::Xbox::System::User^ user, Windows::Xbox::Input::IGamepad^ gamepad, bool moveCurrentUserToFreeSlot)
{
	sysCriticalSection lock(m_Cs);

	userlistDisplayf("User list manually set user info in index (%d) with callstack:", userIndex);
	sysStack::PrintStackTrace();

	if(userlistVerifyf(userIndex >= 0 && userIndex < MAX_USERS, "Invalid user index (%d)!", userIndex))
	{
		userlistDebugf1("Requested move to user info to free slot.");
		ASSERT_ONLY(bool moved = false);

		if(moveCurrentUserToFreeSlot)
		{
			// Find next empty slot in the live list.
			for(u32 liveIndex = 0; liveIndex < MAX_USERS; ++liveIndex)
			{
				// The first null user is an empty slot, even if it has a gamepad.
				if(m_LiveUsers[liveIndex].m_User == nullptr)
				{
					userlistDebugf1("Moving user in index (%d).", userIndex);
					SwapLiveUsers(userIndex, liveIndex);
					ASSERT_ONLY(moved = true);

					break;
				}
			}

			userlistAssertf(moved, "Failed to move user from index %d as there are no free slots!", userIndex);
		}

		// Remove the user from this list if they are in another slot.
		if(user != nullptr)
		{
			for(u32 liveIndex = 0; liveIndex < MAX_USERS; ++liveIndex)
			{
				if(m_LiveUsers[liveIndex].GetPlatformUser() != nullptr && m_LiveUsers[liveIndex].GetPlatformUser()->Id == user->Id)
				{
					userlistDebugf1( "Requested user  %" I64FMT "u is already logged in index %d, removing them!",
									  m_LiveUsers[liveIndex].m_UserId,
									  liveIndex );

					m_LiveUsers[liveIndex].SetUser(nullptr);
					m_LiveUserListUpdated[liveIndex] = true;

					break;
				}
			}
		}

		// Remove the gamepad from this list if they are in another slot.
		if(gamepad != nullptr)
		{
			for(u32 liveIndex = 0; liveIndex < MAX_USERS; ++liveIndex)
			{
				if(m_LiveUsers[liveIndex].GetPlatformGamepad() != nullptr && m_LiveUsers[liveIndex].GetPlatformGamepad()->Id == gamepad->Id)
				{
					userlistDebugf1( "Requested gamepad  %" I64FMT "u is already used in index %d, removing it!",
									  gamepad->Id,
									  liveIndex );

					m_LiveUsers[liveIndex].SetGamepad(nullptr);
					m_LiveUserListUpdated[liveIndex] = true;

					break;
				}
			}
		}

		m_LiveUsers[userIndex].SetUser(user);
		m_LiveUsers[userIndex].SetGamepad(gamepad);
		m_LiveUserListUpdated[userIndex] = true;
		
		userlistDebugf1( "Setting gamepad  %" I64FMT "u and user %" I64FMT "u to index %d!",
						 (gamepad) ? gamepad->Id : 0u,
						 m_LiveUsers[userIndex].m_UserId,
						 userIndex );

	}
}

bool sysDurangoUserList::IsWaitingForLiveUserListUpdate()
{ 
	sysCriticalSection lock(m_Cs);
	for (int i = 0; i < MAX_USERS; i++)
	{
		if (m_LiveUserListUpdated[i]) 
		{
			return true;
		}
	}

	return false;
}

}

#endif // RSG_DURANGO
