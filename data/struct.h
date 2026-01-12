// 
// data/struct.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include "data/resource.h"
#include "system/alloca.h"
#include "system/memops.h"
#include "system/typeinfo.h"

#if RSG_CPU_X86 && !__RESOURCECOMPILER && !__TOOL && 0
#define DECLARE_PADDED_POINTER(t,v)	union { t* v; unsigned __int64 v##padded; }
#define HAS_PADDED_POINTERS 1
#define PADDED_POINTERS_ONLY(x) x
#else
#define DECLARE_PADDED_POINTER(t,v)	t* v
#define HAS_PADDED_POINTERS 0
#define PADDED_POINTERS_ONLY(x)
#endif

#if RSG_ORBIS
#define _COMBINE_HELPER(x,y)	x ## y
#define _COMBINE(x,y)	_COMBINE_HELPER(x,y)
#define _UNIQUE(x)		_COMBINE(x,__LINE__)
#define PAD_FOR_GCC_X64_1		ATTR_UNUSED u8	_UNIQUE(m_PadForOrbis)
#define PAD_FOR_GCC_X64_2		ATTR_UNUSED u16	_UNIQUE(m_PadForOrbis)
#define PAD_FOR_GCC_X64_4		ATTR_UNUSED u32	_UNIQUE(m_PadForOrbis)
#define PAD_FOR_GCC_X64_8		ATTR_UNUSED u64	_UNIQUE(m_PadForOrbis)
#else
#define PAD_FOR_GCC_X64_1
#define PAD_FOR_GCC_X64_2
#define PAD_FOR_GCC_X64_4
#define PAD_FOR_GCC_X64_8
#endif	//RSG_ORBIS


namespace rage {

// __RESOURCECOMPILER implies __DECLARESTRUCT but the converse is not necessarily true.
#define __DECLARESTRUCT		(!__FINAL && !__SPU && (!__PPU || __DEV) && !RSG_ORBIS)

// Flag which enables or disables byte swapping in the datSwapper functions.  Defaults to false.
#if __RESOURCECOMPILER
extern bool g_ByteSwap;
#else
#define g_ByteSwap false
#endif

#if __DECLARESTRUCT
class datTypeStruct {
public:
	datTypeStruct() : m_Name(NULL), m_Offset(0) { }

	int SetTypeString(const char *name) {
		m_Name = name;
		return 0;
	}

	void AddField(size_t offset,size_t thisSize,const char* name=NULL);

