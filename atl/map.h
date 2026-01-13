//
// atl/map.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_MAP_H
#define ATL_MAP_H

#include "atl/hashstring.h"
#include "data/resource.h"
#include "data/struct.h"
#include "diag/tracker.h"

#include "math/amath.h"
#include "string/string.h"
#include "system/memops.h"

#define ATL_MAP_USE_MARKERS 0

#if ATL_MAP_USE_MARKERS
#define DATA_MARKER_INTENTIONAL_HEADER_INCLUDE 1
#include "data/marker.h"
#define ATL_MAP_FUNC() RAGE_FUNC()
#else
#define ATL_MAP_FUNC()
#endif

#define ATL_MAP_MAX_SIZE 65521

#define ENABLE_PRIME_DEBUG (0 && __BANK && !__SPU)

namespace rage {

// Add an appropriate function here if you need to hash a different type.
inline unsigned atHash(unsigned x) { return x; }
inline unsigned atHash(const void *x) { return (unsigned)(size_t)x; }
extern unsigned atHash_const_char(const char *);
extern unsigned atHash_const_charU(const char *);
extern unsigned short atHash16(const char *);
extern unsigned short atHash16U(const char *);
extern unsigned atHash64(u64 x);

extern unsigned short atHashNextSize(unsigned short s);

class datTypeStruct;
class atString;
class ConstString;

#if ENABLE_PRIME_DEBUG
extern size_t g_primeOverhead;
extern size_t g_primeWastedOverhead;
extern void IncrementSlots(unsigned short slots, size_t entrySize);
#endif

// PURPOSE: Default hash function functor, just calls atHash on the key.
template <typename _T>
struct atMapHashFn
{
	unsigned operator ()(const _T& key) const { return atHash(key); }
};

// PURPOSE: Specialized hash function functor for atString keys.
template <>
struct atMapHashFn<const char*>
{
	unsigned operator ()(const char* key) const { return atHash_const_char(key); }
};

// PURPOSE: Specialized hash function functor for atString keys.
template <> struct atMapHashFn<atString> : public atMapHashFn<const char*> { };

// PURPOSE: Specialized hash function functor for ConstString keys.
template <> struct atMapHashFn<ConstString> : public atMapHashFn<const char*> { };


// PURPOSE: Specialized hash function functor for hash value keys.
template <int Namespace> struct atMapHashFn<atNamespacedHashValue<Namespace> > 
{
	unsigned operator()(const atNamespacedHashValue<Namespace>& key) const { return key.GetHash(); }
};

// PURPOSE: Specialized hash function functor for hash string keys.
template <int Namespace> struct atMapHashFn<atNamespacedHashString<Namespace> > 
{
	unsigned operator()(const atNamespacedHashString<Namespace>& key) const { return key.GetHash(); }
};

// PURPOSE: Specialized hash function functor for case-insenitive maps.
struct atMapCaseInsensitiveHashFn
{
	unsigned operator ()(const char* key) const { return atHash_const_charU(key); }
};

// PURPOSE: Specialized hash function functor for u64 keys.
template <>
struct atMapHashFn<u64>
{
	unsigned operator ()(u64 key) const { return (atHash64(key)); }
};

// PURPOSE: Default equality test functor, just calls == on the keys.
template <typename _T>
struct atMapEquals
{
	bool operator ()(const _T& left, const _T& right) const { return left == right; }
};

// PURPOSE: Specialized equality test functor for const char* keys.
template <>
struct atMapEquals<const char*>
{
	bool operator ()(const char * left, const char * right) const { return strcmp(left, right) == 0; }
};

// PURPOSE: Specialized equality test functor for atString keys.
template <> struct atMapEquals<atString> : public atMapEquals<const char*> { };

// PURPOSE: Specialized equality test functor for ConstString keys.
template <> struct atMapEquals<ConstString> : public atMapEquals<const char*> { };

// PURPOSE: Specialized equality test functor for UInt64 keys.
template <>
struct atMapEquals<u64>
{
	bool operator ()(const u64 left, const u64 right) const { return (left == right); }
};

// PURPOSE: Key value pair stored in the map's linked list hash buckets.
template <typename _Key, typename _Data>
struct atMapEntry {
	atMapEntry(const _Key &k,atMapEntry *n) : key(k), data(), next(n) { }
	atMapEntry(const _Key &k,const _Data &d,atMapEntry *n) : key(k), data(d), next(n) { }
	_Key key;
	_Data data;
	atMapEntry *next;
	typedef atMapEntry<_Key, _Data> _ThisType;

	enum ePlaceNoneInitializer {PLACE_NONE};
	enum ePlaceKeyInitializer {PLACE_KEY};
	enum ePlaceDataInitializer {PLACE_DATA};
	enum ePlaceKeyDataInitializer {PLACE_KEY_AND_DATA};

