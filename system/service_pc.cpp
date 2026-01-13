// 
// system/service_pc.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "service.h"
#include "file/file_config.h"
#include "string/stringutil.h"
#include "system/epic.h"
#include "system/memops.h"
#include "system/epic.h"
#include "system/threadtype.h"

#if RSG_PC && !__TOOL
#include "grcore/device.h"
#include "system/param.h"
#include "system/timer.h"

#if __STEAM_BUILD
#pragma warning(disable: 4265)
#include "../../3rdParty/Steam/public/steam/steam_api.h"
#pragma warning(error: 4265)
#endif

#pragma warning(disable: 4668)
#include <windows.h>
#pragma warning(error: 4668)

#define SUPPORT_SIMPLIFIED_CHINESE RSG_PC

namespace rage 
{
	PARAM(steamLanguage, "Force the Steam GetCurrentGameLanguage response");

	NOSTRIP_PARAM(rglLanguage, "Language from the Rockstar Games Launcher");

	static u32 s_ConstrainedStartTime = 0;
	
#if __STEAM_BUILD
	sysService::sysService()
		: m_callResultGameOverlay(this, &sysService::OnGameOverlayCallback)
		, m_SteamOverlayActive(false)
		, m_ExecutedSteamCallbacks(false)
	{

	}
#endif

	void sysService::InitClass() 
	{
#if __STEAM_BUILD
		m_SteamOverlayActive = false;
#endif
	}

	void sysService::ShutdownClass() 
	{

	}

	static bool s_HasFocus = false;
	bool HasFocus()
	{
#if __RGSC_DLL
		return true;
#else
		return !GRCDEVICE.GetLostFocusForAudio();
#endif
	}

	void sysService::UpdateClass()
	{
#if __STEAM_BUILD
		SteamAPI_RunCallbacks();
		m_ExecutedSteamCallbacks = true;
#endif // __STEAM_BUILD

#if EPIC_API_SUPPORTED
		// Can only update Epic API on main thread.
		if (sysThreadType::IsUpdateThread())
		{
			g_SysEpic.UpdateClass();
		}
#endif
		
#if !defined(__RGSC_DLL) || !__RGSC_DLL
		// on PC we're considering the game to be in resource 'constrained' mode if the
		// graphics device isn't ready. i.e. if we've lost the device, minimized, etc.
		// but not if we're just in the background in windowed mode.
		if(GRCDEVICE.IsReady())
		{
			if(s_ConstrainedStartTime != 0)
			{
				sysServiceEvent evnt(sysServiceEvent::UNCONSTRAINED);
				TriggerEvent(&evnt);

				s_ConstrainedStartTime = 0;
			}
		}
		else if(s_ConstrainedStartTime == 0)
		{
			sysServiceEvent evnt(sysServiceEvent::CONSTRAINED);
			TriggerEvent(&evnt);

			s_ConstrainedStartTime = sysTimer::GetSystemMsTime();
		}

		bool hasFocus = HasFocus();

		if(hasFocus && !s_HasFocus)
		{
			sysServiceEvent focus_evnt(sysServiceEvent::FOCUS_GAINED);
			TriggerEvent(&focus_evnt);

			sysServiceEvent input_focus_evnt(sysServiceEvent::INPUT_FOCUS_GAINED);
			TriggerEvent(&input_focus_evnt);
		}
		else if(!hasFocus && s_HasFocus)
		{
			sysServiceEvent focus_evnt(sysServiceEvent::FOCUS_LOST);
			TriggerEvent(&focus_evnt);

			sysServiceEvent input_focus_evnt(sysServiceEvent::INPUT_FOCUS_LOST);
			TriggerEvent(&input_focus_evnt);
		}

		s_HasFocus = hasFocus;
#endif
	}

#if __STEAM_BUILD
	void sysService::OnGameOverlayCallback(GameOverlayActivated_t* goat)
	{
		if (goat)
		{
			m_SteamOverlayActive = goat->m_bActive != 0;
			Displayf("Steam Overlay: %s", m_SteamOverlayActive ? "activated" : "closed");
		}
	}
#endif

