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

/**
 * -------------------------- IMPORTANT --------------------------
 * This file contains the Rockstar IK solvers and helper functions.
 * It must be compilable on SPU, so no external references outside of the RAGE
 * vector/math libraries and our own math macros.
 */

#include "NmRsCommon.h"
#include "NmRsMinimalInclude.h"
#include "NmRsIK.h"

#if !__SPU & ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY

#define NM_VERBOSE_IK_DEBUG 0

namespace ART    
{ 
#if NM_CHECK_VALID_VALUES
  inline void checkValidVector(rage::Vector3 &v)
  {
    if (v.x != v.x)
    {
      AssertMsg(v.x == v.x, "NmRsIk.cpp ART::checkValidVector(Vector3 &v) - invalid input");
      v.x = 1.0f;
      v.y = 0.0f;
      v.z = 0.0f;
      v.w = 0.0f;
    }
  }
#endif

  /**
  * finds lean/twist based on smallest arc from e1 (comfy angles) to endPosLocal
  * Used in ::limbIK()
  */
  inline void getFreeLean(const rage::Quaternion &addedRot,
    rage::Vector3& endPosLocal, 
    rage::Vector3& e1, 
    float &lean1, 
    float &lean2, 
    float &twist)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("getFreeLean");
#endif

    rage::Vector3 twistSwing;
    rage::Quaternion q1, q2;

    // find quaternion that rotates epl -> e1
#if NM_CHECK_VALID_VALUES
    checkValidVector(e1);
    checkValidVector(endPosLocal);
#endif
    q1.FromVectors(e1, endPosLocal);
    q2.Multiply(q1, addedRot);

    // convert rotation to twist/swing, twist on z axis
    twistSwing = rsQuatToRageDriveTwistSwing(q2);

    lean1 = twistSwing.y;
    lean2 = twistSwing.z;
    twist = twistSwing.x;

#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("getFreeLean");
#endif
  }

  inline void getFreeLean2(const rage::Vector3 &poleVector,
    rage::Vector3& endPosLocal, 
    rage::Vector3& e1, 
    float &lean1, 
    float &lean2, 
    float &twist,
    const NmRsLimbIKInput& input)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("getFreeLean2");
#endif

    rage::Vector3 twistSwing;
    rage::Quaternion q1, q2, q3;

    // find quaternion that rotates epl -> e1
#if NM_CHECK_VALID_VALUES
    checkValidVector(e1);
    checkValidVector(endPosLocal);
#endif
    q1.FromVectors(e1, endPosLocal);
    rage::Matrix34 mat; mat.FromQuaternion(q1);

    rage::Vector3 adjustedPole = poleVector - endPosLocal*poleVector.Dot(endPosLocal)/endPosLocal.Mag2();
    adjustedPole.Normalize();
    // TDL below probably shouldn't change the elbow pole
    rage::Vector3 adjustedElbowPole = mat.a - endPosLocal*mat.a.Dot(endPosLocal)/endPosLocal.Mag2();
    adjustedElbowPole.Normalize();
#if NM_CHECK_VALID_VALUES
    checkValidVector(adjustedElbowPole);
    checkValidVector(adjustedPole);
#endif
    q2.FromVectors(adjustedElbowPole,adjustedPole);

    // SOME hackery so that the extra twist is always the right way [for the PointGun].
    rage::Vector3 extraTwistV;
    float extraTwistM;
    q2.ToRotation(extraTwistV,extraTwistM);
    extraTwistV.Scale(-input.direction*extraTwistM);
    if (extraTwistV.Dot(mat.c)<0.f)
    {
      q2.FromVectors(adjustedPole,adjustedElbowPole);
      q2.ToRotation(extraTwistV,extraTwistM);
      extraTwistV.Scale(extraTwistM);
    }

    q3.Multiply(q2,q1);

    // convert rotation to twist/swing, twist on z axis
    twistSwing = rsQuatToRageDriveTwistSwing(q3);

    lean1 = twistSwing.y;
    lean2 = twistSwing.z;
    twist = twistSwing.x;

    /*     
    // convert to world space
    rage::Matrix34 shoulderMat1;
    shoulderMat1 = input.threeDof->m_matrix1;

    rage::Vector3 adjustedPoleWorld;
    rage::Vector3 adjustedElbowPoleWorld;

    shoulderMat1.Transform3x3(adjustedPole,adjustedPoleWorld);
    shoulderMat1.Transform3x3(adjustedElbowPole,adjustedElbowPoleWorld);

    rage::Vector3 elbowP = input.oneDof->m_position;



    //   NM_RS_CBU_DRAWPOINT(adjustedPole,1.f,rage::Vector3(1.f,0.f,0.f));
    //  NM_RS_CBU_DRAWPOINT(adjustedElbowPole,1.f,rage::Vector3(0.f,0.f,1.f));

    NM_RS_CBU_DRAWLINE(elbowP,elbowP + adjustedPoleWorld,rage::Vector3(1.f,0.f,0.f));
    NM_RS_CBU_DRAWLINE(elbowP,elbowP + adjustedElbowPoleWorld,rage::Vector3(0.f,0.f,1.f));

    rage::GetRageProfileDraw().Begin();

    //rage::grcDrawLine(elbowP, elbowP + adjustedPoleWorld, rage::Color32(255,0,0));

    rage::Vector3 mataW;
    rage::Vector3 matbW;
    rage::Vector3 matcW;

    shoulderMat1.Transform3x3(mat.a,mataW);
    shoulderMat1.Transform3x3(mat.b,matbW);
    shoulderMat1.Transform3x3(mat.c,matcW);

    rage::grcDrawLine(elbowP, elbowP + mataW, rage::Color32(255,0,0));
    rage::grcDrawLine(elbowP, elbowP + matbW, rage::Color32(0,255,0));
    rage::grcDrawLine(elbowP, elbowP + matcW, rage::Color32(0,0,255));

    rage::Vector3 extraTwistVW;
    shoulderMat1.Transform3x3(extraTwistV,extraTwistVW);

    rage::grcDrawLine(shoulderMat1.d, shoulderMat1.d + extraTwistVW, rage::Color32(255,255,255));

    rage::GetRageProfileDraw().End();
    */

#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("getFreeLean2");
#endif
  }

#if NM_UNUSED_CODE
  /**
  * finds leans based on twist being fixed. Needed for foot IK where legs have little twist
  * Used in ::limbIK()
  */
  inline void getFixedLean(const rage::Quaternion &addedRot,
    rage::Vector3& endPosLocal, 
    rage::Vector3& e1, 
    float &lean1, 
    float &lean2, 
    float &twist)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("getFixedLean");
#endif

    rage::Vector3 l0, l1;

    l0.Set(e1.y - endPosLocal.y, endPosLocal.x - e1.x, 0.0f);
    float dot = e1.Dot(l0) / (l0.Dot(l0) + NM_RS_FLOATEPS);

    l1.SetScaled(l0, dot);

    float d = e1.Dist(l1) + NM_RS_FLOATEPS;

    float ds2 = e1.Dist(endPosLocal);
    float cosOmega = (2.0f * rage::square(d) - rage::square(ds2)) / (2.0f * rage::square(d));

    float omega = rage::AcosfSafe(cosOmega);

    l0.Normalize();

    rage::Quaternion q1, q2;
    q1.FromRotation(l0, omega);
    q2.Multiply(q1, addedRot);
    rage::Vector3 twistSwing = rsQuatToRageDriveTwistSwing(q2);

    lean1 = twistSwing.y;
    lean2 = twistSwing.z;
    twist = twistSwing.x;

#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("getFixedLean");
#endif
  }
#endif

  /**
   * compensates for rotational and linear velocity of shoulder/thigh.
   * modifies the <tt>target</tt> input parameter.
   * Used in ::limbIK()
   */
  inline rage::Vector3 getStabilisedPos(const NmRsLimbIKInput& input)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("getStabilisedPos");
