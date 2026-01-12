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
#include "NmRsCBU_DBMBalanceSolve.h"
#include "NmRsIK.h"
#if !__SPU & ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY

namespace ART
{
    // #define THIGH_ROTATE_EFFECT // this reduces the ik straightness with thigh rotation
    // #define SLOWER_STANCE_LEG_STRAIGHTEN
    // #define HIP_ROLL_ON_STEP
    // #define LESS_ACUTE_ANKLES
    #define FEEDBACK_EFFECT
    #define PELVIS_DAMPING


    bool dbmBalanceSolve(
      const NmRsCBUDynBal_ReadOnly& ro,
      const NmRsCBUDynBal_FootState& foot,
      const NmRsCBUDynBal_PelvisState& pelvis,
      NmRsCBUDynBal_BodyPacket& body
      )
    {
      rage::Vector3 lFootPos(foot.m_leftFootPos), rFootPos(foot.m_rightFootPos);

      float hipRollAdd = 0.f;
#if defined(HIP_ROLL_ON_STEP)
      if (!foot.state.m_leftGround && foot.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
        hipRollAdd = 0.5f;
      if (!foot.state.m_leftGround && foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
        hipRollAdd = -0.5f;
#endif

      float legLength = ro.m_fullLegLength * ro.m_legStraightness;

      NM_RS_DBG_LOGF(L"balanceLeft: %s  balanceRight %s", foot.state.m_leftFootBalance?L"true":L"false", foot.state.m_rightFootBalance?L"true":L"false");
      NM_RS_DBG_LOGF(L"hipRollAdd %.3f", hipRollAdd);
      NM_RS_DBG_LOGF(L"ro.m_fullLegLength %.3f", ro.m_fullLegLength);
      NM_RS_DBG_LOGF(L"ro.m_legStraightness %.3f", ro.m_legStraightness);
      NM_RS_DBG_LOGF(L"legLength %.3f", legLength);

      float groundHeightLeft = lFootPos.Dot(ro.m_gUp);
      float groundHeightRight = rFootPos.Dot(ro.m_gUp);

      // compensate for leg angle in calculating waistHeight
      Assert(ro.m_fullLegLength > 0.0f);
      float r = ro.m_fullLegLength;
      float rSqr = rage::square(ro.m_fullLegLength);

      //! CFB: leftX/rightX became > r in some cases which caused the input to Sqrtf() to be negative,
      //! my guess is that the minimum operation intended to keep leftX/rightX in the range
      //! (-r * 0.75) .. (r * 0.75) which the fix below makes sure of.
      //float leftX = rage::Min((ro.horizDistance(foot.m_centreOfFeet, lFootPos) - ro.m_hipWidth)*1.3f, r * 0.75f);
      //float rightX = rage::Min((ro.horizDistance(foot.m_centreOfFeet, rFootPos) - ro.m_hipWidth)*1.3f, r * 0.75f);
      //groundHeightLeft  -= r - rage::Sqrtf(rage::square(r) - rage::square(leftX));
      //groundHeightRight -= r - rage::Sqrtf(rage::square(r) - rage::square(rightX));

      // the *1.3 is so the support leg drops down a bit more when doing a large step
      // 0.75 * 0.75 = 0.5625
      //MMMM
      // the hipWidth shouldn't always be negated
      // hipHeight shouldn't be reduced until swing leg is past stance leg? 
      // 
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", rSqr * 0.5625f);
#endif
      float horizL2 = rage::square((ro.horizDistance(foot.m_centreOfFeet, lFootPos) )*1.3f);
      float horizR2 = rage::square((ro.horizDistance(foot.m_centreOfFeet, rFootPos) )*1.3f);
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", horizL2);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", horizR2);
#endif
      horizL2 = rage::square((ro.horizDistance(foot.m_centreOfFeet, lFootPos) - ro.m_hipWidth)*1.3f);
      horizR2 = rage::square((ro.horizDistance(foot.m_centreOfFeet, rFootPos) - ro.m_hipWidth)*1.3f);
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", horizL2);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", horizR2);
#endif
      //float leftXSqr = rage::Min(rage::square((ro.horizDistance(foot.m_centreOfFeet, lFootPos) - ro.m_hipWidth)*1.3f), rSqr * 0.5625f);
      //float rightXSqr = rage::Min(rage::square((ro.horizDistance(foot.m_centreOfFeet, rFootPos) - ro.m_hipWidth)*1.3f), rSqr * 0.5625f);
      float leftXSqr = rage::Min(horizL2, rSqr * 0.5625f);
      float rightXSqr = rage::Min(horizR2, rSqr * 0.5625f);
      //rage::Vector3 leftFootActualPos = ro.m_leftFootProbeHitPos;//body.m_leftFoot.m_tm.d;
      //rage::Vector3 rightFootActualPos = ro.m_rightFootProbeHitPos;//body.m_rightFoot.m_tm.d;
      //rage::Vector3 centreOfFeet = leftFootActualPos + rightFootActualPos;
      //centreOfFeet *= 0.5f;
      //leftXSqr = rage::Min(rage::square((ro.horizDistance(centreOfFeet, leftFootActualPos) - ro.m_hipWidth)*1.3f), rSqr * 0.5625f);
      //rightXSqr = rage::Min(rage::square((ro.horizDistance(centreOfFeet, rightFootActualPos) - ro.m_hipWidth)*1.3f), rSqr * 0.5625f);

#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", r - rage::Sqrtf(rSqr - leftXSqr));
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", r - rage::Sqrtf(rSqr - rightXSqr));
#endif
      groundHeightLeft  -= r - rage::Sqrtf(rSqr - leftXSqr);
      groundHeightRight -= r - rage::Sqrtf(rSqr - rightXSqr);

      NM_RS_DBG_LOGF(L"groundHeightLeftMinus: %3f", r - rage::Sqrtf(rSqr - leftXSqr));
#if NM_NEW_BALANCER
      float l = ro.horizDistance(lFootPos, rFootPos)*0.5f;
#endif
      if (foot.state.m_leftFootBalance)
      {
        //mmmmHEREbalancerParam
        lFootPos.AddScaled(ro.m_gUp, (pelvis.m_hipRollCalc+hipRollAdd) * ro.m_hipWidth);
        //lFootPos.AddScaled(-ro.m_gUp, 0.7f);//MMMM
        NM_RS_DBG_LOGF(L"addingHipRollToLFootHeight: %.4f", pelvis.m_hipRollCalc * ro.m_hipWidth);
      }

      if (foot.state.m_rightFootBalance)
      {
        rFootPos.AddScaled(ro.m_gUp, -(pelvis.m_hipRollCalc+hipRollAdd) * ro.m_hipWidth);
        //rFootPos.AddScaled(-ro.m_gUp, 0.7f);//MMMM
        NM_RS_DBG_LOGF(L"addingHipRollToRFootHeight: %.4f", -pelvis.m_hipRollCalc * ro.m_hipWidth);
      }

      // clamp and set hip roll
      float hipRollLean = rage::Clamp(pelvis.m_hipRollCalc, -0.2f, 0.2f);
      body.m_spine0.m_desiredLean2 = hipRollLean + hipRollAdd;

      NM_RS_DBG_LOGF(L"hipRoll: %.3f", hipRollLean);

      float centreOfFeetHeight = ro.m_gUp.Dot(foot.m_centreOfFeet);

      // calculate waist height as average of floor heights
      float dif = centreOfFeetHeight - rage::Min(groundHeightLeft, groundHeightRight);
      NM_RS_DBG_LOGF(L"dif: %.3f", dif);
      float newWaistHeight = legLength - dif;
      if (!foot.state.m_leftFootBalance || !foot.state.m_rightFootBalance)
        newWaistHeight += 0.03f;

      // this solution to uneven ground isn't right.. need to find another fix to this.
      // probably smooth upwards when difference between low foot and desired pelvis pos increases
      body.m_newWaistHeight = newWaistHeight;
#if NM_NEW_BALANCER
      //float l = ro.horizDistance(body.m_leftFoot.m_tm.d, body.m_rightFoot.m_tm.d)*0.5f;
      float lsquared = l*l;
      //0.5625  0.0625f
      lsquared = rage::Min(lsquared, 0.5625f*legLength*legLength);
      l = rage::Sqrtf(legLength*legLength - lsquared); 
      body.m_newWaistHeight = l;
#endif
      NM_RS_DBG_LOGF(L"waistHeight: %.3f", pelvis.m_waistHeight);
//#if NM_RS_ENABLE_DEBUGDRAW
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 col(1, 0.5, 0);
//        NM_RS_CBU_DRAWPOINT( foot.m_centreOfFeet, 0.2f, col);
//        NM_RS_CBU_DRAWPOINT( lFootPos, 0.1f, col);//mmmmmaddtobspy
//        NM_RS_CBU_DRAWPOINT( rFootPos, 0.1f, col);//mmmmmaddtobspy
//      }
//#endif

      // desired pelvis position calculated from desired COM position
      rage::Vector3 pelvisPos(foot.m_centreOfFeet);
#if !NM_NEW_BALANCER
      pelvisPos.AddScaled(ro.m_gUp, pelvis.m_waistHeight);
#else
      pelvisPos.AddScaled(ro.m_gUp, body.m_newWaistHeight);//mmmmwhy wait?  mmmmtodo check for SPU builds//pelvis.m_waistHeight);
#endif
      pelvisPos.Add(pelvis.m_pelvisSway);

#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", newWaistHeight);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", dif);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", centreOfFeetHeight);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", groundHeightLeft);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", groundHeightRight);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", lFootPos);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", rFootPos);
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", foot.m_centreOfFeet);//this becomes pelvis centre of feet 1 frame later and is used for balancing
      bspyScratchpad(bspyLastSeenAgent, "BalSolve", pelvisPos);
      //bspyScratchpad(bspyLastSeenAgent, "BalSolve", leftFootActualPos);
      //bspyScratchpad(bspyLastSeenAgent, "BalSolve", rightFootActualPos);
#endif
      //MMMM
      //pelvisPos -= 3.f * pelvis.m_pelvisSway;
      //rage::Vector3 comVel(body.m_COMvel);
      //ro.levelVector(comVel,0.f);
      //rightFootCopy += comVel * 0.02f;

#if defined(SLOWER_STANCE_LEG_STRAIGHTEN)
      float height = ro.m_gUp.Dot(pelvisPos);
      float heightPelvis = ro.m_gUp.Dot(body.m_buttocks.m_tm.d);
      if (height > heightPelvis + 0.2f)
        pelvisPos -= ro.m_gUp * (height - (heightPelvis + 0.2f));
#endif

      // this block counter rotates the foot positions and 'up' normals (for balancing feet) so as to try to control the pelvis
      rage::Vector3 pelvisBack(body.m_buttocks.m_tm.c), upL(ro.m_leftFootProbeNormal), upR(ro.m_rightFootProbeNormal);
      rage::Vector3 leftFootCopy(lFootPos), rightFootCopy(rFootPos);
//#if NM_RS_ENABLE_DEBUGDRAW
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 col(1, 1, 1);
//        NM_RS_CBU_DRAWVECTORCOL( rFootPos, upR, col);//mmmmmaddtobspy
//      }
//#endif
      bool airborne = false;
      //Removed this to get better airborne stepping.  May set to false ONLY during forceStep from running animations 
      // if bad things start to happen.  i.e falling over from a push too easily 
      //  - I think this is now covered in RDR by delaying the force so the character is more likely on ground.

      if (foot.state.m_leftFootBalance) 
        airborne = !ro.m_airborneStep && !body.cd.m_leftFootCollided && !body.cd.m_leftFootCollidedLast; 
      else
        airborne = !ro.m_airborneStep && !body.cd.m_rightFootCollided && !body.cd.m_rightFootCollidedLast;
      NM_RS_DBG_LOGF(L"*******airborne: %s", airborne?L"true":L"false", foot.state.m_rightFootBalance?L"true":L"false");
      
      if ((foot.state.m_leftFootBalance || foot.state.m_rightFootBalance) && !airborne)
      {
        rage::Matrix34 tempMat, newMat;

        tempMat.a.Set(ro.m_leanHipgUp);//up
        tempMat.b.Cross(pelvisBack, tempMat.a);//side (pelvisback x up)
        tempMat.b.Normalize();//back
        tempMat.c.Cross(tempMat.a, tempMat.b);
        newMat.FromEulersXYZ(rage::Vector3(pelvis.m_hipYaw, -pelvis.m_hipPitch, pelvis.m_hipRoll-(pelvis.m_hipRollCalc+hipRollAdd)));
        newMat.d.Zero();
        newMat.Dot3x3(tempMat);
#if defined(PELVIS_DAMPING)
        rage::Matrix34 velRot;
        rage::Quaternion quatRot;
        rage::Vector3 angVel(body.m_buttocks.m_angVel);
        angVel.Scale(-0.1f);
        quatRot.FromRotation(angVel);
        velRot.FromQuaternion(quatRot);
        newMat.Dot3x3(velRot);
#endif
        newMat.d.Set(pelvisPos);

        if (foot.state.m_leftFootBalance)
        {
          clampTarget(
            lFootPos,
            ro.m_gUp,
            newMat,
            body.m_leftHip.m_lean1Extent * ro.m_stepClampScale * ro.m_hipLean1Offset,
            body.m_leftHip.m_lean2Extent * ro.m_stepClampScale * ro.m_hipLean2Offset);
        }
        if (foot.state.m_rightFootBalance)
        {
          clampTarget(
            rFootPos,
            ro.m_gUp,
            newMat,
            body.m_rightHip.m_lean1Extent * ro.m_stepClampScale * ro.m_hipLean1Offset,
            body.m_rightHip.m_lean2Extent * ro.m_stepClampScale * ro.m_hipLean2Offset);
        }

//#if NM_RS_ENABLE_DEBUGDRAW
//        if (rage::NMRenderBuffer::getInstance())
//        {
//          rage::NMRenderBuffer::getInstance()->addAxis(rage::NMDRAW_DYNBAL, 0.2f, newMat);//mmmmmaddtobspy
//          rage::NMRenderBuffer::getInstance()->addAxis(rage::NMDRAW_DYNBAL, 0.2f, body.m_buttocks.m_tm);//mmmmmaddtobspy
//        }
//#endif // NM_RS_ENABLE_DEBUGDRAW

//#if NM_RS_ENABLE_DEBUGDRAW
//        if (rage::NMRenderBuffer::getInstance())
//        {
//          rage::Vector3 col(0, 0.5, 0);
//          rage::Vector3 col2(0, 0.5, 0);
//          NM_RS_CBU_DRAWPOINT( leftFootCopy, 1.01f, col);//mmmmmaddtobspy
//          NM_RS_CBU_DRAWPOINT( rightFootCopy, 0.01f, col);//mmmmmaddtobspy
//        }
//#endif // NM_RS_ENABLE_DEBUGDRAW
        newMat.FastInverse();
        newMat.Dot(body.m_buttocks.m_tm);

        if (foot.state.m_leftFootBalance)
        {
          newMat.Transform3x3(ro.m_leftFootProbeNormal, upL);
          newMat.Transform(lFootPos, leftFootCopy);
        }
        if (foot.state.m_rightFootBalance)
        {
          newMat.Transform3x3(ro.m_rightFootProbeNormal, upR);
          newMat.Transform(rFootPos, rightFootCopy);
        }
      }
//#if NM_RS_ENABLE_DEBUGDRAW
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 col(0, 0.9f, 0);
//        rage::Vector3 col2(0, 0.5, 0);
//        NM_RS_CBU_DRAWPOINT( leftFootCopy, 1.01f, col);//mmmmmaddtobspy
//        NM_RS_CBU_DRAWPOINT( rightFootCopy, 0.01f, col);//mmmmmaddtobspy
//      }
//#endif // NM_RS_ENABLE_DEBUGDRAW
      float maxFootSpeed = 5.0f;

      float leftDrag = ro.m_dragReduction;
      float rightDrag = ro.m_dragReduction;

      NM_RS_DBG_LOGF(L"leftDrag: %.3f   rightDrag: %.3f", leftDrag, rightDrag);

      // call the IK solver functions
//     profiler->beginSection("IK");

      // !hdd! IK setup takes ~0.002
      {
        NmRsLimbIKInput leftLegIK_input, rightLegIK_input;
        NmRsLimbIKOutput leftLegIK_output, rightLegIK_output;

        // setup left leg packet
        leftLegIK_input.oneDof      = &body.m_leftKnee;
        leftLegIK_input.threeDof    = &body.m_leftHip;
        leftLegIK_input.rootPart    = &body.m_buttocks;
        leftLegIK_input.endPart     = &body.m_leftFoot;
        leftLegIK_input.elbowMat    = &body.m_leftElbowMat;
        //mmmmStaggerFall
        if (!foot.state.m_rightFootBalance && foot.state.m_leftFootBalance && ro.m_plantLeg && (ro.m_balancerState != ro.bal_LeanAgainst))
          leftFootCopy = lFootPos;
        leftLegIK_input.target      = &leftFootCopy;
        leftLegIK_input.twist       = pelvis.m_twistLeft + ro.m_hipTwistOffset;

        leftLegIK_output.oneDof     = body.m_leftKnee;
        leftLegIK_output.threeDof   = body.m_leftHip;

        leftLegIK_input.option.useTargetVelocity = true;
        leftLegIK_input.targetVelocity = &body.m_COMvel; // works a little better than pelvisVel

        leftLegIK_input.direction = 1.0f;
        leftLegIK_input.hingeDirection = -1.0f;
        leftLegIK_input.option.twistIsFixed = false;
        leftLegIK_input.blend = 1.0f;

        if ((!foot.state.m_leftFootBalance && !airborne) || foot.m_stepWithBoth)
        {
          leftLegIK_input.option.advancedIK = true;
          leftLegIK_input.dragReduction = leftDrag;
          leftLegIK_input.straightness = 0.6f;
#if defined(THIGH_ROTATE_EFFECT)
          if (foot.m_leftGround)
            leftLegIK_input.straightness = 0.4f;
          if (foot.m_leftGround && body.m_leftFoot.m_linVel.Dot(ro.m_gUp) < 0.f)
            leftLegIK_input.straightness = 0.15f;
#endif
#if defined(FEEDBACK_EFFECT)
          rage::Vector3 relativeFootVel = body.m_leftFoot.m_linVel - ro.m_leftFootHitPosVel;
          relativeFootVel.Scale(0.05f);
          rage::Vector3 diff = leftFootCopy - (body.m_leftFoot.m_tm.d + relativeFootVel);
          float change = diff.Dot(ro.m_gUp);
          float scale = 4.f;
          leftLegIK_input.straightness = rage::Clamp(change*scale + 0.1f, 0.15f, 0.6f);
#endif
          leftLegIK_input.maxSpeed = maxFootSpeed;
        }
        else
        {
          leftLegIK_input.option.advancedIK = false;
          leftLegIK_input.dragReduction = 0.0f;
        }


        // setup right leg packet
        rightLegIK_input.oneDof     = &body.m_rightKnee;
        rightLegIK_input.threeDof   = &body.m_rightHip;
        rightLegIK_input.rootPart   = &body.m_buttocks;
        rightLegIK_input.endPart    = &body.m_rightFoot;
        rightLegIK_input.elbowMat   = &body.m_rightElbowMat;
        //mmmmStaggerFall
        if (foot.state.m_rightFootBalance && !foot.state.m_leftFootBalance && ro.m_plantLeg && (ro.m_balancerState != ro.bal_LeanAgainst))
          rightFootCopy = rFootPos;//foot.m_rightFootPos;//leftFootCopy;
 
        rightLegIK_input.target     = &rightFootCopy;
        rightLegIK_input.twist      = pelvis.m_twistRight  + ro.m_hipTwistOffset;

        rightLegIK_output.oneDof    = body.m_rightKnee;
        rightLegIK_output.threeDof  = body.m_rightHip;

        rightLegIK_input.option.useTargetVelocity = true;
        rightLegIK_input.targetVelocity = &body.m_COMvel;  // works a little better than pelvisVel

        rightLegIK_input.direction = -1.0f;
        rightLegIK_input.hingeDirection = -1.0f;
        rightLegIK_input.option.twistIsFixed = false;
        rightLegIK_input.blend = 1.0f;

        if ((!foot.state.m_rightFootBalance && !airborne) || foot.m_stepWithBoth)
        {
          rightLegIK_input.option.advancedIK = true;
          rightLegIK_input.dragReduction = rightDrag;
          rightLegIK_input.straightness = 0.6f;
#if defined(THIGH_ROTATE_EFFECT)
          if (foot.m_leftGround)
            rightLegIK_input.straightness = 0.4f;
          if (foot.m_leftGround && body.m_rightFoot.m_linVel.Dot(ro.m_gUp) < 0.f)
            rightLegIK_input.straightness = 0.15f;
#endif
#if defined(FEEDBACK_EFFECT)
          rage::Vector3 relativeFootVel = body.m_rightFoot.m_linVel - ro.m_rightFootHitPosVel;
          relativeFootVel.Scale(0.05f);
          rage::Vector3 diff = rightFootCopy - (body.m_rightFoot.m_tm.d + relativeFootVel);
          float change = diff.Dot(ro.m_gUp);
          float scale = 4.f;
          rightLegIK_input.straightness = rage::Clamp(change*scale + 0.1f, 0.15f, 0.6f);//mmmmBal Foot height really done here
#endif
          NM_RS_DBG_LOGF(L"rightLegIK_input.straightness: %.3f", rightLegIK_input.straightness);

          rightLegIK_input.maxSpeed = maxFootSpeed;
        }
        else
        {
          rightLegIK_input.option.advancedIK = false;
          rightLegIK_input.dragReduction = 0.0f;
        }


        // !hdd! IK solve + advanced sections that follow take 50% of function time
        // the two limbIK functions accound for 70-80% of the block
//#if NM_RS_ENABLE_DEBUGDRAW
//        if (rage::NMRenderBuffer::getInstance())
//        {
//          ///NM_RS_CBU_DRAWPOINT( rightFootCopy, 0.2f, rage::Vector3(1.f, 0.5f, 0.f));//IK target //mmmmmaddtobspy
//          ///NM_RS_CBU_DRAWPOINT( leftFootCopy, 0.2f, rage::Vector3(1.f, 0.8f, 0.f));//IK target //mmmmmaddtobspy
//        }
//#endif

        //add offset to keep the legs apart 
        if (foot.m_stepWithBoth)
        {
          if (foot.state.m_rightFootBalance)
          {
            rightFootCopy += leftFootCopy;
            rightFootCopy *= 0.5f;
            //rightFootCopy = leftFootCopy;

            rightFootCopy += ro.m_left2right*3.5f;
            rightLegIK_input.target = &rightFootCopy;
            //rightFootCopy = leftFootCopy;
          }
          if (foot.state.m_leftFootBalance)
          {
            leftFootCopy += rightFootCopy;
            leftFootCopy *= 0.5f;
            //leftFootCopy = rightFootCopy;

            leftFootCopy -= ro.m_left2right*3.5f;
            leftLegIK_input.target = &leftFootCopy;
            //leftFootCopy = rightFootCopy;
          }
        }
#if !__SPU & ART_ENABLE_BSPY && 0
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftLegIK_input.blend);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftLegIK_input.dragReduction);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftLegIK_input.maxSpeed);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftLegIK_input.straightness);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftLegIK_input.option.advancedIK);
        
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightLegIK_input.blend);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightLegIK_input.dragReduction);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightLegIK_input.maxSpeed);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightLegIK_input.straightness);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightLegIK_input.option.advancedIK);
#endif 
#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "BalIK", leftFootCopy);
        bspyScratchpad(bspyLastSeenAgent, "BalIK", rightFootCopy);
