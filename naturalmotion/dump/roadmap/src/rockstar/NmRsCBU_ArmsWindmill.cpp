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
 * Arms pedalling or bicycle pedalling type motion
 * 
 */


#include "NmRsInclude.h"
#include "NmRsCBU_ArmsWindmill.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsEngine.h"
#include "NmRsGenericPart.h"

#define PI2 6.2831853f

namespace ART
{
  NmRsCBUArmsWindmill::NmRsCBUArmsWindmill(ART::MemoryManager* services) : CBUTaskBase(services, bvid_armsWindmill)
  {
    initialiseCustomVariables();
  }

  void NmRsCBUArmsWindmill::initialiseCustomVariables()
  {
    m_mask = bvmask_ArmLeft | bvmask_ArmRight;
    m_armDamping = 1.0f;
    m_doLeftArm = true;
    m_doRightArm = true;
    m_oK2Deactivate = true;
  }

  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::onActivate()
  {
    
    Assert(m_character);

    m_leftCircle.angle = 0;
    m_leftCircle.TM.Identity();
    m_leftCircle.target.Zero();
    m_leftCircle.desc = &m_parameters.m_leftCircleDesc;
    m_leftCircle.radius1Delta = 0.f;
    m_leftCircle.radius2Delta = 0.f;

    m_rightCircle.angle = 0;
    m_rightCircle.TM.Identity();
    m_rightCircle.target.Zero();
    m_rightCircle.desc = &m_parameters.m_rightCircleDesc;
    m_rightCircle.radius1Delta = 0.f;
    m_rightCircle.radius2Delta = 0.f;

    m_isForcingSync = false;
    m_armDamping = 1.0f;
    m_armStiffness = 12.0f;
    m_elbowStiffness = 12.0f;
    m_leftSpeed = 0.0f;
    m_rightSpeed = 0.0f;
    m_leftTimeStep = 0.0f;
    m_rightTimeStep = 0.0f;
    m_doLeftArm = true;
    m_doRightArm = true;

    m_leftSpeedDelta = 0.f;
    m_rightSpeedDelta = 0.f;
    m_shoulderStiffnessDelta = 0.f;
    m_elbowStiffnessDelta = 0.f;
    m_phaseOffsetDelta = 0.f;


    updateCirclePose(HAND_LEFT);
    snapToClosestPoint(HAND_LEFT);

    updateCirclePose(HAND_RIGHT);
    snapToClosestPoint(HAND_RIGHT);
  }

  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::onDeactivate()
  {
    Assert(m_character);
    initialiseCustomVariables();
  }

  // setup arm stiffness and damping
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::configureArmStiffnesses()
  {
    if (m_parameters.m_useLeft)
    {
      getLeftArmInputData()->getClavicle()->setStiffness(m_armStiffness, m_armDamping);
      getLeftArmInputData()->getShoulder()->setStiffness(m_armStiffness, m_armDamping);
      getLeftArmInputData()->getElbow()->setStiffness(m_elbowStiffness, m_parameters.m_elbowDamping);
    }
    if (m_parameters.m_useRight)
    {
      getRightArmInputData()->getClavicle()->setStiffness(m_armStiffness, m_armDamping);
      getRightArmInputData()->getShoulder()->setStiffness(m_armStiffness, m_armDamping);
      getRightArmInputData()->getElbow()->setStiffness(m_elbowStiffness, m_parameters.m_elbowDamping);
    }
  }