	void VerifySize(size_t s);

private:
	const char *m_Name;
	size_t m_Offset;
};

#if __SPU
inline void datSwapper(u16&) { }
inline void datSwapper(u32&) { }
inline void datSwapper(u64&) { }
inline void datSwapperGeneric(u8&, int) { }
inline void datSwapper(u128&) { }
// Call this to process the hidden count at the start of an array allocation for any type containing a destructor.
inline void datSwapArrayCount(void *,int ) { }
#else
void datSwapper(u16&);
void datSwapper(u32&);
void datSwapper(u64&);
void datSwapperGeneric(u8&, int);
void datSwapper(u128&);
// Call this to process the hidden count at the start of an array allocation for any type containing a destructor.
extern void datSwapArrayCount(void *pointer,int count);
#endif

inline void datSwapper(u8&) { }
inline void datSwapper(bool&) { }
inline void datSwapper(char&) { }
inline void datSwapper(s8&) { }
inline void datSwapper(s16& v) { datSwapper(reinterpret_cast<u16&>(v)); }
inline void datSwapper(s32& v) { datSwapper(reinterpret_cast<u32&>(v)); }
inline void datSwapper(s64& v) { datSwapper(reinterpret_cast<u64&>(v)); }
inline void datSwapper(double& v){ datSwapperGeneric(reinterpret_cast<u8&>(v), sizeof(double)); }
inline void datSwapper(float& v) { datSwapper(reinterpret_cast<u32&>(v)); }

extern void datRelocatePointer(char*&v);
inline void datRelocatePointer(void*&v) { datRelocatePointer((char*&)v); }
inline void datSwapper(char*& v) { datRelocatePointer(v); datSwapper(reinterpret_cast<size_t&>(v)); }
inline void datSwapper(void*& v) {datSwapper((char*&)v);}
inline void datSwapper(const char*&v) { datSwapper((char*&)v); }

#endif

enum spuInit {SPU_INIT};

template <class _T> struct datRef {
	SYS_MEM_OPS_PTR_LIKE_TYPE(_T);
	datRef() { datResource::Fixup(ptr); }
	datRef(datResource &) { datResource::Fixup(ptr); }
	datRef(spuInit) {}
	static void Place(void *that,datResource &) { datResource::Fixup(((datRef<_T>*)that)->ptr); }
	datRef(_T* p) : ptr(p) { }
	_T& operator*() const { return *ptr; }
	_T* operator->() const { return ptr; }
	operator _T*() const { return ptr; }
	_T*&operator=(_T* that) { return ptr=that; }
	DECLARE_PADDED_POINTER(_T,ptr); 
};

template <class _T> struct datOwner {
	SYS_MEM_OPS_PTR_LIKE_TYPE(_T);
	datOwner() { datResource::Place(ptr); }
	datOwner(datResource &) { datResource::Place(ptr); }
	datOwner(spuInit) {}
	template <class _P1> datOwner(datResource &, _P1 p1) { datResource::Place(ptr, p1); }
	template <class _P1, class _P2> datOwner(datResource &, _P1 p1, _P2 p2) { datResource::Place(ptr, p1, p2); }
	static void Place(void *that,datResource &) { datResource::Place(((datOwner<_T>*)that)->ptr); }
	template <class _P1> static void Place(void *that,datResource &, _P1 p1) { datResource::Place(((datOwner<_T>*)that)->ptr, p1); }
	template <class _P1, class _P2> static void Place(void *that,datResource &, _P1 p1, _P2 p2) { datResource::Place(((datOwner<_T>*)that)->ptr, p1, p2); }
	datOwner(_T* p) : ptr(p) { }
	_T& operator*() const { return *ptr; }
	_T* operator->() const { return ptr; }
	operator _T*() const { return ptr; }
	_T*&operator=(_T* that) { return ptr=that; }
	DECLARE_PADDED_POINTER(_T,ptr); 
};

template<typename T, typename U>
inline
T smart_cast( datOwner<U> u )
{
	return smart_cast<T>(u.ptr);
}

template<typename T, typename U>
inline
T smart_cast( datRef<U> u )
{
	return smart_cast<T>(u.ptr);
}

// Helper to explicitly add _Count * sizeof(_T) bytes of padding to a class
template <unsigned int _Count, typename _T = u8> struct datPadding {
public:
	datPadding() { NOTFINAL_ONLY(for (unsigned int i = 0; i < _Count; i++) m[i] = 0); }
	datPadding(datResource&) { }
	bool operator==(int n) { return (n == 0); }
private:
	_T m[_Count];
};

#if __64BIT
#define datPadding64(count,name)	datPadding<count> name;
#define STRUCT_PADDING64(name)		STRUCT_IGNORE(name)
#define SSTRUCT_PADDING64(s,name)	SSTRUCT_IGNORE(s,name)
#else
#define datPadding64(count,name)
#define STRUCT_PADDING64(name)
#define SSTRUCT_PADDING64(s,name)
#endif

#if __DECLARESTRUCT

template <class _T> inline void datSwapper(_T &obj)				{ datTypeStruct s; obj.DeclareStruct(s); }
template <class _T> inline void datSwapper(const _T& obj)		{ datTypeStruct s; const_cast<_T&>(obj).DeclareStruct(s); }
template <class _T> inline void datSwapper(datRef<_T> &ref)		{ datSwapper((char*&)(ref.ptr)); }
template <class _T> inline void datSwapper(datOwner<_T> &ref)	{ if (ref.ptr) datSwapper(*ref.ptr); datSwapper((char*&)(ref.ptr)); }

// Put this declaration before your DeclareStruct if you want to STRUCT_FIELD an enum
#define datSwapper_ENUM(type) \
template <> inline void datSwapper<type>(type& v) { datSwapper(reinterpret_cast<u32&>(v)); }

// Begin a structure declaration
#define STRUCT_BEGIN(type)			int missing_STRUCT_END = s.SetTypeString(#type);

// Declare a field
#define STRUCT_FIELD(field)			do { s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); ::rage::datSwapper(field); } while (0)

// Declare a field as a void* without regard to what it really is (ie only byte swap this pointer)
// NOTES	Some casting and 'MAY_ALIAS' below may be overkill, but was done to eliminate strict-aliasing warnings arisisng from a simple C-style
//			cast to (void*&).
#ifdef __SNC__
#define STRUCT_FIELD_VP(field)		STRUCT_FIELD( ((void *&)(*(::rage::union_cast<void**>(&(field))))) )
#else
#define STRUCT_FIELD_VP(field)		STRUCT_FIELD( ((void MAY_ALIAS *&)(*(::rage::union_cast<void**>(&(field))))) )
#endif

// Declare a field as a pointer to an object owned by this class (use this if you can't use datOwner for some reason)
#define STRUCT_FIELD_OWNED_PTR(field) do { if (field) ::rage::datSwapper(*field); STRUCT_FIELD_VP(field); } while (0)

// Declare an array of "ct" elements (ct can be either a constant or a member variable)
#define STRUCT_CONTAINED_ARRAY_COUNT(field,ct)	\
	do {	for (int i=0; i<(int)(ct); i++) ::rage::datSwapper(field[i]); \
			s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); } while (0)

// Declare an array of "ct" elements (ct can be either a constant or a member variable)
#define STRUCT_CONTAINED_ARRAY_COUNT_VP(field,ct) \
	do {	for (int i=0; i<(int)(ct); i++) ::rage::datSwapper((void*&)field[i]); \
			s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); } while (0)