#endif
        
        // solve
        leftLegIK_input.maxReachDistance = -1.f;//don't clamp distance//mmmmtodo try replacing minKneeAngle with this?
        rightLegIK_input.maxReachDistance = -1.f;//don't clamp distance
        limbIK(leftLegIK_input, leftLegIK_output);
        limbIK(rightLegIK_input, rightLegIK_output);

        bool steppingUp = false;
#if NM_STEP_UP
        if (ro.m_stepUp)
        {
          if (leftLegIK_input.option.advancedIK)
            steppingUp = ro.m_leftFootProbeHitPos.z > ro.m_rightFootProbeHitPos.z + 0.1f;              
          else if (rightLegIK_input.option.advancedIK)
            steppingUp = ro.m_rightFootProbeHitPos.z > ro.m_leftFootProbeHitPos.z + 0.1f;              
#if !__SPU & ART_ENABLE_BSPY
          bool ID = steppingUp;
          bspyScratchpad(bspyLastSeenAgent, "ID_SolveStepUp", ID);
#endif
        }
#endif //NM_STEP_UP

        //if moving backwards - don't pushoff
        rage::Vector3 bodyBackLevelled(body.m_COMTM.c);
        ro.levelVector(bodyBackLevelled);
        float forwardsLeanVel = -body.m_lvlCOMvelRelative.Dot(bodyBackLevelled);
        static bool goWithMovement = true;//Used for staggering: lean the ankles to help off balance the character in the direction of movement - stationary->moving quicker
        static float goWithMovementMag = 0.8f;//Used for staggering: 
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (ro.m_pushOffBackwards)
          forwardsLeanVel = 2.f;
