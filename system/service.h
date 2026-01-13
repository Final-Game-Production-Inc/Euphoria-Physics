// 
// system/service.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SERVICE_H
#define SYSTEM_SERVICE_H

// This class will initially serve as a wrapper for the PS4 SystemService library,
// but can be extended to support other platforms. Ideally I'd like to see all of
// the ps3/360 randomly spread throughout game code to be housed in this class as well.
// For now, these PS4 SystemService events include networking, UI, system and graphics events, 
// so need to be handled in a common system available to all RAGE projects.
#include "file/file_config.h"

// Rage Headers.
#include "atl/delegate.h"
#include "system/ipc.h"
#include "system/language.h"
#include "system/serviceargs.h"

#if __STEAM_BUILD
#pragma warning(disable: 4265)
#include "../../3rdParty/Steam/public/steam/steam_api.h"
#pragma warning(error: 4265)
#endif

namespace rage 
{
enum sysLanguage;

#if RSG_DURANGO
	class sysDurangoServiceEvent;
#endif

// PURPOSE:	An event that can be raised by sysService.
// NOTES:	Events that originate from rage will have the level set to RAGE_EVENT.
//			Events that originate from the platform/system will have the level
//			set to SYSTEM_EVENT. System events will be an instance of a sub type
//			which may contain extra information. An event with a RAGE_EVENT level
//			will only contain the type of event and no extra information.
class sysServiceEvent
{
public:
	// PURPOSE: The level that the event came from.
	// NOTES:	SYSTEM_EVENTs will come from the host platform and can be
	//			casted to a platform specific event to gain extra (platform specific)
	//			information.
	enum Level
	{
		// PURPOSE:	Represents an event that originated from the system.
		SYSTEM_EVENT = 0,

		// PURPOSE: Represents an event that originated from rage (or the game).
		RAGE_EVENT,
	};

	// PURPOSE:	The type of event that has occurred.
	enum Type
	{
		UNKNOWN = 0,
		EXITING,
		SUSPENDED, // NOTE: On some platforms, this event might not be received as we are suspended.
		SUSPEND_IMMEDIATE, // Called immediately on suspend, not deferred
		RESUMING,
		RESUME_IMMEDIATE, // Called immediately on resume, not deferred
		CONSTRAINED,
		UNCONSTRAINED,
		VISIBILITY_LOST,
		VISIBILITY_GAINED,
		FOCUS_LOST,
		FOCUS_LOST_IMMEDIATE, // Called immediately on focus lost, not deferred
		FOCUS_GAINED,
		FOCUS_GAINED_IMMEDIATE, // Called immediately on focus gained, not deferred
		VISIBLE_SIZE_CHANGED,
		INPUT_FOCUS_LOST,
		INPUT_FOCUS_GAINED,
		ENTITLEMENT_UPDATED,
		SERVICE_ENTITLEMENT_UPDATED,

		// Media Events
		BACK,
		PAUSE, // Could also be used to pause the game.
		PLAY,  // Could also be used to un-pause the game.
		MENU,
		VIEW,

		// PS4 Specific
		INVITATION_RECEIVED,
		GAME_RESUMED,
		LIVESTREAM_UPDATED,
		CUSTOM_GAMEDATA,
		SAFEAREA_UPDATED,
		URL_OPENED,
		APP_LAUNCHED,
		PLAYTOGETHER_HOST,
		
		// MAX
		MAX_SERVICE_EVENTS
	};

	// PURPOSE: Creates a new event.
	// PARAMS:	eventType - the type of event to create.
	// NOTES:	Events created with this constructor WILL have a RAGE_EVENT level!
	sysServiceEvent(Type eventType);
	virtual ~sysServiceEvent();

	// PURPOSE: Retrieves the level the event comes from.
	Level GetLevel() const;

	// PURPOSE: Retrieves the type of event.
	Type  GetType() const;

#if !__NO_OUTPUT
	// PURPOSE: Gets the debug display string of the type
	const char * GetDebugName();
#endif

protected:
	// PURPOSE: Creates a new event.
	// PARAMS:	level - the level of the event.
	//			eventType - the type of event to create.
	// NOTES:	This constructor is protected as a SYSTEM_EVENT level can only be
	//			created by a sub type.
	sysServiceEvent(Level level, Type eventType);

private:
	Level m_Level;
	Type  m_Type;
};


typedef atDelegator<void (sysServiceEvent* event)> ServiceDelegator;
typedef ServiceDelegator::Delegate ServiceDelegate;

class sysService 
{
public:

	// PURPOSE:	The maximum arguments length from the system (including NULL terminator).
	static const u32 MAX_ARG_LENGTH = 256u;

#if __STEAM_BUILD
	sysService();
#endif

	// PURPOSE:	System-specific startup; invoked by startup code
	void InitClass();

	// PURPOSE:	System-specific shutdown
	void ShutdownClass();

	// PURPOSE: System-specific update
	void UpdateClass();

	// PURPOSE: Returns true if the system UI is overlaid
	bool IsUiOverlaid();

	// PURPOSE: Returns true if the software is running in the background.
	bool IsInBackgroundExecution();

	// PURPOSE: Returns true if the software is executing with reduced CPU/GPU allocation.
	bool IsConstrained() const;

	// PURPOSE: Returns the amount of time in ms that we've been in constrained mode. Should only be called when IsConstrained() returns true.
	u32 GetConstrainedDurationMs() const;

	// PURPOSE: Returns true the frame we resume from a suspend.
	bool HasResumedFromSuspend() const;