#endif

    Assert(input.target->x == input.target->x);
    if (input.option.useTargetVelocity)
    {
      Assert(input.targetVelocity->x == input.targetVelocity->x);
    }

    rage::Vector3 toEndPos, vel1, vel2, result;
    rage::Vector3 timeLagDR;

    timeLagDR.Set(
      (input.threeDof->m_muscleDamping / (input.threeDof->m_muscleStrength + 0.1f)) * input.dragReduction);

      toEndPos.Subtract(*input.target, input.threeDof->m_matrix1.d);

    /**
     * Compensate for angular motion of the clavicle / pelvis.
     */

    // Find the linear velocity a point (input target in this case) would have when being offset from
    // rootPart rotating with angVel: vel = angVel x offset.
    // Note that the arguments of cross below are swapped - this is because we want to compensate for angular motion.
    // Note that rootPart is clavicle / pelvis part.
    vel1.Cross(toEndPos, input.rootPart->m_angVel);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    const rage::Vector3 colYellow(1.0f, 1.0f, 0.0f);
    const rage::Vector3 colDeepSkyBlue(0.0f, 0.749f, 1.0f);
    BSPY_DRAW_LINE(input.rootPart->m_tm.d, input.rootPart->m_tm.d + input.rootPart->m_angVel, colYellow); // Angular velocity.
    BSPY_DRAW_LINE(*input.target, *input.target + vel1, colDeepSkyBlue); // Linear velocity.
#endif

    vel1.Multiply(timeLagDR);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    const rage::Vector3 colWhite(1.0f, 1.0f, 1.0f);
    BSPY_DRAW_LINE(*input.target, *input.target + vel1, colWhite);
#endif

    // BBDD: Keep the radial distance from rootPart invariant i.e. equal to dist1.
    const float dist1 = toEndPos.Mag();
    toEndPos += vel1;
    const float dist2 = toEndPos.Mag();
    toEndPos.Normalize();

    vel1.AddScaled(toEndPos, (dist1 - dist2));

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    const rage::Vector3 colRed(1.0f, 0.0f, 0.0f);
    BSPY_DRAW_LINE(*input.target, *input.target + vel1, colRed);
#endif

    /**
     * Compensate for linear motion of the clavicle / pelvis.
     */

    // Calculate relative velocity of the rootPart with respect to target velocity.
    vel2 = input.rootPart->m_linVel;
    if (input.option.useTargetVelocity)
    {
      vel2.Subtract(*input.targetVelocity);
    }

    vel2.Multiply(timeLagDR);

    // Subtraction is used here because we want to compensate.
    vel1.Subtract(vel2);

    // Apply net velocity offset to input target.
    result.Add(*input.target, vel1);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    const rage::Vector3 colOrange (1.0f, 0.7f, 0.0f);
    BSPY_DRAW_LINE(*input.target, *input.target + vel1, colOrange);
#endif

    // Clamp to minimal distance from threeDof along direction to original input target.
    const float minTargetDistance = 0.1f; // BBDD: Warning, rig dependent value, expose.

    rage::Vector3 targetDir;
    targetDir.Subtract(*input.target, input.threeDof->m_matrix1.d);
    targetDir.Normalize();

    const float forwardAmount = (result - input.threeDof->m_matrix1.d).Dot(targetDir);
    if (forwardAmount < minTargetDistance)
    {
      result.AddScaled(targetDir, minTargetDistance - forwardAmount);
    }

#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("getStabilisedPos");
#endif

    return result;
  }

