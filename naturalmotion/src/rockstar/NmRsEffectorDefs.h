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
 * NOTE
 * This file may be included by off-line serialization tools
 */

#ifndef NM_RS_EFFECTORDEFS_H
#define NM_RS_EFFECTORDEFS_H

namespace ART
{
  /**
   * effector parameter data for 3-dof effector
   */
  struct NmRs1DofEffectorParams 
  {
    int   parentIndex, 
          childIndex;
    float minAngle, 
          maxAngle;
    float m_defaultForceCap, 
          m_defaultMuscleStiffness, 
          m_defaultMuscleDamping, 
          m_defaultMuscleStrength;
  };

  /**
   * effector parameter data for 3-dof effector
   */
  struct NmRs3DofEffectorParams
  {
    int   parentIndex, 
          childIndex;
    bool  reverseFirstLeanMotor, 
          reverseSecondLeanMotor, 
          reverseTwistMotor;
    float minFirstLeanAngle, 
          maxFirstLeanAngle, 
          minSecondLeanAngle, 
          maxSecondLeanAngle, 
          minTwistAngle, 
          maxTwistAngle,
          softLimitFirstLeanMultiplier, 
          softLimitSecondLeanMultiplier,
          softLimitTwistMultiplier;
    float m_defaultLeanForceCap, 
          m_defaultTwistForceCap, 
          m_defaultMuscleStiffness, 
          m_defaultMuscleDamping, 
          m_defaultMuscleStrength;

    void NoLimits() 
    { 
      minFirstLeanAngle = 
        maxFirstLeanAngle = 
        minSecondLeanAngle = 
        maxSecondLeanAngle = 
        minTwistAngle = 
        maxTwistAngle = 0.f; 
    }
  };
}

#endif // NM_RS_EFFECTORDEFS_H