	bool sysService::IsUiOverlaid()
	{
#if __STEAM_BUILD
		// Return true until we've executed our first callback. Otherwise, 
		// we don't know if the overlay is active or not.
		return m_SteamOverlayActive || !m_ExecutedSteamCallbacks;
#else
		Assertf(false, "sysService::IsUiOverlaid() is not supported on this platform");
		return false;
#endif
	}

	bool sysService::IsInBackgroundExecution()
	{
		Assertf(false, "sysService::IsInBackgroundExecution() is not supported on this platform");
		return false;
	}

	bool sysService::IsConstrained() const
	{
		return s_ConstrainedStartTime > 0;
	}

	u32 sysService::GetConstrainedDurationMs() const
	{
		return (s_ConstrainedStartTime == 0) ? 0 : sysTimer::GetSystemMsTime() - s_ConstrainedStartTime;
	}

	bool sysService::HasResumedFromSuspend() const
	{
		return false;
	}

	bool sysService::SetMusicPlayerEnabled(bool /* bEnable */)
	{
		Assertf(false, "sysService::SetMusicPlayerEnabled() is not supported on this platform");
		return false;
	}

	void sysService::HideSplashScreen()
	{
		Assertf(false, "sysService::HideSplashScreen() is not supported on this platform");
	}

