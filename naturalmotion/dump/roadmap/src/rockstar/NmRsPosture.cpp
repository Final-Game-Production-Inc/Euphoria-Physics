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
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsUtils.h"
#include "NmRsGenericPart.h"
#include "NmRsEffectors.h"

#define NM_RS_VERBOSE_POSTURE_DEBUG 0
#define NM_USE_FLOOR_VELOCITY 1

namespace ART
{
  // this works out the effector torque needed to oppose gravity.. basically (subtreeCOM-effectorPos) X (gravity*subtreeMass)
  rage::Vector3 NmRsCharacter::getOpposeTorque(int i, const rage::Vector3 &gravity, rage::Vector3 &toCOM, rage::Vector3 &weight)
  {
    rage::Vector3 pos;
    rage::Vector3 torque;
    bool doubleSupport = (m_posture.leftFootConnected && m_posture.rightFootConnected) && i!=m_posture.alternateRoot;
    doubleSupport = doubleSupport && (i >= getLeftLegSetup()->getHip()->getJointIndex() && i <= getRightLegSetup()->getAnkle()->getJointIndex());

    NmRsEffectorBase *effector = getEffector(i);
    bool exclude = effector->isPartOfGroundedChain() || i==m_posture.alternateRoot;
    // specialist code for double support effectors (ie any effectors in the legs)
    //Assumes that left legs have lower joint indexes than the right and that 
    if (exclude && doubleSupport)
    {
      NmRsEffectorBase *effector2 = getEffector(i > getLeftLegSetup()->getAnkle()->getJointIndex() ? i-3 : i+3);
      NmRsGenericPart *parent1 = getGenericPartByIndex(effector->getParentIndex());
      NmRsGenericPart *parent2 = getGenericPartByIndex(effector2->getParentIndex());
      float balance = 0.5f; 
      float subtreeMass = balance*parent1->m_subtreeMass + (1.f-balance)*parent2->m_subtreeMass; // average the subtree mass over the equivalent effectors on each side

      rage::Vector3 pos1(effector->getJoint()->GetJointPosition(getArticulatedBody()));
      rage::Vector3 pos2(effector2->getJoint()->GetJointPosition(getArticulatedBody()));

      rage::Vector3 subtreeCOM;
      // average the predicted subtree COM over the equivalent effectors on each side
      subtreeCOM.Lerp(balance, parent2->m_subtreeCOM+parent2->m_subtreeCOMvel*m_posture.damping, parent1->m_subtreeCOM+parent1->m_subtreeCOMvel*m_posture.damping);
      subtreeCOM /= subtreeMass;
      // predict effector position on each side (eg left hip and right hip).. this gives us our posture damping
#if NM_USE_FLOOR_VELOCITY
        pos1 += (parent1->getVelocityAtLocalPoint(pos1)-m_floorVelocity)*m_posture.damping;
        pos2 += (parent2->getVelocityAtLocalPoint(pos2)-m_floorVelocity)*m_posture.damping;
#else
      pos1 += parent1->getVelocityAtLocalPoint(pos1)*m_posture.damping;
      pos2 += parent2->getVelocityAtLocalPoint(pos2)*m_posture.damping;
#endif
      // do some magic to work out the effective toCOM and gravity vectors
      rage::Vector3 betweenDir = pos2-pos1;
      betweenDir.Normalize();
      Assert(rage::Abs((pos2-pos1).Dot(betweenDir)) > 1e-10f);
      float t = (subtreeCOM - pos1).Dot(betweenDir) / (pos2-pos1).Dot(betweenDir);
      pos.Lerp(t, pos1, pos2);        
      toCOM = (pos - subtreeCOM); 
      weight = gravity * subtreeMass;
      weight -= betweenDir * weight.Dot(betweenDir);
      weight *= 2.f;//Will be removed later if 2 limbs connected
    }
    else // usual path... work out the toCOM vector (subtreeCOM - effectorPos) and weight vector as gravity*mass
    {
      //NmRsEffectorBase *effector = getEffector(i);
      int index = exclude ? effector->getParentIndex() : effector->getChildIndex(); // parent if we're a support leg (reversing through the hierarchy)
      NmRsGenericPart *child = getGenericPartByIndex(index);
      pos.Set(effector->getJoint()->GetJointPosition(getArticulatedBody()));
#if NM_USE_FLOOR_VELOCITY
      pos += (child->getVelocityAtLocalPoint(pos)-m_floorVelocity)*m_posture.damping; // predicted effector pos (for damping)
#else
      pos += child->getVelocityAtLocalPoint(pos)*m_posture.damping; // predicted effector pos (for damping)
#endif
      toCOM = pos - (child->m_subtreeCOM+child->m_subtreeCOMvel*m_posture.damping)/child->m_subtreeMass;
      weight = gravity * child->m_subtreeMass;
    }
    torque.Cross(toCOM, weight); // the oppose torque is now simply toCOM X weight

    int limbsConnected = 0;
    if (effector->isPartOfGroundedChain())
    {
      if (m_posture.leftHandConnected)
      {
        limbsConnected++;
      }
      if (m_posture.rightHandConnected)
      {
        limbsConnected++;
        //TEST CODE FOR POINT AND LINE CONTRAINTS
        //if (i == getRightArmSetup()->getWrist()->getJointIndex() ||
        //  i == getRightArmSetup()->getElbow()->getJointIndex() ||
        //  i == getRightArmSetup()->getShoulder()->getJointIndex() ||
        //  i == getRightArmSetup()->getClavicle()->getJointIndex() ||
        //  i == getSpineSetup()->getSpine3()->getJointIndex() ||
        //  i == getSpineSetup()->getSpine2()->getJointIndex() ||
        //  i == getSpineSetup()->getSpine1()->getJointIndex() ||
        //  i == getSpineSetup()->getSpine0()->getJointIndex())
        //{
        //  //mmmmtodo
        //  //Reverse torque around the line between the jointPosition and a point constraint
        //  static int der1 = 0;
        //  if (der1 == 1)//For point constraints - hardcoded as hand centre
        //  {
        //    rage::Vector3 pos2Constraint = getRightArmSetup()->getHand()->getPosition() - pos;
        //    float len = torque.Dot(pos2Constraint);
        //    pos2Constraint = pos2Constraint*len*2.f;
        //    torque -= pos2Constraint;
        //  }
        //  if (der1 == 2)//For line constraints - hardcoded as thumb2finger
        //  {
        //    rage::Matrix34 handTm;
        //    getRightArmSetup()->getHand()->getMatrix(handTm);
        //    rage::Vector3 lineConstraint = handTm.c;
        //    float len = torque.Dot(lineConstraint);
        //    lineConstraint = lineConstraint*len*2.f;
        //    torque -= lineConstraint;
        //  }

        //}
      }
      if (m_posture.leftFootConnected)
        limbsConnected++;
      if (m_posture.rightFootConnected)
        limbsConnected++;
      if (limbsConnected != 0)
        torque /= (float)limbsConnected;  
    }

    return exclude ? -torque : torque; // -ve if we're going the other way through the tree
  }

