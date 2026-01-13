// 
// grcore/vertexbuffer.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_VERTEXBUFFER_H
#define GRCORE_VERTEXBUFFER_H

#include "data/base.h"
#include "data/struct.h"

#include "diag/stats.h" // __STATS

#include "grcore/fvf.h"
#include "grcore/config.h"
#include "grcore/opengl.h"
#include "grcore/locktypes.h"
#include "grcore/device.h"
#include "grcore/buffer_durango.h"

#include "system/ipc.h"

#if __D3D11
#include "grcore/buffer_d3d11.h"
#elif RSG_ORBIS

namespace rage { 
	enum grcBufferCreateType
	{
		grcsBufferCreate_NeitherReadNorWrite	= 0x0, // Expects data to be available upon creation (equivalent to D3D11_USAGE_IMMUTABLE)
		grcsBufferCreate_DynamicWrite = 0x5 
	}; 
	enum grcsBufferSyncType
	{
		grcsBufferSync_None				= 0x3 // Expects only to be updated on the render thread or not at all.
	};
}
#endif //__D3D11

#define __VBSTATS (__STATS && 0)
#if __VBSTATS
#	define VBSTATS_ONLY(...)    __VA_ARGS__
#else
#	define VBSTATS_ONLY(...)
#endif

#if __XENON
typedef struct D3DVertexBuffer d3dVertexBuffer;
#elif __D3D11
typedef struct ID3D11Buffer d3dVertexBuffer;
#elif __D3D9
typedef struct IDirect3DVertexBuffer9 d3dVertexBuffer;
#endif

// D3DVERTEXBUFFER_ALIGNMENT is only 4, but we use vectorized instructions which will silently
// fail on unaligned memory (dammit) so we need something stricter.
#define RAGE_VERTEXBUFFER_ALIGNMENT	16

#if __RESOURCECOMPILER
#define RESOURCECOMPILER_PURE_VIRTUAL(SIGNATURE)    virtual SIGNATURE = 0
#else
#define RESOURCECOMPILER_PURE_VIRTUAL(SIGNATURE)    SIGNATURE
#endif

#define VERTEX_BUFFER_D3D_INVALID_ADDRESS 0xdeadbeef

namespace rage
{
#if __WIN32PC && !__RESOURCECOMPILER
	class grcVertexManager
	{
	public:
		grcVertexManager();
	private:
		static bool Lost();
		static bool Reset();
	};
#endif // __WIN32PC && !__RESOURCECOMPILER

	class grcVertexBuffer: public datBase
	{
		friend class grcVertexBufferEditor;

		grcVertexBuffer() {}
	protected:
		grcVertexBuffer(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, void* preallocatedMemory);
	public:
		virtual bool IsValid() const = 0;

		class LockHelper
		{
		public:
			LockHelper(const grcVertexBuffer* vertexBuffer, bool readOnly = false)
			: m_VertexBuffer(vertexBuffer), m_ReadOnly(readOnly)
			{
				if (m_VertexBuffer)
				{
#if !__SPU
					Assert(m_VertexBuffer->IsValid());
#endif
					if (readOnly)
					{
						AssertVerify(m_VertexBuffer->LockRO());
					}
					else
					{
						const u32 flags = m_VertexBuffer->IsDynamic() ? grcsNoOverwrite /*grcsDiscard*/ : grcsWrite;
						AssertVerify(m_VertexBuffer->Lock(flags));
					}
				}
			}

			LockHelper(const grcVertexBuffer* vertexBuffer, u32 uLockType, u32 uOffset, u32 uSize)
			: m_VertexBuffer(vertexBuffer), m_ReadOnly(grcVertexBuffer::AreLockFlagsReadOnly(uLockType))
			{
				if (m_VertexBuffer)
				{
#if !__SPU
					Assert(m_VertexBuffer->IsValid());
#endif
					AssertVerify(m_VertexBuffer->Lock(uLockType, uOffset, uSize));
				}
			}

