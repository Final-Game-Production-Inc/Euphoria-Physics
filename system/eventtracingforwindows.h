//
// system/eventtracingforwindows.h
//
// Copyright (C) 2018-2018 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_ETW_H
#define SYSTEM_ETW_H

#define ETW_SUPPORTED   (0 && (RSG_WIN32 || RSG_XBOX) && !RSG_FINAL && !RSG_TOOL)

#if ETW_SUPPORTED

// Sadly, XB1 doesn't support the TDH library required to enable this code,
// so for now at least, this is PC only.
#define ETW_TDH_SUPPORTED (RSG_PC)

#include "system/lambda.h"

#include <string.h>
#include <wchar.h>

namespace rage {

// These match the PAYLOADFIELD_* values, but are listed here to avoid including Windows header files.
// The values are compile time checked in etw.cpp.
enum class EtwPredicateCmp : u16 {
	EQ              = 0,
	NE              = 1,
	LE              = 2,
	GT              = 3,
	LT              = 4,
	GE              = 5,
	BETWEEN         = 6,
	NOTBETWEEN      = 7,
	MODULO          = 8,
	CONTAINS        = 20,
	DOESNTCONTAIN   = 21,
	IS              = 30,
	ISNOT           = 31,
	INVALID         = 32,
};

// These match the EVENT_TRACE_TYPE_* values.
enum class EtwOpcode : u8 {
	INFO            = 0,
	START           = 1,
	END             = 2,
	DC_START        = 3,
	DC_END          = 4,
};


// Forward declaration of types that mirror ETW native types.  Used to avoid including Windows header files.
// Definition lower down in this header to avoid clutter.
struct EtwEventRecord;
struct EtwGuid;
struct EtwClassicEventId;

enum class EtwEventId : u32 {
	INVALID         = ~0u,
};

typedef void (__stdcall *EtwEventRecordCallback)(const EtwEventRecord*);

class EtwSession {
public:
	EtwSession() = delete;
	~EtwSession() = delete;
	EtwSession(const EtwSession&) = delete;
	EtwSession &operator=(const EtwSession&) = delete;

	static EtwSession *Create(const wchar_t *name);
	static EtwSession *CreateKernelSession(u32 enableFlags);
	void Destroy();

#if ETW_TDH_SUPPORTED
	// Start adding a provider.
	bool BeginAddProvider(const EtwGuid &guid);

	// Enable an event from the current provider.
	EtwEventId AddEvent(const wchar_t *taskName,EtwOpcode opcode);

	// Enable stack traces for the current event.
	void EnableStackTrace();

	// Require all predicates on the current event to match.
	void PredicatesMatchAny(bool matchAny);

	// Add a predicate to the current event.
	//
	// NOTE: Predicates require Windows 8.1 to work, so calling code cannot
	// assume only packets matching the predicate will be recieved.  As such,
	// predicates should be used purely as an optimization to try and cut down
	// on ETW traffic.
	//
	void AddPredicate(const wchar_t *fieldName,EtwPredicateCmp compareOp,const wchar_t *value);

	// Finish adding a provider.
	void EndAddProvider();
#endif //ETW_TDH_SUPPORTED

	// Set Sampling Frequency
	static void SetKernelSamplingFrequency(u32 frequency = 1000);

	// Enable stack traces for the selected event.
	void EnableStackTrace(const EtwClassicEventId* eventIds, int count);

	// Start trace once all providers have been added.
	void Start(EtwEventRecordCallback callback = nullptr);

	// To be called on a worker thread.  Blocks until something to do.
	void Process();
	void Process(const LambdaRef<void(const EtwEventRecord*)> &callback);

	// Forces all buffered events to be delivered to consumer thread.
	void Flush();
};

#if ETW_TDH_SUPPORTED
// Helper to extract a specific value from an event.
// Calculating this can be expensive, so should be created once and then cached.
class EtwEventExtractorBase {
public:
	EtwEventExtractorBase() = delete;
	~EtwEventExtractorBase() = delete;
	EtwEventExtractorBase(const EtwEventExtractorBase&) = delete;
	EtwEventExtractorBase &operator=(const EtwEventExtractorBase) = delete;

	void Destroy();

protected:
	u64 GetBase(const EtwEventRecord *rec) const;
};
template<class T>
class EtwEventExtractor: public EtwEventExtractorBase {
public:
	EtwEventExtractor() = delete;
	~EtwEventExtractor() = delete;
	EtwEventExtractor(const EtwEventExtractor&) = delete;
	EtwEventExtractorBase &operator=(const EtwEventExtractorBase) = delete;

