// 
// system/obfuscatedtypes.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_OBFUSCATEDTYPES_H 
#define SYSTEM_OBFUSCATEDTYPES_H 

#include "system/new.h"
#include <stddef.h>		// for size_t

namespace rage {

namespace sysObfuscatedTypes {
	// Obfuscated rand() function. Essentially the same as rand(), but inline, and generates
	// 32-bit values. Not thread safe, it doesn't need to be - we're not storing any meaningful
	// state here.
	__forceinline u32 obfRand()
	{
		static u32 next = 0xCB536E6A;
		next = next * 214013 + 2531011;
		return next;
	}
}

// This template class obfuscates the contained data in memory, by XORing it with a magic number
// and the address of the data itself. Make sure you do NOT perform a bitwise copy of this type!
// The type contained in a sysObfuscated does not need to be a POD type, with two caveats:
// 1.	If external code tries to access the contained object through its 'this' pointer, the
//		external code will access the obfuscated value and the code will likely crash.
// 2.	The copy constructor and operator= will not be called in a standards compliant manner. In
//		particular, the copy constructor may be called with a right-hand-side argument that is not
//		an object that has had a constructor run on it, but is still a valid object, although it
//		will only be aligned to 4 bytes (Default alignment for u32).
//
// Template arguments:
// T:		The type being obfuscated, e.g. u32, float. The type must be a multiple of 32 bits in
//			size.
// TMutate:	If true, then the class will be 2x the size of the base type, and will be further
//			mangled in memory. Every time the data is read, the class representation will change in
//			memory, making it difficult to identify real changes with data breakpoints. This has a
//			performance impact compared to using a non-mutated type.
template<class T, bool TMutate=true> class sysObfuscated
{
public:
	// PURPOSE:	Constructor
	// PARAMS:	rhs - Right-hand side argument for copy constructor
	// PARAMS:	data - Data to initialise with for copy constructor
	sysObfuscated() { CompileTimeAssert((sizeof(T) & 3) == 0); Init(); }
	sysObfuscated(const sysObfuscated<T, TMutate>& rhs) { Init(); Set(rhs.Get()); }
	template<bool TMutateOther> sysObfuscated(const sysObfuscated<T, TMutateOther>& rhs) { Init(); Set(rhs.Get()); }
	explicit sysObfuscated(const T& data) { Init(); Set(data); }

	// PURPOSE:	Destructor
	~sysObfuscated() {}

	// PURPOSE:	Access the unobfuscated value contained
	// RETURNS:	The unobfuscated data.
	__forceinline T Get() const;

	// PURPOSE:	Set the value contained
	// PARAMS:	data - Unobfuscated value to set
	__forceinline void Set(const T& data);

	// PURPOSE:	Implicit conversion operator
	__forceinline operator T() const { return Get(); }

	// PURPOSE:	Comparison against other sysObfuscated's
	// PARAMS:	rhs - Object to compare against
	// NOTES:	Templated to allow you to compare sysObfuscated's with different mutation values
	template<bool TMutateOther> bool operator==(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() == rhs.Get(); }
	template<bool TMutateOther> bool operator!=(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() != rhs.Get(); }
	template<bool TMutateOther> bool operator<(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() < rhs.Get(); }
	template<bool TMutateOther> bool operator<=(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() <= rhs.Get(); }
	template<bool TMutateOther> bool operator>(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() > rhs.Get(); }
	template<bool TMutateOther> bool operator>=(const sysObfuscated<T, TMutateOther>& rhs) const { return Get() >= rhs.Get(); }

	// PURPOSE:	Assignment operator
	// PARAMS:	rhs - Object to assign
	// PARAMS:	data - Value to assign
	template<bool TMutateOther> sysObfuscated<T, TMutate>& operator=(const sysObfuscated<T, TMutateOther>& rhs) { Set(rhs.Get()); return *this; }
	sysObfuscated<T, TMutate>& operator=(const T& data) { Set(data); return *this; }

	// PURPOSE:	Misc. operators
	// PARAMS:	rhs - Object to assign
	// PARAMS:	data - Value to assign
	template<bool TMutateOther> sysObfuscated<T, TMutate>& operator+=(const sysObfuscated<T, TMutateOther>& rhs) { Set(Get()+rhs.Get()); return *this; }
	sysObfuscated<T, TMutate>& operator+=(const T& data) { Set(Get()+data); return *this; }
	template<bool TMutateOther> sysObfuscated<T, TMutate>& operator-=(const sysObfuscated<T, TMutateOther>& rhs) { Set(Get()-rhs.Get()); return *this; }
	sysObfuscated<T, TMutate>& operator-=(const T& data) { Set(Get()-data); return *this; }
	template<bool TMutateOther> sysObfuscated<T, TMutate>& operator*=(const sysObfuscated<T, TMutateOther>& rhs) { Set(Get()*rhs.Get()); return *this; }
	sysObfuscated<T, TMutate>& operator*=(const T& data) { Set(Get()*data); return *this; }
	template<bool TMutateOther> sysObfuscated<T, TMutate>& operator/=(const sysObfuscated<T, TMutateOther>& rhs) { Set(Get()/rhs.Get()); return *this; }
	sysObfuscated<T, TMutate>& operator/=(const T& data) { Set(Get()/data); return *this; }

private:
	// Initialise m_xor and m_mutate to random values
	void Init();

	// The obfuscated data
	mutable u32 m_data[(TMutate ? sizeof(T)*2 : sizeof(T)) / sizeof(u32)];

	// XOR and mutate keys for this type
	mutable u32 m_xor;
	mutable u32 m_mutate;
};

template<class T, bool TMutate> __forceinline void sysObfuscated<T, TMutate>::Init()
{
	m_xor = sysObfuscatedTypes::obfRand();
	if(TMutate)
	{
		m_mutate = sysObfuscatedTypes::obfRand();
	}
}

// While these functions look inefficient, they optimise well
template<class T, bool TMutate> __forceinline T sysObfuscated<T, TMutate>::Get() const
{
	u32 xorVal = m_xor ^ (u32)(size_t)this;
	u32 ret[sizeof(T)/sizeof(u32)];
	u32* src = const_cast<u32*>(&m_data[0]);
	u32* dest = (u32*)&ret;
	for(size_t i=0; i<sizeof(T)/4; ++i)
	{
		if(TMutate)
		{
			// Extract valid data from two words of storage
			u32 a = *src & m_mutate;
			u32 b = src[sizeof(T)/4] & (~m_mutate);

			// Apply entropy in the unused bits: Just flip the two u16's in the u32. We can't do a
			// huge amount more without knowledge of the mutation mask.
			u32 entropyA = ((*src & (~m_mutate)) << 16) | ((*src & (~m_mutate)) >> 16);
			u32 entropyB = ((src[sizeof(T)/4] & m_mutate) << 16) | ((src[sizeof(T)/4] & m_mutate) >> 16);
			*src = (*src & m_mutate) | entropyA;
			src[sizeof(T)/4] = (src[sizeof(T)/4] & (~m_mutate)) | entropyB;

			*dest++ = a | b;
			++src;
		}
		else
		{
			*dest++ = *src++ ^ xorVal;
		}
	}

	// Call Set() to reset the xor and mutate keys on every call to Get()
	if(TMutate)
	{
		const_cast<sysObfuscated<T, TMutate>*>(this)->Set(*(T*)&ret);
	}

	return *(T*)&ret;
}

template<class T, bool TMutate> __forceinline void sysObfuscated<T, TMutate>::Set(const T& data)
{
	// Reset xor and mutate keys
	Init();

	u32 xorVal = m_xor ^ (u32)(size_t)this;
	u32* src = (u32*)&data;
	u32* dest = &m_data[0];
	for(size_t i=0; i<sizeof(T)/4; ++i)
	{
		if(TMutate)
		{
			u32 a = *src & m_mutate;
			u32 b = *src & (~m_mutate);
			++src;

			*dest = a;
			dest[sizeof(T)/4] = b;
			++dest;
		}
		else
		{
			*dest++ = *src++ ^ xorVal;
		}
	}
}

//-------------------------------------------------------------------------------------------------

// This template class keeps track of multiple copies of a variable, and detects if they go out of
// sync, such as by someone modifying the value in memory. When this happens, the tamper callback
// is called.
// Template arguments:
// T:			The type being obfuscated, e.g. u32, float, sysObfuscated<u32>.
// TamperCB:	The tamper callback invoked when a mismatch is detected
template<class T, void (*TamperCB)()> class sysLinkedData
{
public:
	// PURPOSE:	Constructor
	// PARAMS:	rhs - Right-hand side argument for copy constructor
	// PARAMS:	data - Data to initialise with for copy constructor
	sysLinkedData() { InitDefaultLinks(); }
	sysLinkedData(const sysLinkedData<T, TamperCB>& rhs) { Set(rhs.Get()); InitDefaultLinks(); }
	explicit sysLinkedData(const T& data) : m_data(data) { InitDefaultLinks(); }

	// PURPOSE:	Destructor
	~sysLinkedData();

	// PURPOSE:	Access the value contained and check all links for tampering
	// RETURNS:	The raw data.
	__forceinline const T& Get() const;

	// PURPOSE:	Set the value contained on all linked entries
	// PARAMS:	data - Value to set
	__forceinline void Set(const T& data);

	// PURPOSE:	Add a link to another sysLinkedData
	// PARAMS:	pOther - sysLinkedData to link
	// NOTES:	pOther's data will be set to the data contained by this instance
	__forceinline void AddLink(sysLinkedData<T, TamperCB>* pOther);

	// PURPOSE:	Remove a link to another sysLinkedData
	// PARAMS:	pOther - sysLinkedData to remove
	// NOTES:	Does nothing if pOther is not linked to this.
	__forceinline void RemoveLink(sysLinkedData<T, TamperCB>* pOther);

	// PURPOSE:	Walk the list of linked items and return the number of dummy links
	// NOTES: Should only really be used for debugging.
	int GetNumDummyLinks() const;

	// PURPOSE:	Walk the list of linked items and return the number of non-dummy links
	// NOTES: Should only really be used for debugging.
	int GetNumNonDummyLinks() const;

	// PURPOSE:	Walk the list of linked items and return the number of links
	// NOTES: Should only really be used for debugging.
	int GetNumLinks() const;

	// PURPOSE:	Implicit conversion operator
	operator T() const { return Get(); }

	// PURPOSE:	Comparison against other sysLinkedData's
	// PARAMS:	rhs - Object to compare against
	bool operator==(const sysLinkedData<T, TamperCB>& rhs) const { return Get() == rhs.Get(); }
	bool operator!=(const sysLinkedData<T, TamperCB>& rhs) const { return Get() != rhs.Get(); }
	bool operator<(const sysLinkedData<T, TamperCB>& rhs) const { return Get() < rhs.Get(); }
	bool operator<=(const sysLinkedData<T, TamperCB>& rhs) const { return Get() <= rhs.Get(); }
	bool operator>(const sysLinkedData<T, TamperCB>& rhs) const { return Get() > rhs.Get(); }
	bool operator>=(const sysLinkedData<T, TamperCB>& rhs) const { return Get() >= rhs.Get(); }

	bool operator==(const T& rhs) const { return Get() == rhs; }
	bool operator!=(const T& rhs) const { return Get() != rhs; }
	bool operator<(const T& rhs) const { return Get() < rhs; }
	bool operator<=(const T& rhs) const { return Get() <= rhs; }
	bool operator>(const T& rhs) const { return Get() > rhs; }
	bool operator>=(const T& rhs) const { return Get() >= rhs; }

	// PURPOSE:	Assignment operator
	// PARAMS:	rhs - Object to assign
	// PARAMS:	data - Value to assign
	sysLinkedData<T, TamperCB>& operator=(const sysLinkedData<T, TamperCB>& rhs) { Set(rhs.Get()); return *this; }
	sysLinkedData<T, TamperCB>& operator=(const T& data) { Set(data); return *this; }

private:
	// Private c'tor and helper function for creating dummy links
	sysLinkedData(const T& data, u32 nonce) : m_data(data), m_isDummyLink(nonce) {}
	void InitDefaultLinks();

	// See if this is a dummy link
	u32 IsDummyLink() const { return m_isDummyLink & s_isDummyLinkMask; }

	T m_data;

	// Double-linked circular list
	sysLinkedData<T, TamperCB>* m_pPrev;
	sysLinkedData<T, TamperCB>* m_pNext;

	// If this mask is set, this is a dummy link and can be deleted when the last non-dummy link is destroyed
	static const u32 s_isDummyLinkMask = 3 << 3;
	u32 m_isDummyLink;
};

template<class T, void (*TamperCB)()>
__forceinline sysLinkedData<T, TamperCB>::~sysLinkedData()
{
	if(IsDummyLink())
	{
		// Dummy links don't run the full, unchaining destructor. They only get destroyed when the last
		// non-dummy link gets destroyed, so we don't care about properly unlinking everything, just
		// freeing memory.
		return;
	}

	// Is this the last non-dummy link in the chain?
	sysLinkedData<T, TamperCB>* p = m_pNext;
	bool allDummyLinks = true;
	while(p != this)
	{
		if(p->IsDummyLink() == 0)
		{
			allDummyLinks = false;
			break;
		}
		p = p->m_pNext;
	}

	// If every other link is a dummy link, delete the dummy links. NB: Walk through the prev pointer
	// rather than the next pointer as additional obfuscation, but also so memory is freed in the
	// reverse of the order it was acquired in, which might be easier on the allocator.
	if(allDummyLinks)
	{
		p = m_pPrev;
		while(p != this)
		{
			sysLinkedData<T, TamperCB>* pPrev = p->m_pPrev;
			delete p;
			p = pPrev;
		}
		return;
	}

	// There are non-dummy links still in the chain, just remove ourself from the chain
	m_pPrev->RemoveLink(this);
}

template<class T, void (*TamperCB)()>
__forceinline const T& sysLinkedData<T, TamperCB>::Get() const
{
	// Check integrity
	sysLinkedData<T, TamperCB>* p = m_pNext;
	while(p != this)
	{
		if(p->m_data != m_data)
			TamperCB();
		p = p->m_pNext;
	}
	return m_data;
}

template<class T, void (*TamperCB)()>
__forceinline void sysLinkedData<T, TamperCB>::Set(const T& data)
{
	sysLinkedData<T, TamperCB>* p = m_pNext;
	while(p != this)
	{
		p->m_data = data;
		p = p->m_pNext;
	}
	m_data = data;
}

template<class T, void (*TamperCB)()>
__forceinline void sysLinkedData<T, TamperCB>::AddLink(sysLinkedData<T, TamperCB>* pOther)
{
	// Check entry isn't already in list
	Assertf(pOther != this, "Can't add link to ourself");
	sysLinkedData<T, TamperCB>* p = m_pNext;
	while(p != this)
	{
		if(p == pOther)
			return;
		p = p->m_pNext;
	}

	// Set data in all new links
	pOther->Set(m_data);

	// Add link
	pOther->m_pPrev->m_pNext = m_pNext;
	m_pNext->m_pPrev = pOther->m_pPrev;
	m_pNext = pOther;
	pOther->m_pPrev = this;

	// Keep the number of dummy links in the list at a sensible number
	int maxLinksInSystem = 17 + (sysObfuscatedTypes::obfRand()&15); // rand between 17..32
	int linksToCull = GetNumDummyLinks() - maxLinksInSystem;
	p = m_pNext;
	while(linksToCull > 0)
	{
		// Find next dummy link
		while(!p->IsDummyLink())
		{
			p = p->m_pNext;
		}

		// Unlink it
		p->m_pNext->m_pPrev = p->m_pPrev;
		p->m_pPrev->m_pNext = p->m_pNext;

		// Delete it. NB: It's safe to just delete it because dummy links don't do any unlinking
		// in their destructors.
		sysLinkedData<T, TamperCB>* pNext = p->m_pNext;
		delete p;
		p = pNext;
		--linksToCull;
	}
}

template<class T, void (*TamperCB)()>
__forceinline void sysLinkedData<T, TamperCB>::RemoveLink(sysLinkedData<T, TamperCB>* pOther)
{
	// Find entry in list
	Assertf(pOther != this, "Can't remove link to ourself");
	sysLinkedData<T, TamperCB>* p = m_pNext;
	while(p != pOther)
	{
		if(p == this)
			return; // Looped round entire list
		p = p->m_pNext;
	}

	// Remove link
	pOther->m_pNext->m_pPrev = pOther->m_pPrev;
	pOther->m_pPrev->m_pNext = pOther->m_pNext;
	pOther->m_pPrev = pOther;
	pOther->m_pNext = pOther;
}

template<class T, void (*TamperCB)()>
__forceinline void sysLinkedData<T, TamperCB>::InitDefaultLinks()
{
	// Initialise our m_isDummyLink member
	m_isDummyLink = ((u32)(size_t)this << 3) & (~s_isDummyLinkMask);

	// Number of links to create by default. Create more links for small types (pointer or smaller)
	static const u32 numLinks = sizeof(T) <= sizeof(void*) ? 12 : 4;
	sysLinkedData<T, TamperCB>* pLast = this;
	for(u32 i=0; i<numLinks; ++i)
	{
		// Make a nonce value for the dummy link. Doesn't matter what it is, so long as the
		// s_isDummyLinkMask mask is set.
		u32 nonce = m_isDummyLink | s_isDummyLinkMask | 3;
		sysLinkedData<T, TamperCB>* pNewLink = rage_new sysLinkedData<T, TamperCB>(m_data, nonce);

		// Init forward / back pointers
		pNewLink->m_pPrev = pLast;
		pLast->m_pNext = pNewLink;
		pLast = pNewLink;
	}

	// Join the ends of the circular list
	m_pPrev = pLast;
	pLast->m_pNext = this;
}

template<class T, void (*TamperCB)()>
__forceinline int sysLinkedData<T, TamperCB>::GetNumDummyLinks() const
{
	sysLinkedData<T, TamperCB>* p = m_pNext;
	int ret = IsDummyLink() ? 1 : 0;
	while(p != this)
	{
		if(p->IsDummyLink())
			++ret;
		p = p->m_pNext;
	}
	return ret;
}

template<class T, void (*TamperCB)()>
__forceinline int sysLinkedData<T, TamperCB>::GetNumNonDummyLinks() const
{
	sysLinkedData<T, TamperCB>* p = m_pNext;
	int ret = IsDummyLink() ? 0 : 1;
	while(p != this)
	{
		if(!p->IsDummyLink())
			++ret;
		p = p->m_pNext;
	}
	return ret;
}

template<class T, void (*TamperCB)()>
__forceinline int sysLinkedData<T, TamperCB>::GetNumLinks() const
{
	sysLinkedData<T, TamperCB>* p = m_pNext;
	int ret = 1;
	while(p != this)
	{
		++ret;
		p = p->m_pNext;
	}
	return ret;
}


} // namespace rage


// These two functions are obfuscated at protection time, which
// will occur post linking. Unfortunately, having this layer of
// abstraction is the only way to use this particular guard.
// Such is my life. Defining a getter and a setter accordingly
void FastObfuscatedPointerDataGet(void *dst, void *src, size_t sz);
void FastObfuscatedPointerDataSet(void *dst, void *src, size_t sz);

#endif // SYSTEM_OBFUSCATEDTYPES_H 