			~LockHelper()
			{
				if (m_VertexBuffer)
				{
#if !__SPU
					Assert(m_VertexBuffer->IsValid());
#endif
					if(m_ReadOnly)  m_VertexBuffer->UnlockRO();
					else            m_VertexBuffer->UnlockRW();
				}
			}

			void* GetLockPtr() const
			{
				FastAssert(m_VertexBuffer);
				return m_VertexBuffer->GetFastLockPtr();
			}

		private:
			const grcVertexBuffer*	m_VertexBuffer;
			bool                    m_ReadOnly;
		};

		explicit grcVertexBuffer(class datResource& rsc);
		~grcVertexBuffer();

		DECLARE_PLACE(grcVertexBuffer);

		static grcVertexBuffer* Create(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, void* preAllocatedMemory );
#if RSG_PC && __D3D11
		static grcVertexBuffer* Create(int vertCount, const grcFvf& pFvf, grcBufferCreateType CreateType, grcsBufferSyncType SyncType, void* preAllocatedMemory, bool AllocateCPUCopy = false);
		static grcVertexBuffer* CreateWithData(int VertCount, const grcFvf& pFvf, grcBufferCreateType CreateType, const void *pVertexData, bool AllocateCPUCopy = false);
#endif // RSG_PC && __D3D11
#if __D3D11
		static grcVertexBuffer* Clone(class grcVertexBuffer *pExisting);
#endif //__D3D11

#if RSG_DURANGO || RSG_ORBIS
		static grcVertexBuffer* CreateWithData(int VertCount, const grcFvf& pFvf, const void *pVertexData);
#endif // RSG_DURANGO || RSG_ORBIS

		void MakeReadOnly();

		static inline bool AreLockFlagsReadOnly(u32 flags) {return (flags & grcsRead) && !(flags & (grcsWrite | grcsNoOverwrite | grcsDiscard));}

		RESOURCECOMPILER_PURE_VIRTUAL( const void *LockRO() const );
		RESOURCECOMPILER_PURE_VIRTUAL( void *LockWO() const );
		RESOURCECOMPILER_PURE_VIRTUAL( void *LockRW() const );
		RESOURCECOMPILER_PURE_VIRTUAL( void *Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const );
		RESOURCECOMPILER_PURE_VIRTUAL( void UnlockRO() const );
		RESOURCECOMPILER_PURE_VIRTUAL( void UnlockWO() const );
		RESOURCECOMPILER_PURE_VIRTUAL( void UnlockRW() const );

		// PURPOSE: Gets the memory address that would be returned from LockRO,
		// but without obtaining a lock.  This pointer is only intended to be
		// used for locking the memory region against defragging.  The pointer
		// should not be dereferenced.
		RESOURCECOMPILER_PURE_VIRTUAL( const void *GetUnsafeReadPtr() const );

	#if RSG_PC && __D3D11
		d3dVertexBuffer *GetD3DBuffer() const;
	#endif

		virtual bool Lost()									{ return true; }
		virtual bool Reset()								{ return true; }

		//this is for releasing the vertex buffer for cloth in the fragment system, used in place of delete JGG :(
		virtual void ReleaseD3DBufferHack() {}

		void					SetStride(u16 uStride)			{ m_Stride = uStride; } // Warning SetFvf will override this value
		u32						GetVertexStride() const			{ return m_Stride; }
		const grcFvf*			GetFvf() const					{ return m_Fvf; }
		int						GetVertexCount() const			{ return m_VertCount; }
		bool                    IsReadWrite() const             { return (m_Flags & (kReadWrite|kDynamic)) != 0; } // TODO: once all data has been rebuilt, should not need to test kDynamic, should be an assert instead
		bool					IsDynamic() const				{ return (m_Flags & kDynamic) != 0; }
		bool					IsPreallocatedMemory() const	{ return (m_Flags & kPreallocatedMemory) != 0; }
		void*					GetLockPtr() const				{ return m_LockPtr; }
		void*					GetFastLockPtr() const			{ return m_LockPtr; }
		void*					GetVertexData()					{ return m_VertexData; }

	#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s) = 0;
	#endif