	atMapEntry(datResource& rsc, ePlaceNoneInitializer) {rsc.PointerFixup(next);}
	atMapEntry(datResource& rsc, ePlaceKeyInitializer) {key.Place(&key, rsc); rsc.PointerFixup(next);}
	atMapEntry(datResource& rsc, ePlaceDataInitializer) {data.Place(&data, rsc); rsc.PointerFixup(next);}
	atMapEntry(datResource& rsc, ePlaceKeyDataInitializer) {key.Place(&key, rsc), data.Place(&data, rsc); rsc.PointerFixup(next);}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT
};

// PURPOSE: Default memory management class - uses dynamic memory.
template <typename _Key, typename _Data>
class atMapMemory
{
	typedef atMapEntry<_Key, _Data> _EntryType;

public:
	_EntryType* Allocate(const _Key &k, _EntryType *n)
	{ 
		ATL_MAP_FUNC();
		return rage_new _EntryType(k,n); 
	}

	_EntryType* Allocate(const _Key &k,const _Data &d,_EntryType *n) 
	{ 
		ATL_MAP_FUNC();
		return rage_new _EntryType(k,d,n); 
	}

	void DeAllocate(_EntryType* ptr)
	{ 
		delete ptr; 
	}

	// PURPOSE: Allocate hash table (which is an array of pointers to the hash table entries)
	// The pointer to the hash table is passed in as a reference (to be assigned the new hash table location)
	// in order to avoid template resolution issues...
	void AllocateHash(int slotCount, _EntryType**& hash)
	{
		hash = rage_new _EntryType*[slotCount];
	}

	void DeAllocateHash(_EntryType** hash)
	{ 
		delete [] hash; 
	}
};

// PURPOSE: Specialized equality test functor for case-insenitive maps.
struct atMapCaseInsensitiveEquals
{
	bool operator ()(const char * left, const char * right) const { return stricmp(left, right) == 0; }
};

/*
PURPOSE:
	atMap class uses a hash to associate an arbitrary key with arbitrary data.
	You may specify the hash function and equality functors as template arguments, or rely on
	the defaults, in which case the key needs to have an appropriate atHash function defined for it
	(inlined if possible) and the operator== defined on it.

	We supply default implementations for unsigned and void* and atString.  Since atString
	has a conversion constructor for const char*, this also implies that C-style const char*
	is okay as well (admittedly with some extra overhead, but there's no real other option
	without reverting to callback functions for compares, etc).

	The hash table is always prime in size, and limited to about 65,000 entries.  We don't
	do any dynamic memory allocation until the first object is actually inserted into the
	table.  The table will grow as necessary to keep the hash chains short.  The chain nodes
	are dynamically reparented during a grow operation to minimize the amount of dynamic
	memory allocation -- only the toplevel hash slot array actually gets recreated.  We re-hash
	the table whenever the number of items in the table reachs the number of entries in the
	primary hash chain, so the average search chain length should be close to one item.

REMARKS
	There are two ways iterate over all entries in the table.  

	Here's an example of the first way:

	<CODE>
	atMap<_Key, _Data>::Iterator entry = map.CreateIterator();
	for (entry.Start(); !entry.AtEnd(); entry.Next())
	{
		entry.GetData(); /// do something with it
	}
	</CODE>

	Here's an example of the second way:

	<CODE>
	for (int i=0; i<map.GetNumSlots(); i++) {
		Entry *e = map.GetEntry(i);
		while (e) {
			// do something with e->key and/or e->data
			....
			e = e->next;
		}
	}
	</CODE>

PARAMETERS:
	_Key - The type of the key that the hash value is created from
	_Data - the type of the data that is accessed with the key
<FLAG Component>
*/
template <class _Key,class _Data,class _Hash = atMapHashFn<_Key>,class _Equals = atMapEquals<_Key>, class _MemoryPolicy = atMapMemory< _Key, _Data> >
class atMap {
public:
	typedef atMapEntry<_Key, _Data> Entry;
	typedef _Key KeyType;
	typedef _Data DataType;

	// PURPOSE: Default constructor
	atMap(const _Hash& hashFn = _Hash(), const _Equals& equals = _Equals(), const _MemoryPolicy& memory = _MemoryPolicy()) :
		m_HashFn(hashFn),
		m_Equals(equals),
		m_Memory(memory)
	{
		m_Slots = m_Used = 0;
		m_Hash = 0;
		m_AllowReCompute = false;
	}

	// PURPOSE: Copy constructor
	atMap(const atMap &that) {
		CopyFrom(that);
	}

	// PURPOSE: Assigment operator
	atMap& operator=(const atMap &that) {
		Kill();
		CopyFrom(that);
		return *this;
	}

