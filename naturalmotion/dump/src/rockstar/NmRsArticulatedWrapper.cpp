/*
 * Copyright (c) 2005-2012 NaturalMotion Ltd. All rights reserved. 
 *
 * Not to be copied, adapted, modified, used, distributed, sold,
 * licensed or commercially exploited in any manner without the
 * written consent of NaturalMotion. 
 *
 * All non public elements of this software are the confidential
 * information of NaturalMotion and may not be disclosed to any
 * person nor used for any purpose not expressly approved by
 * NaturalMotion in writing.
 *
 */


#include "NmRsInclude.h"
#include "NmRsArticulatedWrapper.h"
#include "NmRsGenericPart.h"

#include "physics/levelnew.h"

namespace ART
{
  NmRsArticulatedWrapper::NmRsArticulatedWrapper(ART::MemoryManager* services, unsigned int maxParts, rage::phArticulatedCollider *inputCollider) : m_level(0),
    m_colliderIndexInLevel(NM_RS_INVALID_LVL_INDEX),
    m_parts(0), 
    m_partIndex(0),
    m_maxParts(maxParts),
    m_articulatedPhysInstance(0)
  {
    m_artMemoryManager = services;
    Assert(inputCollider);

    m_parts = (NmRsGenericPart**)m_artMemoryManager->callocate(sizeof(NmRsGenericPart*) * maxParts, NM_MEMORY_TRACKING_ARGS);

    m_articulatedCollider = inputCollider;
    Assert(inputCollider->GetBody());
    m_articulatedBody = inputCollider->GetBody();
  }

  NmRsArticulatedWrapper::~NmRsArticulatedWrapper()
  {
    if (m_parts)
      m_artMemoryManager->deallocate(m_parts, NM_MEMORY_TRACKING_ARGS);
    m_parts = 0;
  }

  /**
  * add the contents of this wrapper into the given simulator
  */
  void NmRsArticulatedWrapper::addToSimulator(rage::phLevelNew *level, rage::phSimulator *sim)
  {
    Assert(m_level == 0);

    rage::phBoundComposite *allPartBound = (rage::phBoundComposite*)m_articulatedPhysInstance->GetArchetype()->GetBound();
    allPartBound->CalculateCompositeExtents(true);
	if(allPartBound->HasBVHStructure())
	{
    if(m_articulatedPhysInstance->IsInLevel())
    {
      PHLEVEL->RebuildCompositeBvh(m_articulatedPhysInstance->GetLevelIndex());
    }
    else
    {
      allPartBound->UpdateBvh(true);
    }
	}

    if (m_articulatedCollider->GetInstance() && 
      m_colliderIndexInLevel == NM_RS_INVALID_LVL_INDEX)
      m_colliderIndexInLevel = sim->AddActiveObject(m_articulatedCollider);

    m_level = level;
  }

  void NmRsArticulatedWrapper::removeFromSimulator(rage::phSimulator *sim)//unused?
  {
    Assert(m_level != 0);

    if (m_colliderIndexInLevel != NM_RS_INVALID_LVL_INDEX &&
      m_articulatedCollider->GetInstance() &&
      m_articulatedCollider->GetInstance()->IsInLevel())
      sim->DeleteObject(m_colliderIndexInLevel, true);

    m_colliderIndexInLevel = NM_RS_INVALID_LVL_INDEX;

    m_level = 0;
  }

  void NmRsArticulatedWrapper::setArticulatedPhysInstance(rage::phInst* inst, bool doReset)
  {
    m_articulatedPhysInstance = inst;
    setInitialMatrix(RCC_MATRIX34(inst->GetMatrix()));
    m_articulatedPhysInstance->SetMatrix(RCC_MAT34V(m_initialMtx));
    m_articulatedPhysInstance->SetArchetype(m_archetype);
    if (m_articulatedCollider->GetInstance() && doReset)
      m_articulatedCollider->SetInstanceAndReset(inst);
  }

  void NmRsArticulatedWrapper::addPart(NmRsGenericPart *part)
  {
    Assert(m_partIndex < m_maxParts);
    m_parts[m_partIndex++] = part;
  }

  void NmRsArticulatedWrapper::postDeserialize()
  {
    if (m_partIndex == 0)
      return;

    Assert(m_articulatedPhysInstance);
    rage::phBoundComposite *allPartBound = (rage::phBoundComposite*)m_articulatedPhysInstance->GetArchetype()->GetBound();
    Assert(allPartBound);

    int matrixIndex = 0;
    unsigned int i;
    for (i=0; i<m_partIndex; i++)
    {
      NmRsGenericPart *part = m_parts[i];
      part->associateMatrixPointer(
          &const_cast<rage::Matrix34&>(RCC_MATRIX34(allPartBound->GetCurrentMatrix(matrixIndex))),
          &const_cast<rage::Matrix34&>(RCC_MATRIX34(allPartBound->GetLastMatrix(matrixIndex))));

      matrixIndex++;
    }

	  allPartBound->CalculateCompositeExtents(true);	  
    }

  void NmRsArticulatedWrapper::releaseRAGEObjects()
  {
#ifndef ON_DEMAND_AGENT
    if (m_articulatedPhysInstance)
    {   
      rage::phArchetypePhys *arch = (rage::phArchetypePhys *)m_articulatedPhysInstance->GetArchetype();

      rage::phBound* bound = arch->GetBound();
      arch->SetBound(NULL);
      recursiveDeleteBounds(bound);

      arch->AddRef();
      m_articulatedPhysInstance->SetArchetype(NULL);
      arch->Release(false);
      ARTCustomPlacementDelete(arch, phArchetypePhys);
      deletePhInst(m_articulatedPhysInstance);
    }
#endif //not ON_DEMAND_AGENT 
    for (int i = m_articulatedBody->GetNumJoints(); i >= 0; --i)
    {
      m_articulatedBody->RemoveChild(i);
    }
  }

#if NM_SET_WEAPON_BOUND
  void  NmRsArticulatedWrapper::setBound(int index, rage::phBound* bound)
  {
    Assert(bound);
    rage::phBoundComposite *allPartBound = (rage::phBoundComposite*)m_articulatedPhysInstance->GetArchetype()->GetBound();
    Assert(index > -1 && index < allPartBound->GetMaxNumBounds());
    allPartBound->SetBound(index, bound);
    allPartBound->CalculateCompositeExtents(true);
	if(allPartBound->HasBVHStructure())
	{
    if(m_articulatedPhysInstance->IsInLevel())
    {
      PHLEVEL->RebuildCompositeBvh(m_articulatedPhysInstance->GetLevelIndex());
    }
    else
    {
      allPartBound->UpdateBvh(true);
    }
	}
  }
#endif

  rage::phBound* NmRsArticulatedWrapper::getBound(int index)
  {
    rage::phBoundComposite *allPartBound = (rage::phBoundComposite*)m_articulatedPhysInstance->GetArchetype()->GetBound();
    Assert(index > -1 && index < allPartBound->GetMaxNumBounds());
    return allPartBound->GetBound(index);
  }
}

