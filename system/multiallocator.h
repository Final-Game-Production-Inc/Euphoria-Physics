// 
// system/multiallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_MULTIALLOCATOR_H
#define SYSTEM_MULTIALLOCATOR_H

#include "system/memory.h"
#include "atl/array.h"

namespace rage {

extern __THREAD int sysMemBuddyFreeRscMem;


/*
	sysMemMultiAllocator manages multiple heaps under itself; heap zero should
	always be the main game heap, all other heaps should be streaming heaps.
	Heap one is the default virtual memory streaming heap.	Heap two is the 
	default physical memory streaming heap.  
*/
class sysMemMultiAllocator: public sysMemAllocator 
{
public:
	int AddAllocator(sysMemAllocator&);
	int ReserveAllocator();
	void SetAllocator(int,sysMemAllocator&);

	sysMemMultiAllocator();
	virtual ~sysMemMultiAllocator();

	virtual void *Allocate(size_t size,size_t align,int heapIndex);
	virtual void *TryAllocate(size_t size,size_t align,int heapIndex);
	virtual void Resize(const void *ptr,size_t newSmallerSize);
	virtual void Free(const void *ptr);
	virtual size_t GetSize(const void *ptr) const;
	virtual size_t GetSizeWithOverhead(const void *ptr) const;
	virtual size_t GetMemoryUsed(int bucket);
	virtual size_t GetMemoryAvailable();
	virtual size_t GetLargestAvailableBlock();
#if !__FINAL
	virtual void SetBreakOnAlloc(int /*id*/);
#endif
	virtual void SanityCheck();
	virtual bool IsValidPointer(const void *ptr) const;
	virtual void BeginLayer();
	virtual int EndLayer(const char *layerName,const char *leakfile);
	virtual void BeginMemoryLog(const char *filename,bool logStackTracebacks);
	virtual void EndMemoryLog();
	virtual unsigned GetNumAllocators() const;
	virtual const sysMemAllocator *GetAllocator(int heapIndex) const;
	virtual sysMemAllocator *GetAllocator(int heapIndex);
	virtual sysMemAllocator *GetPointerOwner(const void *ptr);
	virtual const void *GetCanonicalBlockPtr(const void *ptr) const;
	virtual bool TryLockBlock(const void *, unsigned);
	virtual void UnlockBlock(const void *, unsigned);
	virtual u32 GetUserData(const void *) const;
	virtual void SetUserData(const void *, u32 userData);

private:
	atFixedArray<sysMemAllocator*,8> m_Heaps;
};

}	// namespace rage

#endif
