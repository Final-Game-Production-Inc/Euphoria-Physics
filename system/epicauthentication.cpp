// 
// system/epicauthentication.cpp
// 
// Copyright (C) 1999-2019 Rockstar Games.  All Rights Reserved. 
// 

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

#include "data/base64.h"
#include "diag/seh.h"
#include "string/string.h"
#include "string/stringutil.h"
#include "system/param.h"

// Epic wrapper includes
#include "epic.h"
#include "epicauthentication.h"

// Epic SDK includes
#include "../../3rdparty/Epic/Include/eos_userinfo.h"
#include "../../3rdparty/Epic/Include/eos_sdk.h"

namespace rage
{
	
NOSTRIP_PC_PARAM(AUTH_PASSWORD, "The Epic Games Launcher passes the exchange code via the -AUTH_PASSWORD=<exchangeCode> command line.");
NOSTRIP_PC_PARAM(AUTH_TYPE, "The Epic Games Launcher indicates usage of the exchange code via the -AUTH_TYPE=exchangeCode command line.");
NOSTRIP_PC_PARAM(epicusername, "The Epic Games Launcher passes in the user's epic user name. Used in offline mode only.");
NOSTRIP_PC_PARAM(epicuserid, "The Epic Games Launcher passes in the user's epic user id. Used in offline mode only.");

PARAM(EpicDevToolHost, "Specifies the hostname and port of the Epic dev auth tool. E.g. 127.0.0.1:22222");
PARAM(EpicDevToolTokenName, "Specifies the name of the token configured in the Epic dev auth tool.");

RAGE_DEFINE_SUBCHANNEL(epic, auth)
#undef __rage_channel
#define __rage_channel epic_auth

extern sysEpic g_SysEpic;

// We need a future Epic SDK to support Epic token exchange from the RGL,
// as the RGL will consume the original exchange code passed by EGL.
// We must rely on ticket transfer using the RGL's signed in profile.
#define USE_EPIC_SDK_AUTH (!RSG_LAUNCHER_CHECK && 0)

sysEpicAuthentication::sysEpicAuthentication(sysEpicApi& epicApi)
: m_EpicApi(epicApi)
, m_EpicPlatform(nullptr)
, m_AuthHandle(nullptr)
, m_NotificationId(EOS_INVALID_NOTIFICATIONID)
, m_AuthToken(nullptr)
, m_EosAccountId(nullptr)
, m_PlayerName(nullptr)
, m_OfflineMode(false)
{
	sysMemSet(m_AccountId, 0, sizeof(m_AccountId));
}

sysEpicAuthentication::~sysEpicAuthentication()
{

}

bool sysEpicAuthentication::Init(EOS_HPlatform hEpicPlatform)
{
	rtry
	{
		rverifyall(hEpicPlatform);

		m_EpicPlatform = hEpicPlatform;
	
		EOS_HAuth authHandle = m_EpicApi.EOS_Platform_GetAuthInterface(m_EpicPlatform);
		rverifyall(authHandle);

		m_AuthHandle = authHandle;

		EOS_Auth_AddNotifyLoginStatusChangedOptions options = {0};
		options.ApiVersion = EOS_AUTH_CREATEDEVICEAUTH_API_LATEST;

		m_NotificationId = m_EpicApi.EOS_Auth_AddNotifyLoginStatusChanged(m_AuthHandle, &options, this, LoginStatusChangedCallbackFn);

		// If the Epic Launcher provided no exchange code, we force offline only mode.
		if (IsAuthTypeExchangeCode())
		{
			const char* exchangeCode = nullptr;
			PARAM_AUTH_PASSWORD.Get(exchangeCode);

			// Until Epic SDK supports an explicit offline mode check,
			// a missing exchange code indicates offline mode.
			m_OfflineMode = StringNullOrEmpty(exchangeCode);
		}

		// Attempt to login if we're using Epic authentication and not in offline mode.
		if (USE_EPIC_SDK_AUTH && !m_OfflineMode)
		{
			return LoginAsync();
		}
		else
		{
			// We're not authenticating with Epic, so read what info we can from command line.
			ReadEpicUserInfoFromCommandLine();

			// Convert the account ID from the command line to an EOS account id
			if (!StringNullOrEmpty(m_AccountId))
			{
				m_EosAccountId = m_EpicApi.EOS_EpicAccountId_FromString(m_AccountId);
			}
			
			return true;
		}
	}
	rcatchall
	{
		return false;
	}
}

void sysEpicAuthentication::Shutdown()
{
	epicDebug("sysEpicAuthentication::Shutdown");

	m_EpicApi.EOS_Auth_RemoveNotifyLoginStatusChanged(m_AuthHandle, m_NotificationId);

	ClearAccountInfo();

	m_EpicPlatform = nullptr;
	m_AuthHandle = nullptr;
	m_NotificationId = EOS_INVALID_NOTIFICATIONID;
}

void sysEpicAuthentication::Update()
{

}

bool sysEpicAuthentication::IsOfflineMode() const
{
	return m_OfflineMode;
}

void sysEpicAuthentication::RefreshAccessTokenAsync()
{
	// Note: this isn't currently async, but we expose the interface as if it were async 
	// in case we need to perform async ops in the future.

	EOS_EResult authTokenResult = EOS_EResult::EOS_InvalidParameters;

	rtry
	{
		EOS_Auth_Token* token;
		EOS_Auth_CopyUserAuthTokenOptions options = {0};
		options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
		authTokenResult = m_EpicApi.EOS_Auth_CopyUserAuthToken(m_AuthHandle, &options, m_EosAccountId, &token);
		rverify(authTokenResult == EOS_EResult::EOS_Success,
				catchall,
				epicError("RefreshAccessTokenAsync :: EOS_Auth_CopyUserAuthToken failed - Error Code: %s", m_EpicApi.EOS_EResult_ToString(authTokenResult)));

		// refresh succeeded, release the old one.
		if(m_AuthToken)
		{
			m_EpicApi.EOS_Auth_Token_Release(m_AuthToken);
			m_AuthToken = nullptr;
		}

		m_AuthToken = token;

#if !__NO_OUTPUT
		PrintAuthToken(m_AuthToken);
#endif
	}
	rcatchall
	{

	}

	// raise the event even on failure, since callers are expecting a response.
	sysEpicEventAccessTokenChanged e(authTokenResult == EOS_EResult::EOS_Success, (int)authTokenResult);
	g_SysEpic.DispatchEvent(&e);
}

const char* sysEpicAuthentication::GetAccessToken() const
{
	return m_AuthToken ? m_AuthToken->AccessToken : "";
}

void sysEpicAuthentication::SetPlayerName(const char* playerName)
{
	if (m_PlayerName)
	{
		StringFree(m_PlayerName);
		m_PlayerName = nullptr;
	}

	m_PlayerName = StringDuplicate(playerName);
}

const char* sysEpicAuthentication::GetPlayerName() const
{
	return m_PlayerName ? m_PlayerName : "";
}

const char* sysEpicAuthentication::GetAccountId() const
{
	return m_AccountId;
}

void sysEpicAuthentication::ClearAccountInfo()
{
	if(m_PlayerName)
	{
		StringFree(m_PlayerName);
		m_PlayerName = nullptr;
	}

	if(m_AuthToken)
	{
		m_EpicApi.EOS_Auth_Token_Release(m_AuthToken);
		m_AuthToken = nullptr;
	}

	sysMemSet(m_AccountId, 0, sizeof(m_AccountId));

	m_EosAccountId = nullptr;
}

void sysEpicAuthentication::ReadEpicUserInfoFromCommandLine()
{
	const char* epicUserName;
	if (PARAM_epicusername.Get(epicUserName))
	{
		epicDebug("Epic user name from command line: %s", epicUserName);
		SetPlayerName(epicUserName);
	}

	const char* epicUserId;
	if (PARAM_epicuserid.Get(epicUserId))
	{
		epicDebug("Epic user id from command line: %s", epicUserId);
		safecpy(m_AccountId, epicUserId);
	}
}

void sysEpicAuthentication::DispatchSignInChangedEvent(const sysEpicSignInStateFlags flags)
{
	sysEpicEventSignInStateChanged e(flags);
	g_SysEpic.DispatchEvent(&e);
}

void sysEpicAuthentication::LoginCompleteCallbackFn(const EOS_Auth_LoginCallbackInfo* data)
{
	if(data == nullptr)
	{
		epicError("EpicAuthenticationLoginCompleteCallbackFn :: data is nullptr");
		return;
	}

	sysEpicAuthentication* auth = (sysEpicAuthentication*)data->ClientData;
	if(auth == nullptr)
	{
		epicError("EpicAuthenticationLoginCompleteCallbackFn :: data->ClientData is nullptr");
		return;
	}

	auth->LoginCompleteCallback(data);
}

void sysEpicAuthentication::LogoutCompleteCallbackFn(const EOS_Auth_LogoutCallbackInfo* data)
{
	if(data == nullptr)
	{
		epicError("LogoutCompleteCallbackFn :: data is nullptr");
		return;
	}

	sysEpicAuthentication* auth = (sysEpicAuthentication*)data->ClientData;
	if(auth == nullptr)
	{
		epicError("LogoutCompleteCallbackFn :: data->ClientData is nullptr");
		return;
	}

	auth->LogoutCompleteCallback(data);
}

void sysEpicAuthentication::LoginStatusChangedCallbackFn(const EOS_Auth_LoginStatusChangedCallbackInfo* data)
{
	if(data == nullptr)
	{
		epicError("LoginStatusChangedCallbackFn :: data is nullptr");
		return;
	}

	sysEpicAuthentication* auth = (sysEpicAuthentication*)data->ClientData;
	if(auth == nullptr)
	{
		epicError("LoginStatusChangedCallbackFn :: data->ClientData is nullptr");
		return;
	}

	auth->LoginStatusChangedCallback(data);
}

#if !__NO_OUTPUT
void sysEpicAuthentication::PrintAuthToken(EOS_Auth_Token* InAuthToken)
{
	char accountId[EOS_EPICACCOUNTID_MAX_LENGTH + 1] = {0};
	int32_t accountIdSize = sizeof(accountId);
	m_EpicApi.EOS_EpicAccountId_ToString(InAuthToken->AccountId, accountId, &accountIdSize);

	epicDebug("AuthToken");
	epicDebug(" - App: %s", InAuthToken->App);
	epicDebug(" - ClientId: %s", InAuthToken->ClientId);
	epicDebug(" - AccountId: %s", accountId);

	epicDebug(" - AccessToken: %s", InAuthToken->AccessToken);
	epicDebug(" - AccessTokenLen: %u", (unsigned)strlen(InAuthToken->AccessToken));
	epicDebug(" - ExpiresIn: %0.2f", InAuthToken->ExpiresIn);
	epicDebug(" - ExpiresAt: %s", InAuthToken->ExpiresAt);
	
	epicDebug(" - Epic JWT (decoded access token):");

	// make a copy, strtok modifies the string
	const unsigned tokenLen = (unsigned)strlen(InAuthToken->AccessToken);
	const unsigned prembleLen = (unsigned)strlen("eg1~");
	if(tokenLen <= prembleLen)
	{
		return;
	}

	char* encodedToken = (char*)Alloca(char, tokenLen + 1);
	if(!encodedToken)
	{
		return;
	}

	safecpy(encodedToken, InAuthToken->AccessToken, tokenLen + 1);

	// skip the Epic preamble
	encodedToken += prembleLen;

	// JWT's have three sections: header, payload, signature.
	enum JwtSection
	{
		JWT_SECTION_HEADER = 0,
		JWT_SECTION_PAYLOAD = 1,
		JWT_SECTION_SIGNATURE = 2,
	};

	JwtSection section = JWT_SECTION_HEADER;

	char *encodedSection = strtok((char*)encodedToken, ".");
	while((encodedSection != nullptr) && (section < JWT_SECTION_SIGNATURE))
	{
		const unsigned decodedSectionMaxLen = datBase64::GetMaxDecodedSize(encodedSection);
		char* decodedSection = (char*)Alloca(char, decodedSectionMaxLen + 1);
	
		if(!decodedSection)
		{
			return;
		}
		sysMemSet(decodedSection, 0, decodedSectionMaxLen + 1);
		unsigned decodedLen = 0;

		if(datBase64::Decode(encodedSection, decodedSectionMaxLen, (u8*)decodedSection, &decodedLen))
		{
			epicDebug("    %s", decodedSection);
		}
		else
		{
			epicDebug("    Failed to base64 decode token section %u: %s", section, encodedSection);
		}

		encodedSection = strtok(NULL, ".");

		section = (JwtSection)(section + 1);
	}
}
#endif

void sysEpicAuthentication::LoginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data)
{
	EOS_EResult result = EOS_EResult::EOS_InvalidParameters;

	rtry
	{
		rverify(data,
				signInFailed,
				result = EOS_EResult::EOS_InvalidParameters;
				epicError("LoginCompleteCallback :: data is nullptr"));

		rverify(data->ResultCode == EOS_EResult::EOS_Success,
				signInFailed,
				result = data->ResultCode);

		epicDebug("LoginCompleteCallback :: Login Succeeded: %s", m_EpicApi.EOS_EResult_ToString(data->ResultCode));

		EOS_Auth_CopyUserAuthTokenOptions options = { 0 };
		options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		EOS_EResult authTokenResult = m_EpicApi.EOS_Auth_CopyUserAuthToken(m_AuthHandle, &options, data->LocalUserId, &m_AuthToken);
		rverify(authTokenResult == EOS_EResult::EOS_Success,
				signInAuthTokenFailed,
				result = authTokenResult;
				epicError("LoginCompleteCallback :: EOS_Auth_CopyUserAuthToken failed - Error Code: %s", m_EpicApi.EOS_EResult_ToString(authTokenResult)));

		int32_t bufSize = sizeof(m_AccountId);
		authTokenResult = m_EpicApi.EOS_EpicAccountId_ToString(data->LocalUserId, m_AccountId, &bufSize);
		rverify(authTokenResult == EOS_EResult::EOS_Success,
				signInAuthTokenFailed,
				result = authTokenResult;
				epicError("LoginCompleteCallback :: EOS_EpicAccountId_ToString failed - Error Code: %s", m_EpicApi.EOS_EResult_ToString(authTokenResult)));

		if(bufSize < sizeof(m_AccountId))
		{
			m_AccountId[bufSize] = '\0';
		}

		m_EosAccountId = data->LocalUserId;

#if !__NO_OUTPUT
		PrintAuthToken(m_AuthToken);

		const int32_t accountsCount = m_EpicApi.EOS_Auth_GetLoggedInAccountsCount(m_AuthHandle);
		for(int32_t accountIdx = 0; accountIdx < accountsCount; ++accountIdx)
		{
			EOS_EpicAccountId accountId;
			accountId = m_EpicApi.EOS_Auth_GetLoggedInAccountByIndex(m_AuthHandle, accountIdx);

			EOS_ELoginStatus loginStatus = m_EpicApi.EOS_Auth_GetLoginStatus(m_AuthHandle, data->LocalUserId);

			epicDebug("LoginCompleteCallback :: AccountIdx: %d Status: %s", accountIdx, EOSLoginStatusToString(loginStatus));
		}
#endif

		RequestDisplayNameAfterSignIn();
	}
	rcatch(signInFailed)
	{
		epicError("LoginCompleteCallback :: Login Failed - Error Code: %s", m_EpicApi.EOS_EResult_ToString(result));

		if (result == EOS_EResult::EOS_NoConnection)
		{
			// Pulling values from offline mode (Provided via command line)
			ReadEpicUserInfoFromCommandLine();
		}

		sysEpicEventSignInError e(sysEpicSignInError::SIGN_IN_FAILED, (int)result);
		g_SysEpic.DispatchEvent(&e);
	}
	rcatch(signInAuthTokenFailed)
	{
		sysEpicEventSignInError e(sysEpicSignInError::SIGN_IN_AUTH_TOKEN_FAILED, (int)result);
		g_SysEpic.DispatchEvent(&e);
	}
}