#if NM_USE_IK_SELF_AVOIDANCE
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  // BBDD Self avoidance tech.
  inline rage::Vector3 calcPointOnPlane(const rage::Vector3& point, const rage::Vector3& planeNormal, const rage::Vector3& planePoint)
  {
    return point - (planeNormal.Dot(point - planePoint) * planeNormal);
  }

  // BBDD Self avoidance tech.
  inline rage::Vector3 calcDirectionOnPlane(const rage::Vector3& point, const rage::Vector3& planeNormal)
  {
    return point - (planeNormal.Dot(point) * planeNormal);
  }

  /** BBDD Self avoidance tech.
   * calcAngleLimitedTarget() function limits angular distance between effector and reach target.
   * One can think of this as a method for swiveling a target about an axis going through some obstacle.
   * It is used for self-avoidance, to generate intermediate targets that will guide the effector around the torso.
   * The reach frame is a frame centered on the chest with up and front direction taken from character spine
   * up and forwards correspondingly. The adjusted target should be "closer" to the effector than the
   * input target and hence more attainable. How much closer is determined by angleLimit and angleFraction.
   * The angleLimit is the maximum angular displacement between effector and adjusted target,
   * the angleFraction is the fraction of the arc (effector to target).
   * The adjusted target is found by rotating input target towards effector about the reach frame up.
   * The direction of rotation is in the direction of the shortest arc from "reach-fwd" to target.
   */
  inline rage::Vector3 calcAngleLimitedTarget(
    const rage::Vector3& target,          // Target position.
    const rage::Vector3& effector,        // Effector position.
    float angleFraction,                  // Place the adjusted target this much along the arc between effector and target.
    float angleLimit,                     // Max value on the effector to adjusted target offset.
    // "Reach frame" parameters define the frame of rotation (origin, up and forward).
    const rage::Vector3& reachOrg,         // Origin.
    const rage::Vector3& reachUp,          // Up direction.
    const rage::Vector3& reachFwd)         // Forward.
  {
    Assert(reachUp.Mag() > 0.01f);

    rage::Vector3 up;
    up.Normalize(reachUp);

    // 1 Calculate the projected forward, target and effector directions.
    rage::Vector3 f(calcDirectionOnPlane(reachFwd, up));
    rage::Vector3 t(calcPointOnPlane(target, up, reachOrg) - reachOrg);
    rage::Vector3 e(calcPointOnPlane(effector, up, reachOrg) - reachOrg);
    f.Normalize();
    t.Normalize();
    e.Normalize();

    // 2 Find the angle offset between effector and target.
    const float targetAngle = acosf(rage::Clamp(t.Dot(f), -1.0f, 1.0f));
    rage::Vector3 targetAngleAxis;
    targetAngleAxis.Cross(f, t);
    targetAngleAxis.Normalize();
    const float eDotF = rage::Clamp(e.Dot(f), -1.0f, 1.0f);
    rage::Vector3 fCrossE;
    fCrossE.Cross(f, e);
    const float effectorAngle = acosf(eDotF) * (targetAngleAxis.Dot(fCrossE) > 0.0f ? 1.0f : -1.0f);
    const float targetToEffectorAngle = effectorAngle - targetAngle;
    // Total effector to target offset.
    const float effectorToTargetOffset = -targetToEffectorAngle;

    // 3 Calculate adjustment given angleLimit and angleFraction.
    const float desiredEffectorToTargetOffset = effectorToTargetOffset * angleFraction;
    const float clampedEffectorToTargetOffset = rage::Clamp(desiredEffectorToTargetOffset, -angleLimit, angleLimit);

    // 4 Rotate input target towards effector.
    rage::Quaternion q;
    q.FromRotation(targetAngleAxis, targetToEffectorAngle + clampedEffectorToTargetOffset);
    rage::Vector3 adjustedTarget;
    q.Transform(target - reachOrg, adjustedTarget);
    adjustedTarget += reachOrg;

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG // BBDD Debug draw is disabled.
    BSPY_DRAW_LINE(reachOrg, reachOrg + e, rage::Vector3(1.0f, 0.0f, 1.0f));
    BSPY_DRAW_LINE(reachOrg, reachOrg + t, rage::Vector3(1.0f, 0.0f, 0.0f));
    BSPY_DRAW_LINE(reachOrg, reachOrg + f, rage::Vector3(0.0f, 1.0f, 0.0f));
    BSPY_DRAW_POINT(adjustedTarget, 0.05f, rage::Vector3(1.0f, 1.0f, 0.0f));
#endif

    return adjustedTarget;
  }

  // BBDD Self avoidance tech. Check if given target is within character spine bounds.
  inline bool inSpineBounds(const NmRsLimbIKInput& input, float offset)
  {
    const rage::Vector3 pelvisPos = input.selfAvoidance.commonSelfAvoidanceParams.pelvisPos;
    const rage::Vector3 chestPos = input.selfAvoidance.commonSelfAvoidanceParams.chestPos;
    rage::Vector3 upSpine = chestPos - pelvisPos;

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG && 0 // BBDD Debug draw disabled.
    BSPY_DRAW_LINE(pelvisPos, pelvisPos + upSpine, rage::Vector3(1.0f, 1.0f, 0.0f));
#endif

    const float spineLength = upSpine.Mag();
    upSpine.Normalize();
    rage::Vector3 targetPos = *(input.target) - pelvisPos;
    const float dotUp = targetPos.Dot(upSpine);
    if ((dotUp > (spineLength + offset)) || (dotUp < -offset))
    {
      return false; // Outside of the spine, don't self avoid.
    }
    else
    {
      return true;
    }
  }

  // BBDD Self avoidance tech.
  inline rage::Vector3 getSelfAvoidPolarPos(NmRsLimbIKInput& input,const rage::Vector3& inputTarget)
  {
    // Self avoidance radius.
    const float radius = input.selfAvoidance.polarSelfAvoidanceParams.radius;
    Assert(radius > 0.0f);

    // 1. Check if target is within the capsule bounds, if not then don't self avoid.
    const rage::Vector3 pelvisPos = input.selfAvoidance.commonSelfAvoidanceParams.pelvisPos;
    const rage::Vector3 chestPos = input.selfAvoidance.commonSelfAvoidanceParams.chestPos;
    rage::Vector3 upSpine = chestPos - pelvisPos;

    // BSpy debug draw.
#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_LINE(pelvisPos, pelvisPos + upSpine, rage::Vector3(1.0f, 1.0f, 0.0f));
#endif

    const float spineLength = upSpine.Mag();
    upSpine.Normalize();
    rage::Vector3 targetPos = inputTarget - pelvisPos;
    const float dotUp = targetPos.Dot(upSpine);
    if ((dotUp > (spineLength + radius)) || (dotUp < -radius))
    {
      return inputTarget; // Outside of the spine capsule.
    }

    // 2. Convert target into local space of spine.
    // a) Get a reference frame to work with.
    rage::Vector3 fwd;
    fwd.Cross(upSpine, input.selfAvoidance.commonSelfAvoidanceParams.rightDir);
    fwd.Normalize();
#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_LINE(pelvisPos, pelvisPos+fwd, rage::Vector3(1.0f, 0.0f, 1.0f));
#endif
    rage::Vector3 side;
    side.Cross(fwd, upSpine);
    side.Normalize();
#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_LINE(pelvisPos, pelvisPos+side, rage::Vector3(0.0f, 0.0f, 1.0f));
#endif
    // 2 b) Get local space current and target positions.
    rage::Vector3 currentPos = input.endPart->m_tm.d;
    currentPos -= pelvisPos;
    const rage::Vector3 currentLocal(currentPos.Dot(side), 0.0f, currentPos.Dot(fwd));
    const rage::Vector3 targetLocal(targetPos.Dot(side), 0.0f, targetPos.Dot(fwd));
    const rage::Vector3 toTargetLocal = targetLocal - currentLocal;

    // Do not self avoid.when hand position overlaps its reaching target.
    if (toTargetLocal.Mag2() < 1.e-6f)
    {
      return inputTarget;
    }

    // 3. Calculate the polar target in local space (the linear target is just targetLocal).
    // This is simply the tangent (rate of change) vector of the polar path, which is a linear interpolation
    // of the angle and length respectively.
    const float currentLocalMag = currentLocal.Mag();
    const float targetLocalMag = targetLocal.Mag();
    float magRateOfChange = targetLocalMag - currentLocalMag;
    float targetAngle = atan2f(targetLocal.x, targetLocal.z);
    float currentAngle = atan2f(currentLocal.x, currentLocal.z);
    float angleRateOfChange = targetAngle - currentAngle;
    rage::Vector3 currentLocalDir = currentLocal;
    currentLocalDir.Normalize();
    const rage::Vector3 lateral(currentLocal.z, 0.0f, -currentLocal.x); // BBDD I.e. vector clockwise rotation by 90 DEG. Note it has magnitude equal to current local magnitude.
    const rage::Vector3 polarTarget = currentLocal + (currentLocalDir * magRateOfChange) + (lateral * angleRateOfChange);

    // 4. Get the closest distance reached if we use an angular motion.
    const float minMagPolar = rage::Min(currentLocalMag, targetLocalMag);

    // 5. Get the closest distance reached if we use a linear motion.
    const float closestDotLinear = rage::Clamp(-currentLocal.Dot(toTargetLocal) / toTargetLocal.Mag2(), 0.0f, 1.0f);
    float minMagLinear = (currentLocal + (toTargetLocal * closestDotLinear)).Mag();

    // 6. Special case: if the linear path is going the long way around then we need to give it a negative magnitude.
    if ((angleRateOfChange * lateral.Dot(toTargetLocal)) < 0.0f)
    {
      minMagLinear = -minMagLinear;
    }

    // 7. Get blend of linear and polar based on these two closest distances,
    // i.e. we want more polar motion when the linear motion gets close to the spine.
    const float W = rage::Clamp((radius - minMagLinear) / (minMagPolar - minMagLinear), 0.0f, 1.0f);

    // 8. Blend the two together.
    const rage::Vector3 resultLocal = (polarTarget * W) + (targetLocal * (1.0f - W));

    // BBDD TODO: Consider clamping resultLocal.x and resultLocal.z values here (see below).

    // 9. Convert back to world space and return.
    rage::Vector3 resultTarget = pelvisPos + (upSpine * dotUp) + (side * resultLocal.x) + (fwd * resultLocal.z);

    // BBDD TODO: Consider adding procedural twist calculation and across-torso reaching support, see getSelfAvoidPos function for reference.

    // Test if vector is valid.
    Assert((resultTarget.x * 0 == 0) && (resultTarget.y * 0 == 0) && (resultTarget.z * 0 == 0));

    // BSpy debug draw.
#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    const rage::Vector3 colDeepSkyBlue(0.0f, 0.749f, 1.0f);
    const rage::Vector3 colWhite      (1.0f, 1.0f, 1.0f);
    const rage::Vector3 colOrange     (1.0f, 0.7f, 0.0f);
    const rage::Vector3 colYellow     (1.0f, 1.0f, 0.0f);
    BSPY_DRAW_POINT((pelvisPos + (upSpine * dotUp)), 0.05f, colYellow);
    BSPY_DRAW_POINT(inputTarget, 0.11f, colOrange);
    BSPY_DRAW_POINT(resultTarget, 0.1f, colDeepSkyBlue);
    BSPY_DRAW_POINT(resultTarget, 0.05f, colWhite);
    const rage::Vector3 from = pelvisPos + (upSpine * dotUp);
    BSPY_DRAW_LINE(from, from + (side * resultLocal.x), colDeepSkyBlue);
    BSPY_DRAW_LINE(from, from + (fwd * resultLocal.z), colWhite);
#endif

    return resultTarget;
  }

  // BBDD Self avoidance tech.
  inline rage::Vector3 getSelfAvoidPos(NmRsLimbIKInput& input, const rage::Vector3& inputTarget)
  {
    const rage::Vector3 effector = input.endPart->m_tm.d;

    const rage::Vector3 chestPos = input.selfAvoidance.commonSelfAvoidanceParams.chestPos;
    const rage::Vector3 pelvisPos = input.selfAvoidance.commonSelfAvoidanceParams.pelvisPos;

    rage::Vector3 upSpine = chestPos - pelvisPos;
    upSpine.Normalize();

    rage::Vector3 forwardDir;
    forwardDir.Cross(upSpine, input.selfAvoidance.commonSelfAvoidanceParams.rightDir);
    forwardDir.Normalize();

    // Swings the target about the torso.
    rage::Vector3 result = calcAngleLimitedTarget(
      inputTarget,
      effector,
      input.selfAvoidance.selfAvoidanceParams.torsoSwingFraction,    // Place the adjusted target this much along the arc between effector and target, value in range [0,1].
      input.selfAvoidance.selfAvoidanceParams.maxTorsoSwingAngleRad, // Max value on the effector to adjusted target offset.
      chestPos,
      upSpine,
      forwardDir);

    // When target is within reaching range, add distance offset to target
    // so that when character is reaching across its own body arms go around it.
    rage::Vector3 t(calcPointOnPlane(result, upSpine, chestPos) - chestPos);
    rage::Vector3 e(calcPointOnPlane(effector, upSpine, chestPos) - chestPos);
    const float tr = t.Mag();
    const float er = e.Mag();
    e.Normalize();
    t.Normalize();

    // Calculate angle between original input target and effector projections.
    rage::Vector3 ot(calcPointOnPlane(inputTarget, upSpine, chestPos) - chestPos);
    ot.Normalize();
    const float cosTheta = rage::Clamp(e.Dot(ot), -1.0f, 1.0f);

    // Calculate angle between wrist (effector) and shoulder effector projections.
    rage::Vector3 es(calcPointOnPlane(input.threeDof->m_position, upSpine, chestPos) - chestPos);
    es.Normalize();
    const float cosPhi = rage::Clamp(e.Dot(es), -1.0f, 1.0f);
    // Total arm length.
    const float wrist2ElbowDst = (effector - input.oneDof->m_position).Mag();
    const float elbow2ShoulderDst = (input.oneDof->m_position - input.threeDof->m_position).Mag();
    const float armLength = wrist2ElbowDst + elbow2ShoulderDst;
    // Calculate amount of self avoidance offset applied when angle from effector to target
    // is greater then right angle i.e. when total offset is a blend between where effector currently is to
    // value that is a product of armLength and selfAvoidAmount. SelfAvoidAmount is in a range between [0, 1].
    const float l = armLength * input.selfAvoidance.selfAvoidanceParams.selfAvoidAmount;
    Assert(l >= 0.0f);
    // Restrict self avoidance to operate on targets that are within character spine bounds only.
    const bool isInSpineBounds = (input.selfAvoidance.selfAvoidanceParams.selfAvoidIfInSpineBoundsOnly) ? inSpineBounds(input, l) : true;
    if ((tr < armLength) && isInSpineBounds)
    {
      const float w = rage::Abs(cosTheta);

      const float offset = er * (1.0f - w) + rage::Selectf(cosTheta, tr, l) * w;

      t *= (rage::Abs(tr - offset));

      // Apply self avoidance offset to the target, that was swung about the torso.
      result += t;

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
      const rage::Vector3 colOrange(1.0f, 0.7f, 0.0f);
      const rage::Vector3 colDeepSkyBlue(0.0f, 0.749f, 1.0f);
      BSPY_DRAW_LINE(pelvisPos, pelvisPos + (chestPos - pelvisPos), colDeepSkyBlue);
      BSPY_DRAW_POINT(inputTarget, 0.05f, colOrange);
      // Draw result.
      rage::Vector3 resultFrom = upSpine;
      resultFrom.Scale((result - pelvisPos).Dot(upSpine));
      BSPY_DRAW_LINE(pelvisPos + resultFrom, result, colDeepSkyBlue);
      BSPY_DRAW_POINT(result, 0.04f, colDeepSkyBlue);
#endif

      // Calculate IK twist, overwriting original value.
      // BBDD WARNING: This shall be used only for arms, it hasn't been tested for legs yet.
      if (input.selfAvoidance.selfAvoidanceParams.overwriteTwist)
      {
        // Min and max twist values when reaching across the body.
        // BBDD. Test & tune twist with the new rig shoulder joint twist limit setup.
        const float maxTwistRad = PI / 4.0f;
        const float minTwistRad = PI / 2.0f;
        input.twist = rage::Selectf(cosPhi, -minTwistRad, -maxTwistRad) * cosPhi;
      }

      return result;
    }
    else
    {
      return inputTarget;
    }
  }
  // BBDD Self avoidance tech.
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
#endif // NM_USE_IK_SELF_AVOIDANCE

