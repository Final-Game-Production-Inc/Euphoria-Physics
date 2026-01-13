// 
// system/epiccommon.h
// 
// Copyright (C) 1999-2019 Rockstar Games.  All Rights Reserved. 
// 

#ifndef EPIC_COMMON_H 
#define EPIC_COMMON_H 

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

#include "diag/channel.h"
#include "diag/seh.h"
#include "system/xtl.h"

// Epic SDK includes
#include "../../3rdparty/Epic/Include/eos_auth_types.h"
#include "../../3rdparty/Epic/Include/eos_common.h"
#include "../../3rdparty/Epic/Include/eos_init.h"
#include "../../3rdparty/Epic/Include/eos_logging.h"
#include "../../3rdparty/Epic/Include/eos_types.h"
#include "../../3rdparty/Epic/Include/eos_userinfo_types.h"
#include "../../3rdparty/Epic/Include/eos_version.h"

namespace rage
{
RAGE_DECLARE_CHANNEL(epic);

#define epicDebug(fmt, ...)						RAGE_DEBUGF1(__rage_channel, fmt, ##__VA_ARGS__)
#define epicDebug1(fmt, ...)					RAGE_DEBUGF1(__rage_channel, fmt, ##__VA_ARGS__)
#define epicDebug2(fmt, ...)					RAGE_DEBUGF2(__rage_channel, fmt, ##__VA_ARGS__)
#define epicDebug3(fmt, ...)					RAGE_DEBUGF3(__rage_channel, fmt, ##__VA_ARGS__)
#define epicWarning(fmt, ...)					RAGE_WARNINGF(__rage_channel, fmt, ##__VA_ARGS__)
#define epicError(fmt, ...)						RAGE_ERRORF(__rage_channel, fmt, ##__VA_ARGS__)
#define epicCondLogf(cond, severity, fmt, ...)	RAGE_CONDLOGF(__rage_channel, cond, severity, fmt, ##__VA_ARGS__)
#define epicVerify(cond)						RAGE_VERIFY(__rage_channel, cond)
#define epicVerifyf(cond, fmt, ...)				RAGE_VERIFYF(__rage_channel, cond, fmt, ##__VA_ARGS__)
#define epicAssert(cond) 						RAGE_ASSERT(__rage_channel, cond)
#define epicAssertf(cond, fmt, ...) 			RAGE_ASSERTF(__rage_channel, cond, fmt, ##__VA_ARGS__)

OUTPUT_ONLY(const char* EOSResultToString(const EOS_EResult Result));
OUTPUT_ONLY(const char* EOSLoginStatusToString(const EOS_ELoginStatus status));

// wrap the exported Epic API functions
class sysEpicApi
{
	friend class sysEpic;
	friend class sysEpicAuthentication;

private:
	sysEpicApi();
	~sysEpicApi();
	bool Init(HMODULE hEpicDll);
	void Clear();
	void Shutdown();

	// Ensure all sysEpic functionality below has not changed when Epic SDK is updated.
	// In the past they have changed the signature of functions which GetProcAddress does not detect.
	CompileTimeAssert(EOS_MAJOR_VERSION == 1);
	CompileTimeAssert(EOS_MINOR_VERSION == 2);
	CompileTimeAssert(EOS_PATCH_VERSION == 0);

#define SYS_EPIC_DECL_FUNC(T, funcName, ...) \
	typedef T (*funcName##Ptr)(__VA_ARGS__); \
	funcName##Ptr funcName;

	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_Initialize, EOS_InitializeOptions* initOptions);
	SYS_EPIC_DECL_FUNC(EOS_HPlatform, EOS_Platform_Create, EOS_Platform_Options* platformOptions);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_EpicAccountId_ToString, EOS_EpicAccountId AccountId, char* OutBuffer, int32_t* InOutBufferLength);
	SYS_EPIC_DECL_FUNC(EOS_EpicAccountId, EOS_EpicAccountId_FromString, const char* AccountIdString);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_Shutdown);
	SYS_EPIC_DECL_FUNC(void, EOS_Platform_Release, EOS_HPlatform Handle);
	SYS_EPIC_DECL_FUNC(void, EOS_Auth_Token_Release, EOS_Auth_Token* AuthToken);
	SYS_EPIC_DECL_FUNC(void, EOS_UserInfo_Release, EOS_UserInfo* UserInfo);
	SYS_EPIC_DECL_FUNC(void, EOS_Platform_Tick, EOS_HPlatform Handle);
	SYS_EPIC_DECL_FUNC(EOS_HAuth, EOS_Platform_GetAuthInterface, EOS_HPlatform Handle);
	SYS_EPIC_DECL_FUNC(EOS_HUserInfo, EOS_Platform_GetUserInfoInterface, EOS_HPlatform Handle);
	SYS_EPIC_DECL_FUNC(void, EOS_Auth_Login, EOS_HAuth Handle, const EOS_Auth_LoginOptions* Options, void* ClientData, const EOS_Auth_OnLoginCallback CompletionDelegate);
	SYS_EPIC_DECL_FUNC(void, EOS_Auth_Logout, EOS_HAuth Handle, const EOS_Auth_LogoutOptions* Options, void* ClientData, const EOS_Auth_OnLogoutCallback CompletionDelegate);
	SYS_EPIC_DECL_FUNC(int32_t, EOS_Auth_GetLoggedInAccountsCount, EOS_HAuth Handle);
	SYS_EPIC_DECL_FUNC(EOS_EpicAccountId, EOS_Auth_GetLoggedInAccountByIndex, EOS_HAuth Handle, int32_t Index);
	SYS_EPIC_DECL_FUNC(EOS_ELoginStatus, EOS_Auth_GetLoginStatus, EOS_HAuth Handle, EOS_EpicAccountId LocalUserId);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_Auth_CopyUserAuthToken, EOS_HAuth Handle, const EOS_Auth_CopyUserAuthTokenOptions* Options, EOS_EpicAccountId LocalUserId, EOS_Auth_Token ** OutUserAuthToken);
	SYS_EPIC_DECL_FUNC(EOS_NotificationId, EOS_Auth_AddNotifyLoginStatusChanged, EOS_HAuth Handle, const EOS_Auth_AddNotifyLoginStatusChangedOptions* Options, void* ClientData, const EOS_Auth_OnLoginStatusChangedCallback Notification);
	SYS_EPIC_DECL_FUNC(void, EOS_Auth_RemoveNotifyLoginStatusChanged, EOS_HAuth Handle, EOS_NotificationId InId);
	SYS_EPIC_DECL_FUNC(void, EOS_UserInfo_QueryUserInfo, EOS_HUserInfo Handle, const EOS_UserInfo_QueryUserInfoOptions* Options, void* ClientData, const EOS_UserInfo_OnQueryUserInfoCallback CompletionDelegate);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_UserInfo_CopyUserInfo, EOS_HUserInfo Handle, const EOS_UserInfo_CopyUserInfoOptions* Options, EOS_UserInfo ** OutUserInfo);

#if !__NO_OUTPUT
	SYS_EPIC_DECL_FUNC(const char*, EOS_EResult_ToString, EOS_EResult result);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_Logging_SetCallback, EOS_LogMessageFunc callback);
	SYS_EPIC_DECL_FUNC(EOS_EResult, EOS_Logging_SetLogLevel, EOS_ELogCategory logCategory, EOS_ELogLevel logLevel);
#endif

#undef SYS_EPIC_DECL_FUNC
};

} // namespace rage

#endif // EPIC_API_SUPPORTED
#endif // EPIC_COMMON_H 
