// 
// system/service_durango.winrt.cpp
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
//

#if RSG_DURANGO

// Main Header.
#include "service_durango.winrt.h"

// Rage Headers.
#include "diag/channel.h"
#include "file/limits.h"
#include "string/unicode.h"
#include "system/appcontent.h"
#include "system/criticalsection.h"
#include "system/timer.h"
#include "system/messagequeue.h"
#include "system/xtl.h"
#include <xdk.h>
#include <winnls.h>

namespace rage
{

RAGE_DEFINE_CHANNEL(servicedurango);

#define sdDebug1(fmt, ...)  RAGE_DEBUGF1(servicedurango, fmt, ##__VA_ARGS__)
#define sdDebug2(fmt, ...)  RAGE_DEBUGF2(servicedurango, fmt, ##__VA_ARGS__)
#define sdDebug3(fmt, ...)  RAGE_DEBUGF3(servicedurango, fmt, ##__VA_ARGS__)
#define sdWarning(fmt, ...) RAGE_WARNINGF(servicedurango, fmt, ##__VA_ARGS__)
#define sdError(fmt, ...)   RAGE_ERRORF(servicedurango, fmt, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////
// WinRT Types
//////////////////////////////////////////////////////////////////////////
//
// NOTE: WinRT has exceptionally long type names (and uses templates)
// making types longer and they like to keep changing their namespaces
// during development. For this reason Pull them into a smaller
// Durango NameSpace (dns).
// 
///////////////////////////////////////////////////////////////////////
namespace dns
{
	using Windows::Foundation::TypedEventHandler;
	using Windows::Foundation::EventRegistrationToken;
	using Windows::Xbox::Media::GameTransportControls;
	using Windows::Xbox::Media::GameTransportControlsButtonPressedEventArgs;
	using Windows::UI::Core::CoreWindow;
	using Windows::UI::Core::CoreProcessEventsOption;
}

#if _XDK_VER >= 9698
static dns::GameTransportControls^ s_GameTransportControls = nullptr;
static dns::EventRegistrationToken s_GameTransportControlsToken;
#endif //  _XDK_VER >= 9698

static ServiceDelegate m_ServiceDelegate;
static sysMessageQueue<sysDurangoServiceEvent, 32> m_deferredEvents;

// the time at which we entered the Constrained PLM mode, or 0 if we're not constrained.
static u32 s_ConstrainedStartTime = 0;

static void OnServiceEvent(sysServiceEvent* evnt)
{
	if(evnt != nullptr)
	{
		if(evnt->GetType() == sysServiceEvent::CONSTRAINED)
		{
			s_ConstrainedStartTime = sysTimer::GetSystemMsTime();
		}
		else if((evnt->GetType() == sysServiceEvent::UNCONSTRAINED) || (evnt->GetType() == sysServiceEvent::RESUMING))
		{
			s_ConstrainedStartTime = 0;
		}
	}
}

void sysService::InitClass()
{
	m_HasResumedFromSuspendThisFrame = false;
	m_HasResumedFromSuspendLastFrame = false;
	m_ThreadId = sysIpcGetCurrentThreadId();

#if _XDK_VER >= 9698
	try
	{
		if(s_GameTransportControls == nullptr)
		{
			s_GameTransportControls = ref new dns::GameTransportControls();
			s_GameTransportControlsToken = s_GameTransportControls->ButtonPressed += ref new dns::TypedEventHandler<dns::GameTransportControls^, dns::GameTransportControlsButtonPressedEventArgs^>(
				[this] (dns::GameTransportControls^, dns::GameTransportControlsButtonPressedEventArgs^ args)
			{
				switch (args->Button)
				{
				case Windows::Xbox::Media::GameTransportControlsButton::Back:
					{
						sysDurangoServiceEvent evt(sysServiceEvent::BACK, args);
						DeferEvent(&evt);
					}
					break;
				case Windows::Xbox::Media::GameTransportControlsButton::Pause:
					{
						sysDurangoServiceEvent evt(sysServiceEvent::PAUSE, args);
						DeferEvent(&evt);
					}
					break;
				case Windows::Xbox::Media::GameTransportControlsButton::Play:
					{
						sysDurangoServiceEvent evt(sysServiceEvent::PLAY, args);
						DeferEvent(&evt);
					}
					break;

				// These are handled in the input code.
				case Windows::Xbox::Media::GameTransportControlsButton::Menu:
					{
						sysDurangoServiceEvent evt(sysServiceEvent::MENU, args);
						DeferEvent(&evt);
					}
					break;

				case Windows::Xbox::Media::GameTransportControlsButton::View:
					{
						sysDurangoServiceEvent evt(sysServiceEvent::VIEW, args);
						DeferEvent(&evt);
					}
					break;
					break;

				default:
					Assertf(false, "Unknown button pressed");
					break;
				}
			}
			);

			s_GameTransportControls->IsPauseEnabled = true;
			s_GameTransportControls->IsPlayEnabled = true;
			s_GameTransportControls->IsBackEnabled = true;
		}
		
		m_ServiceDelegate.Bind(&OnServiceEvent);
		g_SysService.AddDelegate(&m_ServiceDelegate);
	}
	catch(Platform::Exception^)
	{
		Assertf(false, "Failed to register game button events!");
	}
#endif // _XDK_VER >= 9698
}

void sysService::ShutdownClass()
{
#if _XDK_VER >= 9698
	try
	{
		if(s_GameTransportControls != nullptr)
		{
			s_GameTransportControls->ButtonPressed -= s_GameTransportControlsToken;
			s_GameTransportControls = nullptr;
		}
	}
	catch(Platform::Exception^)
	{
		Assertf(false, "Failed to un-register game button events!");
	}
#endif // _XDK_VER >= 9698
}

void sysService::UpdateClass()
{
	if(m_ThreadId == sysIpcGetCurrentThreadId())
	{
		m_ScreenDimmingDisabledLastFrame = m_ScreenDimmingDisabledThisFrame;
		m_ScreenDimmingDisabledThisFrame = false;

		m_HasResumedFromSuspendLastFrame = m_HasResumedFromSuspendThisFrame;
		m_HasResumedFromSuspendThisFrame = false;
	}
}

bool sysService::IsUiOverlaid()
{
	Assertf(false, "sysService::IsUiOverlaid() is not supported on this platform");
	return false;
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
	return m_HasResumedFromSuspendLastFrame || m_HasResumedFromSuspendThisFrame;
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
	sysLanguage language = LANGUAGE_UNDEFINED;

	wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
	int retVal = GetUserDefaultLocaleName( localeName, ARRAYSIZE( localeName ) );

	DWORD error;
	if ( retVal == 0 )
	{
		error = GetLastError();
		sdError("GetUserDefaultLocaleName() failed with error code: 0x%x\n",  error );
		return language;
	}
	else
	{
		sdDebug3("The Locale Name from GetUserDefaultLocaleName: %s\n",  localeName );
	}

	LCID localeID = LocaleNameToLCID( localeName, 0 );
	WORD primary = 0;
	WORD secondary = 0;
	if ( localeID == 0 )
	{
		sdError("LocaleNameToLCID() failed with error code: 0x%x\n",  GetLastError() );
		return language;
	}
	else
	{
		sdDebug3("The LCID for the locale: 0x%x\n",  localeID );
		primary = PRIMARYLANGID( localeID );
		secondary = SUBLANGID( localeID );
		sdDebug3("Primary Language: 0x%x   Secondary Language: 0x%x\n",  primary, secondary );

		// For Japanese builds we only allow English and Japanese (url:bugstar:2031959).
		if(sysAppContent::IsJapaneseBuild())
		{
			if(primary == LANG_JAPANESE)
			{
				return LANGUAGE_JAPANESE;
			}
			else
			{
				return LANGUAGE_ENGLISH;
			}
		}

		switch(primary)
		{
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
		case LANG_POLISH:
			language = LANGUAGE_POLISH;
			break;
		case LANG_PORTUGUESE:
			language = LANGUAGE_PORTUGUESE;
			break;
		case LANG_SPANISH:
			if(secondary == SUBLANG_SPANISH_MEXICAN)
			{
				language = LANGUAGE_MEXICAN;
			}
			else
			{
				language = LANGUAGE_SPANISH;
			}
			break;
		case LANG_RUSSIAN:
			language = LANGUAGE_RUSSIAN;
			break;
		case LANG_JAPANESE:
			language = LANGUAGE_JAPANESE;
			break;
		case LANG_KOREAN:
			language = LANGUAGE_KOREAN;
			break;
		case LANG_CHINESE:
			if(secondary == SUBLANG_CHINESE_SIMPLIFIED)
			{
				// Chinese Simplified defaults to English as per requested in B*3778437
				language = LANGUAGE_ENGLISH;
			}
			else
			{
				language = LANGUAGE_CHINESE_TRADITIONAL;
			}
			break;
		case LANG_CHINESE_TRADITIONAL:
			language = LANGUAGE_CHINESE_TRADITIONAL;
			break;
		}
	}

	return language;
}

bool sysService::GetSystemCountry(char (&countryCode)[3])
{
	GEOID geoID = GetUserGeoID(GEOCLASS_NATION);

	wchar_t wideCode[3] = {0};
	int retVal = GetGeoInfoW(geoID, GEO_ISO2, wideCode, ARRAYSIZE(wideCode), 0);
	if(retVal == 0)
	{
		Errorf("GetGeoInfoW() failed with error code: 0x%x\n", GetLastError());
		return false;
	}

	// convert to ANSII
	countryCode[0] = wideCode[0] & 0xff;
	countryCode[1] = wideCode[1] & 0xff;
	countryCode[2] = '\0';

	return true;
}

bool sysService::TriggerEvent(sysServiceEvent* serviceEvent)
{
	m_Delegator.Dispatch(serviceEvent);
	return true;
}

void sysService::DeferEvent(sysDurangoServiceEvent* serviceEvent)
{
	m_deferredEvents.Push(*serviceEvent);
}

void sysService::DispatchDeferredEvents()
{
	sysDurangoServiceEvent evt;
	while(m_deferredEvents.PopPoll(evt))
	{
		if(evt.GetType() == sysServiceEvent::RESUMING)
		{
			m_HasResumedFromSuspendThisFrame = true;
		}
		m_Delegator.Dispatch(&evt);
	}
}

const char* sysService::GetPersistentStorageRoot()
{
	static char path[RAGE_MAX_PATH] = { 0 };

	// Whilst we could have checked if path is valid, if we fail to retrieve it we may end up constantly trying to retrieve it.
	// We use this guard to not do that as a WinRT exception can take a long time be thrown.
	static bool pathRetrieved = false;
	if (pathRetrieved == false)
	{
		pathRetrieved = true;

		USES_CONVERSION;
		try
		{
			Windows::Storage::ApplicationData^ applicationData = Windows::Storage::ApplicationData::Current;
			if (applicationData != nullptr)
			{
				Windows::Storage::StorageFolder^ storageFolder = applicationData->LocalFolder;
				if (storageFolder != nullptr)
				{
					Platform::String^ storagePath = storageFolder->Path;
					if (storagePath != nullptr)
					{
						formatf(path, "%s\\", WIDE_TO_UTF8(reinterpret_cast<const char16* const>(storagePath->Data())));
					}
				}
			}
		}
		catch (Platform::Exception^ e)
		{
			Assertf(false, "Failed to get persistent storage location. Exception 0x%08x, Msg: %ls",
				e->HResult,
				e->Message != nullptr ? e->Message->Data() : L"NULL");
		}
	}

	int stringLength = istrlen(path);
	// ignore '\\??\\' from the start of the path when returning it
	// not everything copes with it yet, but just starting from T: works fine and works consistently with everything
	// find first 'T' as the question marks may change in the future, but the drive letter won't
	for (int i = 0; i < stringLength; ++i)
	{
		if (path[i] == 'T' || path[i] == 't')
		{
			return &path[i];
		}
	}

	// don't return '\\??\\'
	return path;
}

} // namespace rage

#endif // RSG_DURANGO
