// 
// physics/debugcontext.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "debugcontext.h"
#include "levelnew.h"
#include "archetype.h"
#include "collider.h"
#include "simulator.h"
#include "manifold.h"

#if ENABLE_PH_DEBUG_CONTEXT

namespace rage {

__THREAD const phDebugContext* phDebugContext::sm_pCurrentContext = NULL;

void phDebugContext::PushContext()
{
	m_pPreviousContext = sm_pCurrentContext;
	sm_pCurrentContext = this;
}

phDebugContext::phDebugContext(u16 levelIndex)
{
	m_LevelIndex = levelIndex;
	m_LevelIndexGeneration = (m_LevelIndex != phInst::INVALID_INDEX) ? PHLEVEL->GetGenerationID(levelIndex) : 0xffff;
	PushContext();
}

phDebugContext::phDebugContext(int levelIndex)
{
	Assertf(PHLEVEL->LegitLevelIndex(levelIndex), "Invalid level index passed in as context");
	Assign(m_LevelIndex, levelIndex);
	m_LevelIndexGeneration = (m_LevelIndex != phInst::INVALID_INDEX) ? PHLEVEL->GetGenerationID(levelIndex) : 0xffff;
	PushContext();
}

phDebugContext::phDebugContext(const phInst* pInst)
{
	if (pInst)
	{
		m_LevelIndex = pInst->GetLevelIndex();
		m_LevelIndexGeneration = (m_LevelIndex != phInst::INVALID_INDEX) ? PHLEVEL->GetGenerationID(m_LevelIndex) : 0xffff;
	}
	else
	{
		m_LevelIndex = m_LevelIndexGeneration = 0xffff;
	}
	PushContext();
}

phDebugContext::~phDebugContext()
{
	sm_pCurrentContext = m_pPreviousContext;
}

} // namespace rage
#endif // !__FINAL
