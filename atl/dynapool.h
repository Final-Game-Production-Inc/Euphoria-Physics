//
// atl/dynapool.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_DYNAPOOL_H
#define ATL_DYNAPOOL_H

#include "atl/array.h"
#include "diag/tracker.h"
#include "string/string.h"
#include "system/memops.h"
#include "system/new.h"
#include "system/param.h"
#include "system/typeinfo.h"



// PURPOSE:
//	This file defines the templated class atDynaPool, which implements 
//	a pool of objects with a fixed "best-guess" initial size but also
//	with the ability to grow arbitrarily using run-time allocation.
//
//	Allocating a large array for the initial pool is desirable to avoid 
//	the costs in memory (both from per-allocation overhead, and memory 
//	fragmentation) and time of allocating each object one-at-time on the heap.
//
//	This functionality was originally prompted by various script-driven
//	systems that require the ability to allocate objects "one at a time" 
//	at run time (or at least, at init time) whenever a script command is 
//	called -- without any up-front knowledge of how many objects will be
//	needed in the general or worst case.
//
//	Open questions:
//
//		* Should we allow the creation of objects of various derived types
//			(given that we know their common base class and maximum size?)
//
//		* Would it be a major win to allocate additional objects in large
//			arrays instead of one at a time?  (There'd be overhead in 
//			keeping track of these "chunks"/"pages", of course.)
//
//		* When DestroyObject is called on an object that was allocated
//			dynamically, should we reclaim its memory?  (Optionally?)
//
//		* Are there cleverer/less messy ways to support optional  
//			statistics tracking?

//////////////////////////////////////////////////////////////////////////

// If you enable this define, there will be a global dynapool manager,
// atDynaPoolmanager, which will maintain a list of all dynaPools in the
// application and dump the stats on demand. (Might be a good idea to hook
// up a RAG button to trigger that in your application).
#define ENABLE_DYNAPOOL_MANAGER	(1 && __DEV)

#if ENABLE_DYNAPOOL_MANAGER
	#define DYNAPOOL_MANAGER_ONLY(_x) _x
#else // ENABLE_DYNAPOOL_MANAGER
	#define DYNAPOOL_MANAGER_ONLY(_x)
#endif // !ENABLE_DYNAPOOL_MANAGER


namespace rage {

XPARAM(assertdynapooloverrun);


class atDynaPoolBaseWithStats;
class fiStream;


class atDynaPoolManager
{

public:
	// PURPOSE: Dump the stats of every dynamic pool that has been registered
	// with this manager.
	static void DumpStats();
	static void DumpStatsToFile(fiStream *S);

protected:
	// PURPOSE: Register a dynamic pool so it will show up in DumpStats.
	// This should happen automatically in a pool's constructor.
	static void RegisterPool(atDynaPoolBaseWithStats &pool);

	// PURPOSE: Unregister a dynamic pool. This should happen automatically
	// in a pool's destructor.
	static void UnregisterPool(atDynaPoolBaseWithStats &pool);

private:
#if ENABLE_DYNAPOOL_MANAGER
	// List of all registered pools.
	static atArray<atDynaPoolBaseWithStats *> m_PoolList;
#endif // ENABLE_DYNAPOOL_MANAGER

	friend class atDynaPoolBaseWithStats;
};

#if !ENABLE_DYNAPOOL_MANAGER
	inline void atDynaPoolManager::DumpStats()	{}
	inline void atDynaPoolManager::RegisterPool(atDynaPoolBaseWithStats &/*pool*/) {}
	inline void atDynaPoolManager::UnregisterPool(atDynaPoolBaseWithStats &/*pool*/) {}
#endif // !ENABLE_DYNAPOOL_MANAGER



//////////////////////////////////////////////////////////////////////////

// PURPOSE:
//		A helper base class for atDynaPoolWithStats.  This provides member 
//		variables and defines functionality for statistics tracking.
class atDynaPoolBaseWithStats
{
public:
#if ENABLE_DYNAPOOL_MANAGER
	atDynaPoolBaseWithStats()
	{
		atDynaPoolManager::RegisterPool(*this);
	}

	~atDynaPoolBaseWithStats()
	{
		atDynaPoolManager::UnregisterPool(*this);
	}
#endif // ENABLE_DYNAPOOL_MANAGER

	void StatsInit(int numObjectsInArray, size_t size)
	{
		m_StatsNumRemaining	= numObjectsInArray;
		m_StatsMaxUsed = m_StatsNumUsed = 0;
		m_StatsNumAllocatedDynamically = 0;
		m_ElementSize = size;
	}

	void StatsDecNumRemaining()
	{	m_StatsNumRemaining--;	}

	void StatsIncNumAllocatedDynamically()
	{	m_StatsNumAllocatedDynamically++; }

