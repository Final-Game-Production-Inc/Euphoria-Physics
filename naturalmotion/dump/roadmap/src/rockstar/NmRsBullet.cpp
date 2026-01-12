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


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsBullet.h"
#include "NmRsCBU_DynamicBalancer.h" 

#define ACTION_REACTION 0

namespace ART
{
  BulletApplier::BulletProfile BulletApplier::s_eBulletProfile = BulletApplier::downwardSlopeProfile;

  BulletApplier::Setup BulletApplier::s_nextSetup;
  int BulletApplier::s_lastTorqueDir = 0;
  int BulletApplier::s_activeTorqueCount = 0;

  BulletApplier::BulletApplier(NmRsCharacter* character) : AutoState(character)
  {
    // in case somebody forgets to call reset() before using it
    resetCustomVariables();
  };

  void BulletApplier::resetCustomVariables()
  {
    // reset variables
    m_doTorque = false;
    m_doImpulse = false;
    m_doCounterImpulse = false;
    m_doneCounterImpulse = false;
    m_torqueTimer = 0.0f;
    m_impulseTot = 0.0f;
    m_impulseTimer = 0.0f;
    m_counterImpulseTot = 0.0f;
    m_counterImpulseTimer = 0.0f;
    m_impulseMag = 0.f;
    m_impulseScaleInAir = 1.f;
#if NM_ONE_LEG_BULLET
    m_impulseScaleOneLeg = 1.f;
#endif
  }

  bool BulletApplier::entryCondition() 
  { 
    return m_doTorque || m_doImpulse || m_doCounterImpulse; 
  }  

  bool BulletApplier::exitCondition() 
  { 
    return !m_doTorque && !m_doImpulse && !m_doCounterImpulse; 
  }  

