//
// atl/vector.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_VECTOR_H
#define ATL_VECTOR_H

#include "align.h"
#include "array.h"

#include <algorithm>

namespace rage {

template <class _Type,int _Align,class _CounterType> class atVector;
template <class _Type,int _Align,class _CounterType> datSerialize& operator<<(datSerialize &ser, atVector<_Type,_Align,_CounterType> &);


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

/*
PURPOSE:
	atVector is a lightweight vector-of-objects container class. Similar to std::vector it has built in range checking
	and the ability to change its capacity at runtime. Random access is O(1)
PARAMETERS:
	_Type - the type of the vector's element
	_Align - the minimum alignment of the vector element allocation
	_CounterType - the type of the vector counter (default unsigned short)
NOTES:
	Be careful in __TOOL builds to NOT use a _Type that requires alignement!
<FLAG Component>
*/

template <class _Type,int _Align = 0,class _CounterType = unsigned short>
class atVector {
public:
	static const u32 _CounterMax = (_CounterType)-1;
	CompileTimeAssert(sizeof(_CounterType) >= sizeof(unsigned short) && sizeof(_CounterMax) >= sizeof(unsigned short));

	STL_TYPEDEFS;

private:

	// Internal helper function, allocates raw storage for vector but does NOT initialize it
	_Type* Allocate(int count)
	{
		RAGE_TRACK(atVector);
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
		return (_Type*)result;
	}

	// Internal helper function, frees raw storage for vector
	void Free(_Type* storage)
	{
		delete [] (char*)storage;
	}

	void Construct(iterator start, iterator end)
	{
		ATL_ARRAY_FUNC();
		for (; start != end; ++start)
			::new (start) _Type;
	}

	template<typename _Arg1>
	void Construct(iterator start, iterator end, _Arg1 arg)
	{
		ATL_ARRAY_FUNC();
		for (; start != end; ++start)
			::new (start) _Type(arg);
	}

	// Internal helper function, runs default constructor on untyped vector
	void CopyConstruct(iterator destStart, iterator destEnd, const_iterator srcStart)
	{
		ATL_ARRAY_FUNC();
		for (; destStart != destEnd; ++destStart, ++srcStart)
		{
			::new (destStart) _Type(*srcStart);
		}
	}

	void Assign(iterator destStart, iterator destEnd, const_iterator srcStart)
	{
		ATL_ARRAY_FUNC();
		for(; destStart != destEnd; ++destStart, ++srcStart)
		{
			*destStart = *srcStart;
		}
	}

	// Internal helper function, runs destructor on specified element(s)
	void Destruct(iterator start, iterator end)
	{
		for(; start != end; ++start)
		{
			start->~_Type();
		}
	}

// have to do this as a macro to stop a spurious warning with MS compilers
#define DestructOne(item) (item)->~_Type();

	// ONLY used to increase capacity and preserve the existing elements
	void GrowCapacity(u32 count) {
		ArrayAssert(count<_CounterMax);
		FastAssert(count + m_Count > m_Capacity);

		m_Capacity = _CounterType(count);
		_Type *e = Allocate(count);
		if (m_Elements)
		{
			CopyConstruct(e, e + m_Count, m_Elements);
			Destruct(m_Elements, m_Elements + m_Count);
			Free(m_Elements);
		}
		m_Elements = e;
	}

	void Initialize(_CounterType count, iterator srcElements)
	{
		m_Elements = Allocate(count);
		CopyConstruct(m_Elements, m_Elements + count, srcElements);
		m_Capacity = count;
		m_Count = count;
	}
		

public:

	/* Default constructor.  vector is left totally empty */
	atVector() : m_Elements(0), m_Count(0), m_Capacity(0) { }

	/* Initialize vector allocation to specified size
		PARAMS: count - Number of entries to preconstruct
		PARAMS: capacit - Number of entries to preallocate */
	atVector(u32 count,u32 capacity) : m_Count((_CounterType) count), m_Capacity((_CounterType) capacity)
	{	ArrayAssert(count <= capacity);
		ArrayAssert(capacity<_CounterMax);
		m_Elements = capacity ? Allocate(capacity) : NULL;   
		Construct(m_Elements, m_Elements + count);
	}


	enum eInitialzeContentsInitializer {
		PRESERVE_CONTENTS,
		INIT_CONTENTS
	};
	enum ePlaceNoneInitializer {PLACE_NONE};
	enum ePlaceContentsInitializer {PLACE_CONTENTS};
	enum ePlaceNestedContents {PLACE_NESTED_CONTENTS };


	/* Intended for classes that need resource compiler support */
	explicit atVector(eInitialzeContentsInitializer init) {if (init == INIT_CONTENTS) {m_Elements=0; m_Count=0; m_Capacity=0;}}

