// 
// system/epicauthentication.h
// 
// Copyright (C) 1999-2019 Rockstar Games.  All Rights Reserved. 
// 

#ifndef EPIC_AUTHENTICATION_H 
#define EPIC_AUTHENTICATION_H 

#include "file/file_config.h"

#if EPIC_API_SUPPORTED

#include "epiccommon.h"
#include "epicevents.h"

// Epic SDK includes
#include "../../3rdparty/Epic/Include/eos_auth.h"
#include "../../3rdparty/Epic/Include/eos_auth_types.h"
#include "../../3rdparty/Epic/Include/eos_userinfo_types.h"

namespace rage
{

class sysEpicAuthentication
{
public:

	sysEpicAuthentication(sysEpicApi& epicApi);
	virtual ~sysEpicAuthentication();

	bool Init(EOS_HPlatform hEpicPlatform);
	void Shutdown();
	void Update();

	bool IsOfflineMode() const;

	void RefreshAccessTokenAsync();

	void SetPlayerName(const char* playerName);
	const char* GetPlayerName() const;

	const char* GetAccessToken() const;
	const char* GetAccountId() const;

private:
	OUTPUT_ONLY(void PrintAuthToken(EOS_Auth_Token* InAuthToken));

	void DispatchSignInChangedEvent(const sysEpicSignInStateFlags flags);

	static void EOS_CALL LoginCompleteCallbackFn(const EOS_Auth_LoginCallbackInfo* data);
	void LoginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data);

	static void EOS_CALL LogoutCompleteCallbackFn(const EOS_Auth_LogoutCallbackInfo* data);
	void LogoutCompleteCallback(const EOS_Auth_LogoutCallbackInfo* data);

	static void EOS_CALL LoginStatusChangedCallbackFn(const EOS_Auth_LoginStatusChangedCallbackInfo* data);
	void LoginStatusChangedCallback(const EOS_Auth_LoginStatusChangedCallbackInfo* data);

	static void EOS_CALL QueryUserInfoCallbackFn(const EOS_UserInfo_QueryUserInfoCallbackInfo* data);
	void QueryUserInfoCallback(const EOS_UserInfo_QueryUserInfoCallbackInfo* data);

	bool IsAuthTypeExchangeCode() const;

	bool LoginAsync();
	bool LogoutAsync();

	void RequestDisplayNameAfterSignIn();
	void ClearAccountInfo();
	void ReadEpicUserInfoFromCommandLine();

	sysEpicApi& m_EpicApi;
	EOS_HPlatform m_EpicPlatform;
	EOS_HAuth m_AuthHandle;
	EOS_NotificationId m_NotificationId;
	EOS_EpicAccountId m_EosAccountId;

	EOS_Auth_Token* m_AuthToken;
	char m_AccountId[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
	char* m_PlayerName;

	bool m_OfflineMode;
};

} // namespace rage

#endif // EPIC_API_SUPPORTED
#endif // EPIC_AUTHENTICATION_H 
