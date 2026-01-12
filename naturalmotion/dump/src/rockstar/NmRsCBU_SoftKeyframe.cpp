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
 *
 * the character catches his fall when falling over. 
 * He will twist his spine and look at where he is falling. He will also relax after hitting the ground.
 * He always braces against a horizontal ground.
 */


#include "NmRsInclude.h"
#include "NmRsBodyLayout.h"
#include "NmRsCBU_SoftKeyframe.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "nmutils/TypeUtils.h"
//#define LOADCRAWLFROMFILE

namespace ART
{
  NmRsCBUSoftKeyframe::NmRsCBUSoftKeyframe(ART::MemoryManager* services) : CBUTaskBase(services, bvid_softKeyframe)
  {
    initialiseCustomVariables();
  }

  void NmRsCBUSoftKeyframe::initialiseCustomVariables()
  {
    m_mask = bvmask_Full;
  }

  NmRsCBUSoftKeyframe::~NmRsCBUSoftKeyframe()
  {
  }

  const rage::Vector3 NmRsCBUSoftKeyframe::getPosition(const rage::Matrix34& matrix) const
  {
    return matrix.d;
  }

  void NmRsCBUSoftKeyframe::onActivate()
  {
    Assert(m_character);

    m_character->disableSelfCollision();

    // locally cache the limb definitions
    m_leftArm = m_character->getLeftArmSetup();
    m_rightArm = m_character->getRightArmSetup();
    m_leftLeg = m_character->getLeftLegSetup();
    m_rightLeg = m_character->getRightLegSetup();
    m_spine = m_character->getSpineSetup();

    rage::Vector3 direction = m_spine->getSpine3Part()->getPosition() - m_spine->getPelvisPart()->getPosition();
    m_yawOffset = atan2f(-direction.x, -direction.z);

#if defined (LOADCRAWLFROMFILE)
    // load up the CTM - the big list of transforms
    //        FILE *inTm = fopen("GAME:\\Endiancrawl4.ctm", "rb");        
    FILE *inTm = fopen("d:\\EndianSwap\\crawl4.ctm", "rb");        
    if (inTm)        
    {    
      fseek(inTm, 0, SEEK_END);
      int fSize = ftell(inTm);
      fseek(inTm, 0, SEEK_SET);

      unsigned char* fileData = new unsigned char[fSize];
      fread(fileData, sizeof(unsigned char), fSize, inTm);

      fseek(inTm, 0, SEEK_SET);

      int numCtmTransforms = fSize / sizeof(rage::Matrix34);
      m_ctmTransforms = new rage::Matrix34[numCtmTransforms];
      fread(m_ctmTransforms, sizeof(rage::Matrix34), numCtmTransforms, inTm);

      m_numberOfFrames = numCtmTransforms / 21;

      fclose(inTm);

    }
    m_frame = 0;
#else

#endif
    m_body->resetEffectors(kResetCalibrations | kResetAngles);
    for (int j = 0; j<20; j++)
    {
      float stiffness = 4.f; // TDL up this to 14 if it isn't moving quick enough.

      m_character->getEffectorDirect(j)->setMuscleStrength(stiffness*stiffness);
      m_character->getEffectorDirect(j)->setMuscleDamping(2.f*stiffness*1.f);
    }
    supposedToBe = m_spine->getPelvisPart()->getPosition();
  }

  void NmRsCBUSoftKeyframe::onDeactivate()
  {
    Assert(m_character);

#if defined (LOADCRAWLFROMFILE)
    delete[] m_ctmTransforms;
#endif
    m_active = false;
  }

