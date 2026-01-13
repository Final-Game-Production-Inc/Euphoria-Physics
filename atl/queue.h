//
// atl/queue.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_QUEUE_H
#define ATL_QUEUE_H

namespace rage {

/* 
PURPOSE:
	Implement a type-safe circular queue class.

	Usage here is simlar to an atArray, except that Push and Pop implement a FIFO, not a LIFO. 

	Head, tail, and count are all stored explicitly so the queue can tell empty and full apart
	without having to waste a queue slot.
<FLAG Component>
*/
template <class _Type,int _Size>
class atQueue {
public:
	// PURPOSE: Resets queue to empty
	void Reset() {
		m_Count = 0;
		m_Head = m_Tail = 0;
	}
	
	// PURPOSE: Default constructor
	atQueue() { Reset(); }

	// PURPOSE: Destructor
	~atQueue() { }

	// PURPOSE: Push an object into the queue
	// PARAMS: t - Object to push into queue (uses assignment to copy)
	// RETURNS: True on success, or false of queue was full
	bool Push(const _Type& t) {
		const int cnt = m_Count;
		if(cnt < _Size)
		{
			int head = m_Head;
			if (++head == _Size)
				head = 0;
			m_Q[head] = t;
			m_Head = head;
			m_Count = cnt + 1;
			return true;
		}
		Assertf(cnt == _Size,
				"atQueue overflow, probable multi-threading bug or memory stomp (%d/%d/%d/%d).",
				m_Head, m_Tail, m_Count, _Size);
		return false;
	}

	// PURPOSE: Push an object into the top of the queue
	// PARAMS: t - Object to push into queue (uses assignment to copy)
	// RETURNS: True on success, or false of queue was full
	bool PushTop(const _Type& t) {
		if (m_Count == _Size)
			return false;
		m_Q[m_Tail] = t;
		if (--m_Tail == -1)
			m_Tail = _Size-1;
		++m_Count;
		return true;
	}

	// PURPOSE: Test queue for empty
	// RETURNS: True if queue is empty, else false
	bool IsEmpty() const { return m_Count == 0; }

	// PURPOSE: Test queue for full
	// RETURNS: True if queue is full, else false
	bool IsFull() const { return m_Count == _Size; }

	// PURPOSE: Returns count of objects in the queue
	// RETURNS: Count of objects in the queue
	int GetCount() const { return m_Count; }

	// PURPOSE: Returns total size of the queue
	// RETURNS: Total size of the queue
	int GetSize() const { return _Size; }

	// PURPOSE: Returns number of available slots in the queue
	// RETURNS: Number of available slots in the queue
	int GetAvailable() const { return GetSize() - GetCount(); }

	// PURPOSE: Access queue like an array
	// PARAMS: i - Index (relative to tail) of entry to examine
	//		zero is the same as atQueue::Top
	// RETURNS: Reference to queue entry at specified slot
	// NOTES: Asserts out if i is out of range or queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.
	__forceinline _Type& operator[](int i) { 
		FastAssert((i >= 0) && (i < GetCount())); 
		return m_Q[(m_Tail + i + 1) % (_Size)]; 
	}

	// PURPOSE: The same as the regular [] operator but this time the returned
	// element is const and so is the function itself.
	__forceinline const _Type& operator[](int i) const { 
		FastAssert((i >= 0) && (i < GetCount())); 
		return m_Q[(m_Tail + i + 1) % (_Size)]; 
	}

	// PURPOSE: Delete an item from the middle of the queue (by moving subsequent elements towards the front of the queue)
	void Delete(int i) {
		FastAssert((i >= 0) && (i < GetCount()));
		for (int j = i + 1; j < m_Count; j++)
			m_Q[(m_Tail + j) % (_Size)] = m_Q[(m_Tail + j + 1) % (_Size)];
		--m_Count;
		if (m_Head == 0)
			m_Head = _Size - 1;
		else
			m_Head--;
	}

	
	// PURPOSE: Delete an item from the middle of the queue (by moving preceding elements towards the back of the queue)
	void DeleteByCopyingUpwards(int i) {
		FastAssert((i >= 0) && (i < GetCount()));
		for (int j = i; j > 0; j--)
			m_Q[(m_Tail + j + 1) % (_Size)] = m_Q[(m_Tail + j) % (_Size)];
		--m_Count;
		if (m_Tail == _Size - 1)
			m_Tail = 0;
		else
			m_Tail++;
	}

