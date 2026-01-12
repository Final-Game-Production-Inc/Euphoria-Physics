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
#include "NmRsCBU_DBMFootPlacement.h"

#if !__SPU & ART_ENABLE_BSPY
#include "NmRsSpy.h"
#include "bspy/NmRsSpyPackets.h"
#endif // ART_ENABLE_BSPY

namespace ART
{
#define USE_INSIDE_LIMITS 0

#if USE_INSIDE_LIMITS
    /**
     * 
     */
    void vectorToSwing(
      rage::Vector3::Vector3Param target, 
      const rage::Matrix34& hipMat1, 
      float hipMidLean1,
      float hipMidLean2,
      float direction,
      float &lean1out,
      float &lean2out)
    {
      rage::Vector3 localTarget;

      hipMat1.UnTransform(target, localTarget);
      localTarget.Normalize();

	  if (localTarget.x*0.0f!=0.0f || localTarget.x!=localTarget.x)
	  {
		Assert(localTarget.x*0.0f==0.0f && localTarget.x==localTarget.x);
		localTarget.x = localTarget.y = localTarget.z = 1.0f;
	  }
      float angle = rage::AcosfSafe(localTarget.z);
      float scale = 1.0f / (rage::Sqrtf(rage::square(localTarget.x) + rage::square(localTarget.y)) + NM_RS_FLOATEPS);
      float lean1 = localTarget.x * scale * angle;
      float lean2 = localTarget.y * scale * angle;

      lean1out = (lean1 + hipMidLean1);
      lean2out = (lean2 * direction + hipMidLean2);
    }

    //This routine's output seldom influences the balancer - not seen it happen once while the balancer is standing
    //- it does sometimes affect it when on the floor in handsAndKnees catchFall or when upsidedown
    /**
     * 
     */
    float insideLimits(
      const Shadow3Dof& effector,
      float t1, float t2, 
      float s1, float s2)
    {
      float l1 = effector.m_actualLean1;
      float l2 = effector.m_actualLean2;

      float l1Ext = effector.m_lean1Extent * 1.5f;
      float l2Ext = effector.m_lean2Extent * 1.5f;
      float S1 = (s1 - effector.m_midLean1) / l1Ext;
      float S2 = (s2 - effector.m_midLean2) / l2Ext;
      float T1 = (t1 - effector.m_midLean1) / l1Ext;
      float T2 = (t2 - effector.m_midLean2) / l2Ext;

      float dir1 = T1 - S1;
      float dir2 = T2 - S2;

      S1 = ((l1 + s1) / 2.0f - effector.m_midLean1) / l1Ext;
      S2 = ((l2 + s2) / 2.0f - effector.m_midLean2) / l2Ext;

      T1 = S1 + dir1;
      T2 = S2 + dir2;

#if !__WIN32PC
      float tDotDir = T1 * dir1 + T2 * dir2;
      float sDotDir = S1 * dir1 + S2 * dir2;

      float time = 0.0f;

      if ( rage::Abs( sDotDir - tDotDir ) >= 0.001f )
        time = sDotDir / (sDotDir - tDotDir);
#else
// temporary removal on PC pending fix from Natural Motion
      float time = 0.0f;
#endif

      float p1 = S1 + dir1 * time;
      float p2 = S2 + dir2 * time;
      float dirScale = 1.0f / (rage::Sqrtf(rage::square(dir1) + rage::square(dir2)) + NM_RS_FLOATEPS);

      dir1 *= dirScale;
      dir2 *= dirScale;

      float distFromCentre = rage::Abs(p1 * dir2 - p2 * dir1);
      if (distFromCentre >= 1.0f)
      {
        p1 = l1;
        p2 = l2;
      }
      else
      {
        float extra = rage::Sqrtf(1.0f - rage::square(distFromCentre));
        p1 = p1 + extra * dir1;
        p2 = p2 + extra * dir2;

        p1 = (p1 * l1Ext) + effector.m_midLean1;
        p2 = (p2 * l2Ext) + effector.m_midLean2;
      }

      float d1 = (t1 - s1);
      float d2 = (t2 - s2);
      float dScale = 1.0f / (rage::Sqrtf(rage::square(d1) + rage::square(d2)) + NM_RS_FLOATEPS);

      d1 *= dScale;
      d2 *= dScale;

      return (p1 - l1) * d1 + (p2 - l2) * d2;
    }
#endif//#if USE_INSIDE_LIMITS

  inline void vector3ClampMag(rage::Vector3& vec, float minMag, float maxMag)
  { 
    float mag2 = vec.Mag2();
    Assert(mag2>1.0e-12f && rage::FPIsFinite(mag2));
	if (!(mag2>1.0e-12f && rage::FPIsFinite(mag2)))
		vec.Zero();

    if (mag2>rage::square(maxMag))
    {
      // The vector's magnitude is larger than maxMag, so scale it down.
      vec.Scale(maxMag * rage::invsqrtf(mag2));
    }
    else if (mag2<rage::square(minMag))
    {
      // The vector's magnitude is smaller than minMag, so scale it up.
      vec.Scale(minMag * rage::invsqrtf(mag2));
    }
  }

