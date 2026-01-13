//
// atl/array.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_ARRAY_H
#define ATL_ARRAY_H

#if __SPU &&  defined(SIMPLIFIED_ATL)

#define USING_SIMPLIFIED_ATL 1

#include "data/resource.h"

namespace rage {

#define ValidateSpuPtr(ptr) FastAssert((unsigned)(ptr) < 256*1024)

template <class _Type,int _Align = 0,class _CounterType = unsigned short>
class atArray
{
public:
	atArray() {}

	atArray(datResource& rsc) {
		rsc.PointerFixup(m_Elements);
	}

	atArray(datResource& rsc, bool ) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			_Type::Place(&m_Elements[i], rsc);
	}

	atArray(datResource& rsc, bool, bool) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			_Type::Place(&m_Elements[i], rsc, true);
	}

	static void Place(void *that,datResource &rsc)			{ ::new (that) atArray<_Type>(rsc); }
	static void Place(void *that,datResource &rsc,bool flag)	{ ::new (that) atArray<_Type>(rsc, flag); }

	_Type& operator[](unsigned i) 
	{
		ValidateSpuPtr(m_Elements);
		return m_Elements[i]; 
	}
	const _Type& operator[](unsigned i) const
	{
		ValidateSpuPtr(m_Elements);
		return m_Elements[i]; 
	}
	int GetCount() const { return m_Count; }

	/* RETURNS: Number of items preallocated in the array.  Not necessarily the same as the number of items in the array*/
	int GetCapacity() const { return m_Capacity; }

	DECLARE_PADDED_POINTER(_Type,m_Elements);
	_CounterType m_Count, m_Capacity;
};

template <class _Type,int maxCount> class atFixedArray
{
public:
	_Type& operator[](unsigned i) { return (_Type&)m_Elements[i]; }
	const _Type& operator[](unsigned i) const { return (_Type&)m_Elements[i]; }
	int GetCount() const { return m_Count; }

	char m_Elements[maxCount][sizeof(_Type)];
	int m_Count;
};

template <class _Type,int size> class atRangeArray
{
public:
	_Type& operator[](unsigned i) { return m_Elements[i]; }
	const _Type& operator[](unsigned i) const { return m_Elements[i]; }

	_Type m_Elements[size];
};

} // namespace rage

#else		// !__SPU

#define ValidateSpuPtr(ptr)

#define USING_SIMPLIFIED_ATL 0

#include <iterator>
#include <utility>
#include <stdlib.h>		// for qsort, sigh

#include "data/resource.h"
#include "data/serialize.h"
#include "data/struct.h"
#include "diag/tracker.h"
#include "diag/trap.h"

#define ATL_ARRAY_USE_MARKERS 0

#if ATL_ARRAY_USE_MARKERS
#define DATA_MARKER_INTENTIONAL_HEADER_INCLUDE 1
#include "data/marker.h"
#define ATL_ARRAY_FUNC() RAGE_FUNC()
#else
#define ATL_ARRAY_FUNC()
#endif

namespace rage {

#ifndef ATL_ARRAY_MIN_ALIGN
#if __RESOURCECOMPILER
extern int ATL_ARRAY_MIN_ALIGN;
#else
#define ATL_ARRAY_MIN_ALIGN 16
#endif
#endif 

// broken out so that it can be selectively disabled separately if it's a time sink
#ifndef ArrayAssert
# if __ASSERT
# define ArrayAssert(x)	FastAssert(x)
# define ARRAY_ASSERT_ONLY(x) x
# else
# define ArrayAssert(x)
# define ARRAY_ASSERT_ONLY(x)
# endif
#endif

#if __DECLARESTRUCT
class datTypeStruct;
#endif

/* STL typedefs to make a container a valid STL container and therefore compatible to STL algorithms */

#define STL_TYPEDEFS \
	typedef	_Type value_type;													/* The type of object being stored in the container. */ \
	typedef int size_type;													/* Capable of holding the size of the largest possible object that can be allocated. */ \
	typedef int	difference_type; 											/* Can represent the difference between two addresses. */ \
	typedef value_type&  reference; 										/* A reference to an object of type value_type. */ \
	typedef const value_type& const_reference; 								/* A const reference to an object of type value_type. */ \
	typedef _Type* iterator; 													/* In this implementation an iterator is nothing more than a pointer to an element. */ \
	typedef const _Type* const_iterator; \
	typedef std::reverse_iterator<iterator> reverse_iterator; \
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;




template <class _Type,int _Align,class _CounterType> class atArray;
template <class _Type,int _Align,class _CounterType> datSerialize& operator<<(datSerialize &ser, atArray<_Type,_Align,_CounterType> &);

/*
PURPOSE:
	atArray is a lightweight wrapper around a normal C array, except with built-in range checking and
	the ability to specify its capacity at runtime instead of compile time.  By design, it will
	not allow you to resize the array after creation without destroying its contents.  Random access is O(1).
PARAMETERS:
	_Type - the type of the Array's element
	_Align - the minimum alignment of the array element allocation
	_CounterType - the type of the array counter (default unsigned short)
NOTES:
	Be careful in __TOOL builds to NOT use a _Type that requires alignement!
<FLAG Component>
*/

template <class _Type,int _Align = 0,class _CounterType = unsigned short>
class atArray {
public:
	static const u32 _CounterMax = (_CounterType)-1;
	CompileTimeAssert(sizeof(_CounterType) >= sizeof(unsigned short) && sizeof(_CounterMax) >= sizeof(unsigned short));

private:

	// Internal helper function, runs default constructor on untyped array
	char* Allocate(int count)
	{
		RAGE_TRACK(atArray);
		char *result;
#if __TOOL
		if( __alignof(_Type) > 8 ) // > size of a POD
		{
			result = (char*)_aligned_malloc(count*sizeof(_Type), __alignof(_Type));
		}
		else
#endif
		{
			int align = __alignof(_Type);
			if (_Align)
			{
				if (_Align > align) 
					align = _Align;
			}
			else if (ATL_ARRAY_MIN_ALIGN > align) 
			{
				align = ATL_ARRAY_MIN_ALIGN;
			}
			result = rage_aligned_new(align) char[count * sizeof(_Type)];
		}
		return result;
	}
	_Type* Construct(int count)
	{
		ATL_ARRAY_FUNC();
		char *result = Allocate(count);
		for (int i=0; i<count; i++)
			::new (result + i * sizeof(_Type)) _Type;
		return (_Type*) result;
	}
	// Internal helper function, runs default constructor on untyped array
	_Type* CopyConstruct(int count, _Type* values )
	{
		ATL_ARRAY_FUNC();
		char *result = Allocate(count);
		for (int i=0; i<count; i++)
			::new (result + i * sizeof(_Type)) _Type( values[i]);
		return (_Type*) result;
	}
	// Internal helper function, runs destructor on untyped array
	void Destruct(_Type *elements,int count)
	{
#if __TOOL
		if( __alignof(_Type) > 8 )
		{
			if( elements )
			{
				for (int i=0; i<count; i++)
					(elements+i)->~_Type();
				_aligned_free( (void*)elements );
			}
		}
		else
#endif
		{
			for (int i=0; i<count; i++)
				(elements+i)->~_Type();
			::delete [] (char*) elements;
		}		
	}

public:
	STL_TYPEDEFS;

	/* Default constructor.  Array is left totally empty */
	atArray() : m_Elements(0), m_Count(0), m_Capacity(0) { }

