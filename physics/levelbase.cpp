//
// physics/levelbase.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "levelbase.h"

#include "bank/bank.h"


#if __PPU
#pragma comment(lib,"sync_stub")
#endif

namespace rage {

#if ENABLE_PHYSICS_LOCK
#if !__SPU
	phMultiReaderLockToken g_GlobalPhysicsLock;

	// This is necessary because multiple write locks *on the same thread* are permitted, and it's also okay to acquire a read lock from the same thread that
	//   acquired the write lock.
	__THREAD int g_PerThreadWriteLockCount = 0;
	__THREAD int g_PerThreadReadLockCount = 0;
	__THREAD int g_PerThreadAllowNewReaderLockCount = 0;
#endif
	bool g_OnlyMainThreadIsUpdatingPhysicsLockConfig = true;
#endif

////////////////////////////////////////////////////////////////
// static vars

u32 phLevelBase::sm_WarningFlags = 0xFFFFFFFF;
//phLevelBase * phLevelBase::ActiveInstance = NULL;
int phLevelBase::sm_DebugLevel = 0;

#if __BANK
static int sm_NumActive = 0;
static int sm_NumInactive = 0;
static int sm_NumFixed = 0;
#endif


phLevelBase::phLevelBase()
{
	m_MaxObjects = 0;
	m_MaxActive = 0;

	m_NumObjects = 0;
	m_NumActive = 0;
	m_NumInactive = 0;
	m_NumFixed = 0;

	m_FrozenActiveState = false;

#if LEVELBASE_SPHERECHECKCNTS
	m_nTotalSphereCheckCnt = 0;
	m_nPassedSphereCheckCnt = 0;
#endif
}

phLevelBase::~phLevelBase()
{
//    Assert(!IsInitialized());
}

void phLevelBase::Init()
{
//    Assert(!IsInitialized());
}

void phLevelBase::Shutdown()
{
//    Assert(IsInitialized());
}

void phLevelBase::Reset()
{
}


void phLevelBase::Clear()
{
	m_NumObjects = 0;
	m_NumActive = 0;
	m_NumInactive = 0;
	m_NumFixed = 0;

	m_FrozenActiveState = false;
}


void phLevelBase::FreezeActiveState ()
{
	m_FrozenNumActive = m_NumActive;
#if __BANK
    sm_NumActive = m_NumActive;
    sm_NumInactive = m_NumInactive;
    sm_NumFixed = m_NumFixed;
#endif
	m_FrozenActiveState = true;
}


#if __BANK && !__SPU
void phLevelBase::AddWidgets (bkBank & bank)
{
	bank.AddSlider("Debug Level",&sm_DebugLevel,0,5,1);
    bank.AddSlider("Active Objects", &sm_NumActive, 0, 1024, 0);    
    bank.AddSlider("Inactive Objects", &sm_NumInactive, 0, 1000000, 0);
    bank.AddSlider("Fixed Objects", &sm_NumFixed, 0, 1000000, 0);
}
#endif

} // namespace rage
