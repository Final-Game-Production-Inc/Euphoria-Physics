// 
// system/service_orbis.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#if RSG_ORBIS

#include "service.h"

#include "data/base64.h"
#include "string/string.h"
#include "system/appcontent.h"
#include "system/memops.h"
#include "system/param.h"

#include <system_service.h>
#include <sdk_version.h>
#include <np.h>

#include "service_orbis.h"
#pragma comment (lib, "libSceSystemService_stub_weak.a")

namespace rage 
{

PARAM(npTarget, "Sets the NP target for boot invitation/session events");
PARAM(npInvitationId, "Creates a serviceEvent on boot with the given invitation ID");
PARAM(npSessionId, "Creates a serviceEvent on boot with the given Session ID");

static SceSystemServiceStatus s_Status;

void sysService::InitClass() 
{
	sysMemSet(&s_Status, 0, sizeof(s_Status));
}

void sysService::ShutdownClass() 
{

}

void sysService::UpdateClass()
{
	// This is an event from newer SDKs. Triggered when you buy an item (shark card) in the Store
	#define SCE_SYSTEM_SERVICE_EVENT_SERVICE_ENTITLEMENT_UPDATE 0x1000000e

	int ret = sceSystemServiceGetStatus(&s_Status);
	if (ret == SCE_OK)
	{

#if !__FINAL
		ProcessBootEvents();
#endif

		int numEvents = s_Status.eventNum;
		for  (unsigned int i = 0; i < numEvents; i++) 
		{
			SceSystemServiceEvent event;
			sceSystemServiceReceiveEvent(&event);

			Displayf("SceSystemServiceEvent received :: Type: 0x%08x", event.eventType);

			switch (static_cast<unsigned>(event.eventType))
			{
			case SCE_SYSTEM_SERVICE_EVENT_SESSION_INVITATION:
				{
					sysOrbisServiceEvent e(sysServiceEvent::INVITATION_RECEIVED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_ON_RESUME:
				{
					sysOrbisServiceEvent e(sysServiceEvent::GAME_RESUMED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_GAME_LIVE_STREAMING_STATUS_UPDATE:
				{
					sysOrbisServiceEvent e(sysServiceEvent::LIVESTREAM_UPDATED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_GAME_CUSTOM_DATA:
				{
					sysOrbisServiceEvent e(sysServiceEvent::CUSTOM_GAMEDATA, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_DISPLAY_SAFE_AREA_UPDATE:
				{
					sysOrbisServiceEvent e(sysServiceEvent::SAFEAREA_UPDATED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_URL_OPEN:
				{
					sysOrbisServiceEvent e(sysServiceEvent::URL_OPENED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_LAUNCH_APP:
				{
					char args[MAX_ARG_LENGTH];
					safecpy(args, (char*)event.data.launchApp.arg);

					Displayf("SceSystemServiceEvent received :: SCE_SYSTEM_SERVICE_EVENT_LAUNCH_APP - Args: %s", args);

					if(datBase64::IsValidBase64String(args))
					{
						// we need to decode this to get the real args
						unsigned char decodedArgs[MAX_ARG_LENGTH];
						unsigned decodedLen = 0;
						datBase64::Decode(args, MAX_ARG_LENGTH, decodedArgs, &decodedLen);
						SetArgs((char*)decodedArgs);
					}
					else
					{
						SetArgs(args);
					}

					sysOrbisServiceEvent e(sysServiceEvent::APP_LAUNCHED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_APP_LAUNCH_LINK:
				{
					SetArgs((char*)event.data.appLaunchLink.arg);
					sysOrbisServiceEvent e(sysServiceEvent::APP_LAUNCHED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE:
				{
					sysOrbisServiceEvent e(sysServiceEvent::ENTITLEMENT_UPDATED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_PLAY_TOGETHER_HOST:
				{
					sysOrbisServiceEvent e(sysServiceEvent::PLAYTOGETHER_HOST, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			case SCE_SYSTEM_SERVICE_EVENT_SERVICE_ENTITLEMENT_UPDATE:
				{
					sysOrbisServiceEvent e(sysServiceEvent::SERVICE_ENTITLEMENT_UPDATED, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			default:
				{
					Assertf(false, "Unknown Event %d, hook this up to an existing event or create a new event!", event.eventType);
					sysOrbisServiceEvent e(sysServiceEvent::UNKNOWN, event);
					m_Delegator.Dispatch(&e);
				}
				break;
			}

		}
	}
}

bool sysService::IsUiOverlaid()
{
	return s_Status.isSystemUiOverlaid;
}

bool sysService::IsInBackgroundExecution()
{
	return s_Status.isInBackgroundExecution;
}

bool sysService::IsConstrained() const
{
	// scripters call these functions on all platforms and don't want to wrap them with platform specific checks - don't assert
	//Assertf(false, "sysService::IsConstrained() is not supported on this platform");
	return false;
}

u32 sysService::GetConstrainedDurationMs() const
{
	// scripters call these functions on all platforms and don't want to wrap them with platform specific checks - don't assert
	return 0;
}

bool sysService::HasResumedFromSuspend() const
{
	return false;
}

bool sysService::SetMusicPlayerEnabled(bool bEnable)
{
	int ret = SCE_OK;

	if (bEnable)
	{
		ret = sceSystemServiceReenableMusicPlayer();
	}
	else
	{
		ret = sceSystemServiceDisableMusicPlayer();
	}

	return ret == SCE_OK;
}

void sysService::HideSplashScreen()
{
	sceSystemServiceHideSplashScreen();
}

sysLanguage sysService::GetSystemLanguage()
{
	int language;
	if(Verifyf(sceSystemServiceParamGetInt( SCE_SYSTEM_SERVICE_PARAM_ID_LANG, &language) == SCE_OK, "Failed to retrieve language!"))
	{
		// For Japanese builds we only allow English and Japanese (url:bugstar:2031959).

		if(sysAppContent::IsJapaneseBuild())
		{
			if(language == SCE_SYSTEM_PARAM_LANG_JAPANESE)
			{
				return LANGUAGE_JAPANESE;
			}
			else
			{
				return LANGUAGE_ENGLISH;
			}
		}
		else if(sysAppContent::IsEuropeanBuild())
		{
			switch(language)
			{
			case SCE_SYSTEM_PARAM_LANG_ENGLISH_US:
			case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:
				return LANGUAGE_ENGLISH;

			case SCE_SYSTEM_PARAM_LANG_FRENCH:
				return LANGUAGE_FRENCH;

			case SCE_SYSTEM_PARAM_LANG_SPANISH:
				return LANGUAGE_SPANISH;

			case SCE_SYSTEM_PARAM_LANG_GERMAN:
				return LANGUAGE_GERMAN;

			case SCE_SYSTEM_PARAM_LANG_ITALIAN:
				return LANGUAGE_ITALIAN;
		
			case SCE_SYSTEM_PARAM_LANG_POLISH:
				return LANGUAGE_POLISH;

			case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
			case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
				return LANGUAGE_PORTUGUESE;

			case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
				return LANGUAGE_RUSSIAN;

			default:
				return LANGUAGE_UNDEFINED;
			}
		}
		else if(sysAppContent::IsAmericanBuild())
		{
			switch(language)
			{
			case SCE_SYSTEM_PARAM_LANG_ENGLISH_US:
			case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:
				return LANGUAGE_ENGLISH;

			case SCE_SYSTEM_PARAM_LANG_FRENCH:
				return LANGUAGE_FRENCH;

			case SCE_SYSTEM_PARAM_LANG_SPANISH:
			case SCE_SYSTEM_PARAM_LANG_SPANISH_LA:
				return LANGUAGE_MEXICAN;

			case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
			case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
				return LANGUAGE_PORTUGUESE;

			case SCE_SYSTEM_PARAM_LANG_KOREAN:
				return LANGUAGE_KOREAN;

			case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
			case SCE_SYSTEM_PARAM_LANG_CHINESE_S: // Not supported on consoles, boot to traditional, as requested in B*3778437
				return LANGUAGE_CHINESE_TRADITIONAL;

			default:
				return LANGUAGE_UNDEFINED;
			}
		}
	}

	return LANGUAGE_UNDEFINED;
}

void sysService::ShowSafeAreaSettings()
{
	sceSystemServiceShowDisplaySafeAreaSettings();
}

bool sysService::TriggerEvent(sysServiceEvent* UNUSED_PARAM(serviceEvent))
{
	Assertf(false, "sysService::TriggerEvent() is not supported on this platform");
	return false;
}

// many events aren't supported by the orbis SDK yet, so we fake it...but we still want it to flow through here
// mostly made for SCE_SYSTEM_SERVICE_EVENT_DISPLAY_SAFE_AREA_UPDATE  ... see on forum...
// https://ps4.scedev.net/forums/thread/16426/
void sysService::FakeEvent(sysServiceEvent::Type eventType)
{
	SceSystemServiceEvent event;
	sysOrbisServiceEvent e(eventType, event);
	m_Delegator.Dispatch(&e);
}

#if !__FINAL
void sysService::ProcessBootEvents()
{
	static bool hasDoneEvent = false;
	if (!hasDoneEvent)
	{
		const char* session;
		const char* invitation;
		const char* npTarget;

		// Requires an NP target
		if (PARAM_npTarget.Get(npTarget))
		{
			// If an invitation ID is present
			if (PARAM_npInvitationId.Get(invitation))
			{
				SceSystemServiceEvent event;
				event.eventType = SCE_SYSTEM_SERVICE_EVENT_SESSION_INVITATION;

				SceNpSessionInvitationEventParam invitationParam;
				sysMemSet(&invitationParam, 0, sizeof(invitationParam));

				invitationParam.flag =  SCE_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION;
				safecpy(invitationParam.onlineId.data, npTarget, sizeof(invitationParam.onlineId.data) + 1);
				safecpy(invitationParam.invitationId.data, invitation, sizeof(invitationParam.invitationId.data) + 1);

				sysMemCpy(event.data.param,&invitationParam,sizeof(invitationParam));

				sysOrbisServiceEvent e(sysServiceEvent::INVITATION_RECEIVED, event);
				m_Delegator.Dispatch(&e);
			}

			// If a session ID is present
			if (PARAM_npSessionId.Get(session))
			{
				SceSystemServiceEvent event;
				event.eventType = SCE_SYSTEM_SERVICE_EVENT_SESSION_INVITATION;

				SceNpSessionInvitationEventParam invitationParam;
				sysMemSet(&invitationParam, 0, sizeof(invitationParam));

				invitationParam.flag = 0; // SCE_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION;
				safecpy(invitationParam.onlineId.data, npTarget, sizeof(invitationParam.onlineId.data) + 1);
				safecpy(invitationParam.sessionId.data, session, sizeof(invitationParam.sessionId.data) + 1);

				sysMemCpy(event.data.param,&invitationParam,sizeof(invitationParam));

				sysOrbisServiceEvent e(sysServiceEvent::INVITATION_RECEIVED, event);
				m_Delegator.Dispatch(&e);
			}
		}

		hasDoneEvent = true;
	}
}
#endif

} // namespace rage

#endif //RSG_ORBIS

