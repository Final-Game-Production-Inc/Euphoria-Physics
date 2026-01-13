#ifndef SPU_SCRATCH_H
#define SPU_SCRATCH_H

#include "atl/array.h"
#include "atl/bitset.h"
#include "system/dma.h"
#include "system/memops.h"
#include "system/debugmemoryfill.h"
#include "paging/ref.h"

#if __SPU

namespace rage {

	struct sysScratchInfo
	{
		u8*     ls;
		u32*    ptr;
		u32     ea;
		u32     requested;

		void    Writeback(u32 tag = 0);
	};

	u8* g_pScratchBegin;
	u8* g_pScratch;
	u8* g_pScratchEnd;
	sysScratchInfo* g_pScratchInfo;

	void sysScratchInfo::Writeback(u32 tag)
	{
		if (this)
		{
			if (requested <= 8)
				sysDmaSmallPut((void*)*ptr, ea, requested, tag);
			else
				sysDmaLargePut((void*)*ptr, ea, (requested+15)&~15, tag);
		}
	}

#if !__ASSERT
#define NOASSERT_INLINE inline
#else
#define NOASSERT_INLINE
#endif

	inline u8* sysScratchTryAlloc(u32 size)
	{
		Assert(!(size&15));
		u8* pData = g_pScratch;
		u8* pEnd = pData + size;
		if(Unlikely(pEnd > (u8*)g_pScratchInfo))
		{
			return NULL;
		}
		g_pScratch = pEnd;
		return pData;
	}

	NOASSERT_INLINE u8* sysScratchAlloc(u32 size)
	{
		Assert(!(size&15));
		u8* pData = g_pScratch;
		g_pScratch += size;
		Assertf(g_pScratch <= (u8*)g_pScratchInfo, "out of scratch space (requested %i bytes, only %i available)", size, (int)((u8*)g_pScratchInfo - pData));
		// Fatal error if we overflow here
		if(g_pScratch > (u8*)g_pScratchInfo)
		{
			Quitf("out of scratch space (requested %i bytes, only %i available)", size, (int)((u8*)g_pScratchInfo - pData));
		}
		return pData;
	}

	inline u8* sysScratchTryAllocAligned(u32 size, u32 align)
	{
		Assert(!(size&15));
		Assert(!(align&15));
		u8* pData = (u8*)(((u32)g_pScratch+align-1)&-align);
		u8* pEnd = pData+size;
		if(Unlikely(pEnd > (u8*)g_pScratchInfo))
		{
			return NULL;
		}
		g_pScratch = pEnd;
		return pData;
	}

	NOASSERT_INLINE u8* sysScratchAllocAligned(u32 size, u32 align)
	{
		Assert(!(size&15));
		Assert(!(align&15));
		ASSERT_ONLY(u8 *const prev=g_pScratch;)
		u8* pData = (u8*)(((u32)g_pScratch+align-1)&-align);
		g_pScratch = pData+size;
		Assertf(g_pScratch <= (u8*)g_pScratchInfo, "out of scratch space (requested %i bytes, only %i available)", size, (int)((u8*)g_pScratchInfo - prev));
		// Fatal error if we overflow here
		if(g_pScratch > (u8*)g_pScratchInfo)
		{
			Quitf("out of scratch space (requested %i bytes, only %i available)", size, (int)((u8*)g_pScratchInfo - pData));
		}
		return pData;
	}

	void sysScratchGetData(u32* ptr, u32 size, u32 dmatag = 0)
	{
		Assert(ptr);
		u32 ea = *ptr;
		if (!ea)
			return;
		u32 endea = ea + size;
		u32 offs = ea & 15;
		ea = ea & ~15;
		endea = (endea + 15) & ~15;
		size = endea - ea;
		u8* pSpuData = sysScratchAlloc(size);
		sysDmaLargeGet(pSpuData, ea, size, dmatag);
		*ptr = (u32)(pSpuData + offs);
	}

	sysScratchInfo* sysScratchGetDataAndSave(u32* ptr, u32 size, u32 dmatag = 0)
	{
		Assert(ptr);
		u32 ea = *ptr;
		if (!ea)
			return 0;
		sysScratchInfo* info = --g_pScratchInfo;
		info->ptr = ptr;
		info->ea = ea;
		info->requested = size;
		info->ls = g_pScratch;
		sysScratchGetData(ptr, size, dmatag);    
		return info;
	}

	inline void sysScratchInit(void* pScratch, u32 size)
	{
		g_pScratchBegin = (u8*)pScratch;
		g_pScratch = (u8*)pScratch;		
		g_pScratchEnd = g_pScratch + size;
		g_pScratchInfo = (sysScratchInfo*)g_pScratchEnd;
	}

	inline u8* sysScratchPtr() 
	{
		return g_pScratch;	
	}

	void sysScratchReset(u8* pCheckpoint)
	{
		Assert( g_pScratchBegin <= pCheckpoint );
		for(; (u8*)g_pScratchInfo < g_pScratchEnd && g_pScratchInfo->ls >= pCheckpoint; ++g_pScratchInfo)
			*g_pScratchInfo->ptr = g_pScratchInfo->ea;
		IF_DEBUG_MEMORY_FILL_N(sysMemSet(pCheckpoint, 0xDD, g_pScratch - pCheckpoint),DMF_GAMEHEAP_FREE);
		g_pScratch = pCheckpoint;
	}

	template<class T>
	inline T* sysScratchAllocObj(u32 count = 1)
	{
		if(__alignof__(T) > 16) return (T*)sysScratchAllocAligned(sizeof(T) * count, __alignof__(T));
		else                    return (T*)sysScratchAlloc((sizeof(T) * count + 15) & ~15);
	}

