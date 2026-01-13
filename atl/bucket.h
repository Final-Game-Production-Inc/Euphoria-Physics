// 
// atl/bucket.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 


#ifndef ATL_BUCKET_H
#define ATL_BUCKET_H

#include "data/resource.h"

namespace rage {

// broken out so that it can be selectively disabled separately if it's a time sink
#ifndef bcktArrayAssert
# if __ASSERT
# define bcktArrayAssert(x)	FastAssert(x)
# else
# define bcktArrayAssert(x)
# endif
#endif

/*
PURPOSE:
atBucket is very similar to the atArray in functionality.  However it doesn’t have C++ array semantics 
(ie a atBucket<Vector3> cannot be passed to a function that expects a Vector3[] since the elements are 
not stored contiguously).  The data is stored in multiple buckets that will never be destroyed when resized.
This allows classes to store pointers to the data and not worry that the data will disappear later.  
The key advantage is it is much faster for arrays that need to expand or contract at run time.
PARAMETERS:
_Type - the type of the Array's element
_AllocStep - the number of elements to allocate per bucket (default unsigned short)
<FLAG Component>
*/

template <class _Type,int _AllocStep = 16>		
class atBucket {
public:
	/* Default constructor.  Bucket is left totally empty */
	atBucket() 
	{
		// adjust to powers of two
		m_BucketSize = 1;
		m_BucketShift = 0;
		while (m_BucketSize < _AllocStep)
		{
			m_BucketSize <<= 1;
			m_BucketShift++;
		}
		m_BucketMask = m_BucketSize - 1;

		m_Elements = NULL;
		m_Count = 0;
		m_BucketAlloc = 0;
	}

	/* Initialize bucket allocation to specified size, but bucket still contains zero elements
	PARAMS: baseCount - Number of entries to preallocate */
	atBucket(int baseCount)
	{
		atBucket();
		Reallocate(baseCount);
	}

	/* Resource functionality */
	void Place(datResource &rsc) { ::new (this) atBucket<_Type>(rsc); }

	/* Intended for classes that need resource compiler support */
	explicit atBucket(bool initialize) {if (initialize) {atBucket();}}

	explicit atBucket(datResource &rsc) { rsc.PointerFixup(m_Elements); for (int i=0; i<m_BucketAlloc; i++) {rsc.PointerFixup(m_Elements[i]);} }

	/* We can't change the default rsc ctor without breaking a bunch of stuff. */
	atBucket(datResource &rsc,bool ASSERT_ONLY(flag)) {
		bcktArrayAssert(flag);
		rsc.PointerFixup(m_Elements); 
		for (int i=0; i<m_BucketAlloc; i++)
			rsc.PointerFixup(m_Elements[i]);
		for (int i=0; i<m_Count; i++)
			GetElementRefIdx(i).Place(rsc);
	}

	/* Default destructor */
	~atBucket() { Kill(); }

	/* Construct a bucket from another bucket.  Original bucket is unchanged.
	PARAMS: that - Bucket to copy initial data from */
	atBucket(const atBucket& that) { CopyFrom(that); }

	/* Copy a bucket from another bucket.  Other bucket is unchanged, this bucket has its contents replaced.
	PARAMS: that - Bucket to copy data from */
	atBucket& operator=(const atBucket& that) { if (this != &that) { Kill(); CopyFrom(that); } return *this; }

	/* Assume ownership of a bucket.
	PARAMS: that - Bucket to assume control of.  The bucket is left empty.
	NOTES: Useful when doing batch delete operations */
	void Assume(atBucket& that) {
		CopyFrom(that);
	}

	/* Access bucket, with range checking */
	_Type& operator[](int index)
	{
		bcktArrayAssert(index>=0&&index<m_Count); 
		return GetElementRefIdx(index); 
	}

	/* Access bucket, with range checking */
	const _Type& operator[](int index) const 
	{
		bcktArrayAssert(index>=0&&index<m_Count); 
		return GetElementRefIdx(index); 
	}

	/* RETURNS: Number of valid items in the bucket.  Not necessarily the same as the number of entries allocated */
	int GetCount() const { return m_Count; }

	/* RETURNS: Number of valid items in the bucket.  Not necessarily the same as the number of entries allocated */
	int operator() () const { return m_Count; }