	protected:
	#if __RESOURCECOMPILER
		void ByteSwapBuffer();
		void ByteSwap( int vertIndex );
	#endif

	protected:
		enum Flags
		{
			kDynamic = 1 << 0,
			kPreallocatedMemory = 1 << 1,
			kReadWrite = 1 << 2             // kDynamic => kReadWrite, but !(kReadWrite => kDynamic)
		};
		u16								m_Stride;				// +4
		u8                              m_reserved0;            // +6
		u8								m_Flags;				// +7
		DECLARE_PADDED_POINTER(mutable void,m_LockPtr);				// +8
		u32								m_VertCount;			// +12
		datRef<u8>						m_VertexData;			// +16 -- on PS3, this is really the offset of the vertex buffer.
		u32								m_reserved;				// +20
		datOwner<grcFvf>				m_Fvf;					// +24

	protected:
		void SetFvf(grcFvf* fvf);
		void SetVertexData(u8* data)	{ m_VertexData = data; }
#if __BANK
	public:
#endif
		void SetDynamic();
	};

#if __D3D && !__D3D11
	class grcVertexBufferD3D: public grcVertexBuffer {
		friend class grcVertexBufferEditor;
		friend class grcVertexManager;
	public:
	#if __WIN32PC
		struct D3DInternalData
		{
			uptr                reserved;
			u32					m_Pool;
			u32					m_LockType;
			grcVertexBufferD3D*	m_Next;
			grcVertexBufferD3D*	m_Previous;
			u32					m_uOffset;
			u32					m_uSize;
		};
		static grcVertexBufferD3D *sm_First;
	#endif
	
		grcVertexBufferD3D(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory);
		grcVertexBufferD3D(class datResource& rsc);
		~grcVertexBufferD3D();

#if __XENON
		void LockInternal(bool invalidateReadOnlyCache) const;
#endif
		void UnlockInternal() const;

#if __RESOURCECOMPILER
		virtual const void *LockRO() const;
		virtual void *LockWO() const;
		virtual void *LockRW() const;
		virtual void *Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
		virtual void UnlockRO() const;
		virtual void UnlockWO() const;
		virtual void UnlockRW() const;
		virtual const void *GetUnsafeReadPtr() const;
#endif

#if __RESOURCECOMPILER
		virtual bool IsValid() const { return !g_ByteSwap || m_D3DBuffer!=NULL; }
#else
		virtual bool IsValid() const { return m_D3DBuffer!=NULL; }
#endif 

#if __WIN32PC && !__RESOURCECOMPILER
		virtual bool Lost();
		virtual bool Reset();
#endif // __WIN32PC

		d3dVertexBuffer*		GetD3DBuffer() const	{ return m_D3DBuffer; }
		d3dVertexBuffer* const*	GetppD3DBuffer() const	{ return &m_D3DBuffer; }

		//this is for releasing the vertex buffer for lcoth in the fragment system, used in place of delete JGG :(
		virtual void			ReleaseD3DBufferHack();

#if __WIN32PC
		D3DInternalData* GetInternalData() const { return (D3DInternalData*)storage; }
#endif // __WIN32PC

	#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
	#endif
	private:
		void SetD3DBuffer(d3dVertexBuffer* vertexBuffer) { m_D3DBuffer = vertexBuffer; }
		void CreateInternal();

		DECLARE_PADDED_POINTER(d3dVertexBuffer,m_D3DBuffer);	// +28			
		size_t							storage[8];
		PADDED_POINTERS_ONLY(size_t moreStorage[8]);
	};

	CompileTimeAssertSize(grcVertexBufferD3D,HAS_PADDED_POINTERS? 128 : 64,128);
#endif //__D3D && !__D3D11


#if RSG_PC && __D3D11
	class grcVertexBufferD3D11: public grcVertexBuffer {
		friend class grcVertexBufferEditor;
		friend class grcVertexManager;

	public:
		struct D3D11InternalData
		{
			grcVertexBufferD3D11*	m_Next;
			grcVertexBufferD3D11*	m_Previous;
			grcBufferD3D11			m_Buffer;
		};
		static grcVertexBufferD3D11 *sm_First;