	// PURPOSE: Resource constructor.
	// NOTES: Inlined below.
	inline atMap(datResource &rsc,
			void (*fixupKeyFunc)(datResource &rsc, _Key &key) = NULL,
			void (*fixupDataFunc)(datResource &rsc, _Data &data) = NULL);

	// PURPOSE: Destructor
	~atMap() {
		Kill();
	}

	// PURPOSE: Free all storage associated with the atMap object
	void Kill() {
		for (unsigned i=0; i<m_Slots; i++) {
			Entry *n = m_Hash[i];
			while (n) {
				Entry *m = n;
				n = n->next;
				m_Memory.DeAllocate(m);
			}
		}
		m_Memory.DeAllocateHash(m_Hash);
		m_Hash = 0;
		m_Slots = m_Used = 0;
	}

	// PURPOSE: Free all storage associated with the atMap object except the hash table buffer
	void Reset() {
		unsigned slotCount = m_Slots;
		for (unsigned i=0; i<slotCount; i++) {
			Entry *n = m_Hash[i];
			while (n) {
				Entry *m = n;
				n = n->next;
				m_Memory.DeAllocate(m);
			}
		}
		
		QuickReset();
	}

	// PURPOSE: Reset the slots. DO NOT USE THIS unless you know what you're doing - it will
	// not free up any memory allocated by the slots.
	void QuickReset() {
		sysMemSet(m_Hash, 0, m_Slots * sizeof(Entry *));
		m_Used = 0;
	}

	// PURPOSE: Given a key, return pointer to its data
	// PARAMS: key - Key to search table for
	// RETURNS: Pointer to associated data object, or null if not found
	template <typename _AltKey>
	_Data* Access(const _AltKey &key) {
		if (!m_Slots) return 0;
		Entry *n = m_Hash[m_HashFn(key) % m_Slots];
		while (n) {
			if (m_Equals(key, n->key)) {
				return &n->data;
			}
			n = n->next;
		}
		return 0;
	}

	// PURPOSE: Given a key, return pointer to its data
	// PARAMS: key - Key to search table for
	// RETURNS: Pointer to associated data object, or null if not found
	template <typename _AltKey>
	const _Data* Access(const _AltKey &key) const {
		if (!m_Slots) return 0;
		const Entry *n = m_Hash[m_HashFn(key) % m_Slots];
		while (n) {
			if (m_Equals(key, n->key)) {
				return &n->data;
			}
			n = n->next;
		}
		return 0;
	}

	// PURPOSE: Insert a new object into table
	// PARAMS: key - Key to insert
	//		data - data to associate with key
	// NOTES: Assumes caller has already checked result
	// of Access to avoid extra checking here
	template <typename _AltKey>
	const Entry& Insert(const _AltKey &key,const _Data &data) {
		if (!m_Slots)
			Create();
		if (++m_Used == m_Slots)
			Recompute();
		unsigned h = m_HashFn(key) % m_Slots; //lint !e414 possible division by zero
		m_Hash[h] = m_Memory.Allocate(key,data,m_Hash[h]);//GCC compiler needs a hint
		return *m_Hash[h];
	}

	// PURPOSE: Insert a new object into table
	// PARAMS: key - Key to insert
	// NOTES: Assumes caller has already checked result
	// of Access to avoid extra checking here
	template <typename _AltKey>
	Entry& Insert(const _AltKey &key) {
		if (!m_Slots)
			Create();
		if (++m_Used == m_Slots)
			Recompute();
		unsigned h = m_HashFn(key) % m_Slots; //lint !e414 possible division by zero
		m_Hash[h] = m_Memory.Allocate(key,m_Hash[h]);//GCC compiler needs a hint
		return *m_Hash[h];
	}

	// PURPOSE: Allow associative-array access to table
	// PARAMS: key - key to search on
	// RETURNS: Reference to associated data object
	// NOTES: Always succeeds; will allocate a new entry
	//	if key didn't already exist
	template <typename _AltKey>
	_Data& operator[] (const _AltKey &key) {
		if (!m_Slots)
			Create();
		unsigned h = m_HashFn(key) % m_Slots; //lint !e414 possible division by zero
		Entry *n = m_Hash[h];
		while (n) {
			if (m_Equals(key, n->key))
				return n->data;
			n = n->next;
		}
		if (++m_Used == m_Slots) {
			Recompute();
			h = m_HashFn(key) % m_Slots; //lint !e414 possible division by zero
		}
		m_Hash[h] = m_Memory.Allocate(key,m_Hash[h]);//GCC compiler needs a hint
		return m_Hash[h]->data;
	}

	// PURPOSE: Delete entry from table
	// PARAMS: key - key to search on
	// RETURNS: true if delete was successful
	// NOTES: If key is not found, nothing happens
	template <typename _AltKey>
	bool Delete(const _AltKey &key) {
		if (m_Slots<=0)
			return false;

		Entry **n = &m_Hash[m_HashFn(key) % m_Slots]; //lint !e414 possible division by zero
		while (*n) {
			if (m_Equals(key, (*n)->key)) {
				Entry *m = *n;
				*n = (*n)->next;
					m_Memory.DeAllocate(m);//GCC compiler needs a hint
				--m_Used;
				return true;
			}
			n = &(*n)->next;
		}
		return false;
	}