void sysEpicAuthentication::LogoutCompleteCallback(const EOS_Auth_LogoutCallbackInfo* OUTPUT_ONLY(data))
{
	epicDebug("Logout Complete for Account Id: %s. Result: %s", m_AccountId, m_EpicApi.EOS_EResult_ToString(data->ResultCode));

	ClearAccountInfo();

	// the signed-out event is dispatched in LoginStatusChangedCallback, which includes
	// unsolicited sign-outs (sign-outs we didn't initiate).
}

void sysEpicAuthentication::LoginStatusChangedCallback(const EOS_Auth_LoginStatusChangedCallbackInfo* data)
{
	epicDebug("LoginStatusChangedCallback :: sign-in status changed from %s to %s", EOSLoginStatusToString(data->PrevStatus), EOSLoginStatusToString(data->CurrentStatus));

	if(data->PrevStatus != data->CurrentStatus)
	{
		switch(data->CurrentStatus)
		{
		case EOS_ELoginStatus::EOS_LS_NotLoggedIn:
			{
				ClearAccountInfo();

				// this could be an unsolicited sign-out from the user and therefore our 
				// LogoutCompleteCallbackFn won't be called. Dispatch the event to calling process.
				
				DispatchSignInChangedEvent(sysEpicSignInStateFlags::STATE_SIGNED_OUT);
			}
			break;
		// signed-in events are dispatched to the calling process in LoginCompleteCallback(), which
		// can also handle/report sign-in errors.
		case EOS_ELoginStatus::EOS_LS_UsingLocalProfile:
			break;
		case EOS_ELoginStatus::EOS_LS_LoggedIn:
			break;
		case EOS_ELoginStatus::__EOS_ELoginStatus_PAD_INT32__: // autogenerated epic enum
			break;
		}
	}
}

