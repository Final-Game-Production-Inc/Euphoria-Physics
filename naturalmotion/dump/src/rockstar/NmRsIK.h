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

#ifndef NM_RS_IK_H
#define NM_RS_IK_H

#include "NmRsUtils.h"
#include "NmRsShadows.h"

namespace ART
{
     /**
     * input & output packets for ::limbIK()
     */
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsLimbIKInput
    {
      NmRsLimbIKInput() : 
        target(0), elbowMat(0), targetVelocity(0), poleDirection(0), effectorOffset(0),
          oneDof(0), threeDof(0), endPart(0), rootPart(0)
      {
        option.useTargetVelocity = false;
        option.twistIsFixed = false;
        option.advancedIK = false;
      }

      const rage::Vector3  * target;
      const rage::Matrix34 * elbowMat;
      const rage::Vector3  * targetVelocity; // optional, see useTargetVelocity
      const rage::Vector3  * poleDirection;
      const rage::Vector3  * effectorOffset; // effector offset, end part local

      const Shadow1Dof  * oneDof;

      const Shadow3Dof  * threeDof;
      const ShadowGPart * endPart;
      const ShadowGPart * rootPart;

      float           dragReduction;

      float           twist;
      float           direction;
      float           hingeDirection;
      float           blend;

      float           maxSpeed;
      float           straightness;
      float           maxReachDistance;

      struct 
      {
        bool useTargetVelocity:1;             // whether to use targetVelocity value in getStabilisedPos
        bool twistIsFixed:1;
        bool advancedIK:1;                    // toggled on if max speeds & straightness provided

      } option;

#if NM_USE_IK_SELF_AVOIDANCE
      //////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////
      // BBDD Self Avoidance tech.
      struct NmRsIKSelfAvoidance
      {
        bool useSelfAvoidance;
        bool usePolarPathAlgorithm;
        bool overwriteDragReduction;

        NmRsIKSelfAvoidance()
          : useSelfAvoidance (false),
            usePolarPathAlgorithm (false),
            overwriteDragReduction (false)
        {}

        struct NmRsIKCommonSelfAvoidanceParams
        {
          rage::Vector3 chestPos;
          rage::Vector3 pelvisPos;
          rage::Vector3 rightDir;

        } commonSelfAvoidanceParams;

        struct NmRsIKPolarSelfAvoidanceParams
        {
          float radius;

        } polarSelfAvoidanceParams;

        struct NmRsIKSelfAvoidanceParams
        {
          float torsoSwingFraction;
          float maxTorsoSwingAngleRad;
          float selfAvoidAmount;
          bool  selfAvoidIfInSpineBoundsOnly;
          bool  overwriteTwist;
        } selfAvoidanceParams;

      } selfAvoidance;
      //////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////
#endif // NM_USE_IK_SELF_AVOIDANCE

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    // to support ik based on current itm pose
    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsLimbIKBasePose
    {
      const rage::Matrix34 * threeDofMatrix1;
      const rage::Matrix34 * threeDofMatrix2;
      const rage::Matrix34 * oneDofMatrix1;
      const rage::Matrix34 * oneDofMatrix2;
      const rage::Matrix34 * endPartMatrix;
    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()

    MSVCBeginWarningMacroBlock(disable:MSVCWarning_StructurePadding)
    NM_ALIGN_PREFIX(16) struct NmRsLimbIKOutput
    {
      Shadow1Dof      oneDof;     // 176
      Shadow3Dof      threeDof;   // 144

      rage::Vector3   target;

    } NM_ALIGN_SUFFIX(16);
    MSVCEndWarningMacroBlock()


    /**
     * solve 2-bone IK task
     */
    void limbIK(NmRsLimbIKInput& input, NmRsLimbIKOutput& output);

    // the standard limbIK function now works correctly and these functions
    // are unused.
#if 0
    /**
     *  solve 2-bone IK task
     *
     *  replacement IK function to fix problems associated with
     *  RAGE joint parameterization discrepancy (see MAXP-160)
     */
    void limbIK2(NmRsLimbIKInput& input, NmRsLimbIKOutput& output);
    void limbIK3(NmRsLimbIKInput& input, NmRsLimbIKOutput& output, NmRsLimbIKBasePose& basePose);
#endif

    void getLimbHingePos(
      rage::Vector3 &elbowWorld, 
      Shadow3Dof& threeDof, 
      Shadow1Dof& oneDof, 
      float direction);

    float advancedIK(NmRsLimbIKInput& input, NmRsLimbIKOutput& output);

    float getAnkleAngle(
      rage::Vector3 &kneePos, 
      rage::Vector3 &target, 
      Shadow1Dof& oneDof, 
      const rage::Vector3 &upVector);

    void matchFootToGround(
      ShadowGPart& shin, 
      Shadow3Dof& ankle, 
      const rage::Vector3 &up,
      bool stagger);

    float wristIK(
      rage::Vector3 &target,
      rage::Vector3 &normal,
      rage::Vector3 &elbowPos,
      Shadow3Dof &shoulder, 
      Shadow3Dof &wrist,
      float direction,
      bool useActualAngles, 
      float twistLimit);

}

#endif // NM_RS_IK_H
