// 
// system/stockallocator.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "stockallocator.h"

#if __WIN32 && !__OPTIMIZED
#include <crtdbg.h>
#endif
#if __WIN32
#include <malloc.h>		// for non-standard _msize function
#elif __PPU
#include <stdlib.h>
extern "C" size_t malloc_usable_size(void*);
#else
#include <stdlib.h>
#endif

namespace rage {

stockAllocator::stockAllocator()
{
	// Uncomment this to verify memory on any alloc or free.  Slow but effective.
#if __WIN32 && !__OPTIMIZED
	// _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif
}

#if __WIN32PC
void *stockAllocator::Allocate(size_t size,size_t align,int /*heapIndex*/) 
#else
void *stockAllocator::Allocate(size_t size,size_t ASSERT_ONLY(align),int /*heapIndex*/)
#endif
{
#if __WIN32PC || __ASSERT
	if (!align) align=16;
#endif
	if (IsHeapLocked())
	{
		Errorf("Attempting to allocate %u bytes on a locked heap",(u32)size);
	}
#if __WIN32PC
	return _aligned_malloc(size, align); 
#else
	AssertMsg(((align & 0xf)==0) , "need at least 16 byte alignment");
	return malloc(size);
#endif
}

void*
stockAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
    //Because it doesn't Quitf() on failure we can just call Allocate().
    return this->Allocate(size, align, heapIndex);
}

void stockAllocator::Resize(const void *,size_t) {
	// Intentional no-op, this will waste memory.
	// _expand is incompatible with _aligned_... functions, and _aligned_realloc still moves
	// memory unexpectedly, so we're out of options.
}

void stockAllocator::Free(const void *ptr) {

	if (IsHeapLocked())
	{
		Errorf("Attempting to free memory from a locked heap (%p)",ptr);
	}

#if __WIN32PC
	_aligned_free(const_cast<void*>(ptr)); 
#else
	free(const_cast<void*>(ptr));
#endif
}


size_t stockAllocator::GetSize(const void *ptr) const {
#if __WIN32
	return ptr? _msize(const_cast<void*>(ptr)) : 0;
#elif __PPU || __PSP2
	return ptr? malloc_usable_size(const_cast<void*>(ptr)) : 0;
#else
	return 0;
#endif
}


void stockAllocator::SetBreakOnAlloc(int allocId) {
#if __WIN32 && !__OPTIMIZED && defined(_DEBUG)
	_crtBreakAlloc = allocId;
#else
	(void) allocId;
#endif
}


void stockAllocator::SanityCheck() {
#if __WIN32 && !__OPTIMIZED
	Assert(_CrtCheckMemory());
#endif
}


bool stockAllocator::IsValidPointer(const void *
#if __WIN32 && !__OPTIMIZED && defined(_DEBUG)
	ptr
#endif
	) const {
#if __WIN32 && !__OPTIMIZED && defined(_DEBUG)
	return _CrtIsValidHeapPointer(ptr) != 0;
#else
	return true;
#endif
}


size_t stockAllocator::GetMemoryUsed(int /*bucket*/) {
#if __WIN32 && !__OPTIMIZED && defined(_DEBUG)
	_CrtMemState memState;
	_CrtMemCheckpoint(&memState);
	return memState.lTotalCount;
#else
	return 0;
#endif
}
	

size_t stockAllocator::GetMemoryAvailable() {
	return 0x7FFFFFFF;
}

size_t stockAllocator::GetLargestAvailableBlock()
{
	return 0x7FFFFFFF;
}

int stockAllocator::EndLayer(const char *, const char *) {
#if __WIN32 && !__OPTIMIZED
	return _CrtDumpMemoryLeaks();
#else
	return 0;
#endif
}


#if __TOOL
static stockAllocator toolAllocator;
sysMemAllocator* sysMemAllocator_sm_Current = &toolAllocator;
sysMemAllocator* sysMemAllocator_sm_Master = &toolAllocator;
sysMemAllocator* sysMemAllocator_sm_Container = &toolAllocator;
int sysMemAllocator_sm_LockHeap;
#endif
}		// namespace rage

#if __TOOL
void* operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

void* operator new[](size_t size) {
	return _aligned_malloc(size, 16);
}

void* operator new(size_t size,size_t align) {
	return _aligned_malloc(size, align<16? 16:align);
}

void* operator new[](size_t size,size_t align) {
	return _aligned_malloc(size, align<16? 16:align);
}

void operator delete[](void *ptr) {
	if (ptr) _aligned_free(ptr);
}
#endif