#endif
        // ankle extras
        if (leftLegIK_input.option.advancedIK)
        {
          if (ro.m_flatterSwingFeet || steppingUp)
          {
            float ankleLean = getAnkleAngle(body.m_leftKnee.m_position, body.m_leftFoot.m_tm.d, body.m_leftKnee, upL);
            body.m_leftAnkle.m_desiredLean1 = ankleLean;
          }
          else
          {
          //MMMM
          //.a = sideto out
          //.b = up
          //.c = back
          //works unless foot vertical
          rage::Vector3 footVel(body.m_leftFoot.m_linVel - body.m_floorVelocity);
          rage::Vector3 footBack(body.m_leftFoot.m_tm.c);
          ro.levelVector(footVel,0.f);
          ro.levelVector(footBack,0.f);
          footVel.Normalize();
          footBack.Normalize();
          float dum = footVel.Dot(footBack);
          if (dum<-0.3f)//going forward
              matchFootToGround(body.m_leftShin, body.m_leftAnkle, upL, ro.m_stagger);//outputs ankle.m_desiredLean1
          else
              matchFootToGround(body.m_leftShin, body.m_leftAnkle, upL, false);//outputs ankle.m_desiredLean1

          float angle = 0.f;
          bool newAnkles = false;
          if (body.m_COMTM.c.Dot(footVel) > 0.4f)//foot going backwards (but not if sideways backwards)
            newAnkles = true;
          if (!newAnkles)
          {
            rage::Vector3 temp;
            temp.Cross(body.m_leftFoot.m_tm.a, ro.m_gUpReal);//.a foot right
            temp.Normalize();
            angle = temp.Dot(ro.m_leftFootProbeNormal);//angle of the foot if shin is the normal
          }

#if defined(LESS_ACUTE_ANKLES)
          if (foot.m_leftGround && body.m_leftFoot.m_linVel.Dot(ro.m_gUp) < 0.f)
            body.m_leftAnkle.m_desiredLean1 = angle*0.66f + body.m_leftAnkle.m_desiredLean1*0.33f;
          else
            body.m_leftAnkle.m_desiredLean1 = angle*0.8f + body.m_leftAnkle.m_desiredLean1*0.2f;
#else
          if (!newAnkles)
          {
            body.m_leftAnkle.m_desiredLean1 = angle*0.8f + body.m_leftAnkle.m_desiredLean1*0.2f;
            if (!ro.m_stagger)
            {
              if (body.m_COMTM.c.Dot(footVel) < 0.f)
                body.m_leftAnkle.m_desiredLean1 -= 0.2f;
            }
          }
#endif
          //This just makes bent leg stepping look funny and is not necessary and encourages little never ending steps
#if !NM_NEW_BALANCER 
          body.m_leftAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness, 0.0001f, 0.999f));