  // Both updateCirclePose functions are fairly slow, for what they do.
  void NmRsCBUArmsWindmill::updateCirclePose(Hand hand)
  {
    // update left: always user specified
    if(hand == HAND_LEFT)
    {
      updateCirclePose(m_leftCircle, m_leftCircle.desc->normal, m_leftCircle.desc->centre);
    }
    // update right: depends on whether and which mode of mirroring we use
    else
    {
      if (m_parameters.m_mirrorMode == MIRROR_NONE)
        updateCirclePose(m_rightCircle, m_rightCircle.desc->normal, m_rightCircle.desc->centre);
      else
      {
        rage::Vector3 centre = m_leftCircle.desc->centre;
        int reflection;
        if (m_leftCircle.desc->partID == getSpine()->getPelvisPart()->getPartIndex() ||
            m_leftCircle.desc->partID == getSpine()->getSpine3Part()->getPartIndex() ||
            m_leftCircle.desc->partID == getSpine()->getSpine2Part()->getPartIndex() ||
            m_leftCircle.desc->partID == getSpine()->getSpine1Part()->getPartIndex() ||
            m_leftCircle.desc->partID == getSpine()->getSpine0Part()->getPartIndex())
        {
          reflection = 1;//reflect in y
          centre[1] *= -1;
        }
        else if (m_leftCircle.desc->partID == getLeftArm()->getClaviclePart()->getPartIndex() ||
                 m_leftCircle.desc->partID == getRightArm()->getClaviclePart()->getPartIndex())
        {
          reflection = 2;//reflect in z
          centre[2] *= -1;
        }
        else if (m_leftCircle.desc->partID == getSpine()->getHeadPart()->getPartIndex() ||
          m_leftCircle.desc->partID == getSpine()->getNeckPart()->getPartIndex())
        {
          reflection = 0;//reflect in x
          centre[0] *= -1;
        }
        else //arms or legs
        {
          reflection = 0;//reflect in x
          centre[0] *= -1;
        }
        if (m_parameters.m_mirrorMode == MIRROR_SYMMETRIC)
          updateCirclePose(m_rightCircle, m_leftCircle.desc->normal, centre, &reflection);
        else //m_parameters.m_mirrorMode == MIRROR_PARALLEL
          updateCirclePose(m_rightCircle, m_leftCircle.desc->normal, centre);
      }
    }
  }

  // performs the local to world space transformation of the circle, takes local TM from partID
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::updateCirclePose(Circle& circle, const rage::Vector3& normal, const rage::Vector3& centre, int* reflection)
  {
    rage::Matrix34 localTM; 
    m_character->getGenericPartByIndex(circle.desc->partID)->getMatrix(localTM);

    circle.TM = localTM;
    // first rotate to point outwards
    circle.TM.RotateLocalZ(PI/2.0f);
    // then apply user rotations
    circle.TM.RotateLocalY(normal[1]);
    circle.TM.RotateLocalZ(normal[2]);
    circle.TM.RotateLocalX(normal[0]);
    //apply reflection
    if (reflection)
    {
      rage::Matrix34 TM;
      rage::Vector4 planeN;
      switch (*reflection)
      {
        case 0:
          planeN = localTM.a; break;
        case 1:
          planeN = localTM.b; break;
        case 2:
          planeN = localTM.c; break;
      }
      TM.MirrorOnPlane(planeN);
      circle.TM.Dot3x3(TM);
    }

    // update position relative to specified part
    localTM.Transform(centre, circle.TM.d);
  }


  // solve circle equation: p(t) = C + (r*cos(t))*U + (r*sin(t))*V | t < 2*PI
  // where centre C is stored in TM.d and U,V in TM.b and TM.c ... (TM.a is the circle normal)
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::pointOnCircle(const Circle& c, rage::Vector3& p)
  {
    pointOnCircle(c, c.angle, p);
  }

  void NmRsCBUArmsWindmill::pointOnCircle(const Circle& c, float angle, rage::Vector3& p)
  {
    Assert((angle >=0) && (angle <= PI2));
    p = c.TM.d + (c.desc->radius1+c.radius1Delta)*cos(angle)*c.TM.b + (c.desc->radius2+c.radius2Delta)*sin(angle)*c.TM.c;
  }

  // calculate the 3d position on the circle that is closest to the specified point
  // returns the point in reference pClosest and returns its angle
  // ---------------------------------------------------------------------------------------------------------
  float NmRsCBUArmsWindmill::angleOfClosestPointOnCircle(const Circle&circle, const rage::Vector3& p)
  {
    // move the world space point into local space of the circle
    rage::Vector3 pLocal;
    circle.TM.UnTransform(p, pLocal);
    // project the point onto the circle plane (normal is x-Axis)
    pLocal[0] = 0.0;
    // get angle on circle/ellipse, i.e. angle with first axis of ellipse (y-Axis in this case)
    float angle = rage::Atan2f(pLocal[2], pLocal[1]);
    // wrap negative angle into range [0, 2PI]
    return rage::Wrap(angle, 0.0f, PI2);
  }

#if ART_ENABLE_BSPY & ArmsWindmillBSpyDraw
  // get point on circle closest to a given point in space
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::closestPointOnCircle(const Circle&circle, const rage::Vector3& p, rage::Vector3& pClosest)
  {
    float angle = angleOfClosestPointOnCircle(circle, p);
    pointOnCircle(circle, angle, pClosest);
  }
#endif