	sysLanguage sysService::GetSystemLanguage()
	{
		struct PcLanguageMap
		{
			const char* SteamKey;
			const char* RglKey;
			const char* AltRglKey;
			sysLanguage Language;
		};

		const PcLanguageMap languageMap[] =
		{
			{ "english",	"en-US",	"",			LANGUAGE_ENGLISH },
			{ "tchinese",	"zh-TW",	"zh-CHT",	LANGUAGE_CHINESE_TRADITIONAL },
			{ "schinese",	"zh-CN",	"zh-CHS",	LANGUAGE_CHINESE_SIMPLIFIED },
			{ "french",		"fr-FR",	"",			LANGUAGE_FRENCH },
			{ "german",		"de-DE",	"",			LANGUAGE_GERMAN },
			{ "italian",	"it-IT",	"",			LANGUAGE_ITALIAN },
			{ "japanese",	"ja-JP",	"",			LANGUAGE_JAPANESE },
			{ "koreana",	"ko-KR",	"",			LANGUAGE_KOREAN },
			{ "spanish",	"es-ES",	"",			LANGUAGE_SPANISH },
			{ "latam",		"es-MX",	"",			LANGUAGE_MEXICAN },
			{ "polish",		"pl-PL",	"",			LANGUAGE_POLISH },
			{ "brazilian",	"pt-BR",	"pt-PT",	LANGUAGE_PORTUGUESE },
			{ "russian",	"ru-RU",	"",			LANGUAGE_RUSSIAN },
		};

		sysLanguage language = LANGUAGE_UNDEFINED;

		int langID = GetUserDefaultUILanguage();
		int primID = PRIMARYLANGID(langID);
		int subID = SUBLANGID(langID);

		wchar_t wideLocaleName[LOCALE_NAME_MAX_LENGTH] = {0};
		GetSystemDefaultLocaleName(wideLocaleName, LOCALE_NAME_MAX_LENGTH);

		Displayf("GetUserDefaultUILanguage :: LangId: %d, Prim ID: %d, Sub Id: %d", langID, primID, subID);
		Displayf("GetSystemDefaultLocaleName :: %ls", wideLocaleName);

		const char * steamLanguage = nullptr;
		const char * rglLanguage = nullptr;

#if __STEAM_BUILD
		if (SteamApps())
		{
			// https://partner.steamgames.com/documentation/languages
			const char* paramSteamLanguage;
			steamLanguage = PARAM_steamLanguage.Get(paramSteamLanguage) ? paramSteamLanguage : SteamApps()->GetCurrentGameLanguage();
		}

		// If we detected the steam install language, use it.
		// Otherwise, fall back to the windows default UI language
		if (language != LANGUAGE_UNDEFINED)
		{
			return language;
		}
#endif

		// Query the language provided by the Rockstar Games Launcher
		const char* languageArg = nullptr;
		if (PARAM_rglLanguage.Get(languageArg))
		{
			Displayf("-rglLanguage: %s", languageArg);
			rglLanguage = languageArg;
		}

		// Check the language written to the registry key.
		char languageKey[1024] = {0};

		const char* regKeys[] = {
			"SOFTWARE\\Wow6432Node\\Rockstar Games\\Grand Theft Auto V",
			"SOFTWARE\\Rockstar Games\\Grand Theft Auto V",
			"SOFTWARE\\Wow6432Node\\Rockstar Games\\Launcher",
			"SOFTWARE\\Rockstar Games\\Launcher"
		};

		HKEY hkey;
		for (int i = 0; i < COUNTOF(regKeys); i++)
		{
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regKeys[i], 0, KEY_READ, &hkey) == ERROR_SUCCESS)
			{
				DWORD length = sizeof(languageKey);
				if (RegQueryValueExA(hkey, "Language", NULL, NULL, (LPBYTE)languageKey, &length) == ERROR_SUCCESS)
				{
					RegCloseKey(hkey);
					break;
				}

				RegCloseKey(hkey);
			}
		}

		// Look to see if the language provided by Steam or the Rockstar Games Launcher can be found.
		for (const PcLanguageMap& entry : languageMap)
		{
			if ((!StringNullOrEmpty(entry.SteamKey) && StringIsEqual(steamLanguage, entry.SteamKey, true)) ||
				(!StringNullOrEmpty(entry.RglKey) && StringIsEqual(rglLanguage, entry.RglKey, true)) ||
				(!StringNullOrEmpty(entry.AltRglKey) && StringIsEqual(rglLanguage, entry.AltRglKey, true)))
			{
				return entry.Language;
			}
		}

		// If the launcher did not provide the language, fall back to the registry
		for (const PcLanguageMap& entry : languageMap)
		{
			if ((!StringNullOrEmpty(entry.RglKey) && StringIsEqual(languageKey, entry.RglKey, true)) ||
				(!StringNullOrEmpty(entry.AltRglKey) && StringIsEqual(languageKey, entry.AltRglKey, true)))
			{
				return entry.Language;
			}
		}

		switch(primID)
		{
			// NOTE: LANG_CHINESE and LANG_CHINESE_SIMPLIFIED are identical.
			//	Ensure we check LANG_CHINESE with sublanguage SUBLANG_CHINESE_TRADITIONAL,
			//	or for LANG_CHINESE_TRADITIONAL directly.
		case LANG_CHINESE:
			if (subID == SUBLANG_CHINESE_TRADITIONAL)
			{
				language = LANGUAGE_CHINESE_TRADITIONAL;
			}
			else
			{
				language = LANGUAGE_CHINESE_SIMPLIFIED; 
			}
			break;
		case LANG_CHINESE_TRADITIONAL:
			language = LANGUAGE_CHINESE_TRADITIONAL;
			break;
		case LANG_ENGLISH:
			language = LANGUAGE_ENGLISH;
			break;
		case LANG_FRENCH:
			language = LANGUAGE_FRENCH;
			break;
		case LANG_GERMAN:
			language = LANGUAGE_GERMAN;
			break;
		case LANG_ITALIAN:
			language = LANGUAGE_ITALIAN;
			break;
		case LANG_JAPANESE:
			language = LANGUAGE_JAPANESE;
			break;
		case LANG_KOREAN:
			language = LANGUAGE_KOREAN;
			break;
		case LANG_POLISH:
			language = LANGUAGE_POLISH;
			break;
		case LANG_PORTUGUESE:
			language = LANGUAGE_PORTUGUESE;
			break;
		case LANG_SPANISH:
			if (subID == SUBLANG_SPANISH_MEXICAN)
				language = LANGUAGE_MEXICAN;
			else
				language = LANGUAGE_SPANISH;
			break;
		case LANG_RUSSIAN:
			language = LANGUAGE_RUSSIAN;
			break;
		default:
			language = LANGUAGE_ENGLISH;
			break;
		}

		return language;
	}

	bool sysService::TriggerEvent(sysServiceEvent* serviceEvent)
	{
		m_Delegator.Dispatch(serviceEvent);
		return true;
	}

} // namespace rage

#endif // #if RSG_PC