	// PURPOSE: Find an object in the queue
	// PARAMS: 
	//		t - Object to be found,
	//		indexOut - Index of object, if present.	
	// RETURNS: True on success; false if object not found
	// NOTES: not meant to be fast; other data structures
	//		would do better at Find if speed is necessary.
	bool Find(const _Type& t, int* indexOut=NULL) const {
		int n=0;
		for (; n<m_Count; n++)
		{
			if (operator[](n) == t)
			{
				if (indexOut)
					*indexOut = n;
				return true;
			}
		}
		return false;
	}

	// PURPOSE: Insert an item into the middle of the queue.
	// Everything from slot i and up gets shifted up to make
	// room for the new entry, and t ends up in (*this)[i].
	void Insert(int i, const _Type& t) {
		FastAssert((i >= 0) && (i < GetCount()) && !IsFull());

		// First, append the last element:
		Push((*this)[GetCount()-1]);

		// Next, shift everything over by one:
		for( int idx=GetCount()-2; idx>i; --idx )
			(*this)[idx] = (*this)[idx-1];

		// Finally, set the inserted element to the new value:
		(*this)[i] = t;
	}

	// PURPOSE: Returns reference to topmost entry in queue
	// RETURNS: Reference to topmost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.
	_Type& Top() {
		FastAssert(!IsEmpty());
		int t = m_Tail + 1;
		if (t == _Size)
			t = 0;
		return m_Q[t];
	}
	const _Type& Top() const {
		FastAssert(!IsEmpty());
		int t = m_Tail + 1;
		if (t == _Size)
			t = 0;
		return m_Q[t];
	}

	// PURPOSE: Returns reference to bottommost entry in queue and removes it from queue
	// RETURNS: Reference to bottommost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& End() {
		FastAssert(!IsEmpty());
		return m_Q[m_Head];
	}
	const _Type& End() const {
		FastAssert(!IsEmpty());
		return m_Q[m_Head];
	}

	// PURPOSE: Returns reference to topmost entry in queue and pops it from queue
	// RETURNS: Reference to topmost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& Pop() {
		FastAssert(!IsEmpty());
		--m_Count;
		if (++m_Tail == _Size)
			m_Tail = 0;
		return m_Q[m_Tail];
	}

	// PURPOSE: Removes topmost entry in queue
	// NOTES: Arguably Pop should be sufficient, but this allows you
	// to be more explicit and possibly keep the compiler from performing
	// an unnecessary copy operation
	void Drop() {
		FastAssert(!IsEmpty());
		--m_Count;
		if (++m_Tail == _Size)
			m_Tail = 0;
	}

	/* RETURNS: Reference to a newly appended array element. */
	_Type& Append() { 
		FastAssert(!IsFull());
		if (++m_Head == _Size)
			m_Head = 0;
		++m_Count;
		return m_Q[m_Head];
	}

	// PURPOSE: Returns reference to bottommost entry in queue and removes it from queue
	// RETURNS: Reference to bottommost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& PopEnd() {
		FastAssert(!IsEmpty());
		--m_Count;
		int h = m_Head;
		if (m_Head == 0)
			m_Head = _Size - 1;
		else
			m_Head--;
		return m_Q[h];
	}

	// PURPOSE: Resizes the queue by appending/dropping elements to/from the bottom of the queue
	void Resize(int count) {
		FastAssert(count <= _Size);
		m_Head = (m_Tail + count + 1) % _Size;
		m_Count = count;
	}

private:
	_Type m_Q[_Size];		// Storage space for the queue object
	int m_Head,			// Head of the queue; insertions happen at this point
		m_Tail,			// Tail of the queue; removals happen at the slot after this point
		m_Count;		// Number of items currently in the queue
};


/* 
PURPOSE:
	Implement a type-safe circular queue class.

	Usage here is simlar to an atArray, except that Push and Pop implement a FIFO, not a LIFO. 

	Head, tail, and count are all stored explicitly so the queue can tell empty and full apart
	without having to waste a queue slot.
<FLAG Component>
*/
template<class _Type, int _Size>
struct FixedMemoryController
{
	int GetSize() const { return _Size; }
	_Type m_Q[_Size];
};



template < class _Type,  class MemoryController >
class atQueueImp : public MemoryController
{
public:
	// PURPOSE: Resets queue to empty
	void Reset() {
		m_Count = 0;
		m_Head = m_Tail = 0;
	}
	
	// PURPOSE: Default constructor
	atQueueImp() { Reset(); }

	// PURPOSE: Destructor
	~atQueueImp() { }

	// PURPOSE: Push an object into the queue
	// PARAMS: t - Object to push into queue (uses assignment to copy)
	// RETURNS: True on success, or false of queue was full
	bool Push(const _Type& t) {
		if (m_Count == GetSize())
			return false;
		if (++m_Head == GetSize())
			m_Head = 0;
		MemoryController::m_Q[m_Head] = t;
		++m_Count;
		return true;
	}

