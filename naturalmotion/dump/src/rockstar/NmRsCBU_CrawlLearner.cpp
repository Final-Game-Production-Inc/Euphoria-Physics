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

#if CRAWL_LEARNING

#include "NmRsBodyLayout.h"
#include "NmRsCBU_LearnedCrawl.h" 
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "ART/ARTFeedback.h"
#include "nmutils/TypeUtils.h"


namespace ART
{
    // load up the CTM - the big list of transforms
    void NmRsCBULearnedCrawl::loadCTM()
    {
      FILE *inTm = NULL;
      // open file that we have just produced in behaviour
      if(m_parameters.learnFromAnimPlayback)
      {
#if defined(_XBOX)
        inTm = fopen("GAME:\\Crawl.ctm", "rb");        
#else
        inTm = fopen("c:\\CrawlFromPC.ctm", "rb");        
#endif
        Assertf(inTm!=NULL, "Crawl: in animPlayback mode failed to open just extracted .ctm file");
      }
      // open file that was previously stored
      else
      {
#if defined(_XBOX)
        if (m_animIndex == 0)
          inTm = fopen("GAME:\\gped_crw_full_for_wlk_NM_MocapXBox.ctm", "rb");      
        else if (m_animIndex == 1)
          inTm = fopen("GAME:\\crawlInjuredRightLegXbox.ctm", "rb");        
        else if (m_animIndex == 2)
          inTm = fopen("GAME:\\newCrawlXBox.ctm", "rb");        
        else if (m_animIndex == 3)
          inTm = fopen("GAME:\\savedCrawl.ctm", "rb");        
#else
        inTm = fopen("c:\\gped_crw_full_for_wlk_NM_Mocap.ctm", "rb");        
#endif
        Assertf(inTm!=NULL, "Crawl: in non-animPlayback mode, unable to open stored .ctm file for index %i.", m_animIndex);
      }

      if (inTm != NULL)        
      {    
        fseek(inTm, 0, SEEK_END);
        int fSize = ftell(inTm);
        fseek(inTm, 0, SEEK_SET);

        fseek(inTm, 0, SEEK_SET);

        int numCtmTransforms = fSize / sizeof(rage::Matrix34);
        m_ctmTransforms = rage_new rage::Matrix34[numCtmTransforms];

        m_ctmVels       = rage_new rage::Vector3[numCtmTransforms];

        fread(m_ctmTransforms, sizeof(rage::Matrix34), numCtmTransforms, inTm);

        m_numberOfFrames = numCtmTransforms / 21;

        m_bucket = rage_new Bucket[m_numberOfFrames];
        m_sequence = rage_new Sequence[m_numberOfFrames];
        m_bestSequence = rage_new Sequence[m_numberOfFrames];
        memset(m_bucket, 0, sizeof(Bucket)*m_numberOfFrames);
        memset(m_sequence, 0, sizeof(Sequence)*m_numberOfFrames);
        fclose(inTm);

        m_count = 30.f;
        m_bestTotalError = 0.f;
      }
    }