bool sysEpicAuthentication::IsAuthTypeExchangeCode() const
{
	const char* authType;
	if (PARAM_AUTH_TYPE.Get(authType))
	{
		return stricmp(authType, "exchangecode") == 0;
	}

	return false;
}

bool sysEpicAuthentication::LoginAsync()
{
	epicDebug("Logging In...");
	epicDebug("EOS_AUTH_CREDENTIALS_API_LATEST: %d", EOS_AUTH_CREDENTIALS_API_LATEST);
	epicDebug("EOS_AUTH_LOGIN_API_LATEST: %d", EOS_AUTH_LOGIN_API_LATEST);

	static EOS_Auth_Credentials Credentials = {0};
	Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;

	static EOS_Auth_LoginOptions LoginOptions = {0};
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;

	// the Epic Games launcher passes the exchange code via the -AUTH_PASSWORD=<exchangeCode> -AUTH_TYPE=exchangecode command line
	if (IsAuthTypeExchangeCode())
	{
		const char* exchangeCode = nullptr;
		if (epicVerify(PARAM_AUTH_PASSWORD.Get(exchangeCode)))
		{
			Credentials.Token = exchangeCode;
		}

		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
	}
#if !RSG_FINAL || __FINAL_LOGGING
	else
	{
		const char* devToolHost = nullptr;
		epicVerifyf(PARAM_EpicDevToolHost.Get(devToolHost), "No Epic authentication credentials specified");

		const char* devToolTokenName = nullptr;
		epicVerifyf(PARAM_EpicDevToolTokenName.Get(devToolTokenName), "No Epic authentication credentials specified");
		
		Credentials.Id = devToolHost;
		Credentials.Token = devToolTokenName;
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
	}
#endif

	LoginOptions.Credentials = &Credentials;

	m_EpicApi.EOS_Auth_Login(m_AuthHandle, &LoginOptions, this, LoginCompleteCallbackFn);

	return true;
}

