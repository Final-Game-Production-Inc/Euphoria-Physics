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

#ifndef NM_RS_CBU_TASKBASE_H
#define NM_RS_CBU_TASKBASE_H

#include "art/ARTMemory.h"
#include "NmRsCBU_Shared.h"
#include "NmRsLimbs.h"

#define SCOPED_BODY_CONTEXT(_subTask, _subPriority) \
  NmRsBodyStateHelper helper(m_body, m_bvid, m_priority, _subPriority, m_blend, m_mask DEBUG_LIMBS_PARAMETER(#_subTask));

namespace ART
{
  class MemoryManager;

  // Base class for all CBU tasks defining the minimal interface required.
  MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) class CBUTaskBase
  {
  public:

    enum UpdatePhase
    {
      kInvalidPhase,
      kActivate,
      kTick,
      kDeactivate,
      kNumUpdatePhases
    };

    CBUTaskBase(ART::MemoryManager *services, BehaviourID bvid) :
      m_bvid(bvid),
      m_mask(bvmask_Full),
      m_blend(1.0f),
      m_artMemoryManager(services),
      m_character(0),
      m_active(false),
      m_priority(bvid), // Defaults to previous tick order.
      m_updatePhase(kInvalidPhase),
      m_body(0)
      {}
    virtual ~CBUTaskBase() {}

    virtual void init(class NmRsCharacter* character, CBURecord* cbuParent);
    virtual void onActivate() = 0;
    void activate();
    virtual void onDeactivate() = 0;
    void deactivate();
    virtual void term();

    BehaviourID getBvID() const { return m_bvid; }

    // Tick the behaviour by <tt>timeStep</tt> seconds.
    CBUTaskReturn tick(float timeStep);
    virtual CBUTaskReturn onTick(float timeStep) = 0;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    // Return some indication of whether this task is running / active or not.
    bool isActive() const { return m_active; }

    NmRsCharacter *getCharacter(){ return m_character; }
    virtual void updateBehaviourMessage(const MessageParamsBase* const params) { (void)params; } // TDL could make this pure eventually

    void setPriority(int priority) { m_priority = priority; }
    void setMask(int mask) { m_mask = mask; }
    void setBlend(float blend) { m_blend = blend; }

  protected:

    template <typename T>
#if ART_ENABLE_BSPY_LIMBS
    inline NmRsLimbInput createNmRsLimbInput(int subPriority = 0, float weight = 1.0f, const char* subTask = 0)
#else
    inline NmRsLimbInput createNmRsLimbInput(int subPriority = 0, float weight = 1.0f)
#endif
    {
      // tweak message priority based on which tick phase is active. activate
      // should be slightly lower priority so tick will layer over it.
      // deactivate should be slightly higher priority for similary reasons.
      //
      // todo implement priority offset as separate int value. change priority
      // to int as well. test that layering works correctly
      
      if(m_updatePhase == kActivate)
      {
        subPriority -= 10;
       }
      else if(m_updatePhase == kDeactivate)
      {
        subPriority += 10;
      }
#if ART_ENABLE_BSPY_LIMBS
      return m_body->createNmRsLimbInput<T>(
        subPriority,
        weight,
        bvmask_Full,
        subTask ? subTask : s_phaseNames[m_updatePhase]);
#else
      return m_body->createNmRsLimbInput<T>(subPriority, weight, bvmask_Full);
#endif
    }

    virtual void initialiseCustomVariables()=0;

    BehaviourID                 m_bvid;
    BehaviourMask               m_mask;
    float                       m_blend;
    bool                        m_active;
    ART::MemoryManager         *m_artMemoryManager;
    NmRsCharacter              *m_character;
    CBURecord                  *m_cbuParent;

    // Replaces task manager tick order in limb system.
    int                         m_priority;

    inline NmRsHumanArm*   getLeftArm()  { return static_cast<NmRsHumanArm*>(m_body->getLimb(kLeftArm)); }
    inline NmRsHumanArm*   getRightArm() { return static_cast<NmRsHumanArm*>(m_body->getLimb(kRightArm)); }
    inline NmRsHumanLeg*   getLeftLeg()  { return static_cast<NmRsHumanLeg*>(m_body->getLimb(kLeftLeg)); }
    inline NmRsHumanLeg*   getRightLeg() { return static_cast<NmRsHumanLeg*>(m_body->getLimb(kRightLeg)); }
    inline NmRsHumanSpine* getSpine()    { return static_cast<NmRsHumanSpine*>(m_body->getLimb(kSpine)); }

    inline const NmRsHumanArm*   getLeftArm()  const { return static_cast<const NmRsHumanArm*>(m_body->getLimb(kLeftArm)); }
    inline const NmRsHumanArm*   getRightArm() const { return static_cast<const NmRsHumanArm*>(m_body->getLimb(kRightArm)); }
    inline const NmRsHumanLeg*   getLeftLeg()  const { return static_cast<const NmRsHumanLeg*>(m_body->getLimb(kLeftLeg)); }
    inline const NmRsHumanLeg*   getRightLeg() const { return static_cast<const NmRsHumanLeg*>(m_body->getLimb(kRightLeg)); }
    inline const NmRsHumanSpine* getSpine()    const { return static_cast<const NmRsHumanSpine*>(m_body->getLimb(kSpine)); }

  protected:

    inline NmRsLimbInput& getLeftArmInput()  { return m_body->getLeftArmInput(); }
    inline NmRsLimbInput& getRightArmInput() { return m_body->getRightArmInput(); }
    inline NmRsLimbInput& getLeftLegInput()  { return m_body->getLeftLegInput(); }
    inline NmRsLimbInput& getRightLegInput() { return m_body->getRightLegInput(); }
    inline NmRsLimbInput& getSpineInput()    { return m_body->getSpineInput(); }

    inline NmRsArmInputWrapper* getLeftArmInputData()  { return m_body->getLeftArmInputData(); };
    inline NmRsArmInputWrapper* getRightArmInputData() { return m_body->getRightArmInputData(); };
    inline NmRsLegInputWrapper* getLeftLegInputData()  { return m_body->getLeftLegInputData(); };
    inline NmRsLegInputWrapper* getRightLegInputData() { return m_body->getRightLegInputData(); };
    inline NmRsSpineInputWrapper* getSpineInputData()  { return m_body->getSpineInputData(); };

    UpdatePhase                 m_updatePhase;

    NmRsHumanBody* m_body;

  } NM_ALIGN_SUFFIX(16);
  MSVCEndWarningMacroBlock()

} // namespace ART

#endif //NM_RS_CBU_TASKBASE_H
