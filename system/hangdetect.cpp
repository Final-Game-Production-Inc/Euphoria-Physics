#include "hangdetect.h"
#include "exception.h"
#include "interlocked.h"
#include "ipc.h"
#include "nelem.h"
#include "new.h"
#include "timer.h"
#include "threadtype.h"
#include "tmcommands.h"
#include "service.h"
#include "stack.h"
#include "threadtrace.h"

#include "diag/channel.h"

namespace rage {

#if HANG_DETECT_THREAD

// Useful debug info to add if there is an overflow of the hang detect context stack
#define HANG_DETECT_ENTER_SAVE_CALLSTACK    (0)

bool sysHangDetected = false;	
u32 sysHangDetectCountdown = RSG_PC ? 60 : RSG_ORBIS ? 29 : 30;

static sysIpcThreadId			s_HangDetectThreadId = sysIpcThreadIdInvalid;
static utimer_t					s_TimeStamp = 0;
static volatile bool			s_HangDetectThreadExit = false;
static volatile u32             s_HangDetectSaveZoneCount = 0;
static HangDetectCrashFunc		s_HangDetectCrashFunc = NULL;


#if DETECT_SUSPEND_TIMER_FAILURE
PARAM(detectPLMSupendHang, "[Hang Detection] Detects any PLM suspend timer failures (this will ONLY work when running the game with debug flag on Xbox One).");

const u32						HANG_DETECT_SUSPEND_TIME_LIMIT = 1000;
static volatile u32				s_HangDetectSuspendStartTime = 0;


void sysSuspendFailureDetectStopTimer()
{
	// Only reset the timer if we are in the time limit. If not, let it continue so we detect the failure and create a crash dump file!.
	const u32 timeUntilSuspend = sysTimer::GetSystemMsTime() - s_HangDetectSuspendStartTime;
	if(timeUntilSuspend < HANG_DETECT_SUSPEND_TIME_LIMIT)
	{
		s_HangDetectSuspendStartTime = 0;
	}
}
#endif // DETECT_SUSPEND_TIMER_FAILURE

static ServiceDelegate			s_HangDetectSuspendDelegate;

void sysHangDetectOnServiceEvent(sysServiceEvent* evnt)
{
	if(evnt != NULL)
	{
		switch(evnt->GetType())
		{
		case sysServiceEvent::SUSPEND_IMMEDIATE:
			DETECT_SUSPEND_TIMER_FAILURE_ONLY(s_HangDetectSuspendStartTime = sysTimer::GetSystemMsTime());
			HANG_DETECT_SAVEZONE_ENTER();
			break;

		case sysServiceEvent::RESUME_IMMEDIATE:
			HANG_DETECT_SAVEZONE_EXIT(NULL);
			break;			

		default:
			break;
		}
	}
}

struct HangDetectSaveZoneCtx {
	utimer_t enterTime;
#if HANG_DETECT_ENTER_SAVE_CALLSTACK
	size_t callstack[16];
#endif
};

struct PerThreadHangDetectSaveZoneState {
	HangDetectSaveZoneCtx   m_Context[8];
	unsigned                m_Index; // NB: Relies on TLS default initializing to zero
};

static __THREAD PerThreadHangDetectSaveZoneState s_PerThreadState;

void HangDetectThread(void*) {
	utimer_t prevTimeStamp = 0xfffffff;
	u32 countdown = sysHangDetectCountdown;
	u32 countdownFrom = countdown;
	Displayf("Hang detect thread active");

	s_HangDetectSuspendDelegate.Bind(&sysHangDetectOnServiceEvent);
	g_SysService.AddDelegate(&s_HangDetectSuspendDelegate);

#	if DETECT_SUSPEND_TIMER_FAILURE
		bool isWithinSuspendTimeLimit = true;
		const bool detectPLMSuspendHang = PARAM_detectPLMSupendHang.Get();
#	endif // DETECT_SUSPEND_TIMER_FAILURE

	while (!s_HangDetectThreadExit EXCEPTION_HANDLING_ONLY(&& !sysException::HasBeenThrown())) {
		
		do {
#		if DETECT_SUSPEND_TIMER_FAILURE
			isWithinSuspendTimeLimit = true;

			// If we are suspending then don't sleep for a second as we need finer grained checking to detect
			// suspend time limit failures.
			if(detectPLMSuspendHang && s_HangDetectSuspendStartTime != 0)
			{
				sysIpcSleep(10);

				// NOTE: We test s_HangDetectSuspendStartTime != 0 again as this thread should have gone to sleep during the resume. We could already
				// be in the if above when s_HangDetectSuspendStartTime is set to 0 upon resume.
				if((sysTimer::GetSystemMsTime() - s_HangDetectSuspendStartTime) >= HANG_DETECT_SUSPEND_TIME_LIMIT && s_HangDetectSuspendStartTime != 0)
				{
					isWithinSuspendTimeLimit = false;
				}
			}
			else
#		endif // DETECT_SUSPEND_TIMER_FAILURE
			{
				sysIpcSleep(1000);
			}
		} while (s_HangDetectSaveZoneCount DETECT_SUSPEND_TIMER_FAILURE_ONLY(&& isWithinSuspendTimeLimit));



		if (DETECT_SUSPEND_TIMER_FAILURE_ONLY(isWithinSuspendTimeLimit &&) prevTimeStamp != s_TimeStamp) {
			sysHangDetected = false;
			if (countdown != countdownFrom) {
				OUTPUT_ONLY(u32 seconds = countdownFrom - countdown);
				Warningf("Game hung for %u second%s!!!", seconds, seconds>1?"s":"");
			}
			countdown = countdownFrom = sysHangDetectCountdown;
			prevTimeStamp = s_TimeStamp;
		}
		else if (DETECT_SUSPEND_TIMER_FAILURE_ONLY(isWithinSuspendTimeLimit &&) --countdown) {
			sysHangDetected = true;
			Warningf("Game hang countdown %d", countdown);
		}
		else
		{
			// Make sure output is enabled so that we actually print the exception
			// handler info.
			OUTPUT_ONLY(diagChannel::SetOutput(true);)

#		if DETECT_SUSPEND_TIMER_FAILURE
			if(isWithinSuspendTimeLimit == false)
			{
				Errorf("*** SUSPEND TIMER FAILURE - AUTOMATIC SHUTDOWN ***");
			}
			else
#		endif // DETECT_SUSPEND_TIMER_FAILURE
			{
				Errorf("*** GAME HANG - AUTOMATIC SHUTDOWN ***");
				Errorf("Current timestamp: %u", sysTimer::GetSystemMsTime());
			}
#			if SYSTEM_THREADTRACE_ENABLE
				Errorf("*** Thread stats for this frame ***");
				sysThreadTracePrintThreadStats();
#			endif
			Errorf("*** Detailed log follows ***");
			OUTPUT_ONLY(diagChannel::FlushLogFile());
			sysIpcTriggerAllCallstackDisplay();
			OUTPUT_ONLY(diagChannel::FlushLogFile());

			// Bring the current timestamp onto the stack so it can be inspected in minidumps
			volatile u32 currentTime = sysTimer::GetSystemMsTime();
			--currentTime;
			++currentTime;
			if(s_HangDetectCrashFunc)
				s_HangDetectCrashFunc();
#			if SYSTMCMD_ENABLE
				sysTmCmdCpuHang();
#			else
				*(volatile int*)0 = 0;
#			endif
		}
	}

	g_SysService.RemoveDelegate(&s_HangDetectSuspendDelegate);
	s_HangDetectSuspendDelegate.Unbind();
}

void sysHangDetectSaveZoneEnter() {
	sysInterlockedIncrement(&s_HangDetectSaveZoneCount);
	const unsigned idx = s_PerThreadState.m_Index;

	if (!(idx < NELEM(s_PerThreadState.m_Context)))
	{
		Quitf(-1, "idx (%d) < NELEM(s_PerThreadState.m_Context) (%d)", idx, NELEM(s_PerThreadState.m_Context));
	}

	s_PerThreadState.m_Index = idx + 1;
	s_PerThreadState.m_Context[idx].enterTime = sysTimer::GetTicks();
#if HANG_DETECT_ENTER_SAVE_CALLSTACK
	sysStack::CaptureStackTrace(s_PerThreadState.m_Context[idx].callstack, NELEM(s_PerThreadState.m_Context[idx].callstack));
#endif
}

void sysHangDetectSaveZoneExit(const char *str) {
	const unsigned idx1 = s_PerThreadState.m_Index;
	Assert(idx1);
	const unsigned idx0 = idx1-1;
	s_PerThreadState.m_Index = idx0;

	// Only print a warning if hang detection is enabled (ie, thread exists),
	// and input string is non-NULL (so caller can suppress warning for tools
	// etc).
	if (str && s_HangDetectThreadId!=sysIpcThreadIdInvalid) 
	{
		bool canProceedWithLogging = true;

		// Try to get lock of the diagLogf token to avoid deadlocks
#if !__FINAL 
		canProceedWithLogging = diagChannel::TryLockDiagLogfToken();
#endif // #if !__FINAL 

		if (canProceedWithLogging)
		{
			const float sec = (sysTimer::GetTicks()-s_PerThreadState.m_Context[idx0].enterTime) * sysTimer::GetTicksToSeconds();
			if (sec > 10.f) 
			{
				Warningf("Hang detect save zone %.3fs (\"%s\")", sec, str);
#			if SYSTEM_THREADTRACE_ENABLE
				Warningf("*** Thread stats for this frame ***");
				sysThreadTracePrintThreadStats();
#			endif
			}
#if !__FINAL 
			diagChannel::UnlockDiagLogfToken();
#endif // #if !__FINAL 			
		}		
	}

	sysInterlockedDecrement(&s_HangDetectSaveZoneCount);
}

// called from grcDevice::EndFrame
void sysHangDetectTick()
{
	sysThreadTraceBeginFrame();
	s_TimeStamp = sysTimer::GetTicks();
}

// called from grcSetup::SysutilCallback
void sysHangDetectExit()
{
	s_HangDetectThreadExit = true;
}

void sysHangDetectStartThread()
{
	s_HangDetectThreadId = sysIpcCreateThread(HangDetectThread,NULL,16*1024,PRIO_TIME_CRITICAL,"[RAGE] Hang Detect Thread");
}

HangDetectCrashFunc sysHangDetectSetCrashFunc(HangDetectCrashFunc func)
{
	// If this is called from a thread other than the main thread, do nothing rather than asserting. There are valid places
	// where some code is called from the main and render threads for instance, but we only care about the main thread.
	if(!sysThreadType::IsUpdateThread())
	{
		return NULL;
	}

	HangDetectCrashFunc prevFunc = s_HangDetectCrashFunc;
	s_HangDetectCrashFunc = func;
	return prevFunc;
}

#endif // HANG_DETECT_THREAD

}	// namespace rage