	// Resource constructor for when you don't want to Place() the vector elements
	explicit atVector(datResource &rsc, ePlaceNoneInitializer) { rsc.PointerFixup(m_Elements); }

	// Resource constructor for when you want to Place() all the children
	atVector(datResource &rsc, ePlaceContentsInitializer) {
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			m_Elements[i].Place(&m_Elements[i],rsc);
	}

	/* Resourcing support for 2d atVectors. */
	atVector(datResource &rsc,ePlaceNestedContents) 
	{
		rsc.PointerFixup(m_Elements);
		for (int i=0; i<m_Count; i++)
			m_Elements[i].Place(&m_Elements[i], rsc, PLACE_CONTENTS);
	}

	/* Default destructor */
	~atVector() { Reset(); }

	/* Construct an vector from another vector.  Original vector is unchanged.
		PARAMS: that - vector to copy initial data from */
	explicit atVector(const atVector& that) : m_Elements(0), m_Count(0), m_Capacity(0)  { 
		if (that.m_Count)
		{
			Initialize(that.m_Count, that.m_Elements);
		}
	}

	/* Copy an vector from another vector.  Other vector is unchanged, this vector has its contents replaced.
		PARAMS: that - vector to copy data from */
	atVector& operator=(const atVector& that) { 
		if (this != &that) { 
			_CounterType newCount = that.m_Count;
			_CounterType oldCount = m_Count;
			if (newCount > m_Capacity)
			{
				Reset();	  
				Initialize(newCount, that.m_Elements);
			}
			else
			{
				if (m_Count < newCount) { 
					// need to grow
					Assign(m_Elements, m_Elements + oldCount, that.m_Elements);
					CopyConstruct(m_Elements + oldCount, m_Elements + newCount, that.m_Elements + oldCount);
				}
				else
				{
					// need to shrink
					Assign(m_Elements, m_Elements + newCount, that.m_Elements);
					Destruct(m_Elements + newCount, m_Elements + oldCount);
				}
				m_Count = newCount;
			}
		} 
		return *this; 
	}

	/* Assume ownership of an vector without doing a copy, destroying the other vector in the process.
		PARAMS: that - vector to assume control of.  The vector is left empty.
		NOTES: Useful when doing batch delete operations */
	void Assume(atVector& that) {
		if (this != &that) { 
			Reset(); 
			Swap(that);
		}
	}

	/* Access vector, with range checking */
	__forceinline _Type& operator[](u32 index) { TrapGE((_CounterType)index,m_Count); return m_Elements[index]; }

	/* Access vector, with range checking */
	__forceinline const _Type& operator[](u32 index) const { TrapGE((_CounterType)index,m_Count); return m_Elements[index]; }

	/* RETURNS: Number of valid items in the vector.  Not necessarily the same as the number of entries allocated */
	int GetCount() const { return m_Count; }

	// PURPOSE: Pointer to the counter
	_CounterType* GetCountPointer() { return &m_Count; }

	/* RETURNS: Number of items preallocated in the vector.  Not necessarily the same as the number of items in the vector*/
	// int GetMaxCount() const { return m_Capacity; } // Current Capacity

	/* RETURNS: Number of items preallocated in the vector.  Not necessarily the same as the number of items in the vector*/
	int GetCapacity() const { return m_Capacity; }

	/* Presizes an vector to specified capacity.  vector must be empty first, call Reset first if necessary. */
	void Reserve(u32 capacity) {
		// If m_Capacity is zero, count must already be zero
		ArrayAssert(!m_Capacity);
		ArrayAssert(capacity<_CounterMax);
		m_Elements = capacity ? Allocate(capacity) : NULL;
		m_Capacity = (_CounterType) capacity; 
	}

	/* Resizes the vector.  Must be within current capacity, except in the special case
		where the vector was already empty. */
	void Resize(u32 count) {
		ArrayAssert(count<_CounterMax);
		if (m_Capacity) {
			TrapGT(count, (u32) m_Capacity);
		}
		else
		{
			Reserve(count);
		}
		if (count < m_Count) {
			Destruct(m_Elements + count, m_Elements + m_Count);
		}
		else if (count > m_Count)
		{
			Construct(m_Elements + m_Count, m_Elements + count);
		}
		m_Count = (_CounterType) count;
	}

	/* PURPOSE: Initialize the storage and the size of the array to a certain value.
	 * NOTE that it will not destroy or free the previous elements pointer, if there was one,
	 * nor will it perform any initialization on the new data it is given.
	 *
	 * The caller is entirely responsible for cleaning up the previous storage (if present),
	 * and initializing the new storage.
	 */
	void SetElementsAndCount(_Type *elements, int count, int capacity) {
		m_Elements = elements;

		ArrayAssert((u32) capacity >= (u32) count);
		ArrayAssert((u32)capacity <= _CounterMax); 

		m_Count = (_CounterType) count;
		m_Capacity = (_CounterType) capacity;
	}