bool sysEpicAuthentication::LogoutAsync()
{
	epicDebug("Logging Out...");

	EOS_Auth_LogoutOptions LogoutOptions;
	LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
	LogoutOptions.LocalUserId = m_EosAccountId;

	m_EpicApi.EOS_Auth_Logout(m_AuthHandle, &LogoutOptions, this, LogoutCompleteCallbackFn);

	return true;
}

void sysEpicAuthentication::QueryUserInfoCallbackFn(const EOS_UserInfo_QueryUserInfoCallbackInfo* data)
{
	if(data == nullptr)
	{
		epicError("QueryUserInfoCallbackFn:: data is nullptr");
		return;
	}

	sysEpicAuthentication* auth = (sysEpicAuthentication*)data->ClientData;
	if(auth == nullptr)
	{
		epicError("QueryUserInfoCallbackFn :: data->ClientData is nullptr");
		return;
	}

	auth->QueryUserInfoCallback(data);
}

void sysEpicAuthentication::QueryUserInfoCallback(const EOS_UserInfo_QueryUserInfoCallbackInfo* data)
{
	// sign-in is not blocked if we fail to retrieve the player's display name.

	if(data && (data->ResultCode == EOS_EResult::EOS_Success))
	{
		EOS_HUserInfo UserInfoInterface = m_EpicApi.EOS_Platform_GetUserInfoInterface(m_EpicPlatform);
		EOS_UserInfo_CopyUserInfoOptions Options;
		Options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
		Options.LocalUserId = m_EosAccountId;
		Options.TargetUserId = m_EosAccountId;

		EOS_UserInfo* userInfo = nullptr;
		const EOS_EResult result = m_EpicApi.EOS_UserInfo_CopyUserInfo(UserInfoInterface, &Options, &userInfo);
		if(userInfo)
		{
			if(result == EOS_EResult::EOS_Success)
			{
				epicDebug("Query User Info Succeeded.");
				epicDebug("- Display Name: %s", userInfo->DisplayName);
				epicDebug("- Country: %s", userInfo->Country);
				epicDebug("- Preferred Language: %s", userInfo->PreferredLanguage);

				m_PlayerName = StringDuplicate(userInfo->DisplayName);
			}

			m_EpicApi.EOS_UserInfo_Release(userInfo);
		}
	}
	else
	{
		OUTPUT_ONLY(const EOS_EResult result = data ? data->ResultCode : EOS_EResult::EOS_InvalidParameters);
		epicError("QueryUserInfoCallback :: EOS_UserInfo_CopyUserInfo failed. Error Code: %s", m_EpicApi.EOS_EResult_ToString(result));
	}

	const EOS_ELoginStatus status = m_EpicApi.EOS_Auth_GetLoginStatus(m_AuthHandle, m_EosAccountId);
	epicAssertf(status != EOS_ELoginStatus::EOS_LS_NotLoggedIn,
				"QueryUserInfoCallback :: EOS_Auth_GetLoginStatus :: status %s",
				EOSLoginStatusToString(status));

	if(status == EOS_ELoginStatus::EOS_LS_UsingLocalProfile)
	{
		DispatchSignInChangedEvent(sysEpicSignInStateFlags::STATE_SIGNED_IN_USING_LOCAL_PROFILE);
	}
	else
	{
		DispatchSignInChangedEvent(sysEpicSignInStateFlags::STATE_SIGNED_ONLINE);
	}
}

void sysEpicAuthentication::RequestDisplayNameAfterSignIn()
{
	EOS_HUserInfo epicUserInfoInterface = m_EpicApi.EOS_Platform_GetUserInfoInterface(m_EpicPlatform);

	EOS_UserInfo_QueryUserInfoOptions queryUserInfoOptions;
	queryUserInfoOptions.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
	queryUserInfoOptions.LocalUserId = m_EosAccountId;
	queryUserInfoOptions.TargetUserId = m_EosAccountId;

	m_EpicApi.EOS_UserInfo_QueryUserInfo(epicUserInfoInterface, &queryUserInfoOptions, this, QueryUserInfoCallbackFn);
}

} // namespace rage

#endif // EPIC_API_SUPPORTED