#endif
        }

        }
        else//stance leg
        {
          if (body.m_lvlCOMvelRelative.Mag() < 1.f && ro.m_flatterStaticFeet)//balance where the balance foot is not the desired
          {
            //go against the movement a bit
            upL -= body.m_lvlCOMvelRelative*0.2f;
            upL.Normalize();
            float ankleLean = getAnkleAngle(body.m_leftKnee.m_position, body.m_leftFoot.m_tm.d, body.m_leftKnee, upL);
            body.m_leftAnkle.m_desiredLean1 = ankleLean;
          }
        else
        {
          float comMag = body.m_lvlCOMvelRelative.Mag();
          if (ro.m_stagger && goWithMovement)
          {
            if (comMag < 1.f)
            {
              upL += body.m_lvlCOMvelRelative*goWithMovementMag*comMag;
              upL.Normalize();
            }
          }
          rage::Vector3 kneePos;
          getLimbHingePos(kneePos, leftLegIK_output.threeDof, leftLegIK_output.oneDof, 1.0f);
          float ankleLean = getAnkleAngle(kneePos, leftFootCopy, leftLegIK_output.oneDof, upL);
          //staggerFallTest maybe
          //lFootPos = (lFootPos + leftFootCopy)/2.f;//mmmmStaggerFall
          //if (ro.m_stagger)
          //  ankleLean = getAnkleAngle(kneePos, lFootPos, leftLegIK_output.oneDof, ro.m_rightFootProbeNormal);//mmmmStaggerFall

          //mmmmtodo SLOPE down = dontPushOff although stop_tripping code may have fixed this, moving backwards - don't pushofff
          //if moving backwards - don't pushoff
		  if (ro.m_stagger)
		  {
		    if (comMag > 1.f)
			  //pushoff with foot when stepping
			  body.m_leftAnkle.m_desiredLean1 = -ankleLean;//-rage::Abs(ankleLean);
			else
			{
			  body.m_leftAnkle.m_desiredLean1 = ankleLean;//-rage::Abs(ankleLean);
			  body.m_rightAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness*ro.m_legStraightness, 0.0001f, 0.999f));
			}

		  }
		  else//(ro.m_stagger)
		  {
          if (forwardsLeanVel > 0.f && (steppingUp || (ro.m_pushOff && foot.m_footChoice != NmRsCBUDynBal_FootState::kNotStepping)))
            //pushoff with foot when stepping
            body.m_leftAnkle.m_desiredLean1 = -ankleLean;//-rage::Abs(ankleLean);
          else
            {
              body.m_leftAnkle.m_desiredLean1 = ankleLean;//-rage::Abs(ankleLean);
			  body.m_rightAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness*ro.m_legStraightness, 0.0001f, 0.999f));
            }
          }

          }
        }
        if (rightLegIK_input.option.advancedIK)
        {
          if (ro.m_flatterSwingFeet || steppingUp)
          {
            //mmmmmNOTE have tried to apply this if body.m_COMvel.Mag() < 1.f wether stance or swing
            float ankleLean = getAnkleAngle(body.m_rightKnee.m_position, body.m_rightFoot.m_tm.d, body.m_rightKnee, upL);
            body.m_rightAnkle.m_desiredLean1 = ankleLean;
          }
          else
          {
          //MMMM
          //.a = sideto out
          //.b = up
          //.c = back
          //works unless foot vertical
          rage::Vector3 footVel(body.m_rightFoot.m_linVel - body.m_floorVelocity);
          rage::Vector3 footBack(body.m_rightFoot.m_tm.c);
          ro.levelVector(footVel,0.f);
          ro.levelVector(footBack,0.f);
          footVel.Normalize();
          footBack.Normalize();
          float dum = footVel.Dot(footBack);
          if (dum<-0.3f)//going forward
              matchFootToGround(body.m_rightShin, body.m_rightAnkle, upR, ro.m_stagger);//outputs ankle.m_desiredLean1
          else
              matchFootToGround(body.m_rightShin, body.m_rightAnkle, upR, false);//outputs ankle.m_desiredLean1

          float angle = 0.f;
          footVel.Normalize();
          bool newAnkles = false;
          if (body.m_COMTM.c.Dot(footVel) > 0.4f)//foot going backwards (but not if sideways backwards)
            newAnkles = true;
          if (!newAnkles)
          {
            rage::Vector3 temp;
              temp.Cross(body.m_rightFoot.m_tm.a, ro.m_gUpReal);
            temp.Normalize();
            angle = temp.Dot(ro.m_rightFootProbeNormal);
          }

#if defined(LESS_ACUTE_ANKLES)
          if (foot.m_leftGround && body.m_rightFoot.m_linVel.Dot(ro.m_gUp) < 0.f)
            body.m_rightAnkle.m_desiredLean1 = angle*0.66f + body.m_rightAnkle.m_desiredLean1*0.33f;
          else
            body.m_rightAnkle.m_desiredLean1 = angle*0.8f + body.m_rightAnkle.m_desiredLean1*0.2f;
#else
          if (!newAnkles)
          {
            body.m_rightAnkle.m_desiredLean1 = angle*0.8f + body.m_rightAnkle.m_desiredLean1*0.2f;
            if (!ro.m_stagger)
            {
              if (body.m_COMTM.c.Dot(footVel) < 0.f)
                body.m_rightAnkle.m_desiredLean1 -= 0.2f;
            }
          }
#endif
          //This just makes bent leg stepping look funny and is not necessary and encourages little never ending steps
#if !NM_NEW_BALANCER 
          body.m_rightAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness, 0.0001f, 0.999f));
