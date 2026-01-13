//
// atl/binmap.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_BINMAP_H
#define ATL_BINMAP_H

#include <algorithm>

#include "system/typeinfo.h"

#include "atl/array.h"
#include "string/stringhash.h"

namespace rage {

// PURPOSE: Represent a map from string to arbitrary data, with low memory overhead and fast searching
// Uses a hash function to convert strings into key values which are stored and searched on.
// NOTES:
//		The atBinaryMap can be in one of two states - Sorted and non-Sorted. Normally you would Insert()
//		a bunch of items into the list, then call FinishInsertion(). This sorts the list and makes searches
//		O(log(N)). If the list is unsorted the searches are O(N).
// PARAMS:
//	_T - the data that is indexed with the string
//  _KeyType - the internal storage type for the key value. This type must be ordered.
//   in particular, _KeyType must have valid operators <, =, and ==.
template <class _T, class _KeyType> class atBinaryMap {
public:

	class Iterator
	{
	public:
		// PURPOSE: default ctor (for End)
		Iterator(): Map(NULL), CurIdx(0) {}
		// PURPOSE: ctor for Begin
		explicit Iterator(atBinaryMap<_T,_KeyType>& themap):Map(&themap), CurIdx(0) {}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		Iterator& operator++()
		{
			if(++CurIdx>=Map->Data.GetCount())
				*this = Map->End();
			return *this;
		}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		Iterator operator++(const int)
		{
			Iterator result(*this);
			++(*this);
			return result;
		}

		// PURPOSE: accesses the active map element that this iterator refers to
		_T& operator*() const
		{
			FastAssert(Map);
			AssertMsg(CurIdx < Map->Data.GetCount() , "atBinaryMap Iterator is past end, cannot dereference");
			return Map->Data[CurIdx].data;
		}

		_T* operator->() const
		{
			FastAssert(Map);
			AssertMsg(CurIdx < Map->Data.GetCount() , "atBinaryMap Iterator is past end, cannot dereference");
			return &Map->Data[CurIdx].data;
		}

		// PURPOSE: checks to see if this iterator points to the same element as another
		bool operator==(const Iterator& comp) const
		{
			return (CurIdx == comp.CurIdx) && (Map == comp.Map);
		}

		// PURPOSE: checks to see if this iterator does not refer to the same element as another
		bool operator!=(const Iterator& comp) const
		{
			return !((CurIdx == comp.CurIdx) && (Map == comp.Map));
		}

		// PURPOSE: 
		Iterator& operator=(const Iterator& rhs)
		{
			Map = rhs.Map;
			CurIdx = rhs.CurIdx;
			return *this;
		}

		const _KeyType& GetKey() { FastAssert(Map); return Map->Data[CurIdx].key; }

	private:
		atBinaryMap<_T, _KeyType> * Map;
		int CurIdx;
	}; /******************* END DEFINITION atBinaryMap::Iterator ******************/

	class ConstIterator
	{
	public:
		// PURPOSE: default ctor (for End)
		ConstIterator(): Map(NULL), CurIdx(0) {}

		// PURPOSE: ctor for Begin
		explicit ConstIterator(const atBinaryMap<_T,_KeyType>& themap):Map(&themap), CurIdx(0) {}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		ConstIterator& operator++()
		{
			if(++CurIdx>=Map->Data.GetCount())
				*this = Map->end();
			return *this;
		}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		ConstIterator operator++(const int)
		{
			ConstIterator result(*this);
			++(*this);
			return result;
		}

		// PURPOSE: accesses the active map element that this iterator refers to
		const _T& operator*() const
		{
			FastAssert(Map);
			AssertMsg(CurIdx < Map->Data.GetCount() , "atBinaryMap Iterator is past end, cannot dereference");
			return Map->Data[CurIdx].data;
		}

		const _T* operator->() const
		{
			FastAssert(Map);
			AssertMsg(CurIdx < Map->Data.GetCount() , "atBinaryMap Iterator is past end, cannot dereference");
			return &Map->Data[CurIdx].data;
		}

		// PURPOSE: checks to see if this iterator points to the same element as another
		bool operator==(const ConstIterator& comp) const
		{
			return (CurIdx == comp.CurIdx) && (Map == comp.Map);
		}

		// PURPOSE: checks to see if this iterator does not refer to the same element as another
		bool operator!=(const ConstIterator& comp) const
		{
			return !((CurIdx == comp.CurIdx) && (Map == comp.Map));
		}

		// PURPOSE: 
		ConstIterator& operator=(const ConstIterator& rhs)
		{
			Map = rhs.Map;
			CurIdx = rhs.CurIdx;
			return *this;
		}

		const _KeyType& GetKey() { FastAssert(Map); return Map->Data[CurIdx].key; }

	private:
		const atBinaryMap<_T, _KeyType>* Map;
		int CurIdx;
	}; /******************* END DEFINITION atBinaryMap::Iterator ******************/

	friend class Iterator;
	friend class ConstIterator;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: returns an iterator that points to the beginning of Data
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Iterator Begin()
	{
		if(Data.GetCount()>0)
			return Iterator(*this);
		else return End();
	}

	// Lowercase begin so that we can use the C++0x11 foreach style loops.
	Iterator begin()
	{
		if(Data.GetCount()>0)
			return Iterator(*this);
		else return End();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// const version
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ConstIterator begin() const
	{
		if(Data.GetCount()>0)
			return ConstIterator(*this);
		else return end();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: returns an iterator that points past the end of Data,
	// used to test if an iterator is still valid.
	static Iterator End()
	{
		return Iterator();
	}

	// Lowercase end so that we can use the C++0x11 foreach style loops.
	Iterator end()
	{
		return Iterator();
	}

	// Lowercase end so that we can use the C++0x11 foreach style loops.
	ConstIterator end() const
	{
		return ConstIterator();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty binary map	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atBinaryMap() {
		Sorted = true;   //Empty list is sorted
	}

	enum ePlaceNoneInitializer {PLACE_NONE};
	enum ePlaceKeyInitializer {PLACE_KEY};
	enum ePlaceDataInitializer {PLACE_DATA};
	enum ePlaceKeyDataInitializer {PLACE_KEY_AND_DATA};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Resource constructor
	// NOTES: Pass in one of the PLACE_ enums to define which types (key or data) you want to run resource constructors for
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename _Placer> atBinaryMap(class datResource& rsc, _Placer p)
		: Data(rsc) // NOTE: not calling (rsc,1) because we don't want to fix up all the elements
	{
		for(int i = 0; i < Data.GetCount(); i++)
		{
			Data[i].Place(&Data[i], rsc, p);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Reset() {
		Data.Reset();
		Sorted = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map but doesn't delete allocated memory
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void ResetCount() {
		Data.ResetCount();
		Sorted = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map but doesn't delete allocated memory
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool IsSorted() const {
		return Sorted;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Resizes the data array so that future additions will be fast
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Reserve(int size) {
		Data.Reserve(size);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key - no checking!
// This is O(1) (not counting array resizing time)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void FastInsert(const _KeyType& key, const _T& data) {
		Sorted = false;
		DataPair& d = Data.Grow();
		d.key = key;
		d.data = data;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key and return a pointer to the new data - no checking!
// This is O(1) (not counting array resizing time)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T *FastInsert(const _KeyType& key) {
		Sorted = false;
		DataPair& d = Data.Grow();
		d.key = key;
		return &d.data;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key
// In __DEV builds this will assert if that string key is already in use.
// This is O(N) in dev builds and O(1) in release (not counting array resizing time)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Insert(const _KeyType& key, const _T& data) {
		Assertf(!Has(key), "atBinaryMap already contains key with value %d", (int)key);
		Sorted = false;
		DataPair& d = Data.Grow();
		d.key = key;
		d.data = data;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key and return a pointer to the new data
// In __DEV builds this will assert if that string key is already in use.
// This is O(N) in dev builds and O(1) in release (not counting array resizing time)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T *Insert(const _KeyType& key) {
		Assertf(!Has(key), "atBinaryMap already contains key with value %d", (int)key);
		Sorted = false;
		DataPair& d = Data.Grow();
		d.key = key;
		return &d.data;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key
// This will return false if that string key is already in use.
// This is always O(N)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SafeInsert(const _KeyType& key, const _T& data) {
		if (Has(key)) {
			return false;
		}
		Sorted = false;
		DataPair& d = Data.Grow();
		d.key = key;
		d.data = data;
		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Remove the ith element from the map
// This is O(N) if the list is sorted, O(1) if it's not
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Remove(int i)
	{
		if (Sorted)
		{
			Data.Delete(i);
		}
		else
		{
			Data.DeleteFast(i);
		}
	}

	void RemoveKey(_KeyType key)
	{
		int index = GetIndex(key);
		if (index >= 0)
		{
			Remove(index);
		}
	}

	int GetIndex(_KeyType key) const
	{
		if (Sorted)
		{
			if (const _T* ptr = SafeGet(key))
				return GetIndexFromDataPtr(ptr);
		}
		else
		{
			for(int i = 0; i < Data.GetCount(); ++i)
			{
				if (Data[i].key == key) 
				{
					return i;
				}
			}
		}
		return -1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int GetCount() const
	{
		return Data.GetCount();
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* GetItem(int i)
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].data;
	}
	
	const _T* GetItem(int i) const
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].data;
	}

	_KeyType* GetKey(int i)
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].key;
	}
	
	const _KeyType* GetKey(int i) const
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].key;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Returns true if an item with the given string key exists in the map
// NOTES: If FinishInsertion has been called, this is O(log N), otherwise it's O(N)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool Has(const _KeyType& key) const {
		if (Sorted)
		{
			return SafeGet(key) != NULL;
		}
		else
		{
			for(int i = 0; i < Data.GetCount(); i++)
			{
				if (Data[i].key == key) {
					return true;
				}
			}
			return false;
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Gets the item with the given key, or NULL if no such item exists
// REQUIREMENTS: FinishInsertion() must be called before this is called
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* SafeGet(const _KeyType& key) {
		FastAssert(Sorted);
		if (Data.GetCount() > 0) {
#ifdef STLPORT
			const _KeyType& searchKey = key;
#else
			DataPair searchKey; searchKey.key = key;
#endif
			DataPair* ret = std::lower_bound(Data.begin(), Data.end(), searchKey, DataPair::SearchCompare);
			if (ret != Data.end() && ret->key == key) { // need to test because lower_bound finds the index of the smallest item greater than key
				return &(ret->data); 
			}
		}
		return NULL;
	}

	const _T* SafeGet(const _KeyType& key) const {
		FastAssert(Sorted);
		if (Data.GetCount() > 0) {
#ifdef STLPORT
			const _KeyType& searchKey = key;
#else
			DataPair searchKey; searchKey.key = key;
#endif
			const DataPair* ret = std::lower_bound(Data.begin(), Data.end(), searchKey, DataPair::SearchCompare);
			if (ret != Data.end() && ret->key == key) { // need to test because lower_bound finds the index of the smallest item greater than key
				return &(ret->data); 
			}
		}
		return NULL;
	}
	
	// synonyms for parity with regular atMap
	_T* Access(const _KeyType& key) {
		return SafeGet(key);
	}

	const _T* Access(const _KeyType& key) const {
		return SafeGet(key);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Gets the item with the given key. This functions is unsafe (but slightly faster) - it must exist!
// REQUIREMENTS: FinishInsertion() must be called before this is called
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* UnsafeGet(const _KeyType& key) {
		FastAssert(Sorted);
		DataPair* ret = reinterpret_cast<DataPair*>(bsearch(&key, &Data[0], (size_t)Data.GetCount(), sizeof(DataPair), //lint !e571 !e747
			(int(*)(const void*, const void*))DataPair::SearchCompare)); //lint !e571
		return &(ret->data); 
	}
	_T* UnsafeGet(const _KeyType& key) const {
		FastAssert(Sorted);
		DataPair* ret = reinterpret_cast<DataPair*>(bsearch(&key, &Data[0], (size_t)Data.GetCount(), sizeof(DataPair), //lint !e571 !e747
			(int(*)(const void*, const void*))DataPair::SearchCompare)); //lint !e571
		return &(ret->data);
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Finalzes map creation and prepares map for searching. 
// SafeGet and operator[] may not be used until this function is called
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void FinishInsertion() {
		if(Sorted) return;
		if (Data.GetCount() > 0) {
			std::sort(Data.begin(), Data.end(), DataPair::SortCompare);
		}
		Sorted = true;
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: mark the map as unsorted, use if you've done something dirty to a key.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Unsort() {
		Sorted = false;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: return the item with the given key. Item must exist in the map
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T& operator[](const _KeyType& key) {
		_T* ret = SafeGet(key);
		FastAssert(ret);
		return *ret;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_KeyType& GetKeyFromDataPtr(_T* dataPtr)
	{
		FastAssert((char*)dataPtr >= (char*)Data.GetElements() && (char*)dataPtr < (char*)(Data.GetElements() + Data.GetCount()));
		DataPair* pair = reinterpret_cast<DataPair*>( reinterpret_cast<char*>(dataPtr) - OffsetOf(DataPair, data) );
		return pair->key;
	}

	const _KeyType& GetKeyFromDataPtr(const _T* dataPtr) const
	{
		FastAssert((const char*)dataPtr >= (const char*)Data.GetElements() && (const char*)dataPtr < (const char*)(Data.GetElements() + Data.GetCount()));
		const DataPair* pair = reinterpret_cast<const DataPair*>( reinterpret_cast<const char*>(dataPtr) - OffsetOf(DataPair, data) );
		return pair->key;
	}

	_T& GetDataFromKeyPtr(_KeyType* keyPtr)
	{
		FastAssert((char*)keyPtr >= (char*)Data.GetElements() && (char*)keyPtr < (char*)(Data.GetElements() + Data.GetCount()));
		DataPair* pair = reinterpret_cast<DataPair*>( reinterpret_cast<char*>(keyPtr) - OffsetOf(DataPair, key) );
		return pair->data;
	}

	const _T& GetDataFromKeyPtr(const _KeyType* keyPtr) const
	{
		FastAssert((const char*)keyPtr >= (const char*)Data.GetElements() && (const char*)keyPtr < (const char*)(Data.GetElements() + Data.GetCount()));
		const DataPair* pair = reinterpret_cast<const DataPair*>( reinterpret_cast<const char*>(keyPtr) - OffsetOf(DataPair, key) );
		return pair->data;
	}

	int GetIndexFromDataPtr(const _T* dataPtr) const
	{
		FastAssert((char*)dataPtr >= (char*)Data.GetElements() && (char*)dataPtr < (char*)(Data.GetElements() + Data.GetCount()));
		const DataPair* pair = reinterpret_cast<const DataPair*>( reinterpret_cast<const char*>(dataPtr) - OffsetOf(DataPair, data) );
		return ptrdiff_t_to_int((((u8*)pair) - ((u8*)&Data[0])) / sizeof(DataPair));
	}
	
	struct DataPair {
		_KeyType key;
		_T data;

		DataPair() {}

		DataPair(datResource&, ePlaceNoneInitializer) {	}
		DataPair(datResource& rsc, ePlaceKeyInitializer) : key(rsc) {}
		DataPair(datResource& rsc, ePlaceDataInitializer) : data(rsc) {}
		DataPair(datResource& rsc, ePlaceKeyDataInitializer) : key(rsc), data(rsc) {}

		template<typename _Placer>
		static void Place(void *that,datResource &rsc, _Placer p) { ::new (that) DataPair(rsc, p); }

#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct& s);
#endif

#ifdef STLPORT
		// Note that this does not work with all STLs. Technically the comparison function need the same
		// type for both comparands, so we'd either have to construct a new DataPair object (which could
		// have unknown cost) or store the keys and values in separate lists. It so happens that in stlport's
		// lower_bound it always passes the query value as the 2nd argument to the comparison predicate so 
		// this function is OK. If we ever see it stop compiling we may have to go back to bsearch or do something
		// cleverer.
		static bool SearchCompare(const DataPair& data, const _KeyType& key) {
			return (data.key < key); 
		}
#else
		// Hopefully not too slow on non-STLport STLs.
		static bool SearchCompare(const DataPair& data, const DataPair& key) {
			return (data.key < key.key); 
		}
#endif
		static bool SortCompare(const DataPair& a, const DataPair& b) {
			return (a.key < b.key);
		}
	};

	atArray<DataPair>& GetRawDataArray() {return Data;}

#if !__SPU
protected:
#endif

	bool Sorted;
	char Pad[3];
	atArray<DataPair> Data; 
};

// PURPOSE: Represent a map from string to arbitrary data, with low memory overhead and fast searching
// Uses a hash function to convert strings into key values which are stored and searched on.
// PARAMS:
//	_T - the data that is indexed with the string
//  _HashedKeyType - the internal storage type for the key value. This type must be ordered.
//   in particular, _HashedKeyType must have valid operators <, =, and ==.
//  _HashFnObj - A class with a static function named Hash with the following
//   signature: static _HashedKeyType Hash(const char*)
template <class _T, class _KeyType, class _HashedKeyType, class _HashFnObj> class atHashedMap : public atBinaryMap<_T, _HashedKeyType> {
public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty hashmap
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atHashedMap() {}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Resource constructor
	// NOTES: Defined in binmap_struct.h
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename _Placer>
	explicit atHashedMap(class datResource& rsc, _Placer p);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Inserts a new named element into the map
	// In __DEV builds this will Quitf if that name has been inserted into the map before.
	// This function is O(N) in dev builds but O(1) in release
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Insert(const _KeyType& name, const _T& data) {
		_HashedKeyType key = Hash(name);
		// TODO: specialize this so we only use %s when _KeyType is char*
		Assertf((!atBinaryMap<_T, _HashedKeyType>::Has(key)), "atHashedMap already has key with name %s", name);
		atBinaryMap<_T,_HashedKeyType>::Sorted = false;
		atBinaryMap<_T, _HashedKeyType>::Insert(key, data);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Inserts a new named element into the map
	// In __DEV builds this will return false if that name has been inserted into the map before.
	// This function is O(N) in all builds
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SafeInsert(const _KeyType& name, const _T& data) {
		_HashedKeyType key = Hash(name);
		if (atBinaryMap<_T, _HashedKeyType>::Has(key)) {
			return false;
		}
		atBinaryMap<_T,_HashedKeyType>::Sorted = false;
		atBinaryMap<_T, _HashedKeyType>::Insert(key, data);
		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Returns the index of item with the given string key or -1
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int IndexOf(const _KeyType& name) 
	{
		_HashedKeyType key = Hash(name);
		for(int i = 0; i < atBinaryMap<_T,_HashedKeyType>::Data.GetCount(); i++)
		{
			if (atBinaryMap<_T,_HashedKeyType>::Data[i].key == key) 
				return i;
		}
		return -1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Inserts a new element into the map given a string key -maintaining a sorted list
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void InsertSorted(const _KeyType& name, const _T& data)
	{
		_HashedKeyType key = Hash(name);
		InsertSorted(key, data);
	}

	void InsertSorted(const _HashedKeyType& key, const _T& data)
	{
		int insertIndx=-1;

		// check that it isn't already in the array and prepare the insertndex
		for(int i = 0; i < atBinaryMap<_T,_HashedKeyType>::Data.GetCount(); i++)
		{
			if (atBinaryMap<_T,_HashedKeyType>::Data[i].key == key) 
			{
				//overwrite old data
				atBinaryMap<_T,_HashedKeyType>::Data[i].data = data;
				return;
			}

			if(atBinaryMap<_T,_HashedKeyType>::Data[i].key < key)
				insertIndx=i;
		}

		insertIndx++;
		atBinaryMap<_T,_HashedKeyType>::Data.Grow();
		atBinaryMap<_T,_HashedKeyType>::Data.Resize(atBinaryMap<_T,_HashedKeyType>::Data.GetCount()-1);
		typename atBinaryMap<_T,_HashedKeyType>::DataPair& d = atBinaryMap<_T,_HashedKeyType>::Data.Insert(insertIndx);
		d.key  = key;
		d.data = data;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Returns true if an item with the given name exists in the map
	// REQUIREMENTS: FinishInsertion() must be called before this is called
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool Has(const _KeyType& name) {
		return atBinaryMap<_T, _HashedKeyType>::Has(Hash(name));
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Returns true if an item with the given name exists in the map
	// REQUIREMENTS: FinishInsertion() must be called before this is called
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool Has(const _HashedKeyType& hash)
	{
		return atBinaryMap<_T, _HashedKeyType>::Has(hash);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: returns the hashed value of the passed in string. Just a wrapper for _HashFnObj::Hash, but
	// useful if you don't happen to know what your _HashFnObj was.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_HashedKeyType Hash(const _KeyType& name) const {
		return _HashFnObj::Hash(name);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Gets the item with the given name, or NULL if no such item exists
	// REQUIREMENTS: FinishInsertion() must be called before this is called
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* SafeGet(const _KeyType& name) {
		return atBinaryMap<_T, _HashedKeyType>::SafeGet(Hash(name));
	}
	const _T* SafeGet(const _KeyType& name) const {
		return atBinaryMap<_T, _HashedKeyType>::SafeGet(Hash(name));
	}

	_T* SafeGet(const _HashedKeyType& hash) {
		return atBinaryMap<_T, _HashedKeyType>::SafeGet(hash);
	}
	const _T* SafeGet(const _HashedKeyType& hash) const {
		return atBinaryMap<_T, _HashedKeyType>::SafeGet(hash);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: return the item with the given name. Item must exist in the map
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T& operator[](const _KeyType& name) {
		_T* ret = atBinaryMap<_T, _HashedKeyType>::SafeGet(Hash(name));
#if __DEV
		if (!ret) {Quitf("Error: %s not found in atHashedMap", name);}
#else
		FastAssert(ret);
#endif
		return *ret;
	}

};

#ifndef atStringHashObj_DEFINED
#define atStringHashObj_DEFINED
struct atStringHashObj {
	static u32 Hash (const char* string) {
		return atStringHash(string);
	}
};
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: atStringMap is a map from strings to arbitrary objects. It is a subclass of atHashedMap, and 
// does not actually store the strings, instead it stores the hash of the strings as the map's key.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class _T> 
class atStringMap : public atHashedMap<_T, const char*, u32, atStringHashObj> 
{
public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty string map	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atStringMap() {}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Resource constructor
	// NOTES: Defined in binmap_struct.h
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename _Placer>
	explicit atStringMap(class datResource& rsc, _Placer p);
};

#ifndef atCaseSensitiveStringHashObj_DEFINED
#define atCaseSensitiveStringHashObj_DEFINED
struct atCaseSensitiveStringHashObj {
	static u32 Hash (const char* string) {
		return atLiteralStringHash(string);
	}
};
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: atStringMap is a map from strings to arbitrary objects. It is a subclass of atHashedMap, and 
// does not actually store the strings, instead it stores the hash of the strings as the map's key using a
// case sensitive string hash function.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class _T>
class atCaseSensitiveStringMap : public atHashedMap<_T, const char*, u32, atCaseSensitiveStringHashObj> 
{
public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty string map	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atCaseSensitiveStringMap() {}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Resource constructor
	// NOTES: Defined in binmap_struct.h
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename _Placer>
	explicit atCaseSensitiveStringMap(class datResource& rsc, _Placer p);
};

#ifdef _CPPRTTI
struct atTypeHashObj {
	static u32 Hash(const type_info& type) {
		return atLiteralStringHash(type.name());
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: atStringMap is a map from RTTI types to arbitrary objects. It is a subclass of atHashedMap, and 
// uses hashes of the type name as map keys
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <class _T>
class atTypeMap : public atHashedMap<_T, type_info, u32, atTypeHashObj> 
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty type map
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atTypeMap();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Resource constructor
	// NOTES: Defined in binmap_struct.h
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<typename _Placer>
	explicit atTypeMap(class datResource& rsc, _Placer p);
};

#endif

// helper algorthim to help with delete maps of pointers
template<class T>
void atDeleteAll( T start, T end )
{
	while ( start != end )
	{
		delete *start;
		++start;
	}
}

// PURPOSE: Represent a map from string to arbitrary data, with low memory overhead and fast searching
// Uses a hash function to convert strings into key values which are stored and searched on.
// NOTES:
//		The atBinaryMap can be in one of two states - Sorted and non-Sorted. Normally you would Insert()
//		a bunch of items into the list, then call FinishInsertion(). This sorts the list and makes searches
//		O(log(N)). If the list is unsorted the searches are O(N).
//		This is slightly different from atBinaryMap as the key is stored in the data and it is accessed through a 
//		GetKey() function
// PARAMS:
//	_T - the data that is indexed with the string. This type must have a GetKey() function
//  _KeyType - the internal storage type for the key value. This type must be ordered.
//   in particular, _KeyType must have valid operators <, =, and ==.
template <class _T, class _KeyType> class atInBinaryMap {
public:

	class Iterator
	{
		friend class atInBinaryMap;

	public:
		// PURPOSE: default ctor (for End)
		Iterator(): Map(NULL), CurIdx(0) {}
		// PURPOSE: ctor for Begin
		explicit Iterator(atInBinaryMap<_T,_KeyType>& themap):Map(&themap), CurIdx(0) {}
		explicit Iterator(atInBinaryMap<_T,_KeyType>& themap, const int idx):Map(&themap), CurIdx(idx) {}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		Iterator& operator++()
		{
			if(++CurIdx>=Map->Data.GetCount())
				*this = Map->End();
			return *this;
		}

		// PURPOSE: operator ++ advances the iterator to refer to the next active element
		Iterator operator++(const int)
		{
			Iterator result(*this);
			++(*this);
			return result;
		}

		// PURPOSE: accesses the active map element that this iterator refers to
		_T& operator*() const
		{
			FastAssert(Map);
			AssertMsg(CurIdx < Map->Data.GetCount() , "atInBinaryMap Iterator is past end, cannot dereference");
			return Map->Data[CurIdx];
		}

		// PURPOSE: checks to see if this iterator points to the same element as another
		bool operator==(const Iterator& comp) const
		{
			return (CurIdx == comp.CurIdx) && (Map == comp.Map);
		}

		// PURPOSE: checks to see if this iterator does not refer to the same element as another
		bool operator!=(const Iterator& comp) const
		{
			return !((CurIdx == comp.CurIdx) && (Map == comp.Map));
		}

		// PURPOSE: 
		Iterator& operator=(const Iterator& rhs)
		{
			Map = rhs.Map;
			CurIdx = rhs.CurIdx;
			return *this;
		}

		bool            IsValid()      { return(Map!=NULL); }

		_KeyType&       GetKey()       { FastAssert(Map); return Map->Data[CurIdx].GetKey(); }

		const char* GetKeyName() const { FastAssert(Map); return Map->Data[CurIdx].GetKeyName(); }

	private:
		atInBinaryMap<_T, _KeyType> * Map;
		int CurIdx;
	}; /******************* END DEFINITION atBinaryMap::Iterator ******************/

	friend class Iterator;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: returns an iterator that points to the beginning of Data
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Iterator Begin()
	{
		if(Data.GetCount()>0)
			return Iterator(*this);
		else return End();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: returns an iterator that points past the end of Data,
	// used to test if an iterator is still valid.
	static Iterator End()
	{
		return Iterator();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Creates a new empty binary map	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	atInBinaryMap() {
		Sorted = true;   //Empty list is sorted
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Reset() {
		Data.Reset();
		Sorted = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map but doesn't delete allocated memory
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void ResetCount() {
		Data.ResetCount();
		Sorted = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Clears any data in the map but doesn't delete allocated memory
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool IsSorted() const {
		return Sorted;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Resizes the data array so that future additions will be fast
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Reserve(int size) {
		Data.Reserve(size);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Insert(const _T& data) {
		// I removed the assert checking for duplicates as this can be checked for using FindDuplicates()
		Sorted = false;
		_T& d = Data.Grow();
		d = data;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Inserts a new element into the map given a key
// This will return false if that string key is already in use.
// This is always O(N)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SafeInsert(const _T& data) {
		if (Has(data.GetKey())) {
			return false;
		}
		Sorted = false;
		_T& d = Data.Grow();
		d = data;
		return true;
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Remove the ith element from the map
// This is O(N) if the list is sorted, O(1) if it's not
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Remove(int i)
	{
		if (Sorted)
		{
			Data.Delete(i);
		}
		else
		{
			Data.DeleteFast(i);
		}
	}

	void Remove(Iterator iter)
	{
		if (Sorted)
		{
			Data.Delete(iter.CurIdx);
		}
		else
		{
			Data.DeleteFast(iter.CurIdx);
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	int GetCount() const
	{
		return Data.GetCount();
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* GetItem(int i)
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].data;
	}
	
	const _T* GetItem(int i) const
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].data;
	}

	_KeyType* GetKey(int i)
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].GetKey();
	}
	
	const _KeyType* GetKey(int i) const
	{	
		FastAssert(i>=0 && i<Data.GetCount());
		return &Data[i].GetKey();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Returns true if an item with the given string key exists in the map
// NOTES: If FinishInsertion has been called, this is O(log N), otherwise it's O(N)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool Has(const _KeyType& key) {
		if (Sorted)
		{
			return SafeGet(key) != NULL;
		}
		else
		{
			for(int i = 0; i < Data.GetCount(); i++)
			{
				if (Data[i].GetKey() == key) {
					return true;
				}
			}
			return false;
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: Gets the item with the given key, or NULL if no such item exists
// REQUIREMENTS: FinishInsertion() must be called before this is called
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T* SafeGet(const _KeyType& key) {
		FastAssert(Sorted);
		if (Data.GetCount() > 0) {
			const _KeyType& searchKey = key;
			_T* ret = std::lower_bound(Data.begin(), Data.end(), searchKey, SearchCompare);
			if (ret != Data.end() && ret->GetKey() == key) { // need to test because lower_bound finds the index of the smallest item greater than key
				return (ret); 
			}
		}
		return NULL;
	}

	const _T* SafeGet(const _KeyType& key) const {
		FastAssert(Sorted);
		if (Data.GetCount() > 0) {
			const _KeyType& searchKey = key;
			const _T* ret = std::lower_bound(Data.begin(), Data.end(), searchKey, SearchCompare);
			if (ret != Data.end() && ret->GetKey() == key) { // need to test because lower_bound finds the index of the smallest item greater than key
				return (ret); 
			}
		}
		return NULL;
	}

	bool SafeGetIterator(const _KeyType& key, Iterator& iter) {
		FastAssert(Sorted);
		if (Data.GetCount() > 0) {
			const _KeyType& searchKey = key;
			_T* dataBegin = Data.begin();
			_T* dataEnd   = Data.end();
			_T* dataWhere = std::lower_bound(dataBegin, dataEnd, searchKey, SearchCompare);
			if (dataWhere != dataEnd && dataWhere->GetKey() == key) { // need to test because lower_bound finds the index of the smallest item greater than key
				iter = Iterator(*this,ptrdiff_t_to_int(dataWhere-dataBegin));
				return true;
			}
		}

		return false;
	}

	bool GetIterator(const _KeyType& key, Iterator& iter)
	{
		FastAssert(Sorted);
		void* ret = bsearch(&key, &Data[0], (size_t)Data.GetCount(), sizeof(_T), (int(*)(const void*, const void*))CompareBinarySearch);
		if (ret)
		{
			_T* dataBegin = Data.begin();
			_T* dataWhere = reinterpret_cast<_T*>(ret);
			iter = Iterator(*this, ptrdiff_t_to_int(dataWhere-dataBegin));
			return true;
		}

		return false;
	}

	_T* UnSafeGet(const _KeyType& key) {
		FastAssert(Sorted);
		void* ret = bsearch(&key, &Data[0], (size_t)Data.GetCount(), sizeof(_T), (int(*)(const void*, const void*))CompareBinarySearch);
		if (ret) return reinterpret_cast<_T*>(ret);
		return NULL;
	}

	const _T* UnSafeGet(const _KeyType& key) const {
		FastAssert(Sorted);
		void* ret = bsearch(&key, &Data[0], (size_t)Data.GetCount(), sizeof(_T), (int(*)(const void*, const void*))CompareBinarySearch);
		if (ret) return reinterpret_cast<_T*>(ret);
		return NULL;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Finalzes map creation and prepares map for searching. 
	// SafeGet and operator[] may not be used until this function is called
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void FinishInsertion() {
		if(Sorted) return;
		if (Data.GetCount() > 0) {
			std::sort(Data.begin(), Data.end(), SortCompare);
		}
		Sorted = true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PURPOSE: Checks to see if a binary map has duplicates. 
	// RETURNS: returns first duplicate key it finds or zero if none are found
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_KeyType FindDuplicates() {
		FastAssert(Sorted);
		for (int i=0; i<Data.GetCount()-1; i++) {
			if(Data[i].GetKey() == Data[i+1].GetKey())
				return Data[i].GetKey();
		}
		return _KeyType();
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: mark the map as unsorted, use if you've done something dirty to a key.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Unsort() {
		Sorted = false;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PURPOSE: return the item with the given key. Item must exist in the map
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	_T& operator[](const _KeyType& key) {
		_T* ret = SafeGet(key);
		FastAssert(ret);
		return *ret;
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static bool SearchCompare(const _T& data, const _KeyType& key) {
		return (data.GetKey() < key); 
	}
	static bool SortCompare(const _T& a, const _T& b) {
		return (a.GetKey() < b.GetKey());
	}
	static int CompareBinarySearch(const void* eltA, const void* eltB)
	{
		_T* a = (_T*)eltA;
		_T* b = (_T*)eltB;

		return a->GetKey() < b->GetKey() ? -1 : (a->GetKey() == b->GetKey() ? 0 : 1);
	}


	atArray<_T>& GetRawDataArray() {return Data;}

#if !__SPU
protected:
#endif

	bool Sorted;
	char Pad[3];
	atArray<_T> Data; 
};

}

#endif
