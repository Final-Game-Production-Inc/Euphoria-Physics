// 
// grcore/indexbuffer.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_INDEXBUFFER_H
#define GRCORE_INDEXBUFFER_H

#if __XENON
typedef struct D3DIndexBuffer d3dIndexBuffer;
#else
typedef struct IUnknown d3dIndexBuffer;
#endif

#include "grcore/config.h"
#include "grcore/opengl.h"
#include "grcore/locktypes.h"
#include "grcore/device.h"

#include "data/base.h"
#include "data/struct.h"

#if __D3D11
#include "grcore/buffer_d3d11.h"
#endif //__D3D11
#include "grcore/buffer_durango.h"

#if __XENON
#define	INDEXBUFFERDX9 d3dIndexBuffer
#elif __WIN32PC || RSG_DURANGO
#define	INDEXBUFFERDX9 IDirect3DIndexBuffer9
#define	INDEXBUFFERDX10 ID3D10Buffer
#define	INDEXBUFFERDX11 ID3D11Buffer
#endif

#define INDEX_BUFFER_D3D_INVALID_ADDRESS 0xdeadbeef

namespace rage
{

#define RAGE_INDEXBUFFER_ALIGNMENT	16

#if __WIN32PC && !__RESOURCECOMPILER
class grcIndexManager
{
public:
	grcIndexManager();
private:
	static bool Lost();
	static bool Reset();
};
#endif // __WIN32PC && !__RESOURCECOMPILER

#if __RESOURCECOMPILER
#define RESOURCECOMPILER_PURE_VIRTUAL(SIGNATURE)    virtual SIGNATURE = 0
#else
#define RESOURCECOMPILER_PURE_VIRTUAL(SIGNATURE)    SIGNATURE
#endif

class grcIndexBuffer: public datBase
{
protected:
	explicit grcIndexBuffer(int indexCount);
public:
	explicit grcIndexBuffer(datResource& rsc);
	~grcIndexBuffer();

	DECLARE_PLACE(grcIndexBuffer);

	static grcIndexBuffer* Create(int indexCount, bool dynamic = true, void* preAllocatedMemory = NULL);
#if __D3D11 && RSG_PC
	static grcIndexBuffer* Clone(class grcIndexBuffer *pExisting);
	static grcIndexBuffer* CreateWithData(int indexCount, grcBufferCreateType CreateType, const void* preAllocatedMemory, bool AllocateCPUCopy);
#elif RSG_DURANGO || RSG_ORBIS
	static grcIndexBuffer* Clone(class grcIndexBuffer *pExisting);
	static grcIndexBuffer* CreateWithData(int indexCount, bool bDynamic, const void* preAllocatedMemory);
#endif // RSG_DURANGO

	static inline bool AreLockFlagsReadOnly(u32 flags) {return (flags & grcsRead) && !(flags & (grcsWrite | grcsNoOverwrite | grcsDiscard));}

	// PURPOSE:	Return a read-only lock.  Will fail on compressed index buffers.
	RESOURCECOMPILER_PURE_VIRTUAL(const u16* LockRO() const);

	// PURPOSE:	Return a read-write lock.  Will fail on compressed index buffers.
	RESOURCECOMPILER_PURE_VIRTUAL(u16* LockRW() const);

	// PURPOSE:	Return a write-only lock.  Will fail on compressed index buffers.
	RESOURCECOMPILER_PURE_VIRTUAL(u16* LockWO() const);

	// PURPOSE: Return a read/write lock based off flags.  Will fail on compressed index buffers.
	RESOURCECOMPILER_PURE_VIRTUAL(u16* Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const);

	// PURPOSE:	Unlocks a previously locked index buffer.
	RESOURCECOMPILER_PURE_VIRTUAL(void UnlockRO() const);

	// PURPOSE:	Unlocks a previously locked index buffer.
	RESOURCECOMPILER_PURE_VIRTUAL(void UnlockRW() const);

	// PURPOSE:	Unlocks a previously locked index buffer.
	RESOURCECOMPILER_PURE_VIRTUAL(void UnlockWO() const);