// Declare a 2 dimensional array of ct1 x ct2 elements
#define STRUCT_CONTAINED_2D_ARRAY_COUNT(field, ct1, ct2) \
	do {	for (int i=0; i<(int)(ct1); i++) for( int j=0; j<(int)(ct2); j++ ) ::rage::datSwapper(field[i][j]); \
			s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); } while (0)

// Declare a 2 dimensional array of ct1 x ct2 elements
#define STRUCT_CONTAINED_2D_ARRAY_COUNT_VP(field, ct1, ct2)  \
	do {	for (int i=0; i<(int)(ct1); i++) for( int j=0; j<(int)(ct2); j++ ) ::rage::datSwapper((void*&)field[i][j]); \
	s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); } while (0)

// Declare a contained array and compute its size automatically
#define STRUCT_CONTAINED_ARRAY(field)	\
	do {	STRUCT_CONTAINED_ARRAY_COUNT(field,sizeof(field)/sizeof(field[0])); \
		} while (0)

// Declare a contained array and compute its size automatically
#define STRUCT_CONTAINED_ARRAY_VP(field)	\
	do {	STRUCT_CONTAINED_ARRAY_COUNT_VP(field,sizeof(field)/sizeof(field[0])); \
		} while (0)

// Declare an array of "ct" elements
#define STRUCT_DYNAMIC_ARRAY_NOCOUNT(field,ct)		\
	do {	if (field) for (int i=0; i<(int)(ct); i++) ::rage::datSwapper(field[i]); \
			s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); \
			::rage::datSwapper((char*&)field); } while (0)

#define STRUCT_DYNAMIC_ARRAY(field,ct)		\
	do {	Assert((size_t)&(field)<(size_t)&ct); \
			STRUCT_DYNAMIC_ARRAY_NOCOUNT(field,ct); } while (0)

// Declare an array of "ct*mult" elements (ct must be a member variable located later in the structure, mult can
//   be any constant)
#define STRUCT_DYNAMIC_ARRAY_MULT(field,ct,mult)		\
	do {	Assert((size_t)&(field)<(size_t)&ct); \
	if (field) for (int i=0; i<(int)(ct*mult); i++) ::rage::datSwapper(field[i]); \
	s.AddField((size_t)&(field)-(size_t)this,sizeof(field),#field); \
	::rage::datSwapper((char*&)field); } while (0)

// Skip space in the structure; only use this as a last resort
#define STRUCT_SKIP(field,count)	s.AddField((size_t)&(field)-(size_t)this,count,#field)

// Takes care of the padding that has been inserted by the compiler between two members for structure packing.
// Please use this ONLY if you're handling two types that you don't know (i.e. in a template where the
// user can specify the types, thus leaving you clueless whether or not there IS any padding).
// In all other cases, insert padding manually and call STRUCT_FIELD on it.
#define STRUCT_POTENTIAL_PADDING(field1, field2)				\
	do { Assert((size_t)&(field1)<(size_t)&(field2));				\
	size_t offset = (size_t) &(field2) - (size_t) &(field1);		\
	if (offset != sizeof(field1)) {								\
		const char *tempPtr = (const char *) &(field1);			\
		tempPtr += sizeof(field1);								\
		s.AddField((size_t)tempPtr-(size_t)this, offset - sizeof(field1),#field1); } \
	} while(0)

//Useful in templated structures where you can't use STRUCT_POTENTIAL_PADDING because there is no trailing
//field to pad to. It is also of particular use automating padding at structure ends. Note that it does not
//make sure that the padding is zeroed out and is a little less secure than defining all data by hand but 
//has proved a timesaver.
#define STRUCT_PAD_TO_ALIGNMENT(field1, align)\
	do {	size_t size = sizeof(field1);\
	const char *tempPriorPtr = ((const char *)&(field1));\
	const char *tempPtr = tempPriorPtr + size ;\
	const char *tempPtr2 = (const char *) ((size_t)tempPtr & (size_t)~(align-1));\
	if (tempPtr2 != tempPtr) {\
	s.AddField((size_t)tempPtr-(size_t)this, (size_t)align - ((size_t)tempPtr - (size_t)tempPtr2),#field1 "align_" #align);\
	}\
	} while(0)


// Mark a field as an ignorable entry (making sure it is zero; useful for pointers not used until runtime and pad bytes)
#define STRUCT_IGNORE(field)		do { Assert(field==0); STRUCT_SKIP(field,sizeof(field)); } while (0)

// Marks the end of the structure; guarantees that the size matches up
// If you get an "unreferenced variable s" warning it's because you forgot your STRUCT_END call.
#define STRUCT_END()				s.VerifySize(sizeof(*this) + missing_STRUCT_END)

#endif	// __DECLARESTRUCT

}	// namespace rage

#endif
