// 
// system/taskheader.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_TASKHEADER_H
#define SYSTEM_TASKHEADER_H

#include <stddef.h>		// for size_t

namespace rage {

struct sysTaskHeader;

struct sysTaskBuffer {	// matches order of DMA descriptors on PS3
	size_t Size;		// in bytes
	void *Data;			// pointer to data
};


#define TASK_MAX_USER_DATA	(20)
#define TASK_MAX_READ_ONLY	(4)

#define TASK_EVENT_QUEUE_PORT	(9)				// PS3 specific
#define TASK_SPU_EXTRA_CHECKS (1 && !__FINAL)	// PS3 specific

struct sysTaskParameters {
#if !__SPU
	sysTaskParameters();

	enum SimpleConstructor
	{
		SIMPLE_CONSTRUCTOR
	};
	__forceinline sysTaskParameters(SimpleConstructor) {}
#endif

	sysTaskBuffer Input, Output;				// These can be identical.
	sysTaskBuffer ReadOnly[TASK_MAX_READ_ONLY];	// Optional, up to four of them.
	size_t ReadOnlyCount;						// Number of valid read-only data chunks.
	sysTaskBuffer Scratch;						// Size of scratch area for task (no dynamic memory allocation down on SPU's)
	size_t UserDataCount;
	size_t SpuStackSize;						// SPU tasks/jobs only.  Size of stack, in bytes.  Leave it 0 (default)
												//   and you will get an 8k stack.
	union {
		float asFloat;
		int asInt;
		void *asPtr;
		unsigned int asUInt;
		bool asBool;
	} UserData[TASK_MAX_USER_DATA];
};

#if __PS3
#define SPURS_JOB_START(name)	_binary_job_##name##_job_bin_start
#define SPURS_JOB_SIZE(name)	_binary_job_##name##_job_bin_size
#define SPURS_TASK_START(name)	_binary_task_##name##_task_elf_start
#define SPURS_TASK_SIZE(name)	_binary_task_##name##_task_elf_size
#endif

#if __WIN32 || __PSP2 || RSG_ORBIS
#define DECLARE_TASK_INTERFACE(name)	extern void name(::rage::sysTaskParameters &)
#define RELOAD_TASK_DECLARE(name)
#define RELOAD_TASK_BINARY(name)
#define TASK_INTERFACE(name)			#name, name
#define TASK_INTERFACE_RELOADABLE(name)
#define TASK_INTERFACE_PARAMS			const char *taskName, void (*taskFunc)(rage::sysTaskParameters&)
#define TASK_INTERFACE_PARAMS_PASS		taskName, taskFunc
#elif __DEV
#define DECLARE_TASK_INTERFACE(name)	extern char SPURS_JOB_START(name)[], SPURS_JOB_SIZE(name)[]; extern char* reload_##name##_start; extern char* reload_##name##_size;
#define RELOAD_TASK_DECLARE(name)		char* reload_##name##_start = NULL; char* reload_##name##_size = NULL;
#define RELOAD_TASK_BINARY(name)	\
if (rage::fiStream *elfFile=rage::ASSET.Open("c:\\spu_debug\\"#name".job", "bin")) \
{												\
	unsigned int fileSize = elfFile->Size();	\
	Displayf("Reloaded job binary c:\\spu_debug\\"#name".job.bin, used %d bytes", fileSize); \
	void *elf=memalign(128, fileSize);			\
	elfFile->Read(elf, fileSize);				\
	elfFile->Close();							\
	::reload_##name##_start = (char*)elf;		\
	::reload_##name##_size = (char*)fileSize;	\
}												\
else											\
{												\
	Warningf("Job binary c:\\spu_debug\\"#name".job.bin was not found"); \
}
#define TASK_INTERFACE(name)			#name, SPURS_JOB_START(name), SPURS_JOB_SIZE(name)
#define TASK_INTERFACE_RELOADABLE(name)	#name, reload_##name##_start ? reload_##name##_start : SPURS_JOB_START(name), reload_##name##_size ? reload_##name##_size : SPURS_JOB_SIZE(name)
#define TASK_INTERFACE_PARAMS			const char *taskName,const char *taskStart,const char *taskSize
#define TASK_INTERFACE_PARAMS_PASS		taskName, taskStart, taskSize
#define TASK_INTERFACE_START(name)		(SPURS_JOB_START(name))
#define TASK_INTERFACE_SIZE(name)		(SPURS_JOB_SIZE(name))
#define RELOAD_TASK_INTERFACE_SIZE(name) (::reload_##name##_size? ::reload_##name##_size : SPURS_JOB_SIZE(name))
#else
#define DECLARE_TASK_INTERFACE(name)	extern char SPURS_JOB_START(name)[], SPURS_JOB_SIZE(name)[];
#define RELOAD_TASK_DECLARE(name)
#define RELOAD_TASK_BINARY(name)
#define TASK_INTERFACE(name)			#name, SPURS_JOB_START(name), SPURS_JOB_SIZE(name)
#define TASK_INTERFACE_RELOADABLE(name)	#name, SPURS_JOB_START(name), SPURS_JOB_SIZE(name)
#define TASK_INTERFACE_PARAMS			const char *taskName,const char *taskStart,const char *taskSize
#define TASK_INTERFACE_PARAMS_PASS		taskName, taskStart, taskSize
#define TASK_INTERFACE_START(name)		(SPURS_JOB_START(name))
#define TASK_INTERFACE_SIZE(name)		(SPURS_JOB_SIZE(name))
#define RELOAD_TASK_INTERFACE_SIZE(name) (SPURS_JOB_SIZE(name))
#endif



typedef struct __sysTaskHandle *sysTaskHandle;
typedef void (*sysTaskFunc)(sysTaskParameters&);

#if __WIN32
struct sysPreparedTaskHandle
{
	sysTaskHandle			hdle;
};
#else
struct sysPreparedTaskHandle
{
	void*				JobChainCommandListEAddress;
	void*				JobChainCommandListIndexEAddress;
	void*				JobChainCommandListLockEAddress;
	void*				JobChainEAddress;
	void*				jobToAdd;
	sysTaskHandle		hdle;
	u32					pad[2];  // pad to 32 bytes
};
#endif

// POD to encapsulate task information
struct sysTaskHeader {
	const char *Name;
	sysTaskFunc Func;
	sysTaskParameters Parameters;
};

#if !__SPU
struct sysTaskContext {
	sysTaskContext(TASK_INTERFACE_PARAMS, u32 outputSize = 0, u32 scratchSize = 0, u32 stacksize = 8192);
	sysTaskContext(const sysTaskContext&);
	sysTaskContext& operator = (const sysTaskContext&);
	~sysTaskContext();

	void			DontAssertOnErrors() { ASSERT_ONLY(m_DontAssertOnErrors=true;) }

	bool			IsValid() const;
	void			SetInputOutput();
	void			AddInput(const void* pSrc, u32 size);
	void			AddOutput(size_t size);
	void			AddCacheable(void* pSrc, u32 size);
	void*			AllocUserData(u32 bytes, u32 align = 4);
	sysTaskHandle	Start(int schedulerIndex = 0)       { return TryStartInternal(schedulerIndex, true);  }
	sysTaskHandle	TryStart(int schedulerIndex = 0)    { return TryStartInternal(schedulerIndex, false); }
	void			Cancel();

	int				GetFreeSize() const;
	int				GetInputSize() const;
	int				GetOutputSize() const;
	int				GetScratchSize() const;
	int				GetCodeSize() const;
	int				GetCacheableSize() const;
	int				GetStackSize() const;
	int				GetTotalSize() const;
	bool			FitsOnSpu() const;

	template<class T> T* AllocUserDataAs(u32 count = 1) 
	{
		return (T*)AllocUserData(sizeof(T) * count, __alignof(T));
	}

private:
	void			Init();
	sysTaskHandle   TryStartInternal(int schedulerIndex, bool canAssert);

	sysTaskHandle	m_Handle;
	u32				m_CacheableSize;
	u16				m_OutputIndex;
	u16				m_UserData : 1;
	u16				m_AddedOutput : 1;
	u16				m_Started : 1;
#if __ASSERT
	u16				m_DontAssertOnErrors : 1;
#endif
};
#endif

}	// namespace rage

#endif
