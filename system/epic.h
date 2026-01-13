// 
// system/epic.h
// 
// Copyright (C) 1999-2019 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_EPIC_H
#define SYSTEM_EPIC_H

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

#include "atl/string.h"
#include "epicauthentication.h"
#include "epiccommon.h"
#include "epicevents.h"
#include "system/xtl.h"

namespace rage
{

// PURPOSE
//	Interface to services on the Epic platform.
class sysEpic
{
public:

	// PURPOSE:
	//	Constructor and Destructor
	sysEpic();
	~sysEpic();

	// PURPOSE
	//	The Epic API should be initialized once on startup, updated once per frame, and shutdown.
	bool InitClass(const char* epicProductName,
				   const char* epicProductVersion,
				   const char* epicProductId,
				   const char* epicSandboxId,
				   const char* epicClientId,
				   const char* epicClientSecret,
				   const char* epicDeploymentId);

	void UpdateClass();
	void ShutdownClass();

	// PURPOSE
	//	Returns true if the Epic API is initialized (i.e. we're in Epic mode, the dll has loaded
	//	and the Epic API initialized successfully).
	bool IsInitialized() { return m_Initialized; }

	//PURPOSE
	//  Add/remove a delegate that will be called with event notifications.
	void AddDelegate(sysEpicEventDelegate* dlgt);
	void RemoveDelegate(sysEpicEventDelegate* dlgt);
	void DispatchEvent(const sysEpicEvent* e);

	// PURPOSE
	//	Initiates a refresh of the user's Epic access token. This is an async operation.
	//  An EPIC_EVENT_ACCESS_TOKEN_CHANGED event will be raised when the new token is available.
	//  Call GetAccessToken() after the event is received to obtain the new token.
	//  If refreshing the token fails, the event will still be raised.
	void RefreshAccessTokenAsync();

	// PURPOSE
	//	Accessors for Epic properties
	const char* GetAccessToken() const;
	const char* GetPlayerName() const;
	const char* GetAccountId() const;
	const char* GetProductId() const;
	const char* GetSandboxId() const;
	const char* GetDeploymentId() const;
	bool IsOfflineMode() const;

private:

	bool LoadEpicLib();

	OUTPUT_ONLY(static void EpicSdkOutputCallback(const EOS_LogMessage* msg));
	
	sysEpicApi m_EpicApi;

	sysEpicEventDelegator m_Delegator;

	EOS_HPlatform m_EpicPlatform;
	sysEpicAuthentication m_Auth;

	atString m_ProductId;
	atString m_SandboxId;
	atString m_DeploymentId;

	HMODULE m_hEpicDll;
	bool m_Initialized;
};

// global instance
extern sysEpic g_SysEpic;

} // namespace rage

#endif // EPIC_API_SUPPORTED
#endif // SYSTEM_EPIC_H
