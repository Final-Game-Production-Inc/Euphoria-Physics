//
// system/ipc.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "threadregistry.h"


#if THREAD_REGISTRY

#include "bank/bank.h"
#include "bank/combo.h"
#include "bank/bkmgr.h"
#include "system/nelem.h"
#include "system/param.h"


namespace rage {

/* PURPOSE: Internal control structure for each registered thread
 */
struct sysThreadInfo {

	bool IsValid() const				{ return m_ThreadId != sysIpcThreadIdInvalid; }

	// Go through the command line arguments and adjust this thread's priority based on that.
	void AdjustPrioBasedOnParams();

	// ID of this thread
	sysIpcThreadId m_ThreadId;

	// Current priority, minus PRIO_IDLE
	int m_AdjustedPriority;

	// User-friendly display name
	ConstString m_Name;

	// Name to be used for TTY purposes
	ConstString m_TtyName;

#if __BANK
	// Our RAG representation.
	bkWidget *m_Widget;
#endif // __BANK
};


static const char *s_PriorityNames[] = {
	"IDLE",					// -15
	"LOWEST_12",			// -14
	"LOWEST_11",
	"LOWEST_10",
	"LOWEST_9",
	"LOWEST_8",				// -10
	"LOWEST_7",
	"LOWEST_6",
	"LOWEST_5",
	"LOWEST_4",
	"LOWEST_3",				// -5
	"LOWEST_2",
	"LOWEST_1",
	"LOWEST",				// -2
	"BELOW_NORMAL",
	"NORMAL",				// 0
	"ABOVE_NORMAL",
	"HIGHEST",				// 2
	"HIGHEST_1",
	"HIGHEST_2",
	"HIGHEST_3",			// 5
	"HIGHEST_4",
	"HIGHEST_5",
	"HIGHEST_6",
	"HIGHEST_7",
	"HIGHEST_8",			// 10
	"HIGHEST_9",
	"HIGHEST_10",
	"HIGHEST_11",
	"HIGHEST_12",
	"TIME_CRITICAL",		// 15
};

CompileTimeAssert(NELEM(s_PriorityNames) == PRIO_TIME_CRITICAL - PRIO_IDLE + 1);

// -------- RAG-specific code ---------------------
#if __BANK
static bkGroup *s_ThreadInfoGroup;
static bool s_EnableWidgetCreation = true;


// RAG callback when the priority is changed by the user
void UpdateThreadPriority(sysThreadInfo *info)
{
	sysIpcSetThreadPriority(info->m_ThreadId, (sysIpcPriority) (info->m_AdjustedPriority + PRIO_IDLE));
}

// Add a RAG widget for one specific thread
void AddThreadWidget(sysThreadInfo &info)
{
	if (s_EnableWidgetCreation) {
		char desc[128];
		const char *ttyName = info.m_TtyName.c_str();

		if (ttyName) {
			formatf(desc, "Update priority with %s_priority", ttyName);
		} else {
			formatf(desc, "Can't adjust via command line");
		}

		info.m_Widget = s_ThreadInfoGroup->AddCombo(info.m_Name.c_str(), &info.m_AdjustedPriority, NELEM(s_PriorityNames), s_PriorityNames, datCallback(CFA1(UpdateThreadPriority), &info), desc);
	}
}

#endif // __BANK


// Registry of all known threads
static atArray<sysThreadInfo> s_ThreadInfos;

// True if we already went through the command line arguments and adjusted priorities based on that
static bool s_PriosAdjusted;

static sysCriticalSectionToken s_ThreadRegistryCS; 

/** PURPOSE: Identify a thread priority by name. Returns PRIO_IDLE if the string could not be identified.
 */
sysIpcPriority IdentifyPriority(const char *prioString) {
	for (int x=0; x<NELEM(s_PriorityNames); x++) {
		if (stricmp(prioString, s_PriorityNames[x]) == 0) {
			return (sysIpcPriority) (x + PRIO_IDLE);
		}
	}

	return PRIO_IDLE;
}


/** PURPOSE: Go through all command line arguments and see if it contains an argument that controls
 *  the priority for this thread.
 */
void sysThreadInfo::AdjustPrioBasedOnParams() {
	sysCriticalSection cs(s_ThreadRegistryCS);

	const char *ttyName = m_TtyName.c_str();

	if (ttyName) {
		int ttyNameLen = ustrlen(ttyName);

		int argc = sysParam::GetArgCount();
		char **argv = sysParam::GetArgArray();
		for (int i=1; i<argc; i++) 
		{
			const char *arg = argv[i];
			if (arg[0]!='-')	// Note that sysParam::Init already fixes up em dashes and other such things
				continue;
			else
				++arg;

			// Is this ours?
			if (strnicmp(arg, ttyName, ttyNameLen) == 0) {
				// Is this about the priority?
				if (strnicmp(&arg[ttyNameLen], "_priority", 9) == 0) {
					// Yep - let's adjust it.
					const char *value = strchr(&arg[ttyNameLen+1], '=')+1;
					sysIpcPriority prio = IdentifyPriority(value);

					if (prio == PRIO_IDLE) {
						Assertf(false, "Unknown thread priority %s - must be 'lowest', 'below_normal', 'normal', 'above_normal', 'highest'", value);
					} else {
						m_AdjustedPriority = prio - PRIO_IDLE;
						sysIpcSetThreadPriority(m_ThreadId, prio);
					}
				}
			}
		}
	}
}



void sysThreadRegistry::RegisterThread(sysIpcThreadId threadId, sysIpcPriority priority, const char *name, const char *ttyName, bool possiblyDuplicate) {
	sysCriticalSection cs(s_ThreadRegistryCS);

	if (s_ThreadInfos.GetCapacity() == 0) {
		USE_DEBUG_MEMORY();
		s_ThreadInfos.Reserve(RSG_PC ? 256 : 128);
	}

	bool wasDuplicate = false;

	sysThreadInfo *info = NULL;

	// If this could be a duplicate, see if we already have this thread registered.
	if (possiblyDuplicate) {
		for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
			if (s_ThreadInfos[x].m_ThreadId == threadId) {
				// Yup - there it is.
				info = &s_ThreadInfos[x];
				wasDuplicate = true;
				break;
			}
		}
	}

