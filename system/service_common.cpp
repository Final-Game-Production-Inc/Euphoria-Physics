// 
// system/service_common.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "service.h"
#include "system/memops.h"
#include "system/nelem.h"

namespace rage 
{

sysService g_SysService;

void sysService::AddDelegate(ServiceDelegate* dlgt)
{
	m_Delegator.AddDelegate(dlgt);
}

void sysService::RemoveDelegate(ServiceDelegate* dlgt)
{
	m_Delegator.RemoveDelegate(dlgt);
}

#if !__NO_OUTPUT
const char* sysServiceEvent::GetDebugName()
{
	static const char* s_ServiceStrings[] =
	{
		"UNKNOWN",						
		"EXITING",
		"SUSPENDED",
		"SUSPEND_IMMEDIATE",
		"RESUMING",
		"RESUME_IMMEDIATE",
		"CONSTRAINED",
		"UNCONSTRAINED",
		"VISIBILITY_LOST",
		"VISIBILITY_GAINED",
		"FOCUS_LOST",
		"FOCUS_LOST_IMMEDIATE",
		"FOCUS_GAINED",
		"FOCUS_GAINED_IMMEDIATE",
		"VISIBLE_SIZE_CHANGED",
		"INPUT_FOCUS_LOST",
		"INPUT_FOCUS_GAINED",
		"ENTITLEMENT_UPDATED",
		"SERVICE_ENTITLEMENT_UPDATED",

		// Media Events
		"BACK",
		"PAUSE",
		"PLAY",
		"MENU",
		"VIEW",

		// PS4 Specific
		"INVITATION_RECEIVED",
		"GAME_RESUMED",
		"LIVESTREAM_UPDATED",
		"CUSTOM_GAMEDATA",
		"SAFEAREA_UPDATED",
		"URL_OPENED",
		"APP_LAUNCHED",
		"PLAYTOGETHER_HOST",
	};
	CompileTimeAssert(COUNTOF(s_ServiceStrings) == MAX_SERVICE_EVENTS);

	return s_ServiceStrings[(int)m_Type];
}

#endif

void sysService::SetArgs(const char* args)
{
	m_Args.Set(args);
}

bool sysService::HasArgs() const
{
	return m_Args.HasArgs();
}

bool sysService::HasParam(const char* param) const
{
	return m_Args.HasParam(atHashString(param));
}
	
void sysService::ClearArgs()
{
	m_Args.Clear();
}
	
const char* sysService::GetArgs() const
{
	return m_Args.GetString();
}
	
sysServiceArgs& sysService::Args()
{
	return m_Args;
}

#if RSG_TOOL || (!RSG_PC && !RSG_ORBIS && ! RSG_DURANGO)

void sysService::InitClass() 
{

}

void sysService::ShutdownClass() 
{

}

void sysService::UpdateClass()
{
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
	// scripters call these functions on all platforms and don't want to wrap them with platform specific checks - don't assert
	//Assertf(false, "sysService::IsConstrained() is not supported on this platform");
	return false;
}

u32 sysService::GetConstrainedDurationMs() const
{
	// scripters call these functions on all platforms and don't want to wrap them with platform specific checks - don't assert
	//Assertf(false, "sysService::GetConstrainedDurationMs() is not supported on this platform");
	return 0;
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
	Assertf(false, "sysService::GetSystemLanguage() is not supported on this platform");
	return LANGUAGE_UNDEFINED;
}

bool sysService::TriggerEvent(sysServiceEvent* UNUSED_PARAM(serviceEvent))
{
	Assertf(false, "sysService::TriggerEvent() is not supported on this platform");
	return false;
}

#endif // #if ! __WIN32PC && !RSG_ORBIS && ! RSG_DURANGO

} // namespace rage


