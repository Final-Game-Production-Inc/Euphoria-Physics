// 
// system/dmaplan.h 
// 
// Copyright (C) 1999-2010 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DMAPLAN_H
#define SYSTEM_DMAPLAN_H 

#if __PS3

#include "atl/bitset.h"
#include "math/simplemath.h"
#include "system/dma.h"
#include <cell/dma.h>

#if __SPU
#define DMA_PLAN_ARGS(X) sysDmaPlan& dmaPlan, X* thisMm

// Begin just adds the object to the DMA list without causing a fixup, so it's only useful for the top level object in the plan
#define DMA_BEGIN()						dmaPlan.AddObject(thisMm, false)

// Begin just adds the object to the DMA list without causing a fixup, so it's only useful for the top level object in the plan
#define DMA_BEGIN_READ_ONLY()						dmaPlan.AddObject(thisMm, true)

// For your top level object if you don't need the whole thing
#define DMA_BEGIN_PARTIAL(X, Y)			dmaPlan.AddPartialObject(thisMm, X, Y, false)

// For your top level object if you don't need the whole thing
#define DMA_BEGIN_PARTIAL_READ_ONLY(X, Y)			dmaPlan.AddPartialObject(thisMm, X, Y, true)

// For when you have a pointer that needs to be fixed up but someone else already added it to the DMA list (i.e. someone else owns it)
#define ADD_DMA_REF(X)					do { if (X) { dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be DMAed
#define ADD_DMA_OWNER(X)				do { if (X) { dmaPlan.AddObject(X, false); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be DMAed
#define ADD_DMA_OWNER_READ_ONLY(X)		do { if (X) { dmaPlan.AddObject(X, true); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be partially DMAed
#define ADD_DMA_OWNER_PARTIAL(X, Y, Z)	do { if (X) { dmaPlan.AddPartialObject(X,Y,Z, false); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be partially DMAed
#define ADD_DMA_OWNER_PARTIAL_READ_ONLY(X, Y, Z)	do { if (X) { dmaPlan.AddPartialObject(X,Y,Z, true); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer to an array of objects that you own
#define ADD_DMA_ARRAY_OWNER(X, cnt)		do { if (X) { dmaPlan.AddArray(X, cnt, false); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

// For when you have a pointer to an array of objects that you own
#define ADD_DMA_ARRAY_OWNER_READ_ONLY(X, cnt)		do { if (X) { dmaPlan.AddArray(X, cnt, true); dmaPlan.AddFixup((void**)&(X), (void**)&(thisMm->X)); } } while (0)

#else // __SPU
#define DMA_PLAN_ARGS(X) sysDmaPlan& dmaPlan

// Begin just adds the object to the DMA list without causing a fixup, so it's only useful for the top level object in the plan
#define DMA_BEGIN()						dmaPlan.AddObject(this, false)

// Begin just adds the object to the DMA list without causing a fixup, so it's only useful for the top level object in the plan
#define DMA_BEGIN_READ_ONLY()						dmaPlan.AddObject(this, true)

// For your top level object if you don't need the whole thing
#define DMA_BEGIN_PARTIAL(X, Y)			dmaPlan.AddPartialObject(this, X, Y, false)

// For your top level object if you don't need the whole thing
#define DMA_BEGIN_PARTIAL_READ_ONLY(X, Y)			dmaPlan.AddPartialObject(this, X, Y, true)

// For when you have a pointer that needs to be fixed up but someone else already added it to the DMA list (i.e. someone else owns it)
#define ADD_DMA_REF(X)					do { if (X) { dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be DMAed
#define ADD_DMA_OWNER(X)				do { if (X) { dmaPlan.AddObject(X, false); dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be DMAed
#define ADD_DMA_OWNER_READ_ONLY(X)				do { if (X) { dmaPlan.AddObject(X, true); dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be partially DMAed
#define ADD_DMA_OWNER_PARTIAL(X, Y, Z)	do { if (X) { dmaPlan.AddPartialObject(X,Y,Z,false); dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer that needs to be fixed up, pointing to an object that needs to be partially DMAed
#define ADD_DMA_OWNER_PARTIAL_READ_ONLY(X, Y, Z)	do { if (X) { dmaPlan.AddPartialObject(X,Y,Z,true); dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer to an array of objects that you own
#define ADD_DMA_ARRAY_OWNER(X, cnt)		do { if (X) { dmaPlan.AddArray(X, cnt, false); dmaPlan.AddFixup((void**)&(X)); } } while (0)