	// PURPOSE: Return number of slots in toplevel table
	// RETURNS: Number of slots in table
	int GetNumSlots() const { return m_Slots; }

	// PURPOSE: Returns number of items in table
	// RETURNS: Number of items used in table
	int GetNumUsed() const { return m_Used; }

	// PURPOSE: Returns first entry in specified hash chain
	// PARAMS: i - Zero-based index, should be between 0 and GetNumSlots-1
	// RETURNS: First entry in specified hash chain
	Entry *GetEntry(int i) { FastAssert(i>=0 && i<m_Slots); return m_Hash[i]; }

	// PURPOSE: Returns first entry in specified hash chain
	// PARAMS: i - Zero-based index, should be between 0 and GetNumSlots-1
	// RETURNS: First entry in specified hash chain as a const pointer
	const Entry *GetEntry(int i) const { FastAssert(i>=0 && i<m_Slots); return m_Hash[i]; }

	// PURPOSE: Set allow recompute on the map.
	void SetAllowRecompute(bool allowRecompute) { m_AllowReCompute = allowRecompute; }
	// PURPOSE: Given a new size, rehash the table and all its entries
	// PARAMS: newSize - New size for table; always a return from atHashNextSize
	// NOTES: Only the toplevel hash array is reallocated; internal nodes are
	// just reparented as necessary to minimize fragmentation
	void Recompute() {
		if (m_AllowReCompute)
			RecomputeInternal(atHashNextSize(m_Slots));
	}
	void Recompute(unsigned short newSize) {
		if (m_AllowReCompute)
			RecomputeInternal(newSize);
	}
protected:
	void RecomputeInternal(unsigned short newSize) {
		ATL_MAP_FUNC();
		Entry **newHash;
		m_Memory.AllocateHash(newSize, newHash);
		unsigned i;
		for (i=0; i<newSize; i++)
			newHash[i] = 0;
		for (i=0; i<m_Slots; i++) {
			Entry *n = m_Hash[i];
			while (n) {
				Entry *m = n;
				n = n->next;
				unsigned h = m_HashFn(m->key) % newSize;
				m->next = newHash[h];
				newHash[h] = m;
			}
		}
		
		m_Slots = newSize;
		m_Memory.DeAllocateHash(m_Hash);
		m_Hash = newHash;
	}
public:

	// PURPOSE: Class to encapsulate linear iteration through all inserted items in map
	class Iterator {
	private:
		friend class atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>;

		int hashpos;
		Entry* n;
		atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>* map;

		Iterator(atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>& m) : hashpos(0), n(0), map(&m) { Start(); }

	public:
		// PURPOSE: Reset the iterator to the first entry of the map
		void Start() {
			int localHashPos;
			int slotCount = map->GetNumSlots();
			for (localHashPos = 0; localHashPos < slotCount; localHashPos++) {
				n = map->GetEntry(localHashPos);
				if(n) break;
			}

			hashpos = localHashPos;
		}

		// PURPOSE: Advance the iterator to the next entry in the map
		void Next() {
			if(AtEnd()) return;
			n = n->next;
			int localHashPos = hashpos;
			const int numSlots = map->GetNumSlots();
			while (!n && (localHashPos + 1) < numSlots) {
				localHashPos++;
				n = map->GetEntry(localHashPos);
			}

			hashpos = localHashPos;
		}

		// PURPOSE: Check if the iteration is complete
		// RETURNS: True if there are no more entries for the iterator to advance to
		bool AtEnd() const { return !n; }

		// PURPOSE: Access the key and data of the current map entry
		// RETURNS: Reference to the key or the data the iterator is currently at
		_Key& GetKey() const { FastAssert(n); return n->key; }
		_Data& GetData() const { FastAssert(n); return n->data; }
		_Data* GetDataPtr() const { FastAssert(n); return &n->data; }

		// PURPOSE: Operators to use the iterator as if it were a pointer to the type of data mapped
		// RETURNS: Reference or pointer to template type "_Data"
		_Data& operator * () const { return GetData(); }
		_Data* operator -> () const { FastAssert(n); return &n->data; }

		// PURPOSE: Advance the iterator to the next entry in the map
		Iterator& operator ++ () { Next(); return *this; }

		// PURPOSE: Check if the iterator points to a valid entry
		// RETURNS: True if the iterator points to a valid entry, false if iteration is complete
		operator bool () const { return !AtEnd(); }
	};