	if (!wasDuplicate) {
		// If we're out of free entries, let's go fishing for an unused one.
		if (s_ThreadInfos.GetCount() == s_ThreadInfos.GetCapacity()) {
			for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
				if (!s_ThreadInfos[x].IsValid()) {
					// Found one.
					info = &s_ThreadInfos[x];
					break;
				}
			}

			if (!info) {
				Errorf("Too many threads registered - you either have tons of threads, or some threads weren't unregistered properly. See list below to see if something is leaking:");

				for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
					Errorf("%d: %s", x, s_ThreadInfos[x].m_Name.c_str());
				}
				Quitf(ERR_SYS_THREAD, "Out of thread info blocks - increase the array size in sysThreadRegistry::RegisterThread, and make sure we're not leaking threads. See TTY for details.");
			}
		} else {
			info = &s_ThreadInfos.Append();
		}

#if __BANK
		info->m_Widget = NULL;
#endif // __BANK
	}


	info->m_Name = name;
	info->m_TtyName = ttyName;
	info->m_ThreadId = threadId;
	info->m_AdjustedPriority = int(priority) - PRIO_IDLE;
	TELEMETRY_SET_THREAD_NAME(threadId, name);

	// If we missed the widget creation, add a widget now.
#if __BANK
	if (!wasDuplicate && s_ThreadInfoGroup) {
		AddThreadWidget(*info);
	}
#endif // __BANK
	
	// If we missed the part where we scan the command line arguments, check them now.
	if (s_PriosAdjusted) {
		info->AdjustPrioBasedOnParams();
	}
}

#if __BANK
void RemoveThreadWidgets(size_t udata) 
{
	bkWidget* widget = reinterpret_cast<bkWidget*>(udata);
	if (s_ThreadInfoGroup && widget) {
		s_ThreadInfoGroup->Remove(*widget);
	}
}
#endif

void sysThreadRegistry::UnregisterThread(sysIpcThreadId threadId) {
	sysCriticalSection cs(s_ThreadRegistryCS);

	int count = s_ThreadInfos.GetCount();

	// If there is an info block for this thread...
	for (int x=0; x<count; x++) {
		if (s_ThreadInfos[x].m_ThreadId == threadId) {

			// Remove the RAG widget, if there is one.
#if __BANK
			if (s_ThreadInfoGroup)
			{
				BANKMGR.Invoke(atDelegate<void(size_t)>(RemoveThreadWidgets), reinterpret_cast<size_t>(s_ThreadInfos[x].m_Widget));
			}
#endif
			// Remove the info block if we can.
			if (x == count - 1) {
				s_ThreadInfos.DeleteFast(x);
			} else {
				// We can't remove this block - there are pointers to other blocks, and removing
				// it would break those pointers. Instead, let's just mark this block as free.
				s_ThreadInfos[x].m_ThreadId = sysIpcThreadIdInvalid;
			}
		}
	}
}