// For when you have a pointer to an array of objects that you own
#define ADD_DMA_ARRAY_OWNER_READ_ONLY(X, cnt)		do { if (X) { dmaPlan.AddArray(X, cnt, true); dmaPlan.AddFixup((void**)&(X)); } } while (0)
#endif

namespace rage {

class sysDmaPlan
{
public:
	sysDmaPlan()
		: m_Fixups(NULL)
		, m_NumFixups(0)
		, m_DmaList(NULL)
		, m_NumDmas(0)
		, m_ReadOnly(NULL)
		, m_SpuAlloc(0)
		, m_MaxDmas(0)
		, m_MaxFixups(0)
	{
	}

	struct ObjectInfo
	{
		u32 spuPtr;
		u32 ppuSize;
		u32 ppuPtr;
	};

#if __SPU
	void Initialize(sysDmaPlan* ppuAddr);
#else
	void Initialize();
#endif

	template <class T>
	void AddObject(T* ptr, bool readOnly);

	template <class T>
	void AddPartialObject(T* ptr, void* start, size_t size, bool readOnly);

	template <class T>
	void AddPartialObject(T* ptr, void* start, void* end, bool readOnly);

	template <class T>
	void AddArray(T* ptr, int count, bool readOnly);

	u32 FindSpuOffset(u32 ptr);

	u32 GetSpuSize()
	{
		return m_SpuAlloc;
	}

	void AddFixup(void** ptr);

#if __SPU
	void AddFixup(void** lsPtr, void** mmPtr);
#endif

	void Finalize();

#if !__FINAL
	void PrintDmaList();
#endif

#if __SPU
	void RelocateToLs();
	void RelocateToMm();
	void KickDMAGetFunc(void* ls, u32 tag SYS_DMA_FILELINE_ARGS);
	void KickDMAPutFunc(void* ls, u32 tag SYS_DMA_FILELINE_ARGS);
	void FixupPointersToLs(void* ls);
	void FixupPointersToMm(void* ls);
#endif

	// Arbitrary limit to catch bad data
	static const u32 MAX_NUM_DMAS = 256;

	struct Fixup
	{
		u32 pointer;
		u32 referent;
	};

	sysDmaPlan* m_PpuAddr;
	Fixup* m_Fixups;
	u32 m_NumFixups;
	CellDmaListElement* m_DmaList;
	u32 m_NumDmas;
	atBitSet* m_ReadOnly;
	u32 m_SpuAlloc;
	u32 m_MaxDmas;
	u32 m_MaxFixups;

