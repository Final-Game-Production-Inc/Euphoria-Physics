// 
// physics/debugcontext.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_DEBUGCONTEXT_H 
#define PHYSICS_DEBUGCONTEXT_H 

#include "system/tls.h"

#define ENABLE_PH_DEBUG_CONTEXT __DEV

#if !ENABLE_PH_DEBUG_CONTEXT
#define PH_DEBUG_CONTEXT(x) 
#define PH_DEBUG_CONTEXT_ONLY(x) 
#else

namespace rage {

class phInst;
class phArchetype;
class phBound;
class phCollider;
class phManifold;

class phDebugContext
{
public:
	phDebugContext(u16 levelIndex);
	phDebugContext(int levelIndex);
	phDebugContext(const phInst* pInst);
	
	~phDebugContext();

	u16 GetGeneration() const
	{
		return m_LevelIndexGeneration;
	}

	u16 GetLevelIndex() const
	{
		return m_LevelIndex;
	}

	static const phDebugContext* GetCurrentContext() { return sm_pCurrentContext; }
	const phDebugContext* GetParentContext() const { return m_pPreviousContext; }
		
private:
	phDebugContext() {}
	void PushContext();

	const phDebugContext* m_pPreviousContext;
	u16 m_LevelIndex;
	u16 m_LevelIndexGeneration;
	__THREAD static const phDebugContext* sm_pCurrentContext;
};

// Use this define to create a local debug context variable.
#define PH_DEBUG_CONTEXT(x) phDebugContext debugContext(x)
#define PH_DEBUG_CONTEXT_ONLY(x) x


} // namespace rage

#endif // __FINAL
#endif // PHYSICS_DEBUGCONTEXT_H 
