/*
* Copyright (c) 2005-2010 NaturalMotion Ltd. All rights reserved. 
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
//////////////////////////// ONE SHOT MESSAGES (in alphabetical order) ////////////////////////////
/*
activePose:  This will take any animation currently set as the incoming transforms of the agent and drive the character to the pose defined therein. To constantly drive an agent to an animation, this message must be sent every frame. The stiffness of the characters joints will determine how closely they match the animation pose.
The mask value determines which effector sets will be modified. Specifying 'fb' or no mask value will cause the effect to be applied to the whole character.
Otherwise, specify 'l' or 'u' for the first character to mask off to just lower or upper body parts. Then, you can specify the following for the second character:
When the first character is 'u', the second parameter can be:
* 'c' = just clavicles
* 'l' = left arm only
* 'r' = right arm only
* 'a' = both arms and clavicles
* 's' = just spine
* 't' = torso only, eg. spine & arms and Clavicles
* 'k' = trunk only, eg. spine & head
* 'n' = neck and head
* 'b' = everything
* 'w' = wrists
when the first character is 'l', the second parameter can be:
* 'l' = just left leg
* 'r' = just right leg
* 'b' = both legs
* 'a' = both ankles
note that the characters should be lower-case.
==================================================
To allow allow any combinations of joints to be masked:
1) Mask can be a string of BITWISE logic operators (no brackets or spaces allowed) on the above 2 character masks eg. 'uc&ul' - left clavicle, 'ub&~uw' - ub except for wrists, 'ul|ur' left and right arms.
2) Mask can now also take a bitMask value (limited to 32 bits) expressed as a string. 
Masking is performed on a part basis.  To mask a particular joint, indicate the joint's child part.
* Mask can be a bitMask in hex '0x********'.
* Mask can be a bitMask in binary '0b**...**'.
0 = spine_1,
1 = spine_2,
2 = spine_3,
3 = neck_lower,
4 = neck_upper,
5 = clavicle_jnt_left,
6 = shoulder_left,
7 = elbow_left,
8 = wrist_left,
9 = clavicle_jnt_right,
10 = shoulder_right,
11 = elbow_right,
12 = wrist_right,
13 = spine_0,
14 = hip_left,
15 = knee_left,
16 = ankle_left,
17 = hip_right,
18 = knee_right,
19 = ankle_right.
*/
BEHAVIOUR(activePose)
{
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see notes for explanation)
  PARAMETER(useGravityCompensation, false, bool, false, true); // Apply gravity compensation as well?
  PARAMETER(animSource, ART::kITSourceCurrent, int, ART::kITSourceCurrent, ART::KITSourceCount-1); // AnimSource 0 = CurrentItms, 1 = PreviousItms, 2 = AnimItms
}
/*
addPatch:  Add geometry into the environmental awareness
  geomType
  0 EO_UseGameGeometry,  (i.e. use the actual game intance/bounds geometry type)
  1 EO_Point,
  2 EO_Line,
  3	EO_Corner,
  4	EO_Edge,
  5	EO_Plane, e.g. Walls/Table tops
  6	EO_Disc, e.g. Table tops
  7	EO_Capsule,
  8	EO_Sphere,
  9	EO_Box. E.g for railings/blocks (closed edge)

  Action (bitWise)
  1	EF_Grab,
  2	EF_Brace,
  3	EF_LeanAgainst,
  4	EF_PushOff,
  5	EF_Support,
  6	EF_SlideOver
  7	EF_RollOver
  8	EF_FallOver
  9	EF_JumpOver
  ..32 max

*/
#if NM_EA
BEHAVIOUR(addPatch)
{
  PARAMETER(geomType, 1, int, 0, 9); //
  PARAMETER(action, 0, int, INT_MIN, INT_MAX); //
  PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //
  PARAMETER(boundIndex, 0, int, 0, INT_MAX); //
  PARAMETERV0(corner, FLT_MAX); //
  PARAMETERV0(faceNormal0, FLT_MAX); //
  PARAMETERV0(faceNormal1, FLT_MAX); //
  PARAMETERV0(faceNormal2, FLT_MAX); //
  PARAMETERV0(edgeLengths, FLT_MAX); //
  PARAMETER(edgeRadius, 0.f, float, 0.f, FLT_MAX); //
  PARAMETER(localVectors, true, bool, false, true); //normal,hitPoint are in local coordinates of bodyPart
}
#endif//#if NM_EA
/*
applyImpulse:  Apply an impulse to a named body part
optional equalizeAmount between 0 and 1.
0 means straight impulse (lighter objects will move further)
1 means multiplied by mass relative to the average (all mass objects will move equally)
optional hitPoint in world space
*/
BEHAVIOUR(applyImpulse)
{
  PARAMETER(equalizeAmount, 0.00f, float, 0.f, 1.f); //0 means straight impulse, 1 means multiply by the mass (change in velocity)
  PARAMETER(partIndex, 0, int, 0, 28); //index of part being hit.
  PARAMETERV0(impulse, 4500.f); //impulse vector (impulse is change in momentum)
  PARAMETERV0(hitPoint, FLT_MAX); //optional point on part where hit
  PARAMETER(localHitPointInfo, false, bool, false, true); // hitPoint in local coordinates of bodyPart
  PARAMETER(localImpulseInfo, false, bool, false, true); // impulse in local coordinates of bodyPart
  PARAMETER(angularImpulse, false, bool, false, true); // impulse should be considered an angular impulse
}
/*
applyBulletImpulse:  Apply a bullet impulse to a named body part, optional equalizeAmount between 0 and 1.
0 means straight impulse (lighter objects will move further)
1 means multiplied by mass relative to the average (all mass objects will move equally)
optional hitPoint in world space
*/
BEHAVIOUR(applyBulletImpulse)
{
  PARAMETER(equalizeAmount, 0.00f, float, 0.f, 1.f); //0 means straight impulse, 1 means multiply by the mass (change in velocity)
  PARAMETER(partIndex, 0, int, 0, 28); //index of part being hit.
  PARAMETERV0(impulse, 1000.f); //impulse vector (impulse is change in momentum)
  PARAMETERV0(hitPoint, FLT_MAX); //optional point on part where hit
  PARAMETER(localHitPointInfo, false, bool, false, true); //true = hitPoint is in local coordinates of bodyPart, false = hitpoint is in world coordinates
}
/*
bodyRelax:  Set the amount of relaxation across the whole body; Used to collapse the character into a rag-doll-like state.
*/
BEHAVIOUR(bodyRelax)
{
  PARAMETER(relaxation, 50.00f, float, 0.f, 100.f); //How relaxed the body becomes, in percentage relaxed. 100 being totally rag-dolled, 0 being very stiff and rigid.
  PARAMETER(damping, 1.f, float, 0.f, 2.f);
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
  PARAMETER(holdPose, false, bool, false, true); //automatically hold the current pose as the character relaxes - can be used to avoid relaxing into a t-pose
}
/*
configureBalance:  This single message allows you to configure various parameters used on any behaviour that uses the dynamic balance. 
*/
BEHAVIOUR(configureBalance)
{
  PARAMETER(stepHeight, 0.10f, float, 0.f, 0.4f); //maximum height that character steps vertically (above 0.2 is high...but ok for say underwater)
#if NM_STEP_UP
  PARAMETER(stepHeightInc4Step, 0.10f, float, 0.f, 0.4f); //added to stepHeight if going up steps
#endif
  PARAMETER(legsApartRestep, 0.20f, float, 0.f, 0.7f); //if the legs end up more than (legsApartRestep + hipwidth) apart even though balanced, take another step
  PARAMETER(legsTogetherRestep, 1.f, float, 0.f, 1.0f); //mmmm0.1 for drunk if the legs end up less than (hipwidth - legsTogetherRestep) apart even though balanced, take another step.  A value of 1 will turn off this feature and the max value is hipWidth = 0.23f by default but is model dependent
  PARAMETER(legsApartMax, 2.00f, float, 0.0f, 2.0f); //FRICTION WORKAROUND: if the legs end up more than (legsApartMax + hipwidth) apart when balanced, adjust the feet positions to slide back so they are legsApartMax + hipwidth apart.  Needs to be less than legsApartRestep to see any effect
  PARAMETER(taperKneeStrength, true, bool, false, true); //does the knee strength reduce with angle
  PARAMETER(legStiffness, 12.00f, float, 6.f, 16.f); //stiffness of legs
  PARAMETER(leftLegSwingDamping, 1.00f, float, 0.2f, 4.f); //damping of left leg during swing phase (mmmmDrunk used 1.25 to slow legs movement)
  PARAMETER(rightLegSwingDamping, 1.00f, float, 0.2f, 4.f); //damping of right leg during swing phase (mmmmDrunk used 1.25 to slow legs movement)
	PARAMETER(opposeGravityLegs, 1.00f, float, 0.0f, 4.f); //Gravity opposition applied to hips and knees 
	PARAMETER(opposeGravityAnkles, 1.00f, float, 0.0f, 4.f); //Gravity opposition applied to ankles.  General balancer likes 1.0.  StaggerFall likes 0.1
  PARAMETER(leanAcc, 0.0f, float, 0.0f, 1.0f); //Multiplier on the floorAcceleration added to the lean 
  PARAMETER(hipLeanAcc, 0.5f, float, 0.0f, 1.f); //Multiplier on the floorAcceleration added to the leanHips
  PARAMETER(leanAccMax, 5.00f, float, 0.0f, 10.f); //Max floorAcceleration allowed for lean and leanHips
  PARAMETER(resistAcc, 0.5f, float, 0.0f, 2.f); //Level of cheat force added to character to resist the effect of floorAcceleration (anti-Acceleration) - added to upperbody.
  PARAMETER(resistAccMax, 3.0f, float, 0.0f, 20.f); //Max floorAcceleration allowed for anti-Acceleration. If >20.0 then it is probably in a crash
  PARAMETER(footSlipCompOnMovingFloor, true, bool, false, true); //[This parameter will be removed when footSlipCompensation preserves the foot angle on a moving floor]. If the character detects a moving floor and footSlipCompOnMovingFloor is false then it will turn off footSlipCompensation - at footSlipCompensation preserves the global heading of the feet.  If footSlipCompensation is off then the character usually turns to the side in the end although when turning the vehicle turns it looks promising for a while 
	PARAMETER(ankleEquilibrium, 0.00f, float, -1.0f, 1.f); //ankle equilibrium angle used when static balancing
	PARAMETER(extraFeetApart, 0.00f, float, -1.0f, 1.f); //additional feet apart setting 
  PARAMETER(balanceAbortThreshold, 0.60f, float, 0.f, 1.f); //when the character gives up and goes into a fall
  PARAMETER(giveUpHeight, 0.5f, float, 0.f, 1.5f); //height between lowest foot and COM below which balancer will give up
  PARAMETER(stepClampScale, 1.00f, float, 0.f, 1.f); //
  PARAMETER(stepClampScaleVariance, 0.00f, float, -1.f, 1.f); //Variance in clamp scale every step. if negative only takes away from clampScale
  PARAMETER(predictionTimeHip, 0.3f, float, -1.f, 1.f); //  amount of time (seconds) into the future that the character tries to move hip to (kind of).  Will be controlled by balancer in future but can help recover spine quicker from bending forwards to much.
  PARAMETER(predictionTime, 0.2f, float, 0.f, 1.f); //  amount of time (seconds) into the future that the character tries to step to. bigger values try to recover with fewer, bigger steps. smaller values recover with smaller steps, and generally recover less.
  PARAMETER(predictionTimeVariance, 0.0f, float, -1.f, 1.f); ////Variance in predictionTime every step. if negative only takes away from predictionTime
  PARAMETER(maxSteps, 100, int, 1, INT_MAX);// Maximum number of steps that the balancer will take.
  PARAMETER(maxBalanceTime, 50.f, float, 1.f, FLT_MAX);// Maximum time(seconds) that the balancer will balance for.
  PARAMETER(extraSteps, -1, int, -1, INT_MAX);// Allow the balancer to take this many more steps before hitting maxSteps. If negative nothing happens(safe default)
  PARAMETER(extraTime, -1.0f, float, -1.0f, FLT_MAX);// Allow the balancer to balance for this many more seconds before hitting maxBalanceTime.  If negative nothing happens(safe default)
  PARAMETER(fallType, 0, int, 0, INT_MAX);// How to fall after maxSteps or maxBalanceTime: 0=rampDown stiffness, 1= 0 and dontChangeStep, 2= 0 and forceBalance, 3=0 and slump (BCR has to be active)
  PARAMETER(fallMult, 1.f, float, 0.f, 100.f);// Multiply the rampDown of stiffness on falling by this amount (>1 fall quicker)
  PARAMETER(fallReduceGravityComp, false, bool, false, true);// Reduce gravity compensation as the legs weaken on falling
  PARAMETER(rampHipPitchOnFail, false, bool, false, true);// bend over when falling after maxBalanceTime
  PARAMETER(stableLinSpeedThresh, 0.25f, float, 0.01f, 10.f);// Linear speed threshold for successful balance.
  PARAMETER(stableRotSpeedThresh, 0.25f, float, 0.01f, 10.f);// Rotational speed threshold for successful balance.
  PARAMETER(failMustCollide, false, bool, false, true);// The upper body of the character must be colliding and other failure conditions met to fail
	PARAMETER(ignoreFailure, false, bool, false, true);// Ignore maxSteps and maxBalanceTime and try to balance forever
	PARAMETER(changeStepTime, -1.0f, float, -1.f, 5.f);// time not in contact (airborne) before step is changed. If -ve don't change step
  PARAMETER(balanceIndefinitely, false, bool, false, true);// Ignore maxSteps and maxBalanceTime and try to balance forever
  PARAMETER(movingFloor, false, bool, false, true);// temporary variable to ignore movingFloor code that generally causes the character to fall over if the feet probe a moving object e.g. treading on a gun 
  PARAMETER(airborneStep, true, bool, false, true);// when airborne try to step.  Set to false for e.g. shotGun reaction
  PARAMETER(useComDirTurnVelThresh, 0.f, float, 0.f, 10.f);// Velocity below which the balancer turns in the direction of the COM forward instead of the ComVel - for use with shot from running with high upright constraint use 1.9
  PARAMETER(minKneeAngle, -0.5f, float, -0.5f, 1.5f);// Minimum knee angle (-ve value will mean this functionality is not applied).  0.4 seems a good value
  PARAMETER(flatterSwingFeet, false, bool, false, true);//
  PARAMETER(flatterStaticFeet, false, bool, false, true);//
  PARAMETER(leanAgainstVelocity, 0.f, float, 0.0f, 1.f); // 
  PARAMETER(stepDecisionThreshold, 0.f, float, 0.0f, 1.f); // 
  PARAMETER(stepIfInSupport, true, bool, false, true);//The balancer sometimes decides to step even if balanced
  PARAMETER(alwaysStepWithFarthest, false, bool, false, true);//
  PARAMETER(standUp, false, bool, false, true);//standup more with increased velocity
  PARAMETER(depthFudge, 0.01f, float, 0.0f, 1.f); //Supposed to increase foot friction: Impact depth of a collision with the foot is changed when the balancer is running - impact.SetDepth(impact.GetDepth() - depthFudge)
  PARAMETER(depthFudgeStagger, 0.01f, float, 0.0f, 1.f); //Supposed to increase foot friction: Impact depth of a collision with the foot is changed when staggerFall is running - impact.SetDepth(impact.GetDepth() - depthFudgeStagger) 
  PARAMETER(footFriction, 1.f, float, 0.0f, 40.f); //Foot friction multiplier is multiplied by this amount if balancer is running 
  PARAMETER(footFrictionStagger, 1.f, float, 0.0f, 40.f); //Foot friction multiplier is multiplied by this amount if staggerFall is running  
  
#if DYNBAL_GIVEUP_RAMP
  PARAMETER(giveUpHeightEnd, 0.5f, float, 0.0f, 1.5f); // if this value is different from giveUpHeight, actual giveUpHeight will be ramped toward this value
  PARAMETER(balanceAbortThresholdEnd, 0.6f, float, 0.0f, 1.0f);// if this value is different from balanceAbortThreshold, actual balanceAbortThreshold will be ramped toward this value
  PARAMETER(giveUpRampDuration, -1.0f, float, -1.0f, 10.f); // duration of ramp from start of behaviour for above two parameters. If smaller than 0, no ramp is applied
  PARAMETER(leanToAbort, 0.6f, float, 0.0f, 1.0f); // lean at which to send abort message when maxSteps or maxBalanceTime is reached
#endif
}
/*
configureBalanceReset:  reset the values configurable by the Configure Balance message to their defaults.
*/
BEHAVIOUR(configureBalanceReset)
{
}

#if NM_USE_IK_SELF_AVOIDANCE
/*
configureSelfAvoidance: this single message allows to configure self avoidance for the character.
BBDD Self avoidance tech.
*/
BEHAVIOUR(configureSelfAvoidance)
{
  PARAMETER(useSelfAvoidance, false, bool, false, true); // Enable or disable self avoidance tech.
  PARAMETER(overwriteDragReduction, false, bool, false, true); // Specify whether self avoidance tech should use original IK input target or the target that has been already modified by getStabilisedPos() tech i.e. function that compensates for rotational and linear velocity of shoulder/thigh.
  PARAMETER(torsoSwingFraction, 0.75f, float, 0.0f, 1.0f); // Place the adjusted target this much along the arc between effector (wrist) and target, value in range [0,1].
  PARAMETER(maxTorsoSwingAngleRad, PI/4.0f, float, 0.0f, PI/2.0f);// Max value on the effector (wrist) to adjusted target offset.
  PARAMETER(selfAvoidIfInSpineBoundsOnly, false, bool, false, true); // Restrict self avoidance to operate on targets that are within character torso bounds only.
  PARAMETER(selfAvoidAmount, 0.5f, float, 0.0f, 1.0f); // Amount of self avoidance offset applied when angle from effector (wrist) to target is greater then right angle i.e. when total offset is a blend between where effector currently is to value that is a product of total arm length and selfAvoidAmount. SelfAvoidAmount is in a range between [0, 1].
  PARAMETER(overwriteTwist, false, bool, false, true); // Overwrite desired IK twist with self avoidance procedural twist.
  PARAMETER(usePolarPathAlgorithm, false, bool, false, true); // Use the alternative self avoidance algorithm that is based on linear and polar target blending. WARNING: It only requires "radius" in terms of parametrization.
  PARAMETER(radius, 0.3f, float, 0.0f, 1.0f); // Self avoidance radius, measured out from the spine axis along the plane perpendicular to that axis. The closer is the proximity of reaching target to that radius, the more polar (curved) motion is used for offsetting the target. WARNING: Parameter only used by the alternative algorithm that is based on linear and polar target blending.
}
#endif // NM_USE_IK_SELF_AVOIDANCE

/*
configureBullets:  This single message allows you to configure the way bullets are applied to the character.
NB. Airborne and oneLeg modulation of forces only happen if the dynamicBalancer is trying to balance
*/
BEHAVIOUR(configureBullets)
{
  PARAMETER(impulseSpreadOverParts, false, bool, false, true); // spreads impulse across parts. currently only for spine parts, not limbs.
  PARAMETER(impulseLeakageStrengthScaled, false, bool, false, true); // for weaker characters subsequent impulses remain strong  
  PARAMETER(impulsePeriod, 0.1f, float, 0.0f, 1.0f); // duration that impulse is spread over (triangular shaped)
  PARAMETER(impulseTorqueScale, 1.0f, float, 0.0f, 1.0f); // An impulse applied at a point on a body equivalent to an impulse at the centre of the body and a torque.  This parameter scales the torque component. (The torque component seems to be excite the rage looseness bug which sends the character in a sometimes wildly different direction to an applied impulse)
  PARAMETER(loosenessFix, false, bool, false, true); // Fix the rage looseness bug by applying only the impulse at the centre of the body unless it is a spine part then apply the twist component only of the torque as well.
  PARAMETER(impulseDelay, 0.0f, float, 0.0f, 1.0f); // time from hit before impulses are being applied 
  PARAMETER(impulseReductionPerShot, 0.0f, float, 0.0f, 1.0f); // by how much are subsequent impulses reduced (e.g. 0.0: no reduction, 0.1: 10% reduction each new hit)
  PARAMETER(impulseRecovery, 0.0f, float, 0.0f, 60.0f); // recovery rate of impulse strength per second (impulse strength from 0.0:1.0).  At 60fps a impulseRecovery=60.0 will recover in 1 frame  
  PARAMETER(torqueMode, 0, int, 0, 2); // 0: Disabled | 1: character strength proportional (can reduce impulse amount) | 2: Additive (no reduction of impulse and not proportional to character strength)
  PARAMETER(torqueSpinMode, 0, int, 0, 2); // 0: spin direction from impulse direction | 1: random direction | 2: direction flipped with each bullet (for burst effect)
  PARAMETER(torqueFilterMode, 0, int, 0, 2); // 0: apply torque for every bullet | 1: only apply new torque if previous has finished | 2: Only apply new torque if its spin direction is different from previous torque
  PARAMETER(torqueAlwaysSpine3, true, bool, false, true); // always apply torques to spine3 instead of actual part hit
  PARAMETER(torqueDelay, 0.0f, float, 0.0f, 1.0f); // time from hit before torques are being applied
  PARAMETER(torquePeriod, 0.12f, float, 0.0f, 1.0f); // duration of torque
  PARAMETER(torqueGain, 4.0f, float, 0.0f, 10.0f); // multiplies impulse magnitude to arrive at torque that is applied
  PARAMETER(torqueCutoff, 0.0f, float, 0.0f, 1.0f); // minimum ratio of impulse that remains after converting to torque (if in strength-proportional mode)
  PARAMETER(torqueReductionPerTick, 0.0f, float, 0.0f, 1.0f); // ratio of torque for next tick (e.g. 1.0: not reducing over time, 0.9: each tick torque is reduced by 10%)
  PARAMETER(liftGain, 0.0f, float, 0.0f, 1.0f); // amount of lift (directly multiplies torque axis to give lift force)
  PARAMETER(counterImpulseDelay, 0.03333f, float, 0.0f, 1.0f); // time after impulse is applied that counter impulse is applied
  PARAMETER(counterImpulseMag, 0.5f, float, 0.0f, 1.0f); // amount of the original impulse that is countered
  PARAMETER(counterAfterMagReached, false, bool, false, true); // applies the counter impulse counterImpulseDelay(secs) after counterImpulseMag of the Impulse has been applied
  PARAMETER(doCounterImpulse, false, bool, false, true); //add a counter impulse to the pelvis
  PARAMETER(counterImpulse2Hips, 1.0f, float, 0.0f, 1.0f); // amount of the counter impulse applied to hips - the rest is applied to the part originally hit
  PARAMETER(impulseAirMult, 1.0f, float, 0.0f, 1.0f); // amount to scale impulse by if the character is airborne and dynamicBalance is OK and impulse is above impulseAirMultStart
  PARAMETER(impulseAirMultStart, 100.f, float, 0.0f, FLT_MAX); // if impulse is above this value scale it by impulseAirMult
  PARAMETER(impulseAirMax, 100.f, float, 0.0f, FLT_MAX); // amount to clamp impulse to if character is airborne  and dynamicBalance is OK
  PARAMETER(impulseAirApplyAbove, 399.f, float, 0.0f, FLT_MAX); // if impulse is above this amount then do not scale/clamp just let it through as is - it's a shotgun or cannon
  PARAMETER(impulseAirOn, false, bool, false, true); //scale and/or clamp impulse if the character is airborne and dynamicBalance is OK
#if NM_ONE_LEG_BULLET
  PARAMETER(impulseOneLegMult, 1.0f, float, 0.0f, 1.0f); // amount to scale impulse by if the character is contacting with one foot only and dynamicBalance is OK and impulse is above impulseAirMultStart
  PARAMETER(impulseOneLegMultStart, 100.f, float, 0.0f, FLT_MAX); // if impulse is above this value scale it by impulseOneLegMult
  PARAMETER(impulseOneLegMax, 100.f, float, 0.0f, FLT_MAX); // amount to clamp impulse to if character is contacting with one foot only  and dynamicBalance is OK
  PARAMETER(impulseOneLegApplyAbove, 399.f, float, 0.0f, FLT_MAX); // if impulse is above this amount then do not scale/clamp just let it through as is - it's a shotgun or cannon
  PARAMETER(impulseOneLegOn, false, bool, false, true); //scale and/or clamp impulse if the character is contacting with one leg only and dynamicBalance is OK
#endif// #if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
  //PARAMETER(rbForce, 1.00f, float, 0.f, 1.f); //Leave this alone!!! (for testing of torque removing bullets)
  PARAMETER(rbRatio, 0.00f, float, 0.f, 1.f); //0.0 no rigidBody response, 0.5 half partForce half rigidBody, 1.0 = no partForce full rigidBody
  PARAMETER(rbLowerShare, 0.5f, float, 0.f, 1.f); //rigid body response is shared between the upper and lower body (rbUpperShare = 1-rbLowerShare). rbLowerShare=0.5 gives upper and lower share scaled by mass.  i.e. if 70% ub mass and 30% lower mass then rbLowerShare=0.5 gives actualrbShare of 0.7ub and 0.3lb. rbLowerShare>0.5 scales the ub share down from 0.7 and the lb up from 0.3.  
  PARAMETER(rbMoment, 1.00f, float, 0.f, 1.f); //0.0 only force, 0.5 = force and half the rigid body moment applied, 1.0 = force and full rigidBody moment
  PARAMETER(rbMaxTwistMomentArm, 0.5f, float, 0.f, 2.f); //Maximum twist arm moment of bullet applied
  PARAMETER(rbMaxBroomMomentArm, 1.0f, float, 0.f, 2.f); //Maximum broom((everything but the twist) arm moment of bullet applied
  PARAMETER(rbRatioAirborne, 0.00f, float, 0.f, 1.f); //if Airborne: 0.0 no rigidBody response, 0.5 half partForce half rigidBody, 1.0 = no partForce full rigidBody
  PARAMETER(rbMomentAirborne, 1.00f, float, 0.f, 1.f); //if Airborne: 0.0 only force, 0.5 = force and half the rigid body moment applied, 1.0 = force and full rigidBody moment
  PARAMETER(rbMaxTwistMomentArmAirborne, 0.5f, float, 0.f, 2.f); //if Airborne: Maximum twist arm moment of bullet applied
  PARAMETER(rbMaxBroomMomentArmAirborne, 1.0f, float, 0.f, 2.f); //if Airborne: Maximum broom((everything but the twist) arm moment of bullet applied
  PARAMETER(rbRatioOneLeg, 0.00f, float, 0.f, 1.f); //if only one leg in contact: 0.0 no rigidBody response, 0.5 half partForce half rigidBody, 1.0 = no partForce full rigidBody
  PARAMETER(rbMomentOneLeg, 1.00f, float, 0.f, 1.f); //if only one leg in contact: 0.0 only force, 0.5 = force and half the rigid body moment applied, 1.0 = force and full rigidBody moment
  PARAMETER(rbMaxTwistMomentArmOneLeg, 0.5f, float, 0.f, 2.f); //if only one leg in contact: Maximum twist arm moment of bullet applied
  PARAMETER(rbMaxBroomMomentArmOneLeg, 1.0f, float, 0.f, 2.f); //if only one leg in contact: Maximum broom((everything but the twist) arm moment of bullet applied
  PARAMETER(rbTwistAxis, 0, int, 0, 1); //Twist axis 0=World Up, 1=CharacterCOM up
  PARAMETER(rbPivot, false, bool, false, true); //if false pivot around COM always, if true change pivot depending on foot contact:  to feet centre if both feet in contact, or foot position if 1 foot in contact or COM position if no feet in contact
#endif
}
#if NM_RUNTIME_LIMITS
/*
configureLimits:  Enable/disable/edit character limits in real time.  This adjusts limits in RAGE-native space and will *not* reorient the joint.
*/
BEHAVIOUR(configureLimits)
{
  PARAMETER(mask, "fb", char *, "", "");                    // Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  for joint limits to configure. Ignored if index != -1.
  PARAMETER(enable,true,bool,false,true);                   // If false, disable (set all to PI, -PI) limits.
  PARAMETER(toDesired,false,bool,false,true);               // If true, set limits to accommodate current desired angles
  PARAMETER(restore,false,bool,false,true);                 // Return to cached defaults?
  PARAMETER(toCurAnimation,false,bool,false,true);			// If true, set limits to the current animated limits
  PARAMETER(index, -1, int, -1, INT_MAX);                   // Index of effector to configure.  Set to -1 to use mask.
  PARAMETER(lean1, PI/2, float, 0.f, PI);                   // Custom limit values to use if not setting limits to desired. Limits are RAGE-native, not NM-wrapper-native.
  PARAMETER(lean2, PI/2, float, 0.f, PI);
  PARAMETER(twist, PI/2, float, 0.f, PI);
  PARAMETER(margin, PI/16, float, 0.f, PI);					// Joint limit margin to add to current animation limits when using those to set runtime limits.
}
#endif//NM_RUNTIME_LIMITS
/*
configureShotInjuredArm:  This single message allows you to configure the injured arm reaction during shot
*/
BEHAVIOUR(configureShotInjuredArm)
{
  PARAMETER(injuredArmTime, 0.25f, float, 0.0f, 2.0f); // length of the reaction
  PARAMETER(hipYaw, 0.8f, float, -2.0f, 2.0f); // Amount of hip twist.  (Negative values twist into bullet direction - probably not what is wanted)
  PARAMETER(hipRoll, 0.0f, float, -2.0f, 2.0f); // Amount of hip roll
  PARAMETER(forceStepExtraHeight, 0.07f, float, 0.0f, 0.7f); // Additional height added to stepping foot
  PARAMETER(shrugTime, 0.4f, float, 0.0f, 2.0f); // Exaggerate arm hit by shrugging the clavicles - this unfortunately makes the ik solution for tuck into body too chicken arms so don't do it for too long
  PARAMETER(forceStep, true, bool, false, true); // force a step to be taken whether pushed out of balance or not
  PARAMETER(stepTurn, true, bool, false, true); // turn the character using the balancer
  PARAMETER(velMultiplierStart, 1.f, float, 0.0f, 20.0f); //Start velocity where parameters begin to be ramped down to zero linearly 
  PARAMETER(velMultiplierEnd, 5.f, float, 1.0f, 40.0f); //End velocity of ramp where parameters are scaled to zero 
  PARAMETER(velForceStep, 0.8f, float, 0.0f, 20.0f); //Velocity above which a step is not forced 
  PARAMETER(velStepTurn, 0.8f, float, 0.0f, 20.0f); //Velocity above which a stepTurn is not asked for 
  PARAMETER(velScales,true, bool, false, true); //Use the velocity scaling parameters.  Tune for standing still then use velocity scaling to make sure a running character stays balanced (the turning tends to make the character fall over more at speed) 
}
/*
configureShotInjuredLeg:  This single message allows you to configure the injured leg reaction during shot
*/
BEHAVIOUR(configureShotInjuredLeg)
{
  PARAMETER(timeBeforeCollapseWoundLeg, 0.3f, float, 0.0f, 10.0f); //time before a wounded leg is set to be weak and cause the character to collapse
  PARAMETER(legInjuryTime, 0.4f, float, 0.0f, 2.0f);//Leg inury duration (reaction to being shot in leg)
  PARAMETER(legForceStep, true, bool, false, true); // force a step to be taken whether pushed out of balance or not
  PARAMETER(legLimpBend, 0.0f, float, 0.0f, 1.0f);//Bend the legs via the balancer by this amount if stepping on the injured leg. 0.2 seems a good default
  PARAMETER(legLiftTime, 0.0f, float, 0.0f, 2.0f);//Leg lift duration (reaction to being shot in leg) (lifting happens when not stepping with other leg)
  PARAMETER(legInjury, 0.3f, float, 0.0f, 1.0f);//Leg injury - leg strength is reduced
  PARAMETER(legInjuryHipPitch, 0.0f, float, -1.0f, 1.0f);//Leg injury bend forwards amount when not lifting leg
  PARAMETER(legInjuryLiftHipPitch, 0.0f, float, -1.0f, 1.0f);//Leg injury bend forwards amount when lifting leg (lifting happens when not stepping with other leg)
  PARAMETER(legInjurySpineBend, 0.1f, float, -1.0f, 1.0f);//Leg injury bend forwards amount when not lifting leg
  PARAMETER(legInjuryLiftSpineBend, 0.2f, float, -1.0f, 1.0f);//Leg injury bend forwards amount when lifting leg (lifting happens when not stepping with other leg)
}