	// PURPOSE: Class to encapsulate linear iteration through all inserted items in map, and to maintain const-only access to the items
	class ConstIterator {
	private:
		friend class atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>;

		int hashpos;
		const Entry* cn;
		const atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>* cmap;

		ConstIterator(const atMap<_Key,_Data,_Hash,_Equals,_MemoryPolicy>& m) : hashpos(0), cn(0), cmap(&m) { Start(); }

	public:
		// PURPOSE: Reset the iterator to the first entry of the map
		void Start() {
			for (hashpos = 0; hashpos < cmap->GetNumSlots(); hashpos++) {
				cn = cmap->GetEntry(hashpos);
				if(cn) break;
			}
		}

		// PURPOSE: Advance the iterator to the next entry in the map
		void Next() {
			if(AtEnd()) return;
			cn = cn->next;
			const int numSlots = cmap->GetNumSlots();
			while (!cn && (hashpos + 1) < numSlots) {
				hashpos++;
				cn = cmap->GetEntry(hashpos);
			}
		}

		// PURPOSE: Check if the iteration is complete
		// RETURNS: True if there are no more entries for the iterator to advance to
		bool AtEnd() const { return !cn; }

		// PURPOSE: Access the key and data of the current map entry
		// RETURNS: Reference to the key or the data the iterator is currently at
		const _Key& GetKey() const { FastAssert(cn); return cn->key; }
		const _Data& GetData() const { FastAssert(cn); return cn->data; }
		const _Data* GetDataPtr() const { FastAssert(cn); return &cn->data; }

		// PURPOSE: Operators to use the iterator as if it were a pointer to the type of data mapped
		// RETURNS: Reference or pointer to template type "_Data"
		const _Data& operator * () const { return GetData(); }
#if defined(_MSC_VER) && _MSC_VER > 1200
		const _Data* operator -> () const { FastAssert(cn); return &cn->data; }
#endif // #if defined(_MSC_VER) && _MSC_VER > 1200

		// PURPOSE: Advance the iterator to the next entry in the map
		ConstIterator& operator ++ () { Next(); return *this; }

		// PURPOSE: Check if the iterator points to a valid entry
		// RETURNS: True if the iterator points to a valid entry, false if iteration is complete
		operator bool () const { return !AtEnd(); }
	};

	// PURPOSE: Create an iterator to search through the map linearly
	// RETURNS: An Iterator object pointing at the first entry in the map
	Iterator CreateIterator() { return Iterator(*this); }

	// PURPOSE: Create a const iterator to search through the map linearly
	// RETURNS: A ConstIterator object pointing at the first entry in the map
	ConstIterator CreateIterator() const { return ConstIterator(*this); }
	
	// THIS IS FOR RESOURCING CONVENIENCE AND MUST BE USED WITH CARE (i.e. Prime number of slots, etc)
	// PURPOSE: Create and initialize the top level hash array
		// NOTES: 
	void Create( unsigned short slotCount = 10, bool allowReCompute = true) {
		RAGE_TRACK(atMap);
		ATL_MAP_FUNC();

#if ENABLE_PRIME_DEBUG
		if (!allowReCompute)
			IncrementSlots(slotCount, sizeof(Entry));
#endif
		slotCount = atHashNextSize(slotCount);	//Get out next valid prime size
		m_Memory.AllocateHash(m_Slots = slotCount, m_Hash);
		for (unsigned i=0; i<m_Slots; i++)
		{
			m_Hash[i] = 0;
		}
		m_AllowReCompute = allowReCompute;
	}

	// PURPOSE: Create and initialize the top level hash array
	// NOTES: slotCount MUST be prime, otherwise an assert will occur
	void CreatePrimed(unsigned short slotCount)
	{
		Assertf(IsPrime(slotCount), "atMap::CreatePrimed requires a prime number. %d is NOT prime!", slotCount);

		RAGE_TRACK(atMap);
		ATL_MAP_FUNC();

#if ENABLE_PRIME_DEBUG
		IncrementSlots(slotCount, sizeof(Entry));
#endif
		m_Memory.AllocateHash(m_Slots = slotCount, m_Hash);
		for (unsigned i=0; i<m_Slots; i++)
		{
			m_Hash[i] = 0;
		}
		m_AllowReCompute = false;
	}

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

private:
	// PURPOSE: Copy the data from another map.
	// **NOTE** that this will NOT delete any previous content in this
	// map and would lead to memory leaks if you call it on a map
	// that is already populated. Technically, this should be a private function.
	void CopyFrom(const atMap &that) {
		// Make a carbon-copy of the source map.
		m_Slots = that.m_Slots;
		m_Used = that.m_Used;

		ATL_MAP_FUNC();
		m_Memory.AllocateHash(m_Slots, m_Hash);

		for (int i=0; i<that.m_Slots; i++) {
			Entry *e = that.m_Hash[i];
			m_Hash[i] = NULL;
			Entry **dstE = &m_Hash[i];

			while (e) {
				*dstE = m_Memory.Allocate(e->key, e->data, NULL);//GCC compiler needs a hint
				dstE = &(*dstE)->next;
				e = e->next;
			}
		}

		m_HashFn = that.m_HashFn;
		m_Equals = that.m_Equals;
		m_Memory = that.m_Memory;
		m_AllowReCompute = false;
	}