#if 0
  /*
   *  solve 2-bone IK task
   *
   *  reworked to operate independent of joint parameterization
   *
   */
  void limbIK2(NmRsLimbIKInput& input, NmRsLimbIKOutput& output)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("limbIK2")
#endif
    /*
     *  get the output target from getStabilisedPos() if
     *  a dragReduction was specified; otherwise just copy
     */
    if (input.dragReduction > 0.0f)
      output.target.Set(getStabilisedPos(input));
    else
      output.target.Set(*input.target);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_POINT(output.target, 0.1f, rage::Vector3(1.f,1.f,0.f));
#endif

    /*
     *  adjust target to limit speed. compute blend value
     */
    float advBlend = 0.f;
    if (input.option.advancedIK)
      advBlend = advancedIK(input, output);

    /*
     *  operates in zeroed root local frame unless
     *  otherwise indicated.
     */
    rage::Matrix34 rootTM1 = input.threeDof->m_matrix1;
    rage::Matrix34 rootTM2 = input.threeDof->m_matrix2;
    rage::Matrix34 hingeTM1; hingeTM1.DotTranspose(input.oneDof->m_matrix1, rootTM2);
    rage::Matrix34 hingeTM2; hingeTM2.DotTranspose(input.oneDof->m_matrix2, rootTM2);
    rage::Vector3 h; rootTM2.UnTransform(input.oneDof->m_position, h);

    rage::Vector3 effectorPosition; effectorPosition.Zero();
    if(input.effectorOffset)
      input.endPart->m_tm.Transform(*input.effectorOffset, effectorPosition);
    effectorPosition.Add(input.endPart->m_tm.d);
    rage::Vector3 e; rootTM2.UnTransform(effectorPosition, e);

    hingeTM2.UnTransform(e);
    hingeTM1.Transform(e);
    rage::Vector3 t; rootTM1.UnTransform(*input.target, t);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      // draw target and starting limb configuration
      rage::Vector3 a, b;
      a = rootTM1.d;
      rootTM1.Transform(t, b);
      BSPY_DRAW_POINT(b, 0.05f, rage::Vector3(1.f,1.f,0.f));
      rootTM1.Transform(h, b);
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.f,0.f));
      rootTM1.Transform(e, a);
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.f,0.f));

      // check joint frames look ok in world space
      BSPY_DRAW_COORDINATEFRAME(rootTM1, 0.01f);
      rage::Matrix34 c;
      c.Dot(hingeTM1, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(c, 0.01f);
      c.Dot(hingeTM2, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(c, 0.01f);
      c.Dot(rootTM2, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(input.threeDof->m_matrix2, 0.01f);

      // draw the actual limb configuration??
      BSPY_DRAW_LINE(rootTM1.d, input.oneDof->m_position, rage::Vector3(0.f,0.f,1.f));
      BSPY_DRAW_LINE(input.oneDof->m_position, input.endPart->m_tm.d, rage::Vector3(0.f,0.f,1.f));
    }
#endif

    /*
     *  bend hinge to make root to effector and root
     *  to target distances equal.  bones are not 
     *  guaranteed to be ortho to hinge axis (z), so
     *  flat them against z before measuring.
     */
    rage::Vector3 rh = h;
    rage::Vector3 he = e-h;
    float rtMag = t.Mag();
    float reMag = e.Mag();
    float hAngle = 0;

    /*
     *  arms should never be quite straight or elbow math
     *  gets upset
     */
    if(rtMag > reMag)
      rtMag = reMag - 0.001f;

    /*
     *  math to get hinge angle from desired root effector
     *  distance and zero root and effector position in
     *  hinge space.  courtesy of AR (as in: ask him if you
     *  need an explanation of how it works).
     * 
     *  assumes hinge axis is hinge tm z
     */
    rage::Vector3 pe, pr;
    hingeTM1.UnTransform3x3((e-h), pe);
    hingeTM1.UnTransform3x3(-h, pr);
    float peMag = pe.Mag();
    float prMag = pr.Mag();
    float a = -2.f*pe.x*pr.x-2.f*pe.y*pr.y;
    float b = -2.f*pe.x*pr.y+2.f*pe.y*pr.x;
    float c = rtMag*rtMag-peMag*peMag-prMag*prMag+2.f*pe.z*pr.z;
    // [jrp] had to clamp the input to sqrtf as it occasionally shows up as a very small negative
    // this may be symptomatic of some other evil, but is minor enough to ignore for now.
    hAngle = 2.f*atanf((-b-rage::Sqrtf(rage::Clamp(a*a+b*b-c*c, 0.f, 10.f)))/(-a-c));


    /*
     *  update internal model by rotating hinge to effector
     */
    rage::Quaternion q;
    he = e - h;
    q.FromRotation(hingeTM1.c, hAngle);
    q.Transform(he);
    e = h + he;

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(a, b, rage::Vector3(0.9f,0.5f,0.f));
    }
#endif

    /*
     *  get root rotation to align root to effector with 
     *  root to target.
     */
    rage::Vector3 re = e; re.Normalize();
    rage::Vector3 rt = t; rt.Normalize();
    rage::Vector3 rAxis; rAxis.Cross(rt, re);
    float rAngle = rage::AcosfSafe(re.Dot(rt));
    rAxis.Normalize();
    rage::Quaternion rLeanQuat;
    rLeanQuat.FromRotation(rAxis, rAngle);
    rLeanQuat.UnTransform(h);
    rLeanQuat.UnTransform(e);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform3x3(rAxis, a);
      BSPY_DRAW_LINE(rootTM1.d, rootTM1.d+a*0.1f, rage::Vector3(1.f,1.f,0.f));
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(rootTM1.d, a, rage::Vector3(1.f,0.8f,0.f));
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.8f,0.f));
    }
