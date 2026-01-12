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

/**
 * -------------------------- IMPORTANT --------------------------
 * This file contains the shadow structures for effectors and parts.
 * It must be compilable on SPU, so no external references outside of the RAGE
 * vector/math libraries and our own math macros.
 */

#ifndef NM_RS_SHADOWS_H
#define NM_RS_SHADOWS_H

#include "NMutils/NMTypes.h"

#include "NmRsUtils.h" // Matrix types

/**
* Shadow structures, used to bake out a more complex class' data into a 
* simpler, reduced form. Used when writing code that cannot arbitrarily
* access effectors, parts, physics, etc - such as when executing on an SPU.
* The associated classes support writing, and optionally reading, these structures.
*/

namespace ART
{
  // 144 bytes
  MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
  NM_ALIGN_PREFIX(16) struct Shadow3Dof
  {
    rage::Matrix34  m_matrix1;
    rage::Matrix34  m_matrix2;
    rage::Vector3   m_position;

    float           m_muscleDamping,
      m_muscleStrength,
      m_twistExtent,
      m_lean1Extent,

      m_lean2Extent,
      m_midLean1,
      m_midLean2,
      m_midTwist,
      m_reverseLean1,
      m_reverseLean2,
      m_reverseTwist,

      m_desiredLean1,
      m_desiredLean2,
      m_desiredTwist,
      m_actualLean1,

      m_actualLean2,
      m_actualTwist;

    inline void blendDesiredWithActual(float blend)
    {
      m_desiredLean1 = ( (m_actualLean1 * (1.0f - blend)) + (m_desiredLean1 * blend) );
      m_desiredLean2 = ( (m_actualLean2 * (1.0f - blend)) + (m_desiredLean2 * blend) );
      m_desiredTwist = ( (m_actualTwist * (1.0f - blend)) + (m_desiredTwist * blend) );
    }

    inline void getLimitOvershoot(float &lean1, float &lean2)
    {
      float l1 = (m_desiredLean1 - m_midLean1) / m_lean1Extent;
      float l2 = (m_desiredLean2 - m_midLean2) / m_lean2Extent;
      float mag = rage::SqrtfSafe(l1 * l1 + l2 * l2);
      float scale = rage::Max(0.0f, mag - 1.0f) / (mag + NM_RS_FLOATEPS);

      lean1 = ((m_desiredLean1 - m_midLean1) * scale);
      lean2 = ((m_desiredLean2 - m_midLean2) * scale);
    }

  } NM_ALIGN_SUFFIX(16);
  MSVCEndWarningMacroBlock()

  // 176 bytes
  MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
  NM_ALIGN_PREFIX(16) struct Shadow1Dof
  {
    rage::Matrix34  m_matrix1;
    rage::Matrix34  m_matrix2;
    rage::Vector3   m_position;

    float           m_muscleDamping,
      m_muscleStrength,
      m_extent,
      m_midAngle,

      m_desiredAngle,
      m_actualAngle;

    inline void blendDesiredWithActual(float blend)
    {
      m_desiredAngle = ( (m_actualAngle * (1.0f - blend)) + (m_desiredAngle * blend) );
    }

  } NM_ALIGN_SUFFIX(16);
  MSVCEndWarningMacroBlock()

  // 96 bytes
  MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
  NM_ALIGN_PREFIX(16) struct ShadowGPart
  {
    rage::Matrix34  m_tm;
    rage::Vector3   m_linVel;
    rage::Vector3   m_angVel;

  private:
    ShadowGPart& operator= (const ShadowGPart& rhs) { ((void)rhs); return *this; }

  } NM_ALIGN_SUFFIX(16);
  MSVCEndWarningMacroBlock()

}

#endif // NM_RS_SHADOWS_H

