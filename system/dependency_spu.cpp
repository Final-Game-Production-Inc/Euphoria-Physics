//
// system/dependency_spu.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#include "dependency_config.h"

#define SYS_COMPILE_SPU_DEPENDENCY (SYS_USE_SPU_DEPENDENCY && __SPU)
#define SYS_SPU_DEPENDENCY_MEMORY_GUARD (__DEV)

#if SYS_COMPILE_SPU_DEPENDENCY

#include "dependency.h"
#include "dependency_spu.h"
#include "dependencyscheduler.h"

#include "math/amath.h"
#include "system/timer.h"
#include "vectormath/vectormath.h"

using namespace rage;
using namespace Vec;

#if __SPU
#include "dependencyscheduler.cpp"
#include "system/tinyheap.cpp"
#endif // __SPU

////////////////////////////////////////////////////////////////////////////////

#if !__FINAL
extern "C" const qword __SPU_GUID[1];

static void putUserStartPacket(sysDependency::Callback* cb)
{
	// stop scheduler trace
	static CellSpursTracePacket stop_packet;
	stop_packet.header.tag = CELL_SPURS_TRACE_TAG_STOP;
	AssertVerify(!cellSpursGetSpuGuid(__SPU_GUID, &stop_packet.data.guid));
	cellSpursPutTrace(&stop_packet, CELL_SPURS_KERNEL_DMA_TAG_ID);

	// start user trace
	static CellSpursTracePacket start_packet;
	start_packet.header.tag = CELL_SPURS_TRACE_TAG_START;
	start_packet.data.start.module[0] = 'U';
	start_packet.data.start.module[1] = 'n';
	start_packet.data.start.module[2] = 'i';
	start_packet.data.start.module[3] = 't';
	start_packet.data.start.ls = (uint16_t)((uint32_t)cb >> 2);
	start_packet.data.start.level = 2;
	cellSpursPutTrace( &start_packet, CELL_SPURS_KERNEL_DMA_TAG_ID);
}

////////////////////////////////////////////////////////////////////////////////

static uint64_t getUserGuid(sysDependency::Callback* cb)
{
	const vec_uchar16 guidShufflePattern = {0,1,4,5,8,9,12,13,0,1,4,5,8,9,12,13};
	qword insn = si_roti( *(const qword*)cb, 7 );
	return si_to_ullong( si_shufb( insn, insn, (const qword)guidShufflePattern ) );
}

////////////////////////////////////////////////////////////////////////////////

static void putUserStopPacket(sysDependency::Callback* cb)
{
	// stop user trace
	static CellSpursTracePacket stop_packet;
	stop_packet.header.tag = CELL_SPURS_TRACE_TAG_STOP;
	stop_packet.data.guid = getUserGuid(cb);
	cellSpursPutTrace( &stop_packet, CELL_SPURS_KERNEL_DMA_TAG_ID);

	// start scheduler trace
	static CellSpursTracePacket start_packet;
	start_packet.header.tag = CELL_SPURS_TRACE_TAG_START;
	start_packet.data.start.level = 1;
	start_packet.data.start.ls = (uint16_t)((uint32_t)__SPU_GUID >> 2);
	start_packet.data.start.module[0] = 'D';
	start_packet.data.start.module[1] = 'e';
	start_packet.data.start.module[2] = 'p';
	start_packet.data.start.module[3] = ' ';
	cellSpursPutTrace(&start_packet, CELL_SPURS_KERNEL_DMA_TAG_ID);
}
#endif // !__FINAL

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Software cache to reduce memory usage and DMA transfers
// Use hash table to speed-up search 
struct SoftwareCache
{
	static const u32 sm_NumEntries = 32;
	static u32 Hash(const void* ea);

	void Init(u8* scratchBuffer, u32 size);
	bool Lock(void*& outPtr, u32 size, u32 tag, u32 input);
	void Unlock(void* ea, sysDependency::Callback* cb);

	struct Entry
	{
		Entry* m_Next;
		void* m_Ea;
		void* m_Ls;
		u32 m_RefCount;
		u32 m_Size;
	};

	Entry m_Pool[sm_NumEntries];
	Entry* m_Buckets[sm_NumEntries];
	Entry* m_Free;
	sysTinyHeap m_Allocator;
};

////////////////////////////////////////////////////////////////////////////////