	/* RETURNS: Number of items preallocated in the bucket.  Not necessarily the same as the number of items in the bucket*/
	int GetAllocCount() const { return m_BucketAlloc * m_BucketSize; } // Current Capacity

	/* Initializes bucket with a given size
	PARAMS: count - New size for bucket */
	void Init(int count) {
		Reallocate(count);
		m_Count = count;
	}

	/* Initializes bucket to a given value and a given size
	PARAMS: t - Item to initialize bucket with
	count - New size for bucket */
	void Init(const _Type& t,int count) {
		Init(count);
		for (int i=0; i<count; i++)
			GetElementRefIdx(i) = t;
	}

	/* Clears bucket and reclaims its storage   PARAMS: New preallocation size, defaults to zero */
	void Reset(int size = 0) {
		for (int i=0;i<m_BucketAlloc;i++)
			delete[] m_Elements[i];
		delete[] m_Elements;
		m_Elements = 0;
		m_BucketAlloc = 0;
		m_Count = 0;
		if (size)
			Reallocate(size);
	}

	/* Resizes a bucket, preserving its content. 
	PARAMS: newCount - New number of elements in the bucket
	RETURNS: Pointer to raw storage for the bucket
	NOTES: Both the count and the preallocated size are set to newCount.
	If newCount is smaller than the number of elements previously
	in the bucket, the extra entries are lost.  The bucket is always
	copied.
	SEE ALSO: Resize */
	bool Reallocate(int newCount) {
		if (newCount > 0)
		{
			int newBucket = GetBucketsForLength(newCount);
			if (newBucket > m_BucketAlloc)
			{
				// expand bucket of chunks
				_Type** temp = rage_new _Type*[newBucket];
				if (temp == NULL)
				{
					Assert(0);
					Reset();
					return false;
				}
				else
				{
					// copy existing chunk ptrs
					for (int i=0; i<m_BucketAlloc; i++)
						temp[i] = m_Elements[i];
					// allocate new chunks
					for (int i=m_BucketAlloc; i<newBucket; i++)
					{
						temp[i] = rage_new _Type[m_BucketSize];
						if (temp[i] == NULL)
						{
							Assert(0);
							Reset();
							return false;
						}
					}

					// delete old ptrs and assign the new ones
					delete[] m_Elements;
					m_Elements = temp;
					m_BucketAlloc = newBucket;
				}
			}			
		}

		return true;
	}

	/* Resizes a bucket, preserving its content
	PARAMS: newCount - New number of elements in the bucket
	NOTES: If you are shrinking the bucket, the bucket is not copied.
	*/
	void Resize(int newCount) {
		if (newCount > m_Count)
		{
			if (Reallocate(newCount))
				m_Count = newCount;
		}
		else
		{
			m_Count = newCount;
		}
	}

	void GrowBy(int delta) {
		Reallocate(m_Count + delta);
	}

	/* RETURNS: Reference to a newly appended bucket element. */
	_Type& Append() { 
		Reallocate(m_Count + 1); // Potential crash 
		return GetElementRefIdx(m_Count++);
	}
	_Type& Grow() { 
		return Append();
	}

	/* Adds two elements at once
	PARAMS: a, b - Elements to add */
	void Append2(const _Type& a,const _Type& b) {
		if (Reallocate(m_Count + 2))
		{
			GetElementRefIdx(m_Count) = a;
			GetElementRefIdx(m_Count+1) = b;
			m_Count += 2;
		}
	}

	/* Adds three elements at once
	PARAMS: a, b, c - Elements to add */
	void Append3(const _Type& a,const _Type& b,const _Type& c) {
		if (Reallocate(m_Count + 3))
		{
			GetElementRefIdx(m_Count) = a;
			GetElementRefIdx(m_Count+1) = b;
			GetElementRefIdx(m_Count+2) = c;
			m_Count += 3;
		}
	}

	/* Adds four elements at once
	PARAMS: a, b, c, d - Elements to add */
	void Append4(const _Type& a,const _Type& b,const _Type& c,const _Type& d) {
		if (Reallocate(m_Count + 4))
		{
			GetElementRefIdx(m_Count) = a;
			GetElementRefIdx(m_Count+1) = b;
			GetElementRefIdx(m_Count+2) = c;
			GetElementRefIdx(m_Count+3) = d;
			m_Count += 4;
		}
	}