void sysThreadRegistry::AdjustPriosBasedOnParams() {
	sysCriticalSection cs(s_ThreadRegistryCS);

	int count = s_ThreadInfos.GetCount();

	for (int x=0; x<count; x++) {
		if (s_ThreadInfos[x].IsValid()) 
		{
			TELEMETRY_SET_THREAD_NAME(s_ThreadInfos[x].m_ThreadId, s_ThreadInfos[x].m_Name);
			s_ThreadInfos[x].AdjustPrioBasedOnParams();
		}
	}

	// Tell all the late bloomers that they have to check the command line themselves.
	s_PriosAdjusted = true;
}

void sysThreadRegistry::SetPriority(const char *ttyName, sysIpcPriority priority)
{
	int count = s_ThreadInfos.GetCount();
	ASSERT_ONLY(bool threadFound = false;)

	for (int x=0; x<count; x++) {
		if (s_ThreadInfos[x].IsValid() && s_ThreadInfos[x].m_TtyName) {
			if (!stricmp(s_ThreadInfos[x].m_TtyName, ttyName)) {
				sysIpcSetThreadPriority(s_ThreadInfos[x].m_ThreadId, priority);
				s_ThreadInfos[x].m_AdjustedPriority = priority - PRIO_IDLE;
				ASSERT_ONLY(threadFound = true;)
			}
		}
	}

	Assertf(threadFound, "Unknown thread TTY name '%s'", ttyName);
}

#if !__FINAL
sysIpcPriority sysThreadRegistry::GetPriorityByTtyName(const char *ttyName)
{
	int count = s_ThreadInfos.GetCount();
	ASSERT_ONLY(bool threadFound = false;)

	for (int x=0; x<count; x++) {
		if (s_ThreadInfos[x].IsValid() && s_ThreadInfos[x].m_TtyName) {
			if (!stricmp(s_ThreadInfos[x].m_TtyName, ttyName)) {
				return (sysIpcPriority) (s_ThreadInfos[x].m_AdjustedPriority + PRIO_IDLE);
			}
		}
	}

	Assertf(threadFound, "Unknown thread TTY name '%s'", ttyName);
	return PRIO_IDLE;
}

const char *sysThreadRegistry::GetPriorityName(sysIpcPriority priority)
{
	return s_PriorityNames[(int) priority - PRIO_IDLE];
}

#endif // !__FINAL


#if __BANK
void sysThreadRegistry::AddWidgets(bkGroup &group)
{
	sysCriticalSection cs(s_ThreadRegistryCS);

	Assert(!s_ThreadInfoGroup);

	s_ThreadInfoGroup = group.AddGroup("Thread Priority");

	AddDelayedWidgets();
}

void sysThreadRegistry::EnableWidgetCreation(bool enabled)
{
	s_EnableWidgetCreation = enabled;
}

void sysThreadRegistry::AddDelayedWidgets()
{
	for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
		if (!s_ThreadInfos[x].m_Widget && s_ThreadInfos[x].IsValid()) {
			AddThreadWidget(s_ThreadInfos[x]);
		}
	}
}

void sysThreadRegistry::DestroyWidgets()
{
	sysCriticalSection cs(s_ThreadRegistryCS);

	// This assumes that the group has been destroyed, which will implicitly destroy
	// all widgets.
	s_ThreadInfoGroup = NULL;

	for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
		s_ThreadInfos[x].m_Widget = NULL;
	}
}
#endif // __BANK

#if __WIN32PC
const char* sysThreadRegistry::GetThreadName(sysIpcThreadId id)
{
	sysCriticalSection cs(s_ThreadRegistryCS);

	for (int x=0; x<s_ThreadInfos.GetCount(); x++) {
		if (s_ThreadInfos[x].m_ThreadId == id) {
			return s_ThreadInfos[x].m_Name;
		}
	}

	return "<unknown>";
}
#endif // __WIN32PC

} // namespace rage

#endif // THREAD_REGISTRY
