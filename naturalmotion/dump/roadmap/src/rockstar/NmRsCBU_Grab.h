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

#ifndef NM_RS_CBU_GRAB_H 
#define NM_RS_CBU_GRAB_H 

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

#define GrabBSpyDraw 1

namespace ART
{
  class NmRsCharacter;

#define NMArmGrabRightFeedbackName     "rightArmGrab" 
#define NMArmGrabLeftFeedbackName      "leftArmGrab"
#define NMGrabFailureToGrabFeedBackName  "grabNotGrabbing"
#define NMGrabDropLeftWeaponFeedbackName  "grabDropLeftWeapon"
#define NMGrabDropRightWeaponFeedbackName  "grabDropRightWeapon"

  class NmRsCBUGrab : public CBUTaskBase
  {
  public:

    NmRsCBUGrab(ART::MemoryManager* services);
    ~NmRsCBUGrab();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

    bool getGrabing() const { return (m_constraintL.IsValid() || m_constraintR.IsValid()); }
    bool getGrabingLeft() const { return (m_constraintL.IsValid()); }
    bool getGrabingRight() const { return (m_constraintR.IsValid()); }
    bool getTryingToGrab() const { return m_doGrab; }
    rage::Vector3 getGrabPoint() { if (m_rightTarget.IsZero()) 
      return m_leftTarget; 
    else
      return m_rightTarget;}

#if NM_EA
    rage::Vector3 getClosestGrabPointLeft() {return m_leftClosestTarget;}
    rage::Vector3 getClosestGrabPointRight() {return m_rightClosestTarget;}
#endif//#if NM_EA

    int getGrabType() { 
#if NM_EA
      if(m_parameters.fromEA)
        return 4;
      else
#endif//#if NM_EA
      {
        if(m_parameters.useLineGrab)
          return 1;
        else if (m_parameters.surfaceGrab)
          return 2;
        else if (m_parameters.pointsX4grab)
          return 3;
        else 
          return 0;
      }
    }

    // things to set the parameters

    struct Parameters
    {
      bool useRight;
      bool useLeft;
      bool dropWeaponIfNecessary;
      float dropWeaponDistance;
      float grabStrength;
      float pullUpTime;
      float pullUpStrengthLeft;
      float pullUpStrengthRight;
      float grabHoldMaxTimer;
      float stickyHands;
      int turnToTarget;
      bool handsCollide;
      bool justBrace;
      bool pointsX4grab;
      bool useLineGrab;
      bool surfaceGrab;
      bool dontLetGo;
      rage::Vector3 pos;
      rage::Vector3 pos1;
      rage::Vector3 pos2;
      rage::Vector3 pos3;
      rage::Vector3 normalL;
      rage::Vector3 normalR;
      rage::Vector3 normalL2;
      rage::Vector3 normalR2;
      int boundIndex;
      int instanceIndex;
      float armStiffness;
      float grabDistance;
      float move2Radius;
      float reachAngle;
      float oneSideReachAngle;
      float bodyStiffness;
      float maxReachDistance;
      float orientationConstraintScale;
      float maxWristAngle;
      bool useHeadLookToTarget;
      bool lookAtGrab;
      rage::Vector3 targetForHeadLook;
#if NM_EA
      bool fromEA;
#endif//#if NM_EA
    } m_parameters;

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

  protected:
    void initialiseCustomVariables();

    void updateGrabPos(rage::Vector3 &target, rage::Vector3 &normal, rage::Vector3 &grabPointOne, rage::Vector3 &grabPointTwo,rage::Vector3 &inputNormal,rage::Vector3 &shoulderPos);

    void localShoulderToTarget(rage::Vector3 &grabPosLocal,rage::Vector3 &grabNormalLocal,rage::Vector3 &shoulderPos);
    void projectOntoLine(rage::Vector3 &endOne, rage::Vector3 &endTwo, rage::Vector3 &point);
    float softWristDirectionConstraint(NmRsGenericPart* endPart,rage::Vector3 &normal,float direction);

    void decideToGrab(bool &moveRight, bool &moveLeft);  
    void decideToGrabFromParameters(bool &moveRight, bool &moveLeft);  

    void moveArm(rage::Vector3 &armTarget, rage::Vector3 &armNormal, BehaviourMask effectorMask, rage::phConstraintHandle &constraint, NmRsHumanArm *limb, NmRsLimbInput& input, int direction, bool &isConstrained, float &distance, float &holdTimer, float &localPullUpStrength, bool useHardConstraint = true);

    void sendSuccessFeedback(int direction);
    void sendFailureFeedback(int direction);
    void sendDropWeaponFeedback(int hand);

    void controlHeadLook(bool active, rage::Vector3 *pos);
    void controlSpineTwist(bool active,rage::Vector3 *pos);

    void moveGrabPointsApart(bool mLeft, bool mRight);

    void projectPointOntoPlane(float edgeDist,rage::Vector3 &point,rage::Vector3 &normal);

    bool handsCrossed();

    rage::phConstraintHandle m_constraintL;
    rage::phConstraintHandle m_constraintR;

#if NM_EA
    rage::Vector3 m_rightClosestTarget;
    rage::Vector3 m_leftClosestTarget;
#endif//#if NM_EA
    rage::Vector3 m_rightTarget;
    rage::Vector3 m_leftTarget;
    rage::Vector3 m_rightNormal;
    rage::Vector3 m_leftNormal;

    float m_holdTimerRight;
    float m_holdTimerLeft;

    float m_constraintInhibitTimeRight;
    float m_constraintInhibitTimeLeft;

    float m_distanceR;
    float m_distanceL;

    float m_behaviourTime;
    float m_reachDistance;
    float m_alreadyGrabTimer;
#if ART_ENABLE_BSPY
    float m_leftHandOriError;//bspy only
    float m_rightHandOriError;//bspy only
#endif
#if NM_EA
    int m_grabPatchL;
    int m_grabPatchR;
#endif//#if NM_EA

    bool m_rightIsConstrained;
    bool m_leftIsConstrained;
    
    bool m_doGrab;
    bool m_1stTimeHeadLookDeactivated;
    bool m_alreadyInit;
  };
}

#endif // NM_RS_CBU_ARMGRAB_H 