	/* Resizes the vector.  Must be within current capacity, except in the special case
	where the vector was already empty. */
	template<typename _Arg1>
	void Resize(u32 count, _Arg1 arg1) {
		ArrayAssert(count<_CounterMax);
		if (m_Capacity) {
			TrapGT(count, (u32) m_Capacity);
		}
		else
		{
			Reserve(count);
		}
		if (count < m_Count) {
			Destruct(m_Elements + count, m_Elements + m_Count);
		}
		else if (count > m_Count)
		{
			Construct(m_Elements + m_Count, m_Elements + count, arg1);
		}
		m_Count = (_CounterType) count;
	}

	/* Resizes the vector.  May exceed current capacity, in which case the capacity will
	   grow to the desired size. */
	void ResizeGrow(u32 count)
	{
		if (count <= m_Capacity)
		{
			Resize(count);
		}
		else
		{
			GrowCapacity(count);
			Construct(m_Elements + m_Count, m_Elements + count);
			m_Count = (_CounterType) count;
		}
	}

	template<typename _Arg1> void ResizeGrow(u32 count, const _Arg1 &arg1)
	{
		if (count <= m_Capacity)
		{
			Resize(count, arg1);
		}
		else
		{
			GrowCapacity(count);
			Construct(m_Elements + m_Count, m_Elements + count, arg1);
			m_Count = (_CounterType) count;
		}
	}


	/* Clears vector and reclaims its storage */
	void Reset() {
		if( m_Capacity ) {
			Destruct(m_Elements, m_Elements + m_Count); 
			Free(m_Elements);
			m_Elements = NULL;
			m_Count = 0;
			m_Capacity = 0;
		}
	}

	/* RETURNS: Reference to a newly appended vector element. */
	_Type& Append() { 
		_CounterType oldCount = m_Count;
		TrapGE(oldCount,m_Capacity);
		m_Count++;
		::new (&m_Elements[oldCount])_Type;
		return m_Elements[oldCount]; //lint !e662
	}

	template<typename _Arg1> _Type& Append(_Arg1 arg1) {
		_CounterType oldCount = m_Count;
		TrapGE(oldCount,m_Capacity);
		m_Count++;
		::new (&m_Elements[oldCount])_Type(arg1);
		return m_Elements[oldCount]; //lint !e662
	}

	/* RETURNS: Reference to a newly appended vector element, with resize. 
	   PARAMS: allocStep - resize step */
	_Type& Grow(u32 allocStep = 16) {
		if (m_Count == m_Capacity) {
			GrowCapacity(m_Count + allocStep);
		}
		return Append();
	}

	/* PURPOSE: Allocate a new element via Grow() and construct it using
	   a constructor that takes one argument.
     */
	template<typename _Arg1>
	_Type& GrowAndConstruct1(_Arg1 arg1, _CounterType allocStep = 16) {
		if (m_Count == m_Capacity) {
			GrowCapacity(m_Count + allocStep);
		}
		return Append(arg1);
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
		// NOTE - this has a known inefficiency. If count > m_Count then the new elements that we add to the vector
		// get Constructed then they get Assigned. More efficient would be to just copy-construct them
		Resize(count);
		Assign(m_Elements, m_Elements + count, ptrArray);
	}

	/* Inserts an empty slot into an existing vector
		PARAMS: beforeIndex - vector index to insert an empty slot before
		RETURNS: Reference to the empty slot
		NOTES: The slot is not really empty.  It will contain whatever
			vector data happened to be there before. */
	_Type& Insert(int beforeIndex) {
		ArrayAssert(m_Count != m_Capacity);
		ArrayAssert(beforeIndex>=0 && beforeIndex <= m_Count);
		Append();
		for (int i=m_Count-1; i>beforeIndex; i--)
			m_Elements[i] = m_Elements[i-1];
		return m_Elements[beforeIndex]; //lint !e797
	}

	/* Deletes vector element at specified index
		PARAMS: index - Index of vector element to delete */
	void Delete(int index) {
		_CounterType count = m_Count;
		ArrayAssert(index>=0 && _CounterType(index) < count);
		for (_CounterType i=_CounterType(index); i<count-1; i++)
			m_Elements[i] = m_Elements[i+1];
		--m_Count;
		DestructOne(m_Elements + m_Count);
	}

	/* Deletes vector element at specified index
		PARAMS: index - Index of vector element to delete 
		NOTE: This is ONLY useful if the order of vector elements is not important! 
				Also, this means that the element in index may still be valid */
	void DeleteFast(int index) {
		int count = m_Count;
		ArrayAssert(index>=0 && _CounterType(index) < count);
		--count;
		m_Elements[index] = m_Elements[count];
		DestructOne(m_Elements + count);
		m_Count = _CounterType(count);
	}

