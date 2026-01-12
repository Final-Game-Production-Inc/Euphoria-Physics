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

#ifndef NM_RS_CBU_PEDAL_H 
#define NM_RS_CBU_PEDAL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

  class NmRsCBUPedal : public CBUTaskBase
  {
  public:
    NmRsCBUPedal(ART::MemoryManager* services);
    ~NmRsCBUPedal();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    struct Parameters
    {
      float radius;// = 0.25f  base radius of pedal action
      float radiusVariance; // 0-1 value determining how much radius will be randomly changed while pedalling
      int   randomSeed;// = 100 Random seed used to generate speed changes
      float pedalOffset;// = 0.15f Move the centre of the pedal for the left leg up by this amount, the right leg down by this amount
      float legStiffness;// = 10.0f stiffness of legs
      float angularSpeed;// = 10.0f rate of pedaling
      float speedAsymmetry;// = 0.0f Random offset applied per leg to the angular speed to desynchronise the pedaling - set to 0 to disable, otherwise should be set to less than the angularSpeed value
      float legAngleVariance;
      float dragReduction; // how much to account for the target moving through space rather than being static
      float angSpeedMultiplier4Dragging;// = 0.3 angularSpeed =angSpeedMultiplier4Dragging * linear_speed/pedalRadius
      float centreForwards;// = 0.0f Move the centre of the pedal for both legs forward (or backward -ve)
      float centreSideways;
      float centreUp;//=0.0f Move the centre of the pedal for both legs up (or down -ve)
      float ellipse;

      bool backPedal;// = false pedal forwards or backwards
      bool pedalLeftLeg;// = true pedal with this leg or not
      bool pedalRightLeg;// = true pedal with this leg or not
      bool adaptivePedal4Dragging;// = false Will pedal in the direction of travel (if backPedal = false, against travel if backPedal = true) and with an angular velocity relative to speed upto a maximum of 13(rads/sec).  Use when being dragged by a car.  Overrides angularSpeed.
      bool hula;


    } m_parameters;

    //NB: destroys any de-synch of the leg circles
    inline void setAngle(float angle)
    {
      m_leftAngle = angle; 
      m_rightAngle = m_leftAngle; 
      if (!m_parameters.hula)//start at opposite sides of circle if not hula
        m_rightAngle += PI; 
    }
    inline float getAngle(){return m_leftAngle;}

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:

    void initialiseCustomVariables();

    void moveLeg(NmRsHumanLeg* leg, float angle, float pedalOffset, float moddedRadius);

    void configureLegStiffnesses();
    void calculateCAVs(float angularSpeed);
    

#if ART_ENABLE_BSPY
    rage::Vector3 m_leftCentre;
    rage::Vector3 m_rightCentre;
    rage::Vector3 m_rightTarget;
    rage::Vector3 m_leftTarget;
#endif

    float m_pedalTimer;
    float m_leftAngle;
    float m_rightAngle;
    float m_noiseSeed;
    float m_legDamping;
    float m_leftConstantAngularVelocity;
    float m_rightConstantAngularVelocity;
    float m_leftRandAsymMult;
    float m_rightRandAsymMult;
    float m_startAngleOffset;
    float m_legLength;

    bool m_hula;

  };
}

#endif // NM_RS_CBU_PEDAL_H