  // expanded version of ApplyPostureControl which auto calculates down vector and foot support type
  // based on the COM and the foot support polygon
  void NmRsCharacter::applyZMPPostureControl(float footWidth, float footLength)
  {
    // TDL all this code below looks complex.. 
    // it automatically decides the ZMP based on which feet are on the ground
    // then also decides which legs should be supporting in the gravity compensation based on this
    // however the m_posture.leftFoot/m_posture.rightFoot parameters allows you to override the foot values
    // -2 = not overriding (ie automatic), set in balancer notStepping
    // -1 = foot is off the ground, set in balancer stepping
    // 0-21 = part index of this foot, set in balancer stance foot 
    //leftFoot = -1 if airborne or stepfoot
    //leftFoot = footPartIndex if in contact or stancefoot
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "GravComp", footWidth);
    bspyScratchpad(getBSpyID(), "GravComp", footLength);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.damping);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.clampLimit);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.alternateRoot);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.useZMP);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.leftHandConnected);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.rightHandConnected);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.leftFootConnected);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.rightFootConnected);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.leftArmAutoSet);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.rightArmAutoSet);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.leftLegAutoSet);
    bspyScratchpad(getBSpyID(), "GravComp", m_posture.rightLegAutoSet);
#endif

#if ART_ENABLE_BSPY
      bspyProfileStart("postureControl")
