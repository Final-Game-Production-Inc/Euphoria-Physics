//
// data/marker.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "marker.h"
#include "profile/telemetry.h"
#include "system/tls.h"
#include "system/interlocked.h"
#include "system/timer.h"
#include "system/memory.h"
#include "system/param.h"

#include <string.h>

PARAM(measure,"[system] What to measure with rage markers: ticks, memory, or nothing");

__THREAD int RAGE_LOG_DISABLE = RSG_ORBIS;

#if RAGE_USE_DEJA

#if __XENON
#pragma comment(lib,"DejaLib.X360.lib")
#elif __WIN32PC
#pragma comment(lib,"DejaLib.Win32.lib")
#elif __PS3
#pragma comment(lib,"DejaLib.PS3")
#endif

void RAGE_LOG_NEW(const void *ptr,size_t size,const char *file,int line)
{
	if (!RAGE_LOG_DISABLE && (!::rage::sysMemAllocator::IsCurrentSet() || ::rage::sysMemAllocator::GetCurrent().IsTallied()))
	{
		DEJA_LOG_NEW(const_cast<void*>(ptr),size,file,line);
		TELEMETRY_ALLOC(ptr, size, file, line, "");
	}
}

void RAGE_LOG_DELETE(const void *ptr)
{
	if (!RAGE_LOG_DISABLE && ptr && (!::rage::sysMemAllocator::IsCurrentSet() || ::rage::sysMemAllocator::GetCurrent().IsTallied()))
	{
		DEJA_LOG_DELETE(const_cast<void*>(ptr));
		TELEMETRY_FREE(ptr, __FILE__, __LINE__);
	}
}

#elif RAGE_MEMORY_DEBUG

void RAGE_LOG_NEW(const void*,size_t,const char*,int)
{
}

void RAGE_LOG_DELETE(const void*)
{
}

#endif

namespace rage {

#if RAGE_ENABLE_MARKERS && !RAGE_USE_DEJA

Context *Context::First;

const int MaxContext = 8;
__THREAD Context* RageContextStack[MaxContext];		// 32 bytes
__THREAD u32 RageContextValues[MaxContext];			// 32 bytes
__THREAD int RageContextStackCount;					// 4 bytes
													// Incurs 68 bytes per thread overhead.  This is small enough to more or less ignore, 
													// and is easier than getting dynamic memory allocation involved.

void Marker::Push(Context &tag) {
	// If tag hasn't been seen before, register it in the global list (atomically, in case multiple threads hit this at the same time)
	// (Well, atomic with regard to other Context objects; in theory we could be attempting to add the same Context in two different
	// threads at the exact same time, not sure if this ever happens in practice)
	if (tag.Next == (Context*)-1)
			tag.Next = (Context*) sysInterlockedExchangePointer((void**)(void*)&Context::First, &tag);

	if (RageContextStackCount < MaxContext) {
		RageContextStack[RageContextStackCount] = &tag;
		RageContextValues[RageContextStackCount] = g_RageMeasureValue();
	}
	RageContextStackCount++;
}

void Marker::Pop() {
	--RageContextStackCount;
	if (RageContextStackCount < MaxContext) {
		s32 elapsed = s32(g_RageMeasureValue() - RageContextValues[RageContextStackCount]);
		RageContextStack[RageContextStackCount]->Value += elapsed;
	}
}


u32 _rage_nothing_measure(void) {
	return 0;
}

u32 _rage_timing_measure(void) {
	return (u32) sysTimer::GetTicks();
}

u32 _rage_memory_measure(void) {
	return (u32) sysMemGetMemoryUsed();
}

u32 _rage_basic_measure(void) {
	const char *value;
	if (!PARAM_measure.Get(value) || !strcmp(value,"ticks"))
		g_RageMeasureValue = _rage_timing_measure;
	else if (PARAM_measure.Get(value) && !strcmp(value,"memory"))
		g_RageMeasureValue = _rage_memory_measure;
	else
		g_RageMeasureValue = _rage_nothing_measure;
	return (g_RageMeasureValue)();
}

u32 (*g_RageMeasureValue)(void) = _rage_basic_measure;

#endif

} // namespace rage
