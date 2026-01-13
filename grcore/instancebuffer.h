//
// grcore/instancebuffer.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_INSTANCEBUFFER_H
#define GRCORE_INSTANCEBUFFER_H

#include <stddef.h>			// for size_t
#include "grcore/config.h"
#include "grcore/buffer_d3d11.h"
#include "data/base.h"
#include "system/new.h"
#include "atl/array.h"
#include "math/amath.h"

#if RSG_ORBIS
#include <gnm/buffer.h>
#endif

#if RSG_DURANGO
struct ID3D11Buffer;
#endif

#include "shaderlib/instancebuffer.fxh"

namespace rage {

	class grcCBuffer;

	//Forward Declarations
	class Vector4;
	PS3_ONLY(struct spuCmd_grcDevice__DrawInstancedPrimitive);

/* 
	Abstraction of a buffer of per-instance data.  Depending on the platform, this may be
	a vertex stream, raw DMA packets, or dynamic constant buffers.  Caller is responsible
	for making sure that the buffer lives long enough for the GPU to see the data.
	Furthermore, whether we actually use GPU instancing under the hood is an implementation
	detail; on some platforms it's sufficient to bypass extra layers of RAGE and driver-side code.

	Creating an instance buffer is relatively expensive on some platforms, so we assume that
	higher-level code manages a supply of these to share across systems.  On current generation,
	however, we can pass in preallocated drawlist memory instead.
*/
class grcInstanceBuffer : public datBase {
public:
	// This value is used on 360 to control how many times the index buffer is replicated for true GPU instancing.
	// It also controls the size of the instance array constant buffer.
	static const unsigned MaxPerDraw = MAX_INSTANCES_PER_DRAW;

	// Size of an instance buffer, in bytes
	// We keep them all the same size so they're easier to manage and can be recycled
	// by different clients between frames.
	static const unsigned SizeQW = MaxPerDraw * 5;	//5 registers per instance (5 used in trees, currently seems to be unused for anything else).
	static const unsigned Size = SizeQW * 16;		//(16 bytes * 6 registers) = 96 bytes per instance
	static const unsigned MaxSizeQW = SizeQW;		 

	// PURPOSE: Factory function; default just invokes rage_new and Init with no storage.
	//			This function is assumed to return a suitable buffer quickly enough for
	//			general use (in other words, from a pool of existing objects on platforms
	//			where creation is expensive, or drawlist memory on others)
	// PARAMS:	t - Type of data; this is mostly so that we can provide useful helper functions,
	//				and make sure the format of the data is consistent with what the current
	//				vertex shader is expecting.
	//			maxCount - Maximum number of instances this buffer will ever hold
	//			stride - Stride, in bytes (only necessary for CUSTOM, otherwise it's computed)
	// RETURNS: Pointer to new instance buffer; may be NULL, so calling code should deal
	//			with that cleanly.  Intent here is to allow rage-level code to create buffers
	//			without knowing the details.
	static grcInstanceBuffer *Create() {
		return sm_Create();
	}

	// PURPOSE: Destroy a buffer created via Create.
	static void Destroy(grcInstanceBuffer *ib) {
		sm_Destroy(ib);
	}

	// Call this function once per frame to retire old instance buffers
	static void NextFrame() {
		sm_NextFrame();
	}

	// PURPOSE:	Locks the instance buffer, returning pointer to storage.
	// NOTES:	You are allowed to lock the same buffer more than once, but treat it as write-only.
	virtual void *Lock() = 0;
	virtual void *Lock(size_t finalCount,size_t elemSizeQw) = 0;

	// PURPOSE:	Unlocks the instance buffer.
	// PARAMS:	Final count of objects (must be <= maxCount)
	virtual void Unlock(size_t finalCount,size_t elemSizeQw) = 0;
	virtual void Unlock() = 0;

	// PURPOSE:	Sets next link in chain for management
	virtual void SetNext(grcInstanceBuffer *n) = 0;

	// PURPOSE: Returns next buffer in list (as set up by grcInstanceBufferList)
	virtual grcInstanceBuffer* GetNext() = 0;

	virtual size_t GetCount() const = 0;
	virtual size_t GetElemSizeQW() const = 0;
	virtual const void *GetData() const = 0;

	virtual void Bind(PS3_ONLY(spuCmd_grcDevice__DrawInstancedPrimitive &cmd)) = 0;