  // ---------------------------------------------------------------------------------------------------------
  // enables this substate if appropriate and returns ratio of impulse that should be applied
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::newHit(int partID, const rage::Vector3 &imp, const rage::Vector3 &position, float equalizer)
  {
    //NmRsGenericPart* part = m_character->getGenericPartByIndex(partID);
    //Assertf(part, "Invalid part: %i", partID);

    // 1. part mass compensation, so that impulse would have similar effect, no matter what part was hit?
    // ----------------------------------------------------------------------------------------------------
    rage::Vector3 impulse = imp;
    if(equalizer > 0.0f)
      doPartMassCompensation(partID, impulse, equalizer);

    // if shot was to the head, pass through full impulse...
    // this was a request by design previously but is not longer required!
    //if(part == m_character->getSpineSetup()->getHeadPart())
    //{ 
    //  part->applyImpulse(impulse, position);
    //}
    //// for all non-headshots
    //else
    //{ 
    m_impulse = impulse;
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
    if (m_character->m_bulletImpulseMag>0.f)
    {
      m_impulse.Normalize();
      m_impulse *= m_character->m_bulletImpulseMag;
    }
#endif

    // only when new bullet is started, allow for configuration to change
    m_setup = s_nextSetup;

    // reset, and kick off new extended impulse
    resetCustomVariables();
    m_doImpulse = true;
    m_doCounterImpulse = m_setup.doCounterImpulse;

    // 2. check for torque application. Potentially reduces impulse proportional to character strength!
    // ----------------------------------------------------------------------------------------------------
    if( m_setup.torqueMode != TM_Disabled)
      doTorqueCalculations(partID, m_impulse, position);

    // 3. apply leaky impulse reduction after, so that first impulse comes through unaltered
    // ----------------------------------------------------------------------------------------------------
    float fOrigImpulseMag = m_impulse.Mag();
    m_impulse *= m_character->m_impulseLeakage;
    // subsequent impulses get reduced less the weaker the character (to show stronger impact)
    float cStrengthImpRedGain = m_character->m_impulseLeakageStrengthScaled ? m_character->m_strength : 1.0f;
    m_character->m_impulseLeakage -=  m_character->m_impulseReductionPerShot * cStrengthImpRedGain;
    static float minLeakage = 0.2f;  // This should be made a parameter
    m_character->m_impulseLeakage = rage::Clamp(m_character->m_impulseLeakage, minLeakage, 1.f);

    // save position local to body part for extended impulse !!!
    m_impulsePartID = partID;
    rage::Matrix34 partMat;
    m_character->getGenericPartByIndex(m_impulsePartID)->getMatrix(partMat);
    partMat.UnTransform(position, m_impulsePos);
    //} // if not head shot

    //Work out scaling to be applied based on impulseAirMultStart to impulseAirMax scaling ramp 
    m_impulseMag = m_impulse.Mag();
    m_impulseScaleInAir = 1.f;
    if (m_setup.impulseAirOn && 
      m_impulseMag <  m_setup.impulseAirApplyAbove && //don't scale if really large impulse if the desire is for the character to fall over e.g. shotgun/cannons
      m_impulseMag > m_setup.impulseAirMultStart && //don't scale if impulse below threshold i.e. where impulse is low enough to give a stay on feet/baseline airborne reaction
      m_impulseMag > NM_RS_FLOATEPS)//protect against division by zero
    {
      if (m_impulseMag >= m_setup.impulseAirMax)//but m_impulseMag <  m_setup.impulseAirApplyAbove
        m_impulseScaleInAir = rage::Min(m_impulseMag, m_setup.impulseAirMax)/m_impulseMag;
      else if ((m_setup.impulseAirMax - m_setup.impulseAirMultStart) > 0.000001f)// m_setup.impulseAirMultStart <= m_impulseMag < m_setup.impulseAirMax
        m_impulseScaleInAir = (m_setup.impulseAirMultStart + m_setup.impulseAirMult * (m_impulseMag - m_setup.impulseAirMultStart))/m_impulseMag;
    }
#if NM_ONE_LEG_BULLET
    m_impulseScaleOneLeg = 1.f;
    if (m_setup.impulseAirOn && 
      m_impulseMag <  m_setup.impulseOneLegApplyAbove && //don't scale if really large impulse if the desire is for the character to fall over e.g. shotgun/cannons
      m_impulseMag > m_setup.impulseOneLegMultStart && //don't scale if impulse below threshold i.e. where impulse is low enough to give a stay on feet/baseline airborne reaction
      m_impulseMag > NM_RS_FLOATEPS)//protect against division by zero
    {
      if (m_impulseMag >= m_setup.impulseOneLegMax)//but m_impulseMag <  m_setup.impulseOneLegApplyAbove
        m_impulseScaleOneLeg = rage::Min(m_impulseMag, m_setup.impulseOneLegMax)/m_impulseMag;
      else if ((m_setup.impulseOneLegMax - m_setup.impulseOneLegMultStart) > 0.000001f)// m_setup.impulseOneLegMultStart <= m_impulseMag < m_setup.impulseOneLegMax
        m_impulseScaleOneLeg = (m_setup.impulseOneLegMultStart + m_setup.impulseOneLegMult * (m_impulseMag - m_setup.impulseOneLegMultStart))/m_impulseMag;
    }
#endif //#if NM_ONE_LEG_BULLET

    // Store the current impulse multiplier being used in case the character dies and is switched to a rage ragdoll
    if (fOrigImpulseMag != 0.0f)
      m_character->m_lastImpulseMultiplier = m_impulseMag / fOrigImpulseMag;

    // as it otherwise takes one tick until update:
    applyTorque();
    applyLift();
    applyImpulse();
    applyCounterImpulse();//Note applyCounterImpulse must be after applyImpulse

#if ART_ENABLE_BSPY && BulletBSpyScratchPad
    bspyScratchpad(m_character->getBSpyID(), "Bullet -newHit - impBefore", imp);
    bspyScratchpad(m_character->getBSpyID(), "Bullet -newHit - impAfter", m_impulse);
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // part mass compensation
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::doPartMassCompensation(int partID, rage::Vector3 &impulse, float amount)
  {
    float averageMass = m_character->getTotalMass() / m_character->getNumberOfEffectors();
    float massProp = getPartMass(m_character->getArticulatedBody()->GetLink(partID)) / averageMass;
    impulse *= massProp * amount + 1 * (1.0f - amount);
  }

  // ---------------------------------------------------------------------------------------------------------
  // check if and how much torque to apply
  // implements shockSpin and character strength
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::doTorqueCalculations(int partID, rage::Vector3 &impulse, const rage::Vector3 &position)
  {
    bool isUpperBodyHit = partIsUpperBody(partID);

    // Always spin around body's vertical axis, for aesthetic reasons and stability
    NmRsHumanSpine* spine = m_character->getBody()->getSpine();
    rage::Vector3 torqueVector = spine->getSpine3Part()->getPosition() - 
      spine->getSpine0Part()->getPosition();
    torqueVector.Normalize();

    // torques applied to clavicles are BAD. they make character bend forwards rather then twist.
    // => in this case target spine3 instead (closest part)
    // This is changed before the lever arm otherwize the clavicle centre would be use to work out the direction of twist
    if(m_character->isPartInMask(bvmask_ClavicleLeft |bvmask_ClavicleRight, partID))
    {
      m_torquePartID = spine->getSpine3Part()->getPartIndex();
    }

    // get amount and direction of torque as originally received from impulse
    rage::Vector3 leverArm = position - m_character->getGenericPartByIndex(m_torquePartID)->getPosition();

    if(m_setup.torqueAlwaysSpine3)
    {
      m_torquePartID = spine->getSpine3Part()->getPartIndex();
    }

    // project lever arm on plane perpendicular to torque axis, i.e. subtract the component along the spine axis
    leverArm = leverArm - torqueVector*leverArm.Dot(torqueVector);

    rage::Vector3 realTorque;
    realTorque.Cross(leverArm, impulse);     
    int sign = realTorque.Dot(torqueVector) > 0 ? 1 : -1;

    // apply filter for torques depending on mode
    bool filterOK = true;
    if (TF_LetFinish == m_setup.torqueFilterMode) 
      filterOK = (s_activeTorqueCount == 0);
    else if (TF_OnDirChange == m_setup.torqueFilterMode) 
      filterOK = (sign != s_lastTorqueDir);

    // if we do apply new torque
    if(m_setup.torqueMode != TM_Disabled && isUpperBodyHit && filterOK)
    {
      m_doTorque = true;
      s_activeTorqueCount++;
      m_torqueTimer = 0.0f;

      // modify direction of torque for jittery effect
      if (TS_FlipEachDir == m_setup.torqueSpinMode && s_lastTorqueDir != 0) 
        sign = -s_lastTorqueDir;  // saved in previous tick
      else if (TS_RandomDir == m_setup.torqueSpinMode)
        sign = m_character->getRandom().GetBool() ? 1 : -1;

      // determine amount of torque to apply
      // 1: based on character strength. reduce impulse proportional to torque applied
      if (TM_Strength == m_setup.torqueMode)
      {
        float impToTorqueRatio = m_setup.torqueCutoff + (1-m_setup.torqueCutoff)*(1-m_character->m_strength);
        m_moment = impulse.Mag() * (1-impToTorqueRatio) * sign * m_setup.torqueGain;
        impulse *= impToTorqueRatio;  
      }
      // 2: based on "real" torque around character's up-axis (T_Additive)
      else
      {  
        m_moment = realTorque.Dot(torqueVector) * m_setup.torqueGain;
      }

      // save minimum data needed for extended torques in tick and debug (TODO)
      m_torque = torqueVector * m_moment;
      s_lastTorqueDir = sign;
    }
  }


  // ---------------------------------------------------------------------------------------------------------
  // apply torques extended over time and body parts
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::applyTorque()
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("torque")
#endif

    if(m_doTorque)
    {
      // wait until delay has passed
      if(m_torqueTimer >= m_setup.torqueDelay)
      {
        NmRsGenericPart* torquePart = m_character->getGenericPartByIndex(m_torquePartID);
        Assert(torquePart);

        NmRsHumanSpine* spine = m_character->getBody()->getSpine();
        rage::Vector3 torque = spine->getSpine3Part()->getPosition() - 
          spine->getSpine0Part()->getPosition();
        torque.Normalize();
        torque *= m_moment * 0.25f;   // was calculated and saved in doTorqueCalculations when called by newHit

        // spread out torque a little for extra effect
        torquePart->applyTorque(m_torque);  
        if(m_setup.torqueAlwaysSpine3)
          spine->getSpine2Part()->applyTorque(torque*0.5f);
        spine->getSpine3Part()->applyTorque(m_torque); 
        spine->getSpine1Part()->applyTorque(m_torque); 
        spine->getSpine0Part()->applyTorque(m_torque);

        // reduce after applying, so first one comes through completely
        m_moment *= m_setup.torqueReductionPerTick;

        // debugging, TODO!!
        m_torque = torque;
        m_torquePos = torquePart->getPosition();                  
      }


#if ART_ENABLE_BSPY && BulletBSpyScratchPad
      int characterID = m_character->getBSpyID();
      bspyScratchpad(characterID, m_trqStr, m_doTorque);
      bspyScratchpad(characterID, m_trqStr, m_torque);
      bspyScratchpad(characterID, m_trqStr, m_torquePos);  
      bspyScratchpad(characterID, m_trqStr, m_torqueTimer);
      bspyScratchpad(characterID, m_trqStr, m_torquePartID);
      bspyScratchpad(characterID, m_trqStr, m_moment);
      bspyScratchpad(characterID, m_trqStr, s_lastTorqueDir);
#endif

      // toggle torque
      m_torqueTimer += m_character->getLastKnownUpdateStep();
      if(m_torqueTimer >= (m_setup.torqueDelay + m_setup.torquePeriod) )
      {
        m_doTorque = false;
        s_activeTorqueCount--;
      }

    }
#if ART_ENABLE_BSPY
    bspyProfileEnd("torque")
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // apply impulse extended over time and body parts
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::applyImpulse()
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("impulse")
#endif
    if(m_doImpulse)
    {
      float timeStep = m_character->getLastKnownUpdateStep();
      float impulsePeriod = m_setup.impulsePeriod;

      // check whether current time interval covers the beginning of impulse
      if(m_impulseTimer + timeStep > m_setup.impulseDelay)
      {
        NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_character->getTask(bvid_dynamicBalancer);
        Assert(dynamicBalancerTask);

        bool airborne = false;
        bool oneLegged = false;
        if (m_character->isBiped())
        {
          airborne = dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK &&
            !(m_character->getBody()->getLeftLeg()->getFoot()->collidedWithNotOwnCharacter() ||
            m_character->getBody()->getRightLeg()->getFoot()->collidedWithNotOwnCharacter());
          oneLegged = dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK &&
            !airborne &&             
            (!m_character->getBody()->getLeftLeg()->getFoot()->collidedWithNotOwnCharacter() ||
            !m_character->getBody()->getRightLeg()->getFoot()->collidedWithNotOwnCharacter());
        }

        // shift time by delay (triangle calculations assume triangle covers interval [0,period], ie doesnt take into account the delay
        float timeNow = m_impulseTimer - m_setup.impulseDelay;

        // convert impulse position stored in part's local space to world space
        NmRsGenericPart* part = m_character->getGenericPartByIndex(m_impulsePartID);
        Assertf(part, "Invalid part: %i", m_impulsePartID);
        rage::Matrix34 partMat;
        rage::Vector3 impulsePosWorld;
        part->getMatrix(partMat);
        partMat.Transform(m_impulsePos, impulsePosWorld);

        float forceRatioThisStep = 0.0f;
        if(m_setup.impulsePeriod > 0.0001f)
        {
          switch (s_eBulletProfile)
          {
          case straightProfile:
            forceRatioThisStep = areaUnderStraightProfile(impulsePeriod, timeNow, timeStep);  
            break;
          case triangleProfile:
            forceRatioThisStep = areaUnderTriangleProfile(m_setup.impulsePeriod, timeNow, timeNow + timeStep);   
            break;
          case downwardSlopeProfile:
            forceRatioThisStep = areaUnderDownwardSlopeProfile(m_setup.impulsePeriod, timeNow, timeNow + timeStep);  
            break;
          default:
            break;
          }
        }
        else if (m_setup.impulsePeriod > 0.000001f)
          forceRatioThisStep = 1.0f; // all in one shot
        else
          forceRatioThisStep = 0.f;//no force

        rage::Vector3 forceNow = m_impulse*forceRatioThisStep;

        m_impulseTot += forceNow.Mag();

        if (m_character->isBiped())
        {
          if (airborne)
          {            
            forceNow = m_impulseScaleInAir * m_impulse * forceRatioThisStep;
          }
#if NM_ONE_LEG_BULLET
          else if (oneLegged)
          {            
            forceNow = m_impulseScaleOneLeg * m_impulse * forceRatioThisStep;
          }
#endif //#if NM_ONE_LEG_BULLET
        }
#if ART_ENABLE_BSPY && BulletBSpyScratchPad
        {
          int characterID = m_character->getBSpyID();
          bspyScratchpad(characterID, m_impStr, airborne);
        }
#endif
#if ART_ENABLE_BSPY && BulletBSpyScratchPad
        {
          int characterID = m_character->getBSpyID();
          bspyScratchpad(characterID, m_impStr, oneLegged);
        }
#endif
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        forceNow *= m_character->bulletImpulseMult;
#endif
#if NM_RIGID_BODY_BULLET
        float rbRatio = m_setup.rbRatio;
        //float rbForce = m_setup.rbForce;
        float rbMoment = m_setup.rbMoment;
        if (airborne)
        {
          rbRatio = m_setup.rbRatioAirborne;
          rbMoment = m_setup.rbMomentAirborne;
        }
        else if (oneLegged)
        {
          rbRatio = m_setup.rbRatioOneLeg;
          rbMoment = m_setup.rbMomentOneLeg;
        }

#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (m_character->rbRatio > -0.00001f)
          rbRatio = m_character->rbRatio;
        //rbForce = m_character->rbForce;
        if (m_character->rbMoment > -0.00001f)
          rbMoment = m_character->rbMoment;
#endif
#if ART_ENABLE_BSPY && BulletBSpyScratchPad
        {
          int characterID = m_character->getBSpyID();
          bspyScratchpad(characterID, m_impStr, rbRatio);
          bspyScratchpad(characterID, m_impStr, rbMoment);
        }
#endif

        //apply a character acceleration force
        int i;
        float massProp;
        rage::Vector3 impulse;
        // Linear Accel of Character in N
        rage::Vector3 N_a_Char = rbRatio*forceNow/m_character->getTotalMass();
        rage::Vector3 N_a_CharUpper;
        rage::Vector3 N_a_CharLower;
        //don't split up - div by 0 or rbLowerShare = 0.5
        bool splitRBForce = true;
        float rbLowerShare = m_setup.rbLowerShare;
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (m_character->rbLowerShare > -0.00001f)
          rbLowerShare = m_character->rbLowerShare;
#endif
        rbLowerShare = rage::Clamp(rbLowerShare, 0.f,1.f);
        float lowerShare = 0.5f;//this value shouldn't ever be used - just to keep xbox compiler happy
        if (fabs(m_character->getUpperBodyMass()) < 1e-8 || fabs(m_character->getLowerBodyMass()) < 1e-8 ||
          fabs(rbLowerShare-0.5f) < 0.00001f)
          splitRBForce = false;
        if (splitRBForce)
        {
          float forceNowAdjusted = (1.f - getPartMass(m_character->getArticulatedBody()->GetLink(m_impulsePartID))/m_character->getTotalMass());
          float upperBodyMass = m_character->getUpperBodyMass();
          float lowerBodyMass = m_character->getLowerBodyMass();
          if (m_impulsePartID < 8)
            lowerBodyMass -= getPartMass(m_character->getArticulatedBody()->GetLink(m_impulsePartID));
          else
            upperBodyMass -= getPartMass(m_character->getArticulatedBody()->GetLink(m_impulsePartID));

          if (m_setup.rbLowerShare >= 0.5f)
          {
            lowerShare = lowerBodyMass + 2.f*(rbLowerShare - 0.5f)*upperBodyMass;
          }
          else
          {
            lowerShare = lowerBodyMass - 2.f*(0.5f - rbLowerShare)*lowerBodyMass;
          }
          lowerShare /= (lowerBodyMass+upperBodyMass);//i.e. /(m_character->getTotalMass() - getPartMass(m_character->getArticulatedBody()->GetLink(m_impulsePartID)))
          N_a_CharUpper = rbRatio*(1.f-lowerShare)*forceNowAdjusted*forceNow/upperBodyMass;
          N_a_CharLower = rbRatio*lowerShare*forceNowAdjusted*forceNow/lowerBodyMass;
        }
        // Angular Accel of Character in N
        rage::Vector3 N_alpha_Char;
        rage::Vector3 pivotPoint(m_character->m_COM);
        rage::Matrix34 characterInertiaAboutPivotInN(m_character->m_characterInertiaAboutComInN);
#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
        if (m_character->rbPivot)
        {
          m_character->getCharacterInertiaAboutPivot(&characterInertiaAboutPivotInN, &pivotPoint);
        }
#else
        if (m_setup.rbPivot)
        {
          m_character->getCharacterInertiaAboutPivot(&characterInertiaAboutPivotInN, &pivotPoint);
        }
#endif

        //work out the Moment on the character about COM caused by the impulse
        rage::Vector3 d = impulsePosWorld - pivotPoint; 
        N_alpha_Char.Cross(d,forceNow);
        if (forceNow.Mag() >  1e-10f)//protect against division by zero
        {
          //Clamp the moment arm for twist and broom movement
          //Twist could be about up direction, characterComUp, spine
          rage::Vector3 twistAxis = m_character->m_gUp;
          //Clamp the twist moment arm 
          int rbTwistAxis = m_setup.rbTwistAxis;
          float rbMaxTwistMomentArm = m_setup.rbMaxTwistMomentArm;
          float rbMaxBroomMomentArm = m_setup.rbMaxBroomMomentArm;
          if (airborne)
          {
            rbMaxTwistMomentArm = m_setup.rbMaxTwistMomentArmAirborne;
            rbMaxBroomMomentArm = m_setup.rbMaxBroomMomentArmAirborne;
          }
          else if (oneLegged)
          {
            rbMaxTwistMomentArm = m_setup.rbMaxTwistMomentArmOneLeg;
            rbMaxBroomMomentArm = m_setup.rbMaxBroomMomentArmOneLeg;
          }

#if NM_SCRIPTING && NM_SCRIPTING_VARIABLES
          if (m_character->rbTwistAxis >= 0)
            rbTwistAxis = m_character->rbTwistAxis;
          if (m_character->rbMaxTwistMomentArm > -0.00001f)
            rbMaxTwistMomentArm = m_character->rbMaxTwistMomentArm;
          if (m_character->rbMaxBroomMomentArm > -0.00001f)
            rbMaxBroomMomentArm = m_character->rbMaxBroomMomentArm;
#endif
          if (rbTwistAxis == 1)
            twistAxis = m_character->m_COMTM.b;
          float twistMomentArm = N_alpha_Char.Dot(twistAxis);
          rage::Vector3 broomMoment = N_alpha_Char - twistMomentArm*twistAxis;
          twistMomentArm /= forceNow.Mag();
          float broomMomentArm = broomMoment.Mag()/forceNow.Mag();
          broomMoment.Normalize();
#if ART_ENABLE_BSPY && BulletBSpyScratchPad
          int characterID = m_character->getBSpyID();
          bspyScratchpad(characterID, m_impStr, twistMomentArm);
          bspyScratchpad(characterID, m_impStr, broomMomentArm);
#endif
          twistMomentArm = rage::Clamp(twistMomentArm,-rbMaxTwistMomentArm, rbMaxTwistMomentArm);
          //Clamp the other (broom) moment arm
          broomMomentArm = rage::Clamp(broomMomentArm,-rbMaxBroomMomentArm, rbMaxBroomMomentArm);
          N_alpha_Char = twistMomentArm*twistAxis + broomMomentArm*broomMoment;
          N_alpha_Char *= forceNow.Mag();
        }

        N_alpha_Char *= rbRatio;

        //N_alpha_Char = Inverse(characterInertiaAboutPivotInN)*Moment;
        rage::Matrix34 invCharacterInertiaAboutComInN(characterInertiaAboutPivotInN);
        invCharacterInertiaAboutComInN.Inverse3x3();
        invCharacterInertiaAboutComInN.Transform3x3(N_alpha_Char);
        //why is this not the same as above?characterInertiaAboutPivotInN.UnTransform3x3(N_alpha_Char);

        rage::Vector3 COMrotvel(m_character->m_COMrotvel);
        //rage::Vector3 COMrotvel(m_character->m_angMom);
        //invCharacterInertiaAboutComInN.Transform3x3(COMrotvel);

        rage::Vector3 torque(0,0,0);
        rage::Vector3 NaB(0,0,0);

        for (i=0; i<m_character->getNumberOfParts(); i++)
        {
          massProp = getPartMass(m_character->getArticulatedBody()->GetLink(i));
          //NaB = N_a_Char + comAngVel x (comAngVel x rCom2B) + N_alpha_Char x rCom2B
          rage::Vector3 rCom2B = m_character->getGenericPartByIndex(i)->getPosition() - pivotPoint; 
          rage::Vector3 N_alpha_CharXrCom2B; 
          N_alpha_CharXrCom2B.Cross(N_alpha_Char,rCom2B);
          NaB.Zero();
          //Why doesn't this work? 
          //NaB.Cross(COMrotvel,rCom2B);
          //NaB.Cross(COMrotvel);
          //NaB *=-rbRatio;
#define DISPLAY_COMPONENTS 0
#if DISPLAY_COMPONENTS
			NaB += /*rbForce**/N_a_Char + rbMoment*N_alpha_CharXrCom2B;
#if ART_ENABLE_BSPY
			{
			  rage::Vector3 force = massProp * N_a_Char;
			  float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
			  force /= timeStepClamped*60.f;
			  m_character->bspyDrawLine(m_character->getGenericPartByIndex(i)->getPosition(), m_character->getGenericPartByIndex(i)->getPosition() + force, rage::Vector3(1.f, 1.f, 1.f));
			  force = massProp * rbMoment*N_alpha_CharXrCom2B;
			  force /= timeStepClamped*60.f;
			  m_character->bspyDrawLine(m_character->getGenericPartByIndex(i)->getPosition(), m_character->getGenericPartByIndex(i)->getPosition() + force, rage::Vector3(1.f, 0.f, 0.f));
			}
#endif
           
#else
          if (splitRBForce && m_impulsePartID != i)
          {
            if (i<8)//lowerBody
              NaB += /*rbForce**/N_a_CharLower + rbMoment*N_alpha_CharXrCom2B*lowerShare;//*lowerShare is an approximation
            else
              NaB += /*rbForce**/N_a_CharUpper + rbMoment*N_alpha_CharXrCom2B*(1.f - lowerShare);//*(1.f - lowerShare) is an approximation
          }
          else
            NaB += /*rbForce**/N_a_Char + rbMoment*N_alpha_CharXrCom2B;
#endif

          impulse = massProp * NaB;
          m_character->getGenericPartByIndex(i)->applyImpulse(impulse, m_character->getGenericPartByIndex(i)->getPosition());

          m_character->m_bpInertiaMatrixAboutBpInN[i].Transform3x3(N_alpha_Char,torque);
          m_character->getGenericPartByIndex(i)->applyTorqueImpulse(torque*rbMoment);
        }

        forceNow *= (1.f - rbRatio);
#endif//#if NM_RIGID_BODY_BULLET

        // spread out impulse over parts
        if(m_setup.impulseSpreadOverParts)
        {
          int n1, n2;
          if(getNeighbours(m_impulsePartID, n1, n2))
          {
            rage::Vector3 ipos;

            // part 1
            m_character->getGenericPartByIndex(n1)->getMatrix(partMat);
            partMat.Transform(m_impulsePos, ipos);           
#if NM_BULLETIMPULSEFIX
#if ART_ENABLE_BSPY && BulletBSpyDraw
            //Draw the original bullet
            rage::Vector3 force = forceNow*0.25f;
            float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
            force /= timeStepClamped*60.f;
            m_character->bspyDrawLine(ipos, ipos + force, rage::Vector3(1.f,1.f, 0.f));
#endif
            rage::Vector3 moment;
            moment.Cross(ipos - partMat.d,forceNow);

            if (m_setup.loosenessFix)
            {
              //getNeighbours ensures that is always pelvis or spine
              float magA = moment.Dot(partMat.a);
              moment =  magA * partMat.a; 
            }
            moment *= m_setup.impulseTorqueScale;
            m_character->getGenericPartByIndex(n1)->applyImpulse(forceNow*0.25f, m_character->getGenericPartByIndex(n1)->getPosition());
            m_character->getGenericPartByIndex(n1)->applyTorqueImpulse(moment*0.25f);
#else
            m_character->getGenericPartByIndex(n1)->applyImpulse(forceNow*0.25f, ipos);
#endif
            // part 2
            m_character->getGenericPartByIndex(n2)->getMatrix(partMat);
            partMat.Transform(m_impulsePos, ipos);            
#if NM_BULLETIMPULSEFIX
#if ART_ENABLE_BSPY && BulletBSpyDraw
            //Draw the original bullet
            force = forceNow*0.25f;
            timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
            force /= timeStepClamped*60.f;
            m_character->bspyDrawLine(ipos, ipos + force, rage::Vector3(1.f,1.f, 0.f));
#endif
            moment.Cross(ipos - partMat.d,forceNow);

            if (m_setup.loosenessFix)
            {
              //getNeighbours ensures that is always pelvis or spine
              float magA = moment.Dot(partMat.a);
              moment =  magA * partMat.a; 
            }
            moment *= m_setup.impulseTorqueScale;
            m_character->getGenericPartByIndex(n2)->applyImpulse(forceNow*0.25f, m_character->getGenericPartByIndex(n2)->getPosition());
            m_character->getGenericPartByIndex(n2)->applyTorqueImpulse(moment*0.25f);
#else
            m_character->getGenericPartByIndex(n2)->applyImpulse(forceNow*0.25f, ipos);
#endif
            forceNow *= 0.5f;
          } // if part has neighbours
        } // impulseSpreadOverParts

#if ACTION_REACTION
        bool actionReaction = false;
        bool bulletWithMovement = (forceNow.Dot(m_character->m_COMvel) > 0.f);
        if (bulletWithMovement && !(m_character->getLeftLegSetup()->getFoot()->collidedWithNotOwnCharacter()) && !(m_character->getRightLegSetup()->getFoot()->collidedWithNotOwnCharacter()))
        {
          actionReaction = true;
        }
        if (actionReaction)
          forceNow *= 0.75f;
#endif
#if NM_BULLETIMPULSEFIX
        //Split this force on a bodyPart applied at a point on the bodyPart into:
        //a force at the centre of the part
        //and a moment appled to the part

#if ART_ENABLE_BSPY && BulletBSpyDraw
        //Draw the original bullet
        rage::Vector3 force = forceNow;
        float timeStepClamped = rage::Max(m_character->getLastKnownUpdateStep(), SMALLEST_TIMESTEP);//protect against division by zero
        force /= timeStepClamped*60.f;
        m_character->bspyDrawLine(impulsePosWorld, impulsePosWorld + force, rage::Vector3(1.f,1.f, 0.f));
#endif

        rage::Vector3 moment;
        moment.Cross(impulsePosWorld - partMat.d,forceNow);

        if (m_setup.loosenessFix)
        {
          //Very severe implementation of bug Fix for 
          //  "RAGE looseness bug",
          //  formerly known as "Lateral Movement Physics Bug"
          //  "Ensure enemies fall in correct direction when hit" MAXP-329
          //Description: characters with low muscleStiffness (i.e. loose) do not move in the direction of an applied impulse
          //  To a frontal (esp. to the outside of spine2/3) hit that should move them backwards:
          //  they move completely laterally / pop up into the air / move forwards
          //Fix:
          //  allow twist torque only of spine and hip parts
          //  disallow torque for every other part
          float magA = moment.Dot(partMat.a);
          moment.Zero();
          NmRsHumanSpine* spine = m_character->getBody()->getSpine();
          if (m_impulsePartID  == spine->getPelvisPart()->getPartIndex()
            || m_impulsePartID == spine->getSpine0Part()->getPartIndex()
            || m_impulsePartID == spine->getSpine1Part()->getPartIndex()
            || m_impulsePartID == spine->getSpine2Part()->getPartIndex()
            || m_impulsePartID == spine->getSpine3Part()->getPartIndex())
          {
            moment +=  magA * partMat.a; 
          }

          moment *= m_setup.impulseTorqueScale;
          part->applyImpulse(forceNow, part->getPosition());
          part->applyTorqueImpulse(moment);
        }
        else
        {
          part->applyImpulse(forceNow, impulsePosWorld);
        }
#else
        part->applyImpulse(forceNow, impulsePosWorld);
#endif

#if ACTION_REACTION
        if (actionReaction)
        {
          forceNow *= 0.5f;
          part = m_character->getGenericPartByIndex(16);
          part->applyImpulse(forceNow, part->getPosition());
          part = m_character->getGenericPartByIndex(19);
          part->applyImpulse(forceNow, part->getPosition());
          forceNow *= 0.5f/0.75f/0.5f;
          part = m_character->getGenericPartByIndex(14);
          part->applyImpulse(-forceNow, part->getPosition());
        }
#endif


        //Start the counter impulse if the magnitude of the impulse applied so far is >= counterImpulseMag*m_impulse
        if ((m_impulseTot >= m_impulse.Mag()*m_setup.counterImpulseMag) && (m_setup.doCounterImpulse && m_setup.counterAfterMagReached && !m_doCounterImpulse && !m_doneCounterImpulse))
          m_doCounterImpulse = true;

#if ART_ENABLE_BSPY && BulletBSpyScratchPad
        int characterID = m_character->getBSpyID();
#if NM_SCRIPTING
        bspyScratchpad(characterID, m_impStr, m_character->m_simTime);
#endif//NM_SCRIPTING
        bspyScratchpad(characterID, m_impStr, timeStep);
        bspyScratchpad(characterID, m_impStr, m_doImpulse);
        bspyScratchpad(characterID, m_impStr, m_impulse);    
        //mmmmtodo not the true force now e.g. is true*0.5 if spread over parts
        //mmmmtodo not requested force now e.g. reduced by inAir and
        bspyScratchpad(characterID, m_impStr, forceNow);    
        bspyScratchpad(characterID, m_impStr, impulsePosWorld);    
        bspyScratchpad(characterID, m_impStr, m_impulse.Mag());
        bspyScratchpad(characterID, m_impStr, m_impulsePos);    
        bspyScratchpad(characterID, m_impStr, m_impulsePartID);      
        bspyScratchpad(characterID, m_impStr, m_impulseTimer);      
        bspyScratchpad(characterID, m_impStr, m_character->m_impulseLeakage); 
        bspyScratchpad(characterID, m_impStr, forceRatioThisStep);
        bspyScratchpad(characterID, m_impStr, forceNow.Mag());
        bspyScratchpad(characterID, m_impStr, m_impulseTot);          
        bspyScratchpad(characterID, m_impStr, m_impulseScaleInAir);          
#if NM_ONE_LEG_BULLET
        bspyScratchpad(characterID, m_impStr, m_impulseScaleOneLeg);  
#endif //#if NM_ONE_LEG_BULLET
#endif
      }      

      // toggle impulse
      m_impulseTimer += timeStep;
      if(m_impulseTimer >= (m_setup.impulseDelay + impulsePeriod))
        m_doImpulse = false;
    } // if do impulse

#if ART_ENABLE_BSPY
    bspyProfileEnd("impulse")
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // apply counter impulse to pelvis (extended for half the impulse period - mmmmtodo parameterize later?)
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::applyCounterImpulse()
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("counterImpulse")
#endif
    if(m_doCounterImpulse)
    {
      float timeStep = m_character->getLastKnownUpdateStep();

      // check whether current time interval covers the beginning of impulse
      if( m_counterImpulseTimer + timeStep > (m_setup.impulseDelay + m_setup.counterImpulseDelay) )
      {
        // shift time by delay (triangle calculations assume triangle covers interval [0,period], ie doesnt take into account the delay
        float timeNow = m_counterImpulseTimer - m_setup.impulseDelay - m_setup.counterImpulseDelay;

        // convert impulse position stored in part's local space to world space
        NmRsGenericPart* part = m_character->getBody()->getSpine()->getPelvisPart();
        rage::Matrix34 partMat;
        part->getMatrix(partMat);
        rage::Vector3 impulsePosWorld = partMat.d;

        // get are under curve for this timestep //mmmmmtodo /2.f should be *counterImpulseMag)
        float forceRatioThisStep = areaUnderTriangleProfile(m_setup.impulsePeriod/2.f, timeNow, timeNow + timeStep);        
        rage::Vector3 forceNow = m_impulse*forceRatioThisStep*m_setup.counterImpulseMag;
        bool bulletWithMovement = (m_impulse.Dot(m_character->m_COMvel) > 0.f);

        //Counter Impulse is the impulse negated, leveled and scaled by counterImpulseMag
        forceNow.Negate();
        m_character->levelVector(forceNow);
        forceNow *= m_setup.counterImpulseMag;

        float counterImpulse2Hips = m_setup.counterImpulse2Hips;
        if (!(m_character->getBody()->getLeftLeg()->getFoot()->collidedWithNotOwnCharacter()) && !(m_character->getBody()->getRightLeg()->getFoot()->collidedWithNotOwnCharacter()))
        {
          counterImpulse2Hips = 0.f;
          //old code did m_setup.counterImpulse2Hips = 0.f;
        }

        if (bulletWithMovement)
          part->applyImpulse(forceNow*counterImpulse2Hips, impulsePosWorld);

        part = m_character->getGenericPartByIndex(m_impulsePartID);
        Assertf(part, "Invalid part: %i", m_impulsePartID);
        part->getMatrix(partMat);
        partMat.Transform(m_impulsePos, impulsePosWorld);
        if (bulletWithMovement)
          part->applyImpulse(forceNow*(1.f-counterImpulse2Hips), impulsePosWorld);

        m_counterImpulseTot += forceNow.Mag();          

#if ART_ENABLE_BSPY && BulletBSpyScratchPad
        bspyScratchpad(m_character->getBSpyID(), m_ctrStr, timeStep);
        bspyScratchpad(m_character->getBSpyID(), m_ctrStr, forceRatioThisStep);
        bspyScratchpad(m_character->getBSpyID(), m_ctrStr, forceNow.Mag());
        bspyScratchpad(m_character->getBSpyID(), m_ctrStr, forceNow);
        bspyScratchpad(m_character->getBSpyID(), m_ctrStr, m_counterImpulseTot);  
#endif
      }      

      // toggle counter impulse
      m_counterImpulseTimer += timeStep;//mmmmmtodo /2.f should be *counterImpulseMag)
      if(m_counterImpulseTimer >= (m_setup.impulseDelay + m_setup.counterImpulseDelay + m_setup.impulsePeriod/2.f))
      {
        m_doCounterImpulse = false;
        m_doneCounterImpulse = true;
      }
    } // if do counterImpulse
#if ART_ENABLE_BSPY
    bspyProfileEnd("counterImpulse")
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // apply optional lift to character
  // ---------------------------------------------------------------------------------------------------------
  void BulletApplier::applyLift()
  {
#if ART_ENABLE_BSPY
    bspyProfileStart("lift")
#endif
    //mmmmmtodo
    //Should be in up direction
    //Should be scaled with character weight: liftForce *= m_setup.liftGain*m_character->getTotalMass()*9.81f
    //Should decay or be timed or function of bullet force
    if (m_setup.liftGain > 0.0f)
    {
      // same as torque axis
      NmRsHumanSpine* spine = m_character->getBody()->getSpine();
      rage::Vector3 liftForce = spine->getSpine3Part()->getPosition() - 
        spine->getSpine0Part()->getPosition();
      liftForce.Normalize();
      liftForce *= m_setup.liftGain;
      spine->getSpine2Part()->applyForce(0.5f*liftForce);
      spine->getSpine3Part()->applyForce(0.25f*liftForce);
      spine->getSpine1Part()->applyForce(0.25f*liftForce);
    }
#if ART_ENABLE_BSPY
    bspyProfileEnd("lift")
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // update impulse and torques
  // --------------------------------------------------------------------------------------------------------- 
  void BulletApplier::update(float UNUSED_PARAM(timeStep)) 
  { 
#if ART_ENABLE_BSPY
    bspyProfileStart("update")
#endif

    applyTorque();
    applyImpulse();
    applyCounterImpulse();//Note applyCounterImpulse must be after applyImpulse
    applyLift();

#if ART_ENABLE_BSPY && BulletBSpyScratchPad
    int characterID = m_character->getBSpyID();

    bspyScratchpad(characterID, m_trqStr, m_doTorque);
    bspyScratchpad(characterID, m_idStr, s_lastTorqueDir);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseDelay);
    bspyScratchpad(characterID, m_idStr, m_setup.impulsePeriod);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseSpreadOverParts);
    bspyScratchpad(characterID, m_idStr,  m_character->m_impulseReductionPerShot);
    bspyScratchpad(characterID, m_idStr,  m_character->m_impulseRecovery);
    bspyScratchpad(characterID, m_idStr,  m_character->m_impulseLeakageStrengthScaled);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueGain);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueCutoff);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueReductionPerTick);
    bspyScratchpad(characterID, m_idStr, m_setup.torquePeriod);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueDelay);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueMode);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueSpinMode);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueFilterMode);
    bspyScratchpad(characterID, m_idStr, m_setup.torqueAlwaysSpine3);     
    bspyScratchpad(characterID, m_idStr, m_setup.doCounterImpulse);
    bspyScratchpad(characterID, m_idStr, m_setup.counterAfterMagReached);
    bspyScratchpad(characterID, m_idStr, m_setup.loosenessFix);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseTorqueScale);      
    bspyScratchpad(characterID, m_idStr, m_setup.counterImpulseDelay);
    bspyScratchpad(characterID, m_idStr, m_setup.counterImpulseMag);
    bspyScratchpad(characterID, m_idStr, m_setup.counterImpulse2Hips);   
    bspyScratchpad(characterID, m_idStr, m_setup.liftGain);   

    bspyScratchpad(characterID, m_idStr,  m_setup.impulseAirOn);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseAirMult);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseAirMultStart);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseAirMax);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseAirApplyAbove);
