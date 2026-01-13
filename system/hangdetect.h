#ifndef SYSTEM_HANGDETECT_H
#define SYSTEM_HANGDETECT_H

#define HANG_DETECT_THREAD	(!__RESOURCECOMPILER && !__TOOL && !__GAMETOOL && (RSG_PC || RSG_ORBIS || !__FINAL))



#define DETECT_SUSPEND_TIMER_FAILURE ((1 && RSG_DURANGO) && HANG_DETECT_THREAD && !__FINAL)

#if DETECT_SUSPEND_TIMER_FAILURE
#	define DETECT_SUSPEND_TIMER_FAILURE_ONLY(...) __VA_ARGS__
#else
#	define DETECT_SUSPEND_TIMER_FAILURE_ONLY(x)
#endif // DETECT_SUSPEND_TIMER_FAILURE

namespace rage {

#if HANG_DETECT_THREAD
	void sysHangDetectSaveZoneEnter();
	void sysHangDetectSaveZoneExit(const char *str);
	void sysHangDetectTick();
	void sysHangDetectExit();
	void sysHangDetectStartThread();

#if DETECT_SUSPEND_TIMER_FAILURE
	// PURPOSE:	Stops the suspend failure detection timer.
	// NOTES:	This *MUST* be called the moment before we actaully suspend!.
	void sysSuspendFailureDetectStopTimer();
#endif // DETECT_SUSPEND_TIMER_FAILURE

	// Set the function called when the hang detect thread needs to crash the game. This should usually be a function
	// which causes a crash, such as a __debugbreak() call or *(volatile size_t*)0 = 0;
	// If NULL is passed here, no function will be called.
	// If the function returns without crashing the game, or the function pointer is NULL, the hang detect thread will
	// crash the game.
	// Returns the previous hang detect function
	typedef void (*HangDetectCrashFunc)();
	HangDetectCrashFunc sysHangDetectSetCrashFunc(HangDetectCrashFunc func);

#	define HANG_DETECT_SAVEZONE_ENTER()	sysHangDetectSaveZoneEnter()
#	define HANG_DETECT_SAVEZONE_EXIT(STR)	sysHangDetectSaveZoneExit(STR)
#	define HANG_DETECT_TICK()				sysHangDetectTick()
#	define HANG_DETECT_EXIT()				sysHangDetectExit()

	extern bool sysHangDetected;
	extern u32 sysHangDetectCountdown;

#define AUTO_HANG_DETECT_CRASH_ZONE \
	class sysHangDetectCrashZoner \
	{ \
	public: \
		sysHangDetectCrashZoner() { m_prevFunc = sysHangDetectSetCrashFunc(CrashFunc); } \
		~sysHangDetectCrashZoner() { sysHangDetectSetCrashFunc(m_prevFunc); } \
	private: \
		static void CrashFunc() { *(volatile size_t*)0 = (size_t)__LINE__; } \
		HangDetectCrashFunc m_prevFunc; \
	}; sysHangDetectCrashZoner __hangDetectCrashZone ## __LINE__

#else
#	define HANG_DETECT_SAVEZONE_ENTER()
#	define HANG_DETECT_SAVEZONE_EXIT(STR)
#	define HANG_DETECT_TICK()
#	define HANG_DETECT_EXIT()

#	define sysHangDetected false
#	define AUTO_HANG_DETECT_CRASH_ZONE

#endif
}

#endif	// SYSTEM_HANGDETECT_H