  // set current angle of circle to an angle that is the closest to a give position
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::snapToClosestPoint(Hand hand)
  {
    rage::Vector3 handPos; 
    if(hand == HAND_LEFT)
    {
      rage::Vector3 handPos = getLeftArm()->getHand()->getPosition();
      m_leftCircle.angle = angleOfClosestPointOnCircle(m_leftCircle, handPos);
    }
    else if(hand == HAND_RIGHT)
    {
      rage::Vector3 handPos = getRightArm()->getHand()->getPosition();
      m_rightCircle.angle = angleOfClosestPointOnCircle(m_rightCircle, handPos);
    }
  }


  // used for advancing the current position on the circle
  // wraps around for timer in [0, 2*PI], to avoid these going towards infinity
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::updateAngleOnCircle(Circle& c, float speed, float timeStep)
  {
    // if speed==1 then you'd get 1 revolution per second on a circle of radius 1
    // get current "radius" to compensate speed for eccentricity of ellipse
    /*rage::Vector3 p;
    pointOnCircle(c, p);
    float currentRadius = (p-c.TM.d).Mag();   
    // circumference to adjust speed for different sizes of ellipse
    float a = c.desc->radius1;
    float b = c.desc->radius2;
    float cf = PI*(3*(a+b) - rage::Sqrtf((3*a+b)*(a+3*b)) );
    NM_RS_DBG_LOGF(L"currentRadius: %2.2f | circf: %2.2f", currentRadius, cf);
    */
    //float unitSpeedInc = PI2*timeStep/c.desc->radius1; // TODO: CACHE THIS
    //float unitSpeedInc = PI2*timeStep*currentRadius*(cf/PI2); // TODO: CACHE THIS
    float unitSpeedInc = PI2*timeStep;// /(cf/PI2); // TODO: CACHE THIS
    c.angle += speed*unitSpeedInc;
    c.angle = rage::Wrap(c.angle, 0.0f, PI2);
#if ART_ENABLE_BSPY & 0
    bspyLogf(info, L"updateAngleOnCircle speed = %f", speed);
    bspyLogf(info, L"updateAngleOnCircle timeStep = %f", timeStep);
    bspyLogf(info, L"updateAngleOnCircle angle = %f", c.angle);
#endif
  }


  // used for phase synchronization
  // calculates the angular distance between left and right target, 
  // taking into account the singularity at the transition between 2PI and 0
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::updateRelativeTargetDistances()
  {
    float l = m_leftCircle.angle;
    // adds the phase offset here
    float r = rage::Wrap((m_rightCircle.angle - (m_parameters.m_phaseOffset+m_phaseOffsetDelta)*DtoR), 0.0f, PI2);
    m_relDistLRcw = r > l ? r - l : r + (PI2-l);
    m_relDistLRccw = r > l ? l + (PI2-r) : l - r;

    // exactly the other way round if direction on circle is negative
    if(m_leftCircle.desc->speed < 0 && m_rightCircle.desc->speed < 0)
    {
      rage::SwapEm(m_relDistLRcw, m_relDistLRccw);
    }
  }


