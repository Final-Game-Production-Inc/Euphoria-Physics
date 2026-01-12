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
 *
 *
 * Curls into foetal position, at a speed defined by the strength and damping values;
 * This behaviour is full-body and resets the character when it starts.
 *
 */


#include "NmRsInclude.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsCBU_CrushReaction.h"

namespace ART
{

    bool NmRsCBUCrushReaction::checkJointsObstacleCollision(NmRsEffectorBase* joints[], int numJoints, int obsID)
    {
      // need this until API exists for getting just zmpInst
      rage::Vector3 zmpPos, zmpNormal;
      float zmpDepth = 0;
      rage::phInst* zmpInst1 = NULL;

      bool obsCol = false;
      for (int i = 0; i < numJoints; i++)
      {
        NmRsGenericPart* p = m_character->getGenericPartByIndex(joints[i]->getChildIndex());
        p->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth, &zmpInst1);
        obsCol |= (zmpInst1 && (zmpInst1->GetLevelIndex() == obsID));
        if(obsCol)
        {
          break; //early termination if only per limb info is needed
        }
      }    
      return obsCol;
    }

    bool NmRsCBUCrushReaction::checkLimbObstacleCollision(NmRsGenericPart* limbParts[], int numParts, int obsID)
    {
      // need this until API exists for getting just zmpInst
      rage::Vector3 zmpPos, zmpNormal;
      float zmpDepth = 0;
      rage::phInst* zmpInst1 = NULL;

      bool obsCol = false;
      for (int i = 0; i < numParts ; i++)
      {
        limbParts[i]->getCollisionZMPWithEnvironment(zmpPos, zmpNormal, &zmpDepth, &zmpInst1);
        obsCol |= (zmpInst1 && (zmpInst1->GetLevelIndex() == obsID));
        if(obsCol)
        {
          break; //early termination if only per limb info is needed
        }
      }    
      return obsCol;
    }

    // takes into account injuries applied by the game and doesn't set it to be less injured (to avoid "instant healing")
    float NmRsCBUCrushReaction::applyInjurySafely(NmRsEffectorBase* effector, float newInjury)
    {
      //Assert(newInjury>=0.0f && newInjury <= 1.0f);
      newInjury = rage::Clamp(newInjury, 0.0f, 1.0f);

      float currentInjury = effector->getInjuryAmount();
      newInjury = rage::Max(newInjury, currentInjury);
      effector->setInjured(newInjury);
      return newInjury;
    }

    void NmRsCBUCrushReaction::checkAndApplyInjuryByLimb()
    {
      // limbs todo decide how we want to deal with injuries (not currently used in any other tasks).
      // this behaviour is currently unused.
#if 0
      //NmRsEffectorBase* leftArmJoints [4] = {m_leftArm->getClavicle(), m_leftArm->getShoulder(), m_leftArm->getElbow(), m_leftArm->getWrist()};

      // define limbs by part
      NmRsGenericPart* leftArm [3] = {getLeftArm()->getLowerArm(), getLeftArm()->getUpperArm(), getLeftArm()->getClaviclePart()};
      NmRsGenericPart* rightArm [3] = {getRightArm()->getLowerArm(), getRightArm()->getUpperArm(), getRightArm()->getClaviclePart()};
      NmRsGenericPart* leftLeg [2] = {getLeftLeg()->getShin(), getLeftLeg()->getThigh()};
      NmRsGenericPart* rightLeg [2] = {getRightLeg()->getShin(), getRightLeg()->getThigh()};
      //NmRsGenericPart* torso [4] = {m_spine->getSpine0Part(), m_spine->getSpine1Part(), m_spine->getSpine2Part(), m_spine->getSpine3Part()};
      //NmRsGenericPart* head [2] = {m_spine->getHeadPart(), m_spine->getNeckPart()};


      bool laObsCol = checkLimbObstacleCollision(leftArm, 3, m_parameters.m_obstacleID);
      bool raObsCol = checkLimbObstacleCollision(rightArm, 3, m_parameters.m_obstacleID);
      bool llObsCol = checkLimbObstacleCollision(leftLeg, 2, m_parameters.m_obstacleID);
      bool rlObsCol = checkLimbObstacleCollision(rightLeg, 2, m_parameters.m_obstacleID);
      //bool tObsCol = checkLimbObstacleCollision(torso, 4, m_parameters.m_obstacleID);
      //bool hObsCol = checkLimbObstacleCollision(head, 2, m_parameters.m_obstacleID);   
     // NM_RS_DBG_LOGF(L"Crush: ObsCol %d %d %d %d %d %d", laObsCol, raObsCol, llObsCol, rlObsCol, tObsCol, hObsCol);    


      if(m_parameters.m_useInjuries)
      {
        float injAmount = 0.9f;
        if(laObsCol)
        {
          m_leftArm->getClavicle()->setInjured(injAmount);
          m_leftArm->getElbow()->setInjured(injAmount);
          m_leftArm->getWrist()->setInjured(injAmount);
        }
        if(raObsCol)
        {
          m_rightArm->getClavicle()->setInjured(injAmount);
          m_rightArm->getElbow()->setInjured(injAmount);
          m_rightArm->getWrist()->setInjured(injAmount);
        }
        if(llObsCol)
        {
          m_leftLeg->getHip()->setInjured(injAmount);
          m_leftLeg->getKnee()->setInjured(injAmount);
          m_leftLeg->getAnkle()->setInjured(injAmount);
        }
        if(rlObsCol)
        {
          m_rightLeg->getHip()->setInjured(injAmount);
          m_rightLeg->getKnee()->setInjured(injAmount);
          m_rightLeg->getAnkle()->setInjured(injAmount);
        }
      }
#endif
    }

} // namespace ART