	// PURPOSE: Gets the memory address that would be returned from LockRO, but
	// without obtaining a lock.  This pointer is only intended to be used for
	// locking the memory region against defragging.  The pointer should not be
	// dereferenced.
	RESOURCECOMPILER_PURE_VIRTUAL(const void *GetUnsafeReadPtr() const);

	virtual bool Lost()				{ return true; }
	virtual bool Reset()			{ return true; }
	virtual void ReleaseD3DBuffer() {}

#if RSG_PC && __D3D11
	d3dIndexBuffer *GetD3DBuffer() const;
#endif

	void Set(grcIndexBuffer* pSoruce);

	// PURPOSE: Accessor for the # of indices in this index buffer.
	// RETURNS: The # of indices in this index buffer.
	int	GetIndexCount() const { return m_IndexCountAndFlags & IDX_COUNT_MASK; }
	bool IsPreallocatedMemory() const { return (m_IndexCountAndFlags & FLAG_PREALLOCATED) != 0; }
	void MarkPreallocated() { m_IndexCountAndFlags |= FLAG_PREALLOCATED; }

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif

protected:
#if __RESOURCECOMPILER
	void ByteSwapBuffer();
#endif

	enum
	{
		FLAG_PREALLOCATED 	= 0x01000000,
		FLAGS_MASK          = 0xff000000,
		IDX_COUNT_MASK      = 0x00ffffff,
	};

	u32				m_IndexCountAndFlags;		// +4
	datRef<u16>		m_IndexData;				// +8
												// +12 added by subclass
};


#undef RESOURCECOMPILER_PURE_VIRTUAL


#if !__D3D11

class grcIndexBufferD3D: public grcIndexBuffer {
	friend class grcIndexManager;
public:
#if RSG_PC || RSG_DURANGO
	struct D3DInternalData
	{
		d3dIndexBuffer*		m_D3DStagingBuffer;
		u32					m_Pool;
		u32					m_LockType;
		grcIndexBufferD3D*	m_Next;
		grcIndexBufferD3D*	m_Previous;
		u32					m_uOffset;
		u32					m_uSize;
	};
	static grcIndexBufferD3D* sm_First;
#endif
	grcIndexBufferD3D(int indexCount, void* preAllocatedMemory = NULL);
	grcIndexBufferD3D(datResource&);
	~grcIndexBufferD3D();

#if __RESOURCECOMPILER
	virtual const u16* LockRO() const;
	virtual u16* LockRW() const;
	virtual u16* LockWO() const;
	virtual u16* Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
	virtual void UnlockRO() const;
	virtual void UnlockRW() const;
    virtual void UnlockWO() const;
	virtual const void *GetUnsafeReadPtr() const;
#endif
	u16* LockInternal(u32 lockFlag = 0, u32 offset = 0, u32 size = 0) const;
	void UnlockInternal() const;

#if RSG_PC
	d3dIndexBuffer*	 GetD3DBuffer() const		{ return m_D3DBuffer; }
#endif

	virtual void ReleaseD3DBuffer();

#if __WIN32PC && !__RESOURCECOMPILER
	virtual bool Lost();
	virtual bool Reset();
#endif // __WIN32PC

#if RSG_PC || RSG_DURANGO
	D3DInternalData* GetInternalData() const  { return (D3DInternalData*)storage; };
	d3dIndexBuffer* GetStagingD3DBuffer() const	{ return GetInternalData()->m_D3DStagingBuffer; }
#endif

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif
private:
#if RSG_PC || RSG_DURANGO
	void SetStagingD3DBuffer(d3dIndexBuffer* indexBuffer) { GetInternalData()->m_D3DStagingBuffer = indexBuffer; }
#endif // __WIN32PC

	void CreateInternal(WIN32PC_ONLY(bool bUseStaging = true));

	d3dIndexBuffer*			m_D3DBuffer;
	size_t storage[8];
};

#endif //!__D3D11

#if __D3D11 && RSG_PC

class grcIndexBufferD3D11: public grcIndexBuffer {
	friend class grcIndexManager;

public:
	struct D3D11InternalData
	{
		grcIndexBufferD3D11*	m_Next;
		grcIndexBufferD3D11*	m_Previous;
		grcBufferD3D11			m_Buffer;
	};
	static grcIndexBufferD3D11* sm_First;