#endif
        }
        }
        else//stance foot
        {
          if (body.m_lvlCOMvelRelative.Mag() < 1.f && ro.m_flatterStaticFeet)//balance where the balance foot is not the desired
          {
            //go against the movement a bit
            upR -= body.m_lvlCOMvelRelative*0.2f;
            upR.Normalize();
            float ankleLean = getAnkleAngle(body.m_rightKnee.m_position, body.m_rightFoot.m_tm.d, body.m_rightKnee, upL);
            body.m_rightAnkle.m_desiredLean1 = ankleLean;
          }
        else
        {
		  float comMag = body.m_lvlCOMvelRelative.Mag();
		  if (ro.m_stagger && goWithMovement)
		  {
			if (comMag < 1.f)
			{
			  upR += body.m_lvlCOMvelRelative*goWithMovementMag*comMag;
			  upR.Normalize();
		    }
		  }
          rage::Vector3 kneePos;
          getLimbHingePos(kneePos, rightLegIK_output.threeDof, rightLegIK_output.oneDof, -1.0f);
          float ankleLean = getAnkleAngle(kneePos, rightFootCopy, rightLegIK_output.oneDof, upR);
          //staggerFallTest maybe
          //rFootPos = (rFootPos + rightFootCopy)/2.f;//mmmmStaggerFall
          //if (ro.m_stagger)
          //  ankleLean = getAnkleAngle(kneePos, rFootPos, rightLegIK_output.oneDof, ro.m_rightFootProbeNormal);//mmmmStaggerFall
          if (ro.m_stagger)
		  {
			if (comMag > 1.f)//pushoff
			  body.m_rightAnkle.m_desiredLean1 = -ankleLean;//-rage::Abs(ankleLean);
			else//lean at ankle
			{
			  body.m_rightAnkle.m_desiredLean1 = ankleLean;//-rage::Abs(ankleLean);
			  body.m_rightAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness*ro.m_legStraightness, 0.0001f, 0.999f));
			}			
		  }
		  else//(ro.m_stagger)
		  {
          //mmmmtodo SLOPE down = dontPushOff although stop_tripping code may have fixed this, moving backwards - don't pushofff
          //if moving backwards - don't pushofff
          if (forwardsLeanVel > 0.f && (steppingUp || (ro.m_pushOff && foot.m_footChoice != NmRsCBUDynBal_FootState::kNotStepping)))
            //pushoff with foot when stepping
            body.m_rightAnkle.m_desiredLean1 = -ankleLean;//-rage::Abs(ankleLean);
          else
            {
              body.m_rightAnkle.m_desiredLean1 = ankleLean;//-rage::Abs(ankleLean);
				body.m_rightAnkle.m_desiredLean1 += rage::AcosfSafe(rage::Clamp(ro.m_legStraightness*ro.m_legStraightness, 0.0001f, 0.999f));
        }
		  }

        }
        }