	/* Deletes all entries in an vector matching parameter
		PARAMS: match - Item to check against
		NOTES: vector element's class must define operator==. */
	void DeleteMatches(const _Type& match) {
		iterator newEnd = std::remove(begin(), end(), match);
		Destruct(newEnd, end());
		m_Count = newEnd - m_Elements;
	}

	/* Treat vector like a stack and push element to top
		PARAMS: t - Item to append to end of vector */
	void Push(const _Type& t) {
		Append() = t; 
	}

	/* Treat vector like a stack and push element to top, except grow the vector if needed
		PARAMS: 
			t - Item to append to end of vector 
			allocStep - resize step
	*/
	void PushAndGrow(const _Type& t,_CounterType allocStep = 16) {
		Grow(allocStep) = t; 
	}

	/* Treat vector like a stack and return reference to topmost element
		RETURNS: Reference to top of stack
		NOTES: Throws an assert if the vector is empty */
	_Type& Top() {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1]; //lint !e662 possible creation of out-of-bounds pointer		
	}
	const _Type& Top() const {
		TrapZ(m_Count); 
		return m_Elements[m_Count-1]; //lint !e662 possible creation of out-of-bounds pointer		
	}

	/* Treat vector like a stack and pops last element off of vector
		NOTES: Throws an assert if the vector is empty */
	void Pop() { 
		_CounterType oldCount = m_Count;
		TrapZ(oldCount); 
		DestructOne(m_Elements + oldCount - 1);
		m_Count--;
	}

	/* Searches vector for a match, working forward
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

	/* Searches vector for a match, working backward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is -1, which means the end of the vector)
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

	/* Searches vector for a match, using a log N binary search
	PARAMS:
	t - Element to search for
	RETURNS: Index of first match, or -1 if none found.
	NOTES:	vector must be in sorted order and define < and == operators against _Type */
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

	void Swap(atVector &that)
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

	inline void SetElements(_Type* elements) { m_Elements = elements; }
	inline _Type*& GetElements() { return m_Elements; }
	inline const _Type* GetElements() const { return m_Elements; }

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
	void DatSwapAllButContents(); // swaps the atVector data but NOT the contents of the vector
#endif

