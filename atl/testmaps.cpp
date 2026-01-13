//
// atl/testmaps.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "atl/bintree.h"
#include "atl/hashsimple.h"
#include "atl/map.h"
#include "atl/binmap.h"
#include "diag/output.h"
#include "math/amath.h"
#include "math/random.h"
#include "string/string.h"
#include "system/ipc.h"
#include "system/main.h"
#include "system/timer.h"

#include <algorithm>
#include <map>
#include <hash_map>
#include <functional>
#include <set>

#define HASH_MAP_TEST 1

namespace rage {

// For testing SimpleHash
class HashElement
{
public:
	HashElement() : m_Value(0) { }

	u32 GenerateHash() const { return m_Value; }
	bool IsEqual(const HashElement* other) const { return m_Value == other->m_Value; }
	void SetValue(int value) { m_Value = value; }
	void SetData(ConstString* data) { m_Data = data; }
	ConstString* GetData() { return m_Data; }

private:
	int m_Value;
	ConstString* m_Data;
};

class HashElementReverse
{
public:
	HashElementReverse() : m_Value(NULL) { }

	u32 GenerateHash() const { return atHash(*m_Value); }
	bool IsEqual(const HashElementReverse* other) const { return *m_Value == *other->m_Value; }
	void SetValue(ConstString* value) { m_Value = value; }
	void SetData(int data) { m_Data = data; }
	int GetData() { return m_Data; }

private:
	ConstString* m_Value;
	int m_Data;
};

// Used to test the STL containers
struct ConstStringMapPred : public std::binary_function<const ConstString&, const ConstString&, bool>
{
	bool operator()(const ConstString& string1, const ConstString& string2) const
	{
		return strcmp(string1, string2) < 0;
	}
};

}	// namespace rage


using namespace rage;

