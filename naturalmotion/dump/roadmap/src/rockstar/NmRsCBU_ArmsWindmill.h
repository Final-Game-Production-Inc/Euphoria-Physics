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

#ifndef NM_RS_CBU_WINDMILL_H 
#define NM_RS_CBU_WINDMILL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;
  
  #define ArmsWindmillBSpyDraw 0

  class NmRsCBUArmsWindmill : public CBUTaskBase
  {
  public:

    NmRsCBUArmsWindmill(ART::MemoryManager* services);

    void onActivate();
    void onDeactivate();
      CBUTaskReturn onTick(float timeStep);

    struct CircleDescriptor
    {
      rage::Vector3 normal; // normal of the circle plane specified in the local space of partID
      rage::Vector3 centre; // centre of the circle specified in the local space of partID
      int partID;           // part that the circle is specified relative to
      float radius1;        // the length of the two axes of the ellipse
      float radius2;
      float speed;          // how fast the target moves around the circle in revolutions/sec
      float radius1Delta;        // the length of the two axes of the ellipse
      float radius2Delta;
      float speedDelta;          // how fast the target moves around the circle in revolutions/sec
    };

    enum Hand
    {
      HAND_NONE = 0,
      HAND_LEFT,
      HAND_RIGHT
    };

    enum Direction
    {
      DIR_CW = -1,
      DIR_CCW = 1
    };

    enum Mirror
    {
      MIRROR_NONE,
      MIRROR_SYMMETRIC,
      MIRROR_PARALLEL
    };

    enum Adaptation
    {
      ADAPT_NONE = 0,
      ADAPT_DIR,
      ADAPT_DIR_SPEED,
      ADAPT_DIR_SPEED_STRENGTH
    };

    struct Parameters
    {
      CircleDescriptor m_leftCircleDesc;
      CircleDescriptor m_rightCircleDesc;
      float m_shoulderStiffness;
      float m_shoulderDamping;
      float m_elbowStiffness;
      float m_elbowDamping;
      float m_shoulderStiffnessDelta;
      float m_elbowStiffnessDelta;
      float m_phaseOffset;
      float m_dragReduction;
      float m_IKtwist;
      float m_angVelThreshold;
      float m_angVelGain;
      float m_leftElbowMin;
      float m_rightElbowMin;
      int m_mirrorMode;
      int m_adaptiveMode;
      bool m_forceSync;
      bool m_useLeft;
      bool m_useRight;
      bool m_disableOnImpact;
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    // if externally the circle positions are changed abruptly, this might be useful
    // snaps target of specified hand to closest point on its circle
    void snapToClosestPoint(Hand hand);
    inline bool getOK2Deactivate() const { return m_oK2Deactivate; }
    inline bool couldUseLeft() const { return m_doLeftArm; }
    inline bool couldUseRight() const { return m_doRightArm; }
    inline bool getUsingLeft() const { return m_doLeftArm && m_active; }
    inline bool getUsingRight() const { return m_doRightArm && m_active; }
    inline void setOK2Deactivate(bool oK2Deactivate) { m_oK2Deactivate = oK2Deactivate; }   
    inline void setLeftSpeedDelta(float leftSpeedDelta) { m_leftSpeedDelta = leftSpeedDelta; }   
    inline void setRightSpeedDelta(float rightSpeedDelta) { m_rightSpeedDelta = rightSpeedDelta; }   
    inline void setShoulderStiffnessDelta(float shoulderStiffnessDelta) { m_shoulderStiffnessDelta = shoulderStiffnessDelta; }   
    inline void setElbowStiffnessDelta(float elbowStiffnessDelta) { m_elbowStiffnessDelta = elbowStiffnessDelta; }   
    inline void setPhaseOffsetDelta(float phaseOffsetDelta) { m_phaseOffsetDelta = phaseOffsetDelta; }   
    inline void setLeftRadius1Delta(float radius1Delta) { m_leftCircle.radius1Delta = radius1Delta; }   
    inline void setLeftRadius2Delta(float radius2Delta) { m_leftCircle.radius2Delta = radius2Delta; }   
    inline void setRightRadius1Delta(float radius1Delta) { m_rightCircle.radius1Delta = radius1Delta; }   
    inline void setRightRadius2Delta(float radius2Delta) { m_rightCircle.radius2Delta = radius2Delta; }   

  protected:

    struct Circle
    {
      CircleDescriptor* desc;
      rage::Matrix34 TM;
      rage::Vector3 target;
      float angle;
      float radius1Delta;
      float radius2Delta;

    } m_leftCircle, m_rightCircle;

    void initialiseCustomVariables();
    void configureArmStiffnesses();
    void updateCirclePose(Circle& circle, const rage::Vector3& normal, const rage::Vector3& localPos, int* reflection = NULL);
    void updateCirclePose(Hand s);
    void updateAngleOnCircle(Circle& c, float speed, float timeStep);
    void pointOnCircle(const Circle& c, rage::Vector3& p);
    void pointOnCircle(const Circle& c, float angle, rage::Vector3& p);
    float angleOfClosestPointOnCircle(const Circle&c, const rage::Vector3& p);
    void updateRelativeTargetDistances();
    void updateTimeStepForSync();
    void updateAdaptiveSpeeds();

#if ART_ENABLE_BSPY && ArmsWindmillBSpyDraw
    void closestPointOnCircle(const Circle&c, const rage::Vector3& p, rage::Vector3& closest);
    void drawCircle(Circle& c, const rage::Vector3& color);
#endif

    float 
      m_armDamping,
      m_relDistLRcw,
      m_relDistLRccw,
      m_leftSpeed,
      m_rightSpeed,
      m_armStiffness,
      m_elbowStiffness,
      m_leftTimeStep,
      m_rightTimeStep,
      m_leftSpeedDelta,
      m_rightSpeedDelta,
      m_shoulderStiffnessDelta,
      m_elbowStiffnessDelta,
      m_phaseOffsetDelta;

    bool m_isForcingSync;
    bool m_doLeftArm;
    bool m_doRightArm;
    bool m_oK2Deactivate;
  };
}

#endif // NM_RS_CBU_PEDAL_H