	void StatsIncNumUsed()
	{
		m_StatsNumUsed++;
		if(m_StatsNumUsed > m_StatsMaxUsed)
			m_StatsMaxUsed = m_StatsNumUsed;
	}

	void StatsVerifyHasName()
	{
#ifdef _CPPRTTI
		AssertMsg(GetName()[0] != '[', "Dynapool doesn't have a name. Please set one with SetName() so it can be tracked properly.");
#endif // _CPPRTTI
	}

#if !__FINAL
	void StatsWarnAboutDynamicAllocation(int OUTPUT_ONLY(numObjectsInArray))
	{
//		Assertf(StatsGetNumAllocatedDynamically() != 0, "Running out of objects in the %s pool, allocating dynamically. "
//			"Pool size is %d.", GetName(), numObjectsInArray);

#if __ASSERT
		if (PARAM_assertdynapooloverrun.Get())
		{
			Assertf(StatsGetNumAllocatedDynamically() != 0, "Dynamic pool %s is full (%d entries) and switched to dynamic allocations. Might wanna increase it.", GetName(), numObjectsInArray);
		}
#endif // __ASSERT



		if(StatsGetNumAllocatedDynamically() == 0)
		{
			Warningf("Running out of objects in the %s pool, allocating dynamically. "
					"Pool size is %d.", GetName(), numObjectsInArray);
		}
	}
#endif

	void StatsNoteDestruction()
	{
		m_StatsNumRemaining++;
		m_StatsNumUsed--;
	}
	
	// PURPOSE: The maxmimum number of objects this instance has ever used..
	int	StatsGetMaxUsed() const
	{ return m_StatsMaxUsed; }

	int	&StatsGetMaxUsedRef()
	{ return m_StatsMaxUsed; }

	// PURPOSE: The current number of objects allocated for use. 
	int	StatsGetNumRemaining() const
	{ return m_StatsNumRemaining; }

	// PURPOSE: The current number of objects in use.
	int	StatsGetNumUsed() const
	{ return m_StatsNumUsed; }

	int	&StatsGetNumUsedRef()
	{ return m_StatsNumUsed; }

	// PURPOSE: Total number of objects that have been allocated on the heap.
	int StatsGetNumAllocatedDynamically() const
	{ return m_StatsNumAllocatedDynamically; }

	size_t StatsGetElementSize() const
	{ return m_ElementSize; }

	void StatsSetElementSize(size_t size)
	{ m_ElementSize = size; }

	int &StatsGetNumAllocatedDynamicallyRef()
	{ return m_StatsNumAllocatedDynamically; }

	// RETURNS: The debug name of this pool. This string is guaranteed to be
	// valid.
	const char *GetName() const
	{
#if ENABLE_DYNAPOOL_MANAGER
		return m_DebugName.c_str() ? m_DebugName.c_str() : "[unnamed]";
#else // ENABLE_DYNAPOOL_MANAGER
		return "";
#endif // !ENABLE_DYNAPOOL_MANAGER
	}

	// PURPOSE: Sets the debugging name of this pool. The string will be copied,
	// so it is safe to use a temporary buffer as the input string.
	void SetName(const char *DYNAPOOL_MANAGER_ONLY(name))
	{
#if ENABLE_DYNAPOOL_MANAGER
		m_DebugName = name;
#endif // ENABLE_DYNAPOOL_MANAGER
	}


protected:	
	// PURPOSE: The maxmimum number of objects this instance has ever used..
	int	m_StatsMaxUsed;

	// PURPOSE: The current number of objects allocated for use. 
	int	m_StatsNumRemaining;
	
	// PURPOSE: The current number of objects in use.
	int	m_StatsNumUsed;

	// PURPOSE: Total number of objects that have been allocated on the heap.
	int m_StatsNumAllocatedDynamically;

	// PURPOSE: Size of each element
	size_t m_ElementSize;

#if ENABLE_DYNAPOOL_MANAGER
	// PURPOSE: A debug name that identifies this pool
	ConstString m_DebugName;
#endif // ENABLE_DYNAPOOL_MANAGER
};

//////////////////////////////////////////////////////////////////////////

// PURPOSE:
//		A helper base class for atDynaPool.  This provides no-op
//		stubs for the functions that (in atDynaPoolBaseWithStats)
//		implement statistics tracking.
class atDynaPoolBaseWithoutStats
{
public:
	void	StatsInit(int, size_t)					{}
	void	StatsDecNumRemaining()					{}	
	void	StatsIncNumAllocatedDynamically()		{}
	void	StatsIncNumUsed()						{}
	void	StatsWarnAboutDynamicAllocation(int)	{}
	void	StatsVerifyHasName()					{}
	void	StatsNoteDestruction()					{}