	Entry **m_Hash;				// Storage for toplevel hash array
	unsigned short m_Slots,		// Number of slots in toplevel hash
				m_Used;			// Number of those slots currently in use
	_Hash m_HashFn;				// Hash functor
	_Equals m_Equals;			// Equality functor
	_MemoryPolicy m_Memory;		// Memory functor
	unsigned char m_AllowReCompute;	//Make sure we can't do dynamic memory allocation (good for resourced maps and general dynamic memory use removal)
};



/*
PURPOSE
	This constructor can be called to fix up the pointers of an atMap
	when loading from a resource.
PARAMS
	rsc				- The datResource object.
	fixupKeyFunc	- A function that fixes up the keys of the map, or NULL.
	fixupDataFunc	- A function that fixes up the data in the map, or NULL.
*/
template <class _Key, class _Data, class _Hash, class _Equals, class _MemoryPolicy> atMap<_Key, _Data, _Hash, _Equals, _MemoryPolicy>::atMap(datResource &rsc,
		void (*fixupKeyFunc)(datResource &rsc, _Key &key),
		void (*fixupDataFunc)(datResource &rsc, _Data &data)) : 
		m_HashFn(_Hash()), 
		m_Equals(_Equals())
{
	unsigned i;
	rsc.PointerFixup(m_Hash);
	for(i = 0; i < m_Slots; i++)
	{
		rsc.PointerFixup(m_Hash[i]);
		Entry *node = m_Hash[i];
		while(node)
		{
			rsc.PointerFixup(node->next);
			if(fixupKeyFunc)
				fixupKeyFunc(rsc, node->key);
			if(fixupDataFunc)
				fixupDataFunc(rsc, node->data);
			node = node->next;
		}
	}
}


/*
PURPOSE:
Allows user created pools to throw their own exceptions on an allocation failure.
*/
struct atMapPoolAllocFailPolicy
{
	unsigned operator () () { Quitf(ERR_GEN_MAP,"atMap memory pool out of entries!"); return 0; };
};

/*
PURPOSE:
atMapMemoryPool class to make it a little easier to setup pooled memory systems for the atMap class. The pool class
has small footprint and contains a functor class that is the actual hookup into the pool. This allows multiple maps to
share the same memory pool.

USe atSimplePooledMapType to simplify notation in the usual usage patterns.

The funkyest part of this is that the atMap template takes a set of defaulted types so I had to add
the memory policy class as the last parameter meaning that you have to define all the preexisting options :(

The second funkyest part is that you pass the functor into the map constructor rather than the pool itself.

When trying to eliminate dynamic memory allocations using this pool class it is important to lock the hash table
size by invoking Create on the map itself manually. Without this the map is free to resize its entry table.

Example for defining pool and map types and instantiating a map with an associated pool memory manager
<CODE>
	//typedefs to keep following lines a wee bit shorter!
	typedef atMapMemoryPool<ConstString, MyType*, 4> MyTypeMemoryPool;
	typedef atMap<ConstString, MyType*, atMapHashFn<ConstString>, atMapEquals<ConstString>, MyTypeMemoryPool::MapFunctor > MyType_Map;
	typedef MyType_Map::Entry MyType_MapEntry;

	//Setup map and pool with relation use the functor object to tie them together.
	MyTypeMemoryPool pool;
	MyType_Map map(atMapHashFn<ConstString>(),atMapEquals<ConstString>(), pool.m_Functor)

	//Actually allocate the memory
	pool.Create<MyType_MapEntry>(16);	//The pool is created with the relevant entry type.
	map.Create(16, false);	//Create our entry array and make sure it doesn't get resized to prevent dynamic access
</CODE>
*/
template<typename _Key, typename _Data, int AlignEntryType = 16, typename _AllocFailPolicy = atMapPoolAllocFailPolicy>
class atMapMemoryPool
{
private:
	//Maintain linked list of available pool slots.
	template<typename _EntryType>
	struct PoolItem
	{	union {	char sizeMember[sizeof(_EntryType)];
	PoolItem *pNext;
	} um;
	};

public:
	atMapMemoryPool()
		:m_iUsed(0)
		,m_Head(0)
		,m_PoolBase(0)
	{	m_Functor.Setup(this);	}

	~atMapMemoryPool()
	{	Destroy();	}