	// Construction/Destruction related functions.
public:
	grcIndexBufferD3D11(int indexCount, grcBufferCreateType CreateType = grcsBufferCreate_ReadWriteOnceOnly);
	grcIndexBufferD3D11(int indexCount, grcBufferCreateType CreateType, u16 *pIndexData, bool AllocateCPUCopy);
	grcIndexBufferD3D11(datResource&);
	~grcIndexBufferD3D11();
private:
	void CreateInternal(grcBufferCreateType CreateType, bool AllocateCPUCopy);
#if TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO
public:
	void RecreateInternal();
#endif //TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO

	// Lock()/Unlock() related functions.
public:
#if __RESOURCECOMPILER
	virtual u16* Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
	virtual const u16* LockRO() const;
	virtual u16* LockWO() const;
	virtual u16* LockRW() const;
	virtual void UnlockRO() const;
	virtual void UnlockRW() const;
	virtual void UnlockWO() const;
	virtual const void *GetUnsafeReadPtr() const;
#endif


	// General access functions.
public:
	D3D11InternalData* GetInternalData() const { return (D3D11InternalData*)storage; }

#if RSG_PC
	d3dIndexBuffer*	 GetD3DBuffer() const		{ return (d3dIndexBuffer *)GetInternalData()->m_Buffer.GetD3DBuffer(); }
#endif

	virtual void ReleaseD3DBuffer();

	// Device reset/lost.
private:
	// DX11 TODO:- Nothing special needs to be done on DX11. 
	virtual bool Lost();
	virtual bool Reset();

	// Resource builder functions.
#if __DECLARESTRUCT
public:
	virtual void DeclareStruct(datTypeStruct &s);
#endif

private:
	// NOTE:- THESE DATA MEMBERS MUST MATCH grcIndexBufferD3D UNTIL WE BUILD DX11 RESOURCES.
	d3dIndexBuffer*			m_D3DBuffer_Unused;
	typedef size_t					Storage[8];
	Storage							storage;
	CompileTimeAssert(sizeof(Storage) >= sizeof(D3D11InternalData));
};

CompileTimeAssertSize(grcIndexBufferD3D11,48,96);

#endif //__D3D11 && RSG_PC

#if RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)
class grcIndexBufferDurango: public grcIndexBuffer {
public:
	grcIndexBufferDurango(int indexCount, void* preAllocatedMemory = NULL);
	grcIndexBufferDurango(datResource&);
	~grcIndexBufferDurango();
	void ReleaseD3DBuffer();

	u16* Lock(u32 flags, u32 offset, u32 size) const;
	void Unlock(u32 flags) const;
	const u16* LockRO() const;
	u16* LockRW() const;
	u16* LockWO() const;
	void UnlockRO() const;
	void UnlockRW() const;
	void UnlockWO() const;
	const void *GetUnsafeReadPtr() const;

#if RSG_PC
	d3dIndexBuffer *GetD3DBuffer() const;
#endif

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif
private:
	grcBufferDurango m_Buffer;
};
#endif // RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)


class grcIndexBufferGCM: public grcIndexBuffer {
public:
	grcIndexBufferGCM(int indexCount, bool dynamic, void* preAllocatedMemory = NULL);
	grcIndexBufferGCM(datResource&);
	~grcIndexBufferGCM();
#if __RESOURCECOMPILER
	virtual u16* Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
	virtual const u16* LockRO() const;
	virtual u16* LockRW() const;
	virtual u16* LockWO() const;
	virtual void UnlockRO() const;
	virtual void UnlockRW() const;
	virtual void UnlockWO() const;
	virtual const void *GetUnsafeReadPtr() const;
#endif
	u16* GetGCMBuffer() const		{ return m_IndexData; }
	u32 GetGCMOffset() const		{ return m_Offset; }

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif
private:
	u32 m_Offset;
};

class grcIndexBufferGNM: public grcIndexBuffer {
public:
	grcIndexBufferGNM(int indexCount, bool dynamic, void* preAllocatedMemory = NULL);
	grcIndexBufferGNM(datResource&);
	~grcIndexBufferGNM();
#if __RESOURCECOMPILER
	virtual u16* Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
	virtual const u16* LockRO() const;
	virtual u16* LockRW() const;
	virtual u16* LockWO() const;
	virtual void UnlockRO() const;
	virtual void UnlockRW() const;
	virtual void UnlockWO() const;
	virtual const void *GetUnsafeReadPtr() const;
#endif

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif
};

CompileTimeAssertSize(grcIndexBufferGNM,12,24);

#if (__WIN32PC && !__D3D11) || __XENON || __RESOURCECOMPILER

#	if __RESOURCECOMPILER
#		define GRCINDEXBUFFERD3D           grcIndexBufferD3D
#	else
#		define GRCINDEXBUFFERD3D           grcIndexBuffer
#	endif