	int		StatsGetMaxUsed() const					{return 0;}
	int		StatsGetNumRemaining() const			{return 0;}
	int		StatsGetNumUsed() const					{return 0;}
	int		StatsGetNumAllocatedDynamically() const	{return 0;}
};

//////////////////////////////////////////////////////////////////////////

// PURPOSE:
//		A helper base class for atDynaPool and atDynaPoolWithStats.
//		_Base should be either atDynaPoolBaseWithStats or atDynaPoolBaseWithoutStats.
// NOTE:
//		Most of the atDynaPool functionality is actually implemented here,
//		so that it's shared between atDynaPoolWithStats and atDynaPoolBaseWithoutStats
template <class _Type, class _Base, bool _Destruct=true, bool _Construct=true>
class atDynaPoolBase : public _Base
{
public:
	atDynaPoolBase(int maxNumObjects);
	~atDynaPoolBase();

	void InitPool();
	void ResetPool();
	void DestroyPool();

	void ValidateReset() const;

	_Type &CreateObject();
	void DestroyObject(_Type &obj);

	int GetNumObjectsInArray() const
	{
		return m_NumObjectsInArray;
	}

	
protected:
	union InternalNode
	{	
		InternalNode	*m_Next;
		char			m_ObjectBuffer[sizeof(_Type)];
	};

	// Array of m_NumObjectsInArray preallocated InternalNode objects.
	InternalNode	*m_ObjectArray;

	// PURPOSE: Total number of preallocated objects.
	int				m_NumObjectsInArray;

