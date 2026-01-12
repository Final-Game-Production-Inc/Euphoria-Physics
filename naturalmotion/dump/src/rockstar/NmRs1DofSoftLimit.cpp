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
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"

namespace ART
{

#if NM_USE_1DOF_SOFT_LIMITS

  SoftLimitController::SoftLimitController()
    : m_character(NULL),
      m_eff(NULL),
      m_stiffness(0.0f),
      m_damping(0.0f),
      m_limitAngle(0.0f),
      m_approachDirection(1),
      m_combinedMass(0.0f),
      m_momentArm(0.0f),
      m_velocityScaled(false),
      m_enabled(false)
  {}

  SoftLimitController::~SoftLimitController()
  {}

  void SoftLimitController::init(NmRsCharacter *character, NmRs1DofEffector *eff)
  {
    Assert(character);
    Assert(eff);

    m_character = character;
    m_eff = eff;

    // Once the effector has been obtained pre-calculate values that do not change
    // during the tick().
    preCalcConstants();
  }

  // Pre-calculate combined mass, moment arm and limit max span.
  void SoftLimitController::preCalcConstants()
  {
    // Parent part mass.
    int parentPartIndex = m_eff->getParentIndex();
    const float parentPartMass = m_character->getArticulatedBody()->GetMass(parentPartIndex).Getf();

    // Child part mass.
    int childPartIndex =  m_eff->getJointIndex() + 1;  // NOTE: NM ragdoll model assumption:
                                                       // child part of a joint has partIndex = jointIndex + 1;
    const float childPartMass = m_character->getArticulatedBody()->GetMass(childPartIndex).Getf();

    // Combined mass.
    // Make it so that if one of the parts has zero mass the resultant torque is zero.
    m_combinedMass = (parentPartMass * childPartMass) / (parentPartMass + childPartMass);

    // Moment arm.
    NmRsGenericPart* parentPart = m_character->getGenericPartByIndex(parentPartIndex);
    NmRsGenericPart* childPart = m_character->getGenericPartByIndex(childPartIndex);
    Assert(parentPart);
    Assert(childPart);

    const float eff2ChildPartComDst = (childPart->getPosition() - m_eff->getJointPosition()).Mag();
    const float eff2ParentPartComDst = (parentPart->getPosition() - m_eff->getJointPosition()).Mag();
    m_momentArm = (eff2ChildPartComDst + eff2ParentPartComDst) * 0.5f;
  }

  void SoftLimitController::calculateLimit(float limitAngle)
  {
    Assert(limitAngle >= 0.0f);
    // Calculate soft limit angle.
    const float fromAngle = (m_approachDirection > 0.0f) ? (m_eff->getInfo().minAngle) : (m_eff->getInfo().maxAngle);
    // Calculate max span of the hard limits to clamp.
    const float maxLimitSpan = (m_eff->getInfo().maxAngle) - (m_eff->getInfo().minAngle);
    m_limitAngle = fromAngle + m_approachDirection * rage::Min(limitAngle, maxLimitSpan);
  }

  void SoftLimitController::setLimit (float limitAngle, int approachDirection, bool velocityScaled /* = false */)
  {
    Assert(limitAngle >= 0.0f);
    m_velocityScaled = velocityScaled;
    m_approachDirection = (int)rage::Selectf((float)approachDirection, 1.0f, -1.0f);
    calculateLimit(limitAngle);
  }

  void SoftLimitController::setLimitAngle (float limitAngle)
  {
    Assert(limitAngle >= 0.0f);
    calculateLimit(limitAngle);
  }