	// Cannot easily inline GRCINDEXBUFFERD3D::LockRO here, due to use of D3D structures and macros.
#if !__XENON
	inline const u16 *GRCINDEXBUFFERD3D::LockRO() const
	{
		const u16 *const ret = m_IndexData;
		Assert(ret);
		return ret;
	}
#endif // !__XENON

	inline u16 *GRCINDEXBUFFERD3D::LockRW() const
	{
		return static_cast<const grcIndexBufferD3D*>(this)->LockInternal(grcsRead|grcsWrite, 0, 0);
	}

	inline u16 *GRCINDEXBUFFERD3D::LockWO() const
	{
		return LockRW();
	}

	inline u16 *GRCINDEXBUFFERD3D::Lock(u32 flags, u32 offset, u32 /*size*/) const
	{
		u16 *startLock;
		if(AreLockFlagsReadOnly(flags))
		{
			startLock = const_cast<u16*>(LockRO());
		}
		else
		{
			startLock = LockRW();
		}
		return (u16*)((char*)startLock+offset);
	}

	inline void GRCINDEXBUFFERD3D::UnlockRO() const
	{
	}

	inline void GRCINDEXBUFFERD3D::UnlockRW() const
	{
		static_cast<const grcIndexBufferD3D*>(this)->UnlockInternal();
	}

	inline void GRCINDEXBUFFERD3D::UnlockWO() const
	{
		UnlockRW();
	}

	// Cannot easily inline GRCINDEXBUFFERD3D::LockRO here, due to use of D3D structures and macros.
#if !__XENON
	inline const void *GRCINDEXBUFFERD3D::GetUnsafeReadPtr() const
	{
		return m_IndexData;
	}
#endif

#	undef GRCINDEXBUFFERD3D

#endif // __WIN32PC || __XENON || __RESOURCECOMPILER

#if RSG_PC && __D3D11
	inline d3dIndexBuffer *grcIndexBuffer::GetD3DBuffer() const
	{
		return static_cast<const grcIndexBufferD3D11 *>(this)->GetD3DBuffer();
	}
#endif // RSG_PC && __D3D11

#if __PS3 || __RESOURCECOMPILER

#	if __PS3
#		define GRCINDEXBUFFERGCM    grcIndexBuffer
#	else
#		define GRCINDEXBUFFERGCM    grcIndexBufferGCM
#	endif

	inline const u16 *GRCINDEXBUFFERGCM::LockRO() const
	{
		FastAssert(GetIndexCount()>0);
		return m_IndexData;
	}

	inline u16 *GRCINDEXBUFFERGCM::LockRW() const
	{
		FastAssert(GetIndexCount()>0);
		return m_IndexData;
	}

	inline u16 *GRCINDEXBUFFERGCM::LockWO() const
	{
		return LockRW();
	}

	inline u16 *GRCINDEXBUFFERGCM::Lock(u32 /*flags*/, u32 offset, u32 /*size*/) const
	{
		FastAssert(GetIndexCount()>0);
		return (u16*)((char*)m_IndexData.ptr+offset);
	}

	inline void GRCINDEXBUFFERGCM::UnlockRO() const
	{
	}

	inline void GRCINDEXBUFFERGCM::UnlockRW() const
	{
	}

	inline void GRCINDEXBUFFERGCM::UnlockWO() const
	{
		return UnlockRW();
	}

	inline const void *GRCINDEXBUFFERGCM::GetUnsafeReadPtr() const
	{
		return m_IndexData;
	}

#	undef GRCINDEXBUFFERGCM

#endif // __PS3 || __RESOURCECOMPILER

} // namespace rage

#endif // GRCORE_INDEXBUFFER_H