#if NM_ONE_LEG_BULLET
    bspyScratchpad(characterID, m_idStr,  m_setup.impulseOneLegOn);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseOneLegMult);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseOneLegMultStart);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseOneLegMax);
    bspyScratchpad(characterID, m_idStr, m_setup.impulseOneLegApplyAbove);
#endif//#if NM_ONE_LEG_BULLET
#if NM_RIGID_BODY_BULLET
    bspyScratchpad(characterID, m_idStr,  m_setup.rbPivot);
    bspyScratchpad(characterID, m_idStr, m_setup.rbTwistAxis);
    bspyScratchpad(characterID, m_idStr, m_setup.rbRatio);
    bspyScratchpad(characterID, m_idStr, m_setup.rbLowerShare);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMoment);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxTwistMomentArm);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxBroomMomentArm);
    bspyScratchpad(characterID, m_idStr, m_setup.rbRatioAirborne);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMomentAirborne);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxTwistMomentArmAirborne);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxBroomMomentArmAirborne);
    bspyScratchpad(characterID, m_idStr, m_setup.rbRatioOneLeg);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMomentOneLeg);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxTwistMomentArmOneLeg);
    bspyScratchpad(characterID, m_idStr, m_setup.rbMaxBroomMomentArmOneLeg);
#endif //#if NM_RIGID_BODY_BULLET