	/*	PURPOSE:	Creates the pool memory for the given map entry type. Uses the current
	allocator directly. The map entry type is used to gain size and alignment information.
	*/
	template <typename _EntryType>
	void Create(const int iCount)
	{
		RAGE_TRACK(atMapMemoryPool);

		m_PoolBase = (void*) rage_aligned_new(__alignof(_EntryType)) char[sizeof(_EntryType) * iCount];
		QuickResetPool<_EntryType>(iCount);
	}

	/* PURPOSE:	Reset the pool to mark every element as available. You normally don't want to
	 * call this directly.
	 */
	template <typename _EntryType>
	void QuickResetPool(const int iCount)
	{
		PoolItem<_EntryType> *pBase = (PoolItem<_EntryType> *)m_PoolBase;
		m_Head = m_PoolBase;
		for (int i=0 ; i<iCount-1 ; i++)
		{
			pBase[i].um.pNext = &pBase[i+1];
		}
		pBase[iCount-1].um.pNext = 0;
	}

	/*	PURPOSE: Releases the pool memory to the current allocator
	*/
	void Destroy()
	{
		if (m_PoolBase)
		{
			AssertMsg(!m_iUsed ,"Destroying atMapMemoryPool while items are allocated. Will not call destructors.");

			delete[] (char*) m_PoolBase;
		}
		m_PoolBase = m_Head = 0;
	}

	/*	PURPOSE: Allow code to keep tabs on amount of pool used
	*/
	int GetUsedCount() const
	{
		return m_iUsed;
	}

	/*  PURPOSE: Return true if there are no more entries available.
	*/
	bool IsFull() const
	{
		return m_Head == NULL;
	}

	//The map functor allows multiple maps to share the same pool. Each map makes a copy
	//of the functor internally that gives it access back to the parent pool.
	class MapFunctor
	{
		atMapMemoryPool<_Key, _Data, AlignEntryType, _AllocFailPolicy> *mp_Pool;
	public:
		MapFunctor()
			:mp_Pool(0)
		{	}
		MapFunctor(const MapFunctor& rOther)
			:mp_Pool(rOther.mp_Pool)
		{	}
		void Setup(atMapMemoryPool<_Key, _Data, AlignEntryType, _AllocFailPolicy> *pPool)
		{
			mp_Pool = pPool;
		}

		template <typename _EntryType>
		_EntryType* Allocate(const _Key &k, _EntryType *n) 
		{
			if (mp_Pool)
			{
				return mp_Pool->Allocate<_EntryType>(k,n);
			}
			return 0;
		}

		template <typename _EntryType>
		_EntryType* Allocate(const _Key &k,const _Data &d,_EntryType *n) 
		{ 
			if (mp_Pool)
			{
				return mp_Pool->Allocate<_EntryType>(k,d,n);
			}
			return 0;
		}

		template <typename _EntryType>
		void DeAllocate(_EntryType* ptr) 
		{
			if (mp_Pool)
			{
				return mp_Pool->DeAllocate<_EntryType>(ptr);
			}
		}

		template <typename _EntryType>
		void AllocateHash(int slotCount, _EntryType**& hash)
		{
			hash = (_EntryType**) rage_new _EntryType*[slotCount];
		}

		template <typename _EntryType>
		void DeAllocateHash(_EntryType** hash)
		{ 
			delete [] hash; 
		}

	};

	//Functor object passed into map c'tor
	MapFunctor m_Functor;

private:
	//friend MapFunctor;

	template <typename _EntryType>
	_EntryType* Allocate(const _Key &k, _EntryType *n) 
	{ 
		void *pMem = GetFromPool<_EntryType>();
		if (pMem)
		{
			::new (pMem)_EntryType(k,n); 
			return (_EntryType*)pMem;
		}
		else
		{
			_AllocFailPolicy()();
		}
		return 0;
	}
	template <typename _EntryType>
	_EntryType* Allocate(const _Key &k,const _Data &d,_EntryType *n) 
	{ 
		void *pMem = GetFromPool<_EntryType>();
		if (pMem)
		{
			::new (pMem)_EntryType(k,d,n); 
			return (_EntryType*)pMem;
		}
		else
		{
			_AllocFailPolicy()();
		}
		return 0;
	}

	template <typename _EntryType>
	void DeAllocate(_EntryType* ptr) 
	{ 
		ptr->~_EntryType();
		ReturnToPool<_EntryType>(ptr);
	}


	template<typename _EntryType>
	void* GetFromPool()
	{
		PoolItem<_EntryType> *pHead = (PoolItem<_EntryType>*)m_Head;
		if (pHead)
		{
			m_Head = pHead->um.pNext;
			++m_iUsed;
		}
		return pHead;
	}

