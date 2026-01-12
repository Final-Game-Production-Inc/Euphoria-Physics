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

#ifndef NM_RS_CBU_BRACEFORIMPACT_H 
#define NM_RS_CBU_BRACEFORIMPACT_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"


#define BraceBSpyDraw 1

namespace ART
{
  class NmRsCharacter;

#define NMBraceForImpactFeedbackName      "braceForImpact" 

  class NmRsCBUBraceForImpact : public CBUTaskBase
  {
  public:
    NmRsCBUBraceForImpact(ART::MemoryManager* services);
    ~NmRsCBUBraceForImpact();

    struct Parameters;
    void onActivate();
    void onDeactivate();
    CBUTaskReturn onTick(float timeStep);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);
    void setCarInstGenID();
    inline int getCarInstGenID() const { return m_carInstGenID; };

    struct Parameters
    {                      
      float bodyStiffness;// 12.0 [6:16]. stiffness of character. catch_fall stiffness scales with this too, with its defaults at this values default 
      rage::Vector3 pos;// 0.0 (WorldSpace). Location of the front part of the object to brace against. This should be the centre of where his hands should meet the object
      int instanceIndex;// -1 [-1:large]. levelIndex of object to brace
      float braceDistance;// 0.7 [0:2] (Meters). distance from object at which to raise hands to brace
      float targetPredictionTime;// 0.4 [0:1] (Seconds). Time epected to get arms up from idle
      float reachAbsorbtionTime;// 0.15 [0:1] (Seconds). Larger values and he absorbs the impact more
      rage::Vector3 look;// 0(WorldSpace). Position to look at, eg the driver
      float grabStrength; //105
      float grabDistance;//1
      float grabReachAngle;//1.5
      float grabHoldTimer;//3.5
      float maxGrabCarVelocity; //15
      float timeToBackwardsBrace; // 2
      bool  grabDontLetGo; //false
      float legStiffness;
      float minBraceTime;
      float handsDelayMin;
      float handsDelayMax;

      bool  moveAway;
      float moveAwayAmount;
      float moveAwayLean;
      float moveSideways;

      bool bbArms;
      bool newBrace;
      bool roll2Velocity;
      bool braceOnImpact;
      
      int rollType;

    } m_parameters;

  protected:

    void initialiseCustomVariables();
    //behaviours only referenced by braceForImpact
    void OnFallen();
    void BeforeImpact(float timeStep);
    void SpineBend(float timeStep);
    void Bracing(float timeStep);
    void ArmsBrace(float timeStep);
    void DefaultArms();
    void MoveAwayFromCar();
    bool ShouldBrace();
    void GetCarData();

    //Before Impact
    rage::Vector3 m_target;
    rage::Vector3 m_targetOffset;
    rage::Vector3 m_targetVel;
    rage::Vector3 m_direction;

    rage::Vector3 m_leftHandPos;
    rage::Vector3 m_leftHandNorm;
    rage::Vector3 m_rightHandPos;
    rage::Vector3 m_rightHandNorm;

    rage::Vector3 m_leftHandGrabPos;
    rage::Vector3 m_rightHandGrabPos;
    //bounding box around car
    rage::Vector3 m_corner1;
    rage::Vector3 m_corner2;
    rage::Vector3 m_corner3;
    rage::Vector3 m_corner4;
    rage::Vector3 m_carPos;

    float m_braceTime;
    float m_behaviourTime;

    float m_distanceToTarget;
    float m_spineLean1;
    float m_spineLean2;
    float m_toggleHeadLookTimer;

    //de-sink hands
    float m_handsDelay;

    float m_leftHandSeparation; 
    float m_rightHandSeparation;
    float m_kneeBendBrace;
    float m_kneeBend;
    float m_spineShape;
    float m_spineBendMult;

    float m_legStiffOld;
    enum zone
    {
      toLeft,
      toRight,
      toFront,
      toRear
    };
    zone m_zone;

    int m_carInstGenID;

    // only try to grab once
    bool m_hasGrabbed;

    //de-sink hands
    bool m_initialiseDelay;
    bool m_delayLeftHand;

    bool m_leftConnectedWithCar;
    bool m_rightConnectedWithCar;
    bool m_onCar;
    bool m_carExists;

    bool m_balanceFailHandled;

    bool m_doBrace;
    bool m_shouldBrace;
    bool m_useleft;
    bool m_useright;
    bool m_bracedWithLeft;
    bool m_bracedWithRight;
    bool m_lookAtHands;
  };
}

#endif // NM_RS_CBU_BRACEFORIMPACT_H


