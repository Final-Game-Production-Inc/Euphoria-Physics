//
// system/eventtracingforwindows.cpp
//
// Copyright (C) 2018-2018 Rockstar Games.  All Rights Reserved.
//

#include "eventtracingforwindows.h"

#if ETW_SUPPORTED

#include "atl/array.h"
#include "diag/channel.h"
#include "diag/trap.h"
#include "math/amath.h"
#include "string/string.h"
#include "system/alloca.h"
#include "system/bit.h"
#include "system/criticalsection.h"
#include "system/ipc.h"
#include "system/membarrier.h"
#include "system/memops.h"
#include "system/nelem.h"
#include "system/xtl.h"
#include <initguid.h> // Needed to be included before evntrace.h
#include <evntrace.h>
#include <evntcons.h>
#if RSG_GDK
#include <evntprov.h>
#endif
#include <string.h>
#include <tdh.h>
#pragma warning(push)
#pragma warning(disable: 4800)
#include <VersionHelpers.h>
#pragma warning(pop)
#if RSG_XDK || RSG_GDK
//	Ensure initguid.h included before this and before evntrace.h
#	include "profile/durango_etw_secret_api.h"
#endif

#if RSG_GDK
#include <evntprov.h>
#endif

// Annoyingly, XB1 XDK headers only provide version 1 of ENABLE_TRACE_PARAMETERS.
// We can't simply provide version 2 here, else it will clash, so it was renamed with
// a _V2 suffix in "durango_etw_secret_api.h".
#if !RSG_XDK
	typedef _ENABLE_TRACE_PARAMETERS    _ENABLE_TRACE_PARAMETERS_V2;
	typedef ENABLE_TRACE_PARAMETERS     ENABLE_TRACE_PARAMETERS_V2;
	typedef PENABLE_TRACE_PARAMETERS    PENABLE_TRACE_PARAMETERS_V2;
#endif

#undef FlushTrace
#undef StartTrace

RAGE_DEFINE_OWNED_CHANNEL(Etw,"*Default Code (Engine)*")
#define etwAssert(cond)                     RAGE_ASSERT(Etw,cond)
#define etwAssertf(cond,fmt,...)            RAGE_ASSERTF(Etw,cond,fmt,##__VA_ARGS__)
#define etwAssertMsg(cond,msg)				RAGE_ASSERTF(Etw,cond,msg)
#define etwAssert2(cond)                    RAGE_ASSERT2(Etw,cond)
#define etwAssertf2(cond,fmt,...)           RAGE_ASSERTF2(Etw,cond,fmt,##__VA_ARGS__)
#define etwAssertMsg2(cond,msg)				RAGE_ASSERTF2(Etw,cond,msg)
#define etwAssert3(cond)                    RAGE_ASSERT3(Etw,cond)
#define etwAssertf3(cond,fmt,...)           RAGE_ASSERTF3(Etw,cond,fmt,##__VA_ARGS__)
#define etwAssertMsg3(cond,msg)				RAGE_ASSERTF3(Etw,cond,msg)
#define etwVerify(cond)                     RAGE_VERIFY(Etw,cond)
#define etwVerifyf(cond,fmt,...)            RAGE_VERIFYF(Etw,cond,fmt,##__VA_ARGS__)
#define etwVerify2(cond)                    RAGE_VERIFY2(Etw,cond)
#define etwVerifyf2(cond,fmt,...)           RAGE_VERIFYF2(Etw,cond,fmt,##__VA_ARGS__)
#define etwVerify3(cond)                    RAGE_VERIFY3(Etw,cond)
#define etwVerifyf3(cond,fmt,...)           RAGE_VERIFYF3(Etw,cond,fmt,##__VA_ARGS__)
#define etwErrorf(fmt,...)                  RAGE_ERRORF(Etw,fmt,##__VA_ARGS__)
#define etwWarningf(fmt,...)                RAGE_WARNINGF(Etw,fmt,##__VA_ARGS__)
#define etwDisplayf(fmt,...)                RAGE_DISPLAYF(Etw,fmt,##__VA_ARGS__)
#define etwDebugf1(fmt,...)                 RAGE_DEBUGF1(Etw,fmt,##__VA_ARGS__)
#define etwDebugf2(fmt,...)                 RAGE_DEBUGF2(Etw,fmt,##__VA_ARGS__)
#define etwDebugf3(fmt,...)                 RAGE_DEBUGF3(Etw,fmt,##__VA_ARGS__)
#define etwLogf(severity,fmt,...)           RAGE_LOGF(Etw,severity,fmt,##__VA_ARGS__)

#define GUID_FMT_STR                        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_FMT_ARGS(GUID)                 (GUID).Data1,(GUID).Data2,(GUID).Data3,(GUID).Data4[0],(GUID).Data4[1],(GUID).Data4[2], \
                                            (GUID).Data4[3],(GUID).Data4[4],(GUID).Data4[5],(GUID).Data4[7],(GUID).Data4[7]

// Include kernel addresses in stack traces.
// Since these addresses will not be visible in minidumps, it is rather pointless unless using a remote kernel debugger session.
#define ETW_INCLUDE_KERNEL_STACKS           (0)

// Spam warnings when ProcessTrace encounters an event not explicitly enabled.
#define ETW_WARN_UNEXPECTED_EVENTS          (0)

// ETW buffer size
#define ETW_BUFFER_SIZE (1024 << 10)

// ETW Minimum Buffer Count
// Maximum memory overhead is (ETW_BUFFER_COUNT+20) * ETW_BUFFER_SIZE = ~50 MB
#define ETW_BUFFER_COUNT (32)