int Main ()
{
	// control parameters
	const int kRunSize = 600; // to keep within the atMap limit
	const int kKeyRange = (int)(kRunSize * logf((float)kRunSize) * 2.5f);
	const int kBufferSize = 35;

	sysTimer timer;
	mthRandom randSource;

	// only do one for fair timing
	const bool doATBintree = true;
	const bool doATBintreePrealloc = true;
	const bool doATMap = kRunSize <= 60000;
	const bool doATMapPrealloc = kRunSize <= 60000;
	const bool doSTL = true;
#if HASH_MAP_TEST
	const bool doSTLHash = true;
#endif // HASH_MAP_TEST
	const bool doSimple = true;

	const bool doATReverseBintree = true;
	const bool doATReverseBintreePrealloc = true;
	const bool doATReverseMap = kRunSize <= 60000;
	const bool doATReverseMapPrealloc = kRunSize <= 60000;
	const bool doATReverseStringMap = kRunSize <= 60000;
	const bool doATReverseStringMapPrealloc = kRunSize <= 60000;
	const bool doSTLReverse = true;
	// const bool doSTLHashReverse = true;
	// const bool doSimpleReverse = true;

	atBinTree<int,ConstString> tree;

	char buffer[kBufferSize];

	// make test data
	int i;
	int * keys = rage_new int [kRunSize];
	ConstString * data = rage_new ConstString [kRunSize];
	for (i=0; i<kRunSize; i++)
	{
		// make random data
		for (int j=0; j<kBufferSize-1; j++)
		{
			buffer[j] = 'a' + (char)(randSource.GetRanged(0,25));
		}
		buffer[kBufferSize-1] = 0;

		std::set<int> keySet;
		int newKey = randSource.GetRanged(0,kKeyRange);
		while (keySet.find(newKey) != keySet.end())
		{
			newKey = randSource.GetRanged(0,kKeyRange);
		}
		keySet.insert(newKey);
		keys[i] = newKey;
		data[i] = buffer;
	}

#define TEST_MAP(INSERT, DONE_INSERTING, ASSERT_ACCESS, ACCESS)														\
	timer.Reset();																									\
	unsigned int startPopulate = sysTimer::GetSystemMsTime();														\
	for (i=0; i<kRunSize; i++)																						\
	{																												\
		INSERT;																										\
	}																												\
	DONE_INSERTING;																									\
																													\
	unsigned int populateTime = sysTimer::GetSystemMsTime() - startPopulate;										\
	float populateTimeSmall = timer.GetMsTime();																	\
																													\
	for (i=0; i<10*kRunSize; i++)																					\
	{																												\
		int index = randSource.GetRanged(0, kRunSize-1);															\
		ASSERT_ACCESS;																								\
	}																												\
																													\
	timer.Reset();																									\
	unsigned int startRun = sysTimer::GetSystemMsTime();															\
	for (i=0; i<10*kRunSize; i++)																					\
	{																												\
		int index = randSource.GetRanged(0, kRunSize-1);															\
		ACCESS;																										\
	}																												\
																													\
	unsigned int runTime = sysTimer::GetSystemMsTime() - startRun;													\
	float runTimeSmall = timer.GetMsTime();																			\
																													\
	Displayf("populate %d ms (%f ms) run %d ms (%f ms)",populateTime,populateTimeSmall,runTime,runTimeSmall);		\

	Displayf("* Maps from ints to char strings");
	Displayf(" ");

	if (doATBintree)
	{
		atBinTree<int,ConstString> map;
		
		Displayf("atBinTree<int,ConstString>");																						
		TEST_MAP(map.Insert(keys[i],data[i]),
				 ,
				 AssertVerify(strcmp(*map.Access(keys[index]),data[index])==0),
				 map.Access(keys[index]));

		map.DeleteAll();
	}
	if (doATBintreePrealloc)
	{
		atBinTree<int,ConstString> map(kRunSize);

		Displayf("atBinTree<int,ConstString> prealloc");																						
		TEST_MAP(map.Insert(keys[i],data[i]),
				 ,
				 AssertVerify(strcmp(*map.Access(keys[index]),data[index])==0),
				 map.Access(keys[index]));

		map.DeleteAll();
	}

	if (doATMap)
	{
		atMap<int,ConstString> map;

		Displayf("atMap<int,ConstString>");																						
		TEST_MAP(map.Insert(keys[i],data[i]),
			     ,
				 AssertVerify(strcmp(*map.Access(keys[index]),data[index])==0),
				 map.Access(keys[index]));
	}

	if (doATMapPrealloc)
	{
		atMap<int,ConstString> map;
		map.Recompute(65167);

		Displayf("atMap<int,ConstString> prealloc");																						
		TEST_MAP(map.Insert(keys[i],data[i]),
			     ,
				 AssertVerify(strcmp(*map.Access(keys[index]),data[index])==0),
				 map.Access(keys[index]));
	}

	if (doATReverseStringMapPrealloc)
	{
		atMap<int,ConstString> map;
		map.Recompute(65167);

		Displayf("atMap<int,ConstString> prealloc");																						
		TEST_MAP(map.Insert(keys[i],data[i]),
			     ,
				 AssertVerify(strcmp(*map.Access(keys[index]),data[index])==0),
				 map.Access(keys[index]));
	}

	if (doSTL)
	{
		typedef std::map<int,ConstString> MapType;
		MapType map;

		static MapType::iterator found; // Without this the compiler seems to optimize out the find altogether...
		Displayf("std::map<int,ConstString>");	
		TEST_MAP(map.insert(MapType::value_type(keys[i],data[i])),
				 ,
				 found = map.find(keys[index]); Assert(strcmp(found->second,data[index])==0),
				 found = map.find(keys[index]));
	}

#if HASH_MAP_TEST
	if (doSTLHash)
	{
		typedef std::hash_map<int,ConstString> MapType;
		MapType map;

		static MapType::iterator found; // Without this the compiler seems to optimize out the find altogether...
		Displayf("stdext::hash_map<int,ConstString>");																						
		TEST_MAP(map.insert(MapType::value_type(keys[i],data[i])),
				 ,
				 found = map.find(keys[index]); Assert(strcmp(found->second,data[index])==0),
				 found = map.find(keys[index]));
//		TEST_MAP(map.insert(MapType::value_type(keys[i],data[i])), found = map.find(keys[index]),);
	}
#endif // HASH_MAP_TEST

	if (doSimple)
	{
		SimpleHash<HashElement> map;
		map.Init(kRunSize);

		Displayf("SimpleHash<HashElement>");
		HashElement element;
		static HashElement* found;
		TEST_MAP(element.SetValue(keys[i]); element.SetData(&data[i]); map.Insert(element),
			     ,
				 element.SetValue(keys[index]); found = map.Search(&element); Assert(strcmp(*found->GetData(), data[index]) == 0),
				 element.SetValue(keys[index]); found = map.Search(&element););
	}

	Displayf(" ");
	Displayf("* Maps from char strings to ints");
	Displayf(" ");

	if (doATReverseBintree)
	{
		atBinTree<ConstString,int> map;
		
		Displayf("atBinTree<int,ConstString>");																						
		TEST_MAP(map.Insert(data[i],keys[i]),
			     ,
				 char stackString[256]; strcpy(stackString, data[index]); Assert(*map.Access(stackString) == keys[index]),
				 map.Access(data[index]));

		map.DeleteAll();
	}

	if (doATReverseBintreePrealloc)
	{
		atBinTree<ConstString,int> map(kRunSize);

		Displayf("atBinTree<int,ConstString> prealloc");																						
		TEST_MAP(map.Insert(data[i],keys[i]),
			     ,
				 char stackString[256]; strcpy(stackString, data[index]); Assert(*map.Access(stackString) == keys[index]),
				 map.Access(data[index]));

		map.DeleteAll();
	}

	if (doATReverseMap)
	{
		atMap<ConstString,int> map;

		Displayf("atMap<ConstString, int>");																						
		TEST_MAP(map.Insert(data[i],keys[i]),
			     ,
				 char stackString[256]; strcpy(stackString, data[index]); Assert(*map.Access(stackString) == keys[index]),
				 map.Access(data[index]));
	}

	if (doATReverseMapPrealloc)
	{
		atMap<ConstString, int> map;
		map.Recompute(65167);

		Displayf("atMap<ConstString, int> prealloc");																						
		TEST_MAP(map.Insert(data[i],keys[i]),
			     ,
				 char stackString[256]; strcpy(stackString, data[index]); Assert(*map.Access(stackString) == keys[index]),
				 map.Access(data[index]));
	}

	if (doATReverseStringMap)
	{
		atStringMap<int> map;

		Displayf("atStringMap<int>");
		TEST_MAP(map.Insert(data[i].m_String,keys[i]),
			     map.FinishInsertion(),
				 char stackString[256]; strcpy(stackString, data[index]); Assert(*map.SafeGet(stackString) == keys[index]),
				 map.SafeGet(data[index]));
	}

	if (doSTLReverse)
	{
		typedef std::map<ConstString,int,ConstStringMapPred> MapType;
		MapType map;

		static MapType::iterator found;
		Displayf("std::map<ConstString,int>");																						
		TEST_MAP(map.insert(MapType::value_type(data[i],keys[i])),
			     ,
				 char stackString[256]; strcpy(stackString, data[index]); Assert(map.find(data[index])->second == keys[index]),
				 found = map.find(data[index]));
	}

#if 0
#if HASH_MAP_TEST
	if (doSTLHashReverse)
	{
		typedef std::hash_map<const char*,int> MapType;
		MapType map;

		static MapType::iterator found;
		Displayf("std::hash_map<ConstString,int>");																						
		TEST_MAP(map.insert(MapType::value_type(data[i].m_String,keys[i])), found = map.find(data[index].m_String),);
	}
#endif // HASH_MAP_TEST

	if (doSimpleReverse)
	{
		SimpleHash<HashElementReverse> map;
		map.Init(kRunSize);

		Displayf("SimpleHash<HashElementReverse>");
		HashElementReverse element;
		static HashElementReverse* found;
		TEST_MAP(element.SetValue(&data[i]); element.SetData(keys[i]); map.Insert(element), element.SetValue(&data[index]); found = map.Search(&element); Assert(found->GetData() == keys[index]),);
	}
#endif
	Displayf("Done");

	return 0;
}