		// Construction/Destruction related functions.
	public:
		grcVertexBufferD3D11(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, u8* preAllocatedMemory = NULL, bool AllocateCPUCopy = false);
		grcVertexBufferD3D11(class datResource& rsc);
		~grcVertexBufferD3D11();
		virtual void ReleaseD3DBufferHack();
	private:
		void CreateInternal(grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, bool AllocateCPUCopy);
		void CleanUpInternal();
#if TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO
	public:
		void RecreateInternal();
#endif //TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO

		// Lock()/Unlock() related functions.
	public:
#if __RESOURCECOMPILER
		virtual const void *LockRO() const;
		virtual void *LockWO() const;
		virtual void *LockRW() const;
		virtual void *Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
		virtual void UnlockRO() const;
		virtual void UnlockWO() const;
		virtual void UnlockRW() const;
		virtual const void *GetUnsafeReadPtr() const;
#endif

		void LockInternal(u32 flags, u32 offset, u32 size) const;
		void UnlockInternal() const;

		// General access functions.
	public:
		D3D11InternalData* GetInternalData() const { return (D3D11InternalData*)storage; }
		virtual bool IsValid() const { return GetInternalData()->m_Buffer.GetD3DBuffer() != NULL; }
		d3dVertexBuffer*	GetD3DBuffer() const	{ return GetInternalData()->m_Buffer.GetD3DBuffer(); }

		// Device reset/lost.
	private:
		virtual bool Lost();
		virtual bool Reset();

		// Resource builder functions.
	public:
	#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
	#endif

	private:
		// NOTE:- THESE DATA MEMBERS MUST MATCH grcVertexBufferD3D UNTIL WE BUILD DX11 RESOURCES.
		d3dVertexBuffer*				m_D3DBuffer_Unused;	// +28	
		typedef size_t					Storage[8];
		Storage							storage;
		CompileTimeAssert(sizeof(Storage) >= sizeof(D3D11InternalData));
	};

	CompileTimeAssertSize(grcVertexBufferD3D11,64,128);
#endif // RSG_PC && __D3D11

#if RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)
	class grcVertexBufferGNM: public grcVertexBuffer {
		friend class grcVertexBufferEditor;
	public:
		grcVertexBufferGNM(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory);
		grcVertexBufferGNM(class datResource& rsc);
		~grcVertexBufferGNM();
		virtual bool IsValid() const { return m_VertexData!=NULL; }

#if __RESOURCECOMPILER
		virtual const void *LockRO() const;
		virtual void *LockWO() const;
		virtual void *LockRW() const;
		virtual void *Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
		virtual void UnlockRO() const;
		virtual void UnlockWO() const;
		virtual void UnlockRW() const;
		virtual const void *GetUnsafeReadPtr() const;
#endif
		void SwizzleVertexData();
#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
#endif
	};

#endif // RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)


#if RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)
	class grcVertexBufferDurango: public grcVertexBuffer {
		friend class grcVertexBufferEditor;
	public:
		grcVertexBufferDurango(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory);
		grcVertexBufferDurango(class datResource& rsc);
		~grcVertexBufferDurango();
		void ReleaseD3DBufferHack();
		bool IsValid() const { return m_Buffer.IsValid(); }

		const void *LockRO() const;
		void *LockWO() const;
		void *LockRW() const;
		void *Lock(u32 flags, u32 offset, u32 size) const;
		void UnlockRO() const;
		void UnlockWO() const;
		void UnlockRW() const;
		const void *GetUnsafeReadPtr() const;
		void Unlock(u32 flags) const;

	#if RSG_PC
		d3dVertexBuffer *GetD3DBuffer() const;
	#endif

	#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
	#endif
	private:
		grcBufferDurango m_Buffer;
	};
#endif // RSG_DURANGO || (RSG_PC && __64BIT && __RESOURCECOMPILER)