#endif

    int leftFoot = -1;
    int rightFoot = -1;
    if (m_posture.leftLegAutoSet)
    {
      if (getLeftLegSetup()->getFoot()->collided()) 
        setLeftFootConnected(true);
      else
        setLeftFootConnected(false);
    }
    if (m_posture.leftFootConnected)
      leftFoot = getLeftLegSetup()->getFoot()->getPartIndex();

    if (m_posture.rightLegAutoSet)
    {
      if (getRightLegSetup()->getFoot()->collided()) 
        setRightFootConnected(true);
      else
        setRightFootConnected(false);
    }
    if (m_posture.rightFootConnected)
      rightFoot = getRightLegSetup()->getFoot()->getPartIndex();

    if (m_posture.leftArmAutoSet)
      setLeftHandConnected(getLeftArmSetup()->getHand()->collided()); 

    if (m_posture.rightArmAutoSet)
      setRightHandConnected(getRightArmSetup()->getHand()->collided());

    bool spineConnected = m_posture.leftHandConnected || m_posture.rightHandConnected;

    // todo in phase2 for this to continue to work, we need to ensure
    // part map is correct.
    getEffector(gtaJtSpine_0)->setIsPartOfGroundedChain(spineConnected);
    getEffector(gtaJtSpine_1)->setIsPartOfGroundedChain(spineConnected);
    getEffector(gtaJtSpine_2)->setIsPartOfGroundedChain(spineConnected);
    getEffector(gtaJtSpine_3)->setIsPartOfGroundedChain(spineConnected);

    //This sets whether foot/hand is connected based on contact 
    //overridden by behaviours that set e.g. leftHandConnected
    //Autoset itself can be overidden by behaviours e.g. AnimPose
    m_posture.leftArmAutoSet = false;
    m_posture.rightArmAutoSet = false;
    m_posture.leftLegAutoSet = true;
    m_posture.rightLegAutoSet = true;

    rage::Vector3 ZMP;
    pointInsideFootSupport(m_COM, leftFoot, rightFoot, footWidth, footLength, &ZMP);

#if NM_TEST_NEW_INSIDESUPPORT //ZMP using support polygon
    rage::Vector3 ZMP2;
    rage::Matrix34 leftFoottm;
    rage::Matrix34 rightFoottm;
    if (leftFoot != -1)
      getLeftLegSetup()->getFoot()->getMatrix(leftFoottm);
    if (rightFoot != -1)
      getRightLegSetup()->getFoot()->getMatrix(rightFoottm);
    float insideResult = pointInsideSupportNew(
      m_COM, //tempTarget, 
      leftFoot!=-1 ? const_cast<const rage::Matrix34*>(&leftFoottm) : NULL,
      rightFoot!=-1 ? const_cast<const rage::Matrix34*>(&rightFoottm) : NULL,
      NULL, false,
      NULL, false, 
      footWidth, //mmmmtodo make this the actual size of the feet
      footLength, //mmmmtodo make this the actual size of the feet
      m_gUp, // ro.m_gUp, 
      &ZMP2);
    if (insideResult <= 0.f)
    {
      if (leftFoot != -1 && rightFoot != -1)//inside polygon - zmp height at centre of feet height
        ZMP2.z = (leftFoottm.d + rightFoottm.d).z *0.5f;
      else if (leftFoot != -1)
        ZMP2.z = leftFoottm.d.z;
      else  //(rightFoot != -1)
        ZMP2.z = rightFoottm.d.z;
    }