namespace rage {
#if ETW_TDH_SUPPORTED
// When running on Windows 7, not all the TDH functions are available in
// tdl.dll.  This causes startup failures if the import library is used, so
// instead we need to dynamically load the dll and lookup the function pointers.
//
// Note that being in the rage namespace, these function pointers take
// precedence over the global namespace function declarations (in tdh.h) of the
// same name.
//
static TDHSTATUS (__stdcall *TdhGetEventInformation)(
	_In_ PEVENT_RECORD                                          Event,
	_In_ ULONG                                                  TdhContextCount,
	_In_reads_opt_(TdhContextCount) PTDH_CONTEXT                TdhContext,
	_Out_writes_bytes_opt_(*BufferSize) PTRACE_EVENT_INFO       Buffer,
	_Inout_ PULONG                                              BufferSize
);
static TDHSTATUS (__stdcall *TdhGetEventMapInformation)(
	_In_ PEVENT_RECORD                                          pEvent,
	_In_ PWSTR                                                  pMapName,
	_Out_writes_bytes_opt_(*pBufferSize) PEVENT_MAP_INFO        pBuffer,
	_Inout_ ULONG*                                              pBufferSize
);
static TDHSTATUS (__stdcall *TdhEnumerateManifestProviderEvents)(
	_In_ LPGUID                                                 ProviderGuid,
	_Out_writes_bytes_opt_(*BufferSize) PPROVIDER_EVENT_INFO    Buffer,
	_Inout_ ULONG*                                              BufferSize
);
static TDHSTATUS (__stdcall *TdhGetManifestEventInformation)(
	_In_ LPGUID                                                 ProviderGuid,
	_In_ PEVENT_DESCRIPTOR                                      EventDescriptor,
	_Out_writes_bytes_opt_(*BufferSize) PTRACE_EVENT_INFO       Buffer,
	_Inout_ ULONG*                                              BufferSize
);
static TDHSTATUS (__stdcall *TdhCreatePayloadFilter)(
	_In_ LPCGUID                                                ProviderGuid,
	_In_ PCEVENT_DESCRIPTOR                                     EventDescriptor,
	_In_ BOOLEAN                                                EventMatchANY,
	_In_ ULONG                                                  PayloadPredicateCount,
	_In_reads_(PayloadPredicateCount) PPAYLOAD_FILTER_PREDICATE PayloadPredicates,
	_Outptr_result_maybenull_ PVOID*                            PayloadFilter
);
static TDHSTATUS (__stdcall *TdhDeletePayloadFilter)(
	_Inout_ PVOID*                                              PayloadFilter
);
static TDHSTATUS (__stdcall *TdhAggregatePayloadFilters)(
	_In_ ULONG                                                  PayloadFilterCount,
	_In_reads_(PayloadFilterCount) PVOID*                       PayloadFilterPtrs,
	_In_reads_opt_(PayloadFilterCount) PBOOLEAN                 EventMatchALLFlags,
	_Out_ PEVENT_FILTER_DESCRIPTOR                              EventFilterDescriptor
);
static TDHSTATUS(__stdcall *TdhGetPropertySize)(
	_In_ PEVENT_RECORD pEvent,
	_In_ ULONG TdhContextCount,
	_In_reads_opt_(TdhContextCount) PTDH_CONTEXT pTdhContext,
	_In_ ULONG PropertyDataCount,
	_In_reads_(PropertyDataCount) PPROPERTY_DATA_DESCRIPTOR pPropertyData,
	_Out_ ULONG *pPropertySize
);
static TDHSTATUS(__stdcall *TdhGetProperty)(
	_In_ PEVENT_RECORD pEvent,
	_In_ ULONG TdhContextCount,
	_In_reads_opt_(TdhContextCount) PTDH_CONTEXT pTdhContext,
	_In_ ULONG PropertyDataCount,
	_In_reads_(PropertyDataCount) PPROPERTY_DATA_DESCRIPTOR pPropertyData,
	_In_ ULONG BufferSize,
	_Out_writes_bytes_(BufferSize) PBYTE pBuffer
);

static HMODULE s_TdhLibrary;
#endif //ETW_TDH_SUPPORTED


EtwGuid EtwGuid::INVALID;   // BSS initialzed to zero is fine here

class EventLookupTable {
public:
	EventLookupTable();
	~EventLookupTable();
	bool Init(GUID *providerGuid);
	const EVENT_DESCRIPTOR *GetEventDescriptor(const wchar_t *taskName,EtwOpcode opcode) const;
	const EVENT_DESCRIPTOR *GetEventDescriptor(const wchar_t *taskName) const;
	const EVENT_DESCRIPTOR *GetNextEventDescriptor(const wchar_t *taskName,const EVENT_DESCRIPTOR *currDescriptor) const;
private:
	PROVIDER_EVENT_INFO                    *m_Events;
	wchar_t                               **m_EventNames;
};

struct AddProvider {
	GUID                                    m_Guid;
	EventLookupTable                        m_EventLookupTable;
	struct Event {
		EVENT_DESCRIPTOR                        m_Descriptor;
		bool                                    m_StackTrace;
		bool                                    m_MatchAny;
		atArray<PAYLOAD_FILTER_PREDICATE>       m_Predicates;
	};
	atArray<Event>                          m_Events;
	bool                                    m_IgnoreKeyword0;
	u64                                     m_MatchAnyKeyword;
	u64                                     m_MatchAllKeyword;
	u8                                      m_MaxLevel;
};

struct EtwSessionInternal {
	u32                                     m_ThreadIdx;
	EVENT_TRACE_PROPERTIES                 *m_Properties;
	TRACEHANDLE                             m_SessionHandle;
	TRACEHANDLE                             m_OpenedHandle;
	EVENT_TRACE_LOGFILEW                    m_LogFile;
	EVENT_FILTER_DESCRIPTOR                 m_EventPayloadFilterDescriptor;
	AddProvider                            *m_AddProvider;
#	if ETW_WARN_UNEXPECTED_EVENTS
		bool                                    m_AnyStackTraces;
		struct ExpectedProvider {
			GUID                                    m_Guid;
			atArray<EVENT_DESCRIPTOR>               m_Events;
		};
		atArray<ExpectedProvider>               m_ExpectedProviders;
#	endif
};

// Annoyingly there is no way to pass user data through ProcessTrace, so instead...
static sysCriticalSectionToken s_SessionLock;
enum { MAX_NUM_SESSIONS = 4 };
static unsigned s_NumSessions;
struct ThreadCallback {
	sysIpcThreadId                                  m_Tid;
	const LambdaRef<void(const EtwEventRecord*)>   *m_Callback;
#	if ETW_WARN_UNEXPECTED_EVENTS
		const EtwSessionInternal                       *m_Session;
#	endif
};
static ThreadCallback s_ThreadCallbacks[MAX_NUM_SESSIONS] = {
	{sysIpcThreadIdInvalid, NULL},
	{sysIpcThreadIdInvalid, NULL},
	{sysIpcThreadIdInvalid, NULL},
	{sysIpcThreadIdInvalid, NULL},
};
typedef UIntBits<COMPILE_TIME_MAX_2(CompileTimeRoundUpPow2<MAX_NUM_SESSIONS>::value,8)>::T  ThreadMaskTypeU;
typedef SignedType<ThreadMaskTypeU>::T                                                      ThreadMaskTypeS;
static volatile ThreadMaskTypeU s_FreeThreadCallbacks = (1<<MAX_NUM_SESSIONS)-1;

#if ETW_WARN_UNEXPECTED_EVENTS
	static const EtwGuid ETW_MICROSOFT_WINDOWS_KERNEL_EVENTTRACING_GUID(0xb675ec37u,0xbdb6,0x4648,0xbc92,0xf3fdc74d3ca2uLL); // Microsoft-Windows-Kernel-EventTracing
	static EVENT_DESCRIPTOR *s_StackTraceEventDescriptors;
	static unsigned s_NumStackTraceEventDescriptors;
#endif

static const TRACEHANDLE INVALID_TRACEHANDLE = (TRACEHANDLE)-1;

enum : u32 { INVALID_THREAD_IDX = ~0u };

static u32 IncSessionCountAllocThreadIdx() {
	SYS_CS_SYNC(s_SessionLock);
	const u32 numSessions = s_NumSessions+1;
	FatalAssert(numSessions <= MAX_NUM_SESSIONS);
	if (numSessions == 1) {
		// Sadly this doesn't work on XB1.  TDH.DLL is in the system partition, not the title partition.
#if		ETW_TDH_SUPPORTED
		if (!IsWindows8Point1OrGreater()) {
			etwErrorf("Windows 8.1 or newer required for EtwSession");
			return INVALID_THREAD_IDX;
		}
		const HMODULE tdhLibrary = LoadLibraryW(L"tdh.dll");
		if (!tdhLibrary) {
			etwErrorf("Unable to load tdh.dll, 0x%08x",GetLastError());
			return INVALID_THREAD_IDX;
		}
		s_TdhLibrary = tdhLibrary;
#		define GET_TDH_PROC(NAME)                                               \
			do {                                                                \
				NAME = (decltype(NAME))GetProcAddress(tdhLibrary,#NAME);        \
				if (!NAME) {                                                    \
					etwErrorf("Unable to find " #NAME " in tdh.dll");           \
					FreeLibrary(tdhLibrary);                                    \
					return INVALID_THREAD_IDX;                                  \
				}                                                               \
			} while (0)
		GET_TDH_PROC(TdhGetEventInformation);
		GET_TDH_PROC(TdhGetEventMapInformation);
		GET_TDH_PROC(TdhEnumerateManifestProviderEvents);
		GET_TDH_PROC(TdhGetManifestEventInformation);
		GET_TDH_PROC(TdhCreatePayloadFilter);
		GET_TDH_PROC(TdhDeletePayloadFilter);
		GET_TDH_PROC(TdhAggregatePayloadFilters);
		GET_TDH_PROC(TdhGetPropertySize);
		GET_TDH_PROC(TdhGetProperty);
#		undef GET_TDH_PROC
#		endif //ETW_TDH_SUPPORTED

#		if ETW_WARN_UNEXPECTED_EVENTS
			EventLookupTable lut;
			if (lut.Init((GUID*)&ETW_MICROSOFT_WINDOWS_KERNEL_EVENTTRACING_GUID)) {
				unsigned numDesc = 0;
				static const wchar_t stackTrace[] = L"Stack Trace ";
				const EVENT_DESCRIPTOR *d = lut.GetEventDescriptor(stackTrace);
				while (d) {
					++numDesc;
					d = lut.GetNextEventDescriptor(stackTrace,d);
				}
				EVENT_DESCRIPTOR *const descs = rage_new EVENT_DESCRIPTOR[numDesc];
				EVENT_DESCRIPTOR *p = descs;
				d = lut.GetEventDescriptor(stackTrace);
				while (d) {
					*p++ = *d;
					d = lut.GetNextEventDescriptor(stackTrace,d);
				}
				s_NumStackTraceEventDescriptors = numDesc;
				s_StackTraceEventDescriptors = descs;
			}
#		endif
	}
	s_NumSessions = numSessions;
	const ThreadMaskTypeU prevFreeSlots = s_FreeThreadCallbacks;
	const ThreadMaskTypeU slot = prevFreeSlots&(ThreadMaskTypeU)-(ThreadMaskTypeS)prevFreeSlots; // least significant set bit
	s_FreeThreadCallbacks = prevFreeSlots-slot;
	const u32 threadIdx = Log2Floor(slot);
	FatalAssert(s_ThreadCallbacks[threadIdx].m_Tid == sysIpcThreadIdInvalid);
	return threadIdx;
}

static void DecSessionCountFreeThreadIdx(u32 threadIdx) {
	SYS_CS_SYNC(s_SessionLock);
	s_ThreadCallbacks[threadIdx].m_Tid = sysIpcThreadIdInvalid;
	s_FreeThreadCallbacks |= 1u<<threadIdx;
	const u32 numSessions = s_NumSessions-1;
	FatalAssert(numSessions < MAX_NUM_SESSIONS);
	s_NumSessions = numSessions;
	if (!numSessions) {
#		if ETW_WARN_UNEXPECTED_EVENTS
			delete[] s_StackTraceEventDescriptors;
			s_NumStackTraceEventDescriptors = 0;
#		endif
#		if ETW_TDH_SUPPORTED
		FreeLibrary(s_TdhLibrary);
		s_TdhLibrary = NULL;
#		endif //ETW_TDH_SUPPORTED
	}
}

static EtwSession *CreateInternal(const wchar_t *name,GUID guid,u32 enableFlags) {
	const u32 threadIdx = IncSessionCountAllocThreadIdx();
	if (threadIdx == INVALID_THREAD_IDX)
		return NULL;

	EtwSessionInternal *session = rage_new EtwSessionInternal;
	sysMemSet(session,0,sizeof(*session));
	session->m_ThreadIdx     = threadIdx;
	session->m_SessionHandle = INVALID_TRACEHANDLE;
	session->m_OpenedHandle  = INVALID_TRACEHANDLE;

	const u32 propsSize = (u32)(sizeof(EVENT_TRACE_PROPERTIES) + (wcslen(name)+1)*sizeof(wchar_t));
	EVENT_TRACE_PROPERTIES *const props = (EVENT_TRACE_PROPERTIES*)rage_aligned_new(__alignof(EVENT_TRACE_PROPERTIES)) char[propsSize];
	session->m_Properties = props;
	ULONG status = ERROR_SUCCESS;
	enum { NUM_START_TRACE_ATTEMPTS = 6 };
	for (u32 i=0; i<NUM_START_TRACE_ATTEMPTS; ++i) {
		sysMemSet(props,0,propsSize);
		props->Wnode.BufferSize = propsSize;
		props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
		props->Wnode.ClientContext = 1; // Query performace counter timestamps
		props->Wnode.Guid = guid;
		props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		props->EnableFlags = enableFlags;
		props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

		props->MinimumBuffers = ETW_BUFFER_COUNT;
		props->BufferSize = ETW_BUFFER_SIZE;

		// ERROR_ACCESS_DENIED(5): Only users with administrative privileges, users in the Performance Log Users group, and services running as LocalSystem, LocalService, NetworkService can control event tracing sessions.
		// ERROR_BAD_LENGTH(24): The Wnode.BufferSize member of Properties specifies an incorrect size. Properties does not have sufficient space allocated to hold a copy of SessionName.
		// ERROR_INVALID_PARAMETER(87)
		// ERROR_DISK_FULL(112)
		// ERROR_BAD_PATHNAME(161)
		// ERROR_ALREADY_EXISTS(183): A session with the same name or GUID is already running.
		// ERROR_NO_SUCH_PRIVILEGE(1313): A session doesn't have enough privileges. Check SeSystemProfilePrivilege.
		// ERROR_PRIVILEGE_NOT_HELD(1314): A required privilege is not held by the client.
		status = StartTraceW(&session->m_SessionHandle,name,props);

		if (status == ERROR_ALREADY_EXISTS) {
			ULONG controlStatus = ControlTraceW(0, name, props, EVENT_TRACE_CONTROL_STOP);
			if (controlStatus != ERROR_SUCCESS)
				etwWarningf("ControlTraceW failed, 0x%08x", controlStatus);
		} else {
			break;
		}
	}
	if (status != ERROR_SUCCESS) {
		etwErrorf("StartTraceW failed, 0x%08x",status);
		delete[] (char*)props;
		delete session;
		session = NULL;
		DecSessionCountFreeThreadIdx(threadIdx);
	}

	return (EtwSession*)session;
}

/*static*/ EtwSession *EtwSession::Create(const wchar_t *name) {
	return CreateInternal(name,GUID(),0);
}

/*static*/ EtwSession *EtwSession::CreateKernelSession(u32 enableFlags) {
#if RSG_RSC || RSG_GAMETOOL || RSG_TOOL
	(void) enableFlags;
	return NULL;
#else
	return CreateInternal(KERNEL_LOGGER_NAMEW, SystemTraceControlGuid, enableFlags);
#endif
}

void EtwSession::Destroy() {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	EVENT_TRACE_PROPERTIES *const props = session->m_Properties;
	const wchar_t *const name = (wchar_t*)((uptr)props+props->LoggerNameOffset);
	ULONG status = ControlTraceW(session->m_OpenedHandle,name,props,EVENT_TRACE_CONTROL_STOP);
	if (status != ERROR_SUCCESS) {
		etwWarningf("ControlTraceW failed to stop session, 0x%08x",status);
	}

	// ERROR_CTX_CLOSE_PENDING(7007L): The call was successful. The ProcessTrace function will stop after it has processed all real-time events in its buffers (it will not receive any new events).
	// ERROR_BUSY(170L): Prior to Windows Vista, you cannot close the trace until the ProcessTrace function completes.
	// ERROR_INVALID_HANDLE(6L): One of the following is true: TraceHandle is NULL. TraceHandle is INVALID_HANDLE_VALUE.
	status = CloseTrace(session->m_OpenedHandle);

	if (status != ERROR_SUCCESS) {
		etwWarningf("CloseTrace failed, 0x%08x",status);
	}
	DecSessionCountFreeThreadIdx(session->m_ThreadIdx);
	delete[] (char*)props;
	delete session;
}

void EtwSession::SetKernelSamplingFrequency(u32 frequency /*=1000*/)
{
	TRACE_PROFILE_INTERVAL sampleIntervalID = {0};
	u32 intervalNs = (1000000000u/frequency);
	sampleIntervalID.Interval = intervalNs/100; // Value is in 100ns

	ULONG status = TraceSetInformation(0,TraceSampledProfileIntervalInfo,&sampleIntervalID,sizeof(TRACE_PROFILE_INTERVAL));
	if (status == ERROR_NOT_SUPPORTED)
	{
		// TraceSampledProfileIntervalInfo is supported on Windows 8, Windows Server 2012, and later.
		// Output a warning instead of an error to prevent failing network unit tests.
		etwWarningf("Can't set frequency - error: ERROR_NOT_SUPPORTED");
	}
	else if (status != ERROR_SUCCESS)
	{
		etwErrorf("Can't set frequency - error: %d", status);
	}
}

EtwGuid::EtwGuid(u32 a,u16 b,u16 c,u16 d,u64 e) {
	FatalAssert((e & 0xffff000000000000uLL) == 0);
	Data1 = a;
	Data2 = b;
	Data3 = c;
	Data4[0] = (u8)(d>>8);
	Data4[1] = (u8)d;
	Data4[2] = (u8)(e>>40);
	Data4[3] = (u8)(e>>32);
	Data4[4] = (u8)(e>>24);
	Data4[5] = (u8)(e>>16);
	Data4[6] = (u8)(e>>8);
	Data4[7] = (u8)e;
}

#if ETW_TDH_SUPPORTED
bool EtwSession::BeginAddProvider(const EtwGuid &guid) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	FatalAssert(!session->m_AddProvider);
	AddProvider *const ap = rage_new AddProvider;
	sysMemCpy(&ap->m_Guid,&guid,sizeof(guid));
	if (ap->m_EventLookupTable.Init(&ap->m_Guid)) {
		ap->m_IgnoreKeyword0    = false;
		ap->m_MatchAnyKeyword   = 0;
		ap->m_MatchAllKeyword   = 0;
		ap->m_MaxLevel          = TRACE_LEVEL_VERBOSE;
		session->m_AddProvider  = ap;
		return true;
	}
	else {
		etwWarningf("Unable to find event provider " GUID_FMT_STR ".",GUID_FMT_ARGS(guid));
		delete ap;
		return false;
	}
}

EventLookupTable::EventLookupTable() {
	m_Events = NULL;
	m_EventNames = NULL;
}

EventLookupTable::~EventLookupTable() {
	if (m_EventNames) {
		const u32 numEvents = m_Events->NumberOfEvents;
		for (u32 i=0; i<numEvents; ++i)
			delete[] m_EventNames[i];
		delete[] m_EventNames;
	}
	delete[] (char*)m_Events;
}

bool EventLookupTable::Init(GUID *providerGuid) {
	// Enumerate all events from the provider.
	ULONG size = 0;
	ULONG status = TdhEnumerateManifestProviderEvents(providerGuid,NULL,&size);
	if (status != ERROR_INSUFFICIENT_BUFFER) {
		etwErrorf("TdhEnumerateManifestProviderEvents failed when attempting to determine required buffer size, 0x%08x",status);
		return false;
	}
	PROVIDER_EVENT_INFO *const events = (PROVIDER_EVENT_INFO*)rage_aligned_new(__alignof(PROVIDER_EVENT_INFO)) char[size];
	status = TdhEnumerateManifestProviderEvents(providerGuid,events,&size);
	if (status != ERROR_SUCCESS) {
		etwErrorf("TdhEnumerateManifestProviderEvents failed when attempting to get providers, 0x%08x",status);
		delete[] (char*)events;
		return false;
	}

	// For each event, lookup the task name.
	const u32 numEvents = events->NumberOfEvents;
	wchar_t **const eventNames = rage_new wchar_t*[numEvents];
	for (u32 i=0; i<numEvents; ++i) {
		// TODO: No need to delete and realloc TRACE_EVENT_INFO every loop.
		size = 0;
		EVENT_DESCRIPTOR *const desc = events->EventDescriptorsArray+i;
		status = TdhGetManifestEventInformation(providerGuid,desc,NULL,&size);
		if (status != ERROR_INSUFFICIENT_BUFFER) {
			etwErrorf("TdhGetManifestEventInformation failed when attempting to determine required buffer size, 0x%08x",status);
		}
		else {
			TRACE_EVENT_INFO *const info = (TRACE_EVENT_INFO*)rage_aligned_new(__alignof(TRACE_EVENT_INFO)) char[size];
			status = TdhGetManifestEventInformation(providerGuid,desc,info,&size);
			if (status != ERROR_SUCCESS) {
				etwErrorf("TdhGetManifestEventInformation failed when attempting to get provider info, 0x%08x",status);
				delete[] (char*)info;
			}
			else {
				wchar_t *name = NULL;
				const size_t nameOffset = info->TaskNameOffset;
				if (nameOffset) {
					const wchar_t *const str = (wchar_t*)((uptr)info+nameOffset);
					const size_t numChars = wcslen(str)+1;
					name = rage_new wchar_t[numChars];
					sysMemCpy(name,str,numChars*sizeof(wchar_t));
				}
				eventNames[i] = name;
				delete[] (char*)info;
				continue;
			}
		}

		while (i-->0)
			delete[] eventNames[i];
		delete[] eventNames;
		delete[] (char*)events;
		return false;
	}

	m_Events = events;
	m_EventNames = eventNames;
	return true;
}

const EVENT_DESCRIPTOR *EventLookupTable::GetEventDescriptor(const wchar_t *taskName,EtwOpcode opcode) const {
	if (m_EventNames) {
		const u32 numEvents = m_Events->NumberOfEvents;
		for (u32 i=0; i<numEvents; ++i) {
			const EVENT_DESCRIPTOR *const desc = m_Events->EventDescriptorsArray+i;
			if (desc->Opcode==(int)opcode && m_EventNames[i] && wcscmp(taskName,m_EventNames[i])==0)
				return desc;
		}
	}
	return NULL;
}

const EVENT_DESCRIPTOR *EventLookupTable::GetEventDescriptor(const wchar_t *taskName) const {
	return GetNextEventDescriptor(taskName,m_Events->EventDescriptorsArray-1);
}

const EVENT_DESCRIPTOR *EventLookupTable::GetNextEventDescriptor(const wchar_t *taskName,const EVENT_DESCRIPTOR *currDescriptor) const {
	if (m_EventNames) {
		const u32 numEvents = m_Events->NumberOfEvents;
		for (u32 i=(u32)(currDescriptor-m_Events->EventDescriptorsArray+1); i<numEvents; ++i)
			if (m_EventNames[i] && wcscmp(taskName,m_EventNames[i])==0)
				return m_Events->EventDescriptorsArray+i;
	}
	return NULL;
}

EtwEventId EtwSession::AddEvent(const wchar_t *taskName,EtwOpcode opcode) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	AddProvider *const ap = session->m_AddProvider;
	const EVENT_DESCRIPTOR *const desc = ap->m_EventLookupTable.GetEventDescriptor(taskName,opcode);
	if (!desc)
		return EtwEventId::INVALID;

	if (ap->m_Events.IsEmpty()) {
		ap->m_IgnoreKeyword0  = true;
		ap->m_MatchAnyKeyword = ~0uLL;
		ap->m_MatchAllKeyword =  0uLL;
		ap->m_MaxLevel = 0;
	}

	const u64 keyword = desc->Keyword;
	if (keyword) {
		ap->m_MatchAnyKeyword |= keyword;
		ap->m_MatchAllKeyword &= keyword;
	}
	else {
		ap->m_IgnoreKeyword0  = false;
		ap->m_MatchAnyKeyword = 0;
		ap->m_MatchAllKeyword = 0;
	}
	ap->m_MaxLevel = Max(ap->m_MaxLevel,desc->Level);

	AddProvider::Event *const e = &ap->m_Events.Grow();
	e->m_Descriptor = *desc;
	e->m_StackTrace = false;
	e->m_MatchAny   = false;

	return (EtwEventId)desc->Id;
}

void EtwSession::PredicatesMatchAny(bool matchAny) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	AddProvider *const ap = session->m_AddProvider;
	AddProvider::Event *const e = &ap->m_Events.back();
	e->m_MatchAny = matchAny;
}