#endif

    /*
     *  twist around root to effector
     *
     *  use pole vector to inform the twist, if
     *  provided.  otherwise use twist value.
     *
     *  pole vector is interpreted as desired hinge
     *  axis direction (as opposed to desired hinge
     *  position.
     *
     *  using the hinge axis is not strictly correct,
     *  since there is no guarantee that the arm
     *  "bones" will be ortho to the hinge axis,
     *  however, i can't think of a cleverer way
     *  to do this.
     */
    float twist = input.direction*input.twist;
    if(input.poleDirection)
    {

      rage::Vector3 re = e; re.Normalize();

      rage::Vector3 hingeAxis;
      rLeanQuat.UnTransform(hingeTM1.c, hingeAxis);
      hingeAxis.SubtractScaled(re, hingeAxis.Dot(re));
      hingeAxis.Normalize();

      rage::Vector3 pole = *input.poleDirection; pole.Normalize();
      pole.SubtractScaled(re, pole.Dot(re));
      pole.Normalize();

      twist = input.hingeDirection*rage::AcosfSafe(hingeAxis.Dot(pole));

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
      {
        rage::Vector3 a, b;
        rootTM1.Transform(h, a);
        rootTM1.Transform(h+hingeAxis*0.15f, b);
        BSPY_DRAW_LINE(a, b, rage::Vector3(0.f,1.f,0.5f));
        rootTM1.Transform(h+pole*0.1f, b);
        BSPY_DRAW_LINE(a, b, rage::Vector3(0.5f,1.f,0.f));
      }
#endif

    }

    re = e; re.Normalize();
    rage::Quaternion rTwistQuat, rQuat;
    rTwistQuat.FromRotation(re, twist);

    /*
     *  update internal model by rotating hinge and 
     *  effector.
     */
    rTwistQuat.UnTransform(h);
    rTwistQuat.UnTransform(e);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(rootTM1.d, a, rage::Vector3(1.f,1.f,0.f));
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,1.f,0.f));
    }
#endif

    /*
     *  combine root rotations
     */
    rQuat.Multiply(rLeanQuat, rTwistQuat);

    /*
     *  set hinge angle.
     */
    output.oneDof.m_desiredAngle = hAngle;

    /*
     *  set root angles.
     */
    rQuat.Inverse(); // to accommodate new joint function
    rage::Vector3 ts = rsQuatToRageDriveTwistSwing(rQuat);
    output.threeDof.m_desiredTwist = (input.threeDof->m_reverseTwist ? -1.f : 1.f) * ts.x + input.threeDof->m_midTwist;
    output.threeDof.m_desiredLean1 = (input.threeDof->m_reverseLean1 ? -1.f : 1.f) * ts.y + input.threeDof->m_midLean1;
    output.threeDof.m_desiredLean2 = (input.threeDof->m_reverseLean2 ? -1.f : 1.f) * ts.z + input.threeDof->m_midLean2;

    /*
     *  blend based on advancedIK output.
     */
    if (input.option.advancedIK)
    {
      output.threeDof.blendDesiredWithActual(advBlend);
      output.oneDof.blendDesiredWithActual(advBlend);
    }

#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("limbIK2")
#endif
  }

  /*
   *  solve 2-bone IK task
   *
   *  reworked to use incoming transforms as a base pose
   *
   */
  void limbIK3(NmRsLimbIKInput& input, NmRsLimbIKOutput& output, NmRsLimbIKBasePose& basePose)
  {
    /*
     *  get the output target from getStabilisedPos() if
     *  a dragReduction was specified; otherwise just copy
     */
    if (input.dragReduction > 0.0f)
      output.target.Set(getStabilisedPos(input));
    else
      output.target.Set(*input.target);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_POINT(output.target, 0.1f, rage::Vector3(1.f,1.f,0.f));
#endif

    /*
     *  adjust target to limit speed. compute blend value
     */
    float advBlend = 0.f;
    if (input.option.advancedIK)
      advBlend = advancedIK(input, output);

    /*
     *  operates in zeroed root local frame unless
     *  otherwise indicated.
     */
    rage::Matrix34 rootTM1(input.threeDof->m_matrix1);
    rage::Matrix34 rootTM2Local;
    rootTM2Local.DotTranspose((*basePose.threeDofMatrix2), (*basePose.threeDofMatrix1));  // get itm moving frame in space of *itm* fixed frame
    rage::Matrix34 rootTM2; rootTM2.Dot(rootTM2Local, rootTM1);                           // map moving frame back to world via *actual* fixed frame
    rage::Matrix34 hingeTM1; hingeTM1.DotTranspose(*basePose.oneDofMatrix1, (*basePose.threeDofMatrix2));
    rage::Matrix34 hingeTM2; hingeTM2.DotTranspose(*basePose.oneDofMatrix2, (*basePose.threeDofMatrix2));
    rage::Vector3 h(hingeTM1.d);
    rage::Vector3 e; rootTM2.UnTransform(basePose.endPartMatrix->d, e);

    hingeTM2.UnTransform(e);
    hingeTM1.Transform(e);
    rage::Vector3 t; rootTM1.UnTransform(*input.target, t);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      // draw target and starting limb configuration
      rage::Vector3 a, b;
      a = rootTM1.d;
      rootTM1.Transform(t, b);
      BSPY_DRAW_POINT(b, 0.05f, rage::Vector3(1.f,1.f,0.f));
      rootTM1.Transform(h, b);
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.f,0.f));
      rootTM1.Transform(e, a);
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.f,0.f));

      // check joint frames look ok in world space
      BSPY_DRAW_COORDINATEFRAME(rootTM1, 0.01f);
      rage::Matrix34 c;
      c.Dot(hingeTM1, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(c, 0.01f);
      c.Dot(hingeTM2, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(c, 0.01f);
      c.Dot(rootTM2, rootTM1);
      BSPY_DRAW_COORDINATEFRAME(input.threeDof->m_matrix2, 0.01f);

      // draw the actual limb configuration??
      BSPY_DRAW_LINE(rootTM1.d, input.oneDof->m_position, rage::Vector3(0.f,0.f,1.f));
      BSPY_DRAW_LINE(input.oneDof->m_position, input.endPart->m_tm.d, rage::Vector3(0.f,0.f,1.f));

#if 0 && NM_VERBOSE_IK_DEBUG
      // check ik3 specific changes
      BSPY_DRAW_COORDINATEFRAME(rootTM1, 0.1f)
        rootTM2.d.Add(rage::Vector3(0.01f,0,0));
      BSPY_DRAW_COORDINATEFRAME(rootTM2, 0.075f)
        rage::Matrix34 rootTM2ITM = (*basePose.threeDofMatrix2);
      BSPY_DRAW_COORDINATEFRAME(rootTM2ITM, 0.025f)
#endif
    }
#endif

    /*
     *  bend hinge to make root to effector and root
     *  to target distances equal.  bones are not 
     *  guaranteed to be ortho to hinge axis (z), so
     *  flat them against z before measuring.
     */
    rage::Vector3 rh = h;
    rage::Vector3 he = e-h;
    float rtMag = t.Mag();
    float reMag = e.Mag();
    float hAngle = 0;

    /*
     *  arms should never be quite straight or elbow math
     *  gets upset
     */
    if(rtMag > reMag)
      rtMag = reMag - 0.001f;

    /*
     *  math to get hinge angle from desired root effector
     *  distance and zero root and effector position in
     *  hinge space.  courtesy of AR (as in: ask him if you
     *  need an explanation of how it works).
     *
     *  assumes hinge axis is hinge tm z
     */
    rage::Vector3 pe, pr;
    hingeTM1.UnTransform3x3((e-h), pe);
    hingeTM1.UnTransform3x3(-h, pr);
    float peMag = pe.Mag();
    float prMag = pr.Mag();
    float a = -2.f*pe.x*pr.x-2.f*pe.y*pr.y;
    float b = -2.f*pe.x*pr.y+2.f*pe.y*pr.x;
    float c = rtMag*rtMag-peMag*peMag-prMag*prMag+2.f*pe.z*pr.z;
    // [jrp] had to clamp the input to sqrtf as it occasionally shows up as a very small negative
    // this may be symptomatic of some other evil, but is minor enough to ignore for now.
    hAngle = 2.f*atanf((-b-rage::Sqrtf(rage::Clamp(a*a+b*b-c*c, 0.f, 10.f)))/(-a-c));


    /*
     *  update internal model by rotating hinge to effector
     */
    rage::Quaternion q;
    he = e - h;
    q.FromRotation(hingeTM1.c, hAngle);
    q.Transform(he);
    e = h + he;

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(a, b, rage::Vector3(0.9f,0.5f,0.f));
    }