	static ObjectInfo sm_ObjectInfos[MAX_NUM_DMAS];
};

template <typename _Type, size_t _SizeAlign>
void AssertSizeAlignHelper()
{
	CompileTimeAssert(_SizeAlign == 0);
};

#define AssertSizeAlign(x, align) AssertSizeAlignHelper<x, sizeof(x) & (align - 1)>()

template <class T>
void sysDmaPlan::AddObject(T* ptr, bool readOnly)
{
	Assert(ptr);
	Assert(m_NumDmas < m_MaxDmas);
	Assert(m_NumDmas < MAX_NUM_DMAS);
	Assert(((u32)ptr & 0xf) == 0); // Doesn't work for objects not 16 byte aligned
	AssertSizeAlign(T, 16);

	u32 objectIndex = m_NumDmas++;
	CellDmaListElement& cdle = m_DmaList[objectIndex];
	cdle.size = sizeof(T);
	cdle.eal = u32(ptr);
	cdle.reserved = 0;
	cdle.notify = 0;

	ObjectInfo& objectInfo = sm_ObjectInfos[objectIndex];
	objectInfo.spuPtr = m_SpuAlloc;
	objectInfo.ppuSize = sizeof(T);
	objectInfo.ppuPtr = u32(ptr);

	m_SpuAlloc += RoundUp<16>(sizeof(T));

	if (readOnly)
	{
		Assert(m_ReadOnly);
		m_ReadOnly->Set(objectIndex);
	}
}

template <class T>
void sysDmaPlan::AddPartialObject(T* ptr, void* start, size_t size, bool readOnly)
{
	Assert(ptr);
	Assert(m_NumDmas < m_MaxDmas);
	Assert(m_NumDmas < MAX_NUM_DMAS);
	Assert((u32(ptr) & 0xf) == 0); // Doesn't work for objects not 16 byte aligned

	u32 alignedStart = u32(start) & ~0xf;
	u32 alignedSize = (u32(size) + 0xf) & ~0xf;

	u32 objectIndex = m_NumDmas++;
	CellDmaListElement& cdle = m_DmaList[objectIndex];
	cdle.size = alignedSize;
	cdle.eal = alignedStart;
	cdle.reserved = 0;
	cdle.notify = 0;

	ObjectInfo& objectInfo = sm_ObjectInfos[objectIndex];
	objectInfo.spuPtr = m_SpuAlloc + u32(ptr) - alignedStart;
	objectInfo.ppuSize = sizeof(T);
	objectInfo.ppuPtr = u32(ptr);

	m_SpuAlloc += alignedSize;

	if (readOnly)
	{
		Assert(m_ReadOnly);
		m_ReadOnly->Set(objectIndex);
	}
}

template <class T>
void sysDmaPlan::AddPartialObject(T* ptr, void* start, void* end, bool readOnly)
{
	Assert(ptr);
	Assert(m_NumDmas < m_MaxDmas);
	Assert(m_NumDmas < MAX_NUM_DMAS);
	Assertf((u32(ptr) & 0xf) == 0, "Pointer 0x%p is not 16 byte aligned", ptr);
	u32 alignedStart = u32(start) & ~0xf;
	size_t size = u32(end) - alignedStart;

	u32 alignedSize = (u32(size) + 0xf) & ~0xf;

	u32 objectIndex = m_NumDmas++;
	CellDmaListElement& cdle = m_DmaList[objectIndex];
	cdle.size = alignedSize;
	cdle.eal = alignedStart;
	cdle.reserved = 0;
	cdle.notify = 0;

	ObjectInfo& objectInfo = sm_ObjectInfos[objectIndex];
	objectInfo.spuPtr = m_SpuAlloc + u32(ptr) - alignedStart;
	objectInfo.ppuSize = sizeof(T);
	objectInfo.ppuPtr = u32(ptr);

	m_SpuAlloc += alignedSize;

	if (readOnly)
	{
		Assert(m_ReadOnly);
		m_ReadOnly->Set(objectIndex);
	}
}

template <class T>
void sysDmaPlan::AddArray(T* ptr, int count, bool readOnly)
{
	Assert(m_NumDmas < m_MaxDmas);
	Assertf(((u32)ptr & 0xf) == 0, "Pointer 0x%p is not 16 byte aligned", ptr);

	u32 objectIndex = m_NumDmas++;
	u32 alignedSize = RoundUp<16>(sizeof(T) * count);
	CellDmaListElement& cdle = m_DmaList[objectIndex];
	cdle.size = alignedSize;
	cdle.eal = u32(ptr);
	cdle.reserved = 0;
	cdle.notify = 0;

	ObjectInfo& objectInfo = sm_ObjectInfos[objectIndex];
	objectInfo.spuPtr = m_SpuAlloc;
	objectInfo.ppuSize = alignedSize;
	objectInfo.ppuPtr = u32(ptr);

	m_SpuAlloc += alignedSize;

	if (readOnly)
	{
		Assert(m_ReadOnly);
		m_ReadOnly->Set(objectIndex);
	}
}

extern bool g_DmaPlanAsList;

#if __SPU

__forceinline void sysDmaPlan::RelocateToLs()
{
	u32 fixup = u32(m_PpuAddr) - u32(this);
	m_DmaList = (CellDmaListElement*)(u32(m_DmaList) - fixup);
	m_Fixups = (sysDmaPlan::Fixup*)(u32(m_Fixups) - fixup);
	if (m_ReadOnly)
	{
		m_ReadOnly = (atBitSet*)(u32(m_ReadOnly) - fixup);
		m_ReadOnly->SetBits((unsigned*)(u32(m_ReadOnly->GetBitsPtr()) - fixup));
	}
}

__forceinline void sysDmaPlan::RelocateToMm()
{
	u32 fixup = u32(m_PpuAddr) - u32(this);
	m_DmaList = (CellDmaListElement*)(u32(m_DmaList) + fixup);
	m_Fixups = (sysDmaPlan::Fixup*)(u32(m_Fixups) + fixup);
	if (m_ReadOnly)
	{
		m_ReadOnly->SetBits((unsigned*)(u32(m_ReadOnly->GetBitsPtr()) + fixup));
		m_ReadOnly = (atBitSet*)(u32(m_ReadOnly) + fixup);
	}
}

__forceinline void sysDmaPlan::KickDMAGetFunc(void* ls, u32 tag SYS_DMA_FILELINE_ARGS)
{
#if SYS_DMA_VALIDATION
	Assert(m_NumDmas < MAX_NUM_DMAS);
#endif

	//PrintDmaList();

	if (g_DmaPlanAsList)
	{
		sysDmaListGetFileLine(ls, m_DmaList, m_NumDmas * sizeof(CellDmaListElement), tag);
	}
	else
	{
		const u32 numDmas = m_NumDmas;
		for (u32 i = 0; i < numDmas; ++i)
		{
			uint64_t size = m_DmaList[i].size;
			sysDmaGetFileLine(ls, m_DmaList[i].eal, size, tag);
			ls = (u8*)ls + RoundUp<16>(size); 
		}
	}
}

#define KickDMAGet(X, Y) KickDMAGetFunc(X, Y SYS_DMA_FILELINE_PARAMS)

__forceinline void sysDmaPlan::KickDMAPutFunc(void* ls, u32 tag SYS_DMA_FILELINE_ARGS)
{
#if SYS_DMA_VALIDATION
	Assert(m_NumDmas < MAX_NUM_DMAS);
#endif

	if (m_ReadOnly)
	{
		const u32 numDmas = m_NumDmas;
		for (u32 i = 0; i < numDmas; ++i)
		{
			uint64_t size = m_DmaList[i].size;
			if (!m_ReadOnly->IsSet(i))
			{
				sysDmaPutFileLine(ls, m_DmaList[i].eal, size, tag);
			}
			ls = (u8*)ls + RoundUp<16>(size); 
		}
	}
	else if (g_DmaPlanAsList)
	{
		sysDmaListPutFileLine(ls, m_DmaList, m_NumDmas * sizeof(CellDmaListElement), tag);
	}
	else
	{
		const u32 numDmas = m_NumDmas;
		for (u32 i = 0; i < numDmas; ++i)
		{
			uint64_t size = m_DmaList[i].size;
			sysDmaPutFileLine(ls, m_DmaList[i].eal, size, tag);
			ls = (u8*)ls + RoundUp<16>(size); 
		}
	}
}

#define KickDMAPut(X, Y) KickDMAPutFunc(X, Y SYS_DMA_FILELINE_PARAMS)

__forceinline void sysDmaPlan::FixupPointersToLs(void* ls)
{
	const u32 numPointers = m_NumFixups;
	for (u32 i = 0; i < numPointers; ++i)
	{
		Fixup& fixup = m_Fixups[i];
		void*& pointer = *(void**)(u32(ls) + fixup.pointer);

		void* temp = (void*)(int(pointer));
		pointer = (void*)(int(m_Fixups[i].referent) + int(ls));
		fixup.referent = u32(temp);
	}
}

__forceinline void sysDmaPlan::FixupPointersToMm(void* ls)
{
	const u32 numPointers = m_NumFixups;
	for (u32 i = 0; i < numPointers; ++i)
	{
		Fixup& fixup = m_Fixups[i];
		void*& pointer = *(void**)(u32(ls) + fixup.pointer);

		void* temp = (void*)(int(pointer));
		pointer = (void*)(m_Fixups[i].referent);
		fixup.referent = u32(temp) - int(ls);
	}
}

class sysDmaPlanQuadBuffer
{
public:
	// PURPOSE: Use this ctor if you are going to provide the object buffers for each object
	// PARAMS:
	//	planN: the storage for the DMA plans...hope it's big enough!
	//	tag: this DMA tag will be used, as well as tag + 1, tag + 2, and tag + 3
	__forceinline sysDmaPlanQuadBuffer(sysDmaPlan* plan0, sysDmaPlan* plan1, sysDmaPlan* plan2, sysDmaPlan* plan3, u32 tag)
	{
		m_PlanBuffers[0] = plan0;
		m_PlanBuffers[1] = plan1;
		m_PlanBuffers[2] = plan2;
		m_PlanBuffers[3] = plan3;

		for (u32 buffer = 0; buffer < 4; ++buffer)
		{
			int bufferTag = tag + buffer;
			m_Tags[buffer] = bufferTag;
			m_Masks[buffer] = 1 << bufferTag;
		}
	}