void EtwSession::AddPredicate(const wchar_t *fieldName,EtwPredicateCmp compareOp,const wchar_t *value) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	AddProvider *const ap = session->m_AddProvider;
	AddProvider::Event *const e = &ap->m_Events.back();
	PAYLOAD_FILTER_PREDICATE *const p = &e->m_Predicates.Grow();
	p->FieldName = const_cast<wchar_t*>(fieldName);
	p->CompareOp = (USHORT)compareOp;
	p->Value     = const_cast<wchar_t*>(value);
}

void EtwSession::EndAddProvider() {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	AddProvider *const ap = session->m_AddProvider;
	GUID *const guid = &ap->m_Guid;
	ULONG status;
	u32 numStackTraces = 0;

	const u32 numEvents = ap->m_Events.GetCount();

	const bool payloadFiltersSupported = IsWindows8Point1OrGreater();

	EVENT_FILTER_DESCRIPTOR filters[3];
	u32 numFilters = 0;

	// Create filters for each event.
	if (numEvents) {
		// Create event id filter.
		FatalAssert(numEvents <= MAX_EVENT_FILTER_EVENT_ID_COUNT);
		FatalAssert(numEvents >= ANYSIZE_ARRAY);
		const u32 eventIdsSize = sizeof(EVENT_FILTER_EVENT_ID)+(numEvents-ANYSIZE_ARRAY)*sizeof(u16);
		const uptr eventIdsAlign1 = __alignof(EVENT_FILTER_EVENT_ID)-1;
		EVENT_FILTER_EVENT_ID *const eventIds = (EVENT_FILTER_EVENT_ID*)(((uptr)RageAlloca(eventIdsSize+eventIdsAlign1)+eventIdsAlign1)&~eventIdsAlign1);
		CompileTimeAssert(NELEM(eventIds->Events) == ANYSIZE_ARRAY);
		eventIds->FilterIn = true;
		eventIds->Reserved = 0;
		eventIds->Count    = (USHORT)numEvents;
		for (u32 i=0; i<numEvents; ++i)
			eventIds->Events[i] = ap->m_Events[i].m_Descriptor.Id;
		filters[numFilters].Ptr  = (uptr)eventIds;
		filters[numFilters].Size = eventIdsSize;
		filters[numFilters].Type = EVENT_FILTER_TYPE_EVENT_ID;
		++numFilters;

		// Check if any events want a stack trace, and create a stack trace
		// filter if they do.
		for (u32 i=0; i<numEvents; ++i)
			if (ap->m_Events[i].m_StackTrace)
				++numStackTraces;
		if (numStackTraces) {
			const u32 eventIdsSize = sizeof(EVENT_FILTER_EVENT_ID)+(numStackTraces-ANYSIZE_ARRAY)*sizeof(u16);
			const uptr eventIdsAlign1 = __alignof(EVENT_FILTER_EVENT_ID)-1;
			EVENT_FILTER_EVENT_ID *const eventIds = (EVENT_FILTER_EVENT_ID*)(((uptr)RageAlloca(eventIdsSize+eventIdsAlign1)+eventIdsAlign1)&~eventIdsAlign1);
			CompileTimeAssert(NELEM(eventIds->Events) == ANYSIZE_ARRAY);
			eventIds->FilterIn = true;
			eventIds->Reserved = 0;
			eventIds->Count    = (USHORT)numStackTraces;
			for (u32 i=0,j=0; i<numEvents; ++i)
				if (ap->m_Events[i].m_StackTrace)
					eventIds->Events[j++] = ap->m_Events[i].m_Descriptor.Id;
			filters[numFilters].Ptr  = (uptr)eventIds;
			filters[numFilters].Size = eventIdsSize;
			filters[numFilters].Type = EVENT_FILTER_TYPE_STACKWALK;
			++numFilters;
		}

		// Create temporary payload filters.
		if (payloadFiltersSupported) {
			FatalAssert(numEvents);
			void **const tmpFilters = Alloca(void*,numEvents);
			u32 pfIdx = 0;
			for (u32 i=0; i<numEvents; ++i) {
				AddProvider::Event *const e = &ap->m_Events[i];
				const u32 numPredicates = e->m_Predicates.GetCount();
				if (numPredicates) {
					status = TdhCreatePayloadFilter(guid,&e->m_Descriptor,e->m_MatchAny,numPredicates,e->m_Predicates.begin(),tmpFilters+pfIdx);
					if (status != ERROR_SUCCESS) {
						etwErrorf("TdhCreatePayloadFilter failed, 0x%08x",status);
					}
					else
						++pfIdx;
				}
			}

			// Aggregate temporary payload filters into one EVENT_FILTER_DESCRIPTOR.
			if (pfIdx) {
				BOOLEAN *const matchAllFlags = Alloca(BOOLEAN,pfIdx);
				for (u32 i=0; i<pfIdx; ++i)
					matchAllFlags[i] = TRUE;
				status = TdhAggregatePayloadFilters(pfIdx,tmpFilters,matchAllFlags,&session->m_EventPayloadFilterDescriptor);
				if (status != ERROR_SUCCESS) {
					etwErrorf("TdhAggregatePayloadFilters failed, 0x%08x",status);
				}

				// Destroy the temporary payload filters.
				while (pfIdx-->0)
					TdhDeletePayloadFilter(tmpFilters+pfIdx);

				// If we successfully created the aggregate payload filter, add it to the list.
				if (status == ERROR_SUCCESS) {
					TrapGE(numFilters,NELEM(filters));
					filters[numFilters++] = session->m_EventPayloadFilterDescriptor;
				}
			}
		}
	}

	// Create the ENABLE_TRACE_PARAMETERS_V2 struct for the filters.
	const uptr paramsAlign1 = __alignof(ENABLE_TRACE_PARAMETERS_V2)-1;
	ENABLE_TRACE_PARAMETERS_V2 *params = (ENABLE_TRACE_PARAMETERS_V2*)(((uptr)RageAlloca(sizeof(ENABLE_TRACE_PARAMETERS_V2)+paramsAlign1)+paramsAlign1)&~paramsAlign1);
	params->Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
	params->EnableProperty
		= (ap->m_IgnoreKeyword0 ? EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0 : 0)
		| (numStackTraces       ? EVENT_ENABLE_PROPERTY_STACK_TRACE      : 0);
	params->ControlFlags = 0;
	params->SourceId = session->m_Properties->Wnode.Guid;
	params->EnableFilterDesc = filters;
	params->FilterDescCount = numFilters;

	// Enable provider.
	status = EnableTraceEx2(session->m_SessionHandle,guid,EVENT_CONTROL_CODE_ENABLE_PROVIDER,ap->m_MaxLevel,ap->m_MatchAnyKeyword,ap->m_MatchAllKeyword,INFINITE,params);
	if (status != ERROR_SUCCESS) {
		etwErrorf("EnableTraceEx2 failed, 0x%08x",status);
	}

#	if ETW_WARN_UNEXPECTED_EVENTS
		if (numStackTraces)
			session->m_AnyStackTraces = true;
		EtwSessionInternal::ExpectedProvider *const ep = &session->m_ExpectedProviders.Grow();
		ep->m_Guid = ap->m_Guid;
		ep->m_Events.Reserve(ap->m_Events.GetCount());
		for (auto &e: ap->m_Events)
			ep->m_Events.Push(e.m_Descriptor);
#	endif

	delete ap;
	session->m_AddProvider = NULL;
}

