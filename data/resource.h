// 
// data/resource.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_RESOURCE_H
#define ATL_RESOURCE_H

#include "data/resourceheader.h"
#include "system/new.h"
#include "system/tls.h"
#include "paging/knownrefpool_config.h"

namespace rage {

extern __THREAD int g_DisableInitNan;

class datResource;

// Work around Sony TLS lameness by using globals instead of class statics.
extern __THREAD datResource* datResource_sm_Current;

#if ENABLE_DEFRAGMENTATION
extern __THREAD bool datResource_IsDefragmentation;
#else
const bool datResource_IsDefragmentation = false;
#endif

/*
The datResourceMap class contains information about the memory structures
that have been allocated to contain the resource.  Note that we support both
virtual and physical memory, each with a separate possible chunk size.

Currently we still only support one chunk per type of memory, although the data
structure are designed to support multiple chunks.
*/
#if RESOURCE_HEADER
enum datResourceType { RESOURCE_TYPE_NONE, RESOURCE_TYPE_RPF, RESOURCE_TYPE_DICTIONARY, RESOURCE_TYPE_SCRIPT };
#endif

struct datResourceMap 
{
	datResourceMap() { Init(); }

	void Init() 
	{
		VirtualCount = PhysicalCount = RootVirtualChunk = HeaderType = 0;
		LastSrc = LastDest = 0;
	}

	void Reset() 
	{
		VirtualCount = PhysicalCount = RootVirtualChunk = 0;
		LastSrc = LastDest = 0;
		DisableMerge = false;
	}

	u8 VirtualCount,	// Number of valid entries in each page table.  This times the page size must be at least
		PhysicalCount,	// as large as the total VirtualSize or PhysicalSize of the resource.  Note that the
		RootVirtualChunk, DisableMerge,
		HeaderType;
	void* VirtualBase;
	// last page only needs to be as large as what is actually used; this leaves us the
	// option of partial defragmentation in the future.

	datResourceChunk Chunks[datResourceChunk::MAX_CHUNKS];	// Virtual pages are always first.  Physical comes next, if any.
	mutable int LastSrc, LastDest;		// Simple "last checked" cache

	void *GetVirtualBase() const { return VirtualBase; }

	int ContainsSrc(void *ptr) const {
		int count = PhysicalCount + VirtualCount;
		if (LastSrc < count && Chunks[LastSrc].ContainsSrc(ptr))
			return LastSrc;

		for (int i=0; i<count; i++)
			if (Chunks[i].ContainsSrc(ptr))
				return LastSrc = i;
		return -1;
	}

	int ContainsDest(void *ptr) const {
		int count = PhysicalCount + VirtualCount;
		if (LastDest < count && Chunks[LastDest].ContainsDest(ptr))
			return LastDest;

		for (int i=0; i<count; i++)
			if (Chunks[i].ContainsDest(ptr))
				return LastDest = i;
		return -1;
	}

	size_t GetVirtualSize() const  {size_t size = 0; for (int i=0; i<VirtualCount; ++i){size+=Chunks[i].Size;}return size;}
	size_t GetPhysicalSize() const {size_t size = 0; for (int i=VirtualCount; i<VirtualCount+PhysicalCount; ++i){size+=Chunks[i].Size;}return size;}

	void Print(char* output, size_t count) const; 
	
#if RESOURCE_HEADER
	bool CanOptimize() const {return (VirtualCount == 1) && (Chunks[0].Size > 0) && (Chunks[0].Size <= g_rscVirtualLeafSize); }	
	bool IsOptimized() const;