	/* Initialize array allocation to specified size, but array still contains zero elements
		PARAMS: baseCount - Number of entries to preallocate */
	atArray(u32 count,u32 capacity) : m_Count((_CounterType) count), m_Capacity((_CounterType) capacity)
	{	ArrayAssert(count <= capacity);
		ArrayAssert(capacity<_CounterMax);
		m_Elements = capacity ? Construct(capacity) : NULL;
	}

	static void Place(void *that,datResource &rsc)			{ ::new (that) atArray<_Type>(rsc); }
	static void Place(void *that,datResource &rsc,bool flag)	{ ::new (that) atArray<_Type>(rsc, flag); }

	/* Intended for classes that need resource compiler support */
	explicit atArray(bool initialize) {if (initialize) {m_Elements=0; m_Count=0; m_Capacity=0;}}

	explicit atArray(datResource &rsc) { rsc.PointerFixup(m_Elements); }

	explicit atArray(spuInit) {}

	/* We can't change the default rsc ctor without breaking a bunch of stuff. */
	atArray(datResource &rsc,bool ARRAY_ASSERT_ONLY(flag)) {
		ArrayAssert(flag);
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			m_Elements[i].Place(&m_Elements[i],rsc);
	}

	/* Resourcing support for nested atArrays. */
	atArray(datResource &rsc,bool ARRAY_ASSERT_ONLY(flag1),bool flag2) 
	{
		ArrayAssert(flag1);
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			m_Elements[i].Place(&m_Elements[i], rsc, flag2);
	}

	/* Default destructor */
	~atArray() { Kill(); }

	/* Construct an array from another array.  Original array is unchanged.
		PARAMS: that - Array to copy initial data from */
	explicit atArray(const atArray& that) : m_Elements(0), m_Count(0), m_Capacity(0)  {  CopyFrom(that); }

	/* Copy an array from another array.  Other array is unchanged, this array has its contents replaced.
		PARAMS: that - Array to copy data from */
	atArray& operator=(const atArray& that) { if (this != &that) { if( GetCount() != that.GetCount() ) Kill(); CopyFrom(that); } return *this; }

	/* Assume ownership of an array without doing a copy, destroying the other array in the process.
		PARAMS: that - Array to assume control of.  The array is left empty.
		NOTES: Useful when doing batch delete operations */
	void Assume(atArray& that) {
		if (this != &that) { 
			Kill(); 
			m_Elements = that.m_Elements; m_Count = that.m_Count; m_Capacity = that.m_Capacity; //lint !e423 creation of memory leak
			that.m_Elements = 0; that.m_Count = that.m_Capacity = 0;
		}
	}

	/* Access array, with range checking */
	__forceinline _Type& operator[](u32 index) { TrapGE(index,(u32)m_Count); return m_Elements[index]; }

	/* Access array, with range checking */
	__forceinline const _Type& operator[](u32 index) const { TrapGE(index,(u32)m_Count); return m_Elements[index]; }

	/* RETURNS: Number of valid items in the array.  Not necessarily the same as the number of entries allocated */
	__forceinline int GetCount() const { return m_Count; }

	// PURPOSE: Pointer to the counter
	__forceinline _CounterType* GetCountPointer() { return &m_Count; }

	// PURPOSE: Restart the count at 0, to reuse the array without reallocating memory.
	__forceinline void ResetCount() { m_Count = 0; }

	/* RETURNS: Number of items preallocated in the array.  Not necessarily the same as the number of items in the array*/
	// int GetMaxCount() const { return m_Capacity; } // Current Capacity

	/* RETURNS: Number of items preallocated in the array.  Not necessarily the same as the number of items in the array*/
	__forceinline int GetCapacity() const { return m_Capacity; }

	/* Presizes an array to specified capacity.  Array must be empty first, call Reset first if necessary. */
	void Reserve(u32 capacity) {
		// If m_Capacity is zero, count must already be zero
		ArrayAssert(!m_Capacity);
		ArrayAssert(capacity<_CounterMax);
		m_Elements = capacity ? Construct(capacity) : NULL;
		m_Capacity = (_CounterType) capacity; 
	}

	/* Resizes the array.  Must be within current capacity, except in the special case
		where the array was already empty. */
	void Resize(u32 count) {
		ArrayAssert(count<_CounterMax);
		if (m_Capacity) {
			TrapGT(count, (u32) m_Capacity);
		}
		if (!m_Capacity)
		{
			m_Capacity = (_CounterType) count;
			m_Elements = count ? Construct(count) : NULL;
		}
		m_Count = (_CounterType) count;
	}

	/* Resizes the array.  May exceed current capacity, in which case the capacity will
	   grow to the desired size. */
	void ResizeGrow(u32 count) {
		ArrayAssert(count<_CounterMax);
		if (!m_Capacity || count <= m_Capacity) {
			Resize(count);
			return;
		}
		int oldCapacity = m_Capacity;
		m_Capacity = _CounterType(count);
		_Type *e = Construct(count);
		for (_CounterType i=0; i<m_Count; i++)
			e[i] = m_Elements[i];
		Destruct(m_Elements,oldCapacity);
		m_Elements = e;
		m_Count = (_CounterType) count;
	}

	/* Clears array and reclaims its storage */
	void Reset(bool destruct = true) {
		if (destruct)
		{
			Destruct(m_Elements,m_Capacity);
		}
		m_Elements = 0; 
		m_Capacity = m_Count = 0; 
	}

	/* RETURNS: Reference to a newly appended array element. */
	__forceinline _Type& Append() { 
		TrapGE(m_Count,m_Capacity);
		return m_Elements[m_Count++]; //lint !e662
	}

	/* RETURNS: Reference to a newly appended array element, with resize. 
	   PARAMS: allocStep - resize step */
	_Type& Grow(u32 allocStep = 16) {
		if (m_Count == m_Capacity) {
			TrapGT((m_Capacity + allocStep) , _CounterMax);
			m_Capacity = _CounterType(m_Capacity + allocStep);
			_Type *e = Construct(m_Capacity);
			for (_CounterType i=0; i<m_Count; i++)
				e[i] = m_Elements[i];
			Destruct(m_Elements,m_Count);
			m_Elements = e;
		}
		return Append();
	}

	/* PURPOSE: Allocate a new element via Grow() and construct it using
	   a constructor that takes one argument.
     */
	template<typename _Arg1>
	void GrowAndConstruct1(_Arg1 &arg1,_CounterType allocStep = 16) {
		::new(&Grow(allocStep)) _Type(arg1); 
	}

	//
	// PURPOSE
	//	Copies elements from a C-style array.
	// PARAMS
	//	ptrArray - a point to the C-style array containing the elements to copy
	//	count - the number of elements in ptrArray
	//
	void CopyFrom( const _Type* ptrArray,_CounterType count)
	{
		Resize(count);
		for (_CounterType i=0;i<count;i++)
		{
			m_Elements[i]=ptrArray[i];
		}
	}

	//
	// PURPOSE
	//	Does of an atArray shallow copy.  This is only intended to be used on an empty atArray.
	// PARAMS
	//	ptrArray - a point to the C-style array containing the elements pointer to copy, along with it's count and capacity
	//
	void CopyShallow( const atArray &that) 
	{
		FastAssert( m_Capacity == 0 );
		m_Elements = that.m_Elements;
		m_Capacity = that.m_Capacity;
		m_Count = that.m_Count;
	}