#endif

    /*
     *  get root rotation to align root to effector with 
     *  root to target.
     */
    rage::Vector3 re = e; re.Normalize();
    rage::Vector3 rt = t; rt.Normalize();
    rage::Vector3 rAxis; rAxis.Cross(rt, re);
    float rAngle = rage::AcosfSafe(re.Dot(rt));
    rAxis.Normalize();
    rage::Quaternion rLeanQuat;
    rLeanQuat.FromRotation(rAxis, rAngle);
    rLeanQuat.UnTransform(h);
    rLeanQuat.UnTransform(e);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform3x3(rAxis, a);
      BSPY_DRAW_LINE(rootTM1.d, rootTM1.d+a*0.1f, rage::Vector3(1.f,1.f,0.f));
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(rootTM1.d, a, rage::Vector3(1.f,0.8f,0.f));
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,0.8f,0.f));
    }
#endif

    /*
     *  twist around root to effector
     *
     *  use pole vector to inform the twist, if
     *  provided.  otherwise use twist value.
     *
     *  pole vector is interpreted as desired hinge
     *  axis direction (as opposed to desired hinge
     *  position.
     *
     *  using the hinge axis is not strictly correct,
     *  since there is no guarantee that the arm
     *  "bones" will be ortho to the hinge axis,
     *  however, i can't think of a cleverer way
     *  to do this.
     */
    float twist = input.direction*input.twist;
    if(input.poleDirection)
    {

      rage::Vector3 re = e; re.Normalize();

      rage::Vector3 hingeAxis;
      rLeanQuat.UnTransform(hingeTM1.c, hingeAxis);
      hingeAxis.SubtractScaled(re, hingeAxis.Dot(re));
      hingeAxis.Normalize();

      rage::Vector3 pole = *input.poleDirection; pole.Normalize();
      pole.SubtractScaled(re, pole.Dot(re));
      pole.Normalize();

      twist = input.hingeDirection*rage::AcosfSafe(hingeAxis.Dot(pole));

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
      {
        rage::Vector3 a, b;
        rootTM1.Transform(h, a);
        rootTM1.Transform(h+hingeAxis*0.15f, b);
        BSPY_DRAW_LINE(a, b, rage::Vector3(0.f,1.f,0.5f));
        rootTM1.Transform(h+pole*0.1f, b);
        BSPY_DRAW_LINE(a, b, rage::Vector3(0.5f,1.f,0.f));
      }
#endif

    }

    re = e; re.Normalize();
    rage::Quaternion rTwistQuat, rQuat;
    rTwistQuat.FromRotation(re, twist);

    /*
     *  update internal model by rotating hinge and 
     *  effector.
     */
    rTwistQuat.UnTransform(h);
    rTwistQuat.UnTransform(e);

#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    {
      rage::Vector3 a, b;
      rootTM1.Transform(h, a);
      rootTM1.Transform(e, b);
      BSPY_DRAW_LINE(rootTM1.d, a, rage::Vector3(1.f,1.f,0.f));
      BSPY_DRAW_LINE(a, b, rage::Vector3(1.f,1.f,0.f));
    }
#endif

    /*
     *  combine root rotations
     */
    rQuat.Multiply(rLeanQuat, rTwistQuat);

    /*
     *  set hinge angle.
     */
    output.oneDof.m_desiredAngle = hAngle;

    /*
     *  set root angles.
     */
    rQuat.Inverse(); // to accommodate new joint function
    rage::Vector3 ts = rsQuatToRageDriveTwistSwing(rQuat);
    output.threeDof.m_desiredTwist = (input.threeDof->m_reverseTwist ? -1.f : 1.f) * ts.x + input.threeDof->m_midTwist;
    output.threeDof.m_desiredLean1 = (input.threeDof->m_reverseLean1 ? -1.f : 1.f) * ts.y + input.threeDof->m_midLean1;
    output.threeDof.m_desiredLean2 = (input.threeDof->m_reverseLean2 ? -1.f : 1.f) * ts.z + input.threeDof->m_midLean2;

    /*
     *  blend based on advancedIK output.
     */
    if (input.option.advancedIK)
    {
      output.threeDof.blendDesiredWithActual(advBlend);
      output.oneDof.blendDesiredWithActual(advBlend);
    }

  }
#endif


  /**
   * solve 2-bone IK task
   */
  void limbIK(NmRsLimbIKInput& input, NmRsLimbIKOutput& output)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyScopedProfilePoint("limbIK")
#endif

    Assert(input.target);
    Assert(input.target->x == input.target->x);
    if (input.option.useTargetVelocity)
    {
      Assert(input.targetVelocity->x == input.targetVelocity->x);
    }

#if !__SPU & ART_ENABLE_BSPY & 0
    // debug dump ik input for regression testing
    bspyLogf(info, L"------------------- begin ik input debug -------------------");

    Assert(input.target);
    bspyLogf(info, L"target = [%f, %f, %f]", input.target->x, input.target->y, input.target->z);

    Assert(input.elbowMat);
    bspyLogf(info, L"elbowMat = [%f, %f, %f]", input.elbowMat->a.x, input.elbowMat->a.y, input.elbowMat->a.z);
    bspyLogf(info, L"         = [%f, %f, %f]", input.elbowMat->b.x, input.elbowMat->b.y, input.elbowMat->b.z);
    bspyLogf(info, L"         = [%f, %f, %f]", input.elbowMat->c.x, input.elbowMat->c.y, input.elbowMat->c.z);
    bspyLogf(info, L"         = [%f, %f, %f]", input.elbowMat->d.x, input.elbowMat->d.y, input.elbowMat->d.z);

    if(input.targetVelocity)
    {
      bspyLogf(info, L"targetVelocity = [%f, %f, %f]", input.targetVelocity->x, input.targetVelocity->y, input.targetVelocity->z);
    }
    if(input.poleDirection)
    {
      bspyLogf(info, L"poleDirection = [%f, %f, %f]", input.poleDirection->x, input.poleDirection->y, input.poleDirection->z);
    }
    if(input.effectorOffset)
    {
      bspyLogf(info, L"effectorOffset = [%f, %f, %f]", input.effectorOffset->x, input.effectorOffset->y, input.effectorOffset->z);
    }
    bspyLogf(info, L"dragReduction = %f", input.dragReduction);
    bspyLogf(info, L"twist = %f", input.twist);
    bspyLogf(info, L"direction = %f", input.direction);
    bspyLogf(info, L"hingeDirection = %f", input.hingeDirection);
    bspyLogf(info, L"blend = %f", input.blend);
    bspyLogf(info, L"maxSpeed = %f", input.maxSpeed);
    bspyLogf(info, L"straightness = %f", input.straightness);
    bspyLogf(info, L"maxReachDistance = %f", input.maxReachDistance);

    bspyLogf(info, L"useTargetVelocity = %u", input.option.useTargetVelocity);
    bspyLogf(info, L"twistIsFixed = %u", input.option.twistIsFixed);
    bspyLogf(info, L"advancedIK = %u", input.option.advancedIK);

    bspyLogf(info, L"-------------------- end ik input debug --------------------");
#endif