    /**
     * 
     */
    inline void dbmFootPlacementStepChoice(
      const NmRsCBUDynBal_ReadOnly& ro,
      const NmRsCBUDynBal_BodyPacket& body,
      NmRsCBUDynBal_FootState& foot
      )
    {
      // heuristic for which foot to begin stepping
      bool leftDistBeyondRight = (foot.m_leftDistance > foot.m_rightDistance) && !foot.state.m_isInsideSupport;
      bool rightDistBeyondLeft = (foot.m_rightDistance > foot.m_leftDistance) && !foot.state.m_isInsideSupport;
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "foot", leftDistBeyondRight);
      bspyScratchpad(bspyLastSeenAgent, "foot", rightDistBeyondLeft);
#endif // ART_ENABLE_BSPY
#if NM_NEW_BALANCER
      //const bool sideways = true;
      //if (sideways)
      {
        //This picks the correct leg for initial sideways hits but gets it wrong subsequently leading to early switching/falling over
        // We want the initial support leg to be the inside leg as this gives the best chance of staying up
        // Overall we don't want this for stepping but:
        //   1) do want it for going from not stepping to stepping.
        //   2) Especially when the weight of the character is over the leg - want this leg to be the stance
        //   3) We compromise here by just doing it on the 1st step
        //true left right distance not taking into account hip offsets
        if (foot.m_numOfSteps <= 0)
        {        
          rage::Vector3 progCOM(body.m_COM);        
          progCOM.AddScaled(body.m_lvlCOMvelRelative, ro.m_balanceTime);
          ro.levelVector(progCOM, ro.m_gUp.Dot(foot.m_centreOfFeet));
          float leftDistance = ro.horizDistance(body.m_leftFoot.m_tm.d, progCOM);
          float rightDistance = ro.horizDistance(body.m_rightFoot.m_tm.d, progCOM);
#if !__SPU & ART_ENABLE_BSPY
          bspyScratchpad(bspyLastSeenAgent, "foot", leftDistance);
          bspyScratchpad(bspyLastSeenAgent, "foot", rightDistance);
#endif // ART_ENABLE_BSPY

          leftDistBeyondRight = (leftDistance > rightDistance) && !foot.state.m_isInsideSupport;
          rightDistBeyondLeft = (rightDistance > leftDistance) && !foot.state.m_isInsideSupport;
        }

      }
#endif //NM_NEW_BALANCER    
	  bool airborne = !body.cd.m_leftFootCollided && !body.cd.m_rightFootCollided;
      if ((foot.m_footChoice == NmRsCBUDynBal_FootState::kNotStepping && (!foot.state.m_isInsideSupport || foot.m_stepIfInSupport))
				|| (airborne && ro.m_changeStepTime > 0.f && ro.m_timeTakenForStep > ro.m_changeStepTime))
      {
        bool stepLeft = false;
        if (body.cd.m_rightFootCollided)
        {
          if (foot.m_forceStep == 1 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep)< 1)
            stepLeft = true;
          else
            stepLeft = leftDistBeyondRight || !body.cd.m_leftFootCollided;
        }
        else
        {
          if (foot.m_alwaysStepWithFarthest)
            stepLeft = !body.cd.m_leftFootCollided && leftDistBeyondRight;
          else
            stepLeft = !body.cd.m_leftFootCollided && rightDistBeyondLeft;
        } 
        if (foot.m_forceStep == 1 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
          stepLeft = true;

        bool stepRight = false;
        if (body.cd.m_leftFootCollided)
        {
          if (foot.m_forceStep == 2 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
            stepRight = true;
          else
            stepRight = rightDistBeyondLeft || !body.cd.m_rightFootCollided;
        }
        else
        {
          if (foot.m_alwaysStepWithFarthest)
            stepRight = !body.cd.m_rightFootCollided && rightDistBeyondLeft;
          else
            stepRight = !body.cd.m_rightFootCollided && leftDistBeyondRight;
        }
        if (foot.m_forceStep == 2 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
          stepRight = true;

#if !__SPU & ART_ENABLE_BSPY
		bspyScratchpad(bspyLastSeenAgent, "foot", stepLeft);
    bspyScratchpad(bspyLastSeenAgent, "foot", stepRight);
    bspyScratchpad(bspyLastSeenAgent, "foot", leftDistBeyondRight);
    bspyScratchpad(bspyLastSeenAgent, "foot", rightDistBeyondLeft);
#endif // ART_ENABLE_BSPY

        if (stepLeft || stepRight)
        {
            if (foot.m_forceStep == 2 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
            {
              stepRight = true;
              stepLeft = false;
            }
            if (foot.m_forceStep == 1 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
            {
              stepLeft = true;
              stepRight = false;
            }
        }
        
				if (ro.m_changeStepTime > 0.f && ro.m_timeTakenForStep > ro.m_changeStepTime)
				{
					stepLeft = false;
					stepRight = false;
					if (foot.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
						stepRight = true;
					else if (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
						stepLeft = true;
					else if (foot.m_footChoice == NmRsCBUDynBal_FootState::kNotStepping)
					{
						stepLeft = true;
						stepRight = true;
					}         
				}
        //If unsure which leg to step with choose it randomly
        if (stepLeft && stepRight)
        {
          stepLeft = stepRight = false;
		  if (foot.m_alwaysStepWithFarthest)
		  {
			  if (foot.m_leftDistance > foot.m_rightDistance)
                stepLeft = true;
			  else
                stepRight = true;
		  }
		  else
		  {
          if (ro.m_random < 0.5f)
            stepRight = true;
          else
            stepLeft = true;
		  }
#if !__SPU & ART_ENABLE_BSPY
          bspyScratchpad(bspyLastSeenAgent, "stepUndecided", stepRight);
#endif
        }

        NM_RS_DBG_LOGF(L"leftDistBeyondRight %d", leftDistBeyondRight);
        NM_RS_DBG_LOGF(L"body.cd.m_leftFootCollided %d", body.cd.m_leftFootCollided);
        NM_RS_DBG_LOGF(L"rightDistBeyondLeft %d", rightDistBeyondLeft);
        NM_RS_DBG_LOGF(L"body.cd.m_rightFootCollided %d", body.cd.m_rightFootCollided);

#if !__SPU & ART_ENABLE_BSPY && 0
        bspyScratchpad(bspyLastSeenAgent, "foot", stepLeft);
        bspyScratchpad(bspyLastSeenAgent, "foot", stepRight);
#endif // ART_ENABLE_BSPY
        if (stepLeft)
        {
          foot.m_footChoice = NmRsCBUDynBal_FootState::kLeftStep;
          foot.state.m_leftGround = body.cd.m_rightFootCollided && (!body.cd.m_leftFootCollided && !body.cd.m_leftFootCollidedLast);//MMMM1last so that do switch foot at start
          foot.state.m_achievedGoal = foot.m_leftDistance < foot.m_rightDistance - 0.2f;//MMMMBAL -0.2f added to stop early switching
          if (foot.m_forceStep && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
            foot.state.m_leftGround = false;

          NM_RS_DBG_LOGF(L"take left step");
        }
        else if (stepRight)
        {
          foot.m_footChoice = NmRsCBUDynBal_FootState::kRightStep;
          foot.state.m_leftGround = body.cd.m_leftFootCollided && (!body.cd.m_rightFootCollided && !body.cd.m_rightFootCollidedLast);//MMMM1
          foot.state.m_achievedGoal = foot.m_rightDistance < foot.m_leftDistance - 0.2f;//MMMMBAL -0.2f added to stop early switching
          if (foot.m_forceStep && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) < 1)
            foot.state.m_leftGround = false;
          NM_RS_DBG_LOGF(L"take right step");
        }
        else
          return;

#if !__SPU & ART_ENABLE_BSPY && 0
        bspyScratchpad(bspyLastSeenAgent, "foot", foot.state.m_leftGround);
#endif // ART_ENABLE_BSPY
        // perform entry-state duties

        if (ro.m_movingFloor)
        {
          //this block just to get moving floor stepFootStart correct
          rage::Vector3 leftFootPos = body.m_leftFoot.m_tm.d;
          rage::Vector3 rightFootPos = body.m_rightFoot.m_tm.d;
          //mmmmNote LevelVectorReal improved stepping for a leaned up (although this doesn't make sense)
          ro.levelVector(leftFootPos, foot.m_groundHeightLeft);
          ro.levelVector(rightFootPos, foot.m_groundHeightRight);
          // spread out the feet positions to the amount specified in the character config
          if (!body.cd.m_leftFootCollided || !body.cd.m_rightFootCollided)
		        separateFeet(leftFootPos, rightFootPos, ro.m_legSeparation + ro.m_extraFeetApart);
          // keep feet positions to within a maximum distance apart (this exploits and limits the effect of low lateral friction on the feet - so be prepared to remove if lateral friction improves) 
          // Moves the feet closer to the predicted balance point. 
          if (body.cd.m_leftFootCollided && body.cd.m_rightFootCollided)
          {
            rage::Vector3 progCOM = body.m_COM;
            progCOM.AddScaled(body.m_lvlCOMvelRelative, ro.m_balanceTime);
            constrainFeet(leftFootPos, rightFootPos, ro.m_legSeparation + ro.m_extraFeetApart + ro.m_legsApartMax, progCOM);
          }
          foot.m_stepFootStart = (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)? foot.m_rightFootPos - leftFootPos : foot.m_leftFootPos - rightFootPos; // body.m_rightFoot.m_tm.d:body.m_leftFoot.m_tm.d;
        }
        else
          foot.m_stepFootStart = (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)? foot.m_rightFootPos : foot.m_leftFootPos; // body.m_rightFoot.m_tm.d:body.m_leftFoot.m_tm.d;
         
        foot.m_numOfSteps++;
        foot.m_numOfSteps4Max++;
        //mmmmVariance NewStep therefore change clampScale,balanceTime
        foot.m_newStep = true;
      }
    }


    /**
    * 
    */
    bool dbmFootPlacement(
      const NmRsCBUDynBal_ReadOnly& ro,
      const NmRsCBUDynBal_BodyPacket& body,
      NmRsCBUDynBal_FootState& foot
      )
    {
      // -- Foot Position Controller
      
      // uneven ground controller - take the probe results done in the manager
      // and store the height into the left/right foot positions
      foot.m_groundHeightLeft = ro.m_gUp.Dot(ro.m_leftFootProbeHitPos);
      foot.m_leftFootPos.Set(body.m_leftFoot.m_tm.d);

      foot.m_groundHeightRight = ro.m_gUp.Dot(ro.m_rightFootProbeHitPos);
      foot.m_rightFootPos.Set(body.m_rightFoot.m_tm.d);

      ro.levelVector(foot.m_leftFootPos, foot.m_groundHeightLeft);
      ro.levelVector(foot.m_rightFootPos, foot.m_groundHeightRight);

      // spread out the feet positions to the amount specified in the character config
      if (!body.cd.m_leftFootCollided || !body.cd.m_rightFootCollided)
        separateFeet(foot.m_leftFootPos, foot.m_rightFootPos, ro.m_legSeparation + ro.m_extraFeetApart);

      // keep feet positions to within a maximum distance apart (this exploits and limits the effect of low lateral friction on the feet - so be prepared to remove if lateral friction improves) 
      // Moves the feet closer to the predicted balance point. 
      if (body.cd.m_leftFootCollided && body.cd.m_rightFootCollided)
      {
        rage::Vector3 progCOM = body.m_COM;
        progCOM.AddScaled(body.m_lvlCOMvelRelative, ro.m_balanceTime);
        constrainFeet(foot.m_leftFootPos, foot.m_rightFootPos, ro.m_legSeparation + ro.m_extraFeetApart + ro.m_legsApartMax, progCOM);
      }
      // figure the centre-point of the feet
      foot.calculateCentreOfFeet();
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "footPlacement1", foot.m_centreOfFeet);
#endif // ART_ENABLE_BSPY

      // -- Foot Step Controller
      foot.m_leftDistance = 0.0f;
      foot.m_rightDistance = 0.0f;

      rage::Vector3 progCOM(body.m_COM);        
      if (ro.m_stagger)
        progCOM = body.m_buttocks.m_tm.d;
      ///NM_RS_CBU_DRAWPOINT(progCOM,1.f,rage::Vector3(0.0f,1.0f,0.0f));//mmmmmaddtobspy
      progCOM.AddScaled(body.m_lvlCOMvelRelative, ro.m_balanceTime);
      //MMMM
      //if (ro.m_stagger)
      //{
      //  rage::Vector3 progCOMAng(body.m_COMTM.c);//front
      //  float somersault = body.m_COMrotvel.Dot(body.m_COMTM.a);//side
      //  progCOMAng *= somersault/10.f;
        ////staggerFallTest maybe - add sideSomy 
      //  progCOM += progCOMAng;
      //}

      ro.levelVector(progCOM, ro.m_gUp.Dot(foot.m_centreOfFeet));

#if !__SPU & ART_ENABLE_BSPY
	  bspyScratchpad(bspyLastSeenAgent, "foot", progCOM);
#endif // ART_ENABLE_BSPY

      //avoid stepping into car (braceForImpact)
      //or  - mmmmnote we could possibly make this and
      //avoid stepping over a line (teeter/balancerCollisionsReaction)  
	    if(ro.m_avoidCar)
	    {
        //avoid stepping into car (braceForImpact)
		    setNumOfCorners(4);
		    setCorner(0, ro.m_carCorner4);
		    setCorner(1, ro.m_carCorner3);
		    setCorner(2, ro.m_carCorner2);
		    setCorner(3, ro.m_carCorner1);

		    rage::Vector3 pointOnPolygon;
        //Replace progCom with a point on the car if original progCom would mean stepping into the car
        //Make a line passing through COM and progCom
        //Find the 2 intersections of this line with the polygon 
        //If COM outside polygon: pointOnPolygon = closest intersection or progCom to COM.  I.e don't step through polygon even if progCom outside
        //if COM inside polygon:
        //  pointOnPolygon = progCom if progCom outside polygon
        //  pointOnPolygon = closest intersection or progCom to COM
		    getIntersectionPointOnPolygon(progCOM, body.m_COM, &pointOnPolygon);
		    progCOM = pointOnPolygon;
	    }
      else
      {
        //avoid stepping over a line (teeter/balancerCollisionsReaction)
        if (ro.m_impactOccurred)
        {
          //if progCom wrong side of obstacle put infront of obstacle by reflecting but only by offset infront
          float movement = (body.m_COMvel - body.m_floorVelocity + body.m_COMrotvel).Mag();

          float exclusionZone = ro.m_exclusionZone + rage::Min(movement*0.2f,0.5f);//0.6f;
          if ((ro.m_balancerState == ro.bal_GlancingSpin) || (ro.m_balancerState == ro.bal_DrapeGlancingSpin) )
            exclusionZone = 0.4f;
          if (ro.m_balancerState == ro.bal_Rebound)
            exclusionZone = 0.1f;
          //if (ro.m_balancerState == ro.bal_DrapeForward)
          //  exclusionZone = 0.6f;
          if (ro.m_teeter)
            exclusionZone = ro.m_exclusionZone;
          if (ro.m_normal1stContact.Dot(progCOM-ro.m_pos1stContact-ro.m_normal1stContact*exclusionZone)*ro.m_sideOfPlane > 0.f)//increase offset
          {
            progCOM -= ro.m_normal1stContact.Dot(progCOM - ro.m_pos1stContact-ro.m_normal1stContact*exclusionZone)* ro.m_normal1stContact;
          }
          NM_RS_DBG_LOGF(L"-------exclusionZone = %.4f", exclusionZone);
          
  #if !__SPU & ART_ENABLE_BSPY
          foot.m_exclusionZonePt1 = ro.m_pos1stContact;
          ro.levelVectorReal(foot.m_exclusionZonePt1,body.m_rightFoot.m_tm.d.z);
          foot.m_exclusionZonePt1 += ro.m_normal1stContact*exclusionZone;
          foot.m_exclusionZonePt2.Cross(ro.m_normal1stContact,ro.m_gUpReal);
          foot.m_exclusionZonePt1 += foot.m_exclusionZonePt2;
          foot.m_exclusionZonePt2 = foot.m_exclusionZonePt1 - 2.f*foot.m_exclusionZonePt2;
  #endif //ART_ENABLE_BSPY
          //NM_RS_CBU_DRAWLINE(foot.m_exclusionZonePt1,foot.m_exclusionZonePt2,rage::Vector3(1.0f,1.0f,1.0f));//mmmmmaddtobspy
          //NM_RS_CBU_DRAWPOINT(ro.m_pos1stContact+ro.m_normal1stContact*0.6f,1.f,rage::Vector3(1.0f,1.0f,1.0f));//mmmmmaddtobspy
          //NM_RS_CBU_DRAWPOINT(progCOM,3.f,rage::Vector3(0.0f,1.0f,0.0f));//mmmmmaddtobspy
        }   
	    }

      // test points inside foot support
      float insideResult = pointInsideSupportNew(
        progCOM, //tempTarget, 
        const_cast<const rage::Matrix34*>(&body.m_leftFoot.m_tm), 
        const_cast<const rage::Matrix34*>(&body.m_rightFoot.m_tm), 
        const_cast<const rage::Matrix34*>(&body.m_leftHand.m_tm), body.cd.m_leftHandCollided,
        const_cast<const rage::Matrix34*>(&body.m_rightHand.m_tm), body.cd.m_rightHandCollided, 
        0.05f + ro.m_stepDecisionThreshold, //mmmmtodo make this the actual size of the feet
        0.15f + ro.m_stepDecisionThreshold, //mmmmtodo make this the actual size of the feet
        ro.m_gUpReal, // ro.m_gUp, 
        0);


#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "foot", insideResult);
#endif

      foot.state.m_isInsideSupport = (insideResult <= 0.f);
      foot.state.m_isInsideSupportHonest = foot.state.m_isInsideSupport;
      if (foot.state.m_isInsideSupport) 
      {
        //Keep stepping if your legs end up too far apart... could do other end-state checks here. 
        float legsApart = ro.m_legsApartRestep;
        if (ro.m_balancerState == ro.bal_LeanAgainst || ro.m_balancerState == ro.bal_LeanAgainstStable)
          legsApart = 0.5f;//mmmmtodo make this rig dependent or have an absolute legs apart value for charaters in leanAgainst i.e.LA = legSeparation+legsApart 
        float distanceApart = ro.horizDistance(body.m_leftFoot.m_tm.d, body.m_rightFoot.m_tm.d);
        if (distanceApart > ro.m_legSeparation + ro.m_extraFeetApart + legsApart)
          foot.state.m_isInsideSupport = false;
        //Keep stepping if your legs end up too close(todo crossed)
        if (distanceApart < ro.m_legSeparation + ro.m_extraFeetApart - ro.m_legsTogetherRestep)
          foot.state.m_isInsideSupport = false;
#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "foot", distanceApart);
        bspyScratchpad(bspyLastSeenAgent, "foot", ro.m_legSeparation+legsApart);
#endif // ART_ENABLE_BSPY
      }
#if !__SPU & ART_ENABLE_BSPY
      bspyScratchpad(bspyLastSeenAgent, "foot", progCOM);
      bspyScratchpad(bspyLastSeenAgent, "foot", insideResult);
#endif // ART_ENABLE_BSPY

      //bodyRight = tCom.a;
      //bodyUp = tmCom.b;
      //bodyBack = tmCom.c;
      // find distances to feet
      rage::Vector3 dir = progCOM - body.m_rightFoot.m_tm.d;
      ro.levelVector(dir);
      dir.Normalize();
      float f = dir.Dot(body.m_COMTM.a);
      rage::Vector3 progCOMLeft = progCOM + body.m_COMTM.c*body.m_COMTM.c.Dot(dir)*f*-ro.m_lateralStepOffset;

      dir = progCOM - body.m_leftFoot.m_tm.d;
      ro.levelVector(dir);
      dir.Normalize();//stanceToLevelledProgCom
      f = dir.Dot(body.m_COMTM.a);//f = sidewaysness of stanceToLevelledProgCom
      //add to progCom backwards * sidewaysness of stanceToLevelledProgCom * backwardsness of stanceToLevelledProgCom * hipwidth 
      rage::Vector3 progCOMRight = progCOM + body.m_COMTM.c*body.m_COMTM.c.Dot(dir)*f*ro.m_lateralStepOffset;
      rage::Vector3 desiredPosRight = progCOMRight  +  body.m_COMTM.a * ro.m_lateralStepOffset;
      rage::Vector3 desiredPosLeft = progCOMLeft  -  body.m_COMTM.a * ro.m_lateralStepOffset;
      
      //Restrict desiredPosLeft, desiredPosRight to not step through other leg
      if (ro.m_avoidLeg)
      {
        float leftHeight = desiredPosLeft.z;
        float rightHeight = desiredPosRight.z;
        const float footLength = 0.15f;
        //rage::Vector3 normalInLeft = body.m_leftFoot.m_tm.a;
        //rage::Vector3 forwardLeft = -body.m_leftFoot.m_tm.c;
        rage::Vector3 normalInLeft = body.m_rightThigh.m_tm.d-body.m_leftThigh.m_tm.d;
        ro.levelVectorReal(normalInLeft, 0.0f);//should be level per calc for left and right
        normalInLeft.Normalize();
        rage::Vector3 forwardLeft;
        forwardLeft.Cross(-normalInLeft,ro.m_gUpReal);
        rage::Vector3 insideHeelLeft = body.m_leftFoot.m_tm.d;
        insideHeelLeft.AddScaled(normalInLeft, ro.m_avoidFootWidth);
        insideHeelLeft.AddScaled(body.m_leftFoot.m_tm.c, footLength);
        //rage::Vector3 forwardRight = -body.m_rightFoot.m_tm.c;
        //rage::Vector3 normalInRight = -body.m_rightFoot.m_tm.a;
        rage::Vector3 normalInRight = -normalInLeft;
        rage::Vector3 forwardRight = forwardLeft;
        rage::Vector3 insideHeelRight = body.m_rightFoot.m_tm.d;
        insideHeelRight.AddScaled(normalInRight, ro.m_avoidFootWidth);
        insideHeelRight.AddScaled(body.m_rightFoot.m_tm.c, footLength);

#if !__SPU & ART_ENABLE_BSPY
        //Draw the exclusion lines
        BSPY_DRAW_POINT(desiredPosLeft,0.3f,rage::Vector3(0,1,1));//teal
        BSPY_DRAW_LINE(insideHeelRight- forwardRight,insideHeelRight+forwardRight, rage::Vector3(0,1,1));//teal
        BSPY_DRAW_LINE(insideHeelRight,insideHeelRight+2.0f*normalInRight, rage::Vector3(0,1,1));//teal
        BSPY_DRAW_POINT(desiredPosRight,0.3f,rage::Vector3(1,1,0));//yellow
        BSPY_DRAW_LINE(insideHeelLeft - forwardLeft,insideHeelLeft+forwardLeft, rage::Vector3(1,1,0));//yellow
        BSPY_DRAW_LINE(insideHeelLeft,insideHeelLeft+2.0f*normalInLeft, rage::Vector3(1,1,0));//yellow
#endif
        //If the desiredPos crosses the avoid line put it onto the line
        float normalDistFromLine = normalInRight.Dot(desiredPosLeft-insideHeelRight);
        if (normalDistFromLine < 0.f)
        {
          desiredPosLeft -= normalDistFromLine*normalInRight;
          ro.levelVectorReal(desiredPosLeft, leftHeight);
        }
        normalDistFromLine = normalInLeft.Dot(desiredPosRight-insideHeelLeft);
        if (normalDistFromLine < 0.f)
        {
          desiredPosRight -= normalDistFromLine*normalInLeft;
          ro.levelVectorReal(desiredPosRight, rightHeight);
        }
        //If the actual footPos has crossed the avoid line then move the desiredPos scaled by the actual error.
        normalDistFromLine = normalInRight.Dot(insideHeelLeft-insideHeelRight);
        if (normalDistFromLine < 0.f)
        {
          desiredPosLeft -= ro.m_avoidFeedback*normalDistFromLine*normalInRight;
          ro.levelVectorReal(desiredPosLeft, leftHeight);
        }
        normalDistFromLine = normalInLeft.Dot(insideHeelRight-insideHeelLeft);
        if (normalDistFromLine < 0.f)
        {
          desiredPosRight -= ro.m_avoidFeedback*normalDistFromLine*normalInLeft;
          ro.levelVectorReal(desiredPosRight, rightHeight);
        }
#if !__SPU & ART_ENABLE_BSPY
        //Draw rectangles around the foot projections
        BSPY_DRAW_POINT(desiredPosLeft,0.3f,rage::Vector3(1,1,1));//white
        BSPY_DRAW_POINT(desiredPosRight,0.3f,rage::Vector3(1,0,1));//pink
#endif
      }


      foot.m_leftDistance = ro.horizDistance(body.m_leftFoot.m_tm.d, desiredPosLeft);
      foot.m_rightDistance = ro.horizDistance(body.m_rightFoot.m_tm.d, desiredPosRight);


      //If the step changes here but then we decide to change the step again later in this routine
      // then we'll only count that as an additional 1 step - not 2
      int numOfSteps = foot.m_numOfSteps;
      // check to see if we should be rethinking which foot to step with
      if (!foot.m_forceBalance && !foot.m_dontChangeStep)
        dbmFootPlacementStepChoice(ro, body, foot);
      if (foot.m_footChoice == NmRsCBUDynBal_FootState::kNotStepping)
      {
        foot.state.m_leftFootBalance = body.cd.m_leftFootCollided;
        foot.state.m_rightFootBalance = body.cd.m_rightFootCollided;
        return true;
      }

//#if NM_RS_ENABLE_DEBUGDRAW
//      if (rage::NMRenderBuffer::getInstance())
//      {
//        rage::Vector3 pgCol(0.0f, 0.0f, 1.0f);
//        NM_RS_CBU_DRAWPOINT( progCOM, 0.1f, pgCol);//mmmmmaddtobspy
//      }
//#endif // NM_RS_ENABLE_DEBUGDRAW

      // values assigned based on which foot is stepping
      const Shadow3Dof* activeHip = 0;
      const ShadowGPart* activeFoot = 0;
      //bool activeCollided = false;
#if USE_INSIDE_LIMITS
      float direction = 0.0f;
#endif//#if USE_INSIDE_LIMITS
      rage::Vector3* ikFootPos = 0;
      rage::Vector3 ikStanceFoot;
      rage::Vector3 desiredPos;
      bool leftGround = false;
#if NM_SIDE_STEP_FIX
      float sidewaysL = 0.f;//NM_SIDE_STEP_FIX
      float sidewaysR = 0.f;//NM_SIDE_STEP_FIX
#endif// NM_SIDE_STEP_FIX

      // check for a correct foot choice, assign pointers to the 'active' components of the
      // leg involved in the stepping procedure
      switch (foot.m_footChoice)
      {
      case NmRsCBUDynBal_FootState::kLeftStep:
        {
          activeHip = &body.m_leftHip;
          activeFoot = &body.m_leftFoot;
#if USE_INSIDE_LIMITS
          direction = 1.0f;
#endif//#if USE_INSIDE_LIMITS
          //activeCollided = body.cd.m_leftFootCollided;
          if (foot.m_forceStep == 1 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) <= 1)
            leftGround = body.cd.m_rightFootCollided && (!body.cd.m_leftFootCollided && !body.cd.m_leftFootCollidedLast);//MMMM1last so that do switch foot at start
          else
            leftGround = (!body.cd.m_leftFootCollided);
          desiredPos = desiredPosLeft;

#if NM_SIDE_STEP_FIX
          {
            rage::Vector3 hipRight = -body.m_buttocks.m_tm.b;
            rage::Vector3 hip2DesiredPos = desiredPosLeft - body.m_buttocks.m_tm.d + body.m_buttocks.m_tm.a; //-hipDown
            ro.levelVector(hipRight);
            float toDesired = hip2DesiredPos.Mag();
            ro.levelVector(hip2DesiredPos);
            hipRight.Normalize();
            hip2DesiredPos.Normalize();
            sidewaysL = hipRight.Dot(hip2DesiredPos);
#if !__SPU & ART_ENABLE_BSPY
            bspyScratchpad(bspyLastSeenAgent, "sidewaysL0", desiredPos);
#endif // ART_ENABLE_BSPY
            static float clamp1 = 0.32f;
            if (sidewaysL>0.7f)
            {
              hip2DesiredPos *= toDesired;
              rage::Vector3 hip2DesiredPosErr = hip2DesiredPos;
              hip2DesiredPos.ClampMag(0.f,clamp1);
              hip2DesiredPosErr -= hip2DesiredPos;
              desiredPos -= /*10.0f*(sidewaysL-0.8f)**/hip2DesiredPosErr;
#if !__SPU & ART_ENABLE_BSPY
              bspyScratchpad(bspyLastSeenAgent, "sidewaysL", sidewaysL);
#endif // ART_ENABLE_BSPY

            }
#if !__SPU & ART_ENABLE_BSPY
            bspyScratchpad(bspyLastSeenAgent, "sidewaysL2", sidewaysL);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysL2", hipRight);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysL2", hip2DesiredPos);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysL2", desiredPos);
#endif // ART_ENABLE_BSPY
          }
#endif//NM_SIDE_STEP_FIX
          ikFootPos = &foot.m_leftFootPos;
          ikStanceFoot.Set(foot.m_rightFootPos);
          
          if (foot.m_leftDistance < foot.m_rightDistance - 0.2f)//MMMMBAL -0.2f added to stop early switching
            foot.state.m_achievedGoal = true;
        }
        break;

      case NmRsCBUDynBal_FootState::kRightStep:
        {
          activeHip = &body.m_rightHip;
          activeFoot = &body.m_rightFoot;
#if USE_INSIDE_LIMITS
          direction = -1.0f;
#endif//#if USE_INSIDE_LIMITS
          //activeCollided = body.cd.m_rightFootCollided;
          if (foot.m_forceStep == 2 && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) <= 1)
            leftGround = body.cd.m_leftFootCollided && (!body.cd.m_rightFootCollided && !body.cd.m_rightFootCollidedLast);//MMMM1last so that do switch foot at start
          else
            leftGround = (!body.cd.m_rightFootCollided);

          desiredPos = desiredPosRight;

#if NM_SIDE_STEP_FIX
          {
            rage::Vector3 hipRight = -body.m_buttocks.m_tm.b;
            rage::Vector3 hip2DesiredPos = desiredPosRight - body.m_buttocks.m_tm.d + body.m_buttocks.m_tm.a; //-hipDown
            ro.levelVector(hipRight);
            float toDesired = hip2DesiredPos.Mag();
            ro.levelVector(hip2DesiredPos);
            hipRight.Normalize();
            hip2DesiredPos.Normalize();
            sidewaysR = -hipRight.Dot(hip2DesiredPos);
  #if !__SPU & ART_ENABLE_BSPY
            bspyScratchpad(bspyLastSeenAgent, "sidewaysR0", desiredPos);
  #endif // ART_ENABLE_BSPY
            static float clamp1 = 0.32f;
            if (sidewaysR>0.7f)
            {
              hip2DesiredPos *= toDesired;
              rage::Vector3 hip2DesiredPosErr = hip2DesiredPos;
              hip2DesiredPos.ClampMag(0.f,clamp1);
              hip2DesiredPosErr -= hip2DesiredPos;
              desiredPos -= /*10.0f*(sidewaysR-0.8f)**/hip2DesiredPosErr;
  #if !__SPU & ART_ENABLE_BSPY
               bspyScratchpad(bspyLastSeenAgent, "sidewaysR", sidewaysR);
  #endif // ART_ENABLE_BSPY

            }
  #if !__SPU & ART_ENABLE_BSPY
            bspyScratchpad(bspyLastSeenAgent, "sidewaysR2", sidewaysR);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysR2", hipRight);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysR2", hip2DesiredPos);
            bspyScratchpad(bspyLastSeenAgent, "sidewaysR2", desiredPos);
  #endif // ART_ENABLE_BSPY
          }
#endif//NM_SIDE_STEP_FIX

          

          ikFootPos = &foot.m_rightFootPos;
          ikStanceFoot.Set(foot.m_leftFootPos);
          
          if (foot.m_leftDistance > foot.m_rightDistance - 0.2f)//MMMMBAL -0.2f added to stop early switching
            foot.state.m_achievedGoal = true;

        }
        break;

      default:
        Assertf(0, "NM Foot Placement - foot.m_footChoice state unknown");
        break;
      }


      // handle the logic for a foot step cycle
      bool stepExit = false;
      {
        float stepGrad = 0.5f; // set back to 1.0f if we're hitting the ground too much
#if !NM_NEW_BALANCER
        const float halfBalanceTime = 0.15f;
#else
        float halfBalanceTime = ro.m_balanceTimeHip*0.5f;
#endif 
        bool airborne = false;
        if (foot.state.m_leftFootBalance) 
          airborne = !body.cd.m_leftFootCollided && !body.cd.m_leftFootCollidedLast; 
        else
          airborne = !body.cd.m_rightFootCollided && !body.cd.m_rightFootCollidedLast;

        if (airborne)
          stepGrad = 0.01f;
#if !__SPU & ART_ENABLE_BSPY && 0
        bspyScratchpad(bspyLastSeenAgent, "foot", leftGround);
        bspyScratchpad(bspyLastSeenAgent, "foot", foot.state.m_leftGround);
#endif // ART_ENABLE_BSPY

        //if (!activeCollided)
        //  foot.state.m_leftGround = true;
        if (leftGround)
          foot.state.m_leftGround = true;
#if !__SPU & ART_ENABLE_BSPY && 0
        bspyScratchpad(bspyLastSeenAgent, "foot", foot.state.m_leftGround);
#endif // ART_ENABLE_BSPY
        NM_RS_DBG_LOGF(L"footLeftGround: %.4f", foot.state.m_leftGround);

        foot.m_centreOfFeet.Set(body.m_COM);
        //rage::Vector3 posError = -foot.m_centreOfFeet;//mmmmTesting
        //posError.y = 0.f;
        //posError.Normalize();
        //posError *=-0.2f;
        //foot.m_centreOfFeet += posError;
        foot.m_centreOfFeet.AddScaled(body.m_lvlCOMvelRelative, halfBalanceTime);
#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "footPlacement", desiredPos);
        bspyScratchpad(bspyLastSeenAgent, "footPlacement2", foot.m_centreOfFeet);
#endif // ART_ENABLE_BSPY

        // clamp the desiredPos to within reasonable limits
        clampTarget(
          desiredPos,
          ro.m_gUp,
          body.m_buttocks.m_tm,
          activeHip->m_lean1Extent * ro.m_stepClampScale * ro.m_hipLean1Offset,
          activeHip->m_lean2Extent * ro.m_stepClampScale * ro.m_hipLean2Offset);
#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "footPlacement:Clamped", desiredPos);
#endif // ART_ENABLE_BSPY

        // work out foot height
        rage::Vector3 stepFootStart;
        if (ro.m_movingFloor)
          stepFootStart = (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)? foot.m_leftFootPos + foot.m_stepFootStart : foot.m_rightFootPos + foot.m_stepFootStart;
        else
          stepFootStart = foot.m_stepFootStart;
#if !__SPU & ART_ENABLE_BSPY
          bspyScratchpad(bspyLastSeenAgent, "footPlacement", foot.m_leftFootPos);
          bspyScratchpad(bspyLastSeenAgent, "footPlacement", foot.m_rightFootPos);
          bspyScratchpad(bspyLastSeenAgent, "footPlacement", stepFootStart);
#endif // ART_ENABLE_BSPY

        rage::Vector3 toTarget(desiredPos - stepFootStart);

        ro.levelVector(toTarget);
        toTarget.Normalize();

        float dist = desiredPos.Dot(toTarget) - activeFoot->m_tm.d.Dot(toTarget);
        float ratio_a = ro.horizDistance(activeFoot->m_tm.d, desiredPos);
        float ratio_b = (ro.horizDistance(activeFoot->m_tm.d, stepFootStart) + NM_RS_FLOATEPS);
        float dToStart = ratio_a / ratio_b;
        float difference = body.m_COMvel.Dot(toTarget) - activeFoot->m_linVel.Dot(toTarget);

        // TDL this is toe-brush-ground code, allowing step to keep going if toe brushes the ground
        // the principle is that for light touches of the toe on ground when sweeping the leg, we don't want to end the step
        // the method is to allow the step to continue if the foot is continuing to approach the target
        float footLength = 0.2f;
        rage::Vector3 toe = activeFoot->m_tm.d - activeFoot->m_tm.c*footLength*0.5f;
        float distanceToTarget = ro.horizDistance(desiredPos, toe);
        float oldDistanceToTarget = ro.horizDistance(foot.m_oldDesiredPos, foot.m_oldFootPos);
        bool targetComingToFoot = ro.horizDistance(foot.m_oldDesiredPos, toe)>ro.horizDistance(desiredPos, foot.m_oldFootPos);
        float speedToTarget = (oldDistanceToTarget-distanceToTarget)/ro.m_timeStep; // if speedToTarget is +ve we can keep stepping
        foot.m_oldDesiredPos = desiredPos;
        foot.m_oldFootPos = toe;

        difference = rage::Min(difference, 0.0f);

#if !NM_NEW_BALANCER
        float distanceToGoal = (dist + 0.1f * difference);
#else
        float distanceToGoal = (dist + 0.03f * difference);
#if NM_SIDE_STEP_FIX
        static float side = 1.8f;//mmmmSIDESTEP
        if (sidewaysL>side || sidewaysR>side)
          distanceToGoal = (dist + 0.1f * difference);
#endif//NM_SIDE_STEP_FIX
#endif
#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "footPlacement", distanceToGoal);
#endif // ART_ENABLE_BSPY

        ro.levelVector(desiredPos, ro.m_gUp.Dot(*ikFootPos));

        //rage::Vector3 dpiCol(0.2f, 0.2f, 0.2f);
        //NM_RS_CBU_DRAWPOINT( stepFootStart, 0.03f, dpiCol);//mmmmmaddtobspy

#if USE_INSIDE_LIMITS
        // 
        float t1, t2, s1, s2;
        vectorToSwing(desiredPos, activeHip->m_matrix1, activeHip->m_midLean1, activeHip->m_midLean2, direction, t1, t2);
        vectorToSwing(stepFootStart, activeHip->m_matrix1, activeHip->m_midLean1, activeHip->m_midLean2, direction, s1, s2);
#endif

        float height = stepGrad * distanceToGoal;
        
#if 0
        static bool dontVaryStepHeight = true;
        if (dontVaryStepHeight)
#endif
          height = rage::Clamp(height, -ro.m_stepHeight, ro.m_stepHeight);
#if 0
        else
        {
          float stepHeight = ro.m_stepHeight;
          //float h1 = ro.m_gUp.Dot(body.m_COM) - foot.m_groundHeightLeft;
          //float h2 = ro.m_gUp.Dot(body.m_COM) - foot.m_groundHeightRight;
          //float hipH = rage::Max(h1,h2);
          //stepHeight += (0.91f-hipH)*0.3f;
          stepHeight += ((body.m_COMvel - body.m_floorVelocity).Mag())*0.03f;
          height = rage::Clamp(height, -stepHeight, stepHeight);
        }
#endif

        float h = 0.0f;//toe2HeelDotProbeNormal(Adjusted)
        float scale = 1.0f;

        // TDL this is just to adjust the height with foot angle
        if (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
        {
          rage::Vector3 rot = body.m_rightFoot.m_angVel;
          rot.Cross(body.m_rightFoot.m_tm.c);
          rot.Scale(0.07f);
          h = (body.m_rightFoot.m_tm.c + rot).Dot(ro.m_rightFootProbeNormal);
          scale = 1.5f / (ro.m_rightFootProbeNormal.Dot(ro.m_gUp) + 0.5f); // this increases elevation slightly on slopes
        }
        else
        {
          rage::Vector3 rot = body.m_leftFoot.m_angVel;
          rot.Cross(body.m_leftFoot.m_tm.c);
          rot.Scale(0.07f);
          h = (body.m_leftFoot.m_tm.c + rot).Dot(ro.m_leftFootProbeNormal);
          scale = 1.5f / (ro.m_leftFootProbeNormal.Dot(ro.m_gUp) + 0.5f); // this increases elevation slightly on slopes
        }

#if USE_INSIDE_LIMITS
        //insideLimits' output seldom influences the balancer - not seen it happen once while the balancer is standing
        //- it does sometimes affect it when on the floor in handsAndKnees catchFall or when upsidedown
        float height_b = insideLimits(*activeHip, t1, t2, s1, s2);
        float height_a = height + 0.1f * rage::Min(rage::Abs(h), 1.0f);
        height = rage::Min(height_a, height_b);
#else
        height += 0.1f * rage::Min(rage::Abs(h), 1.0f);
#endif
        NM_RS_DBG_LOGF(L"height!: %.4f", height);

        // add a few cm if we haven't lifted off yet
        rage::Vector3 endPos = foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep ? ro.m_rightFootProbeHitPos : ro.m_leftFootProbeHitPos;
#if !NM_NEW_BALANCER
        if (!foot.state.m_leftGround)
#else
        if (!foot.state.m_leftGround && (!foot.state.m_isInsideSupportHonest)/*|| brushing*/)
#endif
        {
          height += 0.07f;
          if (foot.m_forceStep && (foot.m_numOfSteps - foot.m_numOfStepsAtForceStep) == 1)//if were forcing a step then raise it a bit more
            height += foot.m_forceStepExtraHeight;//0.07f;
          endPos += ro.m_gUp * rage::Max(0.f, (stepFootStart-endPos).Dot(ro.m_gUp)); // keep target up from current position. This line is mainly in to help him step over steps.
        }
        height *= scale;
        height += ro.m_gUp.Dot(endPos);

#if NM_NEW_BALANCER
        if (!foot.state.m_isInsideSupportHonest)
        {
           // This adds the ankle to toe height on (pointed toe more height over flat less height)
           rage::Matrix34 footTM = body.m_leftFoot.m_tm;
           rage::Vector3 anklePos = footTM.d;
           float footL = 0.26448f*0.5f;
           //float footHeight = 0.07522f;
           if (foot.m_footChoice == NmRsCBUDynBal_FootState::kRightStep)
           {
             footTM = body.m_rightFoot.m_tm;
             anklePos = footTM.d;
           }
           rage::Vector3 ankle2toe = footTM.d;
           ankle2toe -= footTM.c*footL*0.5f;
           //ankle2toe += footTM.b*footHeight*0.5f;//bottom of toe
           ankle2toe -= anklePos;
           height -= ankle2toe.z;
           if (ankle2toe.z >0.f)
           {
             height -= ankle2toe.z;
           }
        }
#endif
        // if foot is angle heel down, we don't need this toe-brush-ground code so set speed to -ve
#if !NM_NEW_BALANCER
        if (h < 0.f || targetComingToFoot) // also not a toe brush if target is coming to foot rather than the other way around
#else
        if (h < -0.4f || targetComingToFoot /*|| foot.state.m_isInsideSupportHonest*/) // also not a toe brush if target is coming to foot rather than the other way around
#endif
          speedToTarget = -1.f; // ie so we stop stepping if we hit the ground
        NM_RS_DBG_LOGF(L"h: %.4f", h);
        NM_RS_DBG_LOGF(L"targetComingToFoot: %d", targetComingToFoot);
        NM_RS_DBG_LOGF(L"DistanceToTarget: %.4f", distanceToTarget);
        NM_RS_DBG_LOGF(L"speedToTarget: %.4f", speedToTarget);


        ro.levelVector(desiredPos, height);

        //rage::Vector3 dpiCol(0.0f, 0.5f, 0.0f);
        //NM_RS_CBU_DRAWPOINT( desiredPos, 0.1f, dpiCol);//mmmmmaddtobspy


        rage::Vector3 dif(desiredPos - *ikFootPos);
		Assertf(dif.Mag2()>1.0e-12f && rage::FPIsFinite(dif.Mag2()), "DP = %.3f %.3f %.3f (%.3f). IKFP = %.3f %.3f %.3f (%.3f). LFP = %.3f %.3f %.3f. RFP = %.3f %.3f %.3f. LS EFA LAM = %.3f %.3f %.3f.", 
			desiredPos.x, desiredPos.y, desiredPos.z, desiredPos.Mag(), (*ikFootPos).x, (*ikFootPos).y, (*ikFootPos).z, (*ikFootPos).Mag(), foot.m_leftFootPos.x, foot.m_leftFootPos.y, foot.m_leftFootPos.z, 
			foot.m_rightFootPos.x, foot.m_rightFootPos.y, foot.m_rightFootPos.z, ro.m_legSeparation, ro.m_extraFeetApart, ro.m_legsApartMax);
        ro.levelVector(dif);
		if (!(dif.Mag2()>1.0e-12f && rage::FPIsFinite(dif.Mag2())))
			dif.Zero();
		else 
			vector3ClampMag(dif, 0.f, 0.1f);
        desiredPos += dif;

        clampTarget(
          desiredPos,
          ro.m_gUp,
          body.m_buttocks.m_tm,
          activeHip->m_lean1Extent * ro.m_stepClampScale * ro.m_hipLean1Offset,
          activeHip->m_lean2Extent * ro.m_stepClampScale * ro.m_hipLean2Offset);

        // finally update foot position
        ikFootPos->Set(desiredPos);

        rage::Vector3 desiredCentre(*ikFootPos + ikStanceFoot);
        desiredCentre *= 0.5f;
        ro.levelVector(foot.m_centreOfFeet, ro.m_gUp.Dot(desiredCentre));

#if !__SPU & ART_ENABLE_BSPY
        bspyScratchpad(bspyLastSeenAgent, "stepExit", dToStart);
        bspyScratchpad(bspyLastSeenAgent, "stepExit", speedToTarget);
        bspyScratchpad(bspyLastSeenAgent, "footPlacement3", foot.m_centreOfFeet);
#endif // ART_ENABLE_BSPY
        // toggle balance flags
        if (foot.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
        {
          foot.state.m_leftFootBalance = false;
          foot.state.m_rightFootBalance = true;
          bool hitGroundProper = body.cd.m_leftFootCollided && speedToTarget<0.f; // if we're continuing to get closer to the target, then we haven't 'properly' hit the ground
#if !__SPU & ART_ENABLE_BSPY && 0
          bspyScratchpad(bspyLastSeenAgent, "stepExitL", hitGroundProper);
#endif // ART_ENABLE_BSPY
          // test exit condition
          if ((dToStart < 0.5f || (foot.state.m_achievedGoal && foot.state.m_leftGround)) && hitGroundProper)
          {
            stepExit = true;
            NM_RS_DBG_LOGF(L"dToStart: %.4f", dToStart);
            NM_RS_DBG_LOGF(L"foot.m_achievedGoal: %d", foot.state.m_achievedGoal);
            NM_RS_DBG_LOGF(L"hitGroundProper: %d", hitGroundProper);
            NM_RS_DBG_LOGF(L"speedToTarget: %.4f", speedToTarget);
#if !__SPU & ART_ENABLE_BSPY && 0
            bspyScratchpad(bspyLastSeenAgent, "stepExitL1", true);
#endif // ART_ENABLE_BSPY

          }
          if (body.cd.m_leftFootCollided && !body.cd.m_rightFootCollided && !body.cd.m_rightFootCollidedLast)//MMMM1last is to stop quick foot switching
          {
            stepExit = true;
#if !__SPU & ART_ENABLE_BSPY && 0
            bspyScratchpad(bspyLastSeenAgent, "stepExitL2", true);
#endif // ART_ENABLE_BSPY
          }
        }
        else
        {
          foot.state.m_leftFootBalance = true;
          foot.state.m_rightFootBalance = false;
          bool hitGroundProper = body.cd.m_rightFootCollided && speedToTarget<0.f;

#if !__SPU & ART_ENABLE_BSPY && 0
          bspyScratchpad(bspyLastSeenAgent, "stepExitR", hitGroundProper);
#endif // ART_ENABLE_BSPY
          // test exit condition
          if ((dToStart < 0.5f || (foot.state.m_achievedGoal && foot.state.m_leftGround)) && hitGroundProper)
          {
            stepExit = true;
            NM_RS_DBG_LOGF(L"dToStart: %.4f", dToStart);
            NM_RS_DBG_LOGF(L"foot.m_achievedGoal: %d", foot.state.m_achievedGoal);
            NM_RS_DBG_LOGF(L"hitGroundProper: %d", hitGroundProper);
            NM_RS_DBG_LOGF(L"speedToTarget: %.4f", speedToTarget);
#if !__SPU & ART_ENABLE_BSPY && 0
            bspyScratchpad(bspyLastSeenAgent, "stepExitR1", true);
#endif // ART_ENABLE_BSPY

          }
          if (body.cd.m_rightFootCollided && !body.cd.m_leftFootCollided && !body.cd.m_leftFootCollidedLast)//MMMM1last is to stop quick foot switching
          {
            stepExit = true;
#if !__SPU & ART_ENABLE_BSPY && 0
            bspyScratchpad(bspyLastSeenAgent, "stepExitR2", true);
#endif // ART_ENABLE_BSPY
          }
        }
      }

      // finished with this particular foot?
      if (stepExit)
      {
        if (foot.m_footChoice == NmRsCBUDynBal_FootState::kLeftStep)
          foot.state.m_leftFootBalance = true;
        else
          foot.state.m_rightFootBalance = true;

        foot.m_footChoice = NmRsCBUDynBal_FootState::kNotStepping;

        // perform exit->entry re-evaluation of step choice
        if (!foot.m_forceBalance && !foot.m_dontChangeStep)
        {
          dbmFootPlacementStepChoice(ro, body, foot);
          //If the step has changed twice in this routine only count that as an additional 1 step - not 2
          if (numOfSteps == foot.m_numOfSteps-2)
          {
            foot.m_numOfSteps -= 1;
            foot.m_numOfSteps4Max -=1;
          }
        }
      }

      if (foot.m_forceBalance)
      {
        foot.m_footChoice = NmRsCBUDynBal_FootState::kNotStepping;
      }

      return true;
    }
}

using namespace ART;

void NmRsCBU_DBMFootPlacement(rage::sysTaskParameters& param)
{
  Assert(param.ReadOnlyCount == 2);
  Assert(param.ReadOnly[0].Data);
  Assert(param.ReadOnly[0].Size == sizeof(NmRsCBUDynBal_ReadOnly));

  Assert(param.ReadOnly[1].Data);
  Assert(param.ReadOnly[1].Size == sizeof(NmRsCBUDynBal_BodyPacket));

  Assert(param.Input.Data);
  Assert(param.Input.Size == sizeof(NmRsCBUDynBal_FootState));

  const NmRsCBUDynBal_ReadOnly* ro = static_cast<const NmRsCBUDynBal_ReadOnly*>(param.ReadOnly[0].Data);
  const NmRsCBUDynBal_BodyPacket* body = static_cast<const NmRsCBUDynBal_BodyPacket*>(param.ReadOnly[1].Data);
  NmRsCBUDynBal_FootState* foot = static_cast<NmRsCBUDynBal_FootState*>(param.Input.Data);

  dbmFootPlacement(*ro, *body, *foot);
}


#if __SPU
#include "vector/quaternion.cpp"
#include "NmRsUtils.cpp"
#endif // __SPU