	template<typename _EntryType>
	void ReturnToPool(_EntryType *pType)
	{
		//Assumes relevant deletion code is already called
		PoolItem<_EntryType> *pPoolItem = (PoolItem<_EntryType> *)pType;
		PoolItem<_EntryType> *pPoolHead = (PoolItem<_EntryType> *)m_Head;
		pPoolItem->um.pNext = pPoolHead;
		m_Head = pPoolItem;
		--m_iUsed;
	}
	int m_iUsed;
	void *m_Head;
	void *m_PoolBase;
};

/*
PURPOSE:
 Helper class to setup a pooled map for simple types. Added concrete functions for simple paired pool/map
 member variables or just typedef the class use to automate some of the complication defined above and use
 the internal Pool and Map types.

A simple paired pool and map might look like this
<CODE>
	//Typedef the combo
	atSimplePooledMapType<Constring, MyType*, 4> pooledMap;

	//Actually allocate the memory
	pooledMap.Init(16);	//Pass in pool size.
	pooledMap.m_Map.Insert( someData );
</CODE>

If we wanted to share a pool with many maps we can still use the the template to simplify previously laborious typedefs
<CODE>

	//Instead of:
	typedef atMapMemoryPool<ConstString, MyType*, 4> MyTypeMemoryPool;
	typedef atMap<ConstString, MyType*, atMapHashFn<ConstString>, atMapEquals<ConstString>, MyTypeMemoryPool::MapFunctor > MyType_Map;
	typedef MyType_Map::Entry MyType_MapEntry;

	//Typedef the combo
	typedef atSimplePooledMapType<Constring, MyType*, 4> TPooledMap;
	
	//Now we can use:
	//TPooledMap::Pool
	//TPooledMap::Map
	//TPooledMap::Map::Entry
</CODE>
*/

template <typename key, typename data, int AlignEntryType = 16, typename _AllocFailPolicy = atMapPoolAllocFailPolicy, class hashFn = atMapHashFn<key>, class equalsFn = atMapEquals<key>  >
struct atSimplePooledMapType
{
	typedef atMapMemoryPool<key, data, AlignEntryType, _AllocFailPolicy> Pool;
	typedef atMap<key, data, hashFn, equalsFn, typename Pool::MapFunctor > Map;
	typedef typename Map::Iterator Iterator;
	typedef typename Map::ConstIterator ConstIterator;

	Pool m_Pool;
	Map m_Map;
	int m_Size;		// Not the same as m_Map.m_Slots, the latter will be bumped up to the next prime

	atSimplePooledMapType()
		:m_Map(hashFn(),equalsFn(), m_Pool.m_Functor)
	{
	}

	atSimplePooledMapType(u16 iSize)
		:m_Map(hashFn(),equalsFn(), m_Pool.m_Functor)
	{
		Init(iSize);
	}

	void Init(u16 iPoolSize)
	{
		m_Size = iPoolSize;
		m_Pool.template Create<typename Map::Entry>(iPoolSize);
		m_Map.Create(iPoolSize, false);	//Create our entry array and make sure it doesn't get resized to prevent dynamic access
	}

	void InitPrimed(u16 iPoolSize)
	{
		m_Size = iPoolSize;
		m_Pool.template Create<typename Map::Entry>(iPoolSize);
		m_Map.CreatePrimed(iPoolSize);	//Create our entry array and make sure it doesn't get resized to prevent dynamic access
	}

	void DeInit()
	{
		m_Map.Kill();
		m_Pool.Destroy();
	}

	// compatibility functions to make migration from an atMap to an atSimplePooledMapType as painless as possible

	template <typename _AltKey>
	data* Access(const _AltKey &thekey) {
		return m_Map.Access(thekey);
	}
	
	template <typename _AltKey>
	const data* Access(const _AltKey &thekey) const {
		return m_Map.Access(thekey);
	}

	template <typename _AltKey>
	bool Delete(const _AltKey &thekey) {
		return m_Map.Delete(thekey);
	}

	template <typename _AltKey>
	const typename Map::Entry& Insert(const _AltKey &thekey,const data &thedata) {
#if !__FINAL
		if (m_Map.GetNumSlots() == m_Map.GetNumUsed()) {
			Quitf("Map too large (%d entries) - need to resize.", m_Map.GetNumSlots());
		}
#endif // !__FINAL
		return m_Map.Insert(thekey,thedata);
	}

	template <typename _AltKey>
	typename Map::Entry& Insert(const _AltKey &thekey) {
		// this will quitf at a lower level if the memory allocation policy allocator is full
		return m_Map.Insert(thekey);
	}

	Iterator CreateIterator() { return m_Map.CreateIterator(); }
	ConstIterator CreateIterator() const { return m_Map.CreateIterator(); }

	void Reset() {
		m_Map.Reset();
	}

	void QuickReset() {
		m_Map.QuickReset();
		m_Pool.template QuickResetPool<typename Map::Entry>(m_Size);
	}

	bool IsFull() const {
		return m_Pool.IsFull();
	}
};

}	// namespace rage

#endif