	class grcVertexBufferGCM: public grcVertexBuffer {
		friend class grcVertexBufferEditor;
	public:
		grcVertexBufferGCM(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory);
		grcVertexBufferGCM(class datResource& rsc);
		~grcVertexBufferGCM();
		virtual bool IsValid() const { return m_GCMBuffer!=NULL; }
		void*				GetGCMBuffer() const	{ return m_GCMBuffer; }

#if __RESOURCECOMPILER
		virtual const void *LockRO() const;
		virtual void *LockWO() const;
		virtual void *LockRW() const;
		virtual void *Lock(u32 flags = 0, u32 offset = 0, u32 size = 0) const;
		virtual void UnlockRO() const;
		virtual void UnlockWO() const;
		virtual void UnlockRW() const;
		virtual const void *GetUnsafeReadPtr() const;
#endif

#if __DECLARESTRUCT
		virtual void DeclareStruct(datTypeStruct &s);
#endif
	private:
		DECLARE_PADDED_POINTER(void,m_GCMBuffer);
	};


#undef RESOURCECOMPILER_PURE_VIRTUAL

	inline void grcVertexBuffer::SetFvf(grcFvf* fvf)
	{
		m_Fvf = fvf;
		m_Stride = (u16)m_Fvf->GetTotalSize();
	}

	inline void grcVertexBuffer::SetDynamic()
	{
		m_Flags |= (kDynamic | kReadWrite);
	}

#if RSG_PC || RSG_XENON || RSG_RSC

#	if __RESOURCECOMPILER
#		define GRCVERTEXBUFFERD3D           grcVertexBufferD3D
#	else
#		define GRCVERTEXBUFFERD3D           grcVertexBuffer
#	endif

#	if __D3D11
#		define GRCVERTEXBUFFERD3D_DERIVED   grcVertexBufferD3D11
#	else
#		define GRCVERTEXBUFFERD3D_DERIVED   grcVertexBufferD3D
#	endif

	// Cannot easily inline GRCINDEXBUFFERD3D::LockRO here, due to use of D3D structures and macros.
#if !__XENON
	inline const void *GRCVERTEXBUFFERD3D::LockRO() const
	{
		VBSTATS_ONLY(PF_FUNC(LockRO);)
		m_LockPtr = m_VertexData;
		Assert(m_LockPtr);
		return m_LockPtr;
	}
#endif // !__XENON

	inline void *GRCVERTEXBUFFERD3D::LockWO() const
	{
		VBSTATS_ONLY(PF_FUNC(LockWO);)
		Assert(IsValid());
#if __D3D11
			static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->LockInternal(grcsWrite, 0, 0);
#elif __XENON
			const bool invalidateReadOnlyCache = false;
			static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->LockInternal(invalidateReadOnlyCache);
#else
			m_LockPtr = m_VertexData;
#endif
		Assert(m_LockPtr);
		return m_LockPtr;
	}

	inline void *GRCVERTEXBUFFERD3D::LockRW() const
	{
		VBSTATS_ONLY(PF_FUNC(LockRW);)
		Assert(IsValid());
#if __D3D11
			static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->LockInternal(grcsRead|grcsWrite, 0, 0);
#elif __XENON
			const bool invalidateReadOnlyCache = true;
			static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->LockInternal(invalidateReadOnlyCache);
#else
			m_LockPtr = m_VertexData;
#endif
		Assert(m_LockPtr);
		return m_LockPtr;
	}

	inline void *GRCVERTEXBUFFERD3D::Lock(u32 flags, u32 offset, u32 size) const
	{
		(void) size;
		VBSTATS_ONLY(PF_FUNC(Lock);)
		if(AreLockFlagsReadOnly(flags))
		{
			LockRO();
		}
		else
		{
#if __D3D11
			static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->LockInternal(flags, offset, size);
#else
			LockRW();
#endif
		}
		return (char*)m_LockPtr + offset;
	}

	inline void GRCVERTEXBUFFERD3D::UnlockRO() const
	{
		VBSTATS_ONLY(PF_FUNC(UnlockRO);)
	}