  // lets the angle of circles advance by different amounts,
  // such as to synchronize their phase with a given offset
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::updateTimeStepForSync()
  {
    // determine distance between the two targets in cw and ccw direction and decide who's leading
    updateRelativeTargetDistances();
    bool leftIsLeading = m_relDistLRcw > m_relDistLRccw;
    NM_RS_DBG_LOGF(L"sync LRcw: %2.2f | LRccw: %2.2f | LL ? %d", m_relDistLRcw, m_relDistLRccw, leftIsLeading);     

    float syncDist = 0;
    if(leftIsLeading)
      syncDist = m_relDistLRccw;
    else
      syncDist = m_relDistLRcw;
    NM_RS_DBG_LOGF(L"sync dist: %2.2f, %2.2f", syncDist, syncDist*180.0f/PI);     

    // normalize distance to make it a gain in [0,0.5], i.e. up to 50% slow down or speed up
    syncDist *= 0.5f;
    syncDist = rage::Clamp(syncDist, 0.0f, 0.5f);

    // modify timeStep of both circles
    if(leftIsLeading)
    {
      m_leftTimeStep *= 1-syncDist; // left has to slow down: the larger the distance, the less time it gets to move
      m_rightTimeStep *= 1+syncDist; // right has to speed up: the larger the distance, the more time it gets to move
      NM_RS_DBG_LOGF(L"sync RIGHT SPEEDING BY %2.2f", 1+syncDist);         
      NM_RS_DBG_LOGF(L"sync LEFT SLOWING BY %2.2f", 1-syncDist);     
    }
    else
    {
      m_leftTimeStep *= 1+syncDist; // left has to speed up: the larger the distance, the more time it gets to move
      m_rightTimeStep *= 1-syncDist; // right has to slow down: the larger the distance, the less time it gets to move
      NM_RS_DBG_LOGF(L"sync RIGHT SLOWING BY %2.2f", 1-syncDist);     
      NM_RS_DBG_LOGF(L"sync LEFT SPEEDING BY %2.2f", 1+syncDist);     
    }
  }

  // in adaptive mode, determine speed from angular velocity of character
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::updateAdaptiveSpeeds()
  {
    m_armStiffness = m_parameters.m_shoulderStiffness + m_shoulderStiffnessDelta;
    m_armDamping = m_parameters.m_shoulderDamping;
    m_elbowStiffness = m_parameters.m_elbowStiffness + m_elbowStiffnessDelta;

    // by default, when not in adaptive mode, use user specified values ... //////////////////////////////////////
    if(m_parameters.m_adaptiveMode == ADAPT_NONE)
    {
      m_leftSpeed  = m_leftCircle.desc->speed + m_leftSpeedDelta;
      m_rightSpeed = m_rightCircle.desc->speed + m_rightSpeedDelta;
    }      
    //...otherwise work out direction of arm circling
    else
    {
      rage::Vector3 m_comAngVel = m_character->m_COMrotvel;
      rage::Vector3 m_rotationAxis = m_character->m_COMTM.a;//bodyRight
      float cavD = -m_comAngVel.Dot(m_rotationAxis);

      // only change direction of circling
      if(m_parameters.m_adaptiveMode == ADAPT_DIR)
      {
        float sign = cavD < 0.f ? 1.f : -1.f; 
        m_leftSpeed = sign*fabs(m_leftCircle.desc->speed);
        m_rightSpeed = sign*fabs(m_rightCircle.desc->speed);
      }
      // change direction and speed of circling
      else if(m_parameters.m_adaptiveMode >= ADAPT_DIR_SPEED)
      {
        if(fabs(cavD) > m_parameters.m_angVelThreshold)
        {
          float speed = cavD * m_parameters.m_angVelGain;
          speed = rage::Clamp(speed, -2.0f, 2.0f);
          m_leftSpeed = m_rightSpeed = cavD * m_parameters.m_angVelGain;
        }
        // if angular speed smaller than threshold, don't move targets
        else
        {
          m_leftSpeed = m_rightSpeed = 0;          
        }
      }
       
      // also modify strength if necessary
      if(m_parameters.m_adaptiveMode >= ADAPT_DIR_SPEED_STRENGTH)
      {
        m_armStiffness = rage::Clamp((10.0f*fabs(cavD)),0.001f, 15.0f);
        m_armDamping = rage::Clamp((1.0f*fabs(cavD)),0.1f, 1.0f);
      }

      NM_RS_DBG_LOGF(L"cavD: %2.2f | speeds: %2.2f %2.2f | stiff %2.2f", cavD, m_leftSpeed, m_rightSpeed, m_armStiffness);    
    } // if adapt
    
  }