	/* Inserts an empty slot into an existing array
		PARAMS: beforeIndex - Array index to insert an empty slot before
		RETURNS: Reference to the empty slot
		NOTES: The slot is not really empty.  It will contain whatever
			array data happened to be there before. */
	_Type& Insert(int beforeIndex) {
		ArrayAssert(m_Count != m_Capacity);
		ArrayAssert(beforeIndex>=0 && beforeIndex <= m_Count);
		for (int i=m_Count; i>beforeIndex; i--)
			m_Elements[i] = m_Elements[i-1];
		++m_Count;
		return m_Elements[beforeIndex]; //lint !e797
	}

	/* Deletes array element at specified index
		PARAMS: index - Index of array element to delete */
	void Delete(int index) {
		ArrayAssert(index>=0 && _CounterType(index) < m_Count);
		for (_CounterType i=_CounterType(index); i<m_Count-1; i++)
			m_Elements[i] = m_Elements[i+1];
		--m_Count;
	}

	/* Deletes array element at specified index
		PARAMS: index - Index of array element to delete 
		NOTE: This is ONLY useful if the order of array elements is not important! 
				Also, this means that the element in index may still be valid */
	void DeleteFast(int index) {
		ArrayAssert(index>=0 && _CounterType(index) < m_Count);
		m_Elements[index] = m_Elements[--m_Count];
	}

	/* Deletes all entries in an array matching parameter
		PARAMS: match - Item to check against
		NOTES: Array element's class must define operator==. */
	void DeleteMatches(const _Type& match) {
		for (_CounterType i=0; i<m_Count; )
			if (m_Elements[i] == match)
				Delete(i);
			else
				++i;
	}

	/* Treat array like a stack and push element to top
		PARAMS: t - Item to append to end of array */
	__forceinline void Push(const _Type& t) {
		Append() = t; 
	}

	/* Treat array like a stack and push element to top, except grow the array if needed
		PARAMS: 
			t - Item to append to end of array 
			allocStep - resize step
	*/
	void PushAndGrow(const _Type& t,_CounterType allocStep = 16) {
		Grow(allocStep) = t; 
	}