void EtwSession::EnableStackTrace() {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	AddProvider *const ap = session->m_AddProvider;
	AddProvider::Event *const e = &ap->m_Events.back();
	e->m_StackTrace = true;
}

#endif //ETW_TDH_SUPPORTED

void EtwSession::EnableStackTrace(const EtwClassicEventId* eventIds, int count) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	ULONG status = TraceSetInformation(session->m_SessionHandle,TraceStackTracingInfo,(void*)eventIds,sizeof(CLASSIC_EVENT_ID) * count);
	if (status != ERROR_SUCCESS) {
		etwErrorf("TraceSetInformation failed: 0x%08x", status);
	}
}

static void __stdcall BaseCallback(EVENT_RECORD *rec) {
	const sysIpcThreadId tid = sysIpcGetThreadId();
	ThreadMaskTypeU possibleThreads = s_FreeThreadCallbacks^((1<<MAX_NUM_SESSIONS)-1);
	sysMemReadBarrier(); // Ensure s_FreeThreadCallbacks is read before s_ThreadCallbacks.
	for (;;) {
		FatalAssert(possibleThreads);
		const ThreadMaskTypeU threadMask = possibleThreads&(ThreadMaskTypeU)-(ThreadMaskTypeS)possibleThreads;  // least significant set bit
		const unsigned threadIdx = Log2Floor(threadMask);
		if (s_ThreadCallbacks[threadIdx].m_Tid == tid) {
#			if ETW_WARN_UNEXPECTED_EVENTS
				bool expected = false;
				const EtwSessionInternal *const session = s_ThreadCallbacks[threadIdx].m_Session;
				for (auto &p: session->m_ExpectedProviders)
					if (rec->EventHeader.ProviderId == p.m_Guid)
						for (auto &e: p.m_Events)
							if (memcmp(&e,&rec->EventHeader.EventDescriptor,sizeof(e)) == 0) {
								expected = true;
								break;
							}
				if (!expected) {
					etwWarningf("Unexpected ETW event");
					EtwEventMetadata *const md = EtwEventMetadata::Create((const EtwEventRecord*)rec);
					if (md) {
						md->PrintEvent((const EtwEventRecord*)rec,[](const char *OUTPUT_ONLY(str)){etwWarningf("%s",str);});
						md->Destroy();
					}
				}
#			endif
			(*s_ThreadCallbacks[threadIdx].m_Callback)((const EtwEventRecord*)rec);
			return;
		}
		possibleThreads -= threadMask;
	}
}

void EtwSession::Start(EtwEventRecordCallback callback /* = nullptr*/) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;

#	if ETW_WARN_UNEXPECTED_EVENTS
		if (session->m_AnyStackTraces) {
			const unsigned numStackTraceEventDescriptors = s_NumStackTraceEventDescriptors;
			if (numStackTraceEventDescriptors) {
				EtwSessionInternal::ExpectedProvider *const ep = &session->m_ExpectedProviders.Grow();
				sysMemCpy(&ep->m_Guid,&ETW_MICROSOFT_WINDOWS_KERNEL_EVENTTRACING_GUID,sizeof(GUID));
				ep->m_Events.Reserve(numStackTraceEventDescriptors);
				for (unsigned i=0; i<numStackTraceEventDescriptors; ++i)
					ep->m_Events.Push(s_StackTraceEventDescriptors[i]);
			}
		}
#	endif

	FatalAssert(session->m_OpenedHandle == INVALID_TRACEHANDLE);
	EVENT_TRACE_LOGFILEW *const log = &session->m_LogFile;
	const EVENT_TRACE_PROPERTIES *const props = session->m_Properties;
	sysMemSet(log,0,sizeof(*log));
	log->LoggerName = (wchar_t*)((uptr)props+props->LoggerNameOffset);
	log->ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
	log->EventRecordCallback = callback ? (PEVENT_RECORD_CALLBACK)callback : BaseCallback;
	const TRACEHANDLE handle = OpenTraceW(log);
	if (handle == INVALID_TRACEHANDLE) {
		etwErrorf("OpenTrace failed, 0x%08x",GetLastError());
	}
	session->m_OpenedHandle = handle;
}

void EtwSession::Process(const LambdaRef<void(const EtwEventRecord*)> &callback) {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	const u32 threadIdx = session->m_ThreadIdx;
	s_ThreadCallbacks[threadIdx].m_Tid = sysIpcGetThreadId();
	s_ThreadCallbacks[threadIdx].m_Callback = &callback;
#	if ETW_WARN_UNEXPECTED_EVENTS
		s_ThreadCallbacks[threadIdx].m_Session = session;
#	endif

	Process();
}

void EtwSession::Process() {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	const ULONG status = ::ProcessTrace(&session->m_OpenedHandle,1,NULL,NULL);
	if (status != ERROR_SUCCESS) {
		etwErrorf("ProcessTrace failed, 0x%08x",status);
	}
}

void EtwSession::Flush() {
	EtwSessionInternal *const session = (EtwSessionInternal*)this;
	EVENT_TRACE_PROPERTIES *const props = session->m_Properties;
	const wchar_t *const name = (wchar_t*)((uptr)props+props->LoggerNameOffset);
	const ULONG status = ControlTraceW(session->m_SessionHandle,name,props,EVENT_TRACE_CONTROL_FLUSH);
	if (status != ERROR_SUCCESS) {
		etwWarningf("ControlTraceW failed to flush session, 0x%08x",status);
	}
}

