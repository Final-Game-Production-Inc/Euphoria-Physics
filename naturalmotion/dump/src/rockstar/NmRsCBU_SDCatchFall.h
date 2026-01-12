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

#ifndef NM_RS_CBU_SDCATCHFALL_H 
#define NM_RS_CBU_SDCATCHFALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUSDCatchFall : public CBUTaskBase
  {
  public:
    NmRsCBUSDCatchFall(ART::MemoryManager* services);
    ~NmRsCBUSDCatchFall();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    // orientation of the character
    enum OrientationStatus {
      kFront = 0,
      kRightSide,
      kLeftSide,
      kBack
    };

    struct Parameters
    {
      float m_legsStiffness;
      float m_torsoStiffness;
      float m_armsStiffness;
      int m_backSideFront;
      bool m_useLeft;
      bool m_useRight;
      Parameters()
      { 
        m_legsStiffness   = 8.5f; 
        m_torsoStiffness  = 10.f; 
        m_armsStiffness   = 16.f;
        m_backSideFront = kFront;
        m_useLeft  = true;
        m_useRight = true;
      }
    } m_parameters;

    rage::Vector3 m_floorVel;
    rage::Vector3 m_probeEnd;
    rage::Vector3 m_groundNormal;

  protected:

    void initialiseCustomVariables();

    rage::Vector3 m_fallDirection;
    float m_forwardsAmount;
    float m_kneeBendL;
    float m_kneeBendR;
    float m_timer;
    float m_leftRight; // -1 is left and +1 is right

    const LeftArmSetup *m_leftArm;
    const RightArmSetup *m_rightArm;
    const LeftLegSetup *m_leftLeg;
    const RightLegSetup *m_rightLeg;
    const SpineSetup *m_spine;

    class ArmState 
    {
    public:
      void init(NmRsCharacter *character, NmRsCBUSDCatchFall *parent)
      {
        m_character = character;
        m_parent = parent;
      }
      void enter(const ArmSetup *armSetup, bool leftArm, char *armMask);
      void tick(float timeStep);
      void armIK(const rage::Vector3 &target, float armTwist, float dragReduction, const rage::Vector3 *vel = NULL);
      void wristIK(const rage::Vector3 &target, const rage::Vector3 &normal);

      NmRsCharacter *m_character;
      NmRsCBUSDCatchFall *m_parent;

      const ArmSetup *m_armSetup;
      //  float m_strength;
      float m_onBackRandomL1;
      float m_onBackRandomL2;
      float m_maxElbowAngleRandom;
      char m_armMask[2];

      bool m_leftArmBool;

    } m_leftArmState, m_rightArmState;

    bool m_workOutNormal;

  private:
    NmRsCBUSDCatchFall(const NmRsCBUSDCatchFall& from);
    NmRsCBUSDCatchFall& operator=(const NmRsCBUSDCatchFall& from);
  };
}

#endif // NmRsCBUSDCatchFall