/*
defineAttachedObject:  Call every frame to update the position of an attached object (eg gun) on the character. 
This allows the balancer to balance properly with the extra weight.
*/
BEHAVIOUR(defineAttachedObject)
{
  PARAMETER(partIndex, -1, int, -1, 21); //index of part to attach to
  PARAMETER(objectMass, 0.00f, float, 0.f, FLT_MAX); //mass of the attached object
  PARAMETERV0(worldPos, FLT_MAX); //world position of attached object's centre of mass. must be updated each frame.
}
/*
forceToBodyPart:  Apply an impulse to a named body part
*/
BEHAVIOUR(forceToBodyPart)
{
  PARAMETER(partIndex, 0, int, 0, 28); // part or link or bound index
  PARAMETERV(force, 0.00, -50.00, 0.00,  0.f, 100000.f); //force to apply
  PARAMETER(forceDefinedInPartSpace, false, bool, false, true);
}
/*
leanInDirection:  Use in addition to any balance behaviour. Will lean and head in the specified direction
with a strength given by the leanAmount. 
Note he won't only go in the direction the direction vector will be in addition to any 
velocity he has (eg shot backwards).
*/
BEHAVIOUR(leanInDirection)
{
  PARAMETER(leanAmount, 0.20f, float, -1.f, 1.f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV(dir, 0.00, 0.00, 1.00,  0.f, FLT_MAX); //direction to lean in
}
/*
leanRandom:  Use in addition to any balance behaviour. Will lean and head in a random direction, by a random amount. 
These change at a random time.  
 Note he won't only go in the direction the direction vector will be in addition to any 
 velocity he has (eg shot backwards).
 */
BEHAVIOUR(leanRandom)
{
  PARAMETER(leanAmountMin, 0.20f, float, 0.f, 1.f); //minimum amount of lean
  PARAMETER(leanAmountMax, 0.20f, float, 0.f, 1.f); //maximum amount of lean
  PARAMETER(changeTimeMin, 0.5f, float, 0.f, 20.f); //min time until changing direction
  PARAMETER(changeTimeMax, 1.0f, float, 0.f, 20.f); //maximum time until changing direction
}
/*
leanToPosition:  To use on any balance behaviour (eg body_balance / shot).
The character will lean and head horizontally towards a
position in space with a strength specified by the leanAmount.
*/
BEHAVIOUR(leanToPosition)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(pos, FLT_MAX); //position to head towards
}
/*
leanTowardsObject:  To use on any balance behaviour (eg body_balance / shot).
The character will lean and head horizontally towards an object defined by the level 
index in 'instanceIndex', the position of which is offseted.
*/
BEHAVIOUR(leanTowardsObject)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(offset, 100.f); //offset from instance position added when calculating position to lean to
	PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //levelIndex of object to lean towards
	PARAMETER(boundIndex, 0, int, 0, INT_MAX); //boundIndex of object to lean towards (0 = just use instance coordinates)
}
/*
hipsLeanInDirection:  Use in addition to any balance behaviour. Will lean from the hips in a specified direction
with a strength given by the leanAmount. 
*/
BEHAVIOUR(hipsLeanInDirection)
{
  PARAMETER(leanAmount, 0.20f, float, -1.f, 1.f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV(dir, 0.00, 0.00, 1.00,  0.f, FLT_MAX); //direction to lean in
}
/*
 hipsLeanRandom:  Use in addition to any balance behaviour. Will lean from the hips by a random amount and in a random direction. 
 These change at a random time.  
 */
BEHAVIOUR(hipsLeanRandom)
{
  PARAMETER(leanAmountMin, 0.30f, float, 0.f, 1.f); //minimum amount of lean
  PARAMETER(leanAmountMax, 0.40f, float, 0.f, 1.f); //maximum amount of lean
  PARAMETER(changeTimeMin, 2.0f, float, 0.f, 20.f); //min time until changing direction
  PARAMETER(changeTimeMax, 4.0f, float, 0.f, 20.f); //maximum time until changing direction
}
/*
hipsLeanToPosition:  To use on any balance behaviour (eg body_balance / shot).
The character will lean at the hips towards a
position in space with a strength specified by the leanAmount.
*/
BEHAVIOUR(hipsLeanToPosition)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(pos, FLT_MAX); //position to head towards
}
/*
hipsLeanTowardsObject:  To use on any balance behaviour (eg body_balance / shot).
The character will lean at the hips towards an object defined by the level 
index in 'instanceIndex', the position of which is offseted.
*/
BEHAVIOUR(hipsLeanTowardsObject)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(offset, 100.f); //offset from instance position added when calculating position to lean to
	PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //levelIndex of object to lean hips towards
	PARAMETER(boundIndex, 0, int, 0, INT_MAX); //boundIndex of object to lean hips towards (0 = just use instance coordinates)
}
/*
forceLeanInDirection:  Use in addition to any balance behaviour. Will head in the specified direction (pushed by a force at the hips)
with a strength given by the leanAmount. 
Note he won't only go in the direction the direction vector will be in addition to any 
velocity he has (eg shot backwards).
*/
BEHAVIOUR(forceLeanInDirection)
{
  PARAMETER(leanAmount, 0.20f, float, -1.f, 1.f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV(dir, 0.00, 0.00, 1.00,  0.f, FLT_MAX); //direction to lean in
  PARAMETER(bodyPart, 0, int, 0, 21); //body part that the force is applied to
}
/*
forceLeanRandom:  Use in addition to any balance behaviour. Will head in a random direction, by a random amount (pushed by a force at the hips). 
These change at a random time.  
 Note he won't only go in the direction the direction vector will be in addition to any 
 velocity he has (eg shot backwards).
 */
BEHAVIOUR(forceLeanRandom)
{
  PARAMETER(leanAmountMin, 0.30f, float, 0.f, 1.f); //minimum amount of lean
  PARAMETER(leanAmountMax, 0.40f, float, 0.f, 1.f); //maximum amount of lean
  PARAMETER(changeTimeMin, 2.0f, float, 0.f, 20.f); //min time until changing direction
  PARAMETER(changeTimeMax, 4.0f, float, 0.f, 20.f); //maximum time until changing direction
  PARAMETER(bodyPart, 0, int, 0, 21); //body part that the force is applied to
}
/*
forceLeanToPosition:  To use on any balance behaviour (eg body_balance / shot).
The character will head (pushed by a force at the hips) towards a
position in space with a strength specified by the leanAmount.
*/
BEHAVIOUR(forceLeanToPosition)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(pos, FLT_MAX); //position to head towards
  PARAMETER(bodyPart, 0, int, 0, 21); //body part that the force is applied to
}
/*
forceLeanTowardsObject:  To use on any balance behaviour (eg body_balance / shot).
The character will will head (pushed by a force at the hips) towards an object defined by the level 
index in 'instanceIndex', the position of which is offseted.
*/
BEHAVIOUR(forceLeanTowardsObject)
{
  PARAMETER(leanAmount, 0.20f, float, -0.5f, 0.5f); //amount of lean, 0 to about 0.5. -ve will move away from the target.
  PARAMETERV0(offset, 100.f); //offset from instance position added when calculating position to lean to
	PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //levelIndex of object to move towards
	PARAMETER(boundIndex, 0, int, 0, INT_MAX); //boundIndex of object to move towards (0 = just use instance coordinates)
  PARAMETER(bodyPart, 0, int, 0, 21); //body part that the force is applied to
}
/*
setStiffness:  Use this message to manually set the body stiffness values - 
before using Active Pose to drive to an animated pose, for example.
*/
BEHAVIOUR(setStiffness)
{
  PARAMETER(bodyStiffness, 12.00f, float, 2.f, 20.f); //stiffness of whole character
  PARAMETER(damping, 1.00f, float, 0.f, 3.f); //damping amount, less is underdamped
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
}
/*
setMuscleStiffness:  Use this message to manually set the muscle stiffness values - 
before using Active Pose to drive to an animated pose, for example.
*/
BEHAVIOUR(setMuscleStiffness)
{
  PARAMETER(muscleStiffness, 1.00f, float, 0.f, 20.f); //muscle stiffness of joint/s
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
}
/*
setWeaponMode:  Use this message to set the character's weapon mode.  This is an alternative
to the setWeaponMode public function.
*/
BEHAVIOUR(setWeaponMode)
{
  PARAMETER(weaponMode, 5, int, -1, 6); // Weapon mode. See WeaponMode enum in NmRsUtils.h.
}
/*
registerWeapon:  Use this message to register weapon.  This is an alternative
to the registerWeapon public function.
*/
BEHAVIOUR(registerWeapon)
{
  PARAMETER(hand, 1, int, 0, 1);//What hand the weapon is in. LeftHand = 0, RightHand = 1
  PARAMETER(levelIndex, -1, int, -1, INT_MAX);//Level index of the weapon
  PARAMETER(constraintHandle, -1, int, -1, INT_MAX);// pointer to the hand-gun constraint handle
  PARAMETERV(gunToHandA, 1.0f, 0.0f, 0.0f,  0.0f, FLT_MAX); // A vector of the gunToHand matrix.  The gunToHandMatrix is the desired gunToHandMatrix in the aimingPose. (The gunToHandMatrix when pointGun starts can be different so will be blended to this desired one) 
  PARAMETERV(gunToHandB, 0.0f, 1.0f, 0.0f,  0.0f, FLT_MAX); // B vector of the gunToHand matrix
  PARAMETERV(gunToHandC, 0.0f, 0.0f, 1.0f,  0.0f, FLT_MAX); // C vector of the gunToHand matrix
  PARAMETERV(gunToHandD, 0.0f, 0.0f, 0.0f,  0.0f, FLT_MAX); // D vector of the gunToHand matrix
  PARAMETERV0(gunToMuzzleInGun, FLT_MAX); // Gun centre to muzzle expressed in gun co-ordinates.  To get the line of sight/barrel of the gun. Assumption: the muzzle direction is always along the same primary axis of the gun. 
  PARAMETERV0(gunToButtInGun, FLT_MAX); // Gun centre to butt expressed in gun co-ordinates.  The gun pivots around this point when aiming
  //PARAMETER(gunToHandConstraint, NULL, const void*, NULL, NULL);//Pointer to the constraint (rage::phConstraintHandle) between the gun and the hand
}
/*
shotRelax:  This message should be called during the body shot, at the point where he is dying and should fall to the floor. 
The character will relax over a short time period, and fall to the floor.
*/
BEHAVIOUR(shotRelax)
{
  PARAMETER(relaxPeriodUpper, 2.00f, float, 0.f, 40.f); //time over which to relax to full relaxation for upper body
  PARAMETER(relaxPeriodLower, 0.40f, float, 0.f, 40.f); //time over which to relax to full relaxation for lower body
}
/*
fireWeapon:  One shot message apply a force to the hand as we fire the gun that should be in this hand
*/
BEHAVIOUR(fireWeapon)
{
  PARAMETER(firedWeaponStrength, 1000.f, float, 0.f, 10000.f); //The force of the gun.
  PARAMETER(gunHandEnum,0,int,0,1);//Which hand in the gun in, 0 = left, 1 = right.
  PARAMETER(applyFireGunForceAtClavicle, false, bool, false, true);// Should we apply some of the force at the shoulder. Force double handed weapons (Ak47 etc).
  PARAMETER(inhibitTime, 0.4f, float, 0.f, 10.f);// Minimum time before next fire impulse
  PARAMETERV0(direction, FLT_MAX); // direction of impulse in gun frame
  PARAMETER(split, 0.5f, float, 0.f, 1.f);// Split force between hand and clavicle when applyFireGunForceAtClavicle is true. 1 = all hand, 0 = all clavicle. 
  //mmmtodo add force from muzzle parameter?  At the moment it applies the force to the centre of the hand
}
/*
configureConstraints:  One shot to give state of constraints on character and response to constraints 
*/
BEHAVIOUR(configureConstraints)
{
  PARAMETER(handCuffs, false, bool, false, true);
  PARAMETER(handCuffsBehindBack, false, bool, false, true);//not implemented
  PARAMETER(legCuffs, false, bool, false, true);//not implemented
  PARAMETER(rightDominant, false, bool, false, true);
  PARAMETER(passiveMode, 0, int, 0, 5);//0 setCurrent, 1= IK to dominant, (2=pointGunLikeIK //not implemented)  
  PARAMETER(bespokeBehaviour, false, bool, false, true);//not implemented
  PARAMETER(blend2ZeroPose, 0, float, 0.0, 1.f);//Blend Arms to zero pose 
}
/*
stayUpright:  This soft constraint keeps the character on his feet (even when airborne).
Use in addition to dynamicBalancer, for better stability.
Add to balance behaviours such as shot and brace for impact if necessary.
*/
BEHAVIOUR(stayUpright)
{
  PARAMETER(useForces, false, bool, false, true); // enable force based constraint
  PARAMETER(useTorques, false, bool, false, true); // enable torque based constraint
  PARAMETER(lastStandMode, false, bool, false, true); // Uses position/orientation control on the spine and drifts in the direction of bullets.  This ignores all other stayUpright settings.
  PARAMETER(lastStandSinkRate, 0.3f, float, 0.f, 1.f); // The sink rate (higher for a faster drop).
  PARAMETER(lastStandHorizDamping, 0.4f, float, 0.f, 1.f); // Higher values for more damping
  PARAMETER(lastStandMaxTime, 0.4f, float, 0.f, 5.f); // Max time allowed in last stand mode
  PARAMETER(turnTowardsBullets, false, bool, false, true); // Use cheat torques to face the direction of bullets if not facing too far away
  PARAMETER(velocityBased, false, bool, false, true); // make strength of constraint function of COM velocity.  Uses -1 for forceDamping if the damping is positive.
  PARAMETER(torqueOnlyInAir, false, bool, false, true); // only apply torque based constraint when airBorne
  PARAMETER(forceStrength, 3.0f, float, 0.f, 16.f); // strength of constraint
  PARAMETER(forceDamping, -1.0f, float, -1.0f, 50.f); // damping in constraint: -1 makes it scale automagically with forceStrength.  Other negative values will scale this automagic damping. 
  PARAMETER(forceFeetMult, 1.0f, float, 0.0f, 1.0f); // multiplier to the force applied to the feet
  PARAMETER(forceSpine3Share, 0.3f, float, 0.0f, 1.0f); // share of pelvis force applied to spine3
  PARAMETER(forceLeanReduction, 1.0f, float, 0.0f, 1.0f); // how much the character lean is taken into account when reducing the force. 
  PARAMETER(forceInAirShare, 0.5f, float, 0.0f, 1.0f); // share of the feet force to the airborne foot
  PARAMETER(forceMin, -1.0f, float, -1.0f, 16.f); // when min and max are greater than 0 the constraint strength is determined from character strength, scaled into the range given by min and max
  PARAMETER(forceMax, -1.0f, float, -1.0f, 16.f); // see above
  PARAMETER(forceSaturationVel, 4.0f, float, 0.1f, 10.f); // when in velocityBased mode, the COM velocity at which constraint reaches maximum strength (forceStrength)
  PARAMETER(forceThresholdVel, 0.5f, float, 0.0f, 5.0f); // when in velocityBased mode, the COM velocity above which constraint starts applying forces
  PARAMETER(torqueStrength, 0.0f, float, 0.0f, 16.f); // strength of torque based constraint
  PARAMETER(torqueDamping, 0.5f, float, 0.0f, 16.f); // damping of torque based constraint
  PARAMETER(torqueSaturationVel, 4.0f, float, 0.1f, 10.f); // when in velocityBased mode, the COM velocity at which constraint reaches maximum strength (torqueStrength)
  PARAMETER(torqueThresholdVel, 2.5f, float, 0.0f, 5.0f); // when in velocityBased mode, the COM velocity above which constraint starts applying torques
  PARAMETER(supportPosition, 2.0f, float, -2.0f, 2.0f); // distance the foot is behind Com projection that is still considered able to generate the support for the upright constraint 
  PARAMETER(noSupportForceMult, 1.0f, float, 0.0f, 1.0f); // still apply this fraction of the upright constaint force if the foot is not in a position (defined by supportPosition) to generate the support for the upright constraint 
  PARAMETER(stepUpHelp, 0.0f, float, 0.0f, 16.f); // strength of cheat force applied upwards to spine3 to help the character up steps/slopes
  PARAMETER(stayUpAcc, 0.7f, float, 0.0f, 2.0f); // How much the cheat force takes into account the acceleration of moving platforms
  PARAMETER(stayUpAccMax, 5.0f, float, 0.0f, 15.f); // The maximum floorAcceleration (of a moving platform) that the cheat force takes into account
}
/*
stopAllBehaviours:  Send this message to immediately stop all behaviours from executing.
*/
BEHAVIOUR(stopAllBehaviours)
{
}
/*
setCharacterStrength:  Sets character's strength on the dead-granny-to-healthy-terminator scale: [0..1]
*/
BEHAVIOUR(setCharacterStrength)
{
  PARAMETER(characterStrength, 1.0f, float, 0.0f, 1.0f); // strength of character
}
/*
setFallingReaction:  Sets the type of reaction if catchFall is called
*/
BEHAVIOUR(setFallingReaction)
{
  PARAMETER(handsAndKnees, false, bool, false, true); // set to true to get handsAndKnees catchFall if catchFall called. If true allows the dynBalancer to stay on during the catchfall and modifies the catch fall to give a more alive looking performance (hands and knees for front landing or sitting up for back landing) 
  PARAMETER(callRDS, false, bool, false, true); // If true catchFall will call rollDownstairs if comVel>comVelRDSThresh - prevents excessive sliding in catchFall.  Was previously only true for handsAndKnees
  PARAMETER(comVelRDSThresh, 2.0f, float, 0.0f, 20.0f); //comVel above which rollDownstairs will start - prevents excessive sliding in catchFall
  PARAMETER(resistRolling, false, bool, false, true); //For rds catchFall only: True to resist rolling motion (rolling motion is set off by ub contact and a sliding velocity), false to allow more of a continuous rolling  (rolling motion is set off at a sliding velocity)
  PARAMETER(armReduceSpeed, 2.5f, float, 0.0f, 10.0f); // Strength is reduced in the catchFall when the arms contact the ground.  0.2 is good for handsAndKnees.  2.5 is good for normal catchFall, anything lower than 1.0 for normal catchFall may lead to bad catchFall poses.
  PARAMETER(reachLengthMultiplier, 1.0f, float, 0.3f, 1.0f); // Reach length multiplier that scales characters arm topological length, value in range from (0, 1> where 1.0 means reach length is maximum.
  PARAMETER(inhibitRollingTime, 0.2f, float, 0.0, 10.0); //Time after hitting ground that the catchFall can call rds
  PARAMETER(changeFrictionTime, 0.2f, float, 0.0, 10.0); //Time after hitting ground that the catchFall can change the friction of parts to inhibit sliding
  PARAMETER(groundFriction, 1.0f, float, 0.0f, 10.0f); //(8.0 was used on yanked) Friction multiplier on bodyParts when on ground.  Character can look too slidy with groundFriction = 1.  Higher values give a more jerky reation but this seems timestep dependent especially for dragged by the feet.
  PARAMETER(frictionMin, 0.0f, float, 0.0, 10.0); //Min Friction of an impact with a body part (not head, hands or feet) - to increase friction of slippy environment to get character to roll better.  Applied in catchFall and rollUp(rollDownStairs)
  PARAMETER(frictionMax, 9999.0f, float, 0.0, FLT_MAX); //Max Friction of an impact with a body part (not head, hands or feet) - to increase friction of slippy environment to get character to roll better.  Applied in catchFall and rollUp(rollDownStairs)
}
/*
setCharacterUnderwater:  Sets viscosity applied to damping limbs 
*/
BEHAVIOUR(setCharacterUnderwater)
{
	PARAMETER(underwater, false, bool, false, true); // is character underwater?
	PARAMETER(viscosity, -1.0f, float, -1.0f, 100.0f); // viscosity applied to character's parts
	PARAMETER(gravityFactor, 0.0f, float, -10.0f, 10.0f); // gravity factor applied to character
	PARAMETER(stroke, 0.0f, float, -1000.0f, 1000.0f); // swimming force applied to character as a function of handVelocity and footVelocity
	PARAMETER(linearStroke, false, bool, false, true); // swimming force (linearStroke=true,False) = (f(v),f(v*v))
}