#if ETW_TDH_SUPPORTED
struct EtwEventMetadataInternal {
	size_t  m_InfoOffset;               // offset from this to TRACE_EVENT_INFO struct
	size_t  m_MapOffsetsArrayOffset;    // offset from this to array of offsets to EVENT_MAP_INFO structs
	// [possible padding...]
	// TRACE_EVENT_INFO
	// [possible padding...]
	// Array of size_t offsets from this to EVENT_MAP_INFO structs.  Array has one entry for every TRACE_EVENT_INFO.PropertyCount.
	// [possible padding...]
	// EVENT_MAP_INFO structs
};

/*static*/ EtwEventMetadata *EtwEventMetadata::Create(const EtwEventRecord *templateRec) {
	EVENT_RECORD *const rec = (EVENT_RECORD*)templateRec;
	ULONG status;

	// Determine sizes for TRACE_EVENT_INFO struct.
	ULONG traceEventInfoSize = 0;
	status = TdhGetEventInformation(rec,0,NULL,NULL,&traceEventInfoSize);
	if (status != ERROR_INSUFFICIENT_BUFFER) {
		etwErrorf("TdhGetEventInformation failed when attempting to get required buffer size, 0x%08x",status);
		return NULL;
	}

	// Get a temporary copy of the TRACE_EVENT_INFO struct on the stack.
	const uptr traceEventInfoAlign  = __alignof(TRACE_EVENT_INFO);
	const uptr traceEventInfoAlign1 = traceEventInfoAlign-1;
	TRACE_EVENT_INFO *const traceEventInfoTmp = (TRACE_EVENT_INFO*)(((uptr)RageAlloca(traceEventInfoSize+traceEventInfoAlign1)+traceEventInfoAlign1)&~traceEventInfoAlign1);
	status = TdhGetEventInformation(rec,0,NULL,traceEventInfoTmp,&traceEventInfoSize);
	if (status != ERROR_SUCCESS) {
		etwErrorf("TdhGetEventInformation failed when attempting to get event info, 0x%08x",status);
		return NULL;
	}

	// Get the total size required for all the EVENT_MAP_INFO structs.
	const unsigned propertyCount = traceEventInfoTmp->PropertyCount;
	ULONG *const eventMapInfoSizes = Alloca(ULONG,propertyCount);
	sysMemSet(eventMapInfoSizes,0,sizeof(ULONG)*propertyCount);
	for (unsigned i=0; i<propertyCount; ++i) {
		EVENT_PROPERTY_INFO *const prop = traceEventInfoTmp->EventPropertyInfoArray+i;
		if ((~prop->Flags&PropertyStruct) && prop->nonStructType.MapNameOffset) {
			wchar_t *const mapName = (wchar_t*)((uptr)traceEventInfoTmp+prop->nonStructType.MapNameOffset);
			status = TdhGetEventMapInformation(rec,mapName,NULL,eventMapInfoSizes+i);
			if (status != ERROR_INSUFFICIENT_BUFFER) {
				etwErrorf("TdhGetEventMapInformation failed when attempting to get required buffer size, 0x%08x",status);
				return NULL;
			}
		}
	}

	// Calculate total size and allocate.
	const uptr sizetAlign  = __alignof(size_t);
	const uptr sizetAlign1 = sizetAlign-1;
	const uptr eventMapInfoAlign  = __alignof(EVENT_MAP_INFO);
	const uptr eventMapInfoAlign1 = eventMapInfoAlign-1;
	size_t bufSize = sizeof(EtwEventMetadataInternal);
	bufSize = ((bufSize+traceEventInfoAlign1)&~traceEventInfoAlign1)    + traceEventInfoSize;
	bufSize = ((bufSize+sizetAlign1)&~sizetAlign1)                      + propertyCount*sizeof(size_t);
	for (unsigned i=0; i<propertyCount; ++i)
		bufSize = ((bufSize+eventMapInfoAlign1)&~eventMapInfoAlign1)        + eventMapInfoSizes[i];
	const size_t maxAlign = COMPILE_TIME_MAX_4(__alignof(EtwEventMetadataInternal),traceEventInfoAlign,sizetAlign,eventMapInfoAlign);
	EtwEventMetadataInternal *const metadata = (EtwEventMetadataInternal*)rage_aligned_new(maxAlign) char[bufSize];

	// Fill in all the data.
	TRACE_EVENT_INFO *const traceEventInfo = (TRACE_EVENT_INFO*)(((uptr)(metadata+1)+traceEventInfoAlign1)&~traceEventInfoAlign1);
	metadata->m_InfoOffset = (uptr)traceEventInfo-(uptr)metadata;
	sysMemCpy(traceEventInfo,traceEventInfoTmp,traceEventInfoSize);
	size_t *const eventMapInfoOffsets = (size_t*)(((uptr)traceEventInfo+traceEventInfoSize+sizetAlign1)&~sizetAlign1);
	metadata->m_MapOffsetsArrayOffset = (uptr)eventMapInfoOffsets-(uptr)metadata;
	EVENT_MAP_INFO *eventMapInfo = (EVENT_MAP_INFO*)(((uptr)(eventMapInfoOffsets+propertyCount)+eventMapInfoAlign1)&~eventMapInfoAlign1);
	for (unsigned i=0; i<propertyCount; ++i) {
		EVENT_PROPERTY_INFO *const prop = traceEventInfoTmp->EventPropertyInfoArray+i;
		if ((~prop->Flags&PropertyStruct) && prop->nonStructType.MapNameOffset) {
			wchar_t *const mapName = (wchar_t*)((uptr)traceEventInfoTmp+prop->nonStructType.MapNameOffset);
			status = TdhGetEventMapInformation(rec,mapName,eventMapInfo,eventMapInfoSizes+i);
			if (status != ERROR_SUCCESS) {
				etwErrorf("TdhGetEventMapInformation failed when attempting to get map info, 0x%08x",status);
				delete[] (char*)metadata;
				return NULL;
			}
			eventMapInfoOffsets[i] = (uptr)eventMapInfo-(uptr)metadata;
			eventMapInfo = (EVENT_MAP_INFO*)(((uptr)eventMapInfo+eventMapInfoSizes[i]+eventMapInfoAlign1)&~eventMapInfoAlign1);
		}
		else
			eventMapInfoOffsets[i] = 0;
	}

	return (EtwEventMetadata*)metadata;
}

void EtwEventMetadata::Destroy() {
	delete[] (char*)this;
}

// Extracting a property gets a bit ticky when there are variable sized arrays before it.
//
// For example:
//  Case 1:
//      u32     SizeOfArrayA;
//      u32     ArrayA[SizeOfArrayA/4];
//      u32     SizeOfArrayB;
//      u32     ArrayB[SizeOfArrayB/4];
//      u32     Val;
//
//  Case 2:
//      u32     SizeOfArrayA;
//      u32     SizeOfArrayB;
//      u32     ArrayA[SizeOfArrayA/4];
//      u32     ArrayB[SizeOfArrayB/4];
//      u32     Val;
//
// In the first case, need to read SizeOfArrayA at offset zero, multiply by the
// size of an element in ArrayA, and use that to calulate the offset of
// SizeOfArrayB.  Then read SizeOfArrayB and use that to calculate the offset of
// Val.
//
// The second case is different.  Both SizeOfArrayA and SizeOfArrayB can be read
// directly, and then used to calculate the offset of Val.
//
// The concept of a Level has been created to contain all values that can be
// read using a fixed offset before calculating the next variable offset.  In
// the first example, there would be two levels, one to read SizeOfArrayA, and
// one to read SizeOfArrayB.  In the second example, there is just one level for
// reading SizeOfArrayA and SizeOfArrayB.
//

struct EtwEventExtractorInternal {
	u16         m_FinalReadOffset;
	u8          m_FinalReadNumBytes;
	u8          m_NumLevels;
	struct Level {
		u16         m_FixedSize;
		u8          m_NumReads;
		u8          m_Reserved;
		struct Read {
			u16         m_Offset;
			u8          m_NumBytes;
			u8          m_Reserved;
		};
		Read        m_Reads[1];         // variable sized array with m_NumReads entries.
	};
	Level       m_Levels;               // variable sized array with m_NumLevels entries.
};

// So going back to the two examples, we would have something like
//
//  Case 1:
//      m_FinalReadOffset   0
//      m_FinalReadNumBytes 4
//      m_NumLevels         2
//      m_Levels
//      [0] m_FixedSize     4
//          m_NumReads      1
//          m_Reads
//          [0] m_Offset    0
//              m_NumBytes  4
//      [1] m_FixedSize     4
//          m_NumReads      1
//          m_Reads
//          [0] m_Offset    0
//              m_NumBytes  4
//
//  Case 2:
//      m_FinalReadOffset   0
//      m_FinalReadNumBytes 4
//      m_NumLevels         1
//      m_Levels
//      [0] m_FixedSize     8
//          m_NumReads      2
//          m_Reads
//          [0] m_Offset    0
//              m_NumBytes  4
//          [1] m_Offset    4
//              m_NumBytes  4
//

void EtwEventExtractorBase::Destroy() {
	delete[] (char*)this;
}

u64 EtwEventExtractorBase::GetBase(const EtwEventRecord *rec) const {
	EtwEventExtractorInternal *const extractor = (EtwEventExtractorInternal*)this;
	const char *dataStart = (char*)(((const EVENT_RECORD*)rec)->UserData);
	const unsigned numLevels = extractor->m_NumLevels;
	const EtwEventExtractorInternal::Level *level = &extractor->m_Levels;
	for (unsigned levelIdx=0; levelIdx<numLevels; ++levelIdx) {
		u16 levelSize = 0;
		const unsigned numReads = level->m_NumReads;
		for (unsigned readIdx=0; readIdx<numReads; ++readIdx) {
			u64 val = 0;
			sysMemCpy(&val,dataStart+level->m_Reads[readIdx].m_Offset,level->m_Reads[readIdx].m_NumBytes);
			levelSize += (u16)val;
		}
		dataStart += levelSize;
		level = (EtwEventExtractorInternal::Level*)(level->m_Reads+numReads);
	}
	u64 val = 0;
	sysMemCpy(&val,dataStart+extractor->m_FinalReadOffset,extractor->m_FinalReadNumBytes);
	return val;
}

