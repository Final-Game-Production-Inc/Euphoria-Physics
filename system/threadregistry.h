//
// system/threadregistry.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_THREADREGISTRY_H
#define SYSTEM_THREADREGISTRY_H

#include "string/string.h"
#include "system/ipc.h"

#if defined(__RGSC_DLL) && __RGSC_DLL && RSG_CPU_X86
#define __RGSC_DLL_X86 1
#else
#define __RGSC_DLL_X86 0
#endif

#if __RGSC_DLL_X86
// TODO: NS - GetThreadId() doesn't exist on Windows XP
#define THREAD_REGISTRY	0
#else
#define THREAD_REGISTRY	1
#endif

#if THREAD_REGISTRY

#define THREAD_REGISTRY_ONLY(_x)	_x

namespace rage {

class bkGroup;


class sysThreadRegistry {
public:
	/* PURPOSE: Registers a thread with the thread registry, which will automatically add a RAG widget for it
	 * and allows for changing its priority via the command line if a TTY name is specified.
	 * It's safe to call this function early, even before RAG is initialized. This function is automatically
	 * called by sysIpcCreateThread, mere mortals should not normally call it.
	 *
	 * If this function is called after the RAG widgets have been created, a new widget will be created for
	 * this thread immediately.
	 *
	 * If this function is called after AdjustPriosBasedOnParams() has been called, the command line will
	 * be parsed for arguments to specify this thread's priority immediately.
	 *
	 * PARAMS:
	 *   threadId - Thread ID of the thread to register.
	 *   priority - Current priority of the thread.
	 *   name - User-friendly name for this thread.
	 *   ttyName - Name for the thread for use in a command line, "name_priority=[lowest|below_normal|normal|above_normal|highest]"
	 *             can be used to set it. This parameter can be NULL.
	 *   possiblyDuplicate - If true, this thread may have already been registered. If that's the case, then this call will update the
	 *                       existing entry.
	 */
	static void RegisterThread(sysIpcThreadId threadId, sysIpcPriority priority, const char *name, const char *ttyName, bool possiblyDuplicate = false);

	/** PURPOSE: Call this to indicate that a thread is no longer available.
	 *  This function is automatically called by sysIpcThreadWrapper, mere mortals don't normally have a reason to call it.
	 *
	 *  PARAMS:
	 *   threadId - Thread ID of the thread that is no longer available.
	 */
	static void UnregisterThread(sysIpcThreadId threadId);

	/** PURPOSE: Set the priority of all threads that match this "ttyName".
	 *
	 *  PARAMS:
	 *   ttyName - Name of the threads that are to be changed (this is the string passed in as ttyName in RegisterThread).
	 *   priority - Priority to set the thread to.
	 */
	static void SetPriority(const char *ttyName, sysIpcPriority priority);

	/* PURPOSE: This will go through all registered threads and change their priority if there is a command
	 * line argument to specify that.
	 */
	static void AdjustPriosBasedOnParams();

#if !__FINAL
	/* PURPOSE: Return the priority of the first thread that matches the given TTY name. Typically, all threads with the
	 * same name have the same priority.
	 *
	 * PARAM:
	 *   ttyName - TTY name of the thread to look for.
	 *
	 * RETURNS:
	 *   The priority of the thread, PRIO_IDLE (plus assert) if not found.
	 */
	static sysIpcPriority GetPriorityByTtyName(const char *ttyName);

	/* PURPOSE: Return the name of a priority.
	 */
	static const char *GetPriorityName(sysIpcPriority priority);
#endif // !__FINAL


#if __BANK
	static void AddWidgets(bkGroup &group);
	static void DestroyWidgets();

	/* PURPOSE: Temporarily disable creating widgets for threads. Call AddDelayedWidgets() to create
	 * widgets for threads that were created while widgets had been blocked.
	 */
	static void EnableWidgetCreation(bool enabled);

	/* PURPOSE: Create widgets for all threads that were spawned while EnableWidgetCreation(false) had
	 * been active.
	 */
	static void AddDelayedWidgets();

#endif // __BANK

#if __WIN32PC
	/* PURPOSE: Return the name of a given thread. Required on PC, since Windows does not store the
	 * thread name anywhere.
	 *
	 * RETURNS:
	 *   The name of the thread, or "<unknown>" if not found.
	 */
	static const char* GetThreadName(sysIpcThreadId id);
#endif // __WIN32PC
};

} // namespace rage

#else // THREAD_REGISTRY

#define THREAD_REGISTRY_ONLY(_x)

#endif // THREAD_REGISTRY


#endif // SYSTEM_IPC_H
