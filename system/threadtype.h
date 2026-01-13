#ifndef SYSTEM_THREADTYPE_H_
#define SYSTEM_THREADTYPE_H_

#include "system/bit.h"
#include "system/tls.h"

namespace rage {

/* PURPOSE: The thread type identifies the purpose (type) of the current thread via a TLS
 * bitmask. A thread can be tagged with any number of types. The common ones are update and
 * render threads, but applications are free to add their own types by defining them like
 * USER_DEFINED_TYPE = BIT0 << USER_THREAD_TYPE_SHIFT.
 */
namespace sysThreadType
{
	enum {
		THREAD_TYPE_UPDATE			= BIT0,	// The main update thread
		THREAD_TYPE_RENDER			= BIT1,	// The render thread
		THREAD_TYPE_PROCESS_MAIN	= BIT2,	// The thread that called main()
		THREAD_TYPE_DEPENDENCY		= BIT3, // sysDependency worker thread

		USER_THREAD_TYPE_SHIFT		= 4,	// Any user thread IDs should be shifted left
											// by this value
	};

	extern bool g_IsMainThreadInitted;
	extern __THREAD int g_ThisThreadType;

	// Add a bitmask of types to the existing ones of the current thread.
	void AddCurrentThreadType(int threadtype);

	// Remove a bitmask of types from the current thread.
	void ClearCurrentThreadType(int threadtype);

	// RETURNS: The bitmask of types o the current thread.
	inline int GetCurrentThreadType()	{  return g_ThisThreadType; }

	inline bool IsBeforeMainThreadInit() { return !g_IsMainThreadInitted; }

	// RETURNS: TRUE if the current thread has the update type set, false otherwise.
	inline bool IsUpdateThread()		{ return (GetCurrentThreadType() & THREAD_TYPE_UPDATE) != 0; }

	// RETURNS: TRUE if the current thread has the render type set, false otherwise.
	inline bool IsRenderThread()		{ return (GetCurrentThreadType() & THREAD_TYPE_RENDER) != 0; }

	// RETURNS: TRUE if the current thread has the main type set, false otherwise.
	inline bool IsProcessMainThread()			{ return (GetCurrentThreadType() & THREAD_TYPE_PROCESS_MAIN) != 0; }

}	// namespace sysThreadType

}	// namespace rage

#endif // SYSTEM_THREADTYPE_H_