void *EtwEventMetadata::CreateEventExtractorInternal(const wchar_t *property,size_t numBytes) const {
	FatalAssert(numBytes <= sizeof(u64));
	const EtwEventMetadataInternal *const self = (const EtwEventMetadataInternal*)this;
	const TRACE_EVENT_INFO *const info = (const TRACE_EVENT_INFO*)((uptr)self+self->m_InfoOffset);

	// Find the indices of the variable sized properties, and the indices of their sizes.
	u32 variableSizedProperties[64];
	u32 readIndices[64];
	u32 numReads = 0;
	u32 propertyIdx = ~0u;
	const unsigned numTopLevelProperties = info->TopLevelPropertyCount;
	for (u32 i=0; i<numTopLevelProperties; ++i) {
		const EVENT_PROPERTY_INFO *const prop = info->EventPropertyInfoArray+i;
		if (wcscmp(property,(const wchar_t*)((uptr)info+prop->NameOffset)) == 0) {
			propertyIdx = i;
			break;
		}
		else if (prop->Flags & PropertyParamCount) {
			TrapGE(numReads,NELEM(variableSizedProperties));
			variableSizedProperties[numReads] = i;
			TrapGE(numReads,NELEM(readIndices));
			FatalAssert(prop->lengthPropertyIndex < i);
			readIndices[numReads] = prop->lengthPropertyIndex;
			++numReads;
		}
		else if (prop->Flags & PropertyParamLength) {
			etwErrorf("TODO: Not currently supported");
			return NULL;
		}
		else if (prop->length == 0)
		{
			etwErrorf("TODO: Not currently supported - a property with variable size detected");
			return NULL;
		}
	}
	if (propertyIdx == ~0u)
		return NULL;

	// Calculate the amount of memory we need, and then allocate it.
	u32 numLevels = 0;
	u32 vspIdx = 0;
	u32 riIdx = 0;
	FatalAssert(!numReads || variableSizedProperties[numReads-1] > readIndices[numReads-1]);
	while (vspIdx < numReads) {
		// Iterate through consecutive reads.
		if (riIdx < numReads)
			while (readIndices[riIdx] < variableSizedProperties[vspIdx])
				if (++riIdx >= numReads) {
					FatalAssert(riIdx == numReads);
					break;
				}
		// Iterate through consecutive variable sized arrays.
		FatalAssert(vspIdx < numReads);
		++numLevels;
		while (variableSizedProperties[vspIdx] < readIndices[riIdx])
			if (++vspIdx >= numLevels) {
				FatalAssert(vspIdx == numLevels);
				break;
			}
	}
	const size_t allocSize = sizeof(EtwEventExtractorInternal)-sizeof(EtwEventExtractorInternal::Level)
		+numLevels*(sizeof(EtwEventExtractorInternal::Level)-sizeof(EtwEventExtractorInternal::Level::Read))
		+numReads*sizeof(EtwEventExtractorInternal::Level::Read);
	EtwEventExtractorInternal *const extractor = (EtwEventExtractorInternal*)rage_aligned_new(__alignof(EtwEventExtractorInternal)) char[allocSize];
	sysMemSet(extractor,0,allocSize);

	// Fill in the allocated struct.
	FatalAssert(numLevels < 0x100);
	extractor->m_NumLevels = (u8)numLevels;
	EtwEventExtractorInternal::Level *level = &extractor->m_Levels;
	bool wasVariableSized = false;
	u32 offset = 0;
	vspIdx = 0;
	riIdx = 0;
	for (u32 i=0; i<propertyIdx; ++i) {
		const EVENT_PROPERTY_INFO *const prop = info->EventPropertyInfoArray+i;
		// Property i is an array size ?
		if (riIdx<numReads && readIndices[riIdx]==i) {
			// Start a new level ?
			if (wasVariableSized) {
				FatalAssert(level->m_NumReads);
				FatalAssert(offset < 0x10000);
				level->m_FixedSize = (u16)offset;
				offset = 0;
				level = (EtwEventExtractorInternal::Level*)(level->m_Reads+level->m_NumReads);
				wasVariableSized = false;
			}
			const u8 numReadsThisLevel = level->m_NumReads;
			FatalAssert(offset < 0x10000);
			level->m_Reads[numReadsThisLevel].m_Offset = (u16)offset;
			FatalAssert(prop->length < 0x100);
			level->m_Reads[numReadsThisLevel].m_NumBytes = (u8)prop->length;
			level->m_NumReads = numReadsThisLevel+1;
			offset += prop->length;
			++riIdx;
		}
		else if (vspIdx<numReads && variableSizedProperties[vspIdx]==i) {
			wasVariableSized = true;
			++vspIdx;
		}
		else
			offset += prop->length;
	}
	if (numLevels) {
		FatalAssert(offset < 0x10000);
		level->m_FixedSize = (u16)offset;
		offset = 0;
	}
	FatalAssert(offset < 0x10000);
	extractor->m_FinalReadOffset = (u16)offset;
	FatalAssert(numBytes < 0x100);
	extractor->m_FinalReadNumBytes = (u8)numBytes;

	return extractor;
}

static void Print(const LambdaRef<void(const char*)> &func,const char *fmt,...) {
	va_list args;
	va_start(args,fmt);
	char buf[512];
	vformatf(buf,fmt,args);
	func(buf);
	va_end(args);
}

static void Print(const LambdaRef<void(const char*)> &func,const wchar_t *fmt,...) {
	va_list args;
	va_start(args,fmt);
	wchar_t wbuf[512];
	vformatf(wbuf,fmt,args);
	char buf[NELEM(wbuf)];
	safecpy(buf,wbuf);
	func(buf);
	va_end(args);
}

// See: https://docs.microsoft.com/en-us/windows/desktop/ETW/using-tdhgetproperty-to-consume-event-data

static bool PrintMappedEventProperty(const LambdaRef<void(const char*)> &func,const EtwEventMetadataInternal *metadata,unsigned index,unsigned indent,const wchar_t *name,u32 value) {
	const size_t mapOffset = ((size_t*)((uptr)metadata+metadata->m_MapOffsetsArrayOffset))[index];
	if (mapOffset) {
		const EVENT_MAP_INFO *const mapInfo = (EVENT_MAP_INFO*)((uptr)metadata+mapOffset);

		// Value map ?
		// (this horrible if/else condition comes from the above linked Microsoft example code.)
		if ((mapInfo->Flag&EVENTMAP_INFO_FLAG_MANIFEST_VALUEMAP) == EVENTMAP_INFO_FLAG_MANIFEST_VALUEMAP
			|| ((mapInfo->Flag&EVENTMAP_INFO_FLAG_WBEM_VALUEMAP) == EVENTMAP_INFO_FLAG_WBEM_VALUEMAP
				&& (mapInfo->Flag&~EVENTMAP_INFO_FLAG_WBEM_VALUEMAP) != EVENTMAP_INFO_FLAG_WBEM_FLAG)) {

			// Direct index into map?
			if ((mapInfo->Flag&EVENTMAP_INFO_FLAG_WBEM_NO_MAP) == EVENTMAP_INFO_FLAG_WBEM_NO_MAP) {
				Print(func,L"%*s%s: %s (%u)",indent,L"",name,(wchar_t*)((uptr)mapInfo+mapInfo->MapEntryArray[value].OutputOffset),value);
				return true;
			}

			// Otherwise check for matching value in map
			for (unsigned i=0; i<mapInfo->EntryCount; ++i)
				if (mapInfo->MapEntryArray[i].Value == value) {
					Print(func,L"%*s%s: %s (%u)",indent,L"",name,(wchar_t*)((uptr)mapInfo+mapInfo->MapEntryArray[i].OutputOffset),value);
					return true;
				}
		}

		// Bit map ?
		else if ((mapInfo->Flag&EVENTMAP_INFO_FLAG_MANIFEST_BITMAP) == EVENTMAP_INFO_FLAG_MANIFEST_BITMAP
			|| (mapInfo->Flag&EVENTMAP_INFO_FLAG_WBEM_BITMAP) == EVENTMAP_INFO_FLAG_WBEM_BITMAP
			|| ((mapInfo->Flag&EVENTMAP_INFO_FLAG_WBEM_VALUEMAP) == EVENTMAP_INFO_FLAG_WBEM_VALUEMAP
				&& (mapInfo->Flag&~EVENTMAP_INFO_FLAG_WBEM_VALUEMAP) == EVENTMAP_INFO_FLAG_WBEM_FLAG)) {

			Print(func,L"%*s%s: 0x%08x",indent,L"",name,value);

			const unsigned indent2 = indent+(unsigned)wcslen(name)+2+4;

			// One map entry per bit ?
			if ((mapInfo->Flag&EVENTMAP_INFO_FLAG_WBEM_NO_MAP) == EVENTMAP_INFO_FLAG_WBEM_NO_MAP) {
				for (unsigned i=0; i<mapInfo->EntryCount; ++i)
					if (value&(1uLL<<i))
						Print(func,L"%*s%s",indent2,L"",(wchar_t*)((uptr)mapInfo+mapInfo->MapEntryArray[i].OutputOffset));
			}

			// Only specific bits have a map entry ?
			else
				for (unsigned i=0; i<mapInfo->EntryCount; ++i) {
					const ULONG mask = mapInfo->MapEntryArray[i].Value;
					if ((value&mask) == mask)   // maybe mask can have more than one bit set???  seems odd, but example code checks this.
						Print(func,L"%*s%s",indent2,L"",(wchar_t*)((uptr)mapInfo+mapInfo->MapEntryArray[i].OutputOffset));
				}

			return true;
		}
	}

	return false;
}

// NOTE: Sometimes OutType equals 0 - in this case we need to infer the output type based on the input type
static USHORT GetStructureOutType(const EVENT_PROPERTY_INFO *const prop)
{
	if (prop->nonStructType.OutType)
		return prop->nonStructType.OutType;

	switch (prop->nonStructType.InType)
	{
	case 	TDH_INTYPE_UNICODESTRING:	return TDH_OUTTYPE_STRING;
	case 	TDH_INTYPE_ANSISTRING:		return TDH_OUTTYPE_STRING;
	case 	TDH_INTYPE_INT8:			return TDH_OUTTYPE_BYTE;
	case 	TDH_INTYPE_UINT8:			return TDH_OUTTYPE_UNSIGNEDBYTE;
	case 	TDH_INTYPE_INT16:			return TDH_OUTTYPE_SHORT;
	case 	TDH_INTYPE_UINT16:			return TDH_OUTTYPE_UNSIGNEDSHORT;
	case 	TDH_INTYPE_INT32:			return TDH_OUTTYPE_INT;
	case 	TDH_INTYPE_UINT32:			return TDH_OUTTYPE_UNSIGNEDINT;
	case 	TDH_INTYPE_INT64:			return TDH_OUTTYPE_LONG;
	case 	TDH_INTYPE_UINT64:			return TDH_OUTTYPE_UNSIGNEDLONG;
	case 	TDH_INTYPE_FLOAT:			return TDH_OUTTYPE_FLOAT;
	case 	TDH_INTYPE_DOUBLE:			return TDH_OUTTYPE_DOUBLE;
	case 	TDH_INTYPE_BOOLEAN:			return TDH_OUTTYPE_BOOLEAN;
	//case 	TDH_INTYPE_BINARY: break;
	case 	TDH_INTYPE_GUID:			return TDH_OUTTYPE_GUID;
	//case 	TDH_INTYPE_POINTER: return TDH_OUTTYPE_BOOLEAN;
	case 	TDH_INTYPE_FILETIME:		return TDH_OUTTYPE_CULTURE_INSENSITIVE_DATETIME;
	case 	TDH_INTYPE_SYSTEMTIME:		return TDH_OUTTYPE_CULTURE_INSENSITIVE_DATETIME;
	//case 	TDH_INTYPE_SID:	break;
	case 	TDH_INTYPE_HEXINT32:		return TDH_OUTTYPE_HEXINT32;
	case 	TDH_INTYPE_HEXINT64:		return TDH_OUTTYPE_HEXINT64;
	default:							return TDH_OUTTYPE_NULL;
	}
}