	/* Treat array like a stack and return reference to topmost element
		RETURNS: Reference to top of stack
		NOTES: Throws an assert if the array is empty */
	__forceinline _Type& Top() {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1]; //lint !e662 possible creation of out-of-bounds pointer		
	}
	__forceinline const _Type& Top() const {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1]; //lint !e662 possible creation of out-of-bounds pointer		
	}

	/* Treat array like a stack and pops last element off of array
		RETURNS: Reference to (previous) top of stack
		NOTES: Throws an assert if the array is empty */
	__forceinline _Type& Pop() { 
		TrapZ(m_Count); 
		return m_Elements[--m_Count]; //lint !e662 possible creation of out-of-bounds pointer
	}

	/* Searches array for a match, working forward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is zero)
		RETURNS: Index of first match, or -1 if none found */
	int Find(const _Type& t, int start = 0) const {
		int i;
		ArrayAssert(start>=0);
		for (i=start; i<m_Count; ++i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}

	/* Searches array for a match, working backward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is -1, which means the end of the array)
		RETURNS: Index of first match, or -1 if none found */
	int ReverseFind(const _Type& t, int start = -1) const {
		int i;
		if ( start < 0 )
			start = m_Count - 1;
		ArrayAssert(start<m_Count);
		for (i=start; i>=0; --i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}

	/* Searches array for a match, using a log N binary search
		PARAMS:
			t - Element to search for
		RETURNS: Index of first match, or -1 if none found.
		NOTES:	Array must be in sorted order and define < and == operators against _Type */
	int BinarySearch(const _Type& t) const {
		int low = 0;
		int high = m_Count-1;
		while (low <= high) {
			int mid = (low + high) >> 1;
			if (t == m_Elements[mid])
				return mid;
			else if (t < m_Elements[mid])
				high = mid-1;
			else
				low = mid+1;
		}
		return -1;
	}

	/* Sorts the array using qsort
		PARAMS:
			start - the index of the array to start the sort on
			indices - the number of indices of the array to sort
			compare - comparison function		 
		NOTES:	If indices = -1 the entire array will be sorted after the start index
		If you're sorting an array of pointers, the comparison function must have this signature:
		int func(MyObj* const* a, MyObj* const* b);*/
	void QSort(int start, int indices, int (*compare)(const _Type*, const _Type*))
	{
		ArrayAssert(start >= 0);
		if( indices < 0 )
			indices = m_Count - start;
		qsort(&m_Elements[start], indices, sizeof(_Type), (int (/*__cdecl*/ *)(const void*, const void*))compare);
	}

	void Swap(atArray &that)
	{
		_Type* pTempElements = m_Elements;
		_CounterType tempCount = m_Count;
		_CounterType tempCapacity = m_Capacity;
		m_Elements = that.m_Elements;
		m_Count = that.m_Count;
		m_Capacity = that.m_Capacity;
		that.m_Elements = pTempElements;
		that.m_Count = tempCount;
		that.m_Capacity = tempCapacity;
	}

	__forceinline  void SetElements(_Type* elements) { m_Elements = elements; }
	__forceinline  _Type*& GetElements() { return m_Elements; }
	__forceinline  const _Type* GetElements() const { return m_Elements; }

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
	void DatSwapAllButContents(); // swaps the atArray data but NOT the contents of the array
#endif

	/* Functions needed to make this container STL compliant: */
	iterator begin()				{ return &m_Elements[0]; }
	iterator end()					{ return &m_Elements[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	const_iterator begin() const	{ return &m_Elements[0]; }
	const_iterator end() const		{ return &m_Elements[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	reverse_iterator rbegin()		{ return reverse_iterator(end()); }
	reverse_iterator rend()			{ return reverse_iterator(begin());	}
	const_reverse_iterator rbegin() const	{ return const_reverse_iterator(end());	}
	const_reverse_iterator rend()	const	{ return const_reverse_iterator(begin()); }
	void clear()					{ erase(begin(), end()); }
	__forceinline bool empty() const				{ return GetCount()==0; }
	iterator erase(iterator where);
	iterator erase(iterator first, iterator last);
	iterator insert(iterator where, const _Type &value);
	void insert(iterator where, size_type numNewElements, const _Type &value);
	template<class InputIterator> void insert(iterator where, InputIterator srcFirst, InputIterator srcLast);
	size_type max_size( ) const		{ return _CounterMax-1; }
	size_type size( ) const			{ return GetCount(); }
	__forceinline void swap(atArray &that)		{ Swap(that); }
	_Type& back()					{ FastAssert( m_Count > 0 ); return m_Elements[m_Count-1];}
	// @RSNE @BEGIN -Nicholas Howe: support back on const atArray.
	const _Type& back() const		{ FastAssert( m_Count > 0 ); return m_Elements[m_Count-1];}
	// @RSNE @END -Nicholas Howe

	template <class _T, unsigned int _A, class _CT> 
	friend void swap(atArray<_T,_A,_CT> &left,  atArray<_T,_A,_CT> &right);

protected:
	/* Internal copy helper.  Allocates array of same size and copies elements.
		PARAMS: that - Array to copy */
	void CopyFrom(const atArray &that) {
		FastAssert( m_Capacity >= m_Count );
		if ( m_Count != that.m_Count )
		{
			m_Capacity = m_Count = that.m_Count;
			if(m_Capacity)
				m_Elements = Construct(m_Capacity); //lint !e672 possible memory leak in assignment to pointer
			else
				m_Elements = NULL;
		}
		copy(that.m_Elements, that.m_Elements + m_Count, m_Elements);
	}
	/* Internal cleanup function.  Reclaims all heap memory associated with elements */
	void Kill() { 
		if( m_Capacity ) Destruct(m_Elements,m_Capacity); 
	}

	/* 
	PARAMS:
		first		- first element to copy.
		last		- position one past the final element to copy
		destination	- First element of the destination range
	NOTES:
		Cannot be used to create new elements or insert elements into an empty
		container.*/
	void copy(iterator first, iterator last, iterator destination) {
		for (; first != last; destination++, first++)
			*destination = *first;
	}

	_Type *Insert(int insertionIndex, int numNewElements);

	/* The raw array of elements */
	DECLARE_PADDED_POINTER(_Type,m_Elements);
	_CounterType m_Count, /* Number of valid items in the array */
		m_Capacity;		/* Total number of entries allocated in the array.  Always greater than or equal to m_Count. */
	datPadding64(4,m_Padding);	// Could make _CounterType default to an unsigned but a bunch of code assumes int's are enough...

	// Serialize method -- allow reallocation as needed
	friend datSerialize& operator<< <>(datSerialize &ser, atArray<_Type,_Align,_CounterType> &array);
};


template <class _Type,int _Align,class _CounterType>
typename atArray<_Type,_Align,_CounterType>::iterator atArray<_Type,_Align,_CounterType>::erase(iterator where)
{
	ArrayAssert(where != end());
	return erase(where, where+1);
}


/*
NOTES:
  * Doesn't actually delete anything or call any destructor. Merely shifts already 
	existing elements down over the ones that get 'erased' and adjusts the
	element count. However additional copies of elements will still hang around
	until overwritten with something else. This is in line with the overall behavior
	of atArray.
*/
template <class _Type,int _Align,class _CounterType>
typename atArray<_Type,_Align,_CounterType>::iterator atArray<_Type,_Align,_CounterType>::erase(iterator first, iterator last)
{
	ArrayAssert(first>=begin() && (first<end() || (first==last && first==end())));
	ArrayAssert(last>=begin() && last<=end());
	copy(last, end(), first);
	m_Count= m_Count - static_cast<_CounterType>(last - first);
	return first;
}


/*
Inserts empty elements.
PARAMS:	insertionIndex	- the first new slot will have this index.
		numNewElements	- the number of slots to insert.
RETURNS: Address of the first empty slot.
NOTES: 
  * The new slots aren't really empty.  They will contain whatever
	array data happened to be there before.
  * May grow the array and allocate memory.
*/
template <class _Type,int _Align,class _CounterType>
_Type *atArray<_Type,_Align,_CounterType>::Insert(int insertionIndex, int numNewElements)
{
	ArrayAssert(insertionIndex>=0 && insertionIndex <= m_Count);
	ArrayAssert(numNewElements > 0);
	_Type							*insertionPtr= begin() + insertionIndex;
	if(m_Count + numNewElements > m_Capacity)													//Does the array have to grow?
	{
		const _CounterType		newCapacity= _CounterType(m_Count + numNewElements);
		_Type						*newElements = Construct(newCapacity);

		copy(begin(), insertionPtr, newElements);												//copy elements before the ones that get newly inserted
		copy(insertionPtr, end(), newElements+insertionIndex+numNewElements);					//copy elements following newly inserted values
		Destruct(m_Elements,m_Capacity);
		m_Elements= newElements;
		m_Capacity= newCapacity;
	}
	else
	{
		// use of copy here is a bug, because copy works from bottom to top
		// instead of the other way around
		memmove(insertionPtr+numNewElements, insertionPtr, sizeof(_Type)*(m_Count-insertionIndex));
		//copy(insertionPtr, end(), insertionPtr+numNewElements);									//shift elements following insertion position numNewElements up
	}

	m_Count = m_Count + static_cast<_CounterType>(numNewElements);
	return &m_Elements[insertionIndex];
}


template <class _Type,int _Align,class _CounterType> 
typename atArray<_Type,_Align,_CounterType>::iterator atArray<_Type,_Align,_CounterType>::insert(iterator where, const _Type &value)
{
	_Type			*newElement= Insert(ptrdiff_t_to_int(where - begin()), 1);
	*newElement= value;
	return newElement;
}

template <class _Type,int _Align,class _CounterType> 
void atArray<_Type,_Align,_CounterType>::insert(iterator where, size_type numNewElements, const _Type &value)
{
	if(numNewElements == 0)
		return;
	else
	{
		_Type			*newElements= Insert(ptrdiff_t_to_int(where - begin()), (int)numNewElements);
		for(_Type *dest=newElements; dest<newElements+numNewElements; dest++)
			*dest= value;
		return;
	}
}

/*
NOTES:
	The current implementation does not work with (supposedly rare) pure input
	iterators due to its use of std::distance(). Input operators allow only one
	read access to each	element and std::distance() already uses up that quota.
	To properly	support pure input iterators iterator traits must be used to
	detect that	case and branch to a less efficient implementation.
*/
template <class _Type,int _Align,class _CounterType>
template<class InputIterator> void atArray<_Type,_Align,_CounterType>::insert(iterator where, InputIterator srcFirst, InputIterator srcLast)
{
	const int numNewElements= (int) std::distance(srcFirst, srcLast);							//Cheap for random access iterators, expensive otherwise.
	_Type				*newElements= Insert((int)(where - begin()), numNewElements);
	for(_Type *dest=newElements; dest<newElements+numNewElements; dest++, srcFirst++)
	{
		ArrayAssert(srcFirst!=srcLast);
		*dest= *srcFirst;
	}
	return;
}


template <class _Type,int _Align,class _CounterType> 
void swap(atArray<_Type,_Align,_CounterType> &left,  atArray<_Type,_Align,_CounterType> &right)
{
	left.swap(right);
}


/*
PURPOSE:
	Sometimes you need to pass an atArray into a function in order to return a variable amount of data,
	but you don't want the performance penalty of a dynamic allocation.  atUserArray is derived
	from atArray, so you can use it as a drop-in replacement.  But beware, if the code you pass the
	atArray to tries to reallocate the array, you will suffer a horrible death.  So use with care.
PARAMETERS:
	_Type - the type of the Array's element
	_CounterType - the type of the array counter (default unsigned short)
<FLAG Component>
*/
template <class _Type,int _Align=0,class _CounterType = unsigned short>
class atUserArray : public atArray<_Type, _Align, _CounterType> {
private:
	typedef atArray<_Type, _Align, _CounterType> _Base;
public:
	atUserArray()
	{
		_Base::m_Elements = NULL;
		_Base::m_Count = _Base::m_Capacity = 0;
	}
	atUserArray(_Type* elements, _CounterType capacity)
	{
		_Base::m_Elements = elements;
		_Base::m_Capacity = capacity;
		_Base::m_Count	  = 0;
	}
	atUserArray(_Type* elements, _CounterType count, bool UNUSED_PARAM(preFilled))
	{
		_Base::m_Elements = elements;
		_Base::m_Count = _Base::m_Capacity = count;
	}
	~atUserArray()
	{
		_Base::m_Elements = NULL;
		_Base::m_Count = _Base::m_Capacity = 0;
	}

	// assume ownership of storage
	void Assume(_Type* elements, _CounterType capacity)
	{
		Assert(!_Base::m_Elements);
		_Base::m_Elements = elements;
		_Base::m_Capacity = capacity;
		_Base::m_Count	  = 0;
	}
};


template <class _Type,int _MaxCount> class atFixedArray;
template <class _Type,int _MaxCount> datSerialize& operator<<(datSerialize &ser, atFixedArray<_Type,_MaxCount> &);		

/*
PURPOSE:
	atFixedArray is nearly identical to atArray except that it's parameterized by max size and contains
	the array directly instead of requiring an extra pointer access.  Its main advantages over a normal
	C array are range checking on parameters and a built-in element count.
PARAMETERS:
	_Type - the type of the Array's element
	_MaxCount - the maximum # of instances that this array can hold
<FLAG Component>
*/
template <class _Type,int _MaxCount>
class atFixedArray {
public:
	STL_TYPEDEFS;

	/* Default constructor.  Array is left totally empty */
	atFixedArray() : m_Count(0) { }

	/* Default constructor.  Array is left totally empty */
	explicit atFixedArray(int count) : m_Count(count) { ArrayAssert(count >= 0 && count <= _MaxCount); }

	/* Construct an array from another array.  Original array is unchanged.
	PARAMS: that - Array to copy initial data from */
	explicit atFixedArray(const atFixedArray& that) { CopyFrom(that); }

	/* Copy an array from another array.  Other array is unchanged, this array has its contents replaced.
	PARAMS: that - Array to copy data from */
	atFixedArray& operator=(const atFixedArray& that) { if (this != &that) { CopyFrom(that); } return *this; }

	/* Resource constructor - assumes that the members need to be fixed up as well, just
	 * like its atArray counterpart. However, since the elements are members of this class, they
	 * are fixed up automatically. If they happen to be atArrays, calling this constructor will
	 * make sure that they get fixed up with their members intact. */
	atFixedArray(datResource &UNUSED_PARAM(rsc),bool ARRAY_ASSERT_ONLY(flag)) {
		ArrayAssert(flag);
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	/* Default destructor. */
	~atFixedArray() { }

	/* Access array, with range checking */
	__forceinline _Type& operator[](int index) { TrapGE((u32)index,(u32)m_Count); return m_Elements[index]; }

	/* Access array, with range checking */
	__forceinline const _Type& operator[](int index) const { TrapGE((u32)index,(u32)m_Count); return m_Elements[index]; }

	/* RETURNS: Number of valid items in the array.  Not necessarily the same as the number of entries allocated */
	__forceinline int GetCount() const { return m_Count; }

	/* PARAMS: ct - New element count for array; must be between zero and _MaxCount */
	void SetCount(int ct) { ArrayAssert(ct>=0&&ct<=_MaxCount); m_Count=ct; }

	/* RETURNS: Number of items preallocated in the array.  Not necessarily the same as the number of items in the array */
	__forceinline int GetMaxCount() const { return _MaxCount; }

	// RETURNS: Number of available slots in the queue
	__forceinline int GetAvailable() const { return GetMaxCount() - GetCount(); }

	// RETURNS: True if array is empty, else false
	__forceinline bool IsEmpty() const { return m_Count == 0; }

	/* RETURNS: True if the array is full. */
	__forceinline bool IsFull() const { return m_Count == _MaxCount; }

	/* Clears array */
	__forceinline void Reset() { 
		m_Count = 0; 
	}
	/* Sets size of array */
	__forceinline void Resize(int count) {
		TrapGT((u32) count, (u32) _MaxCount);
		m_Count = count;
	}

	/* Does nothing, here to match other array class APIs */
	void Reallocate(const int ARRAY_ASSERT_ONLY(newCount))	{
		ArrayAssert(newCount <= _MaxCount);
	}

	/* RETURNS: Reference to a newly appended array element. Asserts if array is full. */
	__forceinline _Type& Append() { 
		TrapGE(m_Count,_MaxCount);
		return m_Elements[m_Count++];
	}

	/* Inserts an empty slot into an existing array
		PARAMS: beforeIndex - Array index to insert an empty slot before
		RETURNS: Reference to the empty slot
		NOTES: The slot is not really empty.  It will contain whatever
			array data happened to be there before.  Throws an assert if
			the array is already full. */
	_Type& Insert(int beforeIndex) {
		ArrayAssert(m_Count != _MaxCount);
		ArrayAssert(beforeIndex>=0 && beforeIndex <= m_Count);
		for (int i=m_Count; i>beforeIndex; i--)
			m_Elements[i] = m_Elements[i-1];
		++m_Count;
		return m_Elements[beforeIndex];
	}

	/* Deletes array element at specified index
		PARAMS: index - Index of array element to delete */
	void Delete(int index) {
		ArrayAssert(index>=0 && index < m_Count);
		for (int i=index; i<m_Count-1; i++)
			m_Elements[i] = m_Elements[i+1];
		--m_Count;
	}

	/* Deletes array element at specified index
		PARAMS: index - Index of array element to delete 
		NOTE: This is ONLY useful if the order of array elements is not important! 
				Also, this means that the element in index may still be valid */
	void DeleteFast(int index) {
		ArrayAssert(index>=0 && index < m_Count);
		m_Elements[index] = m_Elements[--m_Count];
	}

	/* Treat array like a stack and push element to top
		PARAMS: t - Item to append to end of array */
	__forceinline void Push(const _Type& t) {
		Append() = t; 
	}

	/* Treat array like a stack and return reference to topmost element
		RETURNS: Reference to top of stack
		NOTES: Throws an assert if the array is empty */
	__forceinline _Type& Top() {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1];		
	}
	__forceinline const _Type& Top() const {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1];		
	}

	/* Treat array like a stack and pops last element off of array
		RETURNS: Reference to (previous) top of stack
		NOTES: Throws an assert if the array is empty */
	__forceinline _Type& Pop() { 
		TrapZ(m_Count); 
		return m_Elements[--m_Count];	
	}

	/* Searches array for a match, working forward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is zero)
		RETURNS: Index of first match, or -1 if none found */
	int Find(const _Type& t, int start = 0) const {
		int i;
		ArrayAssert(start>=0);
		for (i=start; i<m_Count; ++i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}

	/* Searches array for a match, working backward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is -1, which means the end of the array)
		RETURNS: Index of first match, or -1 if none found */
	int ReverseFind(const _Type& t, int start = -1) const {
		int i;
		if ( start < 0 )
			start = m_Count - 1;
		ArrayAssert(start<m_Count);
		for (i=start; i>=0; --i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}
	
	/* Searches array for a match, using a log N binary search
	PARAMS:
	t - Element to search for
	RETURNS: Index of first match, or -1 if none found.
	NOTES:	Array must be in sorted order and define < and == operators against _Type */
	int BinarySearch(const _Type& t) const {
		int low = 0;
		int high = m_Count-1;
		while (low <= high) {
			int mid = (low + high) >> 1;
			if (t == m_Elements[mid])
				return mid;
			else if (t < m_Elements[mid])
				high = mid-1;
			else
				low = mid+1;
		}
		return -1;
	}

	/* Sorts the array using qsort
		PARAMS:
			start - the index of the array to start the sort on
			indices - the number of indices of the array to sort
			compare - comparison function		 
		NOTES:	If indices = -1 the entire array will be sorted after the start index
		If you're sorting an array of pointers, the comparison function must have this signature:
		int func(MyObj* const* a, MyObj* const* b);*/
	void QSort(int start, int indices, int (*compare)(const _Type*, const _Type*))
	{
		ArrayAssert(start >= 0);
		if( indices < 0 )
			indices = m_Count - start;
		qsort(&m_Elements[start], indices, sizeof(_Type), (int (/*__cdecl*/ *)(const void*, const void*))compare);
	}

	inline _Type* GetElements() { return m_Elements; }
	inline const _Type* GetElements() const { return m_Elements; }

	/* Functions needed to make this container STL compliant: */
	iterator begin()				{ return &m_Elements[0]; }
	iterator end()					{ return &m_Elements[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	const_iterator begin() const	{ return &m_Elements[0]; }
	const_iterator end() const		{ return &m_Elements[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	reverse_iterator rbegin()		{ return reverse_iterator(end()); }
	reverse_iterator rend()			{ return reverse_iterator(begin());	}
	const_reverse_iterator rbegin() const	{ return const_reverse_iterator(end());	}
	const_reverse_iterator rend()	const	{ return const_reverse_iterator(begin()); }
	void clear()					{ erase(begin(), end()); }
	bool empty() const				{ return GetCount()==0; }
	iterator erase(iterator where);
	iterator erase(iterator first, iterator last);
	iterator insert(iterator where, const _Type &value);
	void insert(iterator where, size_type numNewElements, const _Type &value);
	template<class InputIterator> void insert(iterator where, InputIterator srcFirst, InputIterator srcLast);
	size_type max_size( ) const		{ return _MaxCount; }
	size_type size( ) const			{ return GetCount(); }
//	void swap(atRangeArray &that)	{ Swap(that); }							//swap is currently not implemented because it would only work with another array of the exact same size and then it would have to copy every single element.

protected:
	/* Internal copy helper. Only copies m_Count elements.
	PARAMS: that - Array to copy */
	void CopyFrom(const atFixedArray &that) {
		m_Count = that.m_Count;
		copy(that.m_Elements, that.m_Elements + m_Count, m_Elements);
	}
	
	/* 
	PARAMS:
		first		- first element to copy.
		last		- position one past the final element to copy
		destination	- First element of the destination range
	NOTES:
		Cannot be used to create new elements or insert elements into an empty
		container.*/
	void copy(const_iterator first, const_iterator last, iterator destination) {
		for (; first != last; destination++, first++)
			*destination = *first;
	}

	_Type *Insert(int insertionIndex, int numNewElements);

private:
	/* Storage for the array itself */
	_Type m_Elements[_MaxCount];
	/* Count of elements currently in the array */
	int m_Count;

	// Serialize method -- allow reallocation as needed
	friend datSerialize & operator<< <>( datSerialize &ser, atFixedArray &array );
};

template <class _Type,int _MaxCount>
typename atFixedArray<_Type,_MaxCount>::iterator atFixedArray<_Type,_MaxCount>::erase(iterator where)
{
	ArrayAssert(where != end());
	return erase(where, where+1);
}


/*
NOTES:
  * Doesn't actually delete anything or call any destructor. Merely shifts already 
	existing elements down over the ones that get 'erased' and adjusts the
	element count. However additional copies of elements will still hang around
	until overwritten with something else. This is in line with the overall behavior
	of atFixedArray.
*/
template <class _Type,int _MaxCount>
typename atFixedArray<_Type,_MaxCount>::iterator atFixedArray<_Type,_MaxCount>::erase(iterator first, iterator last)
{
	ArrayAssert(first>=begin() && (first<end() || (first==last && first==end())));
	ArrayAssert(last>=begin() && last<=end());
	copy(last, end(), first);
	m_Count= m_Count - static_cast<int>(last - first);
	return first;
}


/*
Inserts empty elements.
PARAMS:	insertionIndex	- the first new slot will have this index.
		numNewElements	- the number of slots to insert.
RETURNS: Address of the first empty slot.
NOTES: 
  * The new slots aren't really empty.  They will contain whatever
	array data happened to be there before.
*/
template <class _Type,int _MaxCount>
_Type *atFixedArray<_Type,_MaxCount>::Insert(int insertionIndex, int numNewElements)
{
	ArrayAssert(insertionIndex>=0 && insertionIndex <= m_Count);
	ArrayAssert(numNewElements > 0);
	ArrayAssert(m_Count + numNewElements <= _MaxCount);											//There has to be enough space for the new elements - atFixedArray can't grow.

	_Type							*insertionPtr= begin() + insertionIndex;

	// use of copy here is a bug, because copy works from bottom to top
	// instead of the other way around
	memmove(insertionPtr+numNewElements, insertionPtr, sizeof(_Type)*(m_Count-insertionIndex));
	//copy(insertionPtr, end(), insertionPtr+numNewElements);									//shift elements following insertion position numNewElements up

	m_Count = m_Count + numNewElements;
	return &m_Elements[insertionIndex];
}


template <class _Type,int _MaxCount> 
typename atFixedArray<_Type,_MaxCount>::iterator atFixedArray<_Type,_MaxCount>::insert(iterator where, const _Type &value)
{
	_Type			*newElement= Insert(ptrdiff_t_to_int(where - begin()), 1);
	*newElement= value;
	return newElement;
}

template <class _Type,int _MaxCount> 
void atFixedArray<_Type,_MaxCount>::insert(iterator where, size_type numNewElements, const _Type &value)
{
	if(numNewElements == 0)
		return;
	else
	{
		_Type			*newElements= Insert(ptrdiff_t_to_int(where - begin()), numNewElements);
		for(_Type *dest=newElements; dest<newElements+numNewElements; dest++)
			*dest= value;
		return;
	}
}

/*
NOTES:
	The current implementation does not work with (supposedly rare) pure input
	iterators due to its use of std::distance(). Input operators allow only one
	read access to each	element and std::distance() already uses up that quota.
	To properly	support pure input iterators iterator traits must be used to
	detect that	case and branch to a less efficient implementation.
*/
template <class _Type,int _MaxCount>
template<class InputIterator> void atFixedArray<_Type,_MaxCount>::insert(iterator where, InputIterator srcFirst, InputIterator srcLast)
{
	const int		numNewElements= ptrdiff_t_to_int(std::distance(srcFirst, srcLast));							//Cheap for random access iterators, expensive otherwise.
	ArrayAssert(numNewElements > 0);
	_Type				*newElements= Insert(ptrdiff_t_to_int(where - begin()), numNewElements);
	for(_Type *dest=newElements; dest<newElements+numNewElements; dest++, srcFirst++)
	{
		ArrayAssert(srcFirst!=srcLast);
		*dest= *srcFirst;
	}
	return;
}



//lint -etemplate(1512) -etemplate(114) 

template <class _Type,int _MaxCount> class atRangeArray;
template <class _Type,int _MaxCount> datSerialize& operator<<(datSerialize &ser, atRangeArray<_Type,_MaxCount> &);		

/*
PURPOSE:
	atRangeArray is the simplest canned array class of all.  It has no notion of current size; its
	only benefit over a normal C array is automatic range checking.
PARAMETERS:
	_Type - the type of the Array's element
	_MaxCount - the maximum # of instances that this array can hold
<FLAG Component>
*/
template <class _Type,int _MaxCount>
class atRangeArray {
public:
	STL_TYPEDEFS;

	/* Default constructor. */
	atRangeArray() { }
	explicit atRangeArray(const _Type& value) {
		for (u32 i = 0; i < _MaxCount; i++)
			m_Elements[i] = value;
	}

	static void Place(void *that,datResource &rsc) { ::new (that) atRangeArray(rsc); }

	explicit atRangeArray(datResource &UNUSED_PARAM(rsc)) {}

	/* We can't change the default rsc ctor without breaking a bunch of stuff. */
	atRangeArray(datResource &rsc,bool ARRAY_ASSERT_ONLY(flag)) {
		ArrayAssert(flag);
		for (int i=0; i<_MaxCount; i++)
			m_Elements[i].Place(&m_Elements[i],rsc);
	}



	/* Default destructor. */
	~atRangeArray() { }

	/* Access array, with range checking */
	__forceinline _Type& operator[](int index) { TrapGE((u32)index,(u32)_MaxCount); return m_Elements[index]; }

	/* Access array, with range checking */
	__forceinline const _Type& operator[](int index) const { TrapGE((u32)index,(u32)_MaxCount); return m_Elements[index]; }

	/* RETURNS: Number of items in the array. */
	int GetMaxCount() const { return _MaxCount; }

	/* Searches array for a match, working forward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is zero)
		RETURNS: Index of first match, or -1 if none found */
	int Find(const _Type& t, int start = 0) const {
		int i;
		ArrayAssert(start>=0);
		for (i=start; i<_MaxCount; ++i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}

	/* Searches array for a match, working backward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is -1, which means the end of the array)
		RETURNS: Index of first match, or -1 if none found */
	int ReverseFind(const _Type& t, int start = -1) const {
		int i;
		if ( start < 0 )
			start = _MaxCount - 1;
		ArrayAssert(start<_MaxCount);
		for (i=start; i>=0; --i)
			if (m_Elements[i] == t)
				return i;
		return -1;
	}

	inline _Type* GetElements() { return m_Elements; }
	inline const _Type* GetElements() const { return m_Elements; }

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	/* Functions needed to make this container STL compliant.
	   Note: various functions aren't implemented because atRangeArray
	   has no notion of current size. Others are always returning the
	   same value under the assumption the whole array is in use and
	   containing valid elements (which, technically, it does) */
	iterator begin()				{ return &m_Elements[0]; }
	iterator end()					{ return &m_Elements[_MaxCount]; }		//Since there is no notion of current size the array is assumed to always be in full use. Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	const_iterator begin() const	{ return &m_Elements[0]; }
	const_iterator end() const		{ return &m_Elements[_MaxCount]; }		//Since there is no notion of current size the array is assumed to always be in full use. Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	reverse_iterator rbegin()		{ return reverse_iterator(end()); }
	reverse_iterator rend()			{ return reverse_iterator(begin());	}
	const_reverse_iterator rbegin() const	{ return const_reverse_iterator(end());	}
	const_reverse_iterator rend()	const	{ return const_reverse_iterator(begin()); }
//	void clear()					{ erase(begin(), end()); }
	bool empty() const				{ return false; }
//	iterator erase(iterator where);
//	iterator erase(iterator first, iterator last);
//	iterator insert(iterator where, const _Type &value);
//	void insert(iterator where, size_type numNewElements, const _Type &value);
//	template<class InputIterator> void insert(iterator where, InputIterator srcFirst, InputIterator srcLast);
	size_type max_size( ) const		{ return _MaxCount; }
	size_type size( ) const			{ return max_size(); }					//Since there is no notion of current size the array is assumed to always be in full use.
//	void swap(atRangeArray &that)	{ Swap(that); }							//swap is currently not implemented because it would only work with another array of the exact same size and then it would have to copy every single element.

private:
	/* Storage for the array itself */
	_Type m_Elements[_MaxCount];

	// Serialize method -- allow reallocation as needed
	friend datSerialize & operator<< <>( datSerialize &ser, atRangeArray &array );
};

// defined down here since Doc-o-matic barfs if they're actually defined in the class
template <class _Type,int _Align,class _CounterType> 
datSerialize& operator<<(datSerialize &ser, atArray<_Type,_Align,_CounterType> &array) 
{
	_CounterType count = array.m_Count;
	ser << count;

	if (ser.IsRead() == true && array.m_Count < count)	
	{
		// Free old memory
		if (array.m_Elements)
			array.Destruct(array.m_Elements, array.m_Capacity);
		array.m_Elements = array.Construct(count);
		array.m_Count = array.m_Capacity = count;
	}

	for (_CounterType i = 0; i < count; ++i)
		ser << array.m_Elements[i];

	return ser;
}

template <class _Type,int _MaxCount> 
datSerialize& operator<<(datSerialize &ser, atRangeArray<_Type,_MaxCount> &array) 
{
	// Blindly dump the entire array, since we don't know how many elements to pass
	ser << datArray<_Type>(array.m_Elements, _MaxCount);
	return ser;
}

template <class _Type,int _MaxCount>
datSerialize & operator<< ( datSerialize &ser, atFixedArray<_Type,_MaxCount> &array ) {
	// Use _MaxCount for read and count for write (saves writing unused data)
	ser << array.m_Count << datArray<_Type>(array.m_Elements, (ser.IsRead() ? _MaxCount : array.m_Count) );
	return ser;
}

//
// PURPOSE:
//   atMultiRangeArray is a template for declaring multi-dimensional arrays of <c>atRangeArray</c>s.  Up to 8 dimensions are supported.
// PARAMETERS:
//   _Type - the type of the Array's element
//   _Dim1 - the first dimension
//   _Dim2 - the second dimension
//   _Dim3 - the third dimension
//   _Dim4 - the fourth dimension
//   _Dim5 - the fifth dimension
//   _Dim6 - the sixth dimension
//   _Dim7 - the seventh dimension
//   _Dim8 - the eighth dimension
// EXAMPLE:
// <CODE>
//  // this declares a 3 dimensional array (3 by 4 by 5)
//  atMultiRangeArray<int,3,4,5> intArray;
// </CODE>
//  which is equivalent to:
// <CODE>
//  // this declares a 3 dimensional array (3 by 4 by 5)
//  atRangeArray< atRangeArray< atRangeArray< int, 5>, 4>, 3> intArray;
// </CODE>
// <FLAG Component>
//
template <class _Type,int _Dim1,int _Dim2=0,int _Dim3=0,int _Dim4=0,int _Dim5=0,int _Dim6=0,int _Dim7=0,int _Dim8=0> class atMultiRangeArray : 
public atRangeArray< atMultiRangeArray<_Type, _Dim2, _Dim3, _Dim4, _Dim5, _Dim6, _Dim7, _Dim8>, _Dim1>
{
};

template <class _Type,int _Dim1> class atMultiRangeArray<_Type,_Dim1,0,0,0,0,0,0,0> : public atRangeArray<_Type,_Dim1>
{
};

//
// PURPOSE:
//   atMultiFixedArray is a template for declaring multi-dimensional arrays of <c>atFixedArray</c>s.  Up to 8 dimensions are supported.
// PARAMETERS:
//   _Type - the type of the Array's element
//   _Dim1 - the first dimension
//   _Dim2 - the second dimension
//   _Dim3 - the third dimension
//   _Dim4 - the fourth dimension
//   _Dim5 - the fifth dimension
//   _Dim6 - the sixth dimension
//   _Dim7 - the seventh dimension
//   _Dim8 - the eighth dimension
// EXAMPLE:
// <CODE>
//  // this declares a 3 dimensional array (3 by 4 by 5)
//  atMultiFixedArray<int,3,4,5> intArray;
// </CODE>
//  which is equivalent to:
// <CODE>
//  // this declares a 3 dimensional array (3 by 4 by 5)
//  atFixedArray< atFixedArray< atFixedArray< int, 5>, 4>, 3> intArray;
// </CODE>
// <FLAG Component>
//
template <class _Type,int _Dim1,int _Dim2=0,int _Dim3=0,int _Dim4=0,int _Dim5=0,int _Dim6=0,int _Dim7=0,int _Dim8=0> class atMultiFixedArray : 
public atFixedArray< atMultiFixedArray<_Type, _Dim2, _Dim3, _Dim4, _Dim5, _Dim6, _Dim7, _Dim8>, _Dim1>
{
};

template <class _Type,int _Dim1> class atMultiFixedArray<_Type,_Dim1,0,0,0,0,0,0,0> : public atFixedArray<_Type,_Dim1>
{
};

#if !__SPU // No reason these couldn't be on SPU, if someone wanted to fix the annoying errors

template <class _Type, int _Dimensions, int _Align=0, typename _CounterType=u16>
class atMultiArrayBase
{
public:
	void Reset()
	{
		for(int i = 0; i < _Dimensions; i++)
		{
			m_Lengths[i] = 0;
		}
		m_Storage.Reset();
	}

	_CounterType GetCount(int dimension) { return m_Lengths[dimension]; }

protected:
	void ResizeToLengths()
	{
		FastAssert(m_Storage.empty());
		m_Storage.Resize((u32)ComputeNumElements());
	}

	size_t ComputeNumElements() 
	{
		size_t elts = 1;
		for(int i = 0; i < _Dimensions; i++)
		{
			elts *= m_Lengths[i];
		}
		return elts;
	}

	size_t ComputeStride(int dimension)
	{
		size_t stride = 1;
		for(int i = dimension+1; i < _Dimensions; i++)
		{
			stride *= m_Lengths[i];
		}
		return stride;
	}

	atArray<_Type, _Align, u32> m_Storage;
	atRangeArray<_CounterType, _Dimensions> m_Lengths;
};

template <class _Type, int _Align=0, typename _CounterType=u16>
class atMultiArray2d : public atMultiArrayBase<_Type, 2, _Align, _CounterType>
{
	typedef atMultiArrayBase<_Type, 2, _Align, _CounterType> BaseType;
public:
	void Resize(_CounterType dim0, _CounterType dim1)
	{
		TrapNE(BaseType::m_Lengths[0], 0);
		TrapNE(BaseType::m_Lengths[1], 0);
		BaseType::m_Lengths[0] = dim0;
		BaseType::m_Lengths[1] = dim1;
		BaseType::ResizeToLengths();
	}

	_Type& operator() (size_t dim0, size_t dim1) 
	{ 
		TrapGE(dim0, (size_t)BaseType::m_Lengths[0]);
		TrapGE(dim1, (size_t)BaseType::m_Lengths[1]);
		return BaseType::m_Storage[(u32)(dim0 * BaseType::ComputeStride(0) + dim1 * BaseType::ComputeStride(1))];
	}

	const _Type& operator() (size_t dim0, size_t dim1) const 
	{ 
		TrapGE(dim0, (size_t)BaseType::m_Lengths[0]);
		TrapGE(dim1, (size_t)BaseType::m_Lengths[1]);
		return BaseType::m_Storage[(u32)(dim0 * BaseType::ComputeStride(0) + dim1 * BaseType::ComputeStride(1))];
	}

};


template <class _Type, int _Align=0, typename _CounterType=u16>
class atMultiArray3d : public atMultiArrayBase<_Type, 3, _Align, _CounterType>
{
	typedef atMultiArrayBase<_Type, 3, _Align, _CounterType> BaseType;
public:
	void Resize(_CounterType dim0, _CounterType dim1, _CounterType dim2)
	{
		TrapNE(BaseType::m_Lengths[0], 0);
		TrapNE(BaseType::m_Lengths[1], 0);
		TrapNE(BaseType::m_Lengths[2], 0);
		BaseType::m_Lengths[0] = dim0;
		BaseType::m_Lengths[1] = dim1;
		BaseType::m_Lengths[2] = dim2;
		BaseType::ResizeToLengths();
	}


	_Type& operator() (size_t dim0, size_t dim1, size_t dim2) 
	{ 
		TrapGE(dim0, (size_t)BaseType::m_Lengths[0]);
		TrapGE(dim1, (size_t)BaseType::m_Lengths[1]);
		TrapGE(dim2, (size_t)BaseType::m_Lengths[2]);
		return BaseType::m_Storage[(u32)(dim0 * BaseType::ComputeStride(0) + dim1 * BaseType::ComputeStride(1) + dim2 * BaseType::ComputeStride(2))];
	}

	const _Type& operator() (size_t dim0, size_t dim1, size_t dim2) const
	{ 
		TrapGE(dim0, (size_t)BaseType::m_Lengths[0]);
		TrapGE(dim1, (size_t)BaseType::m_Lengths[1]);
		TrapGE(dim2, (size_t)BaseType::m_Lengths[2]);
		return BaseType::m_Storage[(u32)(dim0 * BaseType::ComputeStride(0) + dim1 * BaseType::ComputeStride(1) + dim2 * BaseType::ComputeStride(2))];
	}
};

#endif

//PURPOSE
//  Implements an array of T where each T is embedded in another object.
//  The size of each object is the span.
//EXAMPLE
//
//  An array of ints, each int embedded in a Foo object
//  struct Foo
//  {
//      int I;
//      float F;
//  };
//
//  Foo foos[10];
//  datSpanArray<int> a;
//  a.Init(&foos[0].I, sizeof(Foo));
//
template<typename T>
class atSpanArray
{
public:

    atSpanArray()
        : m_Base(NULL)
        , m_ItemSpan(0)
		, m_Count(0)
    {
    }

    atSpanArray(T* base, const unsigned itemSpan, const unsigned count)
    {
        Assert(itemSpan >= sizeof(T));
        Init(base, itemSpan, count);
    }

	void Clear()
	{
		m_Base = NULL;
		m_ItemSpan = 0;
		m_Count = 0;
	}

    void Init(T* base, const unsigned itemSpan, const unsigned count)
    {
        Assert(itemSpan >= sizeof(T));
        m_Base = (u8*)base;
        m_ItemSpan = itemSpan;
		m_Count = count;
    }

	unsigned GetCount()
	{
		return m_Count;
	}

    T& operator[](const unsigned index)
    {
        FastAssert(m_Base);
		TrapGE(index,(u32)m_Count);
        return (T&)m_Base[index*m_ItemSpan];
    }

    const T& operator[](const unsigned index) const
    {
        FastAssert(m_Base);
		TrapGE(index,(u32)m_Count);
		return (const T&)m_Base[index*m_ItemSpan];
    }

	bool IsEmpty() const
	{
		return m_Count == 0;
	}

private:

    //No copying
    atSpanArray(const atSpanArray&);
    atSpanArray& operator=(const atSpanArray&);

    u8* m_Base;
    unsigned m_ItemSpan;
	unsigned m_Count;
};

#undef STL_TYPEDEFS

}	// namespace rage

#endif	// !__SPU

#endif