	// PURPOSE: Use this ctor if you are would like the dma plan quad buffer to hold onto the object buffers for you. They're probably all the same size in this case.
	// PARAMS:
	//	planN: the storage for the DMA plans...hope it's big enough!
	//	objectN: the storage for the objects themselves...this had better be big enough too!
	//	tag: this DMA tag will be used, as well as tag + 1, tag + 2, and tag + 3
	__forceinline sysDmaPlanQuadBuffer(sysDmaPlan* plan0, sysDmaPlan* plan1, sysDmaPlan* plan2, sysDmaPlan* plan3, void* object0, void* object1, void* object2, void* object3, u32 tag)
	{
		m_PlanBuffers[0] = plan0;
		m_PlanBuffers[1] = plan1;
		m_PlanBuffers[2] = plan2;
		m_PlanBuffers[3] = plan3;

		m_ObjectBuffers[0] = object0;
		m_ObjectBuffers[1] = object1;
		m_ObjectBuffers[2] = object2;
		m_ObjectBuffers[3] = object3;

		for (u32 buffer = 0; buffer < 4; ++buffer)
		{
			int bufferTag = tag + buffer;
			m_Tags[buffer] = bufferTag;
			m_Masks[buffer] = 1 << bufferTag;
		}
	}

	// PURPOSE: Start DMAing in the DMA plan for one of the slots
	// PARAMS:
	//	bufferIndex - 0 for the current buffer, 1 for the next, etc.
	//	mmAddress - where does the DMA plan sit in main memory?
	//	size - how big is the DMA plan?
	__forceinline void KickPlanGetFunc(u32 bufferIndex, sysDmaPlan* mmAddress, u32 size SYS_DMA_FILELINE_ARGS)
	{
		if (Unlikely(mmAddress))
		{
			// Wait for possible previously queued write operation to complete
			sysDmaWaitTagStatusAllFileLine(m_Masks[bufferIndex]);

			sysDmaGetFileLine(m_PlanBuffers[bufferIndex], (uint64_t)mmAddress, size, m_Tags[bufferIndex]);
		}
	}

#define KickPlanGet(X, Y, Z) KickPlanGetFunc(X, Y, Z SYS_DMA_FILELINE_PARAMS)