static void PrintEventProperty(const EtwEventRecord* rec,const EtwEventMetadataInternal *metadata,const void *dataStart,const void **dataCurr,unsigned index,unsigned indent,const LambdaRef<void(const char*)> &func) {
	const TRACE_EVENT_INFO *const info = (const TRACE_EVENT_INFO*)((uptr)metadata+metadata->m_InfoOffset);
	const EVENT_PROPERTY_INFO *const prop = info->EventPropertyInfoArray+index;

	// Variable sized array ?
	int arrayCount = 1;
	if (prop->Flags & PropertyParamCount) {
		etwWarningf("TODO: PrintEventProperty of variable sized arrays not currently supported");
	}
	else
		arrayCount = prop->count;

	const wchar_t *const name = (const wchar_t*)((uptr)info+prop->NameOffset);

	for (int arrayIndex = 0; arrayIndex < arrayCount; ++arrayIndex)
	{
		PROPERTY_DATA_DESCRIPTOR dataDescriptor = {0};
		dataDescriptor.PropertyName = (ULONGLONG)name;
		dataDescriptor.ArrayIndex = arrayIndex;

		ULONG propertySize = 0;
		ULONG status = TdhGetPropertySize((PEVENT_RECORD)rec,0,NULL,1,&dataDescriptor,&propertySize);
		if (status != ERROR_SUCCESS)
		{
			etwErrorf("TdhGetPropertySize failed, 0x%08x", status);
		}

		const void *p = *dataCurr;
		if (*dataCurr)
			*dataCurr = propertySize>0 ? *(const char**)dataCurr+propertySize : NULL;

		// Structure ?
		if (prop->Flags & PropertyStruct) {
			Print(func,L"%*s%s:",indent,L"",(const wchar_t*)((uptr)info+prop->NameOffset));
			const unsigned numMembers = prop->structType.NumOfStructMembers;
			const unsigned startIdx = prop->structType.StructStartIndex;
			for (unsigned i=0; i<numMembers; ++i)
				PrintEventProperty(rec,metadata,dataStart,&p,startIdx+i,indent+4,func);
		}

		else {
			if (p && propertySize!=0) {   // TODO: length!=-1 check is only here because arrays are not fully supported yet
				const auto inType = prop->nonStructType.InType;
				const auto outType = GetStructureOutType(prop);

				// TDH_INTYPE_UINT32 can have mapped strings to be displayed.
				if (inType==TDH_INTYPE_UINT32)
					switch (outType) {
					case TDH_OUTTYPE_HRESULT:
					case TDH_OUTTYPE_WIN32ERROR:
					case TDH_OUTTYPE_NTSTATUS:
					case TDH_OUTTYPE_HEXINT32:
					case TDH_OUTTYPE_UNSIGNEDINT:
					case TDH_OUTTYPE_IPV4:
						break;
					default:
						if (PrintMappedEventProperty(func,metadata,index,indent,name,*(u32*)p))
							return;
					}

			// TODO: This can definitely be expanded upon (eg, support other OutTypes, etc).
			switch (outType) {
				case TDH_OUTTYPE_BYTE:                  return Print(func,L"%*s%s: %i",indent,L"",name,*(s8*)p);
				case TDH_OUTTYPE_UNSIGNEDBYTE:          return Print(func,L"%*s%s: %u",indent,L"",name,*(u8*)p);
				case TDH_OUTTYPE_SHORT:                 return Print(func,L"%*s%s: %i",indent,L"",name,*(s16*)p);
				case TDH_OUTTYPE_UNSIGNEDSHORT:         return Print(func,L"%*s%s: %u",indent,L"",name,*(u16*)p);
				case TDH_OUTTYPE_INT:                   return Print(func,L"%*s%s: %i",indent,L"",name,*(s32*)p);
				case TDH_OUTTYPE_UNSIGNEDINT:           return Print(func,L"%*s%s: %u",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_LONG:                  return Print(func,L"%*s%s: %" I64FMT "i",indent,L"",name,*(s64*)p);
				case TDH_OUTTYPE_UNSIGNEDLONG:          return Print(func,L"%*s%s: %" I64FMT "u",indent,L"",name,*(u64*)p);
				case TDH_OUTTYPE_FLOAT:                 return Print(func,L"%*s%s: %f",indent,L"",name,*(float*)p);
				case TDH_OUTTYPE_DOUBLE:                return Print(func,L"%*s%s: %f",indent,L"",name,*(double*)p);
				case TDH_OUTTYPE_BOOLEAN:               return Print(func,L"%*s%s: %s",indent,L"",name,*(BOOLEAN*)p?L"TRUE":L"FALSE");
				case TDH_OUTTYPE_HEXINT8:               return Print(func,L"%*s%s: 0x%02x",indent,L"",name,*(u8*)p);
				case TDH_OUTTYPE_HEXINT16:              return Print(func,L"%*s%s: 0x%04x",indent,L"",name,*(u16*)p);
				case TDH_OUTTYPE_HEXINT32:              return Print(func,L"%*s%s: 0x%08x",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_HEXINT64:              return Print(func,L"%*s%s: 0x%016" I64FMT "x",indent,L"",name,*(u64*)p);
				case TDH_OUTTYPE_PID:                   return Print(func,L"%*s%s: %u",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_TID:                   return Print(func,L"%*s%s: %u",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_PORT:                  return Print(func,L"%*s%s: %u",indent,L"",name,(((u16)(*(u8*)p))<<8)+*((u8*)p+1));
				case TDH_OUTTYPE_IPV4:                  return Print(func,L"%*s%s: %u.%u.%u.%u",indent,L"",name,*((u8*)p),*((u8*)p+1),*((u8*)p+2),*((u8*)p+3));
				case TDH_OUTTYPE_ERRORCODE:             return Print(func,L"%*s%s: 0x%08x",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_WIN32ERROR:            return Print(func,L"%*s%s: 0x%08x",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_NTSTATUS:              return Print(func,L"%*s%s: 0x%08x",indent,L"",name,*(u32*)p);
				case TDH_OUTTYPE_HRESULT:               return Print(func,L"%*s%s: 0x%08x",indent,L"",name,*(u32*)p);
				case 0x25/*TDH_OUTTYPE_CODE_POINTER*/:  return Print(func,L"%*s%s: %p",indent,L"",name,*(void**)p);
				case TDH_OUTTYPE_GUID:                  return Print(func,L"%*s%s: " GUID_FMT_STR,indent,L"",name,GUID_FMT_ARGS(*(GUID*)p));

				case TDH_OUTTYPE_STRING:
					switch (prop->nonStructType.InType) {
						case TDH_INTYPE_UNICODESTRING:  return Print(func,L"%*s%s: %s",indent,L"",name,(wchar_t*)p);
						case TDH_INTYPE_ANSISTRING:     return Print(func,L"%*s%s: %S",indent,L"",name,(char*)p);
					}
					break;

				}
			}

			Print(func,L"%*s%s: ?",indent,L"",name);
		}
	}
}

// TRACE_EVENT_INFO::EventNameOffset was added around SDK 10.0.16299.0, but doesn't appear to be any macros to check that, so instead use SFINAE.
template<class T> const wchar_t *GetEventNameHelper(const T *UNUSED_PARAM(info),...) {
	return L"";
}
template<class T> const wchar_t *GetEventNameHelper(const T *info,decltype(info->EventNameOffset)*) {
	const ULONG offset = info->EventNameOffset;
	if (offset && info->DecodingSource==DecodingSourceXMLFile || info->DecodingSource==DecodingSourceTlg)
		return (const wchar_t*)((uptr)info+offset);
	else
		return L"";
}
template<class T> const wchar_t *GetEventName(const T *info) {
	return GetEventNameHelper(info,nullptr);
}

void EtwEventMetadata::PrintEvent(const EtwEventRecord *rec,const LambdaRef<void(const char*)> &func) const {
	const EtwEventMetadataInternal *const self = (const EtwEventMetadataInternal*)this;
	const TRACE_EVENT_INFO *const info = (const TRACE_EVENT_INFO*)((uptr)self+self->m_InfoOffset);

#	define STRING_OFFSET(MEMBER) (info->MEMBER ? (const wchar_t*)((uptr)info+info->MEMBER) : L"")

	Print(func,L"Provider:  " GUID_FMT_STR " %s %s",GUID_FMT_ARGS(info->ProviderGuid),STRING_OFFSET(ProviderNameOffset),STRING_OFFSET(ProviderMessageOffset));
	Print(func,L"Event:     " GUID_FMT_STR " %s %s",GUID_FMT_ARGS(info->EventGuid),GetEventName(info),STRING_OFFSET(EventMessageOffset));
	Print(func, "Id:        0x%04x",info->EventDescriptor.Id);
	Print(func, "Version:   0x%02x",info->EventDescriptor.Version);
	Print(func,L"Opcode:    0x%02x %s",info->EventDescriptor.Opcode,STRING_OFFSET(OpcodeNameOffset));
	Print(func,L"Channel:   0x%02x %s",info->EventDescriptor.Channel,STRING_OFFSET(ChannelNameOffset));
	Print(func,L"Level:     0x%02x %s",info->EventDescriptor.Level,STRING_OFFSET(LevelNameOffset));
	Print(func,L"Task:      0x%04x %s",info->EventDescriptor.Task,STRING_OFFSET(TaskNameOffset));
	Print(func, "Keyword:   0x%016" I64FMT "x",info->EventDescriptor.Keyword);
	if (info->KeywordsNameOffset) {
		const wchar_t *keywordName = (const wchar_t*)((uptr)info+info->KeywordsNameOffset);
		while (*keywordName) {
			Print(func,L"               %s",keywordName);
			keywordName += wcslen(keywordName)+1;
		}
	}

#	undef STRING_OFFSET

	const unsigned numTopLevelProperties = info->TopLevelPropertyCount;
	const void *const dataStart = ((const EVENT_RECORD*)rec)->UserData;
	const void *dataCurr = dataStart;
	for (unsigned i=0; i<numTopLevelProperties; ++i)
		PrintEventProperty(rec,self,dataStart,&dataCurr,i,0,func);
}

u32 EtwGetStack(size_t *dst,u32 dstCount,const EtwEventRecord *rec) {
	u32 numLevels = 0;
	for (u32 i=0; i<rec->ExtendedDataCount; ++i) {
		if (rec->ExtendedData[i].ExtType == EVENT_HEADER_EXT_TYPE_STACK_TRACE64) {
			const u32 srcBytes = rec->ExtendedData[i].DataSize;
			const u32 srcCount = srcBytes/sizeof(size_t);
			u32 si=0,di=0;
			while (si<srcCount && di<dstCount) {
				const size_t s = ((size_t*)(rec->ExtendedData[i].DataPtr))[si++];
#				if ETW_INCLUDE_KERNEL_STACKS
					// There can be zeroes in the stack, so need to remove them for
					// sysStack::PrintCapturedStackTrace to work.
					if (s)
						dst[di++] = s;
#				else
					// Remove all addresses <= 0 (signed comparison).
					if ((ptrdiff_t)s > 0)
						dst[di++] = s;
#				endif
			}
			numLevels = di;
			break;
		}
	}
	sysMemSet(dst+numLevels,0,(dstCount-numLevels)*sizeof(size_t));
	return numLevels;
}

u32 EtwGetProperty(void *dst,u32 dstCount,const EtwEventRecord *rec,const wchar_t *property) {
	PROPERTY_DATA_DESCRIPTOR dataDescriptor = {0};
	dataDescriptor.PropertyName = (ULONGLONG)property;
	dataDescriptor.ArrayIndex = ULONG_MAX;

	ULONG propertySize = 0;
	ULONG status = TdhGetPropertySize((PEVENT_RECORD)rec,0,NULL,1,&dataDescriptor,&propertySize);
	if (status != ERROR_SUCCESS)
	{
		etwErrorf("TdhGetPropertySize failed, 0x%08x", status);
		return 0;
	}

	status = TdhGetProperty((PEVENT_RECORD)rec,0,NULL,1,&dataDescriptor,dstCount,(PBYTE)dst);
	if (status != ERROR_SUCCESS)
	{
		etwErrorf("TdhGetProperty failed, 0x%08x", status);
		return 0;
	}

	return Min(dstCount,(u32)propertySize);
}
#endif //ETW_TDH_SUPPORTED


static void EtwPredicateCmpValidate() {
	CompileTimeAssert((USHORT)EtwPredicateCmp::EQ                           == PAYLOADFIELD_EQ);
	CompileTimeAssert((USHORT)EtwPredicateCmp::NE                           == PAYLOADFIELD_NE);
	CompileTimeAssert((USHORT)EtwPredicateCmp::LE                           == PAYLOADFIELD_LE);
	CompileTimeAssert((USHORT)EtwPredicateCmp::GT                           == PAYLOADFIELD_GT);
	CompileTimeAssert((USHORT)EtwPredicateCmp::LT                           == PAYLOADFIELD_LT);
	CompileTimeAssert((USHORT)EtwPredicateCmp::GE                           == PAYLOADFIELD_GE);
	CompileTimeAssert((USHORT)EtwPredicateCmp::BETWEEN                      == PAYLOADFIELD_BETWEEN);
	CompileTimeAssert((USHORT)EtwPredicateCmp::NOTBETWEEN                   == PAYLOADFIELD_NOTBETWEEN);
	CompileTimeAssert((USHORT)EtwPredicateCmp::MODULO                       == PAYLOADFIELD_MODULO);
	CompileTimeAssert((USHORT)EtwPredicateCmp::CONTAINS                     == PAYLOADFIELD_CONTAINS);
	CompileTimeAssert((USHORT)EtwPredicateCmp::DOESNTCONTAIN                == PAYLOADFIELD_DOESNTCONTAIN);
	CompileTimeAssert((USHORT)EtwPredicateCmp::IS                           == PAYLOADFIELD_IS);
	CompileTimeAssert((USHORT)EtwPredicateCmp::ISNOT                        == PAYLOADFIELD_ISNOT);
	CompileTimeAssert((USHORT)EtwPredicateCmp::INVALID                      == PAYLOADFIELD_INVALID);
}

static void EtwOpcodeValidate() {
	CompileTimeAssert((u8)EtwOpcode::INFO                                   == EVENT_TRACE_TYPE_INFO);
	CompileTimeAssert((u8)EtwOpcode::START                                  == EVENT_TRACE_TYPE_START);
	CompileTimeAssert((u8)EtwOpcode::END                                    == EVENT_TRACE_TYPE_END);
	CompileTimeAssert((u8)EtwOpcode::DC_START                               == EVENT_TRACE_TYPE_DC_START);
	CompileTimeAssert((u8)EtwOpcode::DC_END                                 == EVENT_TRACE_TYPE_DC_END);
}

static void EtwEventRecordValidate() {
	CompileTimeAssert(sizeof(EtwEventDescriptor)                            == sizeof(EVENT_DESCRIPTOR));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Id)                       == offsetof(EVENT_DESCRIPTOR,Id));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Version)                  == offsetof(EVENT_DESCRIPTOR,Version));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Channel)                  == offsetof(EVENT_DESCRIPTOR,Channel));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Level)                    == offsetof(EVENT_DESCRIPTOR,Level));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Opcode)                   == offsetof(EVENT_DESCRIPTOR,Opcode));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Task)                     == offsetof(EVENT_DESCRIPTOR,Task));
	CompileTimeAssert(offsetof(EtwEventDescriptor,Keyword)                  == offsetof(EVENT_DESCRIPTOR,Keyword));

	CompileTimeAssert(sizeof(EtwGuid)                                       == sizeof(GUID));
	CompileTimeAssert(offsetof(EtwGuid,Data1)                               == offsetof(GUID,Data1));
	CompileTimeAssert(offsetof(EtwGuid,Data2)                               == offsetof(GUID,Data2));
	CompileTimeAssert(offsetof(EtwGuid,Data3)                               == offsetof(GUID,Data3));
	CompileTimeAssert(offsetof(EtwGuid,Data4)                               == offsetof(GUID,Data4));

	CompileTimeAssert(sizeof(EtwEventHeader)                                == sizeof(EVENT_HEADER));
	CompileTimeAssert(offsetof(EtwEventHeader,Size)                         == offsetof(EVENT_HEADER,Size));
	CompileTimeAssert(offsetof(EtwEventHeader,HeaderType)                   == offsetof(EVENT_HEADER,HeaderType));
	CompileTimeAssert(offsetof(EtwEventHeader,Flags)                        == offsetof(EVENT_HEADER,Flags));
	CompileTimeAssert(offsetof(EtwEventHeader,EventProperty)                == offsetof(EVENT_HEADER,EventProperty));
	CompileTimeAssert(offsetof(EtwEventHeader,ThreadId)                     == offsetof(EVENT_HEADER,ThreadId));
	CompileTimeAssert(offsetof(EtwEventHeader,ProcessId)                    == offsetof(EVENT_HEADER,ProcessId));
	CompileTimeAssert(offsetof(EtwEventHeader,TimeStamp)                    == offsetof(EVENT_HEADER,TimeStamp));
	CompileTimeAssert(offsetof(EtwEventHeader,ProviderId)                   == offsetof(EVENT_HEADER,ProviderId));
	CompileTimeAssert(offsetof(EtwEventHeader,EventDescriptor)              == offsetof(EVENT_HEADER,EventDescriptor));