	// PURPOSE: Enable/Disable the system Music player
	// NOTES:	Depending upon the platform, this might not be controllable
	//			by the game.
	bool SetMusicPlayerEnabled(bool bEnable);

	// PURPOSE: Stop display of the system startup image
	// NOTES:	On some platforms this is controlled by the system.
	void HideSplashScreen();

	// PURPOSE: Get System Language id.
	// NOTES:	Returns LANGUAGE_UNDEFINED on failure to retrieve language.
	sysLanguage GetSystemLanguage();

	// PURPOSE: Get ISO 3166 two-character country code (+ null terminator).
	// NOTES:	Returns false on failure to retrieve country.
	bool GetSystemCountry(char (&countryCode)[3]);

	// PURPOSE: Show the safe area settings
	void ShowSafeAreaSettings();

	// PURPOSE: Disables screen dimming for one frame if the platform OS supports it.
	void DisableScreenDimmingThisFrame();

	// PURPOSE:	Indicates if screen dimming is enabled or not.
	bool IsScreenDimmingDisabledThisFrame() const;

	// Delegates
	void AddDelegate(ServiceDelegate* dlgt);
	void RemoveDelegate(ServiceDelegate* dlgt);

	// PURPOSE:	Triggers an Event to be fired.
	// PARAMS:	serviceEvent - the event to be sent.
	// RETURNS:	true on success.
	// NOTES:	The event will be sent immediately.
	bool TriggerEvent(sysServiceEvent* serviceEvent);

    // PURPOSE: Set new launch args
    // PARAMS: the args in the format "argkeya=valX argkeyb=valY"
    void SetArgs(const char* args);

    // PURPOSE: Returns true if an arg is set
    bool HasArgs() const;

    // PURPOSE: See if the args contain the specified param
    bool HasParam(const char* param) const;

    // PURPOSE: Clears the launch args
    void ClearArgs();

    //PURPOSE: Returns the args string
    const char* GetArgs() const;

    //PURPOSE: Returns the launch args
    sysServiceArgs& Args();

#if RSG_ORBIS
	// PURPOSE: Fakes an event occuring
	// NOTES: many events aren't supported by the orbis SDK yet, so we fake it...but we still want it to flow through here
	// https://ps4.scedev.net/forums/thread/16426/
	void FakeEvent(sysServiceEvent::Type eventType);

#if !__FINAL
	//PURPOSE
	// Process bootable service events (debug only)
	void ProcessBootEvents();
#endif
#endif

#if RSG_DURANGO
	// PURPOSE:	Defer an Event to be fired at the next call to DispatchDeferredEvents.
	// PARAMS:	serviceEvent - the event to be sent.
	void DeferEvent(sysDurangoServiceEvent* serviceEvent);

	// PURPOSE:	Process all events deferred by DeferEvent.
	void DispatchDeferredEvents();

	// PURPOSE: The root path for persistent storage on Durango
	const char* GetPersistentStorageRoot();
#endif

#if __STEAM_BUILD
	STEAM_CALLBACK( sysService, OnGameOverlayCallback, GameOverlayActivated_t, m_callResultGameOverlay);
	bool m_SteamOverlayActive;
	bool m_ExecutedSteamCallbacks;
#endif

private:
	ServiceDelegator m_Delegator;

	// PURPOSE: Contains the current launch arguments
	sysServiceArgs m_Args;

	// TODO: This will likely need hooking up on other platforms.
#if RSG_DURANGO
	// PURPOSE: Indicates that screen dimming is enabled.
	bool m_ScreenDimmingDisabledThisFrame;

	// PURPOSE: Indicates that screen dimming was enabled last frame.
	bool m_ScreenDimmingDisabledLastFrame;

	// PURPOSE: Indicates we have resumed from a suspend this frame.
	bool m_HasResumedFromSuspendThisFrame;

	// PURPOSE: Indicates we have resumed from a suspend last frame.
	bool m_HasResumedFromSuspendLastFrame;

	// PURPOSE: Keeps track of which thread should update the resumed from suspend flag.
	sysIpcCurrentThreadId m_ThreadId;
#endif // RSG_DURANGO
};

extern sysService g_SysService;

inline sysServiceEvent::sysServiceEvent(Type eventType)
	: m_Level(RAGE_EVENT)
	, m_Type(eventType)
{}

inline sysServiceEvent::sysServiceEvent(Level level, Type eventType)
	: m_Level(level)
	, m_Type(eventType)
{}

inline sysServiceEvent::~sysServiceEvent()
{}

inline sysServiceEvent::Level sysServiceEvent::GetLevel() const
{
	return m_Level;
}

inline sysServiceEvent::Type sysServiceEvent::GetType() const
{
	return m_Type;
}

inline void sysService::DisableScreenDimmingThisFrame()
{
#if RSG_DURANGO
	// NOTE on durango the screen dimming is done inside applicationview_durango.winrt.cpp due to the WinRT agile setting. This is because
	// this is a new feature that MS have added for us and they won't fix the agile issue before submission.
	m_ScreenDimmingDisabledThisFrame = true;
#endif // RSG_DURANGO
}

inline bool sysService::IsScreenDimmingDisabledThisFrame() const
{
#if RSG_DURANGO
	return m_ScreenDimmingDisabledThisFrame || m_ScreenDimmingDisabledLastFrame;
#else
	return false;
#endif // RSG_DURANGO
}

}	// namespace rage

#endif
