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

#ifndef NM_RS_CBU_SPINETWIST_H 
#define NM_RS_CBU_SPINETWIST_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  //#define NMSpineTwistFeedbackName      "Spine Twist" 

  class NmRsCBUSpineTwist : public CBUTaskBase
  {
  public:
    NmRsCBUSpineTwist(ART::MemoryManager* services);
    ~NmRsCBUSpineTwist();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    // things to set the parameters
    inline void setSpineTwistPos(const rage::Vector3 &pos){ m_pos = pos; }
    inline void setSpineTwistVelX(float inputVelX){m_velX = inputVelX;}
    inline void setSpineTwistVelY(float inputVelY){m_velY = inputVelY;}
    inline void setSpineTwistVelZ(float inputVelZ){m_velZ = inputVelZ;}
    //inline void setSpineTwistStiffness(float inputStiffness){m_stiffness = inputStiffness;}
    inline void setSpineTwistTwistClavicles(bool inputTwistClavicles){m_twistClavicles  = inputTwistClavicles;}
    inline void setSpineTwistAllwaysTwist(bool inputAllwaysTwist){m_allwaysTwist  = inputAllwaysTwist;}
    inline void setSpineTwistOffset(float offset){m_offset = offset;}

    struct Parameters
    {
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    //inline bool hasFailed() const { return !isActive() && m_failed; }
    inline bool doingTwist() const { return m_doingTwist; }
    inline float getTwist() const { return m_twist; }

    void initialiseCustomVariables();

  protected:

    rage::Vector3   m_pos;

    float           m_offset; // offset from spine forward.

    float           m_velX;
    float           m_velY;
    float           m_velZ;
    float           m_twist;//Output -- referenced by catch fall

    float m_spineTwistExcess;
    float m_sp0TwistMin, m_sp0TwistMax, m_sp0Strength, m_sp0Damping;
    float m_sp1TwistMin, m_sp1TwistMax, m_sp1Strength, m_sp1Damping;
    float m_sp2TwistMin, m_sp2TwistMax, m_sp2Strength, m_sp2Damping;
    float m_sp3TwistMin, m_sp3TwistMax, m_sp3Strength, m_sp3Damping;
    float m_spTwistMin, m_spTwistMax, m_spStrength, m_spDamping;
    float m_spInvTwistMin, m_spInvTwistMax;

    bool m_twistClavicles;
    bool m_allwaysTwist;
    bool m_doingTwist;
  };
}

#endif // NM_RS_CBU_SPINETWIST_H