	// PURPOSE: Start DMAing the object in according to the DMA plan
	// PARAMS:
	//	bufferIndex - 0 for the current buffer, 1 for the next, etc.
	//	object - DMA the object into this local store address
	__forceinline void KickObjectGetBufferFunc(u32 bufferIndex, void* object SYS_DMA_FILELINE_ARGS)
	{
		sysDmaWaitTagStatusAllFileLine(m_Masks[bufferIndex]);

		sysDmaPlan* dmaPlan = m_PlanBuffers[bufferIndex];
		dmaPlan->RelocateToLs();
		dmaPlan->KickDMAGetFunc(object, m_Tags[bufferIndex] SYS_DMA_FILELINE_PASS);
	}

#define KickObjectGetBuffer(X, Y) KickObjectGetBufferFunc(X, Y SYS_DMA_FILELINE_PARAMS)

	// PURPOSE: Start DMAing the object in according to the DMA plan
	// PARAMS:
	//	bufferIndex - 0 for the current buffer, 1 for the next, etc.
	// NOTES: uses the built-in object buffers to find out where to put the object
	__forceinline void* KickObjectGetFunc(u32 bufferIndex SYS_DMA_FILELINE_ARGS)
	{
		sysDmaWaitTagStatusAllFileLine(m_Masks[bufferIndex]);

		sysDmaPlan* dmaPlan = m_PlanBuffers[bufferIndex];
		dmaPlan->RelocateToLs();
		dmaPlan->KickDMAGetFunc(m_ObjectBuffers[bufferIndex], m_Tags[bufferIndex] SYS_DMA_FILELINE_PASS);

		return m_ObjectBuffers[bufferIndex];
	}

#define KickObjectGet(X) KickObjectGetFunc(X SYS_DMA_FILELINE_PARAMS)