void SoftwareCache::Init(u8* scratchBuffer, u32 size)
{
	m_Allocator.Init(scratchBuffer, size);
	m_Free = m_Pool;
	for(u32 i=0; i < sm_NumEntries-1; i++)
	{
		m_Pool[i].m_Next = &m_Pool[i+1];  
		m_Buckets[i] = NULL;
	}
	m_Pool[sm_NumEntries-1].m_Next = NULL;
	m_Buckets[sm_NumEntries-1] = NULL;

#if SYS_SPU_DEPENDENCY_MEMORY_GUARD
	*reinterpret_cast<Vector_4V*>(m_Allocator.GetHeapEnd()) = V4VConstant(V_FLT_MAX);
#endif
}

////////////////////////////////////////////////////////////////////////////////

__forceinline u32 SoftwareCache::Hash(const void* ea)
{
	uptr key = uptr(ea);
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);
	return key % sm_NumEntries;
}

////////////////////////////////////////////////////////////////////////////////

bool SoftwareCache::Lock(void*& outPtr, u32 size, u32 tag, u32 input)
{
	void* ea = outPtr;
	if(Unlikely(!ea))
		return true;

	// find exiting entry
	Entry** bucket = m_Buckets + Hash(ea);
	Entry* first = *bucket;
	for(Entry* curr = first; curr != NULL; curr = curr->m_Next)
	{
		if(curr->m_Ea == ea)
		{
			Assert(RAGE_ALIGN(size, 4)==curr->m_Size);
			curr->m_RefCount++;
			outPtr = curr->m_Ls;
			return true;
		}
	}

	// create new entry
	Entry* entry = m_Free;
	if(Unlikely(!entry))
		return false;

	u32 size16 = RAGE_ALIGN(size, 4);
	u32 padding = SYS_SPU_DEPENDENCY_MEMORY_GUARD ? 2*sizeof(Vector_4V) : 0;
	u8* ls = reinterpret_cast<u8*>(m_Allocator.Allocate(size16 + padding));
	if(Unlikely(!ls))
		return false;

#if SYS_SPU_DEPENDENCY_MEMORY_GUARD
	Vector_4V* beginStamp = reinterpret_cast<Vector_4V*>(ls);
	Vector_4V* endStamp = reinterpret_cast<Vector_4V*>(ls + sizeof(Vector_4V) + size16);
	*beginStamp = *endStamp = V4VConstant(V_FLT_MAX);
	ls += sizeof(Vector_4V);
#endif

	if(Likely(input))
		sysMemLargeGet(ls, uptr(ea), size16, tag);

	m_Free = entry->m_Next;
	entry->m_Size = size16;
	entry->m_Ea = ea;
	entry->m_Ls = ls;
	entry->m_RefCount = 1;
	entry->m_Next = first;
	*bucket = entry;
	outPtr = ls;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void SoftwareCache::Unlock(void* ea, sysDependency::Callback* cb)
{
	if(Likely(!ea))
		return;

#if SYS_SPU_DEPENDENCY_MEMORY_GUARD
	const Vector_4V* heapEnd = static_cast<const Vector_4V*>(m_Allocator.GetHeapEnd());
	if(!V4IsEqualIntAll(*heapEnd, V4VConstant(V_FLT_MAX)))
	{
		Quitf("Stack overflow at 0x%p on guid 0x%llX", heapEnd, getUserGuid(cb));
	}
#endif

	Entry** bucket = m_Buckets + Hash(ea);
	Entry** prevNext = bucket;
	for(Entry* curr = *bucket; curr != NULL; curr = curr->m_Next)
	{
		if(curr->m_Ea == ea)
		{
			if(--curr->m_RefCount == 0)
			{
				u8* ptr = (u8*)curr->m_Ls;

#if SYS_SPU_DEPENDENCY_MEMORY_GUARD
				ptr -= sizeof(Vector_4V);
				const Vector_4V* beginStamp = reinterpret_cast<const Vector_4V*>(ptr);
				const Vector_4V* endStamp = reinterpret_cast<const Vector_4V*>(ptr + sizeof(Vector_4V) + curr->m_Size);
				if(!V4IsEqualIntAll(*beginStamp, V4VConstant(V_FLT_MAX)))
				{
					Quitf("Heap underrun at 0x%p on guid 0x%llX", ea, getUserGuid(cb));
				}
				if(!V4IsEqualIntAll(*endStamp, V4VConstant(V_FLT_MAX)))
				{
					Quitf("Heap overrun at 0x%p on guid 0x%llX", ea, getUserGuid(cb));
				}
#endif

				m_Allocator.Free(ptr);
				*prevNext = curr->m_Next;
				curr->m_Next = m_Free;
				m_Free = curr;
			}
			return;
		}
		prevNext = &curr->m_Next;
	}
}

////////////////////////////////////////////////////////////////////////////////

// PURPOSE: Software pipelining with a 4 stages pipe.
// Stages :
// 1) Read instruction from the scheduler
// 2) Allocate and read instruction data
// 3) Execute instruction, write data and read parents
// 4) Free instructions data
// Possible hazards :
// 1) No new instruction available
// 2) Not enough memory available for the instruction data
struct SoftwarePipeline
{
	void Init(u64** counters, u8* scratchBuffer, u32 size);
	void Execute();

private:
	void Stage0(u32);
	void Stage1(u32);
	void Stage2(u32);
	void Stage3(u32);
	void Stage4(u32);

	bool IsEmpty() const;

	static const u32 sm_ChainIdx = sysDependency::kChainSpu;
	static const u32 sm_NumStages = SYS_DEPENDENCY_NUM_PIPELINE_STAGES;
	struct Instruction
	{
		sysDependency m_Dependency;
		sysDependency m_Parents[sysDependency::sm_MaxNumParents];
		sysDependency* m_DependencyPPU;
		sysDependency::Callback* m_CodeBuffer;
		void* m_DataBuffers[sysDependency::sm_MaxNumData];
		bool m_Result;
		bool m_Active;
	};

	SoftwareCache m_Cache;
	Instruction m_Instructions[sm_NumStages];
	u64** m_Counters;
};

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Init(u64** counters, u8* scratchBuffer, u32 size)
{
	m_Counters = counters;
	m_Cache.Init(scratchBuffer, size);
	for(u32 i=0; i < SoftwarePipeline::sm_NumStages; i++)
	{
		m_Instructions[i].m_Active = false;
	}
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Execute()
{
	u32 i=0;
	do
	{
		Stage4(i);
		Stage3(i+1);
		Stage2(i+2);
		Stage1(i+3);
		Stage0(i+4);

		i++;

	} while (!IsEmpty());
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Stage0(u32 count)
{
	u32 pipeline = count % sm_NumStages;
	u32 tag = SYS_DEPENDENCY_PIPELINE_0_DMA_TAG + pipeline;
	Instruction& instr = m_Instructions[pipeline];
	if(!instr.m_Active)
	{
		sysDependency* dependencyPPU = sysDependencyScheduler::Fetch(sm_ChainIdx);
		if(dependencyPPU)
		{
			instr.m_Active = true;
			instr.m_DependencyPPU = dependencyPPU;
			sysMemGet(&instr.m_Dependency, uptr(dependencyPPU), sizeof(sysDependency), tag);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Stage1(u32 count)
{
	u32 pipeline = count % sm_NumStages;
	u32 tag = SYS_DEPENDENCY_PIPELINE_0_DMA_TAG + pipeline;
	Instruction& instr = m_Instructions[pipeline];
	if(instr.m_Active)
	{
		sysMemWait(1<<tag);
		sysDependency& dep = instr.m_Dependency;

		// lock code and data
		u32 flags = dep.m_Flags;
		u32 params = flags | (flags >> sysDependency::sm_MaxNumData);
		instr.m_CodeBuffer = dep.m_Callback;
		bool success = m_Cache.Lock(reinterpret_cast<void*&>(dep.m_Callback), dep.m_CodeSize, tag, true);
		for(u32 i=0, bit=1; i < sysDependency::sm_MaxNumData; i++, bit<<=1)
		{
			if(params & bit)
			{
				instr.m_DataBuffers[i] = dep.m_Params[i].m_AsPtr;
				success &= m_Cache.Lock(dep.m_Params[i].m_AsPtr, dep.m_DataSizes[i], tag, flags & bit);
			}
			else
			{
				instr.m_DataBuffers[i] = NULL;
			}
		}

		u32 alloc = flags & sysDepFlag::ALLOC0;
		if(alloc)
		{
			void* ptr = m_Cache.m_Allocator.Allocate(dep.m_DataSizes[0]);
			dep.m_Params[0].m_AsPtr = ptr;
			success &= ptr != NULL;
		}

		if(!success)
		{
			// free allocated buffers
			sysMemWait(1<<tag);
			if(alloc)
			{
				m_Cache.m_Allocator.Free(dep.m_Params[0].m_AsPtr);
			}
			m_Cache.Unlock(reinterpret_cast<void*>(instr.m_CodeBuffer), dep.m_Callback);
			for(u32 i=0; i < sysDependency::sm_MaxNumData; i++)
			{
				m_Cache.Unlock(instr.m_DataBuffers[i], dep.m_Callback);
			}

			// If we detect a 4th failure, this should catch it and perform a fatal assert.
			FatalAssert(instr.m_Dependency.m_JobFailureCount < 3);
			ASSERT_ONLY(instr.m_Dependency.m_JobFailureCount = (u8) rage::Min(instr.m_Dependency.m_JobFailureCount + 1, 3);)

			// re-insert the dependency
			sysDependencyScheduler::InsertInternal(sm_ChainIdx, instr.m_Dependency.m_Priority, instr.m_DependencyPPU);
			instr.m_Active = false;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Stage2(u32)
{
	// empty stage to delay the wait on inputs
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Stage3(u32 count)
{
	u32 pipeline = count % sm_NumStages;
	u32 tag = SYS_DEPENDENCY_PIPELINE_0_DMA_TAG + pipeline;
	Instruction& instr = m_Instructions[pipeline];
	if(instr.m_Active)
	{
		// wait for input transfers
		sysMemWait(1<<tag);

		SPU_ONLY(spu_sync_c());

		sysDependency& dep = instr.m_Dependency;
		sysDependency::Callback* callback = __SPU ? dep.m_Callback : instr.m_CodeBuffer;
		NOTFINAL_ONLY(putUserStartPacket(callback));
		if(m_Counters && dep.m_Id)
		{
 			u64* counter = static_cast<u64*>(sysInterlockedReadPointer(reinterpret_cast<void**>(m_Counters + dep.m_Id)));
 			sysTimer timer;
			instr.m_Result = callback(dep);
 			sysInterlockedAdd(counter, s64(timer.GetTickTime()));
		}
		else
 		{
 			instr.m_Result = callback(dep);
 		}
		NOTFINAL_ONLY(putUserStopPacket(callback));

		// release temporary buffer
		if(dep.m_Flags & sysDepFlag::ALLOC0)
		{
			m_Cache.m_Allocator.Free(dep.m_Params[0].m_AsPtr);
		}

		// write dependency output back
		u32 outputs = dep.m_Flags >> sysDependency::sm_MaxNumData;
		for(u32 i=0, bit=1; i < sysDependency::sm_MaxNumData; i++, bit<<=1)
		{
			if(dep.m_Params[i].m_AsPtr && (outputs & bit))
			{
				Assert(instr.m_DataBuffers[i] && dep.m_DataSizes[i] > 0);
				sysMemLargePut(dep.m_Params[i].m_AsPtr, uptr(instr.m_DataBuffers[i]), dep.m_DataSizes[i], tag);
			}
		}

		// get parents
		for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
		{
			if(dep.m_Parents[i])
			{
				sysMemGet(&instr.m_Parents[i], uptr(dep.m_Parents[i]), sizeof(sysDependency), tag);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void SoftwarePipeline::Stage4(u32 count)
{
	u32 pipeline = count % sm_NumStages;
	u32 tag = SYS_DEPENDENCY_PIPELINE_0_DMA_TAG + pipeline;
	Instruction& instr = m_Instructions[pipeline];
	if(instr.m_Active)
	{
		// wait for output transfers
		sysMemWait(1<<tag);

		// free allocated buffers
		m_Cache.Unlock(reinterpret_cast<void*>(instr.m_CodeBuffer), instr.m_Dependency.m_Callback);
		for(u32 i=0; i < sysDependency::sm_MaxNumData; i++)
		{
			m_Cache.Unlock(instr.m_DataBuffers[i], instr.m_Dependency.m_Callback);
		}

		instr.m_Active = false;

		// remove the dependency from the scheduler
		if(instr.m_Result)
		{
			sysDependency* parents[sysDependency::sm_MaxNumParents];
			for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
			{
				parents[i] = &instr.m_Parents[i];
			}
			u32 priority = instr.m_Dependency.m_Priority;

			// steal dependency and assign to next fetch stage
			u32 parentIdx = sysDependencyScheduler::Steal(sm_ChainIdx, priority, parents, instr.m_Dependency.m_Parents);
			if(parentIdx != sysDependency::sm_MaxNumParents)
			{
				Instruction& stage0 = m_Instructions[(count+sm_NumStages-1)%sm_NumStages];
				stage0.m_Dependency = instr.m_Parents[parentIdx];
				stage0.m_DependencyPPU = instr.m_Dependency.m_Parents[parentIdx];
				stage0.m_Active = true;
			}
		}
		else
		{
			u32 priority = instr.m_Dependency.m_Priority;
			u32 reducePriority = priority != 0 ? priority-1 : priority;
			sysDependencyScheduler::InsertInternal(sm_ChainIdx, reducePriority, instr.m_DependencyPPU);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

bool SoftwarePipeline::IsEmpty() const
{
	// check if any active instruction
	for(u32 i=0; i < sm_NumStages; i++)
	{
		if(m_Instructions[i].m_Active)
		{
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

#endif // SYS_COMPILE_SPU_DEPENDENCY