//      profiler->endSection();


        // send results back to the body packet
        body.m_leftKnee  = leftLegIK_output.oneDof;
        body.m_leftHip   = leftLegIK_output.threeDof;
        body.m_rightKnee = rightLegIK_output.oneDof;
        body.m_rightHip  = rightLegIK_output.threeDof;
      }

      NM_RS_DBG_LOGF(L"ankleLean: %.3f", pelvis.m_ankleLean);

      float left1, left2, right1, right2;
      body.m_leftHip.getLimitOvershoot(left1, left2);
      body.m_rightHip.getLimitOvershoot(right1, right2);

      // add on ankle lean offset
      if (foot.state.m_leftFootBalance)
        body.m_leftAnkle.m_desiredLean1 += pelvis.m_ankleLean + hipRollLean;
      body.m_leftAnkle.m_desiredLean2 = pelvis.m_twistLeft * 0.5f;

      if (foot.state.m_rightFootBalance)
        body.m_rightAnkle.m_desiredLean1 += pelvis.m_ankleLean - hipRollLean;
      body.m_rightAnkle.m_desiredLean2 = pelvis.m_twistRight * 0.5f;

      // this block below allows lean2 to stay inside limits by rolling the hip appropriately.
      // this appears to be the most important part of keeping the character on his feet from sideways hits
      //mmmmherebalance as this is based on hip limits will be different for cowboy
      float exceedLeft, exceedRight;
      exceedRight = rage::Max(-right2, left2);
      exceedLeft  = rage::Max(-left2, right2);

      if (exceedRight > 0 && exceedLeft <= 0)
      {
        body.m_rightHip.m_desiredLean2 += exceedRight;
        if (foot.state.m_leftFootBalance)
          body.m_leftHip.m_desiredLean2 -= exceedRight;

        body.m_spine0.m_desiredLean2 -= exceedRight;

        NM_RS_DBG_LOGF(L"exceedRight: %.3f", exceedRight);
      }

      if (exceedLeft > 0 && exceedRight <= 0)
      {
        body.m_leftHip.m_desiredLean2 += exceedLeft;
        if (foot.state.m_rightFootBalance)
          body.m_rightHip.m_desiredLean2 -= exceedLeft;

        body.m_spine0.m_desiredLean2 -= exceedLeft;

        NM_RS_DBG_LOGF(L"exceedLeft: %.3f", exceedLeft);
      }

      return true;
    }
}

