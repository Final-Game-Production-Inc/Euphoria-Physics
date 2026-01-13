#ifndef GCM_TRACE_H
#define GCM_TRACE_H

#include "profile/trace.h"

#if __PPU && RAGETRACE

namespace rage {

class pfGcmTrace : public pfTrace
{
public:
	pfGcmTrace();

	u32		GetNextSample();
	void	AdvanceTo(u32 timebase);

	void	StartFrame();
	void	Start(pfTraceCounterId* identifier);
	void	Stop();

private:
	pfTraceCounterId	m_RootId;
	bool				m_GotNextTime;
	u64					m_NextTime;
	pfTraceCounterId*	m_GcmCounters[2048];
	u32					m_ReadTimeStamp;
	u32					m_WriteTimeStamp;

	u32					m_ReadFrameTime;
	u32					m_WriteFrameTime;
	u32					m_FrameTime[8]; // ticks value
	u32					m_FrameTimeStart[8]; // gcm counter no
	u32					m_BaseCpuTime;
	u64					m_BaseGpuTime;
};

extern pfGcmTrace* g_pfGcmTrace;

class pfGcmTraceScope
{
public:
	pfGcmTraceScope(pfTraceCounterId* id) 
	{
		if (g_pfGcmTrace)
			g_pfGcmTrace->Start(id);
	}
	~pfGcmTraceScope() 
	{
		if (g_pfGcmTrace)
			g_pfGcmTrace->Stop();
	}
};

} // namespace rage

#define RAGETRACE_GPU_INITFRAME() if (::rage::g_pfGcmTrace) ::rage::g_pfGcmTrace->StartFrame()
#define _RAGETRACE_GPU_PUSH(id) if (::rage::g_pfGcmTrace) ::rage::g_pfGcmTrace->Start(&id)
#define _RAGETRACE_GPU_SCOPE(id) ::rage::pfGcmTraceScope MacroJoin(_ragetrace_,__LINE)__(&id)
#define RAGETRACE_GPU_POP() if (::rage::g_pfGcmTrace) ::rage::g_pfGcmTrace->Stop()

#else

#define RAGETRACE_GPU_INITFRAME()
#define _RAGETRACE_GPU_PUSH(id)
#define _RAGETRACE_GPU_SCOPE(id)
#define RAGETRACE_GPU_POP()

#endif // RAGETRACE

#define RAGETRACE_GPU_PUSH(id) _RAGETRACE_GPU_PUSH(_ragetrace_##id)
#define RAGETRACE_GPU_SCOPE(id) _RAGETRACE_GPU_SCOPE(_ragetrace_##id)
#define RAGETRACE_GPU_PUSHNAME(name) _RAGETRACE_GPU_PUSH(::rage::RageTrace().IdFromName(name))
#define RAGETRACE_GPU_SCOPENAME(name) _RAGETRACE_GPU_SCOPE(::rage::RageTrace().IdFromName(name))

#endif // GCM_TRACE_H
