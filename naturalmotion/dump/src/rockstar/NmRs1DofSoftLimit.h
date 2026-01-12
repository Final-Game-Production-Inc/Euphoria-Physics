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
#ifndef NM_RS_CBU_1DOFSOFTLIMIT_H
#define NM_RS_CBU_1DOFSOFTLIMIT_H

namespace ART
{

  #if NM_USE_1DOF_SOFT_LIMITS
    /**
    * Brief:
    * Solution to the 1dof joint lock i.e. when elbows and knees are going straight
    * when not being driven in NM ragdoll state.
    * The intention is to provide a weak artificial soft limit that may be
    * on all the time that keeps the effector from ever being perfectly straight
    * when they are not powered.
    * Spring-damper acceleration is calculated that combined with compound mass
    * makes the force. Force multiplied by the moment arm allows to calculate torque
    * that is being applied to the joint.
    */

    class SoftLimitController
    {
    public:
      SoftLimitController();
      ~SoftLimitController();
      void init(NmRsCharacter *character, NmRs1DofEffector *eff);
      void setLimit(float limitAngle, int approachDirection, bool velocityScaled = false);
      void setLimitAngle(float limitAngle);
      // m_limitAngle is just the original limitAngle parameter with
      // approachDirection applied, so abs gives us the original value.
      float getLimit() { return rage::Abs(m_limitAngle); }
      void tick() const;
      inline bool isEnabled() { return m_enabled; }
      inline void setEnabled(bool enabled) { m_enabled = enabled; }

      float m_stiffness;  // Stiffness of the soft limit. Parameter is used to calculate spring term
                          // that contributes to the desired acceleration.
      float m_damping;    // Damping of the soft limit. Parameter is used to calculate damper term
                          // that contributes to the desired acceleration.
                          // To have the system critically dampened set m_damping to 1.0.
    private:
      void preCalcConstants();  // Calculate combined mass and moment arm. Assuming that either part masses or
                                // distance between joint and part COM positions do not change during runtime.

      // Function re-calculates soft limit angle from setLimit() or setLimitAngle().
      void calculateLimit(float limitAngle); 
                                
      NmRsCharacter *m_character;
      NmRs1DofEffector *m_eff;
      float m_limitAngle;        // Soft limit angle in RAD.
      int   m_approachDirection; // WARNING: model dependent value, +1 or -1.
                                 // Set it to +1 to measure soft limit angle relatively to hard limit minAngle
                                 // that corresponds to the maximum stretch of the elbow.
                                 // Set it to -1 to measure soft limit angle relatively to hard limit maxAngle
                                 // that corresponds to the maximum stretch of the knee.
      float m_combinedMass;
      float m_momentArm;
      bool  m_velocityScaled;
      bool  m_enabled;
    };
#endif //NM_USE_1DOF_SOFT_LIMITS

} // NMS ART

#endif //NM_RS_CBU_1DOFSOFTLIMIT_H
