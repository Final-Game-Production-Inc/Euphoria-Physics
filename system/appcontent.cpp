//
// system/appcontent.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "appcontent.h"
#include "diag/channel.h"
#include "diag/seh.h"
#include "file/asset.h"
#include "system/param.h"

#if RSG_ORBIS
#include <libsysmodule.h>
#include <app_content.h>
#pragma comment(lib,"libSceAppContent_stub_weak.a")
#endif

#if RSG_DURANGO
#include <xdk.h>
#endif

PARAM(jpnbuild, "Overrides app content to return IsJapaneseBuild() == true");

namespace rage
{
	RAGE_DECLARE_CHANNEL(appcontent);
	RAGE_DEFINE_CHANNEL(appcontent)

	#define acnDebug(fmt, ...)     RAGE_DEBUGF1(appcontent, fmt, ##__VA_ARGS__)
	#define acnWarning(fmt, ...)   RAGE_WARNINGF(appcontent, fmt, ##__VA_ARGS__)
	#define acnError(fmt, ...)     RAGE_ERRORF(appcontent, fmt, ##__VA_ARGS__)

	bool sysAppContent::sm_bIsInitialized = false;
	int sysAppContent::sm_BuildVersion = Build_Unknown;

#if RSG_DURANGO
	unsigned sysAppContent::m_TitleId = 0;
	unsigned sysAppContent::m_JapaneseTitleId = 0;
#endif

	bool sysAppContent::Init()
	{
#if RSG_ORBIS
		rtry
		{
			rcheck(sceSysmoduleIsLoaded(SCE_SYSMODULE_APP_CONTENT) == SCE_SYSMODULE_ERROR_UNLOADED, catchall, );

			SceAppContentInitParam initParam;
			SceAppContentBootParam bootParam;

			memset(&initParam, 0, sizeof(SceAppContentInitParam));
			memset(&bootParam, 0, sizeof(SceAppContentBootParam));

			rverify(sceSysmoduleLoadModule(SCE_SYSMODULE_APP_CONTENT) == SCE_OK, catchall, acnError("Failed to load SCE_SYSMODULE_APP_CONTENT"));
			rverify(sceAppContentInitialize(&initParam, &bootParam) == SCE_OK, catchall, acnError("Failed to initialize SCE_SYSMODULE_APP_CONTENT"));

			// Build version comes from user-defined param 1
			int ret = sceAppContentAppParamGetInt( SCE_APP_CONTENT_APPPARAM_ID_USER_DEFINED_PARAM_1, &sm_BuildVersion );

		#if !__NO_OUTPUT
			if(ret!=SCE_OK || sm_BuildVersion==Build_Unknown)
				Quitf("Invalid system config data. Please check that your working directory is set to $(OutDir) or $(ProjectDir) for SampleSession, and that a valid PARAM.SFO file exists there, within the SCE_SYS dir.");
		#endif	// !__FINAL

			rverify(ret == SCE_OK, catchall, acnError("Failed to get User Defined Param 1: Build Version"));
		}
		rcatchall
		{
			return false;
		}
#elif RSG_DURANGO
		if (m_TitleId == m_JapaneseTitleId)
		{
			sm_BuildVersion = Build_Japanese;
		}
		else
		{
			sm_BuildVersion = Build_European;
		}
#else
		// Default build version is the European build
		sm_BuildVersion = Build_European;
#endif // RSG_ORBIS

		sm_bIsInitialized = true;

		if (PARAM_jpnbuild.Get())
		{
			sm_BuildVersion = Build_Japanese;
		}

		return true;
	}

	bool sysAppContent::Shutdown()
	{
#if RSG_ORBIS
		rtry
		{
			rcheck(sceSysmoduleIsLoaded(SCE_SYSMODULE_APP_CONTENT) == SCE_SYSMODULE_ERROR_UNLOADED, catchall, );
			rverify(sceSysmoduleUnloadModule(SCE_SYSMODULE_APP_CONTENT) == SCE_OK, catchall, acnError("Failed to load SCE_SYSMODULE_APP_CONTENT"));
		}
		rcatchall
		{
			return false;
		}
#endif

		return true;
	}

	bool sysAppContent::IsAmericanBuild()
	{
		Assert(sm_bIsInitialized);
		return (sm_BuildVersion & Build_American) != 0; 
	}

	bool sysAppContent::IsEuropeanBuild() 
	{
		Assert(sm_bIsInitialized);
		return (sm_BuildVersion & Build_European) != 0; 
	}

	bool sysAppContent::IsJapaneseBuild()
	{
		Assert(sm_bIsInitialized);
		return (sm_BuildVersion & Build_Japanese) != 0; 
	}

	const char* sysAppContent::GetBuildLocaleCode()
	{
		static const char regionCodeTable[][3] = { "**", "US", "EU", "JP", "SA" };

		if (sysAppContent::IsAmericanBuild())
		{
			return regionCodeTable[1];
		}
		else if (sysAppContent::IsEuropeanBuild())
		{
			return regionCodeTable[2];
		}
		else if (sysAppContent::IsJapaneseBuild())
		{
			return regionCodeTable[3];
		}

		return regionCodeTable[0];
	}

#if !__NO_OUTPUT
	const char* sysAppContent::GetBuildLocaleString()
	{
		if (sysAppContent::IsEuropeanBuild())
		{
			return "European";
		}
		else if (sysAppContent::IsAmericanBuild())
		{
			return "American";
		}
		else if (sysAppContent::IsJapaneseBuild())
		{
			return "Japanese";
		}
		else
		{
			return "UNKNOWN";
		}
	}
#endif
}