    void NmRsCBULearnedCrawl::buildIKBucket(NmRsHumanArm *arm, int i, Limbs limb)
    {
      rage::Matrix34 *thisMatrix = &m_ctmTransforms[NUM_PARTS*i];

      Bucket *bucket = &m_bucket[i];
      rage::Vector3 *vels = &m_ctmVels[NUM_PARTS*i];
      Sequence *sequence = &m_sequence[i];
      int handIndex;
      //bucket->m_ik[lLeftArm].desiredTwist = ((NmRs3DofEffector *)m_character->getEffector(jLeftShoulder))->getDesiredTwist();
      //bucket->m_ik[lLeftArm].desiredTwistVel = m_rotVelBuffer[jLeftShoulder].z/dt;

      int rootIndex, clavIndex = 0;
      rage::Matrix34 root;
      if(m_parameters.useSpine3Thing)
      {
        rootIndex = getSpine()->getSpine3Part()->getPartIndex();
        clavIndex = arm->getClaviclePart()->getPartIndex();

        root = thisMatrix[rootIndex];

        // root.d.y = getPosition(thisMatrix[clavIndex]).y; // Mod: not gUP safe
        m_character->levelVector(root.d, m_character->vectorHeight(thisMatrix[clavIndex].d));
      }
      else
      {
        rootIndex = arm->getClaviclePart()->getPartIndex();
        root = thisMatrix[rootIndex];
      }

      handIndex = arm->getHand()->getPartIndex();
      bucket->m_ik[limb].desiredTarget = thisMatrix[handIndex].d;

      if (bSupporting[limb])
      {
        bucket->m_ik[limb].desiredTarget -= root.d;
        bucket->m_ik[limb].desiredTargetVel = vels[handIndex] - vels[rootIndex];
        if(m_parameters.useSpine3Thing)
        {
          // bucket->m_ik[limb].desiredTargetVel.y = vels[handIndex].y - vels[clavIndex].y; // Mod: not gUP safe
          float height = m_character->vectorHeight(vels[handIndex]) - m_character->vectorHeight(vels[clavIndex]);
          m_character->levelVector(bucket->m_ik[limb].desiredTargetVel, height);
        }
      }
      else 
      {
        rage::Vector3 leveledRootPos = root.d;
        m_character->levelVector(leveledRootPos);
        bucket->m_ik[limb].desiredTarget -= leveledRootPos;
        rage::Vector3 leveledRootVel = vels[rootIndex];
        m_character->levelVector(leveledRootVel);
        bucket->m_ik[limb].desiredTargetVel = vels[handIndex] - leveledRootVel;
      } 

      sequence->m_ik[limb].target = bucket->m_ik[limb].desiredTarget;
      sequence->m_ik[limb].twist = -0.3f;
    }

    void NmRsCBULearnedCrawl::buildBuckets()
    {
      float animTimeStep = 1.f/30.f;
      for (int i = 0; i<m_numberOfFrames; i++) // temporarily localise the animation to the root.
      {
        rage::Matrix34 *thisMatrix = &m_ctmTransforms[NUM_PARTS*i];
        rage::Matrix34 *nextMatrix = &m_ctmTransforms[NUM_PARTS*((i+1)%m_numberOfFrames)];
        rage::Matrix34 *nextNextMatrix = &m_ctmTransforms[NUM_PARTS*((i+2)%m_numberOfFrames)];

        m_character->setIncomingTransforms(thisMatrix, kITSNone, NUM_PARTS, kITSourcePrevious);
        m_character->setIncomingTransforms(nextMatrix, kITSNone, NUM_PARTS, kITSourceCurrent);
        rage::Vector3 *vels = &m_ctmVels[NUM_PARTS*i];
        if (i==m_numberOfFrames-1) // TDL this isn't ideal, but how do we find out the velocity otherwise?
        {
          thisMatrix = nextMatrix;
          nextMatrix = nextNextMatrix;
        }

        for (int j = 0; j<NUM_PARTS; j++)
          vels[j] = ((nextMatrix[j]).d - (thisMatrix[j]).d)/animTimeStep;

        Bucket *bucket = &m_bucket[i];
        Sequence *sequence = &m_sequence[i];
        for (int j = 0; j<NUM_JOINTS; j++)
        {
          Bucket::EffectorBucket &eff = bucket->m_effector[j];
          if (m_character->getConstEffector(j)->is3DofEffector())
            ((NmRs3DofEffector *)m_character->getConstEffector(j))->activeAnimInfo(animTimeStep, &eff.desiredLean1, &eff.desiredLean2, &eff.desiredTwist, &eff.desiredLean1Vel, &eff.desiredLean2Vel, &eff.desiredTwistVel);
          else
            ((NmRs1DofEffector *)m_character->getConstEffector(j))->activeAnimInfo(animTimeStep, &eff.desiredLean1, &eff.desiredLean1Vel);

        }

        if(m_parameters.useRollBoneCompensation)
        {
          rage::Quaternion quat;
          rage::Vector3 rotVel;

          getLeftLeg()->getKnee()->getJointQuatPlusVelFromIncomingTransform(quat, rotVel);
          rage::Vector3 tss = rsQuatToRageDriveTwistSwing(quat);

          float extraAng = -2.f*atan2f(tss.z, -tss.x);
          int lHip = getLeftLeg()->getHip()->getJointIndex();
          bucket->m_effector[lHip].desiredTwist += extraAng;
          sequence->m_effector[lHip].twist = bucket->m_effector[lHip].desiredTwist;

          getRightLeg()->getKnee()->getJointQuatPlusVelFromIncomingTransform(quat, rotVel);
          tss = rsQuatToRageDriveTwistSwing(quat);      

          extraAng = 2.f*atan2f(tss.z, -tss.x);
          int rHip = getRightLeg()->getHip()->getJointIndex();
          bucket->m_effector[rHip].desiredTwist += extraAng;
          sequence->m_effector[rHip].twist = bucket->m_effector[rHip].desiredTwist;
        }

        for (int j = 0; j<NUM_JOINTS; j++)
        {
          sequence->m_effector[j].lean1 = bucket->m_effector[j].desiredLean1;
          sequence->m_effector[j].lean2 = bucket->m_effector[j].desiredLean2;
          sequence->m_effector[j].twist = bucket->m_effector[j].desiredTwist;
        }

        setSupportingLimbs(bSupporting, i);
        buildIKBucket(getLeftArm(), i, lLeftArm);
        buildIKBucket(getRightArm(), i, lRightArm);
      }
    }

