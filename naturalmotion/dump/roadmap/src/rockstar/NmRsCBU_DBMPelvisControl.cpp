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

#include "NmRsCommon.h"
#include "NmRsMinimalInclude.h"
#include "NmRsCBU_DBMPelvisControl.h"
#if !__SPU & ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY


namespace ART
{
#define STRONGER_WALK_TWIST
    // Tom's PID (proportional-integral-derivative) function
    inline float gPID(float pos, float posScale, float integral, float integralScale, float vel, float velScale, float muscleStrength)
    {
      return (pos * posScale + integral * integralScale + vel * velScale) / muscleStrength;
    }

    bool dbmPelvisControl(
      const NmRsCBUDynBal_ReadOnly& ro,
      const NmRsCBUDynBal_BodyPacket& body,
      NmRsCBUDynBal_PelvisState& pelvis
      )
    {
      // -- Pelvis Sway Controller

      //Turn off the ankle balancing for staggerFall 
	  // so that balanceSolve (using goWithMovement) can lean the ankles to help off balance the character 
	  // in the direction of movement - stationary->moving quicker      
      bool balancing = pelvis.m_leftFootBalance && pelvis.m_rightFootBalance && (!ro.m_stagger) && (!ro.m_fallToKnees);

      // get offset of pelvis from COM
      pelvis.m_pelvisSway.Subtract(body.m_buttocks.m_tm.d, body.m_COM);
      ro.levelVector(pelvis.m_pelvisSway);

      // -- Twist Controller

      float limit = 0.6f;
      bool dualFacing = true;
      rage::Vector3 vel(body.m_lvlCOMvelRelative);
      //mmmmShotFromRunning
      if (body.m_lvlCOMvelRelative.Mag() < ro.m_useComDirTurnVelThresh)
      {
        vel = -body.m_COMTM.c;
        ro.levelVector(vel);
      }
      // override with custom value?
      if (pelvis.m_useCustomTurnDir)
      {
        dualFacing = false;
        vel.Set(pelvis.m_customTurnDir);
      }
#if !__SPU & ART_ENABLE_BSPY
	  bspyScratchpad(bspyLastSeenAgent, "turnDir", vel);
#endif // ART_ENABLE_BSPY

      //mmmmStaggerFall don't turn on toes.  Turn the most when flat feet
      float leftFootFlat = 1.f;
      float rightFootFlat = 1.f;
      if (ro.m_stagger)
      {
        leftFootFlat =  rage::Abs(body.m_leftFoot.m_tm.b.Dot(ro.m_leftFootProbeNormal));
        leftFootFlat *= leftFootFlat * leftFootFlat;
        rightFootFlat = rage::Abs(body.m_rightFoot.m_tm.b.Dot(ro.m_rightFootProbeNormal));
        rightFootFlat *= rightFootFlat * rightFootFlat;
      }

//#if NM_RS_ENABLE_DEBUGDRAW
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 col(0, 0.5, 1);
//        NM_RS_CBU_DRAWVECTORCOL( body.m_leftFoot.m_tm.d, body.m_leftFoot.m_tm.b, col);//mmmmmaddtobspy
//      }
//#endif

      // calculate twistLeft and twistRight
      if (pelvis.m_leftFootBalance && !pelvis.m_rightFootBalance)
      {
#if defined(STRONGER_WALK_TWIST)
        float desiredAngle = rage::Clamp(rotateFromVel(1.0f, body.m_buttocks.m_tm, vel, body.m_COMrotvel, dualFacing, false), -limit, limit);
        //desiredAngle *= 0.3f;//mmmmStaggerFall
        float blend = rage::Min(vel.Mag()*2.f, 1.0f);
        pelvis.m_twistLeft += (desiredAngle - pelvis.m_twistLeft)*blend * 0.2f * leftFootFlat;
        //mmmmHEREbalancer
        //pelvis.m_twistLeft += (desiredAngle - pelvis.m_twistLeft)*blend * 0.2f/2.f;
#else
        pelvis.m_twistLeft = rage::Clamp(pelvis.m_twistLeft + rotateFromVel(1.0f, body.m_buttocks.m_tm, vel, body.m_COMrotvel, dualFacing, true), -limit, limit);
#endif
        pelvis.m_twistRight = pelvis.m_twistLeft * 0.5f;
      }
      else if (!pelvis.m_leftFootBalance && pelvis.m_rightFootBalance)
      {
#if defined(STRONGER_WALK_TWIST)
        float desiredAngle = rage::Clamp(rotateFromVel(-1.0f, body.m_buttocks.m_tm, vel, body.m_COMrotvel, dualFacing, false), -limit, limit);
        //desiredAngle *= 0.3f;
        float blend = rage::Min(vel.Mag()*2.f, 1.0f);
        pelvis.m_twistRight += (desiredAngle - pelvis.m_twistRight)*blend * 0.2f * rightFootFlat;
        //mmmmHEREbalancerParam less twist.
        //pelvis.m_twistRight += (desiredAngle - pelvis.m_twistRight)*blend * 0.2f/2.f;
#else
        pelvis.m_twistRight = rage::Clamp(pelvis.m_twistRight + rotateFromVel(-1.0f, body.m_buttocks.m_tm, vel, body.m_COMrotvel, dualFacing, true), -limit, limit);
#endif
        pelvis.m_twistLeft = pelvis.m_twistRight * 0.5f;
      }
      else if (pelvis.m_useCustomTurnDir)
      {
        float twistMid = (pelvis.m_twistLeft - pelvis.m_twistRight) * 0.5f;
        pelvis.m_twistLeft = rage::Clamp(twistMid + rotateFromVel(1.0f, body.m_buttocks.m_tm, vel, body.m_COMrotvel, dualFacing, true), -limit, limit );
        pelvis.m_twistRight = -pelvis.m_twistLeft;
      }
      else
      {
        const float reduceSpeed = 0.5f;
        float left = ro.m_hipYaw - ro.m_charlieChapliness;
        float right = -ro.m_hipYaw - ro.m_charlieChapliness;

        if (pelvis.m_twistLeft > left)
          pelvis.m_twistLeft = rage::Max(left, pelvis.m_twistLeft - reduceSpeed * ro.m_timeStep);
        else
          pelvis.m_twistLeft = rage::Min(pelvis.m_twistLeft + reduceSpeed * ro.m_timeStep, left);

        if (pelvis.m_twistRight > right)
          pelvis.m_twistRight = rage::Max(right, pelvis.m_twistRight - reduceSpeed * ro.m_timeStep);
        else
          pelvis.m_twistRight = rage::Min(pelvis.m_twistRight + reduceSpeed * ro.m_timeStep, right);
      }

      NM_RS_DBG_LOGF(L"twistLeft: %.3f", pelvis.m_twistLeft);
      NM_RS_DBG_LOGF(L"twistRight: %.3f", pelvis.m_twistRight);

      // -- Balance Controller
      const float ankleP = 476.0f;
      const float ankleI = 208.0f;
      float ankleD = ankleP * ro.m_balanceTime;

      rage::Vector3 bodyBackLevelled(body.m_COMTM.c), bodyRightLevelled(body.m_COMTM.a);
      ro.levelVector(bodyBackLevelled);
      ro.levelVector(bodyRightLevelled);

      rage::Vector3 toCOM(body.m_COM - pelvis.m_centreOfFeet);

      float forwardsLean    = -toCOM.Dot(bodyBackLevelled);
      float rightLean       = toCOM.Dot(bodyRightLevelled);
      float forwardsLeanVel = -body.m_lvlCOMvelRelative.Dot(bodyBackLevelled);
      float rightLeanVel    = body.m_lvlCOMvelRelative.Dot(bodyRightLevelled);

      float footLength = 0.15f;
      forwardsLean = rage::Clamp(forwardsLean, -footLength*0.5f, footLength);

      if (balancing)
      {
        const float iVariation = 0.1f;
        pelvis.m_totalForwards = rage::Clamp(
          pelvis.m_totalForwards + forwardsLean * ro.m_timeStep,
          ro.m_ankleEquilibrium - iVariation,
          ro.m_ankleEquilibrium + iVariation);

        pelvis.m_totalRight = rage::Clamp(
          pelvis.m_totalRight + rightLean * ro.m_timeStep,
          -iVariation,
          iVariation); 
      }

#if !__SPU & ART_ENABLE_BSPY 
      bspyScratchpad(bspyLastSeenAgent, "ankleEq",  pelvis.m_centreOfFeet);
			bspyScratchpad(bspyLastSeenAgent, "ankleEq",  forwardsLean);
			bspyScratchpad(bspyLastSeenAgent, "ankleEq",  forwardsLeanVel);
      bspyScratchpad(bspyLastSeenAgent, "ankleEq Better",  pelvis.m_totalForwards);
      bspyScratchpad(bspyLastSeenAgent, "ankleEq",  rightLean);
      bspyScratchpad(bspyLastSeenAgent, "ankleEq",  rightLeanVel);
      bspyScratchpad(bspyLastSeenAgent, "ankleEq Better",  pelvis.m_totalRight);
      bspyScratchpad(bspyLastSeenAgent, "balancing",  balancing);
#endif // ART_ENABLE_BSPY

      // lower gains on hip
      float legP = ankleP * 0.5f;
      float legI = ankleI * 0.5f;
      float legD = ankleD * 0.5f;

      pelvis.m_hipRollCalc = 0.5f*gPID(rightLean, legP, pelvis.m_totalRight, legI, rightLeanVel, legD, 144.f);
      if (balancing)
      {
        pelvis.m_ankleLean = -gPID(forwardsLean, ankleP, pelvis.m_totalForwards, ankleI, forwardsLeanVel, ankleD, 144.f);
        
#if NM_NEW_BALANCER
        //as the character comes into balance moving backwards it sometimes overshoots slightly
        //causing another step.  Stop this by tilting the hips forward slightly. 
        //float angVelMag = -body.m_COMrotvel.Dot(bodyRightLevelled);
        float tip = -rage::Clamp(-0.7f*forwardsLeanVel/*-0.2f*angVelMag*/, 0.f, 0.5f);        
        pelvis.m_hipPitch += tip;
#endif
      }
      else
      {
        pelvis.m_ankleLean = 0.f;
        // TDL the tip below stops the character from limboing if knocked backwards
        // It also give more range for stepping backwards.. and should look more real.
        if (!ro.m_stagger)
        {
          float tip = -rage::Clamp(-0.2f*forwardsLeanVel, 0.f, 0.5f);        
          pelvis.m_hipPitch += tip;
          NM_RS_DBG_LOGF(L"hip pitch: %.3f", tip);
        }
      }
      // TDL below stops the acute ankle angles on static balance
      float leftFootUp = -body.m_leftFoot.m_tm.c.Dot(ro.m_leftFootProbeNormal);
      float rightFootUp = -body.m_rightFoot.m_tm.c.Dot(ro.m_rightFootProbeNormal);
      float toeLift = leftFootUp + rightFootUp;
      if (toeLift > 0.05f)
        pelvis.m_ankleLean += (0 - pelvis.m_ankleLean)*rage::Min((toeLift - 0.05f)*3.f, 1.f);

      return true;
    }
}

using namespace ART;

void NmRsCBU_DBMPelvisControl(rage::sysTaskParameters& param)
{
  Assert(param.ReadOnlyCount == 2);
  Assert(param.ReadOnly[0].Data);
  Assert(param.ReadOnly[0].Size == sizeof(NmRsCBUDynBal_ReadOnly));

  Assert(param.ReadOnly[1].Data);
  Assert(param.ReadOnly[1].Size == sizeof(NmRsCBUDynBal_BodyPacket));

  Assert(param.Input.Data);
  Assert(param.Input.Size == sizeof(NmRsCBUDynBal_PelvisState));

  const NmRsCBUDynBal_ReadOnly* ro = static_cast<const NmRsCBUDynBal_ReadOnly*>(param.ReadOnly[0].Data);
  const NmRsCBUDynBal_BodyPacket* body = static_cast<const NmRsCBUDynBal_BodyPacket*>(param.ReadOnly[1].Data);
  NmRsCBUDynBal_PelvisState* pelvis = static_cast<NmRsCBUDynBal_PelvisState*>(param.Input.Data);

  dbmPelvisControl(*ro, *body, *pelvis);
}

#if __SPU
#include "vector/quaternion.cpp"
#include "NmRsUtils.cpp"
#endif // __SPU