  CBUTaskReturn NmRsCBUSoftKeyframe::onTick(float timeStep)
  { 
    rage::Matrix34 *matrices;
    rage::Matrix34 *nextMatrices;
    rage::Matrix34 *oldMatrices;

    int pelvisIndex = m_spine->getPelvisPart()->getPartIndex();
#if defined(LOADCRAWLFROMFILE)
    matrices = &m_ctmTransforms[NUM_PARTS * m_frame];
    nextMatrices = &m_ctmTransforms[NUM_PARTS * ((m_frame+1)%m_numberOfFrames)];
    oldMatrices = m_frame==m_numberOfFrames-1 ? nextMatrices : matrices;
    m_frame = (m_frame+1) % m_numberOfFrames;
#else
    IncomingTransformStatus status = kITSNone;
    int numParts = NUM_PARTS;
    m_character->getIncomingTransforms(&matrices, status, numParts, kITSourcePrevious);
    Assert(numParts == NUM_PARTS); // need to pass in the previous transforms too
    m_character->getIncomingTransforms(&nextMatrices, status, numParts, kITSourceCurrent);
    Assert(numParts == NUM_PARTS); // need to pass in the current transforms
    float gap = (getPosition(matrices[pelvisIndex]) - getPosition(nextMatrices[pelvisIndex])).Mag();
    oldMatrices = gap>0.3f ? nextMatrices : matrices;
#endif
    float rotateScale = 5.f;
    float maxRotateSpeed = 0.5f;
    if (m_parameters.targetPosition != rage::Vector3(0,0,0))
    {
      NM_RS_CBU_DRAWPOINT(m_parameters.targetPosition, 1.f, rage::Vector3(1,0,0));
      rage::Vector3 toTarget = m_parameters.targetPosition - m_spine->getSpine3Part()->getPosition();
      toTarget.y = 0.f;
      float yawOffset = atan2f(-toTarget.x, -toTarget.z);
      float yawDelta = yawOffset - m_yawOffset;
      if (yawDelta > PI)
        yawDelta -= 2.f*PI;
      if (yawDelta < -PI)
        yawDelta += 2.f*PI;
      float rotateVel = NMutils::clampValue(rotateScale * yawDelta, -maxRotateSpeed, maxRotateSpeed);
      m_yawOffset += rotateVel * timeStep;
    }
    else
      m_yawOffset = m_parameters.yawOffset;
    rage::Vector3 offset = getPosition(oldMatrices[m_spine->getSpine1Part()->getPartIndex()]);
    rage::Matrix34 rotation;
    rotation.Identity();
    rotation.RotateY(m_yawOffset);
    rage::Matrix34 oldMats[NUM_PARTS], nextMats[NUM_PARTS];
    for (int i = 0; i<NUM_PARTS; i++) // get matrices into a nice usable form.
    {
      oldMats[i] = oldMatrices[i];
      nextMats[i] = nextMatrices[i];
      oldMats[i].d -= offset;
      oldMats[i].Dot(rotation);
      oldMats[i].d += offset;
      nextMats[i].d -= offset;
      nextMats[i].Dot(rotation);
      nextMats[i].d += offset;
    }

    rage::Vector3 rootPos = oldMats[pelvisIndex].d;
    rage::Vector3 currentRootPos = m_character->getGenericPartByIndex(pelvisIndex)->getPosition();
    rage::Vector3 rootVel = m_character->getGenericPartByIndex(pelvisIndex)->getLinearVelocity();

    rage::Matrix34 currentM;
    currentM.Identity();

    rage::Vector3 normal;
    rage::Vector3 limb[4];
    rage::Vector3 totalAnimNormal(0,0,0);
    int leftUpperArm = m_leftArm->getUpperArm()->getPartIndex();
    int rightUpperArm = m_rightArm->getUpperArm()->getPartIndex();
    int rightThigh = m_rightLeg->getThigh()->getPartIndex();
    int leftThigh = m_leftLeg->getThigh()->getPartIndex();
    limb[0] = oldMats[leftUpperArm].d;
    limb[1] = oldMats[rightUpperArm].d;
    limb[2] = oldMats[rightThigh].d;
    limb[3] = oldMats[leftThigh].d;
    for (int i = 0; i<4; i++)
    {
      normal.Cross(limb[(i+1)%4] - limb[i%4], limb[(i+2)%4] - limb[i%4]);
      normal.Normalize();
      totalAnimNormal += normal;
    }
    rage::Vector3 totalCurrentNormal(0,0,0);
    limb[0] = m_character->getGenericPartByIndex(leftUpperArm)->getPosition();
    limb[1] = m_character->getGenericPartByIndex(rightUpperArm)->getPosition();
    limb[2] = m_character->getGenericPartByIndex(rightThigh)->getPosition();
    limb[3] = m_character->getGenericPartByIndex(leftThigh)->getPosition();
    for (int i = 0; i<4; i++)
    {
      normal.Cross(limb[(i+1)%4] - limb[i%4], limb[(i+2)%4] - limb[i%4]);
      normal.Normalize();
      totalCurrentNormal += normal;
    }
    rage::Quaternion quat;
    quat.FromVectors(totalAnimNormal, totalCurrentNormal);
    currentM.FromQuaternion(quat);

    rootPos.y = nextMats[pelvisIndex].d.y - rootVel.Dot(currentM.b)*timeStep;
    bool bRelativeMotion = false;
    if (bRelativeMotion)
      currentM.d = currentRootPos;
    else
    {
      supposedToBe += currentM.b * (currentRootPos - supposedToBe).Dot(currentM.b); // be current in up/down
      if (m_parameters.goSlowerWhenWeaker)
        supposedToBe = supposedToBe*m_parameters.followMultiplier + currentRootPos*(1.f-m_parameters.followMultiplier);
      currentM.d = supposedToBe;
      rage::Vector3 shift = nextMats[pelvisIndex].d - rootPos;
      currentM.Transform3x3(shift);
      supposedToBe += shift;
    }

    //float totalAccY = 0.f;
    //float totalNum = 0.f;
    for (int i = 0; i<NUM_PARTS; i++)
    {
      if(m_character->isPartInMask(m_parameters.mask, i))
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);

        rage::Matrix34 desiredDiff = nextMats[i];
        desiredDiff.d -= rootPos;
        rage::Matrix34 desiredMat;
        desiredMat.Dot(desiredDiff, currentM);

        rage::Vector3 vel = (desiredMat.d - part->getPosition())/timeStep;
        rage::Vector3 acc = (vel - part->getLinearVelocity())/timeStep;
        acc.ClampMag(0.f, m_parameters.maxAcceleration);
        acc *= m_parameters.followMultiplier;
        vel = part->getLinearVelocity() + acc*timeStep;
        part->setLinearVelocity(vel, false);

        rage::Matrix34 currentMat;
        part->getMatrix(currentMat);
        // rage::Matrix34 desiredMat = getRageMatrix2(nextMatrices[i]);
        NM_RS_CBU_DRAWCOORDINATEFRAME(0.2f, desiredMat);
        rage::Matrix34 rotMat;
        currentMat.Transpose();
        rotMat.Dot3x3(currentMat, desiredMat); // TDL don't know why this dot is the wrong way round
        rage::Quaternion quat;
        rotMat.ToQuaternion(quat);
        rage::Vector3 rotVel;
        float angle;
        quat.ToRotation(rotVel, angle);
        rotVel *= angle/timeStep;
        rage::Vector3 rotAcc = (rotVel - part->getAngularVelocity())/timeStep;
        rotAcc.ClampMag(0.f, m_parameters.maxAcceleration);
        rotAcc *= m_parameters.followMultiplier;
        rotVel = part->getAngularVelocity() + rotAcc*timeStep;

        part->setAngularVelocity(rotVel, false);
      }
    }

    return eCBUTaskComplete;
  }

#if ART_ENABLE_BSPY
  void NmRsCBUSoftKeyframe::sendParameters(NmRsSpy& spy)
  {
    CBUTaskBase::sendParameters(spy);

    bspyTaskVar(m_parameters.followMultiplier, true);
    bspyTaskVar(m_parameters.maxAcceleration, true);
    bspyTaskVar(m_parameters.goSlowerWhenWeaker, true);
    bspyTaskVar(m_parameters.yawOffset, true);
    bspyTaskVar(m_parameters.targetPosition, true);

    bspyTaskVar(m_numberOfFrames, false);
    bspyTaskVar(m_frame, false);
    bspyTaskVar(m_yawOffset, false);
  }
#endif // ART_ENABLE_BSPY
}
