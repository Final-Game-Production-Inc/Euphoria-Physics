//
// physics/levelbase.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_LEVELBASE_H
#define PHYSICS_LEVELBASE_H

// This controls whether or not we use a special vectorized version of phLooseOctreeNode::DoesSphereIntersectSpecial().
#define PHLEVEL_VECTORIZATION_TEST 1

////////////////////////////////////////////////////////////////
// external defines

#include "lockconfig.h"

#include "data/base.h"
#include "system/bit.h"

#if ENABLE_PHYSICS_LOCK
#if __PS3
#else
#include "system/criticalsection.h"
#endif
#endif

namespace rage {

class bkBank;
class phInst;


#define LEVELBASE_SPHERECHECKCNTS 0


#define MAX_BATCHED_PROBES 64



////////////////////////////////////////////////////////////////
// phLevelBase

// PURPOSE
//	Base class for physics levels.  It stores lists of objects according to their physical state.  The main states
//	are active, inactive, fixed and static.  Organization of the objects (such as in rooms) is defined in the derived
//	class phLevelSimple.
//
//	Specifies the minimal interface for a physics level to implement.  Note that this minimal interface is currently too limited
//	to be fully useful by itself.  For example, the interface for iterating is not specified at this level as there are two different interfaces
//	implemented by phLevelSimple and phLevelNew.  The one implemented by phLevelNew is the preferable one and will eventually be lifted up to
//	this level once phLevelSimple is completely dead.  As another example, the new physics level does not deal with colliders, this is handled
//	at the phSimulator level, whereas the old physics level handled phColliders directly.  This resulted in differing AddXXX methods.
//	<FLAG Component>
class phLevelBase
#if !__SPU
    : public datBase
#endif
{
#if __SPU
private:
	// Because we're deriving from datBase on non-SPU builds we'd get a v-table in those cases, but not on SPU builds.  In order to preserve data layout
	//   across SPU and non-SPU builds (because SPUs DMA objects over) we add a padding member here
	// Note that, due to alignment, this four-byte member is actually taking sixteen bytes, which is the same as what would happen if the v-table were
	//   present.  If we were to ever be resourcing this class, there might be a warning about the missing padding in the structure.
	u32	m_MissingVTablePad;
#endif

public:
	// flags corresponding to the object states
	enum eObjectState
	{
		OBJECTSTATE_ACTIVE = 0,
		OBJECTSTATE_INACTIVE,
		OBJECTSTATE_FIXED,
		OBJECTSTATE_NONEXISTENT,

		OBJECTSTATE_CNT
	};

		enum eObjectInactiveCollisionFlags
		{
				COLLISIONSTATE_VS_INACTIVE = 1,
				COLLISIONSTATE_VS_FIXED = 2,
		};

		enum eObjectInactiveCollisionFlagBits
		{
				COLLISIONSTATE_VS_INACTIVE_BIT = COLLISIONSTATE_VS_INACTIVE << 2,
				COLLISIONSTATE_VS_FIXED_BIT = COLLISIONSTATE_VS_FIXED << 2,
		};

	enum eNewObjectIncludeFlag
	{
		STATE_FLAG_ACTIVE = BIT(OBJECTSTATE_ACTIVE),
		STATE_FLAG_INACTIVE = BIT(OBJECTSTATE_INACTIVE),
		STATE_FLAG_FIXED = BIT(OBJECTSTATE_FIXED),
		STATE_FLAG_NONEXISTENT = BIT(OBJECTSTATE_NONEXISTENT),
	};

	enum { INVALID_STATE_INDEX = 0xFFFF };

	enum { STATE_FLAGS_ALL = (STATE_FLAG_ACTIVE | STATE_FLAG_INACTIVE | STATE_FLAG_FIXED) };

public:
	////////////////////////////////////////////////////////////
	phLevelBase();														// constructor
	~phLevelBase();												        // destructor

	void Reset ();												// reset

	// PURPOSE
	//   Set the level back to the state it was in when it was first initialized (aka, make there be no objects).
	void Clear ();

	////////////////////////////////////////////////////////////
	// initializing

    // PURPOSE
	//    Prepare the physics level for use.
	// NOTES
	//    Must be called before the level is used, after configuration.
	// SEE ALSO
	//    SetMaxActive
	void Init ();											// init structures: non-default configuration needs to be set first

    // PURPOSE
    //    Find out if Init has been called
    // RETURNS
    //    True if Init has been called since creation, without a corresponding Shutdown call.
//    bool IsInitialized();

	// PURPOSE
	//    Inform the physics level that it will no longer be used.
	void Shutdown ();										// called before blowing away heap

	////////////////////////////////////////////////////////////
	// configuring

	// PURPOSE
	//    Tell the physics level how many active objects it must allow.
	// PARAMS
	//    maxActive - The number of active objects that need to be supported.
	// NOTES
	//    Must be called before Init
	// SEE ALSO
	//    Init
	void SetMaxActive (int maxActive);

	////////////////////////////////////////////////////////////
	// accessing...

	// PURPOSE
	//    Find out if a level index corresponds to an instance that has been Added
	// PARAMS
	//    levelIndex - The level index of the possibly valid instance
	bool LegitLevelIndex(int nLevelIndex) const;

	////////////////////////////////////////////////////////////
	// active state

	// PURPOSE
	//    Refuse to add new active objects after this point, making them pending active instead
	// SEE ALSO
	//    PostCollideActives, GetFrozenActiveState
	void FreezeActiveState();

	// PURPOSE
	//    Return whether the active state is currently frozen.
	// SEE ALSO
	//    PostCollideActives, GetFrozenActiveState
	bool GetFrozenActiveState() const								{ return m_FrozenActiveState; }

	///////////////////////
	// accessors

	// PURPOSE
	//    Return the number of objects currently active
	// RETURNS
	//    The current number of active objects
	int GetNumActive() const;

	// PURPOSE
	//    Return the number of objects currently inactive
	// RETURNS
	//    The current number of inactive objects
	int GetNumInactive() const;

	// PURPOSE
	//    Return the number of fixed objects
	// RETURNS
	//    The current number of fixed objects
	int GetNumFixed() const;

	int GetNumObjects() const;

	// PURPOSE
	//    Return the maximum number of active objects
	// RETURNS
	//    The current number of active objects
	// SEE ALSO
	//    SetMaxActive
	int GetMaxActive() const;

	int GetBitFromExistentState(eObjectState objectState) const;

	////////////////////////////////////////////////////////////
	// debugging
#if LEVELBASE_SPHERECHECKCNTS
	mutable int m_nTotalSphereCheckCnt, m_nPassedSphereCheckCnt;
	int GetTotalSphereCheckCnt() const { return m_nTotalSphereCheckCnt; }
	int GetPassedSphereCheckCnt() const { return m_nPassedSphereCheckCnt; }
	void ResetSphereCheckCnts()
	{
		m_nTotalSphereCheckCnt = 0;
		m_nPassedSphereCheckCnt = 0;
	}
#endif

	////////////////////////////////////////////////////////////
	// bank
#if __BANK
	static void AddWidgets(bkBank & bank);						// add bank widgets
#endif

	////////////////////////////////////////////////////////////
	// warning control
	static bool GetWarningFlag(u32 mask)							{ return (sm_WarningFlags & mask)!=0; }
	static void SetWarningFlags(u32 flags)							{ sm_WarningFlags = flags; }
	static void SetWarningFlag(u32 mask, bool value)				{ sm_WarningFlags = (value?(sm_WarningFlags|mask):(sm_WarningFlags&~mask)); }
	enum eWarnings
	{
		WARN_SUBCLASS_0 = (0x01 << 0),								// first warning for derived phLevels
		WARN_OCTREE_NODE_LIMIT = (0x01 << 1)
	};

protected:
	////////////////////////////////////////////////////////////
	// class variables
	static u32 sm_WarningFlags;										// warning control
	static int sm_DebugLevel;											// debug verbosity (0, none)

	////////////////////////////////////////////////////////////
	// phLevel data
	bool m_FrozenActiveState;											// is the active state frozen

	////////////////////////////////////////////////////////////
	// total objects
	u16 m_MaxObjects;												// max total objects
	u16 m_MaxActive;												// max active objects

	u16 m_NumObjects;												// total number of objects in the level
	u16 m_NumFixed;													// current num fixed objects
	u16 m_NumInactive;												// current num inactive objects
	u16 m_NumActive;												// current num active objects

	s16 m_FrozenNumActive;											// the number of active objects at the time of freezing
} ;

//inline bool phLevelBase::IsInitialized()
//{
//    return m_InstanceStates != NULL;
//}

inline void phLevelBase::SetMaxActive (int maxActive)
{
//    FastAssert(!IsInitialized());
	m_MaxActive = (u16)(maxActive);
}

inline bool phLevelBase::LegitLevelIndex(int nLevelIndex) const
{
	return (nLevelIndex >= 0 && nLevelIndex < m_MaxObjects);
}

inline int phLevelBase::GetNumActive () const
{
	return m_NumActive;
}

inline int phLevelBase::GetNumInactive () const
{
	return m_NumInactive;
}

inline int phLevelBase::GetNumFixed () const
{
	return m_NumFixed;
}

inline int phLevelBase::GetNumObjects () const
{
	return m_NumObjects;
}


inline int phLevelBase::GetMaxActive() const
{
	return m_MaxActive;
}


__forceinline int phLevelBase::GetBitFromExistentState(eObjectState objectState) const
{
#if __ASSERT
	Assertf(objectState < OBJECTSTATE_NONEXISTENT, "Found object with bad state %i while iterating over the level.", objectState);
#else
	FastAssert(objectState < OBJECTSTATE_NONEXISTENT);
#endif
	const int retVal = ((objectState + objectState + objectState) >> 1) + 1;
	return retVal;
}


} // namespace rage

#endif