#if ART_ENABLE_BSPY
    bspyScratchpad(getBSpyID(), "ZMP", ZMP);
    bspyScratchpad(getBSpyID(), "ZMP", ZMP2);
#endif
#endif

    if (m_posture.useZMP)
      applyPostureControl(m_posture.clampLimit, &ZMP);
    else
      applyPostureControl(m_posture.clampLimit);

#if ART_ENABLE_BSPY
      bspyProfileEnd("postureControl")
#endif
  }

  // this is the basic routine for applying gravity opposition and posture control
  void NmRsCharacter::applyPostureControl(float clampScale, rage::Vector3 *ZMP)
  {
    rage::Vector3 gravity = -getUpVector() * rage::phSimulator::GetGravity().Mag() * getFirstInstance()->GetArchetype()->GetGravityFactor();
    if (ZMP) // The ZMP allows the gravity vector to be modified if the character is outside of its stable support
    {
#if ART_ENABLE_BSPY && NM_RS_VERBOSE_POSTURE_DEBUG
      bspyLogf(info, L"applyPostureControl using ZMP");
#endif
      rage::Vector3 toZMP = *ZMP - m_COM;
      toZMP.Normalize();
      gravity = toZMP * gravity.Dot(toZMP);
    }
#if ART_ENABLE_BSPY && NM_RS_VERBOSE_POSTURE_DEBUG
    bspyScratchpad(getBSpyID(), "applyPostureControl", gravity);
#endif
    // initialise all COM and COMvel values to the values at each part.. note that the COM and COMvel are multiplied by mass so we get the correct weighted sum in the end. They're finally divided by the total mass in the getOpposeTorque function
    for (int i = 0; i<getNumberOfParts(); i++)
    {
      NmRsGenericPart *part = getGenericPartByIndex(i);
      rage::phArticulatedBodyPart *bodyPart = (rage::phArticulatedBodyPart *)(part->getDataPtr());
      part->m_subtreeMass = getPartMass(*bodyPart);
      part->m_subtreeCOM = bodyPart->GetPosition() * part->m_subtreeMass;
#if NM_USE_FLOOR_VELOCITY
      part->m_subtreeCOMvel = (VEC3V_TO_VECTOR3(getArticulatedBody()->GetLinearVelocityNoProp(i))-m_floorVelocity) * part->m_subtreeMass; // NoProp
#else
      part->m_subtreeCOMvel = VEC3V_TO_VECTOR3(getArticulatedBody()->GetLinearVelocityNoProp(i)) * part->m_subtreeMass; // NoProp
#endif
    }
    if (m_attachedObject.partIndex >= 0)
    {
      rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetMatrix().GetCol3ConstRef());//Why is this different from below?
      NmRsGenericPart *part = getGenericPartByIndex(m_attachedObject.partIndex);
      part->m_subtreeMass += m_attachedObject.mass * m_attachedObject.massMultiplier;
      part->m_subtreeCOM += (m_attachedObject.worldCOMPos - offset) * m_attachedObject.mass * m_attachedObject.massMultiplier;
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_attachedObject.partIndex);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_attachedObject.mass);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_attachedObject.massMultiplier);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_attachedObject.worldCOMPos);
#endif
    }
    // add any attached weapon masses
    if (m_leftHandWeapon.partIndex >= 0)
    {
      rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetPosition());//Why is this different from above?
      NmRsGenericPart *part = getGenericPartByIndex(m_leftHandWeapon.partIndex);
      part->m_subtreeMass += m_leftHandWeapon.mass * m_leftHandWeapon.massMultiplier;
      part->m_subtreeCOM += (m_leftHandWeapon.worldCOMPos - offset) * m_leftHandWeapon.mass * m_leftHandWeapon.massMultiplier;
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_leftHandWeapon.partIndex);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_leftHandWeapon.mass);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_leftHandWeapon.massMultiplier);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_leftHandWeapon.worldCOMPos);
#endif
    }
    if (m_rightHandWeapon.partIndex >= 0)
    {
      rage::Vector3 offset = RCC_VECTOR3(getArticulatedWrapper()->getArticulatedCollider()->GetPosition());
      NmRsGenericPart *part = getGenericPartByIndex(m_rightHandWeapon.partIndex);
      part->m_subtreeMass += m_rightHandWeapon.mass * m_rightHandWeapon.massMultiplier;
      part->m_subtreeCOM += (m_rightHandWeapon.worldCOMPos - offset) * m_rightHandWeapon.mass * m_rightHandWeapon.massMultiplier;
#if ART_ENABLE_BSPY
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_rightHandWeapon.partIndex);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_rightHandWeapon.mass);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_rightHandWeapon.massMultiplier);
      bspyScratchpad(getBSpyID(), "applyPostureControl", m_rightHandWeapon.worldCOMPos);