#if !__SPU & ART_ENABLE_BSPY && 0
    rage::Vector3 der;
    der.Set(input.target->x,input.target->y,input.target->z);
    BSPY_DRAW_POINT(der, 0.1f, rage::Vector3(1.f,1.f,1.f));
#endif
    rage::Vector3 endPartPos, endPartLocal, e0, e1;
    rage::Vector3 toHand, cross;
    rage::Quaternion quat;
    float upperLength, lowerLength, sinAngle, extraAngle, targetLength, advBlend = 0.0f;

    // get the output target from getStabilisedPos() if
    // a dragReduction was specified; otherwise just copy
    if (input.dragReduction > 0.0f)
      output.target.Set(getStabilisedPos(input));
    else
      output.target.Set(*input.target);

#if NM_USE_IK_SELF_AVOIDANCE
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("selfAvoidance")
#endif
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // BBDD Self avoidance tech.
    if (input.selfAvoidance.useSelfAvoidance)
    {
      // Check whether self avoidance tech should use original IK input target or the target that has already
      // been modified by getStabilisedPos() tech i.e. function that compensates for rotational and linear velocity
      // of shoulder / thigh.
      if (input.selfAvoidance.overwriteDragReduction)
      {
        if (input.selfAvoidance.usePolarPathAlgorithm)
        {
          output.target.Set(getSelfAvoidPolarPos(input, *(input.target)));
        }
        else
        {
          output.target.Set(getSelfAvoidPos(input, *(input.target)));
        }
      }
      else // Use the target that has already been modified by getStabilisedPos().
        // Note that at this point output.target is valid regardless dragReduction was specified or not.
      {
        if (input.selfAvoidance.usePolarPathAlgorithm)
        {
          output.target.Set(getSelfAvoidPolarPos(input, output.target));
        }
        else
        {
          output.target.Set(getSelfAvoidPos(input, output.target));
        }
      }
    }
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("selfAvoidance")
#endif
#endif //NM_USE_IK_SELF_AVOIDANCE

    // apply advanced IK modifications?
    if (input.option.advancedIK)
    {
#if !__SPU & ART_ENABLE_BSPY
      bspyProfileStart("advanced")
#endif
      float maxError = input.maxSpeed * input.threeDof->m_muscleDamping / rage::Max(input.threeDof->m_muscleStrength, 1e-10f);
      float minDist = 0.1f; // 10cm

      rage::Vector3 newTarget(output.target - input.endPart->m_tm.d);

      float length = rage::Min(newTarget.Mag(), maxError) + 0.0001f;
      newTarget.Normalize();
      newTarget *= length;

      float maxStraightness = rage::Max(0.f, 1.f - minDist / rage::Max(length, 1e-10f));  
      float straightness = input.straightness > maxStraightness ? (maxStraightness+input.straightness) * 0.5f : input.straightness;
      //        float straightness = rage::Clamp(1.0f - minDist / length, 0.0f, input.straightness);
      newTarget *= 1.0f - straightness;
      newTarget.Add(input.endPart->m_tm.d);

      output.target = newTarget;
        input.dragReduction *= (1.0f - straightness);//mmmm why?  dragReduction is never used again in this routine
      advBlend = 1.0f / (1.0001f - straightness);
#if !__SPU & ART_ENABLE_BSPY
      bspyProfileEnd("advanced")
#endif
    }

#if !__SPU & ART_ENABLE_BSPY && 0
    rage::Vector3 der;
    der.Set(output.target->x,output.target->y,output.target->z);
    BSPY_DRAW_POINT(der, 0.3f, rage::Vector3(1.f,1.f,0.f));
#endif
    if (input.maxReachDistance > 0.f)
    {
      //clamp the shoulder to target distance
      rage::Vector3 shoulderToTarget = output.target - input.threeDof->m_position;
      float reachDistance = shoulderToTarget.Mag();
      reachDistance = rage::Clamp(reachDistance, 0.f, input.maxReachDistance);
      shoulderToTarget.Normalize();
      output.target = input.threeDof->m_position + reachDistance*shoulderToTarget;
    }
#if !__SPU & ART_ENABLE_BSPY && 0
    der.Set(output.target.x,output.target.y,output.target.z);
    BSPY_DRAW_POINT(der, 0.1f, rage::Vector3(0.5f,1.f,1.f));
#endif

    input.oneDof->m_matrix2.UnTransform(input.endPart->m_tm.d, toHand);

    input.elbowMat->Transform3x3(toHand);

    upperLength = input.elbowMat->d.Mag();
    Assert(upperLength >= 0.0f);
    lowerLength = toHand.Mag();
    Assert(lowerLength >= 0.0f);

    cross.Cross(toHand, input.elbowMat->d);

    sinAngle = input.elbowMat->c.Dot(cross) / (upperLength * lowerLength);

    // TDL sinAngle should never go near 1 or -1... infact could remove the asin for efficiency
    extraAngle = rage::AsinfSafe(sinAngle);

    // transform target with inverse of 3dof matrix
    input.threeDof->m_matrix1.UnTransform(output.target, endPartLocal);

    targetLength = endPartLocal.Mag();

    // Return on bad values
    if (targetLength <= 0.0f || endPartLocal.x != endPartLocal.x || endPartLocal.x*0.0f != 0.0f)
    {
      Assert(false);
#if !__SPU & ART_ENABLE_BSPY
      bspyLogf(info, L"limb IK bad vals: targetLength=%f, endPartX=%f", targetLength, endPartLocal.x);
#endif
      return;
    }

    if (targetLength > 0.0f && targetLength > upperLength + lowerLength)
      endPartLocal *= ((upperLength + lowerLength) / targetLength);

    float cosAngle, oneDofAngle;
    cosAngle = ( rage::square(lowerLength) + rage::square(upperLength) - rage::square(targetLength) ) / (2.0f * upperLength * lowerLength);
    oneDofAngle = extraAngle + input.hingeDirection*((PI - rage::AcosfSafe(cosAngle)));

    rage::Vector3 elbowAxis = input.elbowMat->c;
    Assert(elbowAxis.x == elbowAxis.x && elbowAxis.x * 0.0f == 0.0f);
    elbowAxis *= (oneDofAngle);
    Assert(oneDofAngle != 0.0f);
    quat.FromRotation(elbowAxis);
    quat.Transform(toHand);
    Assert(toHand.x == toHand.x && toHand.x * 0.0f == 0.0f);
    Assert(input.elbowMat->d.x == input.elbowMat->d.x && input.elbowMat->d.x * 0.0f == 0.0f);
    e0 = toHand + input.elbowMat->d;

    rage::Quaternion addedRot;
    addedRot.Identity();
    e1 = e0;
    rage::Vector3 twistVec = e0;
    twistVec.Normalize();
    twistVec *= input.direction*input.twist;
    addedRot.FromRotation(twistVec);

    float lean1, lean2, tw;

    if (input.poleDirection)
    {
      rage::Vector3 poleDir;
      poleDir.Set(*input.poleDirection);
      getFreeLean2(poleDir, endPartLocal, e1, lean1, lean2, tw, input);
    }
    else
    {
      getFreeLean(addedRot, endPartLocal, e1, lean1, lean2, tw);
    } 
    if (rage::Abs(tw) > input.threeDof->m_twistExtent) // deal with hitting a twist limit
    {
      float oldTwist = tw;
      float twistDif = tw - rage::Clamp(tw, -input.threeDof->m_twistExtent, input.threeDof->m_twistExtent);
      rage::Vector3 twistVec = e0;
      twistVec.Normalize();
      twistVec *= input.direction*input.twist - twistDif;
      addedRot.FromRotation(twistVec);
      getFreeLean(addedRot, endPartLocal, e1, lean1, lean2, tw);
      tw = oldTwist;//mmmmherebalancer But it is still over the twist limit then? 
    }
    //}
    float mag = rage::square(lean1) / rage::square(input.threeDof->m_lean1Extent) + rage::square(lean2) / rage::square(input.threeDof->m_lean2Extent);

    // is outside 3dof limits?
    if (mag > 1.0f && (!input.option.twistIsFixed))
    {
      float scale = 1.0f / rage::Sqrtf(mag);
      float mag2 = rage::Sqrtf(rage::square(lean1) + rage::square(lean2));
      oneDofAngle += rage::Clamp(input.hingeDirection * mag2 * (1 - scale), -0.5f, 0.5f);
    }

    // set the hinge/1dof/elbow/knee
    float invBlend = (1.0f - input.blend);
    output.oneDof.m_desiredAngle = (input.oneDof->m_desiredAngle * invBlend + oneDofAngle * input.blend);

    // set the 3dof/shoulder/hip
    output.threeDof.m_desiredLean1 = (input.threeDof->m_desiredLean1 * invBlend + ((input.threeDof->m_midLean1 + lean1) * input.blend));
    output.threeDof.m_desiredLean2 = (input.threeDof->m_desiredLean2 * invBlend + ((input.threeDof->m_midLean2 + input.direction * lean2) * input.blend));

    output.threeDof.m_desiredTwist = (input.threeDof->m_desiredTwist * invBlend + ((input.threeDof->m_midTwist + input.direction * tw) * input.blend));

    // final advanced IK processing
    if (input.option.advancedIK)
    {
      output.threeDof.blendDesiredWithActual(advBlend);
      output.oneDof.blendDesiredWithActual(advBlend);
    }