	// PURPOSE: Test queue for empty
	// RETURNS: True if queue is empty, else false
	bool IsEmpty() const { return m_Count == 0; }

	// PURPOSE: Test queue for full
	// RETURNS: True if queue is full, else false
	bool IsFull() const { return m_Count == GetSize(); }

	// PURPOSE: Returns count of objects in the queue
	// RETURNS: Count of objects in the queue
	int GetCount() const { return m_Count; }

	// PURPOSE: Returns total size of the queue
	// RETURNS: Total size of the queue
	int GetSize() const { return MemoryController::GetSize(); }

	// PURPOSE: Returns number of available slots in the queue
	// RETURNS: Number of available slots in the queue
	int GetAvailable() const { return GetSize() - GetCount(); }

	// PURPOSE: Access queue like an array
	// PARAMS: i - Index (relative to tail) of entry to examine
	//		zero is the same as atQueue::Top
	// RETURNS: Reference to queue entry at specified slot
	// NOTES: Asserts out if i is out of range or queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.
	__forceinline _Type& operator[](int i) { 
		FastAssert((i >= 0) && (i < GetCount())); 
		return MemoryController::m_Q[(m_Tail + i + 1) % (GetSize())]; 
	}

	// PURPOSE: The same as the regular [] operator but this time the returned
	// element is const and so is the function itself.
	__forceinline const _Type& operator[](int i) const { 
		FastAssert((i >= 0) && (i < GetCount())); 
		return MemoryController::m_Q[(m_Tail + i + 1) % (GetSize())]; 
	}

	// PURPOSE: Delete an item from the middle of the queue
	void Delete(int i) {
		FastAssert((i >= 0) && (i < GetCount()));
		for (int j = i + 1; j < m_Count; j++)
			MemoryController::m_Q[(m_Tail + j) % (GetSize())] = MemoryController::m_Q[(m_Tail + j + 1) % (GetSize())];
		--m_Count;
		if (m_Head == 0)
			m_Head = GetSize() - 1;
		else
			m_Head--;
	}

	// PURPOSE: Find an object in the queue
	// PARAMS: 
	//		t - Object to be found,
	//		indexOut - Index of object, if present.	
	// RETURNS: True on success; false if object not found
	// NOTES: not meant to be fast; other data structures
	//		would do better at Find if speed is necessary.
	bool Find(const _Type& t, int* indexOut=NULL) const {
		int n=0;
		for (; n<m_Count; n++)
		{
			if (operator[](n) == t)
			{
				if (indexOut)
					*indexOut = n;
				return true;
			}
		}
		return false;
	}

	// PURPOSE: Insert an item into the middle of the queue.
	// Everything from slot i and up gets shifted up to make
	// room for the new entry, and t ends up in (*this)[i].
	void Insert(int i, const _Type& t) {
		FastAssert((i >= 0) && (i < GetCount()) && !IsFull());

		// First, append the last element:
		Push((*this)[GetCount()-1]);

		// Next, shift everything over by one:
		for( int idx=GetCount()-2; idx>i; --idx )
			(*this)[idx] = (*this)[idx-1];

		// Finally, set the inserted element to the new value:
		(*this)[i] = t;
	}

	// PURPOSE: Returns reference to topmost entry in queue
	// RETURNS: Reference to topmost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.
	_Type& Top() {
		FastAssert(!IsEmpty());
		int t = m_Tail + 1;
		if (t == GetSize() )
			t = 0;
		return MemoryController::m_Q[t];
	}
	const _Type& Top() const {
		FastAssert(!IsEmpty());
		int t = m_Tail + 1;
		if (t == GetSize() )
			t = 0;
		return MemoryController::m_Q[t];
	}