	// PURPOSE: Wait for the object to come into memory, and then apply pointer fixups
	// PARAMS:
	//	object - where is the object in local store (ignoring built-in object buffers)
	__forceinline void WaitForObjectGetBufferFunc(void* object SYS_DMA_FILELINE_ARGS)
	{
		// Wait for read to complete
		sysDmaWaitTagStatusAllFileLine(m_Masks[0]);

		m_PlanBuffers[0]->FixupPointersToLs(object);
	}

#define WaitForObjectGetBuffer(X) WaitForObjectGetBufferFunc(X SYS_DMA_FILELINE_PARAMS)

	// PURPOSE: Wait for the current object to come into memory, and then apply pointer fixups
	// NOTES: uses the built-in object buffers to find out where the object is
	__forceinline void WaitForObjectGetFunc(SYS_DMA_FILELINE_ARGS_SOLO)
	{
		// Wait for read to complete
		sysDmaWaitTagStatusAllFileLine(m_Masks[0]);

		m_PlanBuffers[0]->FixupPointersToLs(m_ObjectBuffers[0]);
	}

#define WaitForObjectGet() WaitForObjectGetFunc(SYS_DMA_FILELINE_PARAMS_SOLO)

	// PURPOSE: Start to DMA the current object back to main memory
	// PARAMS:
	//	object - where is the object in local store (ignoring built-in object buffers)
	__forceinline void KickObjectPutBufferFunc(void* object SYS_DMA_FILELINE_ARGS)
	{
		sysDmaPlan* dmaPlan = m_PlanBuffers[0];

		dmaPlan->FixupPointersToMm(object);
		dmaPlan->KickDMAPutFunc(object, m_Tags[0] SYS_DMA_FILELINE_PASS);
	}

#define KickObjectPutBuffer(X) KickObjectPutBufferFunc(X SYS_DMA_FILELINE_PARAMS)

	// PURPOSE: Start to DMA the current object back to main memory
	// NOTES: uses the built-in object buffers to find out where the object is
	__forceinline void KickObjectPutFunc(SYS_DMA_FILELINE_ARGS_SOLO)
	{
		sysDmaPlan* dmaPlan = m_PlanBuffers[0];

		dmaPlan->FixupPointersToMm(m_ObjectBuffers[0]);
		dmaPlan->KickDMAPutFunc(m_ObjectBuffers[0], m_Tags[0] SYS_DMA_FILELINE_PASS);
	}

#define KickObjectPut() KickObjectPutFunc(SYS_DMA_FILELINE_PARAMS_SOLO)

	// PURPOSE: Wait for the previous object to be finished writing back to main memory
	__forceinline void WaitForObjectPutFunc(SYS_DMA_FILELINE_ARGS_SOLO)
	{
		sysDmaWaitTagStatusAllFileLine(m_Masks[3]);
	}

#define WaitForObjectPut(X) WaitForObjectPutFunc(SYS_DMA_FILELINE_PARAMS_SOLO)

	// PURPOSE: Wait for the previous object to be finished writing back to main memory
	__forceinline void WaitForObjectPrevPutFunc(SYS_DMA_FILELINE_ARGS_SOLO)
	{
		sysDmaWaitTagStatusAllFileLine(m_Masks[2]);
	}

#define WaitForObjectPrevPut(X) WaitForObjectPrevPutFunc(SYS_DMA_FILELINE_PARAMS_SOLO)

	// PURPOSE: Access the previous object
	__forceinline void* GetPreviousObject()
	{
		return m_ObjectBuffers[3];
	}

	// PURPOSE: The next buffer becomes the current, the current becomes the previous, and the next next becomes the next
	// NOTES: does not rotate object buffers!
	__forceinline void RotateBuffers()
	{
		sysDmaPlan* tempPlan = m_PlanBuffers[0];
		m_PlanBuffers[0] = m_PlanBuffers[1];
		m_PlanBuffers[1] = m_PlanBuffers[2];
		m_PlanBuffers[2] = m_PlanBuffers[3];
		m_PlanBuffers[3] = tempPlan;

		u32 tempTag = m_Tags[0];
		m_Tags[0] = m_Tags[1];
		m_Tags[1] = m_Tags[2];
		m_Tags[2] = m_Tags[3];
		m_Tags[3] = tempTag;

		u32 tempMask = m_Masks[0];
		m_Masks[0] = m_Masks[1];
		m_Masks[1] = m_Masks[2];
		m_Masks[2] = m_Masks[3];
		m_Masks[3] = tempMask;
	}

