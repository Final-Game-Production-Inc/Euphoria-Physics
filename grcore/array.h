// 
// grcore/array.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_ARRAY_H 
#define GRCORE_ARRAY_H 

#include "system/memory.h"
#include "data/resource.h"
#include "data/struct.h"
#include "diag/trap.h"
#if __TOOL
#include <malloc.h>
#endif

namespace rage {

enum eGrcArrayPlaceElements { GRCARRAY_PLACEELEMENTS };

/*
	Minimal version of atArray designed to be shared with SPU's via a memory container.
*/
template <class _Type> class grcArray
{
	explicit grcArray(const grcArray&);			// ILLEGAL

	grcArray& operator=(const grcArray& that);	// ILLEGAL

public:
	// Sigh - grmShader still needs the non-automatic behavior
	explicit grcArray(datResource &rsc) {
		rsc.PointerFixup(m_Elements);
	}
	grcArray(eGrcArrayPlaceElements,datResource &rsc) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++) {
			m_Elements[i].Place(&m_Elements[i],rsc);
		}
	}
	template<class ARG0>
	grcArray(eGrcArrayPlaceElements,datResource &rsc,const ARG0 &a0) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++) {
			m_Elements[i].Place(&m_Elements[i],rsc,a0);
		}
	}
	template<class ARG0,class ARG1>
	grcArray(eGrcArrayPlaceElements,datResource &rsc,const ARG0 &a0,const ARG1 &a1) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++) {
			m_Elements[i].Place(&m_Elements[i],rsc,a0,a1);
		}
	}
#if !__SPU
	grcArray() { 
		m_Elements = NULL;
		m_Count = m_Capacity = 0;
	}
	explicit grcArray(int count,int capacity) {
		Init(count,capacity);
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s) {
		STRUCT_BEGIN(grcArray<_T>);
		STRUCT_DYNAMIC_ARRAY(m_Elements,m_Count);
		STRUCT_FIELD(m_Count);
		STRUCT_FIELD(m_Capacity);
		STRUCT_END();
	}
#endif

	__forceinline _Type& Append() { 
		TrapGE(m_Count,m_Capacity);
		_Type& result = m_Elements[m_Count++];
		rage_placement_new(&result) _Type;
		return result;
	}

	/* Deletes array element at specified index.
		DOES NOT RUN DESTRUCTOR (atArray gets this wrong too, mostly because
		we don't trust our copy constructors and risk doing a double delete)
		PARAMS: index - Index of array element to delete */
	void Delete(int index) {
		TrapGE(index,m_Count);
		for (int i=index; i<m_Count-1; i++)
			m_Elements[i] = m_Elements[i+1];
		--m_Count;
	}

	void Init(int count,int capacity) {
		m_Count = (unsigned short) count;
		m_Capacity = (unsigned short) capacity;
#if __TOOL		// Containers don't work in standard heaps (but we need to guarantee alignment for vector lib so use our heap class)
		m_Elements = (_Type*) _aligned_malloc(capacity * sizeof(_Type),16);
#else
		m_Elements = (_Type*) sysMemAllocator::GetContainer().RAGE_LOG_ALLOCATE(capacity * sizeof(_Type),16);
#endif
		for (int i=0; i<count; i++)
			rage_placement_new(m_Elements+i) _Type;
	}
	__forceinline void Resize(int capacityAndCount) {
		Assert(!m_Elements);
		Init(capacityAndCount,capacityAndCount);
	}

	// PURPOSE:	Allocates plain memory for the array
	__forceinline void Reserve(int capacity) {
		Assert(!m_Elements);
		Init(0,capacity);
	}

	// PURPOSE: Opposite of Reserve; invokes destructors on all real objects, frees underlying memory.
	void Reset() {
		while (m_Count) {
			--m_Count;
			(m_Elements+m_Count)->~_Type();
		}
#if __TOOL	// Containers don't work in standard heaps (but we need to guarantee alignment for vector lib so use our heap class)
		_aligned_free(m_Elements);
#else
		delete[] (char*) m_Elements;
#endif
		m_Elements = NULL;
		m_Capacity = 0;
	}

	~grcArray() {
		Reset();
	}
#endif	// !__SPU

	__forceinline _Type& operator[](unsigned index) { 
		TrapGE(index,(unsigned)m_Count); 
		return m_Elements[index]; 
	}
	__forceinline const _Type& operator[](unsigned index) const { 
		TrapGE(index,(unsigned)m_Count); 
		return m_Elements[index]; 
	}
	__forceinline int GetCount() const { 
		return m_Count; 
	}
	__forceinline void SetCount(int c) {
		m_Count = (unsigned short) c;
	}
	__forceinline _Type* Begin() {
		return m_Elements;
	}
	__forceinline const _Type* Begin() const {
		return m_Elements;
	}
	__forceinline _Type* End() {
		return m_Elements+m_Count;
	}
	__forceinline const _Type* End() const {
		return m_Elements+m_Count;
	}

#if !__SPU
private:
#endif
	_Type *m_Elements;
	unsigned short m_Count, m_Capacity;
};

} // namespace rage

#if __TOOL
// Containers don't work in standard heaps yet.
#define rage_contained_new	rage_new
#else
#define rage_contained_new ::new(sysMemAllocator::GetContainer())

__forceinline void* operator new(size_t size,rage::sysMemAllocator &container) {
	return container.Allocate(size,16);
}
__forceinline void* operator new[](size_t size,rage::sysMemAllocator &container) {
	return container.Allocate(size,16);
}
#endif

#endif // GRCORE_ARRAY_H 