using namespace ART;

void NmRsCBU_DBMBalanceSolve(rage::sysTaskParameters& param)
{
  Assert(param.ReadOnlyCount == 3);
  Assert(param.ReadOnly[0].Data);
  Assert(param.ReadOnly[0].Size == sizeof(NmRsCBUDynBal_ReadOnly));
  Assert(param.ReadOnly[1].Data);
  Assert(param.ReadOnly[1].Size == sizeof(NmRsCBUDynBal_FootState));
  Assert(param.ReadOnly[2].Data);
  Assert(param.ReadOnly[2].Size == sizeof(NmRsCBUDynBal_PelvisState));

  Assert(param.Input.Data);
  Assert(param.Input.Size == sizeof(NmRsCBUDynBal_BodyPacket));

  const NmRsCBUDynBal_ReadOnly* ro = static_cast<const NmRsCBUDynBal_ReadOnly*>(param.ReadOnly[0].Data);
  const NmRsCBUDynBal_FootState* foot = static_cast<const NmRsCBUDynBal_FootState*>(param.ReadOnly[1].Data);
  const NmRsCBUDynBal_PelvisState* pelvis = static_cast<const NmRsCBUDynBal_PelvisState*>(param.ReadOnly[2].Data);
  NmRsCBUDynBal_BodyPacket* body = static_cast<NmRsCBUDynBal_BodyPacket*>(param.Input.Data);

  dbmBalanceSolve(*ro, *foot, *pelvis, *body);
}


#if __SPU
#include "vector/matrix34.cpp"
#include "vector/quaternion.cpp"
#include "NmRsUtils.cpp"
#include "NmRsIK.cpp"
#endif // __SPU