	/* Inserts an empty slot into an existing bucket
	PARAMS: beforeIndex - Bucket index to insert an empty slot before
	RETURNS: Reference to the empty slot
	NOTES: The slot is not really empty.  It will contain whatever
	bucket data happened to be there before. */
	_Type& Insert(int beforeIndex) {

		Reallocate(m_Count+1);

		// shift elements over
		for (int i = m_Count; i > beforeIndex; i--)
			GetElementRefIdx(i) = GetElementRefIdx(i-1);

		m_Count++;
		return GetElementRefIdx(beforeIndex);
	}

	/* Deletes bucket element at specified index
	PARAMS: index - Index of bucket element to delete */
	void Delete(int index) {
		bcktArrayAssert(index>=0 && index < m_Count);
		for (int i = index; i < m_Count-1; i++)
			GetElementRefIdx(i) = GetElementRefIdx(i+1);
		m_Count--;
	}

	/* Deletes bucket element at specified index
	PARAMS: index - Index of bucket element to delete */
	void DeleteFast(int index) {
		bcktArrayAssert(index>=0 && index < m_Count);
		if (index != m_Count-1)
			GetElementRefIdx(index) = GetElementRefIdx(m_Count-1);
		m_Count--;
	}

	/* Searches bucket for a match, working forward
	PARAMS: 
	t - Element to search for
	start - Starting index of search (default is zero)
	RETURNS: Index of first match, or -1 if none found */
	int Find(const _Type& t, int start = 0) const {
		int i;
		bcktArrayAssert(start>=0);
		for (i=start; i<m_Count; ++i)
			if (GetElementRefIdx(i) == t)
				return i;
		return -1;
	}

	/* Access bucket, with range checking, ensuring existence of index
	PARAMS: index - index of bucket element to access/add if necessary
	NOTES: this can introduce a div and mod every access in the worst case, shouldn't happen in practice */
	_Type& SafeAccess(int index) {
		bcktArrayAssert(index>=0);
		if(m_Count <= index)
		{
			m_Count = (index + 1);
			Reallocate(m_Count);			
		}

		return GetElementRefIdx(index);
	}

protected:

	int GetBucket(int index) const {return index >> m_BucketShift;}
	int GetIndex(int index) const {return index & m_BucketMask;}
	int GetBucketsForLength(int len) const {return len <= 0 ? 0 : GetBucket(len-1)+1;}
#if __ASSERT
	_Type& GetElementRefIdx(int index) const
	{
		int nBucket = GetBucket(index);
		int nIndex = GetIndex(index);
		bcktArrayAssert(nBucket >= 0 && nBucket < m_BucketAlloc);
		bcktArrayAssert(nIndex >= 0 && nIndex < m_BucketSize);

		return m_Elements[nBucket][nIndex];
	}
#else
	_Type& GetElementRefIdx(int index) const {return m_Elements[GetBucket(index)][GetIndex(index)];}
#endif

	/* Internal copy helper.  Allocates bucket of same size and copies elements.
	PARAMS: that - Bucket to copy */
	void CopyFrom(const atBucket &that) {
		if (this != &that)
		{
			m_Count = 0;
			if (that.m_Count)
			{
				if (Reallocate(that.m_Count))
				{
					for (int i = 0; i < that.m_Count; i++)
						GetElementRefIdx(i) = that.GetElementRefIdx(i);
					m_Count = that.m_Count;
				}
			}
		}
	}

	/* Internal cleanup function.  Reclaims all heap memory associated with elements */
	void Kill() 
	{
		for (int i=0;i<m_BucketAlloc;i++)
			delete[] m_Elements[i];
		delete[] m_Elements;
		m_Elements = 0;
		m_BucketAlloc = 0;
		m_Count = 0;
	}

	/* The raw bucket of elements */
	_Type** m_Elements;

	int m_BucketSize;
	int m_BucketMask;
	int m_BucketShift;

	int m_Count; // logical bucket limit
	int m_BucketAlloc;	
};

} // end name space

#endif //ATL_BUCKET_H