	//Way to distinguish between drawlist vs static memory allocation.
	enum AllocationType
	{
		ALLOC_STANDARD,
		ALLOC_DRAWLIST,

		NUM_ALLOC_TYPES
	};
	virtual AllocationType GetAllocationType() const	{ return ALLOC_STANDARD; }

	// Factory functions; public so we don't need gratuitous set/get functions.
	static grcInstanceBuffer *(*sm_Create)();
	static void (*sm_Destroy)(const grcInstanceBuffer*);
	static void (*sm_NextFrame)();
	static u32 sm_MaxInstancesPerDraw;
	static u32 sm_MaxSizeInQW;

	static void ResetFactoryFunctors();
	static void SetFactoryFunctors(grcInstanceBuffer *(*pCreate)(), void (*pDestroy)(const grcInstanceBuffer*), void (*pNextFrame)(), u32 MaxInstancesPerDraw = grcInstanceBuffer::MaxPerDraw, u32 MaxSizeInQW = grcInstanceBuffer::MaxSizeQW);
	static u32 GetMaxInstancesPerDraw() { return sm_MaxInstancesPerDraw; }
	static u32 GetMaxSizeInQW() { return sm_MaxSizeInQW; }

	// Call this before doing any DrawInstanced calls; it does any special GPU setup
	static void BeginInstancing();

	// Call this after doing all DrawInstanced calls
	static void EndInstancing();

	// Max number of frames we'll keep buffers around.
	static const int MaxFrames = 3;
	static int GetCurrentFrame()				{ return sm_CurrentFrame; }
	static int GetCurrentFrame(int maxFrames)	{ return static_cast<int>(sm_FrameCount % static_cast<u64>(maxFrames)); }
	static u64 GetFrameCount()					{ return sm_FrameCount; }
	static void AdvanceCurrentFrame();

protected:
	virtual ~grcInstanceBuffer() {}

	BANK_ONLY(static u32 sm_BytesLockedThisFrame);
	BANK_ONLY(static u32 sm_BytesLockedMax);

private:
	static int sm_CurrentFrame;
	static u64 sm_FrameCount;
};

class ALIGNAS(16) grcInstanceBufferBasic : public grcInstanceBuffer
{
public:
	virtual void *Lock() { return (void*) m_Storage; }
	virtual void Unlock(size_t finalCount,size_t elemSizeQw);

	virtual void *Lock(size_t finalCount, size_t elemSizeQw) { Assert(finalCount*16*elemSizeQw <= Size); m_Count = (u32)finalCount; m_ElemSizeQW = (u32)elemSizeQw; return Lock(); }
	virtual void Unlock() { Unlock(m_Count, m_ElemSizeQW); }

	virtual void SetNext(grcInstanceBuffer *n) { m_Next = static_cast<grcInstanceBufferBasic *>(n); }
	virtual grcInstanceBuffer* GetNext() { return m_Next; }

	virtual size_t GetCount() const { return m_Count; }
	virtual size_t GetElemSizeQW() const { return m_ElemSizeQW; }
	virtual const void *GetData() const { return (void*)m_Storage; }

	virtual void Bind(PS3_ONLY(spuCmd_grcDevice__DrawInstancedPrimitive &params));

	//Simple allocator interface to be used by grcInstanceBuffer's default create/destroy calls.
	class Allocator
	{
	public:
		virtual ~Allocator() {}
		virtual void *Allocate(size_t size) = 0;
		virtual void Free(void *buf) = 0;
	};

	static void InitClass();
	static void InitClass(Allocator *alloc);

	static void ShutdownClass();

	//Overload operator new/delete to use allocator.
	void *operator new(size_t size RAGE_NEW_EXTRA_ARGS_UNUSED);
	void *operator new(size_t size, size_t align RAGE_NEW_EXTRA_ARGS_UNUSED);
	void operator delete(void *buf);

protected:
	grcInstanceBufferBasic();
	virtual ~grcInstanceBufferBasic();

#if RSG_DURANGO
#elif RSG_PC && __D3D11
	grcCBuffer *GetConstantBuffers() const;
#elif RSG_ORBIS
	sce::Gnm::Buffer *GetConstantBuffers() const { return const_cast<sce::Gnm::Buffer*>(&m_CB); }
#endif

private:
	friend class grcInstanceBuffer;
	static grcInstanceBuffer* DefaultCreate();
	static void DefaultNextFrame();
	static void DefaultDestroy(const grcInstanceBuffer*);