	template<class T>
	inline void sysScratchGet(const T*& ptr, u32 count = 1, u32 dmatag = 0)
	{
		sysScratchGetData((u32*)&ptr, sizeof(T) * count, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(const T*& ptr, u32 count = 1, u32 dmatag = 0)
	{
		return sysScratchGetDataAndSave((u32*)&ptr, sizeof(T) * count, dmatag);
	}

	template<class T>
	inline void sysScratchGet(T*& ptr, u32 count = 1, u32 dmatag = 0)
	{
		sysScratchGetData((u32*)&ptr, sizeof(T) * count, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(T*& ptr, u32 count = 1, u32 dmatag = 0)
	{
		return sysScratchGetDataAndSave((u32*)&ptr, sizeof(T) * count, dmatag);
	}

	template<>
	inline void sysScratchGet(void*& ptr, u32 count, u32 dmatag)
	{
		sysScratchGetData((u32*)&ptr, count, dmatag);
	}

	template<>
	inline sysScratchInfo* sysScratchGetAndSave(void*& ptr, u32 count, u32 dmatag)
	{
		return sysScratchGetDataAndSave((u32*)&ptr, count, dmatag);
	}

	template<class T>
	inline void sysScratchGet(pgRef<T>& ptr, u32 dmatag = 0)
	{
		sysScratchGet(ptr.ptr, 1, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(pgRef<T>& ptr, u32 count = 1, u32 dmatag = 0)
	{
		return sysScratchGetAndSave(ptr.ptr, count, dmatag);
	}

	template<class T>
	inline void sysScratchGet(datRef<T>& ptr, u32 dmatag = 0)
	{
		sysScratchGet(ptr.ptr, 1, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(datRef<T>& ptr, u32 dmatag = 0)
	{
		return sysScratchGetAndSave(ptr.ptr, 1, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(datRef<T>& ptr, u32 count, u32 dmatag)
	{
		return sysScratchGetAndSave(ptr.ptr, count, dmatag);
	}

	template<class T>
	inline void sysScratchGet(datOwner<T>& ptr, u32 dmatag = 0)
	{
		sysScratchGet(ptr.ptr, 1, dmatag);
	}

	template<class T>
	inline sysScratchInfo* sysScratchGetAndSave(datOwner<T>& ptr, u32 dmatag = 0)
	{
		return sysScratchGetAndSave(ptr.ptr, 1, dmatag);
	}

	template<class _Type, int _Align, class _CounterType>
	struct sysScratchArrayHelper : atArray<_Type, _Align, _CounterType>
	{
		_Type*& Elements() const {return const_cast<sysScratchArrayHelper*>(this)->m_Elements;}
	};

	template<class _Type, int _Align, class _CounterType>
	inline void sysScratchGet(atArray<_Type,_Align,_CounterType>& array, u32 dmatag = 0)
	{
		typedef sysScratchArrayHelper<_Type, _Align, _CounterType> tHelper;
		tHelper& helper = static_cast<tHelper&>(array);
		sysScratchGet(helper.Elements(), array.GetCount(), dmatag);
	}

	template<class _Type, int _Align, class _CounterType>
	inline sysScratchInfo* sysScratchGetAndSave(atArray<_Type,_Align,_CounterType>& array, u32 dmatag = 0)
	{
		typedef sysScratchArrayHelper<_Type, _Align, _CounterType> tHelper;
		tHelper& helper = static_cast<tHelper&>(array);
		return sysScratchGetAndSave(helper.Elements(), array.GetCount(), dmatag);
	}

	template<class _Type, int _Align, class _CounterType>
	inline sysScratchInfo* sysScratchGetAndSave(atArray<_Type,_Align,_CounterType>& array, int count, u32 dmatag = 0)
	{
		typedef sysScratchArrayHelper<_Type, _Align, _CounterType> tHelper;
		tHelper& helper = static_cast<tHelper&>(array);
		return sysScratchGetAndSave(helper.Elements(), (u32)count, dmatag);
	}

	struct sysScratchBitsetHelper : atBitSet 
	{    
		u32*& Bits() const {return const_cast<sysScratchBitsetHelper*>(this)->m_Bits;}
		u32 Size() const {return m_Size;}
	};

	inline void sysScratchGet(atBitSet& bitset, u32 dmatag = 0)
	{    
		sysScratchBitsetHelper& helper = static_cast<sysScratchBitsetHelper&>(bitset);
		sysScratchGet(helper.Bits(), helper.Size(), dmatag);
	}

	inline sysScratchInfo* sysScratchGetAndSave(atBitSet& bitset, u32 dmatag = 0)
	{    
		sysScratchBitsetHelper& helper = static_cast<sysScratchBitsetHelper&>(bitset);
		return sysScratchGetAndSave(helper.Bits(), helper.Size(), dmatag);
	}


//#define SPU_SCRATCH_REPORTING

	struct sysScratchScope
	{
		sysScratchScope() : m_begin(sysScratchPtr()) {m_last = m_begin;}
		~sysScratchScope() {sysScratchReset(m_begin);}
#ifdef SPU_SCRATCH_REPORTING
		void Report(const char* name) 
		{
			Displayf("%6i (%6i free) %s", 
				(int)(sysScratchPtr() - m_last), 
				(int)((u8*)g_pScratchInfo - sysScratchPtr()), 
				name); 
			m_last = sysScratchPtr();
		}
#else
		void Report(const char*) {}
#endif
	private:
		u8* m_begin;
		u8* m_last;
	};

} // namespace rage

#endif // __SPU

#endif // SPU_SCRATCH_H
