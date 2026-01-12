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

#ifndef NM_RS_CBU_FALLOVERWALL_H 
#define NM_RS_CBU_FALLOVERWALL_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMFallOverWallFeedbackName      "fallOverWall" 
#define NMFallOverWallStateFeedbackName      "fallOverWallState" 

#define FallOverWallBSpyDraw 1
#define useNewFallOverWall 0

  class NmRsCBUFallOverWall : public CBUTaskBase
  {
  public:
    NmRsCBUFallOverWall(ART::MemoryManager* services);
    ~NmRsCBUFallOverWall();

    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    inline rage::Vector3 getWallHitPos() const { return m_wallHitPos; }

    struct Parameters
    {
      rage::Vector3 fallOverWallEndA;
      rage::Vector3 fallOverWallEndB;
      float bodyDamping;
      float bodyStiffness;
      float magOfForce;
      float forceTimeOut;
      float forceAngleAbort;
      float minLegHeight;
      float maxDistanceFromPelToHitPoint;
      float maxForceDist;
      float stepExclusionZone;
      float minTwistTorqueScale;
      float maxTwistTorqueScale;
      float maxTwist;
      bool moveArms;
      bool moveLegs;
      bool bendSpine;
      float leaningAngleThreshold;
      float angleDirWithWallNormal;
      float maxAngVel;
      bool adaptForcesToLowWall;
      float maxWallHeight;
#if useNewFallOverWall
      bool useArmIK;
      float reachDistanceFromHitPoint;
      float minReachDistanceFromHitPoint;
      float angleTotallyBack;
      float minVelForLiedOnTheGround;
#endif// useNewFallOverWall
      float distanceToSendSuccessMessage;
      float rollingBackThr;
      float rollingPotential;
    } m_parameters;

#define FOW_STATE(_state) \
  _state(ApproachingWall) \
  _state(Aborted) \
  _state(RollingOverWall) \
  _state(OverTheWall) \
  _state(StuckOnWall) \
  _state(RollingBack)

    enum fallOverWallState
    {
#define FOW_ENUM_STATE_NAMES(_name) fow_##_name,
      FOW_STATE(FOW_ENUM_STATE_NAMES)
#undef FOW_ENUM_STATE_NAMES
    };
    inline fallOverWallState getFallOverWallState() const { return m_FOWState; }
  protected:

    void initialiseCustomVariables();
    void applyGetOverWallForceToPart(NmRsGenericPart *part,
      const rage::Vector3 &topOfWall,
      float totMass, 
      const rage::Vector3 &angVelCOM,
      float forceScale = 1.0f) const;
    void projectOntoLine(rage::Vector3 &endOne, 
      rage::Vector3 &endTwo, 
      rage::Vector3 &point) const;
    float getHorizontalDistancePointFromLine(
      const rage::Vector3 &point, 
      const rage::Vector3 &pointline1,
      const rage::Vector3 &pointline2,
      rage::Vector3 *intersectionPoint);
    void RollOver(float timeStep);    

    void getAngVelCOMFromAngMomentum(rage::Vector3 &angVelCOM) const;
    bool getPelvisBelowEdge(float belowEdgeDepth) const;
    float rollingPotentialOnImpact();
    void calculateCommonVariables();
    void stateLogicTick();
    void feedbackLogicTick();

    // set the height of the target to be on the wall edge (useful if the wall edge is not 
    // horizontal) target is already aligned with the wallEdge in a horizontal plane
    float setHeightTarget(const rage::Vector3 &target);
    void sendSuccessFeedback();
    void sendFailureFeedback();
    void sendStateFeedback(int currentState);

#if useNewFallOverWall // functions used to apply an IK to the arms
    // apply IK for both arms
    void applyArmIK(float timeStep);
    void setHandsTarget(//set the target for each hands
      rage::Vector3 *lefthandTarget,
      bool *leftHandHasTarget,
      rage::Vector3 *righthandTarget,
      bool *rightHandHasTarget,
      float distanceTostart,
      float *armTwist);

    // verify if hitPos is between startWall and endWall. if not and it's too far from start or
    // endWal, will modify hitpos to be in the wallEdge.
    // return true if hitpos is in the wall edge either if there is a modification or not.
    // if false, hitpos is NULL
    bool isIntheWallEdge(
      const rage::Vector3 &startWall,
      const rage::Vector3 &endWall,
      rage::Vector3 *hitPos
      );
    //check if the probe sent crosses the wall ( in a horizontal plane)
    bool didHitWall(
      const rage::Vector3 &startProbe,
      const rage::Vector3 &endProbe,
      const rage::Vector3 &startWall,
      const rage::Vector3 &endWall,
      rage::Vector3 *hitPos);

#endif// useNewFallOverWall

    rage::Vector3         
      m_wallEdge, 
      m_wallHitPos,
      m_wallNormal,
      m_pelvisPos;
#if useNewFallOverWall
     rage::Vector3 defaultEndOfProbe,
#endif// useNewFallOverWall

    rage::Vector3 m_projPelvisPos;
    rage::Vector3 m_pelvisToWall;
    rage::Vector3 m_comAngVelFromMomentum;

    // variables
    float                 
      m_blean1,
      m_blean2,
      m_forceMultiplier,
      m_forceTimer,
      m_twist;

    float m_amountOfRoll;
    float m_upNess;
    float m_comAngVelEdgeComponent;
    float m_rollingPotentialOnImpact;

    fallOverWallState m_FOWState;

    bool
#if useNewFallOverWall
      m_isOnTheGround,
      m_isTotallyBack,
      m_neverHitWall,
#endif// useNewFallOverWall
      m_hitWall,
      m_negateTorques;

    bool m_pelvisPastEdge;
    bool m_overWallYet;
    bool m_rollingBack;
    bool m_rollingPotentialOnImpactObtained;
  };
}

#endif  //NM_RS_CBU_FALLOVERWALL_H