    void NmRsCBULearnedCrawl::resetCharacterToStart()
    {
      m_character->disableSelfCollision();
      m_character->getArticulatedWrapper()->getArticulatedCollider()->Reset();

      rage::Matrix34 *frame1 = &m_ctmTransforms[0*21];
      rage::Matrix34 *frame2 = &m_ctmTransforms[1*21];

      for (int i = 0; i<21; i++)
      {
        NmRsGenericPart *part = m_character->getGenericPartByIndex(i);

        rage::Matrix34 mat = frame1[i];

        mat.Normalize();
        part->setMatrix(mat); 
        part->applyVelocitiesToPart(frame1[i], frame2[i], 30.f);
      }
      m_character->updateArticulatedWrapperInertias();
    }

    void NmRsCBULearnedCrawl::saveDriveSequence()
    {
      // load up the CTM - the big list of transforms
#if defined(_XBOX)
      FILE *out = fopen("GAME:\\learnedCrawl.seq", "wb");     
#else
      FILE *out = fopen("c:\\learnedCrawlFromPC.seq", "wb");     
#endif
      if (out)        
      {    
        // save out the desired joint angles:
        fwrite(m_bestSequence, sizeof(Sequence), m_numberOfFrames, out);
        fclose(out);
      }
    }

#if ART_ENABLE_BSPY
    void NmRsCBULearnedCrawl::drawAnimation()
    {
      rage::Matrix34 *m;

      rage::Vector3 v1, v2, v3, v4;
      // left arm
      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getLeftArm()->getClaviclePart()->getPartIndex()];
      v1 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getLeftArm()->getUpperArm()->getPartIndex()];
      v2 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getLeftArm()->getLowerArm()->getPartIndex()];
      v3 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getLeftArm()->getHand()->getPartIndex()];
      v4 = (*m).d;

      m_character->bspyDrawLine(v1, v2, rage::Vector3(1,0,1));
      m_character->bspyDrawLine(v2, v3, rage::Vector3(1,0,1));
      m_character->bspyDrawLine(v3, v4, rage::Vector3(1,0,1));
      // right arm
      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getRightArm()->getClaviclePart()->getPartIndex()];
      v1 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getRightArm()->getUpperArm()->getPartIndex()];
      v2 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getRightArm()->getLowerArm()->getPartIndex()];
      v3 = (*m).d;

      m = &m_ctmTransforms[NUM_PARTS*m_frameIndex + getRightArm()->getHand()->getPartIndex()];
      v4 = (*m).d;

      m_character->bspyDrawLine(v1, v2, rage::Vector3(1,1,0));
      m_character->bspyDrawLine(v2, v3, rage::Vector3(1,1,0));
      m_character->bspyDrawLine(v3, v4, rage::Vector3(1,1,0));

      for (int i = 0; i<21; i++) // draw animation
      {
        rage::Matrix34 rageMatrix = (m_ctmTransforms[NUM_PARTS*m_frameIndex + i]);
        rage::Matrix34 mat;
        mat = rageMatrix;
        //mat.Translate(0.f, 0.f, 2.f);
        m_character->bspyDrawCoordinateFrame(0.1f, mat);
        if (i==getRightArm()->getClaviclePart()->getPartIndex())
        {
          int spine3 = getSpine()->getSpine3Part()->getPartIndex();
          rage::Vector3 pos = (m_ctmTransforms[NUM_PARTS*m_frameIndex + spine3]).d;

          m_character->levelVector(pos, m_character->vectorHeight(mat.d));
          m_character->bspyDrawLine(pos, pos + m_sequence[m_frameIndex].m_ik[lRightArm].target, rage::Vector3(1,1,0));
        }
        if (i==getLeftArm()->getClaviclePart()->getPartIndex())
        {
          int spine3 = getSpine()->getSpine3Part()->getPartIndex();
          rage::Vector3 pos = (m_ctmTransforms[NUM_PARTS*m_frameIndex + spine3]).d;

          m_character->levelVector(pos, m_character->vectorHeight(mat.d));
          m_character->bspyDrawLine(pos, pos + m_sequence[m_frameIndex].m_ik[lLeftArm].target, rage::Vector3(1,0,1));
        }
        if (i== getRightLeg()->getShin()->getPartIndex())
        {
          rage::Vector3 kneePos(0.f, 0.2f, 0.f);
          mat.Transform(kneePos);
          m_character->bspyDrawPoint(kneePos, 0.2f, rage::Vector3(1,1,1));
        }
      }

      if (bSupporting[lLeftArm])
      { 
        m_character->bspyDrawPoint(getLeftArm()->getHand()->getPosition(), 0.1f, rage::Vector3(1,0,0));
      }
      else 
      {                      
        m_character->bspyDrawPoint(getRightArm()->getHand()->getPosition(), 0.1f, rage::Vector3(1,0,0));
      }

      if (bSupporting[lLeftLeg])
      { 
        m_character->bspyDrawPoint(getLeftLeg()->getFoot()->getPosition(), 0.1f, rage::Vector3(1,0,0));
      }
      else
      {
        m_character->bspyDrawPoint(getRightLeg()->getFoot()->getPosition(), 0.1f, rage::Vector3(1,0,0));
      }
    }
