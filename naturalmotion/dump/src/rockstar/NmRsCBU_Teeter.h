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

#ifndef NM_RS_CBU_TEETER_H 
#define NM_RS_CBU_TEETER_H

#include "NmRsCBU_TaskBase.h"
#include "NmRsBodyLayout.h"

namespace ART
{
  class NmRsCharacter;

#define NMTeeterFeedbackName      "teeter" 

  class NmRsCBUTeeter : public CBUTaskBase
  {
  public:

    NmRsCBUTeeter(ART::MemoryManager* services);
    ~NmRsCBUTeeter();

    struct Parameters;
    void onActivate();
    void onDeactivate();

    CBUTaskReturn onTick(float timeStep);
    void sendFeedback(int newState);

#if ART_ENABLE_BSPY
    virtual void sendParameters(NmRsSpy& spy);
#endif // ART_ENABLE_BSPY

    void updateBehaviourMessage(const MessageParamsBase* const params);

    bool isActive() const { return m_active; }
    bool restrictCatchFallArms() {return m_restrictCatchFallArms;}

    bool hasFallen() { return m_highFall; }

    struct Parameters
    {
      rage::Vector3 edgeLeft;
      rage::Vector3 edgeRight;
      float restartDistance;
      float reactDistance;
      bool useExclusionZone;
      bool useHeadLook;
      bool callHighFall;
      bool leanAway;


      float preTeeterTime;
      float leanAwayTime;
      float teeterTime;
      float leanAwayScale;

    } m_parameters;

  protected:

    void initialiseCustomVariables();

    void teetering(float timeStep);
    void falling(float timeStep);

    enum teeterState
    {
      teet_Pre,           //0 nothing happening at the minute
      teet_LeanAwayZone,  //1 Inside the lean away from edge zone
      teet_FootClose2Edge,//2 one of the feet is close to the edge (Stop applying push force here?)
      teet_PreTeeter,     //3 Waving arms around (Stop applying push force here)
      teet_Teeter,        //4 Teetering
      teet_FallOnGround,  //5 Fell over but not over the edge
      teet_OverEdge,      //6 Gone over the edge
      teet_HighFall,      //7 Doing a highFall
    };

    rage::Vector3 m_edgeLeft;
    rage::Vector3 m_edge;
    rage::Vector3 m_edgeNormal;
    rage::Vector3 m_edgeTarget;
    rage::Vector3 m_levelCom;

    float m_com2Edge;
    float m_time2Edge;
    float m_vel2Edge;
    float m_edge2LeftFoot;
    float m_edge2RightFoot;

    float m_teeterTimer;
    float m_restartTime;

    int m_state;

    bool m_characterIsFalling;
    bool m_setSpineToZeroWhenFinished;
    bool m_setTurnToZeroWhenFinished;
    bool m_highFall;
    bool m_restrictCatchFallArms;

  };
}
#endif // NM_RS_CBU_TEETER_H