  void SoftLimitController::tick() const
  {
    if (!m_enabled)
    {
      return;
    }

    const float actualAngle = nmrsGetActualAngle(m_eff);

#if ART_ENABLE_BSPY
    char name[64];
    sprintf(name, "softlimit[%d]", m_eff->getJointIndex());
    bspyScratchpad(m_character->getBSpyID(), name, actualAngle);
    bspyScratchpad(m_character->getBSpyID(), name, m_limitAngle);
    bspyScratchpad(m_character->getBSpyID(), name, m_velocityScaled);
#endif

    // Apply soft limit?
    if (((actualAngle - m_limitAngle) * m_approachDirection) > 0.0f)
    {
      return;
    }
    else
    {
      float stiffness = m_stiffness;

      if(m_velocityScaled)
      {
        const float minVel = 0.0f;
        const float maxVel = 4.0f;
        Assert(maxVel > minVel);

        // velocity scale mapped to 0..1
        float scale = rage::Clamp(m_character->m_COMrotvelMag, minVel, maxVel) / (maxVel - minVel);

#if ART_ENABLE_BSPY
        bspyScratchpad(m_character->getBSpyID(), name, m_character->m_COMrotvelMag);
        bspyScratchpad(m_character->getBSpyID(), name, scale);
#endif

        const float minStiffness = 3.0f;
        stiffness = rage::Max(m_stiffness * scale, minStiffness);
      }

      // Calculate desired acceleration for spring-damper.
      const float angularDisplacement = actualAngle - m_limitAngle;
      const float spring = stiffness * stiffness * angularDisplacement;
      const float damper = stiffness * m_damping * 2.0f * m_eff->getActualAngleVel();
      const float acceleration = -(spring + damper);

      // Calculate torque.
      const float torqueMag = m_combinedMass * acceleration * m_momentArm;

      // Apply torque directly to the effector.
      m_eff->ApplyTorque(torqueMag);

#if ART_ENABLE_BSPY
      bspyScratchpad(m_character->getBSpyID(), name, stiffness);
      bspyScratchpad(m_character->getBSpyID(), name, angularDisplacement);
      bspyScratchpad(m_character->getBSpyID(), name, m_eff->getActualAngleVel());
      bspyScratchpad(m_character->getBSpyID(), name, spring);
      bspyScratchpad(m_character->getBSpyID(), name, damper);
      bspyScratchpad(m_character->getBSpyID(), name, acceleration);
#endif

#define SOFTLIMIT_DEBUG_VERBOSE 0
#if ART_ENABLE_BSPY & SOFTLIMIT_DEBUG_VERBOSE
      // Joint frame.
      rage::Matrix34 jointFrame;
      m_eff->getMatrix1(jointFrame);
      bspyScratchpad(m_character->getBSpyID(), name, jointFrame.d);

      // Torque.
      rage::Vector3 torque;
      torque.Zero();
      torque.AddScaled(jointFrame.c, torqueMag);
      bspyScratchpad(m_character->getBSpyID(), name, torque);

      // Joint limits.
      rage::Quaternion q;
      rage::Vector3 v;

      // Hard joint min angle limit axis.
      rage::Vector3 minAngleAxis;
      v.Zero();
      v.AddScaled(jointFrame.c, m_eff->getInfo().minAngle);
      q.FromRotation(v);
      q.Transform(jointFrame.a, minAngleAxis);
      bspyScratchpad(m_character->getBSpyID(), name, minAngleAxis);

      // Hard joint max angle limit axis.
      rage::Vector3 maxAngleAxis;
      v.Zero();
      v.AddScaled(jointFrame.c, m_eff->getInfo().maxAngle);
      q.FromRotation(v);
      q.Transform(jointFrame.a, maxAngleAxis);
      bspyScratchpad(m_character->getBSpyID(), name, maxAngleAxis);

      // Actual effector angle axis.
      rage::Vector3 actualAngleAxis;
      v.Zero();
      v.AddScaled(jointFrame.c, actualAngle);
      q.FromRotation(v);
      q.Transform(jointFrame.a, actualAngleAxis);
      bspyScratchpad(m_character->getBSpyID(), name, actualAngleAxis);

      // Soft limit angle axis.
      rage::Vector3 softLimitAxis;
      v.Zero();
      v.AddScaled(jointFrame.c, m_limitAngle);
      q.FromRotation(v);
      q.Transform(jointFrame.a, softLimitAxis);
      bspyScratchpad(m_character->getBSpyID(), name, softLimitAxis);
#endif //ART_ENABLE_BSPY
    }
  }
#endif //NM_USE_1DOF_SOFT_LIMITS
} // nms ART