	inline void GRCVERTEXBUFFERD3D::UnlockWO() const
	{
		VBSTATS_ONLY(PF_FUNC(UnlockRW);)
		static_cast<const GRCVERTEXBUFFERD3D_DERIVED*>(this)->UnlockInternal();
	}

	inline void GRCVERTEXBUFFERD3D::UnlockRW() const
	{
		UnlockWO();
	}

	// Cannot easily inline GRCINDEXBUFFERD3D::GetUnsafeReadPtr here, due to use of D3D structures and macros.
#if !__XENON
	inline const void *GRCVERTEXBUFFERD3D::GetUnsafeReadPtr() const
	{
		return m_VertexData;
	}
#endif

#	undef GRCVERTEXBUFFERD3D_DERIVED
#	undef GRCVERTEXBUFFERD3D

#endif // RSG_PC || RSG_XENON || RSG_RSC

#if RSG_PC && __D3D11
inline d3dVertexBuffer *grcVertexBuffer::GetD3DBuffer() const
{
	return static_cast<const grcVertexBufferD3D11 *>(this)->GetD3DBuffer();
}
#endif // RSG_PC && __D3D11

#if RSG_ORBIS
#		define GRCVERTEXBUFFERGNM grcVertexBuffer

	inline const void *GRCVERTEXBUFFERGNM::GetUnsafeReadPtr() const
	{
		return NULL;
	}
#endif // RSG_ORBIS

#if __PS3 || __RESOURCECOMPILER

#	if __PS3
#		define GRCVERTEXBUFFERGCM grcVertexBuffer
		extern u32 g_AllowVertexBufferVramLocks;
#	else
#		define GRCVERTEXBUFFERGCM grcVertexBufferGCM
#	endif

	inline const void *GRCVERTEXBUFFERGCM::LockRO() const
	{
		VBSTATS_ONLY(PF_FUNC(LockRO);)
		PS3_ONLY(AssertMsg(g_AllowVertexBufferVramLocks || IsDynamic(), "Trying to lock VRAM vertex buffer, really slow!");)
		m_LockPtr = static_cast<const grcVertexBufferGCM*>(this)->GetGCMBuffer();
		return m_LockPtr;
	}

	inline void *GRCVERTEXBUFFERGCM::LockWO() const
	{
		VBSTATS_ONLY(PF_FUNC(LockWO);)
		PS3_ONLY(AssertMsg(g_AllowVertexBufferVramLocks || IsDynamic(), "Trying to lock VRAM vertex buffer, really slow!");)
		m_LockPtr = static_cast<const grcVertexBufferGCM*>(this)->GetGCMBuffer();
		return m_LockPtr;
	}

	inline void *GRCVERTEXBUFFERGCM::LockRW() const
	{
		return LockWO();
	}

	inline void *GRCVERTEXBUFFERGCM::Lock(u32 /*flags*/, u32 offset, u32 /*size*/) const
	{
		PS3_ONLY(AssertMsg(g_AllowVertexBufferVramLocks || IsDynamic(), "Trying to lock VRAM vertex buffer, really slow!");)
		m_LockPtr = static_cast<const grcVertexBufferGCM*>(this)->GetGCMBuffer();
		return (char*)m_LockPtr+offset;
	}

	inline void GRCVERTEXBUFFERGCM::UnlockRO() const
	{
		VBSTATS_ONLY(PF_FUNC(UnlockRO);)
	}

	inline void GRCVERTEXBUFFERGCM::UnlockWO() const
	{
		VBSTATS_ONLY(PF_FUNC(UnlockWO);)
	}

	inline void GRCVERTEXBUFFERGCM::UnlockRW() const
	{
		UnlockWO();
	}

	inline const void *GRCVERTEXBUFFERGCM::GetUnsafeReadPtr() const
	{
		return static_cast<const grcVertexBufferGCM*>(this)->GetGCMBuffer();
	}

#	undef GRCVERTEXBUFFERGCM

#endif // __PS3 || __RESOURCECOMPILER



} // namespace rage

#endif