	T Get(const EtwEventRecord *rec) const {
		return (T)GetBase(rec);
	}
};

// Metadata describing an event.  Generally should be created once per event/opcode and then cached.
class EtwEventMetadata {
public:
	EtwEventMetadata() = delete;
	~EtwEventMetadata() = delete;
	EtwEventMetadata(const EtwEventMetadata&) = delete;
	EtwEventMetadata &operator=(const EtwEventMetadata&) = delete;

	static EtwEventMetadata *Create(const EtwEventRecord *templateRec);
	void Destroy();

	template<class T> EtwEventExtractor<T> *CreateEventExtractor(const wchar_t *property) const {
		return (EtwEventExtractor<T>*)CreateEventExtractorInternal(property,sizeof(T));
	}

	void PrintEvent(const EtwEventRecord *rec,const LambdaRef<void(const char*)> &func=[](const char *OUTPUT_ONLY(str)){Displayf("%s",str);}) const;

private:
	void *CreateEventExtractorInternal(const wchar_t *property,size_t numBytes) const;
};
#endif //ETW_TDH_SUPPORTED

// Extract callstack from an event record.
// Returns the number of stack entries writen.
// Remainder of buffer will be filled with zeroes.
u32 EtwGetStack(size_t *dst,u32 dstCount,const EtwEventRecord *rec);

#if ETW_TDH_SUPPORTED
// Slow path to retrieve raw data from the record by property name.
// Please don't use this function for runtime critical code.
// The fast approach is based on EtwEventExtractor class.
// This function is useful for some tricky cases when it's impossible to express the offset with Extractor.
// NOTE: Returns the size of the property copied to the buffer.
u32 EtwGetProperty(void *dst, u32 dstCount, const EtwEventRecord *rec, const wchar_t *property);
#endif //ETW_TDH_SUPPORTED


// Minimal structures to match binary layout of EVENT_RECORD.  Use this in
// preference to pulling in all the Windows header files in client code.
// Basically a copy-paste of evntprov.h in Windows SDK.
struct EtwEventDescriptor {
	u16                             Id;
	u8                              Version;
	u8                              Channel;
	u8                              Level;
	EtwOpcode                       Opcode;
	u16                             Task;
	u64                             Keyword;
};
struct EtwGuid {
	u32                             Data1;
	u16                             Data2;
	u16                             Data3;
	u8                              Data4[8];
	EtwGuid() {
	}
	EtwGuid(u32 a,u16 b,u16 c,u16 d,u64 e);
	bool operator==(const EtwGuid &rhs) const {
		return memcmp(this,&rhs,sizeof(*this)) == 0;
	}
	bool operator!=(const EtwGuid &rhs) const {
		return !operator==(rhs);
	}
	static EtwGuid INVALID;
};
struct EtwClassicEventId {
	EtwGuid							EventGuid;
	u8								Type;
	u8								Reserved[7];
};
struct EtwEventHeader {
	u16                             Size;                   // Event Size
	u16                             HeaderType;             // Header Type
	u16                             Flags;                  // Flags
	u16                             EventProperty;          // User given event property
	u32                             ThreadId;               // Thread Id
	u32                             ProcessId;              // Process Id
	u64                             TimeStamp;              // Event Timestamp
	EtwGuid                         ProviderId;             // Provider Id
	EtwEventDescriptor              EventDescriptor;        // Event Descriptor
	union {
		struct {
			u32                     KernelTime;             // Kernel Mode CPU ticks
			u32                     UserTime;               // User mode CPU ticks
		} DUMMYSTRUCTNAME;
		u64                         ProcessorTime;          // Processor Clock
		// for private session events
	} DUMMYUNIONNAME;
	EtwGuid                         ActivityId;             // Activity Id
};
struct EtwBufferContext {
	union {
		struct {
			u8                      ProcessorNumber;
			u8                      Alignment;
		} DUMMYSTRUCTNAME;
		u16                         ProcessorIndex;
	} DUMMYUNIONNAME;
	u16                             LoggerId;
};
struct EtwEventHeaderExtendedDataItem {
	u16                             Reserved1;              // Reserved for internal use
	u16                             ExtType;                // Extended info type
	struct {
		u16                         Linkage     :  1;       // Indicates additional extended
		// data item
		u16                         Reserved2   : 15;
	};
	u16                             DataSize;               // Size of extended info data
	u64                             DataPtr;                // Pointer to extended info data
};
struct EtwEventRecord {
	EtwEventHeader                  EventHeader;            // Event header
	EtwBufferContext                BufferContext;          // Buffer context
	u16                             ExtendedDataCount;      // Number of extended
	// data items
	u16                             UserDataLength;         // User data length
	EtwEventHeaderExtendedDataItem *ExtendedData;           // Pointer to an array of extended data items
	void                           *UserData;               // Pointer to user data
	void                           *UserContext;            // Context from OpenTrace
};

} // namespace rage

#endif // ETW_SUPPORTED

#endif // SYSTEM_ETW_H