#endif //ART_ENABLE_BSPY

    void NmRsCBULearnedCrawl::learnIKDrive(NmRsHumanArm *arm, Limbs limb, float scale, Bucket *bucket, Sequence *sequence, Sequence *nextSequence)
    {
      //shoulder = (NmRs3DofEffector *)m_character->getEffector(jLeftShoulder);
      //twistErrorVel = bucket->m_ik[lLeftArm].desiredTwistVel - shoulder->getActualTwistVel();
      //bucket->m_ik[lLeftArm].twistError += (twistError + twistErrorVel*d/s)*(twistError + twistErrorVel*d/s);
      //sequence->m_ik[lLeftArm].twist += (twistError + twistErrorVel*d/s) * scale;
      //sequence->m_ik[lLeftArm].twist = rage::Clamp(sequence->m_ik[lRightArm].twist, -2.f, 2.f);
      float s = arm->getShoulder()->getMuscleStrength();
      float d = arm->getShoulder()->getMuscleDamping();

      NmRsGenericPart *clavicle = arm->getClaviclePart();
      rage::Vector3 rootPos = clavicle->getPosition();
      rage::Vector3 hitPos, hitNormal;
      bool bHit = false;
      rage::Vector3 probeUp = 2.0f*m_character->m_gUp;
      if (!bSupporting[limb] || !bNextSupporting[limb])
      {
        bHit = m_character->probeRay(NmRsCharacter::pi_UseNonAsync, rootPos, rootPos - probeUp, rage::phLevelBase::STATE_FLAGS_ALL, TYPE_FLAGS_ALL, m_character->m_probeTypeIncludeFlags, m_character->m_probeTypeExcludeFlags, false);
      }

      NmRsGenericPart *hand = arm->getHand();
      rage::Vector3 actualTarget = hand->getPosition();
      rage::Vector3 velHoriz = hand->getLinearVelocity();
      //velHoriz.y = 0.f; // Mod: not gUP safe
      m_character->levelVector(velHoriz);
      //float shiftY = 0.f;
      // seems to cause some level of divergence
#if 0
      if (bSupporting[limb]) // push down if we're sliding while supporting
      {
      //        float speed = rage::Clamp(velHoriz.Mag() - 0.1f, 0.f, 10.f);
      //        if (speed > 0.f)
      //          shiftY = -speed*scale*10.f; 
      } 
      else // pull up if we catch on ground while in air 
      {
      float speed = velHoriz.Mag();
      rage::Vector3 desiredVel = vels[arm->getHand()->getPartIndex()];
      desiredVel.y = 0.f;
      float desiredSpeed = desiredVel.Mag();
      float diff = rage::Clamp(desiredSpeed - 0.1f - speed, 0.f, 10.f);
      if (diff>0.f)
      shiftY = diff*scale*10.f;
      }
#endif
      //bucket->m_ik[limb].desiredTarget.y += shiftY; // demonstrates that it doesn't help so much with the actual friction
      //      actualTarget.y -= shiftY;

      // position rootPos horizontally at spine3 but keep original height
      if(m_parameters.useSpine3Thing)
      {       
        rage::Vector3 tmp = getSpine()->getSpine3Part()->getPosition();
        // rootPos.x = tmp.x; rootPos.z = tmp.z; // Mod: not gUP safe
        float originalHeight = m_character->vectorHeight(rootPos);
        rootPos = tmp;
        m_character->levelVector(rootPos, originalHeight);
      }

      if (bSupporting[limb])
      {
        actualTarget -= rootPos;
      }
      else
      {
        if (bHit)
        {
          //rootPos.y = hitPos.y; // Mod: not gUP safe
          m_character->levelVector(rootPos, m_character->vectorHeight(hitPos));
        }
        actualTarget -= rootPos;
      }

      rage::Vector3 error = bucket->m_ik[limb].desiredTarget - actualTarget;
      bucket->m_ik[limb].error += error.Mag2();
      rage::Vector3 clavVel = clavicle->getLinearVelocity();

      // make rootVel horizontally like spine3's but keep original height
      if(m_parameters.useSpine3Thing)
      {
        rage::Vector3 tmpVel = getSpine()->getSpine3Part()->getLinearVelocity();
        // clavVel.x = tmpVel.x; clavVel.z = tmpVel.z;// Mod: not gUP safe
        float originalHeight = m_character->vectorHeight(clavVel);
        clavVel = tmpVel;
        m_character->levelVector(clavVel, originalHeight);
      }

      rage::Vector3 actualTargetVel;
      if (bSupporting[limb])
      {
        actualTargetVel = hand->getLinearVelocity() - clavVel;
      }
      else
      {
        //actualTargetVel = hand->getLinearVelocity() - rage::Vector3(clavVel.x, 0, clavVel.z);// Mod: not gUP safe
        rage::Vector3 leveledClavVel = clavVel;
        m_character->levelVector(leveledClavVel);
        actualTargetVel = hand->getLinearVelocity() - leveledClavVel;
      }
      rage::Vector3 errorVel = bucket->m_ik[limb].desiredTargetVel - actualTargetVel;
      bucket->m_ik[limb].error += errorVel.Mag2() * (d*d)/(s*s);
      sequence->m_ik[limb].target += (error + errorVel*d/s) * scale*0.5f;
      rage::Vector3 target = nextSequence->m_ik[limb].target;

      if(m_parameters.useSpine3Thing)
      {
        rootPos = getSpine()->getSpine3Part()->getPosition();
        rootPos.y = clavicle->getPosition().y;// Mod: not gUP safe
        m_character->levelVector(rootPos, m_character->vectorHeight(clavicle->getPosition()));
      }
      else
      {
        rootPos = clavicle->getPosition();
      }

      if (!bNextSupporting[limb])
        if (bHit)
          //rootPos.y = hitPos.y;// Mod: not gUP safe
          m_character->levelVector(rootPos, m_character->vectorHeight(hitPos));
      target += rootPos;

      NmRsLimbInput ikInput = createNmRsLimbInput<NmRsIKInputWrapper>();
      NmRsIKInputWrapper* ikInputData = ikInput.getData<NmRsIKInputWrapper>();
      ikInputData->setTarget(target);
      ikInputData->setTwist(nextSequence->m_ik[limb].twist);
      ikInputData->setDragReduction(0.0f);
      if (limb == lLeftArm)
        getLeftArm()->postInput(ikInput);
      else
        getRightArm()->postInput(ikInput);
    }

    // TDL this is offline learning
    void NmRsCBULearnedCrawl::learnDriveSequence()
    {
      if (m_totalFrame == 0)
        resetCharacterToStart();
#if ART_ENABLE_BSPY
      drawAnimation();
#endif
      Bucket *bucket = &m_bucket[m_frameIndex];
      Sequence *sequence = &m_sequence[m_frameIndex];
      Sequence *nextSequence = &m_sequence[(m_frameIndex + 1) % m_numberOfFrames];

      float scale = 0.05f;

      // TDL spine twist code to hopefully help with skidding
      float extraTwist = 0.f;
      //rage::Vector3 kneePos(0.f, 0.2f, 0.f);

      if(m_parameters.useTwister)
      {
        float twistAmount = 0.05f; // 0.05 seems better, shouldn't be any more than 0.2.
        //MMMM no twist if supporting on both arms at same time.  I need to think about this.  Maybe look at if leftArm and rightLeg are supporting.
        if (bNextSupporting[lLeftArm])
          extraTwist -= twistAmount;
        if (bNextSupporting[lRightArm])
          extraTwist += twistAmount;
        if (bNextSupporting[lLeftLeg])
          extraTwist += twistAmount;
        if (bNextSupporting[lRightLeg])
          extraTwist -= twistAmount;
      }

      //NM_RS_CBU_DRAWPOINT(kneePos, 0.2f, rage::Vector3(1,1,1));

      for (int i = 0; i<NUM_JOINTS; i++) // learn non-ik joint angles
      {
        if (i >= getLeftArm()->getShoulder()->getJointIndex() && i<= getLeftArm()->getWrist()->getJointIndex() &&
          i >= getRightArm()->getShoulder()->getJointIndex() && i<= getRightArm()->getWrist()->getJointIndex())
          continue; // doing IK for these joints

        float s = m_character->getConstEffector(i)->getMuscleStrength();
        float d = m_character->getConstEffector(i)->getMuscleDamping();
        if (m_character->getConstEffector(i)->is3DofEffector())
        {
          NmRs3DofEffector *threeDof = (NmRs3DofEffector *)m_character->getEffectorDirect(i);

          float extra = 0.f;
          int h0 = getSpine()->getSpine0()->getJointIndex();
          int h1 = getSpine()->getSpine1()->getJointIndex();
          int h2 = getSpine()->getSpine3()->getJointIndex();
          if ((i >= h1 && i <= h2) || i==h0)
            extra = extraTwist;

          float errorL1 = bucket->m_effector[i].desiredLean1 - threeDof->getActualLean1();
          bucket->m_effector[i].lean1Error += errorL1*errorL1;
          sequence->m_effector[i].lean1 += errorL1 * scale;
          float errorL2 = bucket->m_effector[i].desiredLean2 - threeDof->getActualLean2();
          bucket->m_effector[i].lean2Error += errorL2*errorL2;
          sequence->m_effector[i].lean2 += errorL2 * scale;
          float errorTw = bucket->m_effector[i].desiredTwist+extra - threeDof->getActualTwist();
          bucket->m_effector[i].twistError += errorTw*errorTw;
          sequence->m_effector[i].twist += errorTw * scale;

          float errorL1Vel = bucket->m_effector[i].desiredLean1Vel - threeDof->getActualLean1Vel();
          bucket->m_effector[i].lean1VelError += errorL1Vel*errorL1Vel * (d*d)/(s*s);
          sequence->m_effector[i].lean1 += errorL1Vel * scale* d/s;
          float errorL2Vel = bucket->m_effector[i].desiredLean2Vel - threeDof->getActualLean2Vel();
          bucket->m_effector[i].lean2VelError += errorL2Vel*errorL2Vel * (d*d)/(s*s);
          sequence->m_effector[i].lean2 += errorL2Vel * scale* d/s;
          float errorTwVel = bucket->m_effector[i].desiredTwistVel - threeDof->getActualTwistVel();
          bucket->m_effector[i].twistVelError += errorTwVel*errorTwVel * (d*d)/(s*s);
          sequence->m_effector[i].twist += errorTwVel * scale* d/s;

          sequence->m_effector[i].lean1 = rage::Clamp(sequence->m_effector[i].lean1, -4.f, 4.f);
          sequence->m_effector[i].lean2 = rage::Clamp(sequence->m_effector[i].lean2, -4.f, 4.f);
          sequence->m_effector[i].twist = rage::Clamp(sequence->m_effector[i].twist, -4.f, 4.f); 
          threeDof->setDesiredLean1(nextSequence->m_effector[i].lean1);
          threeDof->setDesiredLean2(nextSequence->m_effector[i].lean2);
          threeDof->setDesiredTwist(nextSequence->m_effector[i].twist); 
        }
        else
        {
          NmRs1DofEffector *oneDof = (NmRs1DofEffector *)m_character->getEffectorDirect(i);

          float error = bucket->m_effector[i].desiredLean1 - oneDof->getActualAngle();
          bucket->m_effector[i].lean1Error += error*error;
          sequence->m_effector[i].lean1 += error * scale;

          float errorVel = bucket->m_effector[i].desiredLean1Vel - oneDof->getActualAngleVel();
          bucket->m_effector[i].lean1VelError += errorVel*errorVel * (d*d)/(s*s);
          sequence->m_effector[i].lean1 += errorVel * scale * d/s; 

          sequence->m_effector[i].lean1 = rage::Clamp(sequence->m_effector[i].lean1, -4.f, 4.f);
          oneDof->setDesiredAngle(nextSequence->m_effector[i].lean1); 
        }
      }

      // do IK learning
      //rage::Vector3 *vels    = &m_ctmVels[NUM_PARTS*m_frameIndex];
      learnIKDrive(getLeftArm(), lLeftArm, scale, bucket, sequence, nextSequence);
      learnIKDrive(getRightArm(), lRightArm, scale, bucket, sequence, nextSequence);

      m_totalFrame++;
      m_frameIndex = m_totalFrame % m_numberOfFrames;
      //      if (m_totalFrame == (int)m_count)
      if (m_frameIndex == 0)
      {
        m_totalError = 0.f;
        int top = (int)m_count > m_numberOfFrames ? m_numberOfFrames : (int)m_count;
        for (int i = 0; i<top; i++)
        {
          for (int j = 0; j<NUM_JOINTS; j++)
          {
            m_totalError += m_bucket[i].m_effector[j].lean1Error;
            m_totalError += m_bucket[i].m_effector[j].lean2Error;
            m_totalError += m_bucket[i].m_effector[j].twistError;
            m_totalError += m_bucket[i].m_effector[j].lean1VelError;
            m_totalError += m_bucket[i].m_effector[j].lean2VelError;
            m_totalError += m_bucket[i].m_effector[j].twistVelError;
          } 
          m_totalError += m_bucket[i].m_ik[0].error;
          m_totalError += m_bucket[i].m_ik[1].error;
        }
        NM_RS_DBG_LOGF(L"error: %f", m_totalError / m_count);
        for (int i = 0; i<m_numberOfFrames; i++)
        {
          for (int j = 0; j<NUM_JOINTS; j++)
          {
            m_bucket[i].m_effector[j].lean1Error = 0.f;
            m_bucket[i].m_effector[j].lean2Error = 0.f;
            m_bucket[i].m_effector[j].twistError = 0.f;
            m_bucket[i].m_effector[j].lean1VelError = 0.f;
            m_bucket[i].m_effector[j].lean2VelError = 0.f;
            m_bucket[i].m_effector[j].twistVelError = 0.f;
          }
          m_bucket[i].m_ik[0].error = 0.f;
          m_bucket[i].m_ik[1].error = 0.f;
        }
        if ((int)m_count >= m_numberOfFrames)
        {
          if (m_totalError < m_bestTotalError || m_bestTotalError == 0.f)
          {
            memcpy(m_bestSequence, m_sequence, sizeof(Sequence)*m_numberOfFrames);
            saveDriveSequence();
            m_bestTotalError = m_totalError;
          }
          bool doAverage = true;
          if (doAverage) // try an averaging step here
          {
            for (int i = 0; i<m_numberOfFrames; i++)
            {
              int min = (i+m_numberOfFrames-1)%m_numberOfFrames;
              int max = (i+1)%m_numberOfFrames;
              for (int j = 0; j<NUM_JOINTS; j++)
              {
                m_bestSequence[i].m_effector[j].lean1 = (m_sequence[min].m_effector[j].lean1 + 8.f*m_sequence[i].m_effector[j].lean1 + m_sequence[max].m_effector[j].lean1)/10.f;
                m_bestSequence[i].m_effector[j].lean2 = (m_sequence[min].m_effector[j].lean2 + 8.f*m_sequence[i].m_effector[j].lean2 + m_sequence[max].m_effector[j].lean2)/10.f;
                m_bestSequence[i].m_effector[j].twist = (m_sequence[min].m_effector[j].twist + 8.f*m_sequence[i].m_effector[j].twist + m_sequence[max].m_effector[j].twist)/10.f;
              }
              for (int j = 0; j<2; j++)
                m_bestSequence[i].m_ik[j].target = (m_sequence[min].m_ik[j].target + m_sequence[i].m_ik[j].target*8.f + m_sequence[max].m_ik[j].target)/10.f;

              // because of the change of coordinates we need to average the y parts carefully
              if (min == leftHandSwing[m_animIndex]-1)  
                min = leftHandSwing[m_animIndex];
              if (max == leftHandStance[m_animIndex])
                max = leftHandStance[m_animIndex]-1;
              if (min == leftHandStance[m_animIndex]-1)
                min = leftHandStance[m_animIndex];
              if (max == leftHandSwing[m_animIndex])
                max = leftHandSwing[m_animIndex]-1;
              for (int j = 0; j<2; j++) // Mod: not gUP safe
              { // not sure whether this is a valid correction, 
                // but it doesn't seem to make much difference in the resulting behaviour
                if(m_character->m_gUp.y > 0.9f)
                  m_bestSequence[i].m_ik[j].target.y = (m_sequence[min].m_ik[j].target.y + m_sequence[i].m_ik[j].target.y*8.f + m_sequence[max].m_ik[j].target.y)/10.f;
                else
                  m_bestSequence[i].m_ik[j].target.z = (m_sequence[min].m_ik[j].target.z + m_sequence[i].m_ik[j].target.z*8.f + m_sequence[max].m_ik[j].target.z)/10.f;
              }
            }
            memcpy(m_sequence, m_bestSequence, sizeof(Sequence)*m_numberOfFrames);
          }
        }
        m_count += 1.f;
        if (m_count > (float)m_numberOfFrames)
          m_count = (float)m_numberOfFrames;
        else
          m_totalFrame = 0;
      }
      NM_RS_DBG_LOGF(L"m_totalError: %f", m_totalError);
    }
  }
#endif //CRAWL_LEARNING
