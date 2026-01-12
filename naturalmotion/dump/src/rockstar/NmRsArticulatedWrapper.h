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

#ifndef NM_ROCKSTAR_ARTICWRAPPER_H
#define NM_ROCKSTAR_ARTICWRAPPER_H

#include "physics/inst.h"
#include "physics/leveldefs.h"
#include "NmRsBodyLayout.h"

namespace rage
{
  class phSimulator;
  class phCollider;
  class phBound;
  class phArticulatedCollider;
  class phJoint1Dof;
  class phJoint3Dof;
  class Matrix34;
}

namespace ART
{
     class NmRsGenericPart;

    /**
     * NmRsArticulatedWrapper is a management class that encapsulates a complete chain
     * of articulated parts in the Rockstar physics simulation. We will usually have
     * one of these per character (in the clump) as the character can be described 
     * by one continuous branching articulated chain
     */
    class NmRsArticulatedWrapper
    {
    public:

      NmRsArticulatedWrapper(ART::MemoryManager* services, unsigned int maxParts, rage::phArticulatedCollider *inputCollider = NULL);
      ~NmRsArticulatedWrapper();

      /**
       * called during deserialize to add a part to the list
       */
      void addPart(NmRsGenericPart *part);

      /**
       * called as the last step in the deserialize on the new
       * NmRsArticulatedWrapper object, connects with the parts assigned
       */
      void postDeserialize();

      /**
       * add the wrapper contents to the simulator
       */
      void addToSimulator(rage::phLevelNew *level, rage::phSimulator *sim);

      /**
       * remove the wrapper contents from the simulator
       */
      void removeFromSimulator(rage::phSimulator *sim);//unused?


      inline int getNumberOfJoints() const { return m_articulatedBody->GetNumJoints(); }

      inline void setArchetype(rage::phArchetypePhys* archetype) { m_archetype = archetype; }
      inline rage::phArchetypePhys* getArchetype() const { return m_archetype; }

      void setArticulatedPhysInstance(rage::phInst* inst, bool doReset = true);
      inline rage::phInst* getArticulatedPhysInstance() const { return m_articulatedPhysInstance; }

      inline void setInitialMatrix(const rage::Matrix34 &init) { m_initialMtx.Set(init); }
      inline rage::Matrix34 *getInitialMatrix() { return &m_initialMtx; }

      inline rage::phArticulatedCollider* getArticulatedCollider() const { return m_articulatedCollider; }
      inline rage::phArticulatedBody* getArticulatedBody() const { return m_articulatedBody; }

#if NM_SET_WEAPON_BOUND
      void  setBound(int index, rage::phBound* bound);
#endif
      rage::phBound* getBound(int index);

    protected:

      /**
       * release any owned RAGE objects & data, called during dtor
       */
      void releaseRAGEObjects();

      rage::Matrix34                  m_initialMtx;
      rage::phLevelNew               *m_level;
      int                             m_colliderIndexInLevel;
      NmRsGenericPart               **m_parts;
      unsigned int                    m_partIndex,
                                      m_maxParts;

      rage::phArticulatedBody        *m_articulatedBody;
      rage::phInst                   *m_articulatedPhysInstance;
      rage::phArchetypePhys          *m_archetype;
      rage::phArticulatedCollider    *m_articulatedCollider;

      ART::MemoryManager      *m_artMemoryManager;
    };

    extern void recursiveDeleteBounds(rage::phBound *bound);
    extern void deletePhInst(rage::phInst *phInst);
}

#endif // NM_ROCKSTAR_ARTICWRAPPER_H