	/* Functions needed to make this container STL compliant: */
	iterator begin()				{ return m_Elements; }
	iterator end()					{ return m_Elements + m_Count; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	const_iterator begin() const	{ return m_Elements; }
	const_iterator end() const		{ return m_Elements + m_Count; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	reverse_iterator rbegin()		{ return reverse_iterator(end()); }
	reverse_iterator rend()			{ return reverse_iterator(begin());	}
	const_reverse_iterator rbegin() const	{ return const_reverse_iterator(end());	}
	const_reverse_iterator rend()	const	{ return const_reverse_iterator(begin()); }
	void clear()					{ Reset(); }
	bool empty() const				{ return GetCount()==0; }
	iterator erase(iterator where);
	iterator erase(iterator first, iterator last);
	iterator insert(iterator where, const _Type &value);
	void insert(iterator where, size_type numNewElements, const _Type &value);
	template<class InputIterator> void insert(iterator where, InputIterator srcFirst, InputIterator srcLast);
	size_type max_size( ) const		{ return _CounterMax-1; }
	size_type size( ) const			{ return GetCount(); }
	void swap(atVector &that)		{ Swap(that); }
	_Type& front()					{ FastAssert(m_Count > 0); return m_Elements[0]; }
	const _Type& front() const		{ FastAssert(m_Count > 0); return m_Elements[0]; }
	_Type& back()					{ FastAssert( m_Count > 0 ); return m_Elements[m_Count - 1];}
	const _Type& back() const		{ FastAssert( m_Count > 0 ); return m_Elements[m_Count - 1];}
	void resize(size_type sz, const _Type &c = _Type()) { ResizeGrow(sz, c); }
	size_type capacity( ) const		{ return GetCapacity(); }
	void reserve(size_type n )		{ if (n > GetCapacity()) GrowCapacity(n); }
	_Type& at(size_type n)			{ return m_Elements[n]; }
	const _Type& at(size_type n) const { return m_Elements[n]; }
	
	template <class InputIterator>
	void assign(InputIterator first, InputIterator last) 
	{
		// Note that this is a lot slower than it could be. Because we're using InputIterators instead of random access iterators
		// we can't get the distance between the iterators, so can't pre-size the vector.
		clear();
		for(; first != last; ++first)
		{
			GrowAndConstruct1(*first);
		}
	}

	void assign(size_type n, const _Type& u)
	{
		// NOTE: This could be more efficient. Could use assignment op for first elements, copy ctor for rest instead of
		// destroying all existing ones and copying over them. see also CopyFrom()
		clear();
		ResizeGrow(n, u);
	}

	void push_back(const _Type& x)	{ PushAndGrow(x); }
	void pop_back(const _Type& x)	{ Pop(); }

	template <class _T, unsigned int _A, class _CT> 
	friend void swap(atVector<_T,_A,_CT> &left,  atVector<_T,_A,_CT> &right);

protected:

	_Type *Insert(int insertionIndex, int numNewElements);

	/* The raw array of elements */
	_Type* m_Elements;
	_CounterType m_Count, /* Number of valid items in the array */
		m_Capacity;		/* Total number of entries allocated in the array.  Always greater than or equal to m_Count. */

	// Serialize method -- allow reallocation as needed
	friend datSerialize& operator<< <>(datSerialize &ser, atVector<_Type,_Align,_CounterType> &vector);

#undef DestructOne
};


template <class _Type,int _Align,class _CounterType>
typename atVector<_Type,_Align,_CounterType>::iterator atVector<_Type,_Align,_CounterType>::erase(iterator where)
{
	ArrayAssert(where != end());
	Delete(where - begin());
}


template <class _Type,int _Align,class _CounterType>
typename atVector<_Type,_Align,_CounterType>::iterator atVector<_Type,_Align,_CounterType>::erase(iterator first, iterator last)
{
	ArrayAssert(first>=begin() && (first<end() || (first==last && first==end())));
	ArrayAssert(last>=begin() && last<=end());
	_CounterType eraseCount = last - first;
	CopyConstruct(last, end(), first);
	Destruct(first + eraseCount, last);
	m_Count= m_Count - eraseCount;
	return first;
}


/*
Inserts empty elements.
PARAMS:	insertionIndex	- the first new slot will have this index.
		numNewElements	- the number of slots to insert.
RETURNS: Address of the first empty slot.
NOTES: 
  * May grow the vector and allocate memory.
  * May leave old data at the locations that were inserted
*/
template <class _Type,int _Align,class _CounterType>
_Type *atVector<_Type,_Align,_CounterType>::Insert(int insertionIndex, int numNewElements)
{
	ArrayAssert(insertionIndex>=0 && insertionIndex <= m_Count);
	ArrayAssert(numNewElements > 0);
	_Type							*insertionPtr= begin() + insertionIndex;
	if(m_Count + numNewElements > m_Capacity)													//Does the vector have to grow?
	{
		const _CounterType		newCapacity= _CounterType(m_Count + numNewElements);
		_Type* newElements = Allocate(newCapacity);
		_Type* oldElements = m_Elements;

		CopyConstruct(newElements, newElements + insertionIndex, oldElements);
		Construct(newElements + insertionIndex, newElements + insertionIndex + numNewElements);
		CopyConstruct(newElements + insertionIndex + numNewElements, newElements + newCapacity, oldElements + insertionIndex);

		Destruct(oldElements, oldElements + m_Count);
		Free(oldElements);

		m_Elements = newElements;
		m_Capacity = newCapacity;
		m_Count = newCapacity;
	}
	else
	{
		// Note: Could do this with copy construction instead of construct + op=
		int newCount = m_Count + numNewElements;
		Construct(m_Elements + m_Count, m_Elements + newCount);

		for(int i = newCount-1; i >= insertionIndex; i--)
		{
			m_Elements[i] = m_Elements[i - numNewElements];
		}		  

		m_Count = newCount;
	}

	return &m_Elements[insertionIndex];
}


template <class _Type,int _Align,class _CounterType> 
typename atVector<_Type,_Align,_CounterType>::iterator atVector<_Type,_Align,_CounterType>::insert(iterator where, const _Type &value)
{
	_Type			*newElement= Insert(where - begin(), 1, false);
	*newElement= value;
	return newElement;
}

template <class _Type,int _Align,class _CounterType> 
void atVector<_Type,_Align,_CounterType>::insert(iterator where, size_type numNewElements, const _Type &value)
{
	if(numNewElements == 0)
		return;
	else
	{
		_Type			*newElements= Insert((int)(where - begin()), (int)numNewElements);
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
template<class InputIterator> void atVector<_Type,_Align,_CounterType>::insert(iterator where, InputIterator srcFirst, InputIterator srcLast)
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
void swap(atVector<_Type,_Align,_CounterType> &left,  atVector<_Type,_Align,_CounterType> &right)
{
	left.swap(right);
}

template <class _Type,int _MaxCount> class atFixedVector;
template <class _Type,int _MaxCount> datSerialize& operator<<(datSerialize &ser, atFixedVector<_Type,_MaxCount> &);		

/*
PURPOSE:
	atFixedVector is nearly identical to atVector except that it's parameterized by max size and contains
	the vector directly instead of requiring an extra pointer access.  Its main advantages over a normal
	C array are range checking on parameters, a built-in element count, and construction/destruction of elements as needed
PARAMETERS:
	_Type - the type of the vector's element
	_MaxCount - the maximum # of instances that this vector can hold
<FLAG Component>
*/
template <class _Type,int _MaxCount>
class atFixedVector {
public:
	STL_TYPEDEFS;

	void Construct(iterator start, iterator end)
	{
		ATL_ARRAY_FUNC();
		for (; start != end; ++start)
			::new (start) _Type;
	}

	template<typename _Arg1>
	void Construct(iterator start, iterator end, _Arg1 arg)
	{
		ATL_ARRAY_FUNC();
		for (; start != end; ++start)
			::new (start) _Type(arg);
	}

	// Internal helper function, runs default constructor on untyped vector
	void CopyConstruct(iterator destStart, iterator destEnd, const_iterator srcStart)
	{
		ATL_ARRAY_FUNC();
		for (; destStart != destEnd; ++destStart, ++srcStart)
		{
			::new (destStart) _Type(*srcStart);
		}
	}

	void Assign(iterator destStart, iterator destEnd, const_iterator srcStart)
	{
		ATL_ARRAY_FUNC();
		for(; destStart != destEnd; ++destStart, ++srcStart)
		{
			*destStart = *srcStart;
		}
	}

	// Internal helper function, runs destructor on specified element(s)
	void Destruct(iterator start, iterator end)
	{
		for(; start != end; ++start)
		{
			start->~_Type();
		}
	}

// Have to do this as a macro to avoid a spurious MS compiler warning
#define DestructOne(item) (item)->~_Type()

public:

	/* Default constructor.  vector is left totally empty */
	atFixedVector() : m_Count(0) { }

	explicit atFixedVector(int count) : m_Count(count) { 
		ArrayAssert(count >= 0 && count <= _MaxCount); 
		Construct(GetElements(), GetElements() + count);
	}

	/* Construct an vector from another vector.  Original vector is unchanged.
	PARAMS: that - vector to copy initial data from */
	explicit atFixedVector(const atFixedVector& that) { 
		m_Count = that.m_Count;
		CopyConstruct(GetElements(), GetElements() + m_Count, that.GetElements());
	}

	/* Copy an vector from another vector.  Other vector is unchanged, this vector has its contents replaced.
	PARAMS: that - vector to copy data from */
	atFixedVector& operator=(const atFixedVector& that) { 
		if (this != &that) { 
			int newCount = that.m_Count;
			int oldCount = m_Count;
			if (oldCount < newCount)
			{
				// need to grow
				Assign(GetElements(), GetElements() + oldCount, that.GetElements());
				CopyConstruct(GetElements() + oldCount, GetElements() + newCount, that.GetElements() + oldCount);
			}
			else
			{
				// need to shrink
				Assign(GetElements(), GetElements() + newCount, that.GetElements());
				Destruct(GetElements() + newCount, GetElements() + oldCount);
			}
			m_Count = newCount;
		}
		return *this; 
	}

	enum ePlaceNoneInitializer {PLACE_NONE};
	enum ePlaceContentsInitializer {PLACE_CONTENTS};

	atFixedVector(datResource &rsc, ePlaceNoneInitializer) {
	}

	atFixedVector(datResource &rsc, ePlaceContentsInitializer) {
		Construct(GetElements(), GetElements() + m_Count, rsc);
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	/* Default destructor. */
	~atFixedVector() { Reset(); }

	/* Access vector, with range checking */
	__forceinline _Type& operator[](int index) { TrapGE((u32)index,(u32)m_Count); return GetElements()[index]; }

	/* Access vector, with range checking */
	__forceinline const _Type& operator[](int index) const { TrapGE((u32)index,(u32)m_Count); return GetElements()[index]; }

	/* RETURNS: Number of valid items in the vector.  Not necessarily the same as the number of entries allocated */
	int GetCount() const { return m_Count; }

	/* PARAMS: ct - New element count for vector; must be between zero and _MaxCount */
	void SetCount(int newCount) { 
		ArrayAssert((u32)newCount<=(u32)_MaxCount); 
		int oldCount = m_Count;
		if (newCount < oldCount)
		{
			Destruct(GetElements() + newCount, GetElements() + oldCount);
		}
		else
		{
			Construct(GetElements() + oldCount, GetElements() + newCount);
		}
		m_Count=newCount; 
	}

	/* PARAMS: ct - New element count for vector; must be between zero and _MaxCount */
	template<typename _Arg1>
	void SetCount(int newCount, _Arg1 arg1) { 
		ArrayAssert((u32)newCount<=(u32)_MaxCount); 
		int oldCount = m_Count;
		if (newCount < oldCount)
		{
			Destruct(GetElements() + newCount, GetElements() + oldCount);
		}
		else
		{
			Construct(GetElements() + oldCount, GetElements() + newCount, arg1);
		}
		m_Count=newCount; 
	}

	/* RETURNS: Number of items preallocated in the vector.  Not necessarily the same as the number of items in the vector */
	int GetMaxCount() const { return _MaxCount; }

	/* RETURNS: True if the vector is full. */
	bool IsFull() const { return m_Count == _MaxCount; }

	/* Clears vector */
	void Reset() {
		Destruct(GetElements(), GetElements() + m_Count);
		m_Count = 0; 
	}

	/* Sets size of vector */
	void Resize(int count) {
		SetCount(count);
	}

	/* Does nothing, here to match other vector class APIs */
	void Reallocate(const int ARRAY_ASSERT_ONLY(newCount))	{
		ArrayAssert(newCount <= _MaxCount);
	}

	/* RETURNS: Reference to a newly appended vector element. Asserts if vector is full. */
	_Type& Append() { 
		TrapGE(m_Count,_MaxCount);
		int count = m_Count;
		::new (GetElements() + count) _Type;
		m_Count++;
		return GetElements()[count];
	}

	/* Inserts an element into an vector
		PARAMS: beforeIndex - vector index to insert a new slot before
		RETURNS: Reference to the inserted slot
		NOTES:  vec[beforeIndex] will be unchanged before and after the call. Consider using the STL-style insert() if this isn't what you want*/
	_Type& Insert(int beforeIndex) {
		ArrayAssert(m_Count != _MaxCount);
		ArrayAssert(beforeIndex>=0 && beforeIndex <= m_Count);
		Append();
		for (int i=m_Count-1; i>beforeIndex; i--)
			GetElements()[i] = GetElements()[i-1];
		return GetElements()[beforeIndex];
	}

	/* Deletes vector element at specified index
		PARAMS: index - Index of vector element to delete */
	void Delete(int index) {
		int oldCount = m_Count;
		ArrayAssert(index>=0 && index < oldCount);
		for (int i=index; i<oldCount-1; i++)
			GetElements()[i] = GetElements()[i+1];
		DestructOne(GetElements() + oldCount - 1);
		--m_Count;
	}

	/* Deletes vector element at specified index
		PARAMS: index - Index of vector element to delete 
		NOTE: This is ONLY useful if the order of vector elements is not important! */
	void DeleteFast(int index) {
		ArrayAssert(index>=0 && index < m_Count);
		int newCount = m_Count - 1;
		GetElements()[index] = GetElements()[newCount];
		DestructOne(GetElements() + newCount);
		m_Count = newCount;
	}

	/* Treat vector like a stack and push element to top
		PARAMS: t - Item to append to end of vector */
	void Push(const _Type& t) {
		Append() = t; 
	}

	/* Treat vector like a stack and return reference to topmost element
		RETURNS: Reference to top of stack
		NOTES: Throws an assert if the vector is empty */
	_Type& Top() {
		TrapZ(m_Count); 
		return GetElements()[m_Count-1];		
	}
	const _Type& Top() const {
		TrapZ(m_Count); 
		return GetElements()[m_Count-1];		
	}

	/* Treat vector like a stack and pops last element off of vector
		RETURNS: Reference to (previous) top of stack
		NOTES: Throws an assert if the vector is empty */
	void Pop() { 
		TrapZ(m_Count); 
		int newCount = m_Count - 1;
		DestructOne(GetElements() + newCount);
		m_Count = newCount;
	}

	/* Searches vector for a match, working forward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is zero)
		RETURNS: Index of first match, or -1 if none found */
	int Find(const _Type& t, int start = 0) const {
		int i;
		ArrayAssert(start>=0);
		for (i=start; i<m_Count; ++i)
			if (GetElements()[i] == t)
				return i;
		return -1;
	}

	/* Searches vector for a match, working backward
		PARAMS: 
			t - Element to search for
			start - Starting index of search (default is -1, which means the end of the vector)
		RETURNS: Index of first match, or -1 if none found */
	int ReverseFind(const _Type& t, int start = -1) const {
		int i;
		if ( start < 0 )
			start = m_Count - 1;
		ArrayAssert(start<m_Count);
		for (i=start; i>=0; --i)
			if (GetElements()[i] == t)
				return i;
		return -1;
	}
	
	/* Searches vector for a match, using a log N binary search
	PARAMS:
		t - Element to search for
	RETURNS: Index of first match, or -1 if none found.
	NOTES:	vector must be in sorted order and define < and == operators against _Type */
	int BinarySearch(const _Type& t) const {
		int low = 0;
		int high = m_Count-1;
		while (low <= high) {
			int mid = (low + high) >> 1;
			if (t == m_Storage.m_Data[mid])
				return mid;
			else if (t < m_Storage.m_Data[mid])
				high = mid-1;
			else
				low = mid+1;
		}
		return -1;
	}

	inline _Type* GetElements() { return (_Type*)m_Storage.m_Data; }
	inline const _Type* GetElements() const { return (_Type*)m_Storage.m_Data; }

	/* Functions needed to make this container STL compliant: */
	iterator begin()				{ return &GetElements()[0]; }
	iterator end()					{ return &GetElements()[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
	const_iterator begin() const	{ return &GetElements()[0]; }
	const_iterator end() const		{ return &GetElements()[m_Count]; }		//Note: Upper limit is one beyond last element as is always the case with STL. Note, this does not actualy result in an out of bounds memory access.
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
//	void swap(atRangeVector &that)	{ Swap(that); }							//swap is currently not implemented because it would only work with another array of the exact same size and then it would have to copy every single element.

	void resize(int sz, _Type c = _Type()) { SetCount(sz, c); }
	int capacity() const { return _MaxCount; }
	void reserve(int /*n*/) { }

	const _Type& at(int sz) const { return GetElements()[sz]; }
	_Type& at(int sz) { return GetElements()[sz]; }
	const _Type& front() const { return *begin(); }
	_Type& front() { return *begin(); }
	const _Type& back() const { return *(end()-1); }
	_Type& back() { return *(end() - 1); }

	template<class InputIterator>
	void assign(InputIterator first, InputIterator last)
	{
		Reset();
		while(first != last)
		{
			Append() = *first;
			++first;
		}
	}

	void assign(size_type n, const _Type& u) 
	{
		Reset();
		SetCount(n, u);
	}

	void push_back(const _Type& x) { Push(x); }
	void pop_back() { Pop(); }


protected:

	_Type *Insert(int insertionIndex, int numNewElements);

private:
	/* Storage for the array itself */
	atAlignedStorage<_MaxCount * sizeof(_Type), __alignof(_Type)> m_Storage;
	/* Count of elements currently in the array */
	int m_Count;

	// Serialize method -- allow reallocation as needed
	friend datSerialize & operator<< <>( datSerialize &ser, atFixedVector &array );

#undef DestructOne
};

template <class _Type,int _MaxCount>
typename atFixedVector<_Type,_MaxCount>::iterator atFixedVector<_Type,_MaxCount>::erase(iterator where)
{
	ArrayAssert(where != end());
	return erase(where, where+1);
}


template <class _Type,int _MaxCount>
typename atFixedVector<_Type,_MaxCount>::iterator atFixedVector<_Type,_MaxCount>::erase(iterator first, iterator last)
{
	ArrayAssert(first>=begin() && (first<end() || (first==last && first==end())));
	ArrayAssert(last>=begin() && last<=end());
	int erasedElts = ptrdiff_t_to_int(last - first);
	Assign(first, first + (end() - last), last);  // Copy _from_ last to end() _to_ first 
	Destruct(end() - erasedElts, end());
	m_Count= m_Count - erasedElts;
	return first;
}


/*
Inserts empty elements.
PARAMS:	insertionIndex	- the first new slot will have this index.
		numNewElements	- the number of slots to insert.
RETURNS: Address of the first empty slot.
*/
template <class _Type,int _MaxCount>
_Type *atFixedVector<_Type,_MaxCount>::Insert(int insertionIndex, int numNewElements)
{
	ArrayAssert(insertionIndex>=0 && insertionIndex <= m_Count);
	ArrayAssert(numNewElements > 0);
	ArrayAssert(m_Count + numNewElements <= _MaxCount);											//There has to be enough space for the new elements - atFixedVector can't grow.

	_Type							*insertionPtr= begin() + insertionIndex;

	// NOTE: Some of this is redundant, could copyconstruct part of the array and assign part instead.
	Construct(end(), end() + numNewElements);
	std::copy_backward(insertionPtr, end(), end() + numNewElements);

	m_Count = m_Count + numNewElements;
	return &GetElements()[insertionIndex];
}


template <class _Type,int _MaxCount> 
typename atFixedVector<_Type,_MaxCount>::iterator atFixedVector<_Type,_MaxCount>::insert(iterator where, const _Type &value)
{
	_Type			*newElement= Insert(where - begin(), 1);
	*newElement= value;
	return newElement;
}

template <class _Type,int _MaxCount> 
void atFixedVector<_Type,_MaxCount>::insert(iterator where, size_type numNewElements, const _Type &value)
{
	if(numNewElements == 0)
		return;
	else
	{
		_Type			*newElements= Insert((int)(where - begin()), numNewElements);
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
template<class InputIterator> void atFixedVector<_Type,_MaxCount>::insert(iterator where, InputIterator srcFirst, InputIterator srcLast)
{
	const int		numNewElements= std::distance(srcFirst, srcLast);							//Cheap for random access iterators, expensive otherwise.
	ArrayAssert(numNewElements > 0);
	_Type				*newElements= Insert(where - begin(), numNewElements);
	for(_Type *dest=newElements; dest<newElements+numNewElements; dest++, srcFirst++)
	{
		ArrayAssert(srcFirst!=srcLast);
		*dest= *srcFirst;
	}
	return;
}




#undef STL_TYPEDEFS

} // namespace rage

#endif