	// PURPOSE:	Pool of currently available objects. These may be coming from
	//			m_ObjectArray, or they may have been dynamically allocated if
	//			running out of space in the pool.
	InternalNode	*m_FreeNodePool;
};

//////////////////////////////////////////////////////////////////////////

template <class _Type, class _Base, bool _Destruct, bool _Construct>
atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::atDynaPoolBase(int maxNumObjects)
{
	m_NumObjectsInArray = maxNumObjects;

	InitPool();
}

//////////////////////////////////////////////////////////////////////////

template <class _Type, class _Base, bool _Destruct, bool _Construct>
atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::~atDynaPoolBase()
{
	DestroyPool();
}

//////////////////////////////////////////////////////////////////////////
template <class _Type, class _Base, bool _Destruct, bool _Construct>
void atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::InitPool()
{
	RAGE_TRACK(DynaPoolArray);

	m_ObjectArray = rage_new InternalNode[m_NumObjectsInArray];
	m_FreeNodePool = NULL;

	int i;
	for(i = 0; i < m_NumObjectsInArray; i++)
	{	
		// Append the object array nodes at the head of the list.
		m_ObjectArray[i].m_Next	= m_FreeNodePool;
		m_FreeNodePool			= &(m_ObjectArray[i]);
	}
	
	_Base::StatsInit(m_NumObjectsInArray, sizeof(_Type));
}

//////////////////////////////////////////////////////////////////////////
template <class _Type, class _Base, bool _Destruct, bool _Construct>
void atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::ResetPool()
{
	DestroyPool();
	InitPool();
}

//////////////////////////////////////////////////////////////////////////
template <class _Type, class _Base, bool _Destruct, bool _Construct>
void atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::DestroyPool()
{
	InternalNode *node=m_FreeNodePool, *next;
	while(node != NULL)
	{
		next = node->m_Next;
		int arrayIndex = (int) (node - m_ObjectArray);
		if(arrayIndex < 0 || arrayIndex >= m_NumObjectsInArray)
		{
			delete node;
		}
		node = next;
	}
	delete []m_ObjectArray;
}

//////////////////////////////////////////////////////////////////////////

template <class _Type, class _Base, bool _Destruct, bool _Construct>
void atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::ValidateReset() const
{
#if !__FINAL
	const InternalNode *node;
	int numFreeObjects = 0;
	for(node=m_FreeNodePool; node; node=node->m_Next)
		numFreeObjects++;

	const int dynamicallyAllocated = _Base::StatsGetNumAllocatedDynamically();
	const int totalAllocated = m_NumObjectsInArray + dynamicallyAllocated;

	// Note: Ideally, we would want numFreeObjects and totalAllocated to be the
	// same, and this check here to use != instead of <. It used to be like that,
	// but that only works if StatsGetNumAllocatedDynamically() is implemented to
	// return the correct value, i.e. if deriving from atDynaPoolBaseWithStats.
	// For atDynaPool (without stats), this could fail whenever objects had been
	// dynamically allocated, before. We could make ValidateReset() a member function
	// only of atDynaPoolBaseWithStats, but then we'd miss out on the validation that
	// is possible without StatsGetNumAllocatedDynamically(), or we could make
	// atDynaPoolBaseWithoutStats also implement StatsGetNumAllocatedDynamically(),
	// but that doesn't seem completely right if it's only needed for a validation check.
	// I ended up requiring a perfect match if StatsGetNumAllocatedDynamically() returned
	// non-zero. /FF
	if((dynamicallyAllocated && numFreeObjects != totalAllocated) || (numFreeObjects < totalAllocated))
	{
		// This used to be Quitf(), but non-ignorable fatal errors can be very
		// annoying and hold people up. This Verifyf()/Errorf() combo is
		// hopefully enough to indicate that it's a serious issue that should be
		// fixed. /FF
		// Once we get to the point where Errorf's can safely cause popups in the new system,
		// this can be collapsed to just an Errorf.
#if __ASSERT
		Assertf(0, "atDynaPoolBase:: %d _Type objects still in use.", totalAllocated - numFreeObjects);
#else
		Errorf("atDynaPoolBase:: %d _Type objects still in use.", totalAllocated - numFreeObjects);
#endif
	}
#endif
}

//////////////////////////////////////////////////////////////////////////

template <class _Type, class _Base, bool _Destruct, bool _Construct>
_Type &atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::CreateObject()
{
	InternalNode *node = m_FreeNodePool;

#if __ASSERT
	_Base::StatsVerifyHasName();
#endif // __ASSERT

	if(node)
	{
		m_FreeNodePool = node->m_Next;
		_Base::StatsDecNumRemaining();
	}

	if(!node)
	{
#if RAGE_TRACKING && defined(_CPPRTTI)
		::rage::diagTrackerHelper trackDynaPool(typeid(_Type).name());
#else // RAGE_TRACKING && defined(_CPPRTTI)
		RAGE_TRACK(DynaPool);
#endif // !RAGE_TRACKING && defined(_CPPRTTI)

#if !__FINAL
		_Base::StatsWarnAboutDynamicAllocation(m_NumObjectsInArray);	
#endif
		node = rage_new InternalNode;
		_Base::StatsIncNumAllocatedDynamically();
	}

	_Base::StatsIncNumUsed();

	_Type* object = reinterpret_cast<_Type*>(&(node->m_ObjectBuffer));

	// Invoke placement new.
	if (_Construct)
	{
		new	(object) _Type;	
	}

	return *object;
}

//////////////////////////////////////////////////////////////////////////

template <class _Type, class _Base, bool _Destruct, bool _Construct>
void atDynaPoolBase<_Type, _Base, _Destruct, _Construct>::DestroyObject(_Type &object)
{
	if (_Destruct)
	{
		// Explicitly call the destructor to match the placement new call in CreateObject.
		object.~_Type();
	}

#if !__FINAL
	sysMemSet(&object, 0xd7, sizeof(_Type));
#endif // !__FINAL

	// Note: for objects that were allocated dynamically, we could
	// delete them here (detecting them by comparing the address
	// against the array). However, instead we put them back in the
	// pool, since chances are that they will be needed again soon
	// anyway.

	InternalNode* node = reinterpret_cast<InternalNode*>(&object);
	node->m_Next = m_FreeNodePool;
	m_FreeNodePool = node;
	
	_Base::StatsNoteDestruction();
}

//////////////////////////////////////////////////////////////////////////
//
// PURPOSE:
//		This class represents the standard form of an atDynaPool,
//		which implements a pool of objects with a fixed "best-guess" 
//		initial size but also with the ability to grow arbitrarily 
//		using run-time allocation.  
//	NOTE:
//		Unlike with an atArray, these objects won't end up moving to a new
//		location in memory when the pool needs to allocate new elements.
//
template <class _Type, bool _Destruct=true, bool _Construct=true>
class atDynaPool : public atDynaPoolBase<_Type, atDynaPoolBaseWithoutStats, _Destruct, _Construct>
{
public:
	atDynaPool(int maxNumObjects) 
		: atDynaPoolBase<_Type, atDynaPoolBaseWithoutStats, _Destruct, _Construct>(maxNumObjects) 
	{	}
};

//////////////////////////////////////////////////////////////////////////
//
// PURPOSE:
//		This class represents the standard form of an atDynaPool, 
//		plus statistics tracking.
//
template <class _Type, bool _Destruct=true, bool _Construct=true>
class atDynaPoolWithStats : public atDynaPoolBase<_Type, atDynaPoolBaseWithStats, _Destruct, _Construct>
{	
public:
	atDynaPoolWithStats(int maxNumObjects, const char *debugName = NULL) 
		: atDynaPoolBase<_Type, atDynaPoolBaseWithStats, _Destruct, _Construct>(maxNumObjects) 
	{
		RAGE_TRACK(DynaPoolName);

		if (debugName)
		{
			this->SetName(debugName);
		}
		else
		{
#if defined(_CPPRTTI)
			this->SetName(typeid(_Type).name());
#endif // defined(_CPPRTTI)
		}
	}
};

//////////////////////////////////////////////////////////////////////////

}	// namespace rage

//////////////////////////////////////////////////////////////////////////

#endif

/* End of file atl/dynapool.h */