#endif

#if ART_ENABLE_BSPY
    bspyProfileEnd("update")
#endif
  }

  // ---------------------------------------------------------------------------------------------------------
  // helper function for cleaner code above
  // ---------------------------------------------------------------------------------------------------------

  // area under triangle curve in interval [a,b]. assumes unit area triangle. w = width of triangle
  float BulletApplier::areaUnderTriangleProfile(float w, float a, float b)
  {
    Assertf(a<w && b>0 && a<b, "Triangle params invalid: base = %f | a = %f | b = %f", w, a, b);

    // clamp to ends of triangle. As it's made of two lines, we could get negative results for area if we didn't clamp
    if(a < 0) a = 0;
    if(b > w) b = w;

    float midpoint = w/2;

    // get area: A(f,a,b) under a line f is [f(a) + f(b)]/2 * (b-a)
    // if both ends of the interval lie on the same side of the midpoint, i.e. interval is not covering the peak:
    if ( (a<midpoint && b<midpoint) ||
      (a>midpoint && b>midpoint) )
    {
      return (triangleAt(a,w) + triangleAt(b,w)) / 2.0f * (b-a);
    }
    // if interval covers peak, break up into two intervals, each basically a line segment:
    else
    {
      float area1 = (triangleAt(a,w) + triangleAt(midpoint,w)) / 2.0f * (midpoint-a);
      float area2 = (triangleAt(midpoint,w) + triangleAt(b,w)) / 2.0f * (b-midpoint);
      return area1 + area2;
    }
  }

  float BulletApplier::areaUnderStraightProfile(float impulsePeriod, float timeNow, float timeStep)
  {
    float impulseTimeLeft = impulsePeriod - timeNow;
    if (timeStep < impulseTimeLeft)
      return timeStep/impulsePeriod;
    else
      return impulseTimeLeft/impulsePeriod;   
  }

  float BulletApplier::areaUnderDownwardSlopeProfile(float w, float a, float b)
  {
    Assertf(a<w && b>0 && a<b, "Triangle params invalid: base = %f | a = %f | b = %f", w, a, b);

    // clamp to ends of triangle. As it's made of two lines, we could get negative results for area if we didn't clamp
    if(a < 0) a = 0;
    if(b > w) b = w;

    float heightA = 1.0f - (a / w);
    float heightB = 1.0f - (b / w);
    float averageHeight = (heightA + heightB) / 2.0f;
    float delta = b-a;
    float normSeg = delta / w;
    return 2.0f * averageHeight * normSeg;  // multiplied 2 so that the total area under the downward slope is 1.0
  }

  // assumes triangle starts ascending leg at x=0, and has equal sides (ie peak in the middle)
  // as well as unit area
  float BulletApplier::downwardSlopeAt(float x, float w)
  {
    Assertf(x>=0 && x<=w, "Triangle params invalid: x = %f | w = %f", x, w);

    // from triangle base w, get its height h given that its area A = 1
    float h = 2.0f / w;

    // calculate slope of triangle sides: m = h/(b/2) = 2*(h/b), assumes peak is in the middle.
    float m = 2.0f*h/w;

    // left of peak, i.e. ascending side
    if(x < w/2)
      return m*x;
    else
      return h - m*(x - w/2);      
  }

  // assumes triangle starts ascending leg at x=0, and has equal sides (ie peak in the middle)
  // as well as unit area
  float BulletApplier::triangleAt(float x, float w)
  {
    Assertf(x>=0 && x<=w, "Triangle params invalid: x = %f | w = %f", x, w);

    // from triangle base w, get its height h given that its area A = 1
    float h = 2.0f / w;

    // calculate slope of triangle sides: m = h/(b/2) = 2*(h/b), assumes peak is in the middle.
    float m = 2.0f*h/w;

    // left of peak, i.e. ascending side
    if(x < w/2)
      return m*x;
    else
      return h - m*(x - w/2);      
  }

  // these are considered for torque application
  bool BulletApplier::partIsUpperBody(int partID)
  {
    //Removed upperarm from torque reaction as is covered by armShot in RDR
    NmRsHumanSpine* spine = m_character->getBody()->getSpine();
    return
      partID == spine->getSpine0Part()->getPartIndex() ||
      partID == spine->getSpine1Part()->getPartIndex() ||
      partID == spine->getSpine2Part()->getPartIndex() ||
      partID == spine->getSpine3Part()->getPartIndex() ||
      partID == m_character->getBody()->getLeftArm()->getClaviclePart()->getPartIndex() ||
      partID == m_character->getBody()->getRightArm()->getClaviclePart()->getPartIndex();
  }

  // get 2 "neighbouring" parts to spread impulse to
  bool BulletApplier::getNeighbours(int partID, int& n1, int& n2)
  {
    NmRsHumanSpine* spine = m_character->getBody()->getSpine();
    if(partID == spine->getPelvisPart()->getPartIndex())
    {
      n1 = spine->getSpine0Part()->getPartIndex();
      n2 = spine->getSpine1Part()->getPartIndex(); 
      return true;
    }
    else if(partID == spine->getSpine0Part()->getPartIndex())
    {
      n1 = spine->getPelvisPart()->getPartIndex();
      n2 = spine->getSpine1Part()->getPartIndex();          
      return true;
    }
    else if(partID == spine->getSpine1Part()->getPartIndex())
    {
      n1 = spine->getSpine0Part()->getPartIndex();
      n2 = spine->getSpine2Part()->getPartIndex();          
      return true;
    }
    else if(partID == spine->getSpine2Part()->getPartIndex())
    {
      n1 = spine->getSpine1Part()->getPartIndex();
      n2 = spine->getSpine3Part()->getPartIndex();          
      return true;
    }
    else if(partID == spine->getSpine3Part()->getPartIndex())
    {
      n1 = spine->getSpine1Part()->getPartIndex();
      n2 = spine->getSpine2Part()->getPartIndex();          
      return true;
    }
    else
    {
      return false;
    }
  }

} // nms ART