	// PURPOSE: The next buffer becomes the current, the current becomes the previous, and the next next becomes the next
	// NOTES: use this version if you're using the object buffers
	__forceinline void RotateBuffersObject()
	{
		sysDmaPlan* tempPlan = m_PlanBuffers[0];
		m_PlanBuffers[0] = m_PlanBuffers[1];
		m_PlanBuffers[1] = m_PlanBuffers[2];
		m_PlanBuffers[2] = m_PlanBuffers[3];
		m_PlanBuffers[3] = tempPlan;

		void* tempObject = m_ObjectBuffers[0];
		m_ObjectBuffers[0] = m_ObjectBuffers[1];
		m_ObjectBuffers[1] = m_ObjectBuffers[2];
		m_ObjectBuffers[2] = m_ObjectBuffers[3];
		m_ObjectBuffers[3] = tempObject;

		u32 tempTag = m_Tags[0];
		m_Tags[0] = m_Tags[1];
		m_Tags[1] = m_Tags[2];
		m_Tags[2] = m_Tags[3];
		m_Tags[3] = tempTag;

		u32 tempMask = m_Masks[0];
		m_Masks[0] = m_Masks[1];
		m_Masks[1] = m_Masks[2];
		m_Masks[2] = m_Masks[3];
		m_Masks[3] = tempMask;
	}

	// PURPOSE: The current buffer is untouched, but the other buffers rotate
	// NOTES: does not rotate object buffers!
	__forceinline void RotateBuffersKeepCurrent()
	{
		sysDmaPlan* tempPlan = m_PlanBuffers[1];
		m_PlanBuffers[1] = m_PlanBuffers[2];
		m_PlanBuffers[2] = m_PlanBuffers[3];
		m_PlanBuffers[3] = tempPlan;

		u32 tempTag = m_Tags[1];
		m_Tags[1] = m_Tags[2];
		m_Tags[2] = m_Tags[3];
		m_Tags[3] = tempTag;

		u32 tempMask = m_Masks[1];
		m_Masks[1] = m_Masks[2];
		m_Masks[2] = m_Masks[3];
		m_Masks[3] = tempMask;
	}

	// PURPOSE: The current buffer is untouched, but the other buffers rotate
	// NOTES: use this version if you're using the object buffers
	__forceinline void RotateBuffersObjectKeepCurrent()
	{
		sysDmaPlan* tempPlan = m_PlanBuffers[1];
		m_PlanBuffers[1] = m_PlanBuffers[2];
		m_PlanBuffers[2] = m_PlanBuffers[3];
		m_PlanBuffers[3] = tempPlan;

		void* tempObject = m_ObjectBuffers[1];
		m_ObjectBuffers[1] = m_ObjectBuffers[2];
		m_ObjectBuffers[2] = m_ObjectBuffers[3];
		m_ObjectBuffers[3] = tempObject;

		u32 tempTag = m_Tags[1];
		m_Tags[1] = m_Tags[2];
		m_Tags[2] = m_Tags[3];
		m_Tags[3] = tempTag;

		u32 tempMask = m_Masks[1];
		m_Masks[1] = m_Masks[2];
		m_Masks[2] = m_Masks[3];
		m_Masks[3] = tempMask;
	}

private:
	sysDmaPlan* m_PlanBuffers[4];
	void* m_ObjectBuffers[4];
	u32 m_Tags[4];
	u32 m_Masks[4];
};

// For the quad buffer ctors, this can help since you're generally passing in four different array elements
// e.g.:	sysDmaPlanQuadBuffer colliderPlansA(QUAD_ARRAY_PARAM((sysDmaPlan*)colliderPlanStorageA), DMA_TAG(19));
#define QUAD_ARRAY_PARAM(X) X[0], X[1], X[2], X[3]

#endif // __SPU

} // namespace rage

#endif // __PS3

#endif // SYSTEM_DMAPLAN_H 