  // do stuff
  // ---------------------------------------------------------------------------------------------------------
    CBUTaskReturn NmRsCBUArmsWindmill::onTick(float timeStep)
    {
      // calculate speed of circling: user specified or calculated when in adaptive mode
      updateAdaptiveSpeeds();

      configureArmStiffnesses();

      // if phase synchronization is enabled adjust the two circle's time step accordingly ////////////
      // speeds up or slows down targets if necessary by adjusting integration step size
      m_leftTimeStep = m_rightTimeStep = timeStep;
      if ( m_parameters.m_forceSync 
           && (m_parameters.m_useRight && m_parameters.m_useLeft)
           && (rage::Sign(m_leftCircle.desc->speed) == rage::Sign(m_leftCircle.desc->speed))              
         )
      {        
        updateTimeStepForSync();
      }

      // Disable arm permanently if an impact occurs.
      if (m_doLeftArm)
        m_doLeftArm = !(m_parameters.m_disableOnImpact && getLeftArm()->getHand()->collidedWithNotOwnCharacter());
      if (m_doRightArm)
        m_doRightArm = !(m_parameters.m_disableOnImpact && getRightArm()->getHand()->collidedWithNotOwnCharacter());
        

      // update left circle and IK target //////////////////////////////////////////////////////////
      if(m_parameters.m_useLeft && !(!m_doLeftArm && m_parameters.m_disableOnImpact))     
      {
        // position the circle in space relative to specified character part
        updateCirclePose(HAND_LEFT);
        // advance the current angle on the circle by timeStep and speed
        updateAngleOnCircle(m_leftCircle, m_leftSpeed, m_leftTimeStep);  
        // get 3d point from current angle on circle and use as target for IK
        pointOnCircle(m_leftCircle, m_leftCircle.target);      
        // do IK stuff
        rage::Vector3 vel = getLeftArm()->getClaviclePart()->getLinearVelocity();

        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();

        ikInputData->setTarget(m_leftCircle.target);
        ikInputData->setTwist(m_parameters.m_IKtwist);
        ikInputData->setDragReduction(m_parameters.m_dragReduction);
        ikInputData->setVelocity(vel);
        ikInputData->setTwistIsFixed(true);
        ikInputData->setMatchClavicle(kMatchClavicleBetter);
        float elbowAng = getLeftArm()->getElbow()->getActualAngle();
        if (elbowAng < m_parameters.m_leftElbowMin)
        {
          ikInputData->setMinimumElbowAngle(elbowAng + 2.0f*timeStep * (m_parameters.m_leftElbowMin - elbowAng));
#if ART_ENABLE_BSPY 
          float angleAdd = 2.0f*timeStep * (m_parameters.m_leftElbowMin - elbowAng);
          bspyScratchpad(m_character->getID(), "ArmsWindmillLeft", elbowAng);
          bspyScratchpad(m_character->getID(), "ArmsWindmillLeft", m_parameters.m_leftElbowMin);
          bspyScratchpad(m_character->getID(), "ArmsWindmillLeft", angleAdd);
#endif
        }
        getLeftArm()->postInput(ikInput);

        // wrist
        getLeftArmInputData()->getWrist()->setDesiredAngles(0.f,0.f,0.f);
      }

      // update right circle and IK target //////////////////////////////////////////////////////////
      if(m_parameters.m_useRight && !(!m_doRightArm && m_parameters.m_disableOnImpact))   
      {
        // position the circle in space relative to specified character part
        updateCirclePose(HAND_RIGHT);
        // advance the current angle on the circle by timeStep and speed
        updateAngleOnCircle(m_rightCircle, m_rightSpeed, m_rightTimeStep);
        // get 3d point from current angle on circle and use as target for IK
        pointOnCircle(m_rightCircle, m_rightCircle.target);

#if ART_ENABLE_BSPY && ArmsWindmillBSpyDraw
        bspyLogf(info, L"TM.a   = [%f, %f, %f]", m_rightCircle.TM.a.x, m_rightCircle.TM.a.y, m_rightCircle.TM.a.z);
        bspyLogf(info, L"TM.b   = [%f, %f, %f]", m_rightCircle.TM.b.x, m_rightCircle.TM.b.y, m_rightCircle.TM.b.z);
        bspyLogf(info, L"TM.c   = [%f, %f, %f]", m_rightCircle.TM.c.x, m_rightCircle.TM.c.y, m_rightCircle.TM.c.z);
        bspyLogf(info, L"TM.d   = [%f, %f, %f]", m_rightCircle.TM.d.x, m_rightCircle.TM.d.y, m_rightCircle.TM.d.z);
        bspyLogf(info, L"target = [%f, %f, %f]", m_rightCircle.target.x, m_rightCircle.target.y, m_rightCircle.target.z);
        bspyLogf(info, L"angle = %f", m_rightCircle.angle);
        bspyLogf(info, L"radius1Delta = %f", m_rightCircle.radius1Delta);
        bspyLogf(info, L"radius2Delta = %f", m_rightCircle.radius2Delta);
        bspyLogf(info, L"normal = [%f, %f, %f]", m_rightCircle.desc->normal.x, m_rightCircle.desc->normal.y, m_rightCircle.desc->normal.z);
        bspyLogf(info, L"centre = [%f, %f, %f]", m_rightCircle.desc->centre.x, m_rightCircle.desc->centre.y, m_rightCircle.desc->centre.z);
        bspyLogf(info, L"partID = %d", m_rightCircle.desc->partID);
        bspyLogf(info, L"radius1 = %f", m_rightCircle.desc->radius1);
        bspyLogf(info, L"radius2 = %f", m_rightCircle.desc->radius2);
        bspyLogf(info, L"speed = %f", m_rightCircle.desc->speed);
        bspyLogf(info, L"radius1Delta = %f", m_rightCircle.desc->radius1Delta);
        bspyLogf(info, L"radius2Delta = %f", m_rightCircle.desc->radius2Delta);
        bspyLogf(info, L"speedDelta = %f", m_rightCircle.desc->speedDelta);
#endif

        // do IK stuff
        rage::Vector3 vel = getRightArm()->getClaviclePart()->getLinearVelocity();
        NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>(1);
        NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
        ikInputData->setTarget(m_rightCircle.target);
        ikInputData->setTwist(m_parameters.m_IKtwist);
        ikInputData->setDragReduction(m_parameters.m_dragReduction);
        ikInputData->setVelocity(vel);
        ikInputData->setTwistIsFixed(true);
        ikInputData->setMatchClavicle(kMatchClavicleBetter);
        float elbowAng = getRightArm()->getElbow()->getActualAngle();
        if(elbowAng < m_parameters.m_rightElbowMin)
          ikInputData->setMinimumElbowAngle(elbowAng + 2.0f*timeStep * (m_parameters.m_rightElbowMin - elbowAng));
        getRightArm()->postInput(ikInput);

        // wrist
        getRightArmInputData()->getWrist()->setDesiredAngles(0.f,0.f,0.f);
      }
// draw circles
#if ART_ENABLE_BSPY && ArmsWindmillBSpyDraw      
      // draw circle's local space TM
      rage::Matrix34 localTM; 
      NM_RS_DBG_LOGF(L"angles: %2.2f %2.2f", m_leftCircle.angle, m_rightCircle.angle);      
      if(m_parameters.m_useLeft && !(!m_doLeftArm && m_parameters.m_disableOnImpact))     
      {
        m_character->getGenericPartByIndex(m_leftCircle.desc->partID)->getMatrix(localTM);
        m_character->bspyDrawCoordinateFrame(0.3f, localTM);

        // draw circles
        drawCircle(m_leftCircle, rage::Vector3(1.0f, 1.0f, 0.0f));//yellow

        // draw point on circle closest to hand
        rage::Vector3 closestPointLeft;
        rage::Vector3 leftHandPos = getLeftArm()->getHand()->getPosition();
        closestPointOnCircle(m_leftCircle, leftHandPos, closestPointLeft);
        m_character->bspyDrawPoint(closestPointLeft, 0.1f, rage::Vector3(1.0f, 0.5f, 0.1f));
      }

      if(m_parameters.m_useRight && !(!m_doRightArm && m_parameters.m_disableOnImpact))   
      {
        m_character->getGenericPartByIndex(m_rightCircle.desc->partID)->getMatrix(localTM);
        m_character->bspyDrawCoordinateFrame(0.3f, localTM);
        
        drawCircle(m_rightCircle, rage::Vector3(1.0f, 1.0f, 1.0f));//white

        // draw point on circle closest to hand
        rage::Vector3 closestPointLeft, closestPointRight;
        rage::Vector3 rightHandPos = getRightArm()->getHand()->getPosition();
        closestPointOnCircle(m_rightCircle, rightHandPos, closestPointRight);
        m_character->bspyDrawPoint(closestPointRight, 0.1f, rage::Vector3(1.0f,0.5f, 0.1f));
      }
#endif

    return eCBUTaskComplete;

  } // tick

#if ART_ENABLE_BSPY && ArmsWindmillBSpyDraw       
  // used solely for debug purposes
  // ---------------------------------------------------------------------------------------------------------
  void NmRsCBUArmsWindmill::drawCircle(Circle& c, const rage::Vector3& color)
  {
    int segN = 16;
    rage::Vector3 X0, X1;
    for (int i = 0; i < segN-1; i++)
    {
      // start and end in parameter space
      float t0 = i*PI2/segN;
      float t1 = t0 + PI2/segN;
      pointOnCircle(c, t0, X0);
      pointOnCircle(c, t1, X1);
      // draw segment
      m_character->bspyDrawLine(X0,X1,color);
    }
    // draw normals
    m_character->bspyDrawCoordinateFrame(1.0, c.TM);

    // draw current angle
    pointOnCircle(c, c.angle, X0);
    m_character->bspyDrawPoint(X0, 0.1f, color);
  }
#endif


#if ART_ENABLE_BSPY
  void NmRsCBUArmsWindmill::sendParameters(NmRsSpy& spy)
  {      
    CBUTaskBase::sendParameters(spy);

    // params
    bspyTaskVar(m_parameters.m_leftCircleDesc.partID, true); 
    bspyTaskVar(m_parameters.m_leftCircleDesc.radius1, true);
    bspyTaskVar(m_parameters.m_leftCircleDesc.radius2, true);
    bspyTaskVar(m_parameters.m_leftCircleDesc.speed, true);
    bspyTaskVar(m_parameters.m_leftCircleDesc.normal, true);
    bspyTaskVar(m_parameters.m_leftCircleDesc.centre, true);

    bspyTaskVar(m_parameters.m_rightCircleDesc.partID, true); 
    bspyTaskVar(m_parameters.m_rightCircleDesc.radius1, true);
    bspyTaskVar(m_parameters.m_rightCircleDesc.radius2, true);
    bspyTaskVar(m_parameters.m_rightCircleDesc.speed, true);
    bspyTaskVar(m_parameters.m_rightCircleDesc.normal, true);
    bspyTaskVar(m_parameters.m_rightCircleDesc.centre, true);

    bspyTaskVar(m_parameters.m_shoulderStiffness, true);
    bspyTaskVar(m_parameters.m_shoulderDamping, true);
    bspyTaskVar(m_parameters.m_elbowStiffness, true);
    bspyTaskVar(m_parameters.m_elbowDamping, true);
    bspyTaskVar(m_parameters.m_leftElbowMin, true);
    bspyTaskVar(m_parameters.m_rightElbowMin, true);
    bspyTaskVar(m_parameters.m_phaseOffset, true);
    bspyTaskVar(m_parameters.m_dragReduction, true);
    bspyTaskVar(m_parameters.m_IKtwist, true);
    bspyTaskVar(m_parameters.m_angVelThreshold, true);
    bspyTaskVar(m_parameters.m_angVelGain, true);

    bspyTaskVar(m_parameters.m_mirrorMode, true);
    bspyTaskVar(m_parameters.m_adaptiveMode, true);

    bspyTaskVar(m_parameters.m_forceSync, true);
    bspyTaskVar(m_parameters.m_useLeft, true);
    bspyTaskVar(m_parameters.m_useRight, true);
    bspyTaskVar(m_parameters.m_disableOnImpact, true);	

    //vars
    bspyTaskVar(m_leftSpeed, false);
    bspyTaskVar(m_rightSpeed, false);
    bspyTaskVar(m_armStiffness, false);
    bspyTaskVar(m_armDamping, false);
    
    bspyTaskVar(m_leftSpeedDelta, false);
    bspyTaskVar(m_rightSpeedDelta, false);
    bspyTaskVar(m_shoulderStiffnessDelta, false);
    bspyTaskVar(m_elbowStiffnessDelta, false);
    bspyTaskVar(m_phaseOffsetDelta, false);

    bspyTaskVar(m_rightCircle.angle, false);
    bspyTaskVar(m_leftCircle.target, false);
    bspyTaskVar(m_rightCircle.target, false);
    bspyTaskVar(m_doLeftArm, false);
    bspyTaskVar(m_doRightArm, false);
    bspyTaskVar(m_oK2Deactivate, false);

  }
#endif // ART_ENABLE_BSPY
}