#if !__SPU & ART_ENABLE_BSPY & 0
    // debug dump ik output for regression testing
    bspyLogf(info, L"------------------- begin ik output debug -------------------");

    Assert(input.target);
    bspyLogf(info, L"threeDof = [%f, %f, %f]", output.threeDof.m_desiredTwist, output.threeDof.m_desiredLean1, output.threeDof.m_desiredLean2);
    bspyLogf(info, L"oneDof   = %f", output.oneDof.m_desiredAngle);

    bspyLogf(info, L"-------------------- end ik output debug --------------------");
#endif
  }

  /**
   * port of getElbowPos
   */
  void getLimbHingePos(
    rage::Vector3 &elbowWorld, 
    Shadow3Dof& threeDof, 
    Shadow1Dof& oneDof, 
    float direction)
  {
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileStart("getLimbHingePos")
#endif
    rage::Vector3 midPtRs;
    rage::Vector3 midPt, ts;
    rage::Quaternion q;     

    float upperLength = threeDof.m_matrix1.d.Dist(oneDof.m_matrix1.d);

    midPt.Set(0, 0, upperLength);
    ts.Set(
      (threeDof.m_desiredTwist - threeDof.m_midTwist) * direction, 
      threeDof.m_desiredLean1 - threeDof.m_midLean1, 
      (threeDof.m_desiredLean2 - threeDof.m_midLean2) * direction);

    q = rsRageDriveTwistSwingToQuat(ts);
    q.Transform(midPt);

    midPtRs.Set(midPt.x, midPt.y, midPt.z);
    threeDof.m_matrix1.Transform(midPtRs, elbowWorld);
#if !__SPU & ART_ENABLE_BSPY
    bspyProfileEnd("getLimbHingePos")
#endif
  }

  float getAnkleAngle(
    rage::Vector3 &kneePos, 
    rage::Vector3 &target, 
    Shadow1Dof& oneDof, 
    const rage::Vector3 &upVector)
  {
    rage::Vector3 sideways, toKnee, kneeCross;

    sideways = oneDof.m_matrix1.c;

    toKnee.Subtract(kneePos, target);
    toKnee.Normalize();
    kneeCross.Cross(toKnee, upVector);
#if !__SPU & ART_ENABLE_BSPY && NM_VERBOSE_IK_DEBUG
    BSPY_DRAW_LINE(kneePos,kneePos+sideways, rage::Vector3(0.f,1.f,0.f));
    BSPY_DRAW_LINE(kneePos,kneePos+toKnee, rage::Vector3(1.f,0.f,0.f));
    BSPY_DRAW_LINE(kneePos,kneePos+kneeCross, rage::Vector3(1.f,1.f,1.f));
#endif

    float sinAngle = kneeCross.Dot(sideways);
    return rage::AsinfSafe(sinAngle);
  }

  float wristIK(
    rage::Vector3 &target,
    rage::Vector3 &normal,
    rage::Vector3 &elbowPos,
    Shadow3Dof& shoulder, 
    Shadow3Dof& wrist,
    float direction, 
    bool useActualAngles, 
    float twistLimit)
  {
    rage::Vector3 shoulderVec, wristVec;
    rage::Vector3 xAxis, yAxis, zAxis, xyVec;
    float dotX, dotY, dotZ, dotXY, invDist, leanAngle;

    xAxis = wrist.m_matrix1.a;//ulna2radius (direction of palm centre to thumb [but not on the hand])
    zAxis = wrist.m_matrix1.c;//elbow2wrist

    shoulderVec = shoulder.m_matrix1.d; 

    shoulderVec -= elbowPos;
    wristVec = target - elbowPos;

    yAxis.Cross(wristVec, shoulderVec);//ikelbow2target x ikelbow2shoulder
    float length = yAxis.Mag();// = |ikelbow2target| . |ikelbow2shoulder| sin(q) n~
    yAxis.Normalize();
    if (length > 0.05f  && !useActualAngles)
    {
      zAxis.Set(wristVec);
      zAxis.Normalize();
      xAxis.Cross(yAxis, zAxis);
      //x = y x z                             = ulna2radius of virtualIK forearm
      //y = ikelbow2target x ikelbow2shoulder = out of virtualIK forearm top
      //z = ikelbow2target                    = ikelbow 2 virtualIKWrist
    }
    else
      yAxis.Cross(zAxis, xAxis);
    //x = ulna2radius (direction of palm centre to thumb [but not on the hand])
    //y = out of forearm top (out of back of hand [but not on the hand]
    //z = elbow2wrist

    yAxis *= direction;

    dotX = xAxis.Dot(normal);
    dotY = -yAxis.Dot(normal);
    dotZ = zAxis.Dot(normal);

    float dist = rage::Sqrtf(dotX * dotX + dotY * dotY) + NM_RS_FLOATEPS;
    invDist = 1.0f / dist;

    dotX *= invDist;
    dotY *= invDist;
    float limit = twistLimit * dist; // reduce limit down with parallel-ness of arm with normal

    float twist = rage::Atan2f(dotX, dotY);
    twist = rage::Clamp(twist , -limit, limit);
    wrist.m_desiredTwist = twist; // may need to add on midTwist here for if there's asymmetric limits

    rage::cos_and_sin(dotY, dotX, twist);

    xyVec.Zero();
    xyVec.AddScaled(xAxis, dotX);
    xyVec.AddScaled(yAxis, -dotY);

    dotXY = xyVec.Dot(normal);
    leanAngle = rage::Atan2f(dotZ, dotXY);

    wrist.m_desiredLean1 = (-leanAngle * dotX);
    wrist.m_desiredLean2 = (leanAngle * dotY);

    return -normal.Dot(yAxis);
  }

  void matchFootToGround(
    ShadowGPart& shin, 
    Shadow3Dof& ankle, 
    const rage::Vector3 &up,
    bool m_stagger)
  {
    float height = shin.m_tm.c.Dot(up);
    float angVel = shin.m_angVel.Dot(shin.m_tm.a) * (ankle.m_muscleDamping / rage::Max(ankle.m_muscleStrength, 1e-10f));

    ankle.m_desiredLean1 = (height - angVel);
    if (m_stagger)
      ankle.m_desiredLean1 = (height + angVel);//+0.4f
  }

  /*
  *  broken out from limbIK function. modifies target to
  *  limit speed. returns blend value.
  */
  float advancedIK(NmRsLimbIKInput& input, NmRsLimbIKOutput& output)
  {
    float maxError = input.maxSpeed * input.threeDof->m_muscleDamping / rage::Max(input.threeDof->m_muscleStrength, 1e-10f);
    float minDist = 0.1f; // 10cm

    rage::Vector3 newTarget(output.target - input.endPart->m_tm.d);

    float length = rage::Min(newTarget.Mag(), maxError) + 0.0001f;
    newTarget.Normalize();
    newTarget *= length;

    float maxStraightness = rage::Max(0.f, 1.f - minDist / rage::Max(length, 1e-10f));  
    float straightness = input.straightness > maxStraightness ? (maxStraightness+input.straightness) * 0.5f : input.straightness;
    newTarget *= 1.0f - straightness;
    newTarget.Add(input.endPart->m_tm.d);

    output.target = newTarget;
    input.dragReduction *= (1.0f - straightness);
    return 1.0f / (1.0001f - straightness);
  }

} // namespace ART