	// PURPOSE: Returns reference to bottommost entry in queue and removes it from queue
	// RETURNS: Reference to bottommost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& End() {
		FastAssert(!IsEmpty());
		return MemoryController::m_Q[m_Head];
	}
	const _Type& End() const {
		FastAssert(!IsEmpty());
		return MemoryController::m_Q[m_Head];
	}

	// PURPOSE: Returns reference to topmost entry in queue and pops it from queue
	// RETURNS: Reference to topmost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& Pop() {
		FastAssert(!IsEmpty());
		--m_Count;
		if (++m_Tail == GetSize())
			m_Tail = 0;
		return MemoryController::m_Q[m_Tail];
	}

	// PURPOSE: Removes topmost entry in queue
	// NOTES: Arguably Pop should be sufficient, but this allows you
	// to be more explicit and possibly keep the compiler from performing
	// an unnecessary copy operation
	void Drop() {
		FastAssert(!IsEmpty());
		--m_Count;
		if (++m_Tail == GetSize())
			m_Tail = 0;
	}

	/* RETURNS: Reference to a newly appended array element. */
	_Type& Append() { 
		FastAssert(!IsFull());
		if (++m_Head == GetSize())
			m_Head = 0;
		++m_Count;
		return MemoryController::m_Q[m_Head];
	}

	// PURPOSE: Returns reference to bottommost entry in queue and removes it from queue
	// RETURNS: Reference to bottommost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& PopEnd() {
		FastAssert(!IsEmpty());
		--m_Count;
		int h = m_Head;
		if (m_Head == 0)
			m_Head = GetSize() - 1;
		else
			m_Head--;
		return MemoryController::m_Q[h];
	}

	// PURPOSE: Resizes the queue by appending/dropping elements to/from the bottom of the queue
	void Resize(int count) {
		FastAssert(count <= GetSize());
		m_Head = (m_Tail + count + 1) % GetSize();
		m_Count = count;
	}

private:
	//
	//_Type m_Q[_Size];		// Storage space for the queue object
	int m_Head,			// Head of the queue; insertions happen at this point
		m_Tail,			// Tail of the queue; removals happen at the slot after this point
		m_Count;		// Number of items currently in the queue
};

/* 
PURPOSE:
	Wrapper around atQueue that provides the appearance of an infinitely long
	array, although you can only access elements within a sliding window _Size
	elements wide.
<FLAG Component>
*/
template <class _Type,int _Size>
class atSlidingArray : public atQueue<_Type, _Size>
{
	typedef atQueue<_Type, _Size> _Base;

public:
	// PURPOSE: Default constructor
	atSlidingArray() { Reset(); }

	// PURPOSE: Resets queue to empty
	void Reset() {
		_Base::Reset();
		m_Offset = 0;
	}

	// PURPOSE: Returns reference to topmost entry in queue and pops it from queue
	// RETURNS: Reference to topmost entry in queue
	// NOTES: Asserts out if queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.  Note that if you
	//	plan on pushing more objects into the queue immediately, you may
	//	want to do an explicit copy; otherwise, your data may disappear out
	//	from under you.
	_Type& Pop() {
		++m_Offset;
		return _Base::Pop();
	}

	// PURPOSE: Removes topmost entry in queue
	// NOTES: Arguably Pop should be sufficient, but this allows you
	// to be more explicit and possibly keep the compiler from performing
	// an unnecessary copy operation
	void Drop() {
		++m_Offset;
		_Base::Drop();
	}

	// PURPOSE: Delete an item from the middle of the queue
	void Delete(int i) {
		_Base::Delete(i - m_Offset);
	}

	// PURPOSE: Find an object in the queue
	// PARAMS: 
	//		t - Object to be found,
	//		indexOut - Index of object, if present.	
	// RETURNS: True on success; false if object not found
	// NOTES: not meant to be fast; other data structures
	//		would do better at Find if speed is necessary.
	bool Find(const _Type& t, int* indexOut=NULL) {
		if (_Base::Find(t, indexOut))
		{
			if (indexOut)
				*indexOut += m_Offset;
			return true;
		}
		return false;
	}

	// PURPOSE: Insert an item into the middle of the queue.
	// Everything from slot i and up gets shifted up to make
	// room for the new entry, and t ends up in (*this)[i].
	void Insert(int i, const _Type& t) {
		_Base::Insert(i - m_Offset, t);
	}

	// PURPOSE: Returns count of objects in the queue
	// RETURNS: Count of objects in the queue
	int GetCount() const { return m_Offset + _Base::GetCount(); }

	int GetOffset() const { return m_Offset; }

	// PURPOSE: Access queue like an array
	// PARAMS: i - Index (relative to tail) of entry to examine
	//		zero is the same as atQueue::Top
	// RETURNS: Reference to queue entry at specified slot
	// NOTES: Asserts out if i is out of range or queue is empty
	// REMARKS: Value is returned by non-const reference, so you
	//	can modify the entry directly if you want.
	__forceinline _Type& operator[](int i) { 
		return _Base::operator[](i - m_Offset);
	}

	// PURPOSE: The same as the regular [] operator but this time the returned
	// element is const and so is the function itself.
	__forceinline const _Type& operator[](int i) const { 
		return _Base::operator[](i - m_Offset);
	}

	// PURPOSE: Resizes the queue by appending/dropping elements to/from the bottom of the queue
	void Resize(int count) {
		if (count >= m_Offset)
			_Base::Resize(count - m_Offset);
		else
			Reset();
	}

private:
	int m_Offset;
};








}	// namespace rage

#endif