	static void *DoAllocate(size_t size, size_t align);

	static grcInstanceBufferBasic *sm_FirstAlloc[MaxFrames];
	static grcInstanceBufferBasic *sm_LastAlloc[MaxFrames];
	static grcInstanceBufferBasic *sm_FirstFree;
	static Allocator *sm_Allocator;

	// Count of actual entries in buffer (set by Unlock; could be a smaller type given current value of Size)
	u32 m_Count, m_ElemSizeQW;

	//The storage parameter *must* be 16-byte aligned, otherwise writing a 16-byte aligned value to it will overwrite unaligned bits! (Such as the vftp)
	//On 64-bit platforms, the vftp will be 8-byes, so the 2 u32s above would suffice to pad out the storage class. However, on 32-bit, we need an additional
	//4 bytes of padding.
#if !__64BIT
	u32 m_Padding;
#endif

	u8 m_Storage[Size];

#if !__64BIT
	u32 m_Padding2;
#endif

#if RSG_PC &&__D3D11
	grcCBuffer *m_CB;
#elif RSG_DURANGO
#elif RSG_ORBIS
	sce::Gnm::Buffer m_CB;
#elif !__64BIT
	u32 m_Padding3;
#endif

	// Next buffer in list chain (as used by grcInstanceBufferList)
	grcInstanceBufferBasic *m_Next;
	// Next buffer in allocation chain
	grcInstanceBufferBasic *m_NextAlloc;
} ;


class grcInstanceBufferList {
public:
	virtual ~grcInstanceBufferList() { }

	// PURPOSE:	Gets a pointer to the 1st instance buffer in the list
	virtual grcInstanceBuffer *GetFirst() = 0;
};

//Fwd declare so this class can be declared as a friend below.
template <int NumFramesToBuffer> class grcBufferedStaticInstanceBufferList;

class grcStaticInstanceBufferList : public grcInstanceBufferList
{
public:
	//Requirements that the ib_type interface must support:
	//	* Public constructor/destructor - Instance Buffers are not created using grcInstanceBuffer's factory functions.
	//	* SetParentList(const grcStaticInstanceBufferList *) - IBs get a pointer to this list for optimization purposes.
	typedef grcInstanceBufferList parent_type;

	grcStaticInstanceBufferList(u32 numInstances, u32 numRegistersPerInstance) { Init(numInstances, numRegistersPerInstance); }

	//List interface:
	virtual grcInstanceBuffer *GetFirst() { return &(m_IBs[0]); }

	//Accessors
	u32 GetNumInstances() const		{ return m_NumInstances; }
	u32 GetStrideQW() const			{ return m_StrideQW; }
	u32 GetMaxCountPerBatch() const	{ return m_MaxCountPerBatch; }

private:
	class StaticInstanceBuffer : public grcInstanceBufferBasic
	{
	};

	typedef atArray<StaticInstanceBuffer, 128> ib_array;

	ib_array m_IBs;
	u32 m_NumInstances, m_StrideQW, m_MaxCountPerBatch;

public:
	//Mem reporting
	size_t GetIBMemUsage() const	{ return sizeof(ib_array::value_type) * m_IBs.size(); }

private:
	//Interface to allow grcBufferedStaticInstanceBufferList to delay initialization of this class.
	template <int NumFramesToBuffer> friend class grcBufferedStaticInstanceBufferList;
	template <class _Type,int _MaxCount> friend class atRangeArray;

	grcStaticInstanceBufferList();
	void Init(u32 numInstances, u32 numRegistersPerInstance);
};

template <int NumFramesToBuffer>
class grcBufferedStaticInstanceBufferList : public grcInstanceBufferList
{
public:
	typedef grcStaticInstanceBufferList list_type;
	static const int MaxFrames = (NumFramesToBuffer >= 1 ? NumFramesToBuffer : 1); //Max(NumFramesToBuffer, 1);

	grcBufferedStaticInstanceBufferList(u32 numInstances, u32 numRegistersPerInstance)
	{
		for(int i = 0; i < MaxFrames; ++i)
			m_Lists[i].Init(numInstances, numRegistersPerInstance);
	}

	list_type &GetList(u32 index)	{ return m_Lists[index]; }
	list_type &GetCurrentList()		{ return GetList(grcInstanceBuffer::GetCurrentFrame(MaxFrames)); }