	bool HasHeaderType() const { return HeaderType != RESOURCE_TYPE_NONE; }
	bool IsDictionary() const { return HeaderType == RESOURCE_TYPE_DICTIONARY; }
	bool IsRPF() const { return HeaderType == RESOURCE_TYPE_RPF; }
	bool IsScript() const { return HeaderType == RESOURCE_TYPE_SCRIPT; }
#endif
};

#if __SPU

class datResource {
public:
	datResource(void *containerLs,void *containerPpu,size_t containerSize) {
		m_Next = datResource_sm_Current;
		datResource_sm_Current = this;
		m_Fixup = (int)containerLs - (int)containerPpu;
		m_ContainerEa = (u32)containerPpu;
		m_ContainerSize = containerSize;
	}
	~datResource() {
		datResource_sm_Current = m_Next;
	}
	bool ContainsThisAddress(const void *ptr) const {
		return (u32)ptr - m_ContainerEa < m_ContainerSize;
	}
	template <class _Ptr> void PointerFixupNonNull(_Ptr& ptr) const {
		Assertf(ContainsThisAddress(ptr),"Pointer %p not in %x,%x",ptr,m_ContainerEa,m_ContainerSize);
		ptr = (_Ptr)((char*)(ptr) + m_Fixup);
	}
	template <class _Ptr> void PointerFixup(_Ptr& ptr) const {
#if __ASSERT
		if (ptr)
			PointerFixupNonNull(ptr);
#else
		ptr = (_Ptr)((char*)(ptr) + (ptr? m_Fixup : 0));
#endif
	}
	template <class _Ptr> static void Fixup(_Ptr &ptr) {
		datResource *curr = datResource_sm_Current;
		if (ptr && curr)
			curr->PointerFixupNonNull(ptr);
	}
	template <class _Ptr> static void Place(_Ptr &ptr) {
		datResource *curr = datResource_sm_Current;
		if (ptr && curr) {
			curr->PointerFixupNonNull(ptr);
			ptr->Place(ptr,*curr);
		}
	}
	template <class _Ptr,class _P1> static void Place(_Ptr &ptr,_P1 p1) {
		datResource *curr = datResource_sm_Current;
		if (ptr && curr) {
			curr->PointerFixupNonNull(ptr);
			ptr->Place(ptr,*curr,p1);
		}
	}
	template <class _Ptr,class _P1,class _P2> static void Place(_Ptr &ptr,_P1 p1,_P2 p2) {
		datResource *curr = datResource_sm_Current;
		if (ptr && curr) {
			curr->PointerFixupNonNull(ptr);
			ptr->Place(ptr,*curr,p1,p2);
		}
	}

	inline bool IsDefragmentation() const { return false; }
private:
	int m_Fixup;
	u32 m_ContainerEa, m_ContainerSize;
	datResource *m_Next;
} ;

#else

#define TABLE_SEARCH_FIXUPS		((RSG_PS3 || RSG_XENON) && ENABLE_KNOWN_REFERENCES)
#define BINARY_SEARCH_FIXUPS	(1)

#if BINARY_SEARCH_FIXUPS
struct datResourceSortedChunk {
	void *Start;
#if __64BIT
	size_t Size:56, SelfIndex:8;
#else
	size_t Size:24, SelfIndex:8;
#endif
};
#endif

/*
The datResource class contains fixup information necessary to relocate runtime
data from the address it was originally created at to the address is has been
reloaded to on a subsequent run of the game.  Many classes in rage have a so-called
resource constructor which accepts only a datResource& as a parameter, and are
invoked using placement new.
<FLAG Component>
*/
class datResource {
public:
	// PURPOSE:	Constructor
	// PARAMS:	map - Structure containing remap information for virtual and physical memory.
	//			hdr - The original resource header (
	//			debugName - Human-readable name for display if we detect an invalid fixup,
	//				must be valid for the lifetime of the datResource object.
	//			isDefrag - true if this is a result of defragmentation, not initial streamin.
	//				This means that the object may already be in a partially usable state.
	// NOTES:	Internally datResource objects form a stack of nested resources so that
	//			code can determine the active fixup at runtime, automating some sorts
	//			of initialization.  You will probably never have to create a datResource
	//			yourself; it is generally done for you by templated functions in paging/paging.h.
	datResource(const datResourceMap &map,const char *debugName,bool isDefrag = false);

	// PURPOSE: Destructor; datResource objects should always be destroyed in reverse order of creation.
	~datResource();

	// RETURNS:	Fixup value of object (difference between current and original addresses)
	ptrdiff_t GetFixup(void *ptr) const;

	// PURPOSE: Fixes up a pointer type by adding the currently active datResource
	//			fixup value, or setting the pointer to NULL if there is no active datResource
	// PARAMS:	ptr - Pointer to fixup
	// NOTES:	This is used by the atRscRef class.
	template <class _Ptr> static void Fixup(_Ptr &ptr);

	// PURPOSE:	Fixes up a pointer type and then invokes the resource constructor on the
	//			resulting new address.
	// PARAMS:	ptr - Pointer to fixup and resource construct (via a Place function supplied by the type)
	// NOTES:	This is used by the atRscOwner class
	template <class _Ptr> static void Place(_Ptr &ptr);
	template <class _Ptr, class _P1> static void Place(_Ptr &ptr, _P1 p1);
	template <class _Ptr, class _P1, class _P2> static void Place(_Ptr &ptr, _P1 p1, _P2 p2);

