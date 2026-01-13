// 
// system/epic.cpp
// 
// Copyright (C) 1999-2019 Rockstar Games.  All Rights Reserved. 
// 

#include "epic.h"

#if EPIC_API_SUPPORTED

#include "diag/channel.h"
#include "diag/seh.h"
#include "rline/rldiag.h"
#include "string/string.h"
#include "system/memops.h"
#include "system/param.h"
#include "system/xtl.h"

// Epic SDK includes
#include "../../3rdparty/Epic/Include/eos_sdk.h"
#include "../../3rdparty/Epic/Include/eos_init.h"
#include "../../3rdparty/Epic/Include/eos_logging.h"

namespace rage
{

// Global Instance
sysEpic g_SysEpic;

// Command line passed in by the Launcher or Bootstrapper to activate Epic APIs.
NOSTRIP_PC_PARAM(useEpic, "Activates the Epic APIs");

// Channeled Logging
RAGE_DEFINE_CHANNEL(epic)
#undef __rage_channel
#define __rage_channel epic

#define EPIC_DLL_NAME "EOSSDK-Win64-Shipping.dll"
#define EPIC_DLL_NAMEW L"EOSSDK-Win64-Shipping.dll"

//////////////////////////////////////////////////////////////////////////
// sysEpic
//////////////////////////////////////////////////////////////////////////
sysEpic::sysEpic()
: m_Initialized(false)
, m_EpicPlatform(nullptr)
, m_Auth(m_EpicApi)
, m_hEpicDll(NULL)
{

}

sysEpic::~sysEpic()
{

}

bool sysEpic::LoadEpicLib()
{
	rtry
	{
		// Get the executable directory.
		wchar_t wideDllPath[MAX_PATH] = { 0 };
		GetModuleFileNameW(GetModuleHandleW(NULL), wideDllPath, MAX_PATH);

		// Remove file name (i.e. text after trailing slash).
		// Note "::PathRemoveFileSpec" is now depreciated, and "PathCchRemoveFileSpec" is for Windows 8+
		int lastSlash = 0;
		for(int i = 0; i < MAX_PATH && *wideDllPath; i++)
		{
			if(wideDllPath[i] == L'\\' || wideDllPath[i] == L'/')
				lastSlash = i;
		}

		// Remove trailing slash.
		if(lastSlash > 0)
			wideDllPath[lastSlash] = 0;

		// Append the epic API dll name
		safecat(wideDllPath, L"\\" EPIC_DLL_NAMEW);

		// We use the LOAD_WITH_ALTERED_SEARCH_PATH flag to guarantee we load our Epic DLL
		// instead of the default LoadLibrary options (which could pick up another dll).
		m_hEpicDll = LoadLibraryExW(wideDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

#if RSG_DEV
		if(m_hEpicDll == NULL)
		{
			::GetCurrentDirectoryW(MAX_PATH, wideDllPath);
			safecat(wideDllPath, L"\\" EPIC_DLL_NAMEW);
			m_hEpicDll = LoadLibraryExW(wideDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		}
#endif // RSG_DEV

		rverify(m_hEpicDll != NULL, catchall, epicError("LoadLibraryExW %ls failed with error: 0x%08x", wideDllPath, GetLastError()));
		epicDebug("Successfully loaded library from: %ls", wideDllPath);

		rverifyall(m_EpicApi.Init(m_hEpicDll));

		return true;
	}
	rcatchall
	{
		return false;
	}
}

bool sysEpic::InitClass(const char* epicProductName, const char* epicProductVersion, const char* epicProductId, const char* epicSandboxId, const char* epicClientId, const char* epicClientSecret, const char* epicDeploymentId)
{
	bool success = false;

	rtry
	{
		// Only initialize if we're an Epic SKU
		rcheck(PARAM_useEpic.Get(), nonepic,);

		rcheckall(LoadEpicLib());

		// Init EOS SDK
		EOS_InitializeOptions SDKOptions;
		SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
		SDKOptions.AllocateMemoryFunction = nullptr;
		SDKOptions.ReallocateMemoryFunction = nullptr;
		SDKOptions.ReleaseMemoryFunction = nullptr;
		SDKOptions.ProductName = epicProductName;
		SDKOptions.ProductVersion = epicProductVersion;
		SDKOptions.Reserved = nullptr;

		epicDebug("Initializing... API Version: %d, Product Name: %s, Product Version: %s",
				  SDKOptions.ApiVersion, SDKOptions.ProductName, SDKOptions.ProductVersion);

		EOS_EResult result = m_EpicApi.EOS_Initialize(&SDKOptions);
		rverify(result == EOS_EResult::EOS_Success, catchall, epicError("EOS_Initialize failed: %s", m_EpicApi.EOS_EResult_ToString(result)));

		// Create platform instance
		EOS_Platform_Options platformOptions = {0};
		platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		platformOptions.bIsServer = EOS_FALSE;
		platformOptions.Reserved = NULL;
		platformOptions.ProductId = epicProductId;
		platformOptions.SandboxId = epicSandboxId;
		platformOptions.ClientCredentials.ClientId = epicClientId;
		platformOptions.ClientCredentials.ClientSecret = epicClientSecret;
		platformOptions.DeploymentId = epicDeploymentId;

		// TODO: NS - Note, as of Epic SDK 1.2.0, enabling the Epic overlay causes the mouse cursor to appear
		// on screen indefinitely. We will need to revisit this once Epic has released an update that fixes it.
		platformOptions.Flags = EOS_PF_DISABLE_OVERLAY;

		m_EpicPlatform = m_EpicApi.EOS_Platform_Create(&platformOptions);
		rverify(m_EpicPlatform != nullptr, catchall, epicError("EOS_Platform_Create failed"));

		m_ProductId = epicProductId;
		m_SandboxId = epicSandboxId;

#if !__NO_OUTPUT
		EOS_EResult SetLogCallbackResult = m_EpicApi.EOS_Logging_SetCallback(&EpicSdkOutputCallback);
		if(SetLogCallbackResult != EOS_EResult::EOS_Success)
		{
			epicError("EOS_Logging_SetCallback failed: %s", m_EpicApi.EOS_EResult_ToString(result));
		}
		else
		{
			m_EpicApi.EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);
		}
#endif

		rverifyall(m_Auth.Init(m_EpicPlatform));

		m_Initialized = true;
		success = true;
	}
	rcatch(nonepic)
	{
		success = true;
	}
	rcatchall
	{
		ShutdownClass();
	}

	return success;
}

void sysEpic::UpdateClass()
{
	if (!m_Initialized)
	{
		return;
	}

	m_EpicApi.EOS_Platform_Tick(m_EpicPlatform);

	m_Auth.Update();
}

void sysEpic::ShutdownClass()
{
	if(m_Initialized)
	{
		epicDebug("sysEpic::ShutdownClass");

		m_Auth.Shutdown();

		if(m_EpicPlatform)
		{
			epicDebug("sysEpic::ShutdownClass :: EOS_Platform_Release");

			m_EpicApi.EOS_Platform_Release(m_EpicPlatform);
			m_EpicPlatform = NULL;
		}

		EOS_EResult result = m_EpicApi.EOS_Shutdown();
		if(result != EOS_EResult::EOS_Success)
		{
			epicError("EOS_Shutdown failed: %s", m_EpicApi.EOS_EResult_ToString(result));
		}
		else
		{
			epicDebug("EOS_Shutdown succeeded.");
		}
	}

	if(m_hEpicDll != NULL)
	{
		if(::FreeLibrary(m_hEpicDll) == FALSE)
		{
			epicDebug("FreeLibrary failed: 0x%08x", GetLastError());
		}
		else
		{
			epicDebug("FreeLibrary succeeded.");
		}
		m_hEpicDll = NULL;
	}

	m_EpicApi.Shutdown();

	m_Initialized = false;
}

#if !__NO_OUTPUT
void sysEpic::EpicSdkOutputCallback(const EOS_LogMessage* msg)
{
	if(msg->Level == EOS_ELogLevel::EOS_LOG_Fatal)
	{
		epicAssertf(false, "[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
	else if(msg->Level == EOS_ELogLevel::EOS_LOG_Error)
	{
		epicError("[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
	else if(msg->Level == EOS_ELogLevel::EOS_LOG_Warning)
	{
		epicWarning("[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
	else if(msg->Level == EOS_ELogLevel::EOS_LOG_Info)
	{
		epicDebug("[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
	else if(msg->Level == EOS_ELogLevel::EOS_LOG_Verbose)
	{
		epicDebug2("[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
	else
	{
		epicDebug3("[EOS SDK] %s: %s", msg->Category, msg->Message);
	}
}
#endif

void sysEpic::AddDelegate(sysEpicEventDelegate* dlgt)
{
	m_Delegator.AddDelegate(dlgt);
}

void sysEpic::RemoveDelegate(sysEpicEventDelegate* dlgt)
{
	m_Delegator.RemoveDelegate(dlgt);
}

void sysEpic::DispatchEvent(const sysEpicEvent* e)
{
	m_Delegator.Dispatch(this, e);
}

void sysEpic::RefreshAccessTokenAsync()
{
	m_Auth.RefreshAccessTokenAsync();
}

const char* sysEpic::GetAccessToken() const
{
	return m_Auth.GetAccessToken();
}

const char* sysEpic::GetPlayerName() const
{
	return m_Auth.GetPlayerName();
}

const char* sysEpic::GetAccountId() const
{
	return m_Auth.GetAccountId();
}

const char* sysEpic::GetProductId() const
{
	return m_ProductId.c_str();
}

const char* sysEpic::GetSandboxId() const
{
	return m_SandboxId.c_str();
}

const char* sysEpic::GetDeploymentId() const
{
	return m_DeploymentId.c_str();
}

bool sysEpic::IsOfflineMode() const
{
	return m_Auth.IsOfflineMode();
}

} // namespace rage

#endif // EPIC_API_SUPPORTED