	//List interface:
	virtual grcInstanceBuffer *GetFirst() { return GetCurrentList().GetFirst(); }

	//Mem reporting
	size_t GetIBMemUsage() const
	{
		size_t usage = 0;
		for(int i = 0; i < m_Lists.size(); ++i)
			usage += m_Lists[i].GetIBMemUsage();
		return usage;
	}

private:
	atRangeArray<list_type, MaxFrames> m_Lists;
};

class grcInstanceBufferListBasic : public grcInstanceBufferList
{
public:
	grcInstanceBufferListBasic() : m_First(0), m_Buffer(0), m_Count(0), m_ElemSizeQW(0) { }

	// PURPOSE:	Closes off the instance buffer list, returns first one in list or NULL if Append
	//			was never called.
	grcInstanceBuffer *GetFirst() { 
		if (m_Buffer) {
			if (m_Buffer)
				m_Buffer->Unlock(m_Count, m_ElemSizeQW);
			m_Buffer = NULL;
		}
		return m_First; 
	}

	size_t GetCount() const		{ return m_Count; }
	size_t GetSizeQW() const	{ return m_ElemSizeQW; }

protected:
	grcInstanceBuffer *m_First, *m_Buffer;
	size_t m_Count, m_ElemSizeQW;
};

/*
	Helper class which assembles a list of instance buffers.

	grcTypedInstanceBufferList<Mat44V> list;
	for (each item) {
		if (item is visible)
			item->ExtractMatrix(list.Append());
	}
	if (list.GetFirst()) {
		for (each model in item archetype) {
			model->SetState();
			for (grcInstance *i=list.GetFirst(); i; i=i->GetNext())
				model->DrawInstance(*i);
		}
	}
*/
template <typename _T> class grcTypedInstanceBufferList: public grcInstanceBufferListBasic {
public:
	grcTypedInstanceBufferList() : m_Current(0), m_Stop(0) { m_ElemSizeQW = sizeof(_T)/16; }

	// PURPOSE:	Allocate a new instance slot, return reference to this storage.
	//			Done this way to avoid an extra copy in situations where we're
	//			constructing the data just in time anyway.
	_T *Append() {
		if (m_Current < m_Stop)
			return ++m_Count, m_Current++;
		else
			return NewBuffer();
	}

private:
	_T *NewBuffer() {
		if(grcInstanceBuffer *newBuffer = grcInstanceBuffer::Create())
		{
			if (m_Buffer) {
				m_Buffer->Unlock(m_Count,sizeof(_T)/16);
				m_Buffer->SetNext(newBuffer);
			}
			else
				m_First = newBuffer;
			m_Buffer = newBuffer;
			m_Current = (_T*) newBuffer->Lock();
			m_Stop = m_Current + (grcInstanceBuffer::Size / sizeof(_T));
			m_Count = 1;
			return m_Current++;
		}

		return NULL; //Allocation failed! Leave state the same so next time we try to allocate again.
	}

	_T *m_Current, *m_Stop;
};

/*
	Helper class which assembles a list of instance buffers.

	Not quite as safe as the typed version above, as you rely on the user to only write the amount of data they specified in the constructor.
*/
class grcVecArrayInstanceBufferList: public grcInstanceBufferListBasic {
public:
	grcVecArrayInstanceBufferList(u32 numRegistersPerInstance);

	// PURPOSE:	Allocate a new instance slot, return reference to this storage.
	Vector4 *Append();

	inline u32 GetNumRegsPerInstance() const	{ return m_StrideQW; }

private:
	Vector4 *NewBuffer();

	Vector4 *m_Current;
	u32 m_StrideQW, m_MaxCountPerBatch;
};



class grcVec4BufferInstanceBufferList: public grcInstanceBufferList 
{
public:
	grcVec4BufferInstanceBufferList(u32 numRegistersPerInstance);
	u32 GetMaxInstancesPerBuffer() const { return m_MaxCountPerBatch; }
	Vector4 *OpenBuffer(u32 noOfInstances);
	void CloseBuffer();

	grcInstanceBuffer *GetFirst(); 
	inline u32 GetNumRegsPerInstance() const	{ return m_StrideQW; }
protected:
	grcInstanceBuffer *m_First, *m_Buffer;
	u32 m_StrideQW, m_MaxCountPerBatch;
	size_t m_Count, m_ElemSizeQW;
};


}	// namespace rage

#endif