#endif
    }

    // we now accumulate these values down the tree so they become the total subtree values
    for (int i = getNumberOfParts() - 2; i>=0; i--)
    {
      NmRsEffectorBase *effector = getEffector(i);
      if (effector->isPartOfGroundedChain() || i==m_posture.alternateRoot)
        continue;
      NmRsGenericPart *child = getGenericPartByIndex(effector->getChildIndex());
      NmRsGenericPart *parent = getGenericPartByIndex(effector->getParentIndex());
      parent->m_subtreeMass += child->m_subtreeMass;
      parent->m_subtreeCOM.Add(child->m_subtreeCOM);
      parent->m_subtreeCOMvel.Add(child->m_subtreeCOMvel);
    }

    if (m_posture.alternateRoot >= 0)//really assumes the alternate root is spine_0.  
    {
      NmRsEffectorBase *effector = getEffector(m_posture.alternateRoot);
      NmRsGenericPart *child = getGenericPartByIndex(effector->getChildIndex());
      NmRsGenericPart *parent = getGenericPartByIndex(effector->getParentIndex());
      //We do support limb here first as algorithm assumes subtree mass is trying to get to pelvis
      if (effector->isPartOfGroundedChain())
      {
        parent->m_subtreeMass += child->m_subtreeMass;
        parent->m_subtreeCOM.Add(child->m_subtreeCOM);
        parent->m_subtreeCOMvel.Add(child->m_subtreeCOMvel);
      }
      else
      {
        child->m_subtreeMass += parent->m_subtreeMass;
        child->m_subtreeCOM.Add(parent->m_subtreeCOM);
        child->m_subtreeCOMvel.Add(parent->m_subtreeCOMvel);
      }
    }
    // support limbs are indexed the other way, so we accumulate in the reverse order
    for (int i = 0; i<getNumberOfParts()-1; i++)
    {
      NmRsEffectorBase *effector = getEffector(i);
      if (effector->isPartOfGroundedChain() && i!=m_posture.alternateRoot)
      {
        NmRsGenericPart *child = getGenericPartByIndex(effector->getChildIndex());
        NmRsGenericPart *parent = getGenericPartByIndex(effector->getParentIndex());
        child->m_subtreeMass += parent->m_subtreeMass;
        child->m_subtreeCOM.Add(parent->m_subtreeCOM);
        child->m_subtreeCOMvel.Add(parent->m_subtreeCOMvel);
      }
    }
    // now this applies the gravity opposition and posture control for each part
    for (int i = 0; i<getNumberOfParts() - 1; i++)
    {
      NmRsEffectorBase *effector = getEffector(i);
      rage::Vector3 toCOM, weight;
      // get the torque required to oppose gravity
      if (effector->getJoint()->GetDriveState() != rage::phJoint::DRIVE_STATE_FREE)
      {
        rage::Vector3 torque = getOpposeTorque(i, gravity, toCOM, weight);
        if (effector->is3DofEffector())
        {
          NmRs3DofEffector *e3dof = (NmRs3DofEffector *)effector;
          if (e3dof->getOpposeGravity())
          { // gravity opposition, just apply the calculated torque
            float limit = e3dof->getMuscleStrength()*clampScale*e3dof->getOpposeGravity();
            torque.ClampMag(0, limit);
            // TDL this block below is a bit of a hack to get the character not to stick his heels up on being pushed backwards
            if (e3dof == getLeftLegSetup()->getAnkle() || e3dof == getRightLegSetup()->getAnkle())
            {
              rage::Matrix34 mat;
              getGenericPartByIndex(e3dof->getChildIndex())->getMatrix(mat);
              rage::Vector3 sideways =  mat.a;
              float dot = sideways.Dot(torque);
              if (dot > limit*0.25f) // basically have 1/4 the max torque when tipping backwards (we only have the heel to use)
                torque.SubtractScaled(sideways, (dot - limit*0.25f));
              rage::Vector3 forwards = mat.c;
              dot = forwards.Dot(torque);
              if (dot > limit*0.5f)
                dot -= limit*0.5f;
              else if (dot < -limit*0.5f)
                dot += limit*0.5f;
              else 
                dot = 0.f;
              torque.SubtractScaled(forwards, dot); 
  #if ART_ENABLE_BSPY & 0
              if (e3dof == getLeftLegSetup()->getAnkle())
              {
                bspyScratchpad(getBSpyID(), "anklePostureControlL", torque);
                bspyScratchpad(getBSpyID(), "anklePostureControlL", e3dof->getJointPosition());
                bspyScratchpad(getBSpyID(), "anklePostureControlL", torque.Mag());
              }
              else
              {
                bspyScratchpad(getBSpyID(), "anklePostureControlR", torque);
                bspyScratchpad(getBSpyID(), "anklePostureControlR", e3dof->getJointPosition());
                bspyScratchpad(getBSpyID(), "anklePostureControlR", torque.Mag());
              }
  #endif
            } 
			e3dof->ApplyTorque(VECTOR3_TO_INTRIN(torque));
  #if ART_ENABLE_BSPY && NM_RS_VERBOSE_POSTURE_DEBUG
            bspyDrawLine(e3dof->getJointPosition(), e3dof->getJointPosition()+torque, rage::Vector3(0.f, 1.f, 1.f));
  #endif
          }
        }
        else // 1dof
        {
          NmRs1DofEffector *e1dof = (NmRs1DofEffector *)effector;
          if (e1dof->getOpposeGravity())
          { // gravity opposition, just apply the calculated torque (only in bendable axis)
            rage::phJoint1Dof *oneDof = e1dof->get1DofJoint();
            rage::Vector3 axis = oneDof->GetRotationAxis();
            float dot = torque.Dot(axis);
            float size = e1dof->getMuscleStrength()*clampScale*e1dof->getOpposeGravity();
            dot = rage::Clamp(dot, -size, size);
            axis.Scale(dot);
			e1dof->ApplyTorque(dot);
  #if ART_ENABLE_BSPY && NM_RS_VERBOSE_POSTURE_DEBUG
            bspyDrawLine(e1dof->getJointPosition(), e1dof->getJointPosition()+axis, rage::Vector3(1.f, 0.f, 1.f));
  #endif
          }
        }//if (effector->getJoint()->GetDriveState() != rage::phJoint::DRIVE_STATE_FREE)
      }

    }
    m_attachedObject.partIndex = -1; // so you have to specify the attached object every frame
  }
}