// 	CompileTimeAssert(offsetof(EtwEventHeader,KernelTime)                   == offsetof(EVENT_HEADER,KernelTime));                      // -+
// 	CompileTimeAssert(offsetof(EtwEventHeader,UserTime)                     == offsetof(EVENT_HEADER,UserTime));                        //  |- unable to validate nested structs/unions
// 	CompileTimeAssert(offsetof(EtwEventHeader,ProcessorTime)                == offsetof(EVENT_HEADER,ProcessorTime));                   // -+
	CompileTimeAssert(offsetof(EtwEventHeader,ActivityId)                   == offsetof(EVENT_HEADER,ActivityId));

	CompileTimeAssert(sizeof(EtwBufferContext)                              == sizeof(ETW_BUFFER_CONTEXT));
// 	CompileTimeAssert(offsetof(EtwBufferContext,ProcessorNumber)            == offsetof(ETW_BUFFER_CONTEXT,ProcessorNumber));           // -+
// 	CompileTimeAssert(offsetof(EtwBufferContext,Alignment)                  == offsetof(ETW_BUFFER_CONTEXT,Alignment));                 //  |- unable to validate nested structs/unions
// 	CompileTimeAssert(offsetof(EtwBufferContext,ProcessorIndex)             == offsetof(ETW_BUFFER_CONTEXT,ProcessorIndex));            // -+
	CompileTimeAssert(offsetof(EtwBufferContext,LoggerId)                   == offsetof(ETW_BUFFER_CONTEXT,LoggerId));

	CompileTimeAssert(sizeof(EtwEventHeaderExtendedDataItem)                == sizeof(EVENT_HEADER_EXTENDED_DATA_ITEM));
	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,Reserved1)    == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,Reserved1));
	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,ExtType)      == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,ExtType));
// 	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,Linkage)      == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,Linkage));      // -+_ unable to validate bitfields
// 	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,Reserved2)    == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,Reserved2));    // -+
	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,DataSize)     == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,DataSize));
	CompileTimeAssert(offsetof(EtwEventHeaderExtendedDataItem,DataPtr)      == offsetof(EVENT_HEADER_EXTENDED_DATA_ITEM,DataPtr));

	CompileTimeAssert(sizeof(EtwEventRecord)                                == sizeof(EVENT_RECORD));
	CompileTimeAssert(offsetof(EtwEventRecord,EventHeader)                  == offsetof(EVENT_RECORD,EventHeader));
	CompileTimeAssert(offsetof(EtwEventRecord,BufferContext)                == offsetof(EVENT_RECORD,BufferContext));
	CompileTimeAssert(offsetof(EtwEventRecord,ExtendedDataCount)            == offsetof(EVENT_RECORD,ExtendedDataCount));
	CompileTimeAssert(offsetof(EtwEventRecord,UserDataLength)               == offsetof(EVENT_RECORD,UserDataLength));
	CompileTimeAssert(offsetof(EtwEventRecord,ExtendedData)                 == offsetof(EVENT_RECORD,ExtendedData));
	CompileTimeAssert(offsetof(EtwEventRecord,UserData)                     == offsetof(EVENT_RECORD,UserData));
	CompileTimeAssert(offsetof(EtwEventRecord,UserContext)                  == offsetof(EVENT_RECORD,UserContext));

	CompileTimeAssert(sizeof(EtwClassicEventId)								== sizeof(CLASSIC_EVENT_ID));
	CompileTimeAssert(offsetof(EtwClassicEventId, EventGuid)				== offsetof(CLASSIC_EVENT_ID,EventGuid));
	CompileTimeAssert(offsetof(EtwClassicEventId, Type)						== offsetof(CLASSIC_EVENT_ID,Type));
	CompileTimeAssert(offsetof(EtwClassicEventId, Reserved)					== offsetof(CLASSIC_EVENT_ID,Reserved));
}

} // namespace rage

#undef GUID_FMT_ARGS
#undef GUID_FMT_STR

#endif // ETW_SUPPORTED