/////////////////////////// RANGED MESSAGES (in alphabetical order) ////////////////////////////
/*
animPose:  This will take any animation currently set as the incoming transforms of the agent and drive the 
character to the pose defined therein.
You can choose to have the animPose override the Headlook/PointArm/PointGun behaviours if the animPose
mask includes (all) neck and head/ua,ul,ur/ua,ul,ur respectively.
The mask value determines which effector sets will be modified. Specifying 'fb' or no mask value will cause the effect to be applied to the whole character.
Otherwise, specify 'l' or 'u' for the first character to mask off to just lower or upper body parts. Then, you can specify the following for the second character:
When the first character is 'u', the second parameter can be:
* 'c' = just clavicles
* 'l' = left arm only
* 'r' = right arm only
* 'a' = both arms and clavicles
* 's' = just spine
* 't' = torso only, eg. spine & arms and Clavicles
* 'k' = trunk only, eg. spine & head
* 'n' = neck and head
* 'b' = everything
* 'w' = wrists
when the first character is 'l', the second parameter can be:
* 'l' = just left leg
* 'r' = just right leg
* 'b' = both legs
note that the characters should be lower-case.

TickOrderException: can overide Headlook, PointArm and PointGun.
Muscle parameters and gravity compensation can be set by animPose or by other behaviours
BodyParts:  Whole body maskable
*/
BEHAVIOUR(animPose)
{
  PARAMETER(muscleStiffness, -1.f, float, -1.1f, 10.0f); // muscleStiffness of masked joints. -values mean don't apply (just use defaults or ones applied by behaviours - safer if you are going to return to a behaviour)
  PARAMETER(stiffness, -1.f, float, -1.1f, 16.0f); // stiffness of masked joints. -ve values mean don't apply stiffness or damping (just use defaults or ones applied by behaviours).  If you are using animpose fullbody on its own then this gives the opprtunity to use setStffness and setMuscleStiffness messages to set up the character's muscles. mmmmtodo get rid of this -ve
  PARAMETER(damping, 1.f, float, 0.0f, 2.0f); // damping of masked joints
  PARAMETER(effectorMask, "ub", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see notes for explanation)
  PARAMETER(overideHeadlook, false, bool, false, true); // overide Headlook behaviour (if animPose includes the head) 
  PARAMETER(overidePointArm, false, bool, false, true); // overide PointArm behaviour (if animPose includes the arm/arms)
  PARAMETER(overidePointGun, false, bool, false, true); // overide PointGun behaviour (if animPose includes the arm/arms)//mmmmtodo not used at moment
  PARAMETER(useZMPGravityCompensation, true, bool, false, true); // If true then modify gravity compensation based on stance (can reduce gravity compensation to zero if cofm is outside of balance area)
  PARAMETER(gravityCompensation, -1.f, float, -1.f, 14.0f); // gravity compensation applied to joints in the effectorMask. If -ve then not applied (use current setting)
  PARAMETER(muscleStiffnessLeftArm, -1.f, float, -1.f, 10.0f); // muscle stiffness applied to left arm (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(muscleStiffnessRightArm, -1.f, float, -1.f, 10.0f); // muscle stiffness applied to right arm (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(muscleStiffnessSpine, -1.f, float, -1.f, 10.0f); // muscle stiffness applied to spine (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(muscleStiffnessLeftLeg, -1.f, float, -1.f, 10.0f); // muscle stiffness applied to left leg (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(muscleStiffnessRightLeg, -1.f, float, -1.f, 10.0f); // muscle stiffness applied to right leg (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(stiffnessLeftArm, -1.f, float, -1.f, 16.0f); // stiffness  applied to left arm (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(stiffnessRightArm, -1.f, float, -1.f, 16.0f); // stiffness applied to right arm (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(stiffnessSpine, -1.f, float, -1.f, 16.0f); // stiffness applied to spine (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(stiffnessLeftLeg, -1.f, float, -1.f, 16.0f); // stiffness applied to left leg (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(stiffnessRightLeg, -1.f, float, -1.f, 16.0f); // stiffness applied to right leg (applied after stiffness). If -ve then not applied (use current setting)
  PARAMETER(dampingLeftArm, 1.f, float, 0.f, 2.0f); // damping applied to left arm (applied after stiffness). If stiffness -ve then not applied (use current setting)
  PARAMETER(dampingRightArm, 1.f, float, 0.f, 2.0f); // damping applied to right arm (applied after stiffness). If stiffness -ve then not applied (use current setting)
  PARAMETER(dampingSpine, 1.f, float, 0.f, 2.0f); // damping applied to spine (applied after stiffness). If stiffness-ve then not applied (use current setting)
  PARAMETER(dampingLeftLeg, 1.f, float, 0.f, 2.0f); // damping applied to left leg (applied after stiffness). If stiffness-ve then not applied (use current setting)
  PARAMETER(dampingRightLeg, 1.f, float, 0.f, 2.0f); // damping applied to right leg (applied after stiffness). If stiffness -ve then not applied (use current setting)
  PARAMETER(gravCompLeftArm, -1.f, float, -1.f, 14.0f); // gravity compensation applied to left arm (applied after gravityCompensation). If -ve then not applied (use current setting)
  PARAMETER(gravCompRightArm, -1.f, float, -1.f, 14.0f); // gravity compensation applied to right arm (applied after gravityCompensation). If -ve then not applied (use current setting)
  PARAMETER(gravCompSpine, -1.f, float, -1.f, 14.0f); // gravity compensation applied to spine (applied after gravityCompensation). If -ve then not applied (use current setting)
  PARAMETER(gravCompLeftLeg, -1.f, float, -1.f, 14.0f); // gravity compensation applied to left leg (applied after gravityCompensation). If -ve then not applied (use current setting)
  PARAMETER(gravCompRightLeg, -1.f, float, -1.f, 14.0f); // gravity compensation applied to right leg (applied after gravityCompensation). If -ve then not applied (use current setting)
  PARAMETER(connectedLeftHand, 0, int, -1, 2); // Is the left hand constrained to the world/ an object: -1=auto decide by impact info, 0=no, 1=part fully constrained (not implemented:, 2=part point constraint, 3=line constraint)
  PARAMETER(connectedRightHand, 0, int, -1, 2); // Is the right hand constrained to the world/ an object: -1=auto decide by impact info, 0=no, 1=part fully constrained (not implemented:, 2=part point constraint, 3=line constraint)
  PARAMETER(connectedLeftFoot, -2, int, -2, 2); // Is the left foot constrained to the world/ an object: -2=do not set in animpose (e.g. let the balancer decide), -1=auto decide by impact info, 0=no, 1=part fully constrained (not implemented:, 2=part point constraint, 3=line constraint)
  PARAMETER(connectedRightFoot, -2, int, -2, 2); // Is the right foot constrained to the world/ an object: -2=do not set in animpose (e.g. let the balancer decide),-1=auto decide by impact info, 0=no, 1=part fully constrained (not implemented:, 2=part point constraint, 3=line constraint)
  PARAMETER(animSource, ART::kITSourceCurrent, int, ART::kITSourceCurrent, ART::KITSourceCount-1); // AnimSource 0 = CurrentItms, 1 = PreviousItms, 2 = AnimItms
}
/*
armsWindmill:  Arms pedaling motion with target ellipses of arbitrary orientation.
Things to know: the orientation of an ellipse is defined by rotations around the axes of its local TM, which is initially taken
to be the TM of the part whose ID is given. The x-Axis is the ellipse's normal, and y-Axis/z-Axis are the major and minor axes 
respectively. By default (or when all components of the rotation are 0) the normal of the ellipse points straight out the
side of a character. From that starting pose, first a y-rotation is applied, rotating the circle around its vertical axis, 
followed by a z-rotation, controlling how parallel to the floor the ellipse is. Finally an x-rotation twists the ellipse
around its normal. 
To simplify things a mirror-mode can be enabled such that only one orientation has to be specified. In that case,
the right ellipse will either be symmetrically mirrored from the left around the body's vertical plane (mode=1), 
or will be exactly parallel to the left (mode=2).
Also, an adaptive mode can be enabled, in which case the arms rotate depending on the character's rotation around the
somersault axis. In mode=1, only the direction of rotation changes. Mode=2 makes the speed proportional to the characters
current angular rotation and mode=3 adds modulation of arm strengths, allowing for loose limbs when character has low angular
rotation.
BodyParts:  Left and/or right arms (including clavicles)
*/
BEHAVIOUR(armsWindmill)
{
  PARAMETER(leftPartID, ART::gtaSpine3, int, 0, 21); // ID of part that the circle uses as local space for positioning
  PARAMETER(leftRadius1, 0.75f, float, 0.0f, 1.0f); // radius for first axis of ellipse
  PARAMETER(leftRadius2, 0.75f, float, 0.0f, 1.0f ); // radius for second axis of ellipse
  PARAMETER(leftSpeed, 1.0f, float, -2.0f, 2.0f); // speed of target around the circle
  PARAMETERV(leftNormal, 0.0f, 0.2f, 0.2f,  0.0f, FLT_MAX); // Euler Angles orientation of circle in space of part with part ID
  PARAMETERV(leftCentre, 0.0f, 0.5f, -0.1f,  0.0f, FLT_MAX); // centre of circle in the space of partID
  PARAMETER(rightPartID, ART::gtaSpine3, int, 0, 21);// ID of part that the circle uses as local space for positioning
  PARAMETER(rightRadius1, 0.75f, float, 0.0f, 1.0f); // radius for first axis of ellipse
  PARAMETER(rightRadius2, 0.75f, float, 0.0f, 1.0f ); // radius for second axis of ellipse
  PARAMETER(rightSpeed, 1.0f, float, -2.0f, 2.0f); // speed of target around the circle
  PARAMETERV(rightNormal, 0.0f, -0.2f, -0.2f,  0.0f, FLT_MAX); // Euler Angles orientation of circle in space of part with part ID
  PARAMETERV(rightCentre, 0.0f, -0.5f, -0.1f,  0.0f, FLT_MAX); // centre of circle in the space of partID
  PARAMETER(shoulderStiffness, 12.0f, float, 1.0f, 16.0f); //Stiffness applied to the shoulders
  PARAMETER(shoulderDamping, 1.0f, float, 0.0f, 2.0f); //Damping applied to the shoulders
  PARAMETER(elbowStiffness, 12.0f, float, 1.0f, 16.0f); //Stiffness applied to the elbows
  PARAMETER(elbowDamping, 1.0f, float, 0.0f, 2.0f); //Damping applied to the elbows
  PARAMETER(leftElbowMin, 0.5f, float, 0.0f, 1.7f); //Minimum left elbow bend
  PARAMETER(rightElbowMin, 0.5f, float, 0.0f, 1.7f); //Minimum right elbow bend
  PARAMETER(phaseOffset, 0.0f, float, -360.0f, 360.0f); // phase offset(degrees) when phase synchronization is turned on.
  PARAMETER(dragReduction, 0.2f, float, 0.0f, 1.0f); // how much to compensate for movement of character/target 
  PARAMETER(IKtwist, 0.0f, float, -PI, PI); // angle of elbow around twist axis ? 
  PARAMETER(angVelThreshold, 0.1f, float, 0.0f, 1.0f); // value of character angular speed above which adaptive arm motion starts
  PARAMETER(angVelGain, 1.0f, float, 0.0f, 10.0f); // multiplies angular speed of character to get speed of arms
  PARAMETER(mirrorMode, 1, int, 0, 2); // 0: circle orientations are independent, 1: they mirror each other, 2: they're parallel (leftArm parmeters are used)
  PARAMETER(adaptiveMode, 0, int, 0, 3); // 0:not adaptive, 1:only direction, 2: dir and speed, 3: dir, speed and strength
  PARAMETER(forceSync, true, bool, false, true); // toggles phase synchronization
  PARAMETER(useLeft, true, bool, false, true); //Use the left arm
  PARAMETER(useRight, true, bool, false, true); //Use the right arm
  PARAMETER(disableOnImpact, true, bool, false, true); //If true, each arm will stop windmilling if it hits the ground
}
/*
armsWindmillAdaptive:  Wave the arms around in a windmilling motion; this behaviour rotates the arms based on which way the character is rotating.
BodyParts:  Maskable. Left and/or right arms (including clavicles) and/or(setBackAngles) spine l1=0,l2=0,twistVariable and/or neck just twist
*/
BEHAVIOUR(armsWindmillAdaptive)
{
  PARAMETER(angSpeed, 6.28f, float, 0.1f, 10.f); //Controls the speed of the windmilling
  PARAMETER(bodyStiffness, 11.00f, float, 6.f, 16.f); //Controls how stiff the rest of the body is
  PARAMETER(amplitude, 0.60f, float, 0.f, 2.f); //Controls how large the motion is, higher values means the character waves his arms in a massive arc
  PARAMETER(phase, 0.00f, float, -4.f, 8.f); //Set to a non-zero value to desynchronise the left and right arms motion.
  PARAMETER(armStiffness, 14.14f, float, 6.f, 16.f); //How stiff the arms are controls how pronounced the windmilling motion appears; smaller values means weaker movement
  PARAMETER(leftElbowAngle, -1.f, float, -1.f, 6.f); //If not negative then left arm will blend to this angle 
  PARAMETER(rightElbowAngle, -1.f, float, -1.f, 6.f); //If not negative then right arm will blend to this angle 
	PARAMETER(lean1mult, 1.f, float, 0.f, 2.f); //0 arms go up and down at the side. 1 circles. 0..1 elipse 
	PARAMETER(lean1offset, 0.f, float, -6.f, 6.f); //0.f centre of circle at side.   
  PARAMETER(elbowRate, 1.f, float, 0.f, 6.f); //rate at which elbow tries to match *ElbowAngle   
  PARAMETER(armDirection, 0, int, -1, 1); //Arm circling direction.  -1 = Backwards, 0 = Adaptive, 1 = Forwards
  PARAMETER(disableOnImpact, true, bool, false, true); //If true, each arm will stop windmilling if it hits the ground
  PARAMETER(setBackAngles, true, bool, false, true); //If true, back angles will be set to compliment arms windmill
  PARAMETER(useAngMom, false, bool, false, true); //If true, use angular momentum about com to choose arm circling direction. Otherwise use com angular velocity
  PARAMETER(bendLeftElbow, false, bool, false, true); //If true, bend the left elbow to give a stuntman type scramble look
  PARAMETER(bendRightElbow, false, bool, false, true); //If true, bend the right elbow to give a stuntman type scramble look
  PARAMETER(mask, "ub", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
}
/*
balancerCollisionsReaction:  React to collisions in the environment
Spins if contacts walls at a shallow angle
Leans backwards against walls and tries to stabilise then slumps down wall
Leans forwars against walls and tries to push away from them
Slumps forward over tables
Rolls backwards onto tables
Falls over walls
BEHAVIOURS CALLED: BodyFoetal, Catchfall, RollDownStairs, FallOverWall, Flinch
BEHAVIOURS MODIFIED: DynamicBalancer
BodyParts:  Whole body
*/
BEHAVIOUR(balancerCollisionsReaction)
{
  PARAMETER(numStepsTillSlump, 4, int, 0, INT_MAX); //Begin slump and stop stepping after this many steps  
  PARAMETER(stable2SlumpTime, 0.0, float, 0.0, FLT_MAX); //Time after becoming stable leaning against a wall that slump starts  
  PARAMETER(exclusionZone, 0.2, float, 0.0, FLT_MAX); //Steps are ihibited to not go closer to the wall than this (after impact).
  PARAMETER(footFrictionMultStart, 1.0, float, 0.0, 4.0); //Friction multiplier applied to feet when slump starts 
  PARAMETER(footFrictionMultRate, 2.0, float, 0.0, 50.0); //Friction multiplier reduced by this amount every second after slump starts (only if character is not slumping) 
  PARAMETER(backFrictionMultStart, 1.0, float, 0.0, 4.0); //Friction multiplier applied to back when slump starts  
  PARAMETER(backFrictionMultRate, 2.0, float, 0.0, 50.0); //Friction multiplier reduced by this amount every second after slump starts (only if character is not slumping) 
  PARAMETER(impactLegStiffReduction, 3.0, float, 0.0, 16.0); //Reduce the stiffness of the legs by this much as soon as an impact is detected
  PARAMETER(slumpLegStiffReduction, 1.0, float, 0.0, 16.0); //Reduce the stiffness of the legs by this much as soon as slump starts
  PARAMETER(slumpLegStiffRate, 8.0, float, 0.0, 50.0); //Rate at which the stiffness of the legs is reduced during slump 
  PARAMETER(reactTime, 0.3, float, 0.0, 2.0); //Time that the character reacts to the impact with ub flinch and writhe
  PARAMETER(impactExagTime, 0.3, float, 0.0, 2.0); //Time that the character exaggerates impact with spine 
  PARAMETER(glanceSpinTime, 0.5, float, 0.0, 10.0); //Duration that the glance torque is applied for 
  PARAMETER(glanceSpinMag, 50.0, float, 0.0, 1000.0); //Magnitude of the glance torque
  PARAMETER(glanceSpinDecayMult, 0.3, float, 0.0, 10.0); //multiplier used when decaying torque spin over time; 4.0 = decay in ~1/4th of a second. 1.0 = decay over ~1 second.
  PARAMETER(ignoreColWithIndex, -2, int, -2, INT_MAX); //used so impact with the character that is pushing you over doesn't set off the behaviour
  PARAMETER(slumpMode, 1, int, 0, 2); //0=Normal slump(less movement then slump & movement<small), 1=fast slump, 2=less movement then slump
  PARAMETER(reboundMode, 0, int, 0, 3); //0=fall2knees/slump if shot not running, 1=stumble, 2=slump, 3=restart
  PARAMETER(ignoreColMassBelow, 20.0, float, -1.0, 1000.0); //collisions with non-fixed objects with mass below this will not set this behaviour off (e.g. ignore guns)
  PARAMETER(ignoreColVolumeBelow, 0.1, float, -1.0, 1000.0); //collisions with non-fixed objects with volume below this will not set this behaviour off  
  PARAMETER(fallOverWallDrape, true, bool, false, true); //use fallOverWall as the main drape reaction
  PARAMETER(fallOverHighWalls, false, bool, false, true); //trigger fall over wall if hit up to spine2 else only if hit up to spine1
  PARAMETER(snap, false, bool, false, true); //Add a Snap to when you hit a wall to emphasize the hit.
  PARAMETER(snapMag, -0.6f, float, -1.0f, 1.f);//The magnitude of the snap reaction 
  PARAMETER(snapDirectionRandomness, 0.3f, float, 0.0f, 1.f);//The character snaps in a prescribed way (decided by bullet direction) - Higher the value the more random this direction is.
  PARAMETER(snapLeftArm, false, bool, false, true); //snap the leftArm.
  PARAMETER(snapRightArm, false, bool, false, true); //snap the rightArm.
  PARAMETER(snapLeftLeg, false, bool, false, true); //snap the leftLeg.
  PARAMETER(snapRightLeg, false, bool, false, true); //snap the rightLeg.
  PARAMETER(snapSpine, true, bool, false, true); //snap the spine.
  PARAMETER(snapNeck, true, bool, false, true); //snap the neck.
  PARAMETER(snapPhasedLegs, true, bool, false, true); //Legs are either in phase with each other or not
  PARAMETER(snapHipType, 0, int, 0, 2);//type of hip reaction 0=none, 1=side2side 2=steplike
  PARAMETER(unSnapInterval, 0.01f, float, 0.0f, 100.f);//Interval before applying reverse snap
  PARAMETER(unSnapRatio, 0.7f, float, 0.0f, 100.f);//The magnitude of the reverse snap 
  PARAMETER(snapUseTorques, true, bool, false, true); //use torques to make the snap otherwise use a change in the parts angular velocity

  PARAMETER(impactWeaknessZeroDuration, 0.2f, float, 0.0f, 10.0f); // duration for which the character's upper body stays at minimum stiffness (not quite zero)
  PARAMETER(impactWeaknessRampDuration, 0.01f, float, 0.01f, 10.0f); // duration of the ramp to bring the character's upper body stiffness back to normal levels
  PARAMETER(impactLoosenessAmount, 1.0f, float, 0.0f, 1.0f); //how loose the character is on impact. between 0 and 1

  PARAMETER(objectBehindVictim, false, bool, false, true); // detected an object behind a shot victim in the direction of a bullet?
  PARAMETERV0(objectBehindVictimPos, FLT_MAX); // the intersection pos of a detected object behind a shot victim in the direction of a bullet
  PARAMETERV0(objectBehindVictimNormal, 1.0f); // the normal of a detected object behind a shot victim in the direction of a bullet

  FEEDBACK(event,balanceState);   // Sent when the state changes.
  FEEDBACKPARAM(balanceState, 0, int);//BalanceState
  FEEDBACKDESCR(balanceState, 0, 0, bal_Normal);//normal balancer state
  FEEDBACKDESCR(balanceState, 0, 1, bal_Impact);//an impact has occurred
  FEEDBACKDESCR(balanceState, 0, 2, bal_LeanAgainst);//Lean Against (sideways or backwards)
  FEEDBACKDESCR(balanceState, 0, 3, bal_LeanAgainstStable);//Lean Against is stable - slump now or let the game recover to animation
  FEEDBACKDESCR(balanceState, 0, 4, bal_Slump);//slide down wall or just fall over
  FEEDBACKDESCR(balanceState, 0, 5, bal_GlancingSpin);//Hit object at an angle therefore emphasize by adding some spin
  FEEDBACKDESCR(balanceState, 0, 6, bal_Rebound);//Moving away from impact - reduce strength and stop stepping soon
  FEEDBACKDESCR(balanceState, 0, 7, bal_Trip);//Used to stop stepping - e.g. after a predetermined number of steps after impact
  FEEDBACKDESCR(balanceState, 0, 8, bal_Drape);//iHit a table or something low - do a loose catchfall then rollup
  FEEDBACKDESCR(balanceState, 0, 9, bal_DrapeForward);//Hit a table or something low - do a loose catchfall then rollup
  FEEDBACKDESCR(balanceState, 0, 10, bal_DrapeGlancingSpin);//Hit a table or something low at an angle - add some spin
  FEEDBACKDESCR(balanceState, 0, 11, bal_Draped);//Triggered when balancer fails naturally during a drape or drapeGlancing spin or at end of drape (as drape can force the balancer to fail without the balancer assuming draped)
  FEEDBACKDESCR(balanceState, 0, 12, bal_End);//balancerCollisionsReaction no longer needed
}
/*
bodyBalance:  This provides a generic balancing behaviour; the lower body takes care of stepping and stabilizing the character, while the upper body provides a standard swinging-arms motion. This behaviour is zero-pose aware and will revert back to the pose (if set) when stable / unmoving.

BEHAVIOURS CALLED: Catchfall. Headlook, 
BEHAVIOURS REFERENCED: BalancerCollisionsReaction
BEHAVIOURS MODIFIED: DynamicBalancer
BodyParts:  Whole body
*/
BEHAVIOUR(bodyBalance)
{     
  PARAMETER(armStiffness, 9.0, float, 6.0, 16.0); //NB. WAS m_bodyStiffness ClaviclesStiffness=9.0f
  PARAMETER(elbow, 0.9, float, 0.0, 4.0); //How much the elbow swings based on the leg movement
  PARAMETER(shoulder, 1.00, float, 0.0, 4.0); //How much the shoulder(lean1) swings based on the leg movement 
  PARAMETER(armDamping, 0.7, float, 0.0, 2.0); //NB. WAS m_damping NeckDamping=1 ClaviclesDamping=1
  PARAMETER(useHeadLook, false, bool, false, true); //enable and provide a look-at target to make the character's head turn to face it while balancing
  PARAMETERV0(headLookPos, FLT_MAX); //position of thing to look at; world-space if instance index = -1, otherwise local-space to that object
  PARAMETER(headLookInstanceIndex, -1, int, -1, INT_MAX); //level index of thing to look at
  PARAMETER(spineStiffness, 10.0, float, 6.0, 16.0);
  PARAMETER(somersaultAngle, 1.0, float, 0.0, 2.0); //multiplier of the somersault 'angle' (lean forward/back) for arms out (lean2) 
  PARAMETER(somersaultAngleThreshold, 0.25, float, 0.0, 10.0); //Amount of somersault 'angle' before m_somersaultAngle is used for ArmsOut. Unless drunk - DO NOT EXCEED 0.8
  PARAMETER(sideSomersaultAngle, 1.0, float, 0.0, 10.0); //Amount of side somersault 'angle' before sideSomersault is used for ArmsOut. Unless drunk - DO NOT EXCEED 0.8
  PARAMETER(sideSomersaultAngleThreshold, 0.25, float, 0.0, 10.0);
  PARAMETER(backwardsAutoTurn, false, bool, false, true); //Automatically turn around if moving backwards
  PARAMETER(backwardsArms, false, bool, false, true); //Bend elbows, relax shoulders and inhibit spine twist when moving backwards
  PARAMETER(blendToZeroPose, false, bool, false, true); //Blend upper body to zero pose as the character comes to rest. If false blend to a stored pose 
  PARAMETER(armsOutOnPush, true, bool, false, true); //Put arms out based on lean2 of legs, or angular velocity (lean or twist), or lean (front/back or side/side)
  PARAMETER(armsOutOnPushMultiplier, 1.0, float, 0.0, 2.0); //Arms out based on lean2 of the legs to simulate being pushed
  PARAMETER(armsOutOnPushTimeout, 1.1, float, 0.0, 2.0); //number of seconds before turning off the armsOutOnPush response only for Arms out based on lean2 of the legs (NOT for the angle or angular velocity) 
  PARAMETER(returningToBalanceArmsOut, 0.0, float, 0.0, 1.0); //range 0:1 0 = don't raise arms if returning to upright position, 0.x = 0.x*raise arms based on angvel and 'angle' settings, 1 = raise arms based on angvel and 'angle' settings 
  PARAMETER(armsOutStraightenElbows, 0.0, float, 0.0, 1.0); //multiplier for straightening the elbows based on the amount of arms out(lean2) 0 = dont straighten elbows. Otherwise straighten elbows proportionately to armsOut
  PARAMETER(armsOutMinLean2, -9.9, float, -10.0, 0.0); // Minimum desiredLean2 applied to shoulder (to stop arms going above shoulder height or not)
  PARAMETER(spineDamping, 1.0, float, 0.0, 2.0);
  PARAMETER(useBodyTurn, true, bool, false, true);
  PARAMETER(elbowAngleOnContact, 1.9, float, 0.0, 3.0); //on contact with upperbody the desired elbow angle is set to at least this value
  PARAMETER(bendElbowsTime, 0.3, float, 0.0, 2.0); //Time after contact (with Upper body) that the min m_elbowAngleOnContact is applied
  PARAMETER(bendElbowsGait, 0.7, float, -3.0, 3.0); //Minimum desired angle of elbow during non contact arm swing
  PARAMETER(hipL2ArmL2, 0.3, float, 0.0, 1.0); //mmmmdrunk = 0.2 multiplier of hip lean2 (star jump) to give shoulder lean2 (flapping) 
  PARAMETER(shoulderL2, 0.5, float, -3.0, 3.0); //mmmmdrunk = 0.7 shoulder lean2 offset
  PARAMETER(shoulderL1, 0.0, float, -1.0, 2.0); //mmmmdrunk 1.1 shoulder lean1 offset (+ve frankenstein)
  PARAMETER(shoulderTwist, -0.35, float, -3.0, 3.0); //mmmmdrunk = 0.0 shoulder twist  
  PARAMETER(headLookAtVelProb, -1.0, float, -1.0, 1.0); //Probability [0-1] that headLook will be looking in the direction of velocity when stepping
  PARAMETER(turnOffProb, 0.1, float, 0.0, 1.0); //Weighted Probability that turn will be off. This is one of six turn type weights.
  PARAMETER(turn2VelProb, 0.3, float, 0.0, 1.0); //Weighted Probability of turning towards velocity. This is one of six turn type weights.
  PARAMETER(turnAwayProb, 0.15, float, 0.0, 1.0); //Weighted Probability of turning away from headLook target. This is one of six turn type weights.
  PARAMETER(turnLeftProb, 0.125, float, 0.0, 1.0); //Weighted Probability of turning left. This is one of six turn type weights.
  PARAMETER(turnRightProb, 0.125, float, 0.0, 1.0); //Weighted Probability of turning right. This is one of six turn type weights.
  PARAMETER(turn2TargetProb, 0.2, float, 0.0, 1.0); //Weighted Probability of turning towards headLook target. This is one of six turn type weights.
  PARAMETERV(angVelMultiplier, 4.0f,1.0f,4.0f,  0.0, 20.0); //(somersault, twist, sideSomersault) multiplier of the angular velocity  for arms out (lean2) (somersault, twist, sideSomersault) 
  PARAMETERV(angVelThreshold, 1.2f,3.0f,1.2f,  0.0, 40.0); //(somersault, twist, sideSomersault) threshold above which angVel is used for arms out (lean2) Unless drunk - DO NOT EXCEED 7.0 for each component   
}
/*
bodyFoetal:  Curls into foetal position, at a speed defined by the strength and damping values; This behaviour is full-body and resets the character when it starts.
BodyParts:  Whole body maskable
*/
BEHAVIOUR(bodyFoetal)
{
  PARAMETER(stiffness, 9.00f, float, 6.f, 16.f); //The stiffness of the body determines how fast the character moves into the position, and how well that they hold it.
  PARAMETER(dampingFactor, 1.40f, float, 0.f, 2.f); //Sets damping value for the character joints
  PARAMETER(asymmetry, 0.00f, float, 0.f, 1.f); //A value between 0-1 that controls how asymmetric the results are by varying stiffness across the body
  PARAMETER(randomSeed, 100, int, 0, INT_MAX); //Random seed used to generate asymmetry values
  PARAMETER(backTwist, 0.00f, float, 0.f, 1.f); //Amount of random back twist to add
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
}
/*
bodyRollUp:  Rolling motion. Apply when the character is to be tumbling, either down a hill, after being blown by an explosion, or hit by a car.
The character is in a rough foetal position, he puts his arms out to brace against collisions with the ground, and he will relax after he stops tumbling.
sends success feedback: "Roll Up: stopped rolling" when rolling has stopped
BEHAVIOURS REFERENCED: BraceForImpact, RollDownStairs
BodyParts:  Whole body maskable
*/
BEHAVIOUR(bodyRollUp)
{
  PARAMETER(stiffness, 10.00f, float, 6.f, 16.f); //stiffness of whole body
  PARAMETER(useArmToSlowDown, 1.30f, float, -2.f, 3.f); //the degree to which the character will try to stop a barrel roll with his arms
  PARAMETER(armReachAmount, 1.40f, float, 0.f, 3.f); //the likeliness of the character reaching for the ground with its arms
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
  PARAMETER(legPush, 0.00f, float, -1.f, 2.f); //used to keep rolling down slope, 1 is full (kicks legs out when pointing upwards)
  PARAMETER(asymmetricalLegs, 0.00f, float, -2.f, 2.f); //0 is no leg asymmetry in 'foetal' position.  greater than 0 a asymmetricalLegs-rand(30%), added/minus each joint of the legs in radians.  Random number changes about once every roll.  0.4 gives a lot of asymmetry
  PARAMETER(noRollTimeBeforeSuccess, 0.5f, float, 0.0f, 2.0f); // time that roll velocity has to be lower than rollVelForSuccess, before success message is sent
  PARAMETER(rollVelForSuccess, 0.2f, float, 0.0f, 1.0f); // lower threshold for roll velocity at which success message can be sent
  PARAMETER(rollVelLinearContribution, 1.0f, float, 0.0f, 1.0f); // contribution of linear COM velocity to roll Velocity (if 0, roll velocity equal to COM angular velocity)
  FEEDBACK(success,this);//Sent when the character comes to a rest. 
}
/*
bodyWrithe:  simple writhe behaviour, works under any initial state
BodyParts:  Whole body (except ankles/wrists) maskable
Can blend with output from other behaviours
*/
BEHAVIOUR(bodyWrithe)
{
  PARAMETER(armStiffness, 13.00f, float, 6.f, 16.f); 
  PARAMETER(backStiffness, 13.00f, float, 6.f, 16.f); 
  PARAMETER(legStiffness, 13.00f, float, 6.f, 16.f); //The stiffness of the character will determine how 'determined' a writhe this is - high values will make him thrash about wildly
  PARAMETER(armDamping, 0.50f, float, 0.f, 3.f); //damping amount, less is underdamped
  PARAMETER(backDamping, 0.50f, float, 0.f, 3.f); //damping amount, less is underdamped
  PARAMETER(legDamping, 0.50f, float, 0.f, 3.f); //damping amount, less is underdamped
  PARAMETER(armPeriod, 1.00f, float, 0.f, 4.f); //Controls how fast the writhe is executed, smaller values make faster motions
  PARAMETER(backPeriod, 1.00f, float, 0.f, 4.f); //Controls how fast the writhe is executed, smaller values make faster motions
  PARAMETER(legPeriod, 1.00f, float, 0.f, 4.f); //Controls how fast the writhe is executed, smaller values make faster motions
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
  PARAMETER(armAmplitude, 1.00f, float, 0.f, 3.f); 
  PARAMETER(backAmplitude, 1.00f, float, 0.f, 3.f); //scales the amount of writhe. 0 = no writhe
  PARAMETER(legAmplitude, 1.00f, float, 0.f, 3.f); //scales the amount of writhe. 0 = no writhe
  PARAMETER(elbowAmplitude, 1.00f, float, 0.f, 3.f); 
  PARAMETER(kneeAmplitude, 1.00f, float, 0.f, 3.f); 
  PARAMETER(rollOverFlag, false, bool, false, true); //Flag to set trying to rollOver
  PARAMETER(blendArms, 1.f, float, 0.f, 1.f); //Blend the writhe arms with the current desired arms (0=don't apply any writhe, 1=only writhe) 
  PARAMETER(blendBack, 1.f, float, 0.f, 1.f); //Blend the writhe spine and neck with the current desired (0=don't apply any writhe, 1=only writhe)
  PARAMETER(blendLegs, 1.f, float, 0.f, 1.f); //Blend the writhe legs with the current desired legs (0=don't apply any writhe, 1=only writhe)
  PARAMETER(applyStiffness, true, bool, false, true); //Use writhe stiffnesses if true. If false don't set any stiffnesses 
}
/*
braceForImpact:  bracing for an impact, specifically from a slow moving car.
The character is expected to be upright and in a roughly balanced position. He will turn to face the object that he is to brace against. He will take corrective steps if he unbalances and will also catch his fall if he falls over.
The character will crouch slightly and lean into the impact, and if the impact object starts to move away, the character will righten itself and its upper body will assume the zero pose.

BEHAVIOURS CALLED: Catchfall, Grab. Headlook, Pedal, RollDownStairs, SpineTwist.
BEHAVIOURS MODIFIED: DynamicBalancer
BodyParts:  Whole body
*/
BEHAVIOUR(braceForImpact)
{
  PARAMETER(braceDistance, 0.5f, float, 0.0f, 1.0f); //distance from object at which to raise hands to brace 0.5 good if newBrace=true - otherwise 0.65
  PARAMETER(targetPredictionTime, 0.45f, float, 0.0f, 1.0f); //time epected to get arms up from idle
  PARAMETER(reachAbsorbtionTime, 0.15f, float, 0.0f, 1.0f); //larger values and he absorbs the impact more
  PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //levelIndex of object to brace
  PARAMETER(bodyStiffness, 12.0f, float, 6.0f, 16.0f); //stiffness of character. catch_fall stiffness scales with this too, with its defaults at this values default 
  PARAMETER(grabDontLetGo, false, bool, false, true); //Once a constraint is made, keep reaching with whatever hand is allowed
  PARAMETER(grabStrength, 40.0f, float, -1.0f, 1000.0f); //strength in hands for grabbing (kg m/s), -1 to ignore/disable
  PARAMETER(grabDistance, 2.0f, float, 0.0f, 4.0f); //Relative distance at which the grab starts.
  PARAMETER(grabReachAngle, 1.5f, float, 0.0f, 3.16f); //Angle from front at which the grab activates. If the point is outside this angle from front will not try to grab.
  PARAMETER(grabHoldTimer, 2.5f, float, 0.0f, 10.0f); //amount of time, in seconds, before grab automatically bails
  PARAMETER(maxGrabCarVelocity, 95.0f, float, 0.0f, 1000.0f); //Don't try to grab a car moving above this speed mmmmtodo make this the relative velocity of car to character?
  PARAMETER(legStiffness, 12.0f, float, 6.0f, 16.0f); //Balancer leg stiffness mmmmtodo remove this parameter and use configureBalance?
  PARAMETER(timeToBackwardsBrace, 1.0f, float, 0.0f, 10.0f); //time before arm brace kicks in when hit from behind
  PARAMETERV0(look, FLT_MAX); //position to look at, e.g. the driver
  PARAMETERV0(pos, FLT_MAX); //location of the front part of the object to brace against. This should be the centre of where his hands should meet the object
  PARAMETER(minBraceTime, 0.3f, float, 0.0f, 3.0f); //minimum bracing time so the character doesn't look twitchy 
  PARAMETER(handsDelayMin, 0.1f, float, 0.0f, 3.0f); //If bracing with 2 hands delay one hand by at least this amount of time to introduce some asymmetry.
  PARAMETER(handsDelayMax, 0.3f, float, 0.0f, 3.0f); //If bracing with 2 hands delay one hand by at most this amount of time to introduce some asymmetry.
  PARAMETER(moveAway, false, bool, false, true); //move away from the car (if in reaching zone)
  PARAMETER(moveAwayAmount, 0.1f, float, -1.0f, 1.0f); //forceLean away amount (-ve is lean towards)
  PARAMETER(moveAwayLean, 0.05f, float, -0.5, 0.5); //Lean away amount (-ve is lean towards)
  PARAMETER(moveSideways, 0.3f, float, 0.0f, 10.0f); //Amount of sideways movement if at the front or back of the car to add to the move away from car 
  PARAMETER(bbArms, false, bool, false, true); //Use bodyBalance arms for the default (non bracing) behaviour if bodyBalance is active
  PARAMETER(newBrace, true, bool, false, true); //Use the new brace prediction code 
  PARAMETER(braceOnImpact, false, bool, false, true); //If true then if a shin or thigh is in contact with the car then brace. NB: newBrace must be true.  For those situations where the car has pushed the ped backwards (at the same speed as the car) before the behaviour has been started and so doesn't predict an impact.
  PARAMETER(roll2Velocity, false, bool, false, true); //When rollDownStairs is running use roll2Velocity to control the helper torques (this only attempts to roll to the chaarcter's velocity not some default linear velocity mag 
  PARAMETER(rollType, 3, int, 0, 3); //0 = original/roll off/stay on car:  Roll with character velocity, 1 = //Gentle: roll off/stay on car = use relative velocity of character to car to roll against, 2 = //roll over car:  Roll against character velocity.  i.e. roll against any velocity picked up by hitting car, 3 = //Gentle: roll over car:  use relative velocity of character to car to roll with 
}
/*
buoyancy:  Simple buoyancy model.  No character movement just fluid forces/torques added to parts.
*/
BEHAVIOUR(buoyancy)
{
  PARAMETERV0(surfacePoint, FLT_MAX); // Arbitrary point on surface of water.
  PARAMETERV(surfaceNormal, 0,0,1,  0.0, FLT_MAX); // Normal to surface of water.
  PARAMETER(buoyancy, 1.f, float, 0.f, FLT_MAX); // Buoyancy multiplier.
  PARAMETER(chestBuoyancy, 8.f, float, 0.f, FLT_MAX); // Buoyancy mulplier for spine2/3. Helps character float upright.
  PARAMETER(damping, 40.f, float, 0.f, FLT_MAX); // Damping for submerged parts.
  PARAMETER(righting, true, bool, false, true); // Use righting torque to being character face-up in water?
  PARAMETER(rightingStrength, 25.f, float, 0.f, FLT_MAX); // Strength of righting torque.
  PARAMETER(rightingTime, 1.f, float, 0.f, FLT_MAX);  // How long to wait after chest hits water to begin righting torque.
} 
/*
catchFall:  the character catches his fall when falling over. 
He will twist his spine and look at where he is falling. He will also relax after hitting the ground.
He always braces against a horizontal ground.
BEHAVIOURS CALLED: SpineTwist, HeadLook, RollDownStairs(if handsAndKnees (health>0.7)) 
BEHAVIOURS MODIFIED: DynamicBalancer if handsAndKnees (health>0.7)
BEHAVIOURS REFERENCED: BalancerCollisionsReaction, Teeter
BodyParts:  Whole body maskable
*/
BEHAVIOUR(catchFall)
{
  PARAMETER(torsoStiffness, 9.0, float, 6.0, 16.0); //stiffness of torso
  PARAMETER(legsStiffness, 6.0, float, 4.0, 16.0); //stiffness of legs
  PARAMETER(armsStiffness, 15.0, float, 6.0, 16.0); //stiffness of arms
  PARAMETER(backwardsMinArmOffset, -0.25, float, -1.0, 0.0); //0 will prop arms up near his shoulders. -0.3 will place hands nearer his behind
  PARAMETER(forwardMaxArmOffset, 0.35, float, 0.0, 1.0); //0 will point arms down with angled body, 0.45 will point arms forward a bit to catch nearer the head
  PARAMETER(zAxisSpinReduction, 0.0, float, 0.0, 1.0); //Tries to reduce the spin around the Z axis. Scale 0 - 1.
  PARAMETER(useHeadLook, true, bool, false, true); //Toggle to use the head look in this behaviour.
  PARAMETER(mask, "fb", char *, "", ""); //Two character body-masking value, bitwise joint mask or bitwise logic string of two character body-masking value  (see Active Pose notes for possible values)
  FEEDBACK(finish,this);//Sent when the character has come to a stop and is fully relaxed. 
  FEEDBACK(success,this);//When the fall has been caught. 
}
#if ALLOW_DEBUG_BEHAVIOURS
/*
debugRig:  NB.  Debug only behaviour - Cycles through the effectors and moves them from 2*(min to max)
*/
BEHAVIOUR(debugRig)
{
  PARAMETER(muscleStiffness, 4.00f, float, 0.1f, 10.f); //The muscle stiffness of the body determines how fast the character moves into the position, and how well that they hold it.
  PARAMETER(stiffness, 13.00f, float, 6.f, 16.f); //The stiffness of the body determines how fast the character moves into the position, and how well that they hold it.
  PARAMETER(damping, 1.0f, float, 0.f, 2.f); //Sets damping value for the character joints
  PARAMETER(speed, 0.5f, float, 0.001f, 2.f); //speed to increase angle
  PARAMETER(joint, -1, int, -1, 30); //joint to move - if =-1 cycle through all joints
}
#if ART_ENABLE_BSPY
/*
debugSkeleton:  Debug only behaviour - Configures skeleton debug draw
*/
BEHAVIOUR(debugSkeleton)
{
  PARAMETER(mode, 1, int, 0, 2);        // Draw mode.  0 = off, 1 = desired, 2 = actual
  PARAMETER(root, 0, int, 0, INT_MAX);  // Desired root part of the skeleton, eg. where desired and actual should coincide
  PARAMETER(mask, "fb", char *, "", "");// Parts to draw.  Keep this to a minimum for non-crap bSpy performance.
}
#endif//ART_ENABLE_BSPY
#endif //ALLOW_DEBUG_BEHAVIOURS
/*
dragged:  This provides a dragged behaviour.  The character is constrained to an object by a rope and climbs along the rope.  
Climbing can be input controlled or automatic 

BEHAVIOURS CALLED: Pedal.
BodyParts:  Whole body
*/
BEHAVIOUR(dragged)
{
  PARAMETER(armStiffness, 10.0, float, 6.0, 16.0); //stiffness of arms.
  PARAMETER(armDamping, 0.8, float, 0.0, 2.0); //Sets damping value for the arms
  PARAMETER(armMuscleStiffness, 7.0, float, 0.0, 10.0); //Sets musclestiffness value for the arms
  PARAMETER(radiusTolerance, 0.07, float, 0.0, 2.0); //Hands within this radius of the rope will grab it
  PARAMETER(ropeAttachedToInstance, -1, int, -1, INT_MAX); //Instance of object rope is attached to
  PARAMETERV0(ropePos, FLT_MAX); //position (on instance) the rope is attached to 
  PARAMETER(ropedBodyPart, -1, int, -1, 21); //Starting constraint to body part. If -1 then right hand is selected
  PARAMETER(ropeTaut, true, bool, false, true); //if the rope is taut the character will try to grab the rope 
  PARAMETER(playerControl, false, bool, false, true); //player control of this behaviour 
  PARAMETER(grabLeft, true, bool, false, true); //player wants to grab with left hand
  PARAMETER(grabRight, true, bool, false, true); //player wants to grab with right hand
  PARAMETER(lengthTolerance, 0.1, float, 0.0, 2.0); //How close along rope towards hands from the rope target the hand will grab 
  PARAMETER(armTwist, 0.25, float, -6.0, 6.0); //Twist in IK of reach
  PARAMETER(reach, 0.6, float, 0.0, 8.0); //Radius of semisphere of reach from shoulder the target will be within
  FEEDBACK(event,this);//gives feedback when the state of the rope constraints has changed: (bool LeftHandHasGrabbed, bool RightHandHasGrabbed, int bodyPartNumberOfUpperConstraint - either the hands or ropedBodyPart) 
}
/*
electrocute:  E.g. by a tazer.  Provides a vibration of bodyParts
BEHAVIOURS MODIFIED: Catchfall
BEHAVIOURS REFERENCED: Catchfall
BodyParts:  Whole body - selectable using parameters
*/
BEHAVIOUR(electrocute)
{
  PARAMETER(tazeMag, 0.25f, float, 0.0f, 1.0f);//The magnitude of the reaction 
  PARAMETER(initialMult, 1.0f, float, 0.0f, 20.0f);//initialMult*tazeMag = The magnitude of the 1st snap reaction (other mults are applied after this)
  PARAMETER(largeMult, 1.0f, float, 0.0f, 20.f);//largeMult*tazeMag = The magnitude of a random large snap reaction (other mults are applied after this)
  PARAMETER(largeMinTime, 1.0f, float, 0.0f, 200.0f);//min time to next large random snap (about 14 snaps with tazeInterval = 0.07s) 
  PARAMETER(largeMaxTime, 2.0f, float, 0.0f, 200.0f);//max time to next large random snap (about 28 snaps with tazeInterval = 0.07s)
  PARAMETER(movingMult, 1.0f, float, 0.0f, 20.0f);//movingMult*tazeMag = The magnitude of the reaction if moving(comVelMag) faster than movingThresh
  PARAMETER(balancingMult, 1.0f, float, 0.0f, 20.0f);//balancingMult*tazeMag = The magnitude of the reaction if balancing = (not lying on the floor/ not upper body not collided) and not airborne 
  PARAMETER(airborneMult, 1.0f, float, 0.0f, 20.0f);//airborneMult*tazeMag = The magnitude of the reaction if airborne 
  PARAMETER(movingThresh, 1.0f, float, 0.0f, 20.0f);//If moving(comVelMag) faster than movingThresh then mvingMult applied to tazeMag   
  PARAMETER(tazeInterval, 0.07f, float, 0.0f, 10.0f);//Direction flips every tazeInterval
  PARAMETER(directionRandomness, 0.3f, float, 0.0f, 1.0f);//The character vibrates in a prescribed way - Higher the value the more random this direction is.
  PARAMETER(leftArm, true, bool, false, true); //vibrate the leftArm.
  PARAMETER(rightArm, true, bool, false, true); //vibrate the rightArm.
  PARAMETER(leftLeg, true, bool, false, true); //vibrate the leftLeg.
  PARAMETER(rightLeg, true, bool, false, true); //vibrate the rightLeg.
  PARAMETER(spine, true, bool, false, true); //vibrate the spine.
  PARAMETER(neck, true, bool, false, true); //vibrate the neck.
  PARAMETER(phasedLegs, true, bool, false, true); //Legs are either in phase with each other or not
  PARAMETER(applyStiffness, true, bool, false, true); //let electrocute apply a (higher generally) stiffness to the character whilst being vibrated
  PARAMETER(useTorques, true, bool, false, true); //use torques to make vibration otherwise use a change in the parts angular velocity
  PARAMETER(hipType, 2, int, 0, 2);//type of hip reaction 0=none, 1=side2side 2=steplike
}
/*
fallOverWall:  A stunt-man style behaviour used to encourage the character to flip himself over waist-high-or-lower wall obstacles. This uses a pair of cheat forces, 
one to get the character lifted over the wall, another to provide twist to the spine as he does so to make the flip look less artificial. 
The behaviour waits for collision with the spine or lower arms/hands to determine which point to flip over, or a custom point can be specified in the message instead.
Works in conjunction with a BodyBalance and a leanInDirection/leanToPosition/leanTowardsObject.
BEHAVIOURS CALLED: DynamicBalancer, BalancerCollisionsReaction, #if useNewFallOverWall-Catchfall
BodyParts:  Whole body - selectable using moveArms, moveLegs, bendSpine

*/
BEHAVIOUR(fallOverWall)
{
  PARAMETER(bodyStiffness, 9.00f, float, 6.0f, 16.0f); //stiffness of the body, roll up stiffness scales with this and defaults at this default value
  PARAMETER(damping, 0.50f, float, 0.0f, 3.0f); //Damping in the effectors
  PARAMETER(magOfForce, 0.5f, float, 0.0f, 2.0f); //Magnitude of the falloverWall helper force
  PARAMETER(maxDistanceFromPelToHitPoint, 0.25f, float, 0.01f, 1.0f); //The maximum distance away from the pelvis that hit points will be registered.
  PARAMETER(maxForceDist, 0.8f, float, 0.01f, 2.0f); // maximum distance between hitPoint and body part at which forces are applied to part
  PARAMETER(stepExclusionZone, 0.5f, float, 0.01f, 2.0f); // Specifies extent of area in front of the wall in which balancer won't try to take another step
  PARAMETER(minLegHeight, 0.4f, float, 0.1f, 2.0f); //minimum height of pelvis above feet at which fallOverWall is attempted
  PARAMETER(bodyTwist, 0.54f, float, 0.0f, 1.0f); //amount of twist to apply to the spine as the character tries to fling himself over the wall, provides more of a believable roll but increases the amount of lateral space the character needs to successfully flip. 
  PARAMETER(maxTwist, PI, float, 0.0f, 10.0f); //max angle the character can twist before twsit helper torques are turned off
  PARAMETERV0(fallOverWallEndA, FLT_MAX); //One end of the wall to try to fall over.
  PARAMETERV0(fallOverWallEndB, FLT_MAX); //One end of the wall over which we are trying to fall over.
  PARAMETER(forceAngleAbort, -0.2f, float, -FLT_MAX, FLT_MAX); //The angle abort threshold.
  PARAMETER(forceTimeOut, 2.0f, float, -FLT_MAX, FLT_MAX); //The force time out.
  PARAMETER(moveArms, true, bool, false, true); //Lift the arms up if true.  Do nothing with the arms if false (eg when using catchfall arms or brace etc) 
  PARAMETER(moveLegs, true, bool, false, true); //Move the legs if true.  Do nothing with the legs if false (eg when using dynamicBalancer etc)  
  PARAMETER(bendSpine, true, bool, false, true); //Bend spine to help falloverwall if true.  Do nothing with the spine if false.
  PARAMETER(angleDirWithWallNormal, 180.0f, float, 0.0f, 180.0f); //Maximum angle in degrees (between the direction of the velocity of the COM and the wall normal) to start to apply forces and torques to fall over the wall.
  PARAMETER(leaningAngleThreshold, 180.0f, float, 0.0f, 180.0f);//Maximum angle in degrees (between the vertical vector and a vector from pelvis to lower neck) to start to apply forces and torques to fall over the wall.
  PARAMETER(maxAngVel, 2.0f, float, -1.0f, 30.0f); //if the angular velocity is higher than maxAngVel, the torques and forces are not applied.
  PARAMETER(adaptForcesToLowWall, false, bool, false, true); //Will reduce the magnitude of the forces applied to the character to help him to fall over wall
  PARAMETER(maxWallHeight, -1.0f, float, -1.0f, 3.0f); //Maximum height (from the lowest foot) to start to apply forces and torques to fall over the wall.
  PARAMETER(distanceToSendSuccessMessage, -1.0f, float, -1.0f, 3.0f); //Minimum distance between the pelvis and the wall to send the success message. If negative doesn't take this parameter into account when sending feedback.
  PARAMETER(rollingBackThr, 0.5f, float, 0.0f, 10.0f);//Value of the angular velocity about the wallEgde above which the character is considered as rolling backwards i.e. goes in to fow_RollingBack state
  PARAMETER(rollingPotential, 0.3f, float, -1.0f, 10.0f);//On impact with the wall if the rollingPotential(calculated from the characters linear velocity w.r.t the wall) is greater than this value the character will try to go over the wall otherwise it won't try (fow_Aborted).
#if useNewFallOverWall
  PARAMETER(useArmIK, false, bool, false, true); //Try to reach the wallEdge. To configure the IK : use limitAngleBack, limitAngleFront and limitAngleTotallyBack.
  PARAMETER(reachDistanceFromHitPoint, 0.3f, float, 0.0f, 1.0f); //distance from predicted hitpoint where each hands will try to reach the wall.
  PARAMETER(minReachDistanceFromHitPoint, 0.1f, float, 0.0f, 1.0f); //minimal distance from predicted hitpoint where each hands will try to reach the wall. Used if the hand target is outside the wall Edge.
  PARAMETER(angleTotallyBack, 15.0f, float, 0.0f, 180.0f); //max angle in degrees (between 1.the vector between two hips and 2. wallEdge) to try to reach the wall just behind his pelvis with his arms when the character is back to the wall.
#endif//useNewFallOverWall
  FEEDBACK(success,fallOverWall);//Sent when the character is over the wall (Pelvis on opposite side of the wall edge and below edge) fallOverWallState = fow_OverTheWall.
  FEEDBACK(failure,fallOverWall);//Sent when the character decides not to try to go over the wall on impact (fow_Aborted), is stuck on the wall(fow_StuckOnWall) or is falling back to the original side of the wall(bal_RollingBack). 
  FEEDBACK(event,fallOverWallState);//Sent when the state changes.
  FEEDBACKPARAM(fallOverWallState, 0, int);//fallOverWallState
  FEEDBACKDESCR(fallOverWallState, 0, 0, fow_ApproachingWall);//Character has not hit the wall yet or is still thinking about going over (doesn't mean the character is necessarily moving towards the wall)
  FEEDBACKDESCR(fallOverWallState, 0, 1, fow_Aborted);//Not enough speed on impact to give a good roll.
  FEEDBACKDESCR(fallOverWallState, 0, 2, fow_RollingOverWall);//Currently trying to get overTheWall
  FEEDBACKDESCR(fallOverWallState, 0, 3, fow_OverTheWall);//Success - Pelvis on opposite side of the wall edge and below edge (Is over but may be falling down the side of the wall still)
  FEEDBACKDESCR(fallOverWallState, 0, 4, fow_StuckOnWall);//Character has come to a halt on the top of the wall probably
  FEEDBACKDESCR(fallOverWallState, 0, 5, bal_RollingBack);//falling back to the original side of the wall
}
/*
grab:  NOTE: An upper body behaviour.
An upper body behaviour that Grabs or Braces against a point, the nearest point on a line or the nearest point on a surface.  Turns head, twists body and pelvis. Set useleft and useright to use the desired hands. Grab will create a constrant between the hand and a object. Brace will not create any constraint. 
This behaviour can be played over other behaviours. However it includes logic about if and when to grab, so will not grab things behind the character or things that are out of reach.  
There are 3 ways to specify the grab points.
Points: right grab/brace point (pos1) and/or left grab/brace point(pos2).
Line: grabs/braces with the left and/or right hand to the nearest point on the line between (pos1) and (pos2). 
Quad Surface: grabs/braces to the surface specified by (pos1), (pos2), (pos3) and (pos4). These points must be specified in a anitclockwise order.
The normals ( right/left are normal(normalR)/normal(normalL) respectively) can be specified for all of the grab point input methods. If no normal is specified the behaviour will attempt to find the appropriate normal.
The grab points are specified in the coord frame of the instance specified by instanceIndex. ( -1 = world space).
PullUp: setting a pull up strength , pullUpStrength, result in the arms trying to pull relative to this strength over a time pullUpTime. 0 = no attempt to pull up. 1 = attemp to pull up the maximum amount.

BEHAVIOURS CALLED: SpineTwist, HeadLook
BEHAVIOURS MODIFIED: DynamicBalancer (turns and leans)
BodyParts:  UpperBody:  head/neck controlled by useHeadLookToTarget, Arms by useLeft, useRight
*/
BEHAVIOUR(grab)
{
  PARAMETER(useLeft, false, bool, false, true);//  Flag to toggle use of left hand                       
  PARAMETER(useRight, false, bool, false, true);//Flag to toggle the use of the Right hand
  PARAMETER(dropWeaponIfNecessary, false, bool, false, true); // if hasn't grabbed when weapon carrying hand is close to target, grab anyway
  PARAMETER(dropWeaponDistance, 0.3f, float, 0.0f, 1.0f); // distance below which a weapon carrying hand will request weapon to be dropped
  PARAMETER(grabStrength, -1.0, float, -1.0, 10000.0);//strength in hands for grabbing (kg m/s), -1 to ignore/disable
  PARAMETER(stickyHands, 4.0, float, 0.0, 10.0);//strength of cheat force on hands to pull towards target and stick to target ("cleverHandIK" strength)
  PARAMETER(turnToTarget, 1, int, 0, 2);//0=don't turn, 1=turnToTarget, 2=turnAwayFromTarget

  PARAMETER(grabHoldMaxTimer, 100.0, float, 0.0, 1000.0);//amount of time, in seconds, before grab automatically bails
  PARAMETER(pullUpTime, 1.0, float, 0.0, 4.0);//Time to reach the full pullup strength
  PARAMETER(pullUpStrengthRight, 0.0, float, 0.0, 1.0);// Strength to pull up with the right arm. 0 = no pull up.
  PARAMETER(pullUpStrengthLeft, 0.0, float, 0.0, 1.0);// Strength to pull up with the left arm. 0 = no pull up.
  PARAMETERV0(pos1, FLT_MAX);//Grab pos1, right hand if not using line or surface grab.
  PARAMETERV0(pos2, FLT_MAX);//Grab pos2, left hand if not using line or surface grab.
  PARAMETERV0(pos3, FLT_MAX);//
  PARAMETERV0(pos4, FLT_MAX);//
  PARAMETERV0(normalR, 1.0);//Normal for the right grab point.
  PARAMETERV0(normalL, 1.0);//Normal for the left grab point.
	PARAMETERV0(normalR2, 1.0);//Normal for the 2nd right grab point (if pointsX4grab=true).
	PARAMETERV0(normalL2, 1.0);//Normal for the 3rd left grab point (if pointsX4grab=true).
	PARAMETER(handsCollide, false, bool, false, true);//Hand collisions on when grabbing (false turns off hand collisions making grab more stable esp. to grab points slightly inside geometry) 
	PARAMETER(justBrace, false, bool, false, true);//Flag to toggle between grabbing and bracing
  PARAMETER(useLineGrab, false, bool, false, true);//use the line grab, Grab along the line (x-x2)
	PARAMETER(pointsX4grab, false, bool, false, true);//use 2 point
#if NM_EA
	PARAMETER(fromEA, false, bool, false, true);//use 2 point
#endif//#if NM_EA
  PARAMETER(surfaceGrab, false, bool, false, true);//Toggle surface grab on. Requires pos1,pos2,pos3 and pos4 to be specified.
  PARAMETER(instanceIndex, -1, int, -1, INT_MAX);//levelIndex of instance to grab (-1 = world coordinates)
  PARAMETER(instancePartIndex, 0, int, 0, INT_MAX);//boundIndex of part on instance to grab (0 = just use instance coordinates)
  PARAMETER(dontLetGo, false, bool, false, true);//Once a constraint is made, keep reaching with whatever hand is allowed
  PARAMETER(bodyStiffness, 11.0, float, 6.0, 16.0);//stiffness of upper body. Scales the arm grab such that the armStiffness is default when this is at default value
  PARAMETER(reachAngle, 2.80, float, 0.0, 3.0);//Angle from front at which the grab activates. If the poit is outside this angle from front will not try to grab.
  PARAMETER(oneSideReachAngle, 1.4, float, 0.0, 3.0);//Angle at which we will only reach with one hand.
  PARAMETER(grabDistance, 1.0, float, 0.0, 4.0);//Relative distance at which the grab starts.
	PARAMETER(move2Radius, 0.f, float, 0.0, 14.0);//Relative distance (additional to grabDistance - doesn't try to move inside grabDistance)at which the grab tries to use the balancer to move to the grab point.
  PARAMETER(armStiffness, 14.0, float, 6.0, 16.0);// Stiffness of the arm.
  PARAMETER(maxReachDistance, 0.7, float, 0.0, 4.0);// distance to reach out towards the grab point.
  PARAMETER(orientationConstraintScale, 1.0, float, 0.0, 4.0); // scale torque used to rotate hands to face normals
  PARAMETER(maxWristAngle, PI, float, 0.f, PI+0.01f);// When we are grabbing the max angle the wrist ccan be at before we break the grab. 
  PARAMETER(useHeadLookToTarget, false, bool, false, true);//if true, the character will look at targetForHeadLook after a hand grabs until the end of the behavior. (Before grabbing it looks at the grab target)
  PARAMETER(lookAtGrab, true, bool, false, true);//if true, the character will look at the grab
  PARAMETERV0(targetForHeadLook, FLT_MAX);//Only used if useHeadLookToTarget is true, the target in world space to look at.
  FEEDBACK(success,this);//Sent when a hand grabs. NOTE: feedback string either "rightArmGrab" or "leftArmGrab".
  FEEDBACK(event,this);//Sent every frame when the grab is not actively trying to move the arms to grab. NOTE: feedback string "grabNotGrabbing".
}
/*
headLook:  Acts on the neck to make the head look at the position defined by pos[X,Y,Z]. 
Optionally the velocity of the point can be compensated for by setting vel[X,Y,Z].
If alwayslook is true the character will try to look at the point even if it is out side the field of view. 
BEHAVIOURS REFERENCED: AnimPose - allows animPose to override
BodyParts: head/neck + Spine(if twistSpine (adds to current desired))
*/
BEHAVIOUR(headLook)
{
  PARAMETER(damping, 1.00f, float, 0.f, 3.f); //Damping  of the muscles
  PARAMETER(stiffness, 10.00f, float, 6.f, 16.f); //Stiffness of the muscles
  PARAMETER(instanceIndex, -1, int, -1, INT_MAX); //levelIndex of object to be looked at. vel parameters are ignored if this is non -1 
  PARAMETERV0(vel, 100.f); //The velocity of the point being looked at
  PARAMETERV0(pos, FLT_MAX); //The point being looked at
  PARAMETER(alwaysLook, false, bool, false, true); //Flag to force always to look
  PARAMETER(eyesHorizontal, true, bool, false, true); //Keep the eyes horizontal.  Use true for impact with cars.  Use false if you want better look at target accuracy when the character is on the floor or leaned over alot.
  PARAMETER(alwaysEyesHorizontal, true, bool, false, true); //Keep the eyes horizontal.  Use true for impact with cars.  Use false if you want better look at target accuracy when the character is on the floor or leaned over (when not leaned over the eyes are still kept horizontal if eyesHorizontal=true ) alot.
  PARAMETER(keepHeadAwayFromGround, false, bool, false, true);
  PARAMETER(twistSpine, true, bool, false, true); // Allow headlook to twist spine.
}
/*
highFall:  Controls the character during a jump/fall. During the jump/fall tries to maintain an upright stance, 
windmills arms, pedal legs and looks down. As approaches the ground tries to transition to either a zero pose or 
land the jump resulting in transition to the body balance.

BEHAVIOURS CALLED: HeadLook. Pedal. ArmsWindMillAdaptive. RollUp. CatchFall. BodyBalance. DynamicBalancer. RollDownStairs, BodyFoetal, BodyWrithe
BodyParts: wholebody
*/
BEHAVIOUR(highFall)
{
  PARAMETER(bodyStiffness, 11.0f, float, 6.0f, 16.0f); //stiffness of body. Value feeds through to bodyBalance (synched with defaults), to armsWindmill (14 for this value at default ), legs pedal, head look and roll down stairs directly
  PARAMETER(bodydamping, 1.0f, float, 0.0f, 3.0f); //The damping of the joints.
  PARAMETER(catchfalltime, 0.30f, float, 0.0f, 1.0f); //The length of time before the impact that the character transitions to the landing.
  PARAMETER(crashOrLandCutOff, 0.868f, float, -1.0f, 1.0f); //0.52angle is 0.868 dot//A threshold for deciding how far away from upright the character needs to be before bailing out (going into a foetal) instead of trying to land (keeping stretched out).  NB: never does bailout if ignorWorldCollisions true
  PARAMETER(pdStrength, 0.0f, float, 0.0f, 1.0f); //Strength of the controller to keep the character at angle aimAngleBase from vertical.
  PARAMETER(pdDamping, 1.0f, float, 0.0f, 5.0f); //Damping multiplier of the controller to keep the character at angle aimAngleBase from vertical.  The actual damping is pdDamping*pdStrength*constant*angVel.
  PARAMETER(armAngSpeed, 7.85f, float, 0.0f, 20.f); //arm circling speed in armWindMillAdaptive
  PARAMETER(armAmplitude, 2.0f, float, 0.0f, 10.0f); //in armWindMillAdaptive
  PARAMETER(armPhase, 3.1f, float, 0.0f, 2.0f*PI); //in armWindMillAdaptive 3.1 opposite for stuntman.  1.0 old default.  0.0 in phase.
  PARAMETER(armBendElbows, true, bool, false, true); //in armWindMillAdaptive bend the elbows as a function of armAngle.  For stuntman true otherwise false.
  PARAMETER(legRadius, 0.4f, float, 0.0f, 0.5f); //radius of legs on pedal
  PARAMETER(legAngSpeed, 7.85f, float, 0.0f, 15.f); //in pedal
  PARAMETER(legAsymmetry, 4.0f, float, -10.0f, 10.0f);//0.0 for stuntman.  Random offset applied per leg to the angular speed to desynchronise the pedaling - set to 0 to disable, otherwise should be set to less than the angularSpeed value.
  PARAMETER(arms2LegsPhase, 0.0f, float, 0.0f, 6.5f); //phase angle between the arms and legs circling angle
  PARAMETER(arms2LegsSync, 1, int, 0, 2); //0=not synched, 1=always synched, 2= synch at start only.  Synchs the arms angle to what the leg angle is.  All speed/direction parameters of armswindmill are overwritten if = 1.  If 2 and you want synced arms/legs then armAngSpeed=legAngSpeed, legAsymmetry = 0.0 (to stop randomizations of the leg cicle speed)
  PARAMETER(armsUp, -3.1f, float, -4.0f, 2.0f); //Where to put the arms when preparing to land. Approx 1 = above head, 0 = head height, -1 = down.  <-2.0 use catchFall arms, <-3.0 use prepare for landing pose if Agent is due to land vertically, feet first.
  PARAMETER(orientateBodyToFallDirection, false, bool, false, true); //toggle to orientate to fall direction.  i.e. orientate so that the character faces the horizontal velocity direction
  PARAMETER(orientateTwist, true, bool, false, true); //If false don't worry about the twist angle of the character when orientating the character.  If false this allows the twist axis of the character to be free (You can get a nice twisting highFall like the one in dieHard 4 when the car goes into the helicopter)
  PARAMETER(orientateMax, 300.0f, float, 0.0f, 2000.0f); //DEVEL parameter - suggest you don't edit it.  Maximum torque the orientation controller can apply.  If 0 then no helper torques will be used.  300 will orientate the character soflty for all but extreme angles away from aimAngleBase.  If abs (current -aimAngleBase) is getting near 3.0 then this can be reduced to give a softer feel.
  PARAMETER(alanRickman, false, bool, false, true); //If true then orientate the character to face the point from where it started falling.  HighFall like the one in dieHard with Alan Rickman
  PARAMETER(fowardRoll, false, bool, false, true); //Try to execute a forward Roll on landing
  PARAMETER(useZeroPose_withFowardRoll, false, bool, false, true); //Blend to a zero pose when forward roll is attempted.
  PARAMETER(aimAngleBase, 0.18f, float, -PI, PI); //Angle from vertical the pdController is driving to ( positive = forwards)
  PARAMETER(fowardVelRotation, -0.02f, float, -1.0f, 1.0f); //scale to add/subtract from aimAngle based on forward speed (Internal)
  PARAMETER(footVelCompScale, 0.05f, float, 0.0f, 1.0f); //Scale to change to amount of vel that is added to the foot ik from the velocity (Internal)
  PARAMETER(sideD, 0.2f, float, -1.0f, 1.0f); //sideoffset for the feet during prepareForLanding. +ve = right.
  PARAMETER(fowardOffsetOfLegIK, 0.0f, float, 0.0f, 1.0f); //Forward offset for the feet during prepareForLanding
  PARAMETER(legL, 1.0f, float, 0.f, 2.0f); //Leg Length for ik (Internal)//unused
  PARAMETER(catchFallCutOff, 0.878f, float, -1.0f, 1.0f); //0.5angle is 0.878 dot. Cutoff to go to the catchFall ( internal) //mmmtodo do like crashOrLandCutOff
  PARAMETER(legStrength, 12.0f, float, 6.0f, 16.f); //Strength of the legs at landing
  PARAMETER(balance, true, bool, false, true); //If true have enough strength to balance.  If false not enough strength in legs to balance (even though bodyBlance is called)
  PARAMETER(ignorWorldCollisions, false, bool, false, true); //Never go into bailout (foetal)
  PARAMETER(adaptiveCircling, true, bool, false, true); //stuntman type fall.  Arm and legs circling direction controlled by angmom and orientation
  PARAMETER(hula, true, bool, false, true); //With stuntman type fall.  Hula reaction if can't see floor and not rotating fast
  PARAMETER(maxSpeedForRecoverableFall, 15.0f, float, 0.0f, 100.0f); // Character needs to be moving less than this speed to consider fall as a recoverable one.
  PARAMETER(minSpeedForBrace, 10.0f, float, 0.0f, 100.0f); // Character needs to be moving at least this fast horizontally to start bracing for impact if there is an object along its trajectory.
  PARAMETER(landingNormal, 0.6f, float, 0.0f, 1.0f); // Ray-cast normal doted with up direction has to be greater than this number to consider object flat enough to land on it.
  FEEDBACK(failure,this);//Sent when the character has come to a rest lying on the ground.
  FEEDBACK(success,this);//Sent when the character has come to a rest standing up.
}
#if ALLOW_TRAINING_BEHAVIOURS
/* 
Landing :Controls the character during a jump/fall. During the jump/fall tries to maintain an upright stance, 
windmills arms, pedal legs and looks down. As approaches the ground tries to keep balance or doing a forward
roll following by standing up.In case of a vertical high fall, can do a sideroll if sideroll parameter is true.
Messages : success : if stand up at the end ( after forward roll or after balance); failure : if bail out or catchfall;

BEHAVIOURS CALLED: HeadLook. Pedal. ArmsWindmill. RollUp. CatchFall. BodyBalance. DynamicBalancer. RollDownStairs.
*/
BEHAVIOUR(landing)
{
  PARAMETER(bodyStiffness, 11.00f, float, 6.f, 16.f); //stiffness of body. Value feeds through to bodyBalance (synched with defaults), to armsWindmill (14 for this value at default ), legs pedal, head look and roll down stairs directly
  PARAMETER(bodydamping, 1.00f, float, 0.f, 3.f); //The damping of the joints.
  PARAMETER(catchfalltime, 0.3f, float, 0.f, 1.f); //The length of time before the impact that the character transitions to the landing.
  PARAMETER(crashOrLandCutOff, 0.52f, float, 0.f, 1.f); //A threshold for deciding how close to upright the characters needs to be for the success message to be sent. 
  PARAMETER(angleToCatchFallCutOff, 45.f, float, 0.f, 180.f); //Maximum angle (in degree) between orientation of the character and velocity of the COM during falling for deciding if the character will try to land OK or not (bail out). 

  PARAMETER(pdStrength, 0.30f, float, 0.f, 1.f); //Strength of the controller to keep the character is the uprigth position.

  PARAMETER(legRadius, 0.25f, float, 0.f, 0.5f); //radius of legs on pedal
  PARAMETER(legAngSpeed, 1.80f, float, 0.f, 15.f); //in pedal

  PARAMETER(armsUp, 0.0f, float, -0.5f, 0.5f); //Where to put the arms when preparing to land. Approx 1 = above head, 0 = head height, -1 = down.
  PARAMETER(armsFrontward, 0.1f, float, -0.2f, 0.5f); //Where to put the arms when preparing to land. Approx 0.5 = front of head, 0 = head, -0.2 = behind.

  PARAMETER(orientateBodyToFallDirection, true, bool, false, true); //toggle to orientate to fall direction 
  PARAMETER(predictedTimeToOrientateBodytoFallDirection, 0.3f, float, 0.0f, 2.0f); //Used only if orientateBodyToFallDirection is true, Time to predict the orientation during the falling and apply a more effective torque to orientate body to fall direction(Internal)
  PARAMETER(factorToReduceInitialAngularVelocity, 0.3f, float, 0.0f, 1.0f); //Factor to reduce, during the activation, the angular velocity of all parts of the character. If 0.f, no angular velocity (Internal)
  PARAMETER(limitNormalFall, 5.0f, float, 0.f, 10000.f); // limit height of the fall between a normal fall and a high fall (if high fall, will do a catch fall directly when he is landing)

  PARAMETER(aimAngleBase, 0.05f, float, -2.f, 2.f); //Angle from vertical the pdController is driving to ( positive = forwards)
  PARAMETER(fowardVelRotation, -0.01f, float, -1.f, 1.f); //scale to add/subtract from aimAngle based on forward speed (Internal)
  PARAMETER(sideD, 0.15f, float, 0.f, 1.f); //sideoffset for the foot ik (Internal)
  PARAMETER(legL, 0.9f, float, 0.6f, 1.f); //Leg Length for ik (Internal)
  PARAMETER(legStrength, 12.8f, float, 6.f, 16.f); //Strength of the legs at landing
  PARAMETER(ignorWorldCollisions, false, bool, false, true); //

  PARAMETER(forwardRoll, true, bool, false, true); //to prevent the use of forward roll
  PARAMETER(feetBehindCOM, -0.05f, float, -1.f, 1.f); //difference between the IK position for the feet and the COM in the backward axis. Positive value indicates the feet will be behind the COM.
  PARAMETER(feetBehindCOMVel, -0.04f, float, -0.5f, 0.5f); //Scale to change to amount of vel that is added to the foot ik from the velocity (Internal)
  PARAMETER(cheatingTorqueToForwardRoll, 0.8f, float, -2.f, 2.f); //apply a cheating torque on the pelvis during the forward roll
  PARAMETER(maxAngVelForForwardRoll, 5.5f, float, 0.f, 10.f);//Sent to the RollDownStairs behaviour as the parameter maxAngVel (internal)
  PARAMETER(stopFWCOMRoT, 0.2f, float, 0.0f, 10.0f); //Minimum angular velocity of the COM around a side axis to continue the forward roll. If COMROt inf. to this value, will do a catchfall(Internal)
  PARAMETER(stopEndFWCOMRoT, 0.3f, float, 0.0f, 10.0f); //Minimum angular velocity of the COM around a side axis to continue the forward roll during the last phase of the forward roll. If COMROt inf. to this value, will do a catchfall(Internal)

  PARAMETER(standUpCOMBehindFeet, 0.35f, float, -1.0f, 1.0f);//distance between COM position and feet to stand up (multiplied by the height of the COM) after the forward roll (Internal)
  PARAMETER(standUpRotVel, 0.03f, float, -1.0f, 1.0f);//scale to add/subtract from threeshold (distance between COM position and feet) to stand up after the forward roll ; based on forward speed (Internal)
  PARAMETER(strengthKneeToStandUp, 1.3f, float, 0.f, 2.f); //strength of knees during the stand up phase after the forward roll

  PARAMETER(sideRoll, true, bool, false, true);//authorize a sideroll if it s a vertical high fall
  PARAMETER(maxVelForSideRoll, 2.5f, float, 0.f, 10.f);//maximum horizontal velocity to start a side roll ; otherwise will try to keep balance at the landing.

  FEEDBACK(failure,this);//Sent when the character has come to a rest lying on the ground.
  FEEDBACK(success,this);//Sent when the character has come to a rest standing up.
}
#endif
/*
incomingTransforms:  Enable incoming transforms when 'start' is true, disable them if it is false; 
This will completely drive the character to the incoming animations (HARD KEYING). 
To do driven-physics animation, use Active_Pose.
*/
BEHAVIOUR(incomingTransforms)
{
}
/*
InjuredOnGround
*/
BEHAVIOUR(injuredOnGround)
{
	PARAMETER(numInjuries, 0, int, 0, 2); 
	PARAMETER(injury1Component, 0, int, 0, INT_MAX); 
	PARAMETER(injury2Component, 0, int, 0, INT_MAX); 
	PARAMETERV0(injury1LocalPosition, FLT_MAX); 
	PARAMETERV0(injury2LocalPosition, FLT_MAX); 
	PARAMETERV(injury1LocalNormal, 1,0,0,  0.0, 1.0); 
	PARAMETERV(injury2LocalNormal, 1,0,0,  0.0, 1.0); 
	PARAMETERV(attackerPos, 1,0,0,  0.0, FLT_MAX); 
	PARAMETER(dontReachWithLeft, false, bool, false, true); 
	PARAMETER(dontReachWithRight, false, bool, false, true); 
	PARAMETER(strongRollForce, false, bool, false, true); 
}
/*
Carried
*/
BEHAVIOUR(carried)
{
}
/*
Dangle
*/
BEHAVIOUR(dangle)
{
	PARAMETER(doGrab, true, bool, false, true); 
	PARAMETER(grabFrequency, 1.0f, float, 0.0f, 1.0f); 
}
/*
learnedCrawl:  1) learn a crawl from a given animation.  2) Crawl along (towards a target)
BodyParts: FullBody
*/
BEHAVIOUR(learnedCrawl)
{
  PARAMETER(stiffness, 10, float, 6.0, 16.0); 
  PARAMETER(damping, 1, float, 0.0, 2.0); 
  PARAMETER(learn, false, bool, false, true); // true will make the character learn the animation
  PARAMETER(numFrames2Learn, 98, int, 0, INT_MAX);//Number of frame of animation to learn
  PARAMETER(inputSequence, NULL, void *, NULL, NULL); 
  PARAMETER(inputSequenceSize, 0, int, 0, 100000); 
  PARAMETER(yawOffset, 0.0, float, -7.0, 7.0); // rotates the hand's IK targets during playback
  PARAMETERV0(targetPosition, FLT_MAX); // target to crawl towards? when enabled, calculates an internal yaw offset that's used instead of yawOffset param.
  PARAMETER(speed, 1.0, float, 0.0, 2.0); // playback  speed of learned sequence. also scales muscle strength.
  PARAMETER(animIndex, 4, int, 0, 4);//index into position for storing and retrieving the swing/stance frames
  PARAMETER(learnFromAnimPlayback, true, bool, false, true); // If disabled, will use an already existing .ctm file. Otherwise, will initially go through one animPose cycle and save out the desired transforms into .ctm file.
  PARAMETER(useSpine3Thing, false, bool, false, true); // does something useful to the clavicle
  PARAMETER(useRollBoneCompensation, false, bool, false, true); // adds extra twist to hips for smoother locomotion
  PARAMETER(useTwister, false, bool, false, true); // adds extra twist to spine for smoother locomotion
  FEEDBACK(success, this); //When the character reaches his target
  FEEDBACK(failure, this); //If the character rolls onto his back
  FEEDBACK(event,this);//Returns the current phase of the animation and the distance traveled over the last complete cycle every frame.
}
/*
pedalLegs:  Legs pedalling or bicycle pedalling type motion
  In Normal mode: Center is set so as to put the lower part of the circle on the foot bottom with legs straight down, then centre offsets are applied
  In Hula mode: Center is set at the hip joint centre, then centre offsets are applied
bodyParts: Legs = selectable based on pedalLeftLeg, pedalRightLeg 
*/
BEHAVIOUR(pedalLegs)
{
  PARAMETER(pedalLeftLeg, true, bool, false, true);//pedal with this leg or not
  PARAMETER(pedalRightLeg, true, bool, false, true);//pedal with this leg or not
  PARAMETER(backPedal, false, bool, false, true);//pedal forwards or backwards
  PARAMETER(radius, 0.25f, float, 0.0f, 2.0f);//base radius of pedal action
  PARAMETER(angularSpeed, 10.0f, float, 0.0f, 100.0f);//rate of pedaling
  PARAMETER(legStiffness, 10.0f, float, 6.0f, 16.0f);//stiffness of legs
  PARAMETER(pedalOffset, 0.0f, float, 0.0f, 1.0f);//Move the centre of the pedal for the left leg up by this amount, the right leg down by this amount
  PARAMETER(randomSeed, 100, int, 0, INT_MAX);//Random seed used to generate speed changes
  PARAMETER(speedAsymmetry, 8.0f, float, -10.0f, 10.0f);//Random offset applied per leg to the angular speed to desynchronise the pedaling - set to 0 to disable, otherwise should be set to less than the angularSpeed value.
  PARAMETER(adaptivePedal4Dragging, false, bool, false, true);//Will pedal in the direction of travel (if backPedal = false, against travel if backPedal = true) and with an angular velocity relative to speed upto a maximum of 13(rads/sec).  Use when being dragged by a car.  Overrides angularSpeed.
  PARAMETER(angSpeedMultiplier4Dragging, 0.3f, float, 0.0f, 1.0f);//angularSpeed =angSpeedMultiplier4Dragging * linear_speed/pedalRadius
  PARAMETER(radiusVariance, 0.4f, float, 0.0f, 1.0f);//0-1 value used to add variance to the radius value while pedalling, to desynchonize the legs' movement and provide some variety
  PARAMETER(legAngleVariance, 0.5f, float, 0.0f, 1.0f);//0-1 value used to vary the angle of the legs from the hips during the pedal; used to provide some extra variety to the motion
  PARAMETER(centreSideways, 0.0f, float, -1.0f, 1.0f);//Move the centre of the pedal for both legs sideways (+ve = right).  NB: not applied to hula.
  PARAMETER(centreForwards, 0.0f, float, -1.0f, 1.0f);//Move the centre of the pedal for both legs forward (or backward -ve)
  PARAMETER(centreUp, 0.0f, float, -1.0f, 1.0f);//Move the centre of the pedal for both legs up (or down -ve)
  PARAMETER(ellipse, 1.0f, float, -1.0f, 1.0f);////Turn the circle into an ellipse.  Ellipse has horizontal radius a and vertical radius b.  If ellipse is +ve then a=radius*ellipse and b=radius.  If ellipse is -ve then a=radius and b = radius*ellipse.  0.0 = vertical line of length 2*radius, 0.0:1.0 circle squashed horizontally (vertical radius = radius), 1.0=circle.  -0.001 = horizontal line of length 2*radius, -0.0:-1.0 circle squashed vertically (horizontal radius = radius), -1.0 = circle
  PARAMETER(dragReduction, 0.25f, float, 0.0f, 1.0f);//how much to account for the target moving through space rather than being static
  PARAMETER(hula, false, bool, false, true);//If true circle the legs in a hula motion.
}
/*
pointArm:  
BEHAVIOURS REFERENCED: AnimPose - allows animPose to override
bodyParts: Arms (useLeftArm, useRightArm)
*/
BEHAVIOUR(pointArm)
{
  PARAMETERV0(targetLeft, FLT_MAX); //point to point to (in world space)
  PARAMETER(twistLeft, 0.3, float, -1.0, 1.0); //twist of the arm around point direction
  PARAMETER(armStraightnessLeft, 0.8, float, 0.0, 2.0); //values less than 1 can give the arm a more bent look
  PARAMETER(useLeftArm, false, bool, false, true);
  PARAMETER(armStiffnessLeft, 15.0, float, 6.0, 16.0); //stiffness of arm
  PARAMETER(armDampingLeft, 1.0, float, 0.0, 2.0); //damping value for arm used to point
  PARAMETER(instanceIndexLeft, -1, int, -1, INT_MAX); //level index of thing to point at, or -1 for none. if -1, target is specified in world space, otherwise it is an offset from the object specified by this index.
  PARAMETER(pointSwingLimitLeft, 1.5, float, 0.0, 3.0); //Swing limit 
  PARAMETER(useZeroPoseWhenNotPointingLeft, false, bool, false, true);
  PARAMETERV0(targetRight, FLT_MAX); //point to point to (in world space)
  PARAMETER(twistRight, 0.3, float, -1.0, 1.0); //twist of the arm around point direction
  PARAMETER(armStraightnessRight, 0.8, float, 0.0, 2.0); //values less than 1 can give the arm a more bent look
  PARAMETER(useRightArm, false, bool, false, true);
  PARAMETER(armStiffnessRight, 15.0, float, 6.0, 16.0); //stiffness of arm
  PARAMETER(armDampingRight, 1.0, float, 0.0, 2.0); //damping value for arm used to point
  PARAMETER(instanceIndexRight, -1, int, -1, INT_MAX); //level index of thing to point at, or -1 for none. if -1, target is specified in world space, otherwise it is an offset from the object specified by this index.
  PARAMETER(pointSwingLimitRight, 1.5, float, 0.0, 3.0); //Swing limit 
  PARAMETER(useZeroPoseWhenNotPointingRight, false, bool, false, true);
}
/*
pointGun:  Point a gun at something.  Can cope with 1 or 2 single handed guns.  Pistol with support hand.  2 handed Rifle.  PointGun always disables contacts between left and right hand.
BEHAVIOURS CALLED: spineTwist, headLook
BEHAVIOURS REFERENCED: AnimPose - allows animPose to override, balancerCollisionsReaction (pushing away from a wall)
BEHAVIOURS MODIFIED: dynamicBalancer (turning - useTurnToTarget)
bodyParts: Arms (enableLeft, enableRight), spine (useSpineTwist), neck/head (useHeadLook) 
*/
BEHAVIOUR(pointGun)
{
  PARAMETER(enableRight, true, bool, false, true); // Allow right hand to point/support?
  PARAMETER(enableLeft, true, bool, false, true); // Allow right hand to point/support?
  PARAMETERV0(leftHandTarget, FLT_MAX); // Target for the left Hand
  PARAMETER(leftHandTargetIndex, -1, int, -INT_MAX, INT_MAX); // Index of the object that the left hand target is specified in, -1 is world space.
  PARAMETERV0(rightHandTarget, FLT_MAX);// Target for the right Hand
  PARAMETER(rightHandTargetIndex, -1, int, -INT_MAX, INT_MAX);// Index of the object that the right hand target is specified in, -1 is world space.
  PARAMETER(leadTarget, 5.0f, float, 0.0f, 10.0f); //Not used currently. Amount to lead target based on target velocity relative to the chest.
 
  PARAMETER(armStiffness, 14.0f, float , 2.0f, 15.0f); // Stiffness of the arm.
  PARAMETER(armStiffnessDetSupport, 8.0f, float , 2.0f, 15.0f); // Stiffness of the arm on pointing arm when a support arm is detached from a two-handed weapon.
  PARAMETER(armDamping, 1.0f, float , 0.1f, 5.0f);// Damping.
  PARAMETER(gravityOpposition, 1.0f, float, 0.0f, 2.0f); // Amount of gravity opposition on pointing arm.
  PARAMETER(gravOppDetachedSupport, 0.5f, float, 0.0f, 2.0f); // Amount of gravity opposition on pointing arm when a support arm is detached from a two-handed weapon.
  PARAMETER(massMultDetachedSupport, 0.1f, float, 0.0f, 1.0f); // Amount of mass of weapon taken into account by gravity opposition on pointing arm when a support arm is detached from a two-handed weapon.  The lower the value the more the character doesn't know about the weapon mass and therefore is more affected by it.
  PARAMETER(allowShotLooseness, false, bool, false, true); // Allow shot to set a lower arm muscleStiffness than pointGun normally would.

  PARAMETER(clavicleBlend, 0.0f, float, 0.0f, 1.0f); // How much of blend should come from incoming transforms 0(all IK) .. 1(all ITMs)   For pointing arms only.  (Support arm uses the IK solution as is for clavicles)
  PARAMETER(elbowAttitude, 0.3f, float, -1.0f, 1.0f); // Controls arm twist. (except in pistolIK)

  PARAMETER(supportConstraint,1, int, 0, 2);// Type of constraint between the support hand and gun.  0=no constraint, 1=hard point constraint, 2=Force based constraint
  PARAMETER(constraintMinDistance, 0.015f, float, 0.0f, 0.1f); //For supportConstraint = 1: Support hand constraint distance will be slowly reduced until it hits this value.
  PARAMETER(makeConstraintDistance, 0.1f, float, 0.0f, 3.0f);  //For supportConstraint = 1:  Minimum distance within which support hand constraint will be made.
  PARAMETER(reduceConstraintLengthVel, 1.5f, float, 0.1f, 10.0f); //For supportConstraint = 1:  Velocity at which to reduce the support hand constraint length
  PARAMETER(breakingStrength, -1.0f, float, -1.0f, 1000.0f);      //For supportConstraint = 1: strength of the supporting hands constraint (kg m/s), -1 to ignore/disable
  PARAMETER(constraintStrength, 2.0f, float, 0.0f, 5.0f); //For supportConstraint = 2: force constraint strength of the supporting hands - it gets shaky at about 4.0
  PARAMETER(constraintThresh, 0.1f, float, 0.0f, 1.0f);  //For supportConstraint = 2:  Like makeConstraintDistance. Force starts acting when the hands are < 3.0*thresh apart but is maximum strength < thresh. For comparison: 0.1 is used for reachForWound in shot, 0.25 is used in grab.
  PARAMETER(brokenSupportTime, 1.0f, float, 0.0f, 5.0f); //Once constraint is broken then do not try to reconnect/support for this amount of time 
  PARAMETER(brokenToSideProb, 0.5f, float, 0.0f, 1.0f); //Probability that the when a constraint is broken that during brokenSupportTime a side pose will be selected.
  PARAMETER(connectAfter, 0.7f, float, 0.0f, 5.0f); //If gunArm has been controlled by other behaviours for this time when it could have been pointing but couldn't due to pointing only allowed if connected, change gunArm pose to something that could connect for connectFor seconds
  PARAMETER(connectFor, 0.55f, float, 0.0f, 5.0f); //Time to try to reconnect for

  //Always constrain should come out naturally if the don't break constraint and neutral poses are supportable
  PARAMETER(oneHandedPointing , 1, int, 0, 3); // 0 = don't allow, 1= allow for kPistol(two handed pistol) only, 2 = allow for kRifle only, 3 = allow for kPistol and kRifle. Allow one handed pointing – no constraint if cant be supported .  If not allowed then gunHand does not try to point at target if it cannot be supported – the constraint will be controlled by always support.
  PARAMETER(alwaysSupport, false, bool, false, true); // Support a non pointing gunHand i.e. if in zero pose (constrain as well  if constraint possible)
  PARAMETER(poseUnusedGunArm, false, bool, false, true); // Apply neutral pose when a gun arm isn't in use.  NB: at the moment Rifle hand is always controlled by pointGun.
  PARAMETER(poseUnusedSupportArm, false, bool, false, true); // Apply neutral pose when a support arm isn't in use.
  PARAMETER(poseUnusedOtherArm, false, bool, false, true); // Apply neutral pose to the non-gun arm (otherwise it is always under the control of other behaviours or not set). If the non-gun hand is a supporting hand it is not controlled by this parameter but by poseUnusedSupportArm
  PARAMETER(maxAngleAcross, 90.0f, float, 0.0f, 180.0f); //max aiming angle(deg) sideways across body midline measured from chest forward that the character will try to point
  PARAMETER(maxAngleAway, 90.0f, float, 0.0f, 180.0f); //max aiming angle(deg) sideways away from body midline measured from chest forward that the character will try to point
  PARAMETER(pistolNeutralType , 0, int, 0, 2); // 0 = byFace, 1=acrossFront, 2=bySide.  NB: bySide is not connectible so be careful if combined with kPistol and oneHandedPointing = 0 or 2
  PARAMETER(neutralPoint4Pistols, false, bool, false, true); //NOT IMPLEMENTED YET KEEP=false - use pointing for neutral targets in pistol modes
  PARAMETER(neutralPoint4Rifle, true, bool, false, true); //use pointing for neutral targets in rifle mode
  PARAMETER(checkNeutralPoint, false, bool, false, true); //Check the neutral pointing is pointable, if it isn't then choose a neutral pose instead
  PARAMETERV(point2Side, 5.0f, -5.0f, -2.0f,  0.0f, FLT_MAX); //(side, up, back) side is left for left arm, right for right arm mmmmtodo
  PARAMETER(add2WeaponDistSide, 0.3f, float, -1.0f, 1000.0f); //add to weaponDistance for point2Side neutral pointing (to straighten the arm)
  PARAMETERV(point2Connect, -1.0f, -0.9f, -0.2f,  0.0f, FLT_MAX); //(side, up, back) side is left for left arm, right for rght arm mmmmtodo
  PARAMETER(add2WeaponDistConnect, 0.0f, float, -1.0f, 1000.0f); //add to weaponDistance for point2Connect neutral pointing (to straighten the arm)

  PARAMETER(usePistolIK, true, bool, false, true); // enable new ik for pistol pointing.
  PARAMETER(useSpineTwist, true, bool, false, true); // Use spine twist to orient chest?
  PARAMETER(useTurnToTarget, false, bool, false, true); // Turn balancer to help gun point at target
  PARAMETER(useHeadLook, true, bool, false, true); // Use head look to drive head?

  PARAMETER(errorThreshold, PI/8.0f, float, 0.0f, PI); // angular difference between pointing direction and target direction above which feedback will be generated. 
  PARAMETER(fireWeaponRelaxTime,0.4f,float,0.0f,5.0f); // Duration of arms relax following firing weapon.  NB:This is clamped (0,5) in pointGun
  PARAMETER(fireWeaponRelaxAmount,0.5f,float,0.1f,1.0f); // Relax multiplier following firing weapon. Recovers over relaxTime.
  PARAMETER(fireWeaponRelaxDistance,0.05f,float,0.0f,0.25f); // Range of motion for ik-based recoil.

  //gun and gun to character info
  PARAMETER(extraTilt,0.0f,float,-PI,PI); // Extra twist around gun pointing axis//mmmmUnused?
  PARAMETER(useIncomingTransforms,true,bool,false,true);// Use the incoming transforms to inform the pointGun of the primaryWeaponDistance, poleVector for the arm
  PARAMETER(measureParentOffset,true,bool,false,true);// If useIncomingTransforms = true and measureParentOffset=true then measure the Pointing-from offset from parent effector, using itms - this should point the barrel of the gun to the target.  This is added to the rightHandParentOffset. NB NOT used if rightHandParentEffector<0
  PARAMETERV0(leftHandParentOffset, FLT_MAX);// Pointing-from offset from parent effector, expressed in spine3's frame, x = back/forward, y = right/left, z = up/down.
  PARAMETER(leftHandParentEffector, -1, int, -1, 21); // -1 = Use leftShoulder. Effector from which the left hand pointing originates. ie, point from this part to the target. -1 causes default offset for active weapon mode to be applied.
  //PARAMETER(leftHandParentEffector, ART::gtaJtShoulder_Left, int, -1, 21); // -1 = Use leftShoulder. Effector from which the left hand pointing originates. ie, point from this part to the target. -1 causes default offset for active weapon mode to be applied.
  PARAMETERV0(rightHandParentOffset, FLT_MAX);// Pointing-from offset from parent effector, expressed in spine3's frame, x = back/forward, y = right/left, z = up/down. This is added to the measured one if useIncomingTransforms=true and measureParentOffset=true.  NB NOT used if rightHandParentEffector<0.  Pistol(0,0,0) Rifle(0.0032, 0.0, -0.0)
  PARAMETER(rightHandParentEffector, -1, int, -1, 21);//  -1 = Use rightShoulder.. Effector from which the right hand pointing originates. ie, point from this part to the target. -1 causes default offset for active weapon mode to be applied.
  PARAMETER(primaryHandWeaponDistance, -1.0f, float , -1.0f, 1.0f);// Distance from the shoulder to hold the weapon. If -1 and useIncomingTransforms then weaponDistance is read from ITMs. weaponDistance=primaryHandWeaponDistance clamped [0.2f:m_maxArmReach=0.65] if useIncomingTransforms = false. pistol 0.60383, rifle 0.336
  PARAMETER(weaponMask,(1 << 10),int,0,INT_MAX); // RAGE bit mask to exclude weapons from ray probe - currently defaults to MP3 weapon flag

  PARAMETER(constrainRifle, true, bool, false, true); // Use hard constraint to keep rifle stock against shoulder?
  PARAMETER(rifleConstraintMinDistance, 0.2f, float, 0.0f, FLT_MAX); // Rifle constraint distance. Deliberately kept large to create a flat constraint surface where rifle meets the shoulder.
  PARAMETER(stabilizeRifleStock, false, bool, false, true); // Enable assistance keeping rifle stock on shoulder (work in progress).

  PARAMETER(timeWarpActive, false, bool, false, true); // Is timeWarpActive enabled?
  PARAMETER(timeWarpStrengthScale, 1.0f, float, 0.1f, 2.0f); // Scale for arm and helper strength when timewarp is enabled. 1 = normal compensation.

  PARAMETER(oriStiff, 0.0f, float, 0.0f, 100.0f); // Hand stabilization controller stiffness.
  PARAMETER(oriDamp, 0.0f, float, 0.0f, 2.0f);  // Hand stabilization controller damping.
  PARAMETER(posStiff, 0.0f, float, 0.0f, 100.0f); // Hand stabilization controller stiffness.
  PARAMETER(posDamp, 0.0f, float, 0.0f, 2.0f);  // Hand stabilization controller damping.

  PARAMETER(disableArmCollisions, false, bool, false, true); // Disable collisions between right hand/forearm and the torso/legs.
  PARAMETER(disableRifleCollisions, false, bool, false, true); // Disable collisions between right hand/forearm and spine3/spine2 if in rifle mode.

  FEEDBACK(event, pointGun); //Sent when the behaviour changes state.  Or when pointGun deactivates to give information to decide the animation to blend out to
  FEEDBACK(event, handAnimation); //Sent when the behaviour wants the game to set a specific animation on the hand e.g. for gripping a gun
  FEEDBACKPARAM(handAnimation, 0, int);//Hand - NmRsHand
  FEEDBACKDESCR(handAnimation, 0, 0, kLeftHand);
  FEEDBACKDESCR(handAnimation, 0, 1, kRightHand);
  FEEDBACKPARAM(handAnimation, 1, int);//HandAnimationType
  FEEDBACKDESCR(handAnimation, 1, 0, haNone);//Dunno
  FEEDBACKDESCR(handAnimation, 1, 1, haLoose);//Loose
  FEEDBACKDESCR(handAnimation, 1, 2, haHoldingWeapon);//Holding Weapon (support hand)
  FEEDBACKDESCR(handAnimation, 1, 3, haGrab);//Grabbing
  FEEDBACKDESCR(handAnimation, 1, 4, haBrace);//Bracing/Flat
}
/*
rollDownStairs:  Curls into foetal position, at a speed defined by the strength and damping values; This behaviour is full-body and resets the character when it starts.
forcemag  sets the strength of the helper force, use a range of [0,1]. 0 is no helper force.
Only applies the helper forces when a body part is touching the ground, Unless spinWhileInAir is set to true.

BEHAVIOURS CALLED: RollUp.
BodyParts: wholeBody
*/
BEHAVIOUR(rollDownStairs)
{
  PARAMETER(stiffness, 11.0, float, 6.0, 16.0); //Effector Stiffness. value feeds through to rollUp directly
  PARAMETER(damping, 1.4, float, 0.0, 4.0); //Effector  Damping. 
  PARAMETER(forcemag, 0.55, float, 0.0, 10.0); //Helper force strength.  Do not go above 1 for a rollDownStairs/roll along ground reaction.
  PARAMETER(asymmetry, 0.0, float, -1.0, 1.0); //Add some assymetry into the helper forces.  //unused
  PARAMETER(m_useArmToSlowDown, -1.9, float, -3.0, 3.0); //the degree to which the character will try to stop a barrel roll with his arms
  PARAMETER(useZeroPose, false, bool, false, true); //Blends between a zeroPose and the Rollup, Faster the character is rotating the less the zeroPose
  PARAMETER(spinWhenInAir, false, bool, false, true); //Applied cheat forces to spin the character when in the air, the forces are 40% of the forces applied when touching the ground.  Be careful little bunny rabbits, the character could spin unnaturally in the air.  
  PARAMETER(m_armReachAmount, 1.4, float, 0.0, 3.0); //how much the character reaches with his arms to brace against the ground
  PARAMETER(m_legPush, 1.0, float, 0.0, 2.0); //amount that the legs push outwards when tumbling
  PARAMETER(tryToAvoidHeadButtingGround, false, bool, false, true); //Blends between a zeroPose and the Rollup, Faster the character is rotating the less the zeroPose
  PARAMETER(armReachLength, 0.4, float, 0.0, 1.0); //the length that the arm reaches and so how much it straightens
  PARAMETERV(customRollDir, 0, 0, 1,  1.0, 1.0); //pass in a custom direction in to have the character try and roll in that direction
  PARAMETER(useCustomRollDir, false, bool, false, true); //pass in true to use the customRollDir parameter
  PARAMETER(stiffnessDecayTarget, 9.0, float, 0.0, 20.0); // The target linear velocity used to start the rolling.
  PARAMETER(stiffnessDecayTime, -1.0, float, -1.0, 10.0); //time, in seconds, to decay stiffness down to the stiffnessDecayTarget value (or -1 to disable)
  PARAMETER(asymmetricalLegs, 0.0, float, -1.0, 1.0); //0 is no leg asymmetry in 'foetal' position.  greater than 0 a asymmetricalLegs-rand(30%), added/minus each joint of the legs in radians.  Random number changes about once every roll.  0.4 gives a lot of asymmetry
  PARAMETER(zAxisSpinReduction, 0.0, float, 0.0, 1.0); //Tries to reduce the spin around the z axis. Scale 0 - 1
  PARAMETER(targetLinearVelocityDecayTime, 0.5, float, 0.0, 2.0); //Time for the targetlinearVelocity to decay to zero.
  PARAMETER(targetLinearVelocity, 1.0, float, 0.0, 10.0); //Helper torques are applied to match the spin of the character to the max of targetLinearVelocity and COMVelMag
  PARAMETER(onlyApplyHelperForces, false, bool, false, true); //Don't use rollup if true

  PARAMETER(useVelocityOfObjectBelow, false, bool, false, true); //scale applied cheat forces/torques to (zero) if object underneath character has velocity greater than 1.f
  PARAMETER(useRelativeVelocity, false, bool, false, true); //useVelocityOfObjectBelow uses a relative velocity of the character to the object underneath

  PARAMETER(applyFoetalToLegs, false, bool, false, true); //if true, use rollup for upper body and a kind of foetal behavior for legs 
  PARAMETER(movementLegsInFoetalPosition, 1.3f, float, 0.0f, 10.0f);//Only used if applyFoetalToLegs = true : define the variation of angles for the joints of the legs

  PARAMETER(maxAngVelAroundFrontwardAxis, 2.0, float, -1.0, 10.0);//Only used if applyNewRollingCheatingTorques or applyHelPerTorqueToAlign defined to true : maximal angular velocity around frontward axis of the pelvis to apply cheating torques.
  PARAMETER(minAngVel, 0.5, float, 0.0, 10.0);//Only used if applyNewRollingCheatingTorques or applyHelPerTorqueToAlign defined to true : minimal angular velocity of the roll to apply cheating torques 

  PARAMETER(applyNewRollingCheatingTorques, false, bool, false, true); //if true will use the new way to apply cheating torques (like in fallOverWall), otherwise will use the old way
  PARAMETER(maxAngVel, 5.0, float, 0.0, 10.0);//Only used if applyNewRollingCheatingTorques defined to true : maximal angular velocity of the roll to apply cheating torque
  PARAMETER(magOfTorqueToRoll, 50.0, float, 0.0, 500.0);//Only used if applyNewRollingCheatingTorques defined to true : magnitude of the torque to roll down the stairs

  PARAMETER(applyHelPerTorqueToAlign, false, bool, false, true);//apply torque to align the body orthogonally to the direction of the roll 
  PARAMETER(delayToAlignBody, 0.2, float, 0.0, 10.0);//Only used if applyHelPerTorqueToAlign defined to true : delay to start to apply torques
  PARAMETER(magOfTorqueToAlign, 50.0, float, 0.0, 500.0);//Only used if applyHelPerTorqueToAlign defined to true : magnitude of the torque to align orthogonally the body
  PARAMETER(airborneReduction, 0.85, float, 0.0, 1.0);//Ordinarily keep at 0.85.  Make this lower if you want spinning in the air.
}
/*
shot:  Behaviour when shot by a gun. General affect is to reach for the body part and stagger... weakening only when given a shot_relax message.
You can be shot after hitting the ground and the character will react. 
Arms that are not holding or injured will flail to maintain balance. 
The character weakens briefly on each shot so as to emphasize the effect of each shot.
BEHAVIOURS CALLED: HeadLook. SpineTwist. ArmsWindmill. ArmsWindmillAdaptive(underWater arms), BodyWrithe(underwater), RollUp. CatchFall, DynamicBalancer (no behaviour message). StaggerFall. BalancerCollisionsReaction, PointGun
bodyParts:  wholeBody
*/
BEHAVIOUR(shot)
{
  PARAMETER(bodyStiffness, 11.0f, float, 6.0f, 16.0f); //stiffness of body. Feeds through to roll_up
  PARAMETER(spineDamping, 1.0f, float, 0.1f, 10.0f); //stiffness of body. Feeds through to roll_up
  PARAMETER(armStiffness, 10.0f, float, 6.0f, 16.0f); //arm stiffness
  PARAMETER(initialNeckStiffness, 14.0f, float, 3.0f, 16.0f); //initial stiffness of neck after being shot.
  PARAMETER(initialNeckDamping, 1.0f, float, 0.1f, 10.0f); //intial damping of neck after being shot.
  PARAMETER(neckStiffness, 14.0f, float, 3.0f, 16.0f); //stiffness of neck.
  PARAMETER(neckDamping, 1.0f, float, 0.1f, 10.0f); //damping of neck.
  PARAMETER(kMultOnLoose, 0.0f, float, 0.0f, 1.0f); //how much to add to upperbody stiffness dependent on looseness
  PARAMETER(kMult4Legs, 0.3f, float, 0.0f, 1.0f); //how much to add to leg stiffnesses dependent on looseness
  PARAMETER(loosenessAmount, 1.0f, float, 0.0f, 1.0f); //how loose the character is made by a newBullet. between 0 and 1
  PARAMETER(looseness4Fall, 0.0f, float, 0.0f, 1.0f); //how loose the character is made by a newBullet if falling
  PARAMETER(looseness4Stagger, 0.0f, float, 0.0f, 1.0f); //how loose the upperBody of the character is made by a newBullet if staggerFall is running (and not falling).  Note atm the neck ramp values are ignored in staggerFall
  PARAMETER(minArmsLooseness, 0.1f, float, 0.0f, 1.0f); //minimum looseness to apply to the arms 
  PARAMETER(minLegsLooseness, 0.1f, float, 0.0f, 1.0f); //minimum looseness to apply to the Legs
  PARAMETER(grabHoldTime, 2.0f, float, 0.0f, 10.0f); //how long to hold for before returning to relaxed arm position
  PARAMETER(spineBlendExagCPain, false, bool, false, true); //true: spine is blended with zero pose, false: spine is blended with zero pose if not setting exag or cpain  
  PARAMETER(spineBlendZero, 0.6f, float, -0.1f, 1.f); //spine is always blended with zero pose this much and up to 1 as the character become stationary.  If negative no blend is ever applied.
  PARAMETER(bulletProofVest, false, bool, false, true); // looseness applied to spine is different if bulletProofVest is true
  PARAMETER(alwaysResetLooseness, true, bool, false, true); // looseness always reset on shotNewBullet even if previous looseness ramp still running.  Except for the neck which has it's own ramp.
  PARAMETER(alwaysResetNeckLooseness, true, bool, false, true); // Neck looseness always reset on shotNewBullet even if previous looseness ramp still running
  PARAMETER(angVelScale, 1.0f, float, 0.0f, 1.0f);  // How much to scale the angular velocity coming in from animation of a part if it is in angVelScaleMask (otherwise scale by 1.0)
  PARAMETER(angVelScaleMask, "fb", char *, "", ""); //Parts to scale the initial angular velocity by angVelScale (otherwize scale by 1.0)
  PARAMETER(flingWidth, 0.5f, float, 0.0f, 1.0f); //Width of the fling behaviour. 
  PARAMETER(timeBeforeReachForWound, 0.2f, float, 0.0f, 10.0f); //time, in seconds, before the character begins to grab for the wound on the first hit
  PARAMETER(exagDuration, 0.0f, float, 0.0f, 10.0f);//exaggerate bullet duration (at exagMag/exagTwistMag) 
  PARAMETER(exagMag, 1.0f, float, 0.0f, 10.0f); //exaggerate bullet spine Lean magnitude
  PARAMETER(exagTwistMag, 0.5f, float, 0.0f, 10.0f); //exaggerate bullet spine Twist magnitude
  PARAMETER(exagSmooth2Zero, 0.0f, float, 0.0f, 10.0f); //exaggerate bullet duration ramping to zero after exagDuration
  PARAMETER(exagZeroTime, 0.0f, float, 0.0f, 10.0f);//exaggerate bullet time spent at 0 spine lean/twist after exagDuration + exagSmooth2Zero
  PARAMETER(cpainSmooth2Time, 0.2f, float, 0.0f, 10.0f);//conscious pain duration ramping from zero to cpainMag/cpainTwistMag 
  PARAMETER(cpainDuration, 0.0f, float, 0.0f, 10.0f); //conscious pain duration at cpainMag/cpainTwistMag after cpainSmooth2Time
  PARAMETER(cpainMag, 1.0f, float, 0.0, 10.0f); //conscious pain spine Lean magnitude (Replaces spinePainMultiplier)
  PARAMETER(cpainTwistMag, 0.5f, float, 0.0f, 10.0f); //conscious pain spine Twist magnitude Replaces spinePainTwistMultiplier) 
  PARAMETER(cpainSmooth2Zero, 1.5f, float, 0.0f, 10.0f); //conscious pain ramping to zero after cpainSmooth2Time + cpainDuration (Replaces spinePainTime)
  PARAMETER(crouching, false, bool, false, true); //is the guy crouching or not
  PARAMETER(chickenArms, false, bool, false, true); //Type of reaction
  PARAMETER(reachForWound, true, bool, false, true); //Type of reaction
  PARAMETER(fling, false, bool, false, true); //Type of reaction
  PARAMETER(allowInjuredArm, false, bool, false, true); // injured arm code runs if arm hit (turns and steps and bends injured arm)
  PARAMETER(allowInjuredLeg, true, bool, false, true); // when false injured leg is not bent and character does not bend to reach it
  PARAMETER(allowInjuredLowerLegReach, false, bool, false, true); // when false don't try to reach for injured Lower Legs (shins/feet)
  PARAMETER(allowInjuredThighReach, true, bool, false, true); // when false don't try to reach for injured Thighs  
  PARAMETER(stableHandsAndNeck, false, bool, false, true); //additional stability for hands and neck (less loose)
  PARAMETER(melee, false, bool, false, true);
  PARAMETER(useCatchFallOnFall, false, bool, false, true); //use the catch fall instead of RollUp when the balance fails during Shot (eg. the character isn't dead yet!)
  PARAMETER(useExtendedCatchFall, false, bool, false, true); //keep the character active instead of relaxing at the end of the catch fall  
  PARAMETER(initialWeaknessZeroDuration, 0.0f, float, 0.0f, 10.0f); // duration for which the character's upper body stays at minimum stiffness (not quite zero)
  PARAMETER(initialWeaknessRampDuration, 0.4f, float, 0.01f, 10.0f); // duration of the ramp to bring the character's upper body stiffness back to normal levels
  PARAMETER(initialNeckDuration, 0.0f, float, 0.0f, 10.0f); // duration for which the neck stays at intial stiffness/damping
  PARAMETER(initialNeckRampDuration, 0.4f, float, 0.01f, 10.0f); // duration of the ramp to bring the neck stiffness/damping back to normal levels
  PARAMETER(useCStrModulation, false, bool, false, true); // if enabled upper and lower body strength scales with character strength, using the range given by parameters below
  PARAMETER(cStrUpperMin, 0.1f, float, 0.1f, 1.0f); // proportions to what the strength would be normally
  PARAMETER(cStrUpperMax, 1.0f, float, 0.1f, 1.0f);
  PARAMETER(cStrLowerMin, 0.1f, float, 0.1f, 1.0f);
  PARAMETER(cStrLowerMax, 1.0f, float, 0.1f, 1.0f);
	PARAMETER(deathTime, -1.0f, float, -1.f, 1000.f);//time to death (HACK for underwater). If -ve don't ever die

  FEEDBACK(success, this); //Sent when: A) the character has completed relaxing after a shoot or B) when the crouch shoot stopped moving.
  FEEDBACK(event, this); //Sent when the character finishes looking at his wound.
}
/*
shotNewBullet:  Send new wound information to the shot.  Can cause shot to restart it's performance in part or in whole.
*/
BEHAVIOUR(shotNewBullet)
{
	PARAMETER(bodyPart, 0, int, 0, 21); //part ID on the body where the bullet hit
	PARAMETER(localHitPointInfo, true, bool, false, true); //if true then normal and hitPoint should be supplied in local coordinates of bodyPart.  If false then normal and hitPoint should be supplied in World coordinates
	PARAMETERV(normal, 0,0,-1,  1.0, 1.0); //Normal coming out of impact point on character.  Can be local or global depending on localHitPointInfo
	PARAMETERV0(hitPoint, FLT_MAX); //position of impact on character. Can be local or global depending on localHitPointInfo
	PARAMETERV0(bulletVel, 2000.0); //bullet velocity in world coordinates
}
/*
 shotSnap:  set up the snap reaction that the shot has to a newBullet(wound).  
 The character reacts with a snap (torqued bodyParts one way then the opposite) simulating a general shock reflex
 bodyParts: wholebody selectable using parameters - HOWEVER muscle parmas and desired angles NOT changed. 
 */
BEHAVIOUR(shotSnap)
{
  PARAMETER(snap, false, bool, false, true); //Add a Snap to shot.
  PARAMETER(snapMag, 0.4f, float, 0.0f, 1.f);//The magnitude of the reaction 
  PARAMETER(snapMovingMult, 1.f, float, 0.0, 20.f);//movingMult*snapMag = The magnitude of the reaction if moving(comVelMag) faster than movingThresh
  PARAMETER(snapBalancingMult, 1.f, float, 0.0, 20.f);//balancingMult*snapMag = The magnitude of the reaction if balancing = (not lying on the floor/ not upper body not collided) and not airborne 
  PARAMETER(snapAirborneMult, 1.f, float, 0.0, 20.f);//airborneMult*snapMag = The magnitude of the reaction if airborne 
  PARAMETER(snapMovingThresh, 1.f, float, 0.0, 20.f);//If moving(comVelMag) faster than movingThresh then mvingMult applied to tazeMag   
  PARAMETER(snapDirectionRandomness, 0.3f, float, 0.0f, 1.f);//The character snaps in a prescribed way (decided by bullet direction) - Higher the value the more random this direction is.
  PARAMETER(snapLeftArm, false, bool, false, true); //snap the leftArm.
  PARAMETER(snapRightArm, false, bool, false, true); //snap the rightArm.
  PARAMETER(snapLeftLeg, false, bool, false, true); //snap the leftLeg.
  PARAMETER(snapRightLeg, false, bool, false, true); //snap the rightLeg.
  PARAMETER(snapSpine, true, bool, false, true); //snap the spine.
  PARAMETER(snapNeck, true, bool, false, true); //snap the neck.
  PARAMETER(snapPhasedLegs, true, bool, false, true); //Legs are either in phase with each other or not
  PARAMETER(snapHipType, 0, int, 0, 2);//type of hip reaction 0=none, 1=side2side 2=steplike
  PARAMETER(snapUseBulletDir, true, bool, false, true); //Legs are either in phase with each other or not
  PARAMETER(snapHitPart, false, bool, false, true); //Snap only around the wounded part//mmmmtodo check whether bodyPart doesn't have to be remembered for unSnap 
  PARAMETER(unSnapInterval, 0.01f, float, 0.0f, 100.f);//Interval before applying reverse snap
  PARAMETER(unSnapRatio, 0.7f, float, 0.0f, 100.f);//The magnitude of the reverse snap 
  PARAMETER(snapUseTorques, true, bool, false, true); //use torques to make the snap otherwise use a change in the parts angular velocity
}
/*
shotShockSpin: configure the shockSpin effect in shot.  Spin/Lift the character using cheat torques/forces
*/
BEHAVIOUR(shotShockSpin)
{
  PARAMETER(addShockSpin, false, bool, false, true); //if enabled, add a short 'shock' of torque to the character's spine to exaggerate bullet impact
  PARAMETER(randomizeShockSpinDirection, false, bool, false, true); //for use with close-range shotgun blasts, or similar; choose a random direction to apply shock-spin, rather than derive direction from bullet velocity/impact
  PARAMETER(alwaysAddShockSpin, false, bool, false, true); //if true, apply the shock spin no matter which body component was hit. otherwise only apply if the spine or clavicles get hit
  PARAMETER(shockSpinMin, 50.0, float, 0.0, 1000.0); //minimum amount of torque to add if using shock-spin feature
  PARAMETER(shockSpinMax, 90.0, float, 0.0, 1000.0); //maxiumum amount of torque to add if using shock-spin feature
  PARAMETER(shockSpinLiftForceMult, 0.0, float, 0.0, 2.0); //if greater than 0, apply a force to lift the character up while the torque is applied, trying to produce a dramatic spun/twist shotgun-to-the-chest effect. this is a scale of the torque applied, so 8.0 or so would give a reasonable amount of lift
  PARAMETER(shockSpinDecayMult, 4.0, float, 0.0, 10.0); //multiplier used when decaying torque spin over time; 4.0 = decay in ~1/4th of a second. 1.0 = decay over ~1 second.
  PARAMETER(shockSpinScalePerComponent, 0.5, float, 0.0, 2.0); //torque applied is scaled by this amount across the spine components - spine2 recieving the full amount, then 3 and 1 and finally 0. each time, this value is used to scale it down. 0.5 means half the torque each time.
  PARAMETER(shockSpinMaxTwistVel, -1.f, float, -1.f, 200.0); //shock spin ends when twist velocity is greater than this value (try 6.0).  If set to -1 does not stop
  PARAMETER(shockSpinScaleByLeverArm, true, bool, false, true); //shock spin scales by lever arm of bullet i.e. bullet impact point to centre line
  PARAMETER(shockSpinAirMult, 1.f, float, 0.f, 1.f); //shockSpin's torque is multipied by this value when both the character's feet are not in contact
  PARAMETER(shockSpin1FootMult, 1.f, float, 0.f, 1.f); //shockSpin's torque is multipied by this value when the one of the character's feet are not in contact
  PARAMETER(shockSpinFootGripMult, 1.f, float, 0.f, 1.f); //shockSpin scales the torques applied to the feet by footSlipCompensation
  PARAMETER(bracedSideSpinMult, 1.f, float, 1.f, 5.f); //If shot on a side with a forward foot and both feet are on the ground and balanced, increase the shockspin to compensate for the balancer naturally resisting spin to that side
}
/*
shotFallToKnees:  configure the fall to knees shot.
*/
BEHAVIOUR(shotFallToKnees)
{
  PARAMETER(fallToKnees, false, bool, false, true); //Type of reaction
  PARAMETER(ftkAlwaysChangeFall, false, bool, false, true); //Always change fall behaviour.  If false only change when falling forward
  PARAMETER(ftkBalanceTime, 0.7f, float, 0.0f, 5.0f); //How long the balancer runs for before fallToKnees starts
  PARAMETER(ftkHelperForce, 200.f, float, 0.0f, 2000.0f); //Hip helper force magnitude - to help character lean over balance point of line between toes
  PARAMETER(ftkHelperForceOnSpine, true, bool, false, true); //Helper force applied to spine3 aswell
  PARAMETER(ftkLeanHelp, 0.05f, float, 0.0f, 0.3f); //Help balancer lean amount - to help character lean over balance point of line between toes.  Half of this is also applied as hipLean 
  PARAMETER(ftkSpineBend, -0.0f, float, -0.2f, 0.3f); //Bend applied to spine when falling from knees. (+ve forward - try -0.1) (only if rds called)
  PARAMETER(ftkStiffSpine, false, bool, false, true); //Stiffen spine when falling from knees (only if rds called)
  PARAMETER(ftkImpactLooseness, 0.5f, float, 0.0f, 1.f); //Looseness (muscleStiffness = 1.01f - m_parameters.ftkImpactLooseness) applied to upperBody on knee impacts
  PARAMETER(ftkImpactLoosenessTime, 0.2f, float, -0.1f, 1.0f); //Time that looseness is applied after knee impacts
  PARAMETER(ftkBendRate, 0.7f, float, 0.0f, 4.0f); //Rate at which the legs are bent to go from standing to on knees 
  PARAMETER(ftkHipBlend, 0.3f, float, 0.0f, 1.0f); //Blend from current hip to balancing on knees hip angle 
  PARAMETER(ftkFricMult, 1.0f, float, 0.0f, 5.0f); //Multiplier on the reduction of friction for the feet based on angle away from horizontal - helps the character fall to knees quicker
  PARAMETER(ftkHipAngleFall, 0.5f, float, -1.0f, 1.0f); //Apply this hip angle when the character starts to fall backwards when on knees
  PARAMETER(ftkPitchForwards, 0.1f, float, -0.5f, 0.5f); //Hip pitch applied (+ve forward, -ve backwards) if character is falling forwards on way down to it's knees 
  PARAMETER(ftkPitchBackwards, 0.1f, float, -0.5f, 0.5f); //Hip pitch applied (+ve forward, -ve backwards) if character is falling backwards on way down to it's knees
  PARAMETER(ftkFallBelowStab, 0.5f, float, 0.0f, 5.0f); //Balancer instability below which the character starts to bend legs even if it isn't going to fall on to it's knees (i.e. if going backwards). 0.3 almost ensures a fall to knees but means the character will keep stepping backward until it slows down enough.
  PARAMETER(ftkBalanceAbortThreshold, 2.0f, float, 0.0f, 4.0f); //when the character gives up and goes into a fall
  PARAMETER(ftkOnKneesArmType, 2, int, 0, 2); //Type of arm response when on knees falling forward 0=useFallArms (from RollDownstairs or catchFall), 1= armsIn, 2=armsOut
  PARAMETER(ftkReleaseReachForWound, false, bool, false, true); //Release the reachForWound when knees hit
  PARAMETER(ftkReleasePointGun, false, bool, false, true); //Override the pointGun when knees hit
  PARAMETER(ftkFailMustCollide, true, bool, false, true); // The upper body of the character must be colliding and other failure conditions met to fail
}
/*
shotFromBehind:  configure the shot from behind reaction
*/
BEHAVIOUR(shotFromBehind)
{
  PARAMETER(shotFromBehind, false, bool, false, true); //Type of reaction
  PARAMETER(sfbSpineAmount, 4.0f, float, 0.0f, 10.0f); // SpineBend.
  PARAMETER(sfbNeckAmount, 1.0f, float, 0.0f, 10.0f); // Neck Bend.
  PARAMETER(sfbHipAmount, 1.0f, float, 0.0f, 10.0f); // hip Pitch
  PARAMETER(sfbKneeAmount, 0.05f, float, 0.0f, 1.0f); // knee bend
  PARAMETER(sfbPeriod, 0.7f, float, 0.0f, 10.0f); // shotFromBehind reaction period after being shot
  PARAMETER(sfbForceBalancePeriod, 0.3f, float, 0.0f, 10.0f); // amount of time not taking a step
  PARAMETER(sfbArmsOnset, 0.0f, float, 0.0f, 10.0f); // amount of time before applying spread out arms pose
  PARAMETER(sfbKneesOnset, 0.0f, float, 0.0f, 10.0f); // amount of time before bending knees a bit
  PARAMETER(sfbNoiseGain, 0.0f, float, 0.0f, 2.0f); // Controls additional independent randomized bending of left/right elbows 
}
/*
shotInGuts:  configure the shot in guts reaction
*/
BEHAVIOUR(shotInGuts)
{
  PARAMETER(shotInGuts, false, bool, false, true); //Type of reaction
  PARAMETER(sigSpineAmount, 2.0f, float, 0.0f, 10.0f); // SpineBend.
  PARAMETER(sigNeckAmount, 1.0f, float, 0.0f, 10.0f); // Neck Bend.
  PARAMETER(sigHipAmount, 1.0f, float, 0.0f, 10.0f); // hip Pitch
  PARAMETER(sigKneeAmount, 0.05f, float, 0.0f, 1.0f); // knee bend
  PARAMETER(sigPeriod, 2.0f, float, 0.0f, 10.0f); // active time after being shot
  PARAMETER(sigForceBalancePeriod, 0.0f, float, 0.0f, 10.0f); // amount of time not taking a step
  PARAMETER(sigKneesOnset, 0.0f, float, 0.0f, 10.0f); // amount of time not taking a step
}
/*
shotHeadLook:  configure the headlook in shot.
Head will look at wound or headLookPos(if headlookPos = 0 then either velocity direction if not backwards or just looks forward) alternatively for a random amount of time between headLookAtHeadPosMinTimer and headLookAtHeadPosMaxTimer. Initially will look at wound for random betweeen headLookWoundMinTimer and headLookWoundMaxTimer secs.
To not look at wound set headLookAtHeadPosMinTimer and headLookAtHeadPosMaxTimer to 0.0
*/
BEHAVIOUR(shotHeadLook)
{
  PARAMETER(useHeadLook, false, bool, false, true); //Use headLook.  Default: looks at provided target or if this is zero -  looks forward or in velocity direction. If reachForWound is enabled, switches between looking at the wound and at the default target.
  PARAMETERV0(headLook, FLT_MAX); //position to look at with headlook flag
  PARAMETER(headLookAtWoundMinTimer, 0.25f, float, 0.0f, 10.0f);//Min time to look at wound
  PARAMETER(headLookAtWoundMaxTimer, 0.8f, float, 0.0f, 10.0f);//Max time to look at wound
  PARAMETER(headLookAtHeadPosMaxTimer, 1.7f, float, 0.0f, 10.0f);//Min time to look headLook or if zero - forward or in velocity direction
  PARAMETER(headLookAtHeadPosMinTimer, 0.6f, float, 0.0f, 10.0f);//Max time to look headLook or if zero - forward or in velocity direction
}
/*
shotConfigureArms:  configure the arm reactions in shot
*/
BEHAVIOUR(shotConfigureArms)
{
  PARAMETER(brace, true, bool, false, true);//blind brace with arms if appropriate
  PARAMETER(pointGun, false, bool, false, true);//Point gun if appropriate.
  PARAMETER(useArmsWindmill, true, bool, false, true);//armsWindmill if going backwards fast enough
  PARAMETER(releaseWound, 1, int, 0, 2);//release wound if going sideways/forward fast enough.  0 = don't. 1 = only if bracing. 2 = any default arm reaction
  PARAMETER(alwaysReachTime, 0.3f, float, 0.f, 10.f);//Inhibit arms brace for this amount of time after reachForWound has begun
  PARAMETER(AWSpeedMult, 1.f, float, 0.f, 1.f);//For armsWindmill, multiplier on character speed - increase of speed of circling is proportional to character speed (max speed of circliing increase = 1.5). eg. lowering the value increases the range of velocity that the 0-1.5 is applied over
  PARAMETER(AWRadiusMult, 1.f, float, 0.f, 1.f);//For armsWindmill, multiplier on character speed - increase of radii is proportional to character speed (max radius increase = 0.45). eg. lowering the value increases the range of velocity that the 0-0.45 is applied over
  PARAMETER(AWStiffnessAdd, 4.f, float, 0.f, 16.f);//For armsWindmill, added arm stiffness ranges from 0 to AWStiffnessAdd
  PARAMETER(reachWithOneHand, 0, int, 0, 2);//Force character to reach for wound with only one hand.  0= allow 2 handed reach, 1= left only if 2 handed possible, 2= right only if 2 handed possible
  PARAMETER(allowLeftPistolRFW, true, bool, false, true);//Allow character to reach for wound with left hand if holding a pistol.  It never will for a rifle. If pointGun is running this will only happen if the hand cannot point and pointGun:poseUnusedGunArm = false
  PARAMETER(allowRightPistolRFW, false, bool, false, true);//Allow character to reach for wound with right hand if holding a pistol. It never will for a rifle. If pointGun is running this will only happen if the hand cannot point and pointGun:poseUnusedGunArm = false
  PARAMETER(rfwWithPistol, false, bool, false, true);// Override pointGun and reachForWound if desired if holding a pistol.  It never will for a rifle
}

/*
staggerFall:  This provides a staggerFall behaviour.  The character tries to keep it's momentum going/increasing and weakens eventually leading to
falling over.

BEHAVIOURS CALLED: DynamicBalancer. HeadLook. CatchFall. RollDownstairs. ArmsWindmillAdaptive. 
BEHAVIOURS REFERENCED: Shot (allow shot to do arms if shot Param braceArms=true), BalancerCollisionsReaction, Yanked (don't call RollDownstairs if yanked is running)
BEHAVIOURS MODIFIED: BalancerCollisionsReaction
BodyParts: WholeBody or lowerBody(upperBodyReaction = false)
*/
BEHAVIOUR(staggerFall)
{
  PARAMETER(armStiffness, 12.0, float, 0.0, 16.0); //stiffness of arms. catch_fall's stiffness scales with this value, but has default values when this is default
  PARAMETER(armDamping, 1.0, float, 0.0, 2.0); //Sets damping value for the arms
  PARAMETER(spineDamping, 1.0, float, 0.0, 2.0);
  PARAMETER(spineStiffness, 10.0, float, 0.0, 16.0);
  PARAMETER(armStiffnessStart, 3.0, float, 0.0, 16.0); //armStiffness during the yanked timescale ie timeAtStartValues
  PARAMETER(armDampingStart, 0.1, float, 0.0, 2.0); //armDamping during the yanked timescale ie timeAtStartValues
  PARAMETER(spineDampingStart, 0.1, float, 0.0, 2.0); //spineDamping during the yanked timescale ie timeAtStartValues
  PARAMETER(spineStiffnessStart, 3.0, float, 0.0, 16.0); //spineStiffness during the yanked timescale ie timeAtStartValues
  PARAMETER(timeAtStartValues, 0.0, float, 0.0, 2.0); //time spent with Start values for arms and spine stiffness and damping ie for whiplash efffect
  PARAMETER(rampTimeFromStartValues, 0.0, float, 0.0, 2.0); //time spent ramping from Start to end values for arms and spine stiffness and damping ie for whiplash efffect (occurs after timeAtStartValues)
  PARAMETER(staggerStepProb, 0.0, float, 0.0, 1.0); //Probability per step of time spent in a stagger step 
  PARAMETER(stepsTillStartEnd, 2, int, 0, 100); //steps taken before lowerBodyStiffness starts ramping down
  PARAMETER(timeStartEnd, 100.0, float, 0.0, 100.0); //time from start of behaviour before lowerBodyStiffness starts ramping down by perStepReduction1
  PARAMETER(rampTimeToEndValues, 0.0, float, 0.0, 10.0); //time spent ramping from lowerBodyStiffness to lowerBodyStiffnessEnd
  PARAMETER(lowerBodyStiffness, 13.0, float, 0.0, 16.0); //lowerBodyStiffness should be 12
  PARAMETER(lowerBodyStiffnessEnd, 8.0, float, 0.0, 16.0); //lowerBodyStiffness at end
  PARAMETER(predictionTime, 0.1f, float, 0.f, 1.f); //amount of time (seconds) into the future that the character tries to step to. bigger values try to recover with fewer, bigger steps. smaller values recover with smaller steps, and generally recover less.
  PARAMETER(perStepReduction1, 0.7f, float, 0.f, 10.f); //LowerBody stiffness will be reduced every step to make the character fallover 
  PARAMETER(leanInDirRate, 1.f, float, 0.f, 10.f); //leanInDirection will be increased from 0 to leanInDirMax linearly at this rate
  PARAMETER(leanInDirMaxF, 0.1f, float, 0.f, 1.f); //Max of leanInDirection magnitude when going forwards
  PARAMETER(leanInDirMaxB, 0.3f, float, 0.f, 1.f); //Max of leanInDirection magnitude when going backwards
  PARAMETER(leanHipsMaxF, 0.0f, float, 0.f, 1.f); //Max of leanInDirectionHips magnitude when going forwards
  PARAMETER(leanHipsMaxB, 0.0f, float, 0.f, 1.f); //Max of leanInDirectionHips magnitude when going backwards
  PARAMETER(lean2multF, -1.0f, float, -5.0f, 5.0f); //Lean of spine to side in side velocity direction when going forwards
  PARAMETER(lean2multB, -2.0f, float, -5.0f, 5.0f); //Lean of spine to side in side velocity direction when going backwards
  PARAMETER(pushOffDist, 0.2f, float, -1.f, 1.f); //amount stance foot is behind com in the direction of velocity before the leg tries to pushOff to increase momentum.  Increase to lower the probability of the pushOff making the character bouncy
  PARAMETER(maxPushoffVel, 20.0f, float, -20.f, 20.f); //stance leg will only pushOff to increase momentum if the vertical hip velocity is less than this value. 0.4 seems like a good value.  The higher it is the the less this functionality is applied.  If it is very low or negative this can stop the pushOff altogether 
  PARAMETER(hipBendMult, 0.0f, float, -10.f, 10.f); //hipBend scaled with velocity 
  PARAMETER(alwaysBendForwards, false, bool, false, true); //bend forwards at the hip (hipBendMult) whether moving backwards or forwards
  PARAMETER(spineBendMult, 0.4f, float, -10.f, 10.f); //spine bend scaled with velocity
  PARAMETER(useHeadLook, true, bool, false, true); //enable and provide a look-at target to make the character's head turn to face it while balancing, balancer default is 0.2
  PARAMETERV0(headLookPos, FLT_MAX); //position of thing to look at; world-space if instance index = -1, otherwise local-space to that object
  PARAMETER(headLookInstanceIndex, -1, int, -1, INT_MAX); //level index of thing to look at
  PARAMETER(headLookAtVelProb, 1.0, float, -1.0, 1.0); // Probability [0-1] that headLook will be looking in the direction of velocity when stepping
  PARAMETER(turnOffProb, 0.0, float, 0.0, 1.0); //Weighted Probability that turn will be off. This is one of six turn type weights.
  PARAMETER(turn2TargetProb, 0.0, float, 0.0, 1.0); //Weighted Probability of turning towards headLook target. This is one of six turn type weights.
  PARAMETER(turn2VelProb, 1.0, float, 0.0, 1.0); //Weighted Probability of turning towards velocity. This is one of six turn type weights.
  PARAMETER(turnAwayProb, 0.0, float, 0.0, 1.0); //Weighted Probability of turning away from headLook target. This is one of six turn type weights.
  PARAMETER(turnLeftProb, 0.0, float, 0.0, 1.0); //Weighted Probability of turning left. This is one of six turn type weights.
  PARAMETER(turnRightProb, 0.0, float, 0.0, 1.0); //Weighted Probability of turning right. This is one of six turn type weights.
  PARAMETER(useBodyTurn, false, bool, false, true);  //enable and provide a positive bodyTurnTimeout and provide a look-at target to make the character turn to face it while balancing
  PARAMETER(upperBodyReaction, true, bool, false, true);  //enable upper body reaction ie blindBrace and armswindmill
}
/*
stumble:  Stumble around and fall2knees/grab/fallOn all fours then collapse
BEHAVIOURS CALLED: BodyBalance, HeadLook, SpineTwist
BEHAVIOURS MODIFIED: DynamicBalancer
BEHAVIOURS REFERENCED: Grab, BalancerCollisionsReaction,Teeter
BodyParts:  Whole body
*/
BEHAVIOUR(stumble)
{
  PARAMETER(torsoStiffness, 9.0f, float, 6.0f, 16.0f); //stiffness of torso when falling
  PARAMETER(legsStiffness, 6.0f, float, 4.0f, 16.0f); //stiffness of legs when falling
  PARAMETER(armsStiffness, 13.0f, float, 6.0f, 16.0f); //stiffness of arms when falling
  PARAMETER(armReduceSpeed, 2.5f, float, 0.0f, 4.0f); //how quickly the arm strength decays
  PARAMETER(armTwist, -0.2f, float, -1.5f, 1.5f); //Arm twist when falling
  PARAMETER(staggerTime, 2.f, float, 0.f, 30.0f); //Max time for stumbling around before falling to ground
  PARAMETER(dropVal, 0.15f, float, 0.f, 15.0f); //speed of falling to knees	
  PARAMETER(injuryRate, 0.15f, float, 0.f, 1.f); //UNUSED Rate at which the body looses strength
  PARAMETER(backwardsMinArmOffset, -0.25f, float, -1.0f, 0.0f); //0 will prop arms up near his shoulders. -0.3 will place hands nearer his behind
  PARAMETER(forwardMaxArmOffset, 0.35f, float, 0.0f, 1.0f); //0 will point arms down with angled body, 0.45 will point arms forward a bit to catch nearer the head
  PARAMETER(zAxisSpinReduction, 0.0f, float, 0.0f, 1.0f); //Tries to reduce the spin around the Z axis. Scale 0 - 1.	
  PARAMETER(twistSpine, 0, int, 0, 2); //0 = no twist, 1 = headlook twist, 2 = pushoff from floor twist
  PARAMETER(dampPelvis, true, bool, false, true); //damp pelvis velocity when controlling spine angle as falls to knees
  PARAMETER(pitchInContact, true, bool, false, true); // leg controls spine angle as falls to knees ONLY if foot in contact (true), always (false)
  PARAMETER(different, true, bool, false, true); //gives a different fall to knees experience (if true finer control of the spine angle as falls to knees)
  PARAMETER(useHeadLook, true, bool, false, true); //Toggle to use the head look in this behaviour.
  PARAMETER(leanRate, 0.01f, float, 0.f, 1.f); //how quickly the character leans hips when staggering
  PARAMETER(maxLeanBack, 0.4f, float, 0.f, 1.5f); //max the character leans hips back when staggering
  PARAMETER(maxLeanForward, 0.8f, float, 0.f, 1.5f); //max the character leans hips forwards when staggering
  PARAMETER(grabRadius2, 6.f, float, 0.f, 20.0f); //if within this SQUARED distance from grabPoint will move towards the grab point
  PARAMETER(leanTowards, 0.1f, float, 0.f, 0.5f); //if within this SQUARED distance from grabPoint will move towards the grab point
  PARAMETER(wristMS, 8.0f, float, 0.5f, 15.0f); //DEVEL wrist muscle stiffness
  PARAMETER(feetMS, 8.f, float, 0.f, 16.5f); //DEVEL ankle muscle stiffness
  PARAMETER(fallMask, "ua", char *, "", ""); //Upper body fall mask, use "ub"/"ua"/"us" etc 
  FEEDBACK(finish,this);//Sent when the character has come to a stop and is fully relaxed. 
  FEEDBACK(success,this);//When the fall has been caught. 
}
/*
teeter:  Will teeter at the edge of a building.  Use with shot, bodyBalance, flinch (staggerFall doesn't do too well with it at the moment)  
Will do a high fall if the balancer fails over the edge.

BEHAVIOURS CALLED: SpineTwist, ArmsWindmill, BodyBalance(deactivated only), Catchfall(deactivated only), bodyWrithe, HeadLook, HighFall
BEHAVIOURS REFERENCED: BalancerCollisionsReaction, Catchfall, DynamicBalancer, Grab 
BEHAVIOURS MODIFIED: DynamicBalancer, RollDownStairs

BodyParts:  wholeBody
*/
BEHAVIOUR(teeter)
{
  PARAMETERV(edgeLeft, 39.47f,38.89f, 21.12f,  0.0, FLT_MAX);//Defines the left edge point (left of character facing edge) 
  PARAMETERV(edgeRight, 39.47f,39.89f, 21.12f,  0.0, FLT_MAX);//Defines the right edge point (right of character facing edge) 
  PARAMETER(useExclusionZone, true, bool, false, true);  //stop stepping across the line defined by edgeLeft and edgeRight
  PARAMETER(callHighFall, true, bool, false, true);  //call highFall if fallen over the edge.  If false just call blended writhe (to go over the top of the fall behaviour of the underlying behaviour e.g. bodyBalance)
  PARAMETER(leanAway, true, bool, false, true);  //lean away from the edge based on velocity towards the edge (if closer than 2m from edge)
}

/*
upperBodyFlinch:  Will brace arms and twist head and body in reaction to medium speed incoming object from a point(xd,yd,zd). 
It will lead with the nearest hand to the incoming object if no hand or both hands are specified. Will look at the target. 
Note: there is no logic about when/when not to brace. 
Note: lowerBody reaction is foetal like if the balancer has failed

BEHAVIOURS CALLED: SpineTwist, HeadLook
BEHAVIOURS MODIFIED: DynamicBalancer (turn, height)
BodyParts: wholeBody (selectable useRightArm, useLeftArm)
*/
BEHAVIOUR(upperBodyFlinch)
{
  PARAMETER(rightHanded, false, bool, false, true);//Lead with the right hand
  PARAMETER(leftHanded, false, bool, false, true);//Lead with the left hand
  PARAMETER(handDistanceLeftRight, 0.1, float, 0.0, 1.0);//Left-Right distance between the hands
  PARAMETER(handDistanceFrontBack, 0.06, float, 0.0, 1.0);//Front-Back distance between the hands
  PARAMETER(handDistanceVertical, 0.1, float, 0.0, 1.0);//Vertical distance between the hands
  PARAMETER(bodyStiffness, 11.0, float, 6.0, 16.0);//stiffness of body. Value carries over to head look, spine twist
  PARAMETER(bodyDamping, 1.0, float, 0.0, 2.0);//damping value used for upper body
  PARAMETER(backBendAmount, -0.55, float, -1.0, 1.0);//Amount to bend the back during the flinch
  PARAMETER(useRightArm, true, bool, false, true);//Toggle to use the right arm.
  PARAMETER(useLeftArm, true, bool, false, true);//Toggle to Use the Left arm
  PARAMETER(noiseScale, 0.1, float, 0.0, 1.0);//Amplitude of the perlin noise applied to the arms positions in the flicnh to the front part of the behaviour.
  PARAMETER(newHit, true, bool, false, true);//Relaxes the character for 1 frame if set.
  PARAMETER(protectHeadToggle, false, bool, false, true);//Always protect head. Note if false then character flinches if target is in front, protects head if target is behind
  PARAMETER(dontBraceHead, false, bool, false, true);//don't protect head only brace from front. Turned on by bcr
  PARAMETER(applyStiffness, true, bool, false, true);//Turned of by bcr
  PARAMETER(headLookAwayFromTarget, false, bool, false, true);//Look away from target (unless protecting head then look between feet)
  PARAMETER(useHeadLook, true, bool, false, true);//Use headlook  
  PARAMETER(turnTowards, 1, int, -2, 2);// +ve balancer turn Towards, negative balancer turn Away, 0 balancer won't turn. NB.There is a 50% chance that the character will not turn even if this parameter is set to turn   
  PARAMETERV0(pos, FLT_MAX); //position in world-space of object to flinch from
}
/*
yanked:  This provides a yanked behaviour.  Used in RDR for a lasso reaction - lasso was either on upperTorso nr spine2/3 or around both feet. 
To provide a whiplash effect from the force applied the stiffness/damping of the upper body can be at a start value for a specified time then it is ramped up from this value to the final value over a specified time.
Blindly tries to grab at where a rope around upper torso would be.
Tries to twist and turn /Leans hips against direction of yank - as though struggling.
Modifies friction on feet to give a sliding effect. 
On floor:  CatchFall with hula/escapologist reaction.  Augmented with a roll over behaviour.

BEHAVIOURS CALLED: DynamicBalance. HeadLook. CatchFall. 
BEHAVIOURS REFERENCED: StaggerFall
*/
BEHAVIOUR(yanked)
{
  PARAMETER(armStiffness, 11.0, float, 6.0, 16.0); //stiffness of arms when upright. 
  PARAMETER(armDamping, 1.0, float, 0.0, 2.0); //Sets damping value for the arms when upright.
  PARAMETER(spineDamping, 1.0, float, 0.0, 2.0);// Spine Damping when upright.
  PARAMETER(spineStiffness, 10.0, float, 6.0, 16.0);//Spine Stiffness  when upright..
  PARAMETER(armStiffnessStart, 3.0, float, 0.0, 16.0); //armStiffness during the yanked timescale ie timeAtStartValues
  PARAMETER(armDampingStart, 0.1, float, 0.0, 2.0); //armDamping during the yanked timescale ie timeAtStartValues
  PARAMETER(spineDampingStart, 0.1, float, 0.0, 2.0); //spineDamping during the yanked timescale ie timeAtStartValues
  PARAMETER(spineStiffnessStart, 3.0, float, 0.0, 16.0); //spineStiffness during the yanked timescale ie timeAtStartValues
  PARAMETER(timeAtStartValues, 0.4, float, 0.0, 2.0); //time spent with Start values for arms and spine stiffness and damping ie for whiplash efffect
  PARAMETER(rampTimeFromStartValues, 0.1, float, 0.0, 2.0); //time spent ramping from Start to end values for arms and spine stiffness and damping ie for whiplash efffect (occurs after timeAtStartValues)
  PARAMETER(stepsTillStartEnd, 2, int, 0, 100); //steps taken before lowerBodyStiffness starts ramping down
  PARAMETER(timeStartEnd, 100.0, float, 0.0, 100.0); //time from start of behaviour before lowerBodyStiffness starts ramping down by perStepReduction1
  PARAMETER(rampTimeToEndValues, 0.0, float, 0.0, 10.0); //time spent ramping from lowerBodyStiffness to lowerBodyStiffnessEnd
  PARAMETER(lowerBodyStiffness, 12.0, float, 0.0, 16.0); //lowerBodyStiffness should be 12
  PARAMETER(lowerBodyStiffnessEnd, 8.0, float, 0.0, 16.0); //lowerBodyStiffness at end
  PARAMETER(perStepReduction, 1.5f, float, 0.f, 10.f); //LowerBody stiffness will be reduced every step to make the character fallover   
  PARAMETER(hipPitchForward, 0.6f, float, -1.3f, 1.3f); // Amount to bend forward at the hips (+ve forward, -ve backwards).  Behaviour switches between hipPitchForward and hipPitchBack
  PARAMETER(hipPitchBack, 1.0, float, -1.3f, 1.3f); // Amount to bend backwards at the hips (+ve backwards, -ve forwards).  Behaviour switches between hipPitchForward and hipPitchBack
  PARAMETER(spineBend, 0.7f, float, 0.0, 1.0); // Bend/Twist the spine amount 
  PARAMETER(footFriction, 1.0, float, 0.0, 10.0); // Foot friction when standing/stepping.  0.5 gives a good slide sometimes 
  PARAMETER(turnThresholdMin, 0.6f, float, -0.1f, 1.0f); //min angle at which the turn with toggle to the other direction (actual toggle angle is chosen randomly in range min to max). If it is 1 then it will never toggle. If negative then no turn is applied.  
  PARAMETER(turnThresholdMax, 0.6f, float, -0.1f, 1.0f); //max angle at which the turn with toggle to the other direction (actual toggle angle is chosen randomly in range min to max). If it is 1 then it will never toggle. If negative then no turn is applied. 
  PARAMETER(useHeadLook, false, bool, false, true); //enable and provide a look-at target to make the character's head turn to face it while balancing
  PARAMETERV0(headLookPos, FLT_MAX); //position of thing to look at; world-space if instance index = -1, otherwise local-space to that object
  PARAMETER(headLookInstanceIndex, -1, int, -1, INT_MAX); //level index of thing to look at
  PARAMETER(headLookAtVelProb, -1.0, float, -1.0, 1.0); // Probability [0-1] that headLook will be looking in the direction of velocity when stepping
  PARAMETER(comVelRDSThresh, 2.0, float, 0.0, 20.0); //for handsAndKnees catchfall ONLY: comVel above which rollDownstairs will start
  PARAMETER(hulaPeriod, 0.25, float, 0.0, 2.0); //0.25 A complete wiggle will take 4*hulaPeriod
  PARAMETER(hipAmplitude, 1.0, float, 0.0, 4.0); //Amount of hip movement
  PARAMETER(spineAmplitude, 1.0, float, 0.0, 4.0); //Amount of spine movement
  PARAMETER(minRelaxPeriod, 0.3, float, -5.0, 5.0); //wriggle relaxes for a minimum of minRelaxPeriod (if it is negative it is a multiplier on the time previously spent wriggling)
  PARAMETER(maxRelaxPeriod, 1.5, float, -5.0, 5.0); //wriggle relaxes for a maximum of maxRelaxPeriod (if it is negative it is a multiplier on the time previously spent wriggling)
  PARAMETER(rollHelp, 0.5, float, 0.0, 1.0); //Amount of cheat torque applied to turn the character over
  PARAMETER(groundLegStiffness, 11, float, 0.0, 16.0); //Leg Stiffness when on the ground
  PARAMETER(groundArmStiffness, 11, float, 0.0, 16.0); //Arm Stiffness when on the ground
  PARAMETER(groundSpineStiffness, 14, float, 0.0, 16.0); //Spine Stiffness when on the ground
  PARAMETER(groundLegDamping, 0.5, float, 0.0, 2.0); //Leg Damping when on the ground
  PARAMETER(groundArmDamping, 0.5, float, 0.0, 2.0); //Arm Damping when on the ground
  PARAMETER(groundSpineDamping, 0.5, float, 0.0, 2.0); //Spine Damping when on the ground
  PARAMETER(groundFriction, 8.f, float, 0.0, 10.0); //Friction multiplier on bodyParts when on ground.  Character can look too slidy with groundFriction = 1.  Higher values give a more jerky reation but this seems timestep dependent especially for dragged by the feet.
}
/*
QUADRAPED BEHAVIOUR ONLY
quadDeath:  Simple death behaviour for the dog and horse rigs.  The horse tries to keep it's head away from the ground.  
The legs use a tendon model so that weak legs crumple in a horse like way.
BodyParts: wholeBody
*/
BEHAVIOUR(quadDeath)
{
  PARAMETER(startStiffness, 11.0, float, 3.0, 16.0);  // Effector stiffness at start of ramping period
  PARAMETER(endStiffness, 3.0, float, 3.0, 16.0);     // Effector stiffness at end of ramping period
  PARAMETER(legStartStiffness, 7.0, float, 3.0, 16.0);// Leg effector stiffness at start of ramping period
  PARAMETER(legEndStiffness, 3.0, float, 3.0, 16.0);  // Leg effector stiffness at start of ramping period
  PARAMETER(bodyRampDuration, 1.0, float, 0.1f, 10.0);// Length (seconds) of relaxation period for spine
  PARAMETER(neckRampDuration, 1.5, float, 0.1f, 10.0);// Length (seconds) of relaxation period for neck
  PARAMETER(legsRampDuration, 2.0, float, 0.1f, 10.0);// Length (seconds) of relaxation period forlegs
  PARAMETER(dampingScale, 0.5, float, 0.0f, 1.0);     // Scales effector damping
  PARAMETER(muscleStiffness, 0.5, float, 0.0, 1.0);   // Muscle stiffness (has a damping-like effect) to ramp to
  PARAMETER(headLift, 1.0, float, 0.0f, 1.0);         // Scales head lift target
  PARAMETER(legsFlexAmount, 0.7, float, 0.0f, 1.0);   // Scales legs collapse (+ is more)
  PARAMETER(legsFlexDuration, 0.6, float, 0.0f, 10.0);// Time (seconds) to reach desired leg flex
  PARAMETER(tendonOffset, 0.0, float, 0.0f, 1.0);     // Sets dead zone for tendon solve
  PARAMETER(helperImpulse, 0.0, float, 0.0f, 25.0);   // Max helper impulse for butt-in-air problem
  PARAMETER(angVelScale, 1.0, float, 0.0f, 1.0);  // How much to scale the angular velocity coming in from animation of a part if it is in angVelScaleMask (otherwise scale by 1.0)
  PARAMETER(angVelScaleMask, "fb", char *, "", ""); //Parts to scale the initial angular velocity by angVelScale (otherwize scale by 1.0)
}