	// PURPOSE:	Fixes up a pointer type, verifying that the resulting pointer is still in range.
	// PARAMS:	ptr - Pointer to fix up
	template <class _Ptr> void PointerFixup(_Ptr& ptr);

	// RETURNS: True if the address specified is within this resource heap, false if not
	bool ContainsThisAddress(void *address) const;

	// PURPOSE:	Returns active datResource object, if any
	inline static datResource* GetCurrent();

	// PURPOSE: Returns the base address of the resource (ie the location of the first virtual page)
	inline void *GetBase() const;

	// PURPOSE: Returns the resource debug name
	inline const char* GetDebugName() const {return m_DebugName;}

	// PURPOSE:	Returns whether this operation is a defragmentation or not
	static inline bool IsDefragmentation() { return datResource_IsDefragmentation; }

	inline const datResourceMap &GetMap() const { return m_Map; }

private:
	void operator=(const datResource&);			// UNIMPLEMENTED
	void ResourceFailure(const char *msg) const;
	const datResourceMap &m_Map;
	datResource *m_Next;
	const char *m_DebugName;
#if BINARY_SEARCH_FIXUPS
	datResourceSortedChunk Src[datResourceChunk::MAX_CHUNKS], Dest[datResourceChunk::MAX_CHUNKS];
	u8 m_MapCount;
#endif
	bool m_WasDefrag;
};


template <class _Ptr> PPU_ONLY(__attribute__((noinline))) void datResource::Fixup(_Ptr &ptr) {
	// If there's a fixup in progress (ie a datResource on the stack somewhere) then perform the operation
	if (datResource_sm_Current && datResource_sm_Current->ContainsThisAddress(&ptr))
		datResource_sm_Current->PointerFixup(ptr);
	// Otherwise just null out the pointer, assuming we're in a non-resource context.
	else if (!datResource_IsDefragmentation)
		ptr = NULL;
}


template <class _Ptr> PPU_ONLY(__attribute__((noinline))) void datResource::Place(_Ptr &ptr) {
	// If there's a fixup in progress (ie a datResource on the stack somewhere) then perform the operation
	if (datResource_sm_Current && ptr && datResource_sm_Current->ContainsThisAddress(&ptr)) {
		_Ptr newPtr = (_Ptr)((char*)(ptr) + datResource_sm_Current->GetFixup((void*)ptr));
		newPtr->Place(newPtr, *datResource_sm_Current);
		ptr = newPtr;
	}
	// Otherwise just null out the pointer, assuming we're in a non-resource context.
	else if (!datResource_IsDefragmentation)
		ptr = NULL;
}

template <class _Ptr, class _P1> PPU_ONLY(__attribute__((noinline))) void datResource::Place(_Ptr &ptr, _P1 p1) {
	// If there's a fixup in progress (ie a datResource on the stack somewhere) then perform the operation
	if (datResource_sm_Current && ptr && datResource_sm_Current->ContainsThisAddress(&ptr)) {
		_Ptr newPtr = (_Ptr)((char*)(ptr) + datResource_sm_Current->GetFixup((void*)ptr));
		newPtr->Place(newPtr, *datResource_sm_Current, p1);
		ptr = newPtr;
	}
	// Otherwise just null out the pointer, assuming we're in a non-resource context.
	else if (!datResource_IsDefragmentation)
		ptr = NULL;
}


template <class _Ptr, class _P1, class _P2> PPU_ONLY(__attribute__((noinline))) void datResource::Place(_Ptr &ptr, _P1 p1, _P2 p2) {
	// If there's a fixup in progress (ie a datResource on the stack somewhere) then perform the operation
	if (datResource_sm_Current && ptr && datResource_sm_Current->ContainsThisAddress(&ptr)) {
		_Ptr newPtr = (_Ptr)((char*)(ptr) + datResource_sm_Current->GetFixup((void*)ptr));
		newPtr->Place(newPtr, *datResource_sm_Current, p1, p2);
		ptr = newPtr;
	}
	// Otherwise just null out the pointer, assuming we're in a non-resource context.
	else if (!datResource_IsDefragmentation)
		ptr = NULL;
}


template <class _Ptr> PPU_ONLY(__attribute__((noinline))) void datResource::PointerFixup(_Ptr& ptr) {
	if (ptr)
		ptr = (_Ptr)((char*)(ptr) + GetFixup((void*)ptr));
}


inline datResource* datResource::GetCurrent() { 
	return datResource_sm_Current; 
}


inline void * datResource::GetBase() const {
	return m_Map.GetVirtualBase();
}

#endif	// __SPU


// PURPOSE: Helper macro which declares suitable Place functions.
// PARAMS:	__type - Name of the enclosing class
// NOTES:	datResource::Place will call a class Place function
#define DECLARE_PLACE(__type) \
	static void Place(__type *that,::rage::datResource&)

// PURPOSE:	Helper macro which both declares and defines a dummy Place
//			function for a simple class that doesn't need resource fixups.
// PARAMS:	__type - Name of the enclosing class
#define DECLARE_DUMMY_PLACE(__type) \
	static void Place(__type *,::rage::datResource&) { } \

// PURPOSE: Helper macro which declares suitable Place functions.
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
// NOTES:	datResource::Place will call a class Place function
#define DECLARE_PLACE_1(__type,__arg0) \
	static void Place(__type *that,::rage::datResource&,__arg0)

// PURPOSE:	Helper macro which both declares and defines a dummy Place
//			function for a simple class that doesn't need resource fixups.
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
#define DECLARE_DUMMY_PLACE_1(__type,__arg0) \
	static void Place(__type *,::rage::datResource&,__arg0) { } \

// PURPOSE: Helper macro which declares suitable Place functions.
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
//          __arg1 - Type of first argument
// NOTES:	datResource::Place will call a class Place function
#define DECLARE_PLACE_2(__type,__arg0,__arg1) \
	static void Place(__type *that,::rage::datResource&,__arg0,__arg1)

// PURPOSE:	Helper macro which both declares and defines a dummy Place
//			function for a simple class that doesn't need resource fixups.
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
//          __arg1 - Type of first argument
#define DECLARE_DUMMY_PLACE_2(__type,__arg0,__arg1) \
	static void Place(__type *,::rage::datResource&,__arg0,__arg1) { } \


#if __TOOL

#define IMPLEMENT_PLACE(__type)         void __type::Place(__type *,::rage::datResource &) { }
#define IMPLEMENT_PLACE_INLINE(__type)  static void Place(__type *,::rage::datResource &) { }
#define IMPLEMENT_PLACE_1(__type,__arg0)        void __type::Place(__type *,::rage::datResource &,__arg0) { }
#define IMPLEMENT_PLACE_INLINE_1(__type,__arg0) static void Place(__type *,::rage::datResource &,__arg0) { }
#define IMPLEMENT_PLACE_2(__type,__arg0,__arg1)         void __type::Place(__type *,::rage::datResource &,__arg0,__arg1) { }
#define IMPLEMENT_PLACE_INLINE_2(__type,__arg0,__arg1)  static void Place(__type *,::rage::datResource &,__arg0,__arg1) { }

#else

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
#define IMPLEMENT_PLACE(__type) \
	void __type::Place(__type *that,::rage::datResource &rsc) { \
        ::new (that) __type(rsc); \
}

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
//			This macro is useful for an inline implementation in a header file
#define IMPLEMENT_PLACE_INLINE(__type) \
	static void Place(__type *that,::rage::datResource &rsc) { \
        ::new (that) __type(rsc); \
}

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
#define IMPLEMENT_PLACE_1(__type,__arg0) \
	void __type::Place(__type *that,::rage::datResource &rsc,__arg0 a0) { \
        ::new (that) __type(rsc,a0); \
}

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
//			This macro is useful for an inline implementation in a header file
#define IMPLEMENT_PLACE_INLINE_1(__type,__arg0) \
	static void Place(__type *that,::rage::datResource &rsc,__arg0 a0) { \
        ::new (that) __type(rsc,a0); \
}

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
//          __arg1 - Type of second argument
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
#define IMPLEMENT_PLACE_2(__type,__arg0,__arg1) \
	void __type::Place(__type *that,::rage::datResource &rsc,__arg0 a0,__arg1 a1) { \
        ::new (that) __type(rsc,a0,a1); \
}

// PURPOSE:	Helper macro which implements a typical Place function
// PARAMS:	__type - Name of the enclosing class
//          __arg0 - Type of first argument
//          __arg1 - Type of second argument
// NOTES:	A typical Place function just invokes a resource constructor in-place.
//			However, classes which implement virtual class factories will typically
//			need a switch statement on a contained enumerant to force creation of
//			an appropriate subclass.
//			This macro is useful for an inline implementation in a header file
#define IMPLEMENT_PLACE_INLINE_2(__type,__arg0,__arg1) \
	static void Place(__type *that,::rage::datResource &rsc,__arg0 a0,__arg1 a1) { \
        ::new (that) __type(rsc,a0,a1); \
}

#endif	// !__TOOL

}	// namespace rage

#endif
