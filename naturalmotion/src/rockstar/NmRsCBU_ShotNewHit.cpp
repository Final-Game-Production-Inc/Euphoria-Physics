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
#include "NmRsCBU_Shot.h"
#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsGenericPart.h"
#include "NmRsCBU_DynamicBalancer.h" 

namespace ART
{
     //----------------NEW HIT------------------------------------------------
    void NmRsCBUShot::newHit(NmRsHumanBody& body)
    {
#if NM_NEW_HIT_EXTENDS_BALANCE
      const float additionalBalanceTime = 1.f;
      const int   additionalSteps = 2;
      NmRsCBUShot* shotTask = (NmRsCBUShot *)m_character->getTask(bvid_shot);
      Assert(shotTask);
      if(shotTask->isActive() && m_newHit == false) // ensure this only happens once per tick.
      {
        NmRsCBUDynamicBalancer *balancerTask = (NmRsCBUDynamicBalancer*)m_character->getTask(bvid_dynamicBalancer);
        Assert(balancerTask);
        if (balancerTask->isActive())
        {
          balancerTask->decrementTime(additionalBalanceTime);
          balancerTask->decrementSteps(additionalSteps);
        }
      }
#endif
      if (m_falling)
        m_reachedForFallen = true;
      //reset reachForWound i.e. clear old hit/reach points
      //  (we could allow for an old target to stay around quite easily 
      //   except it is complicated by injured limb code)
      m_hitTimeRight = m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound+0.1f;
      m_hitTimeLeft = m_parameters.grabHoldTime + m_parameters.timeBeforeReachForWound+0.1f;
      
      //Remember that a leg is injured but this hit may not be to the leg
      bool injuredLLeg = false;
      bool injuredRLeg = false;
      //if(m_injuredLLeg)
      //  injuredLeftLeg_exit();
      //if(m_injuredRLeg)
      //  injuredRightLeg_exit();

      m_newHit = true;
      m_archBack = false;
      m_hitFromBehind = false;
      m_hitTime = 0.0f;

      //find local and global versions for hitPoint and normal
      rage::Vector3 hitNormalWorld;
      rage::Matrix34 mat;
      if (m_parameters.localHitPointInfo)
      {
        m_hitPointLocal = m_parameters.hitPoint;
        m_hitNormalLocal = m_parameters.normal;

        m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
        mat.Transform(m_hitPointLocal, m_hitPointWorld);
        mat.Transform3x3(m_hitNormalLocal, hitNormalWorld);
      }
      else
      {
        m_hitPointWorld = m_parameters.hitPoint;
        hitNormalWorld = m_parameters.normal;

        m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
        mat.UnTransform(m_hitPointWorld, m_hitPointLocal);
        NM_RS_DBG_LOGF(L"localised hit Pos");
        mat.UnTransform3x3(hitNormalWorld, m_hitNormalLocal);
      }
#define NM_CHECK_HITPOINTS 0
#define NM_FIX_BAD_HITPOINTS 0 && NM_CHECK_HITPOINTS 
#if ART_ENABLE_BSPY & NM_CHECK_HITPOINTS
      bool hitOK = true;
      //Check hitPoint is near surface of bound
      if(m_character->isPartInMask(bvmask_BodyExceptHandsAndFeet, m_parameters.bodyPart))
      {
        GPartDescriptorPacket pdp;
         // extract bound data
        const rage::phBound* bound = m_character->getGenericPartByIndex(m_parameters.bodyPart)->getBound();
        phBoundToShapePrimitive(bound, pdp.m_shape);
        //y=length x,z radius
        static float skinInside = 0.01f;
        static float skinOutside = 0.05f;
        float length = pdp.m_shape.m_data.capsule.m_length;
        float radius = pdp.m_shape.m_data.capsule.m_radius;
        float hitRadius = rage::Sqrtf(m_hitPointLocal.x*m_hitPointLocal.x + m_hitPointLocal.z*m_hitPointLocal.z);
        float hitLength = m_hitPointLocal.y;
        float outsideCylinder = rage::Abs(hitLength) - (length-radius)*0.5f; 
        if (outsideCylinder > 0.0f)
          hitRadius = rage::Sqrtf(m_hitPointLocal.x*m_hitPointLocal.x + outsideCylinder*outsideCylinder + m_hitPointLocal.z*m_hitPointLocal.z);
        hitOK = (hitRadius > (radius - skinInside)) && (hitRadius < (radius + skinOutside));
        float distanceFromSphyl = hitRadius - radius;
        distanceFromSphyl = hitRadius - radius;//repeated line as distanceFromSphyl gives compile error for XBox Bank Release
        Assertf(hitOK, "distanceFromSphyl is %.3f, radius is %.3f", distanceFromSphyl, radius);

      }
#endif
      // positive if shot was from behind
      m_velForwards = -m_character->m_COMTM.c.Dot(m_parameters.bulletVel);
      if (m_velForwards >= 0.f) // need to find exit wound to reach for...
      {
        NM_RS_DBG_LOGF(L"HIT FROM BEHIND");
        NmRsGenericPart *part = m_character->getGenericPartByIndex(m_parameters.bodyPart);
        if(part == getSpine()->getPelvisPart() ||
          part == getSpine()->getSpine0Part() ||
          part == getSpine()->getSpine1Part() ||
          part == getSpine()->getSpine2Part() ||
          part == getSpine()->getSpine3Part())
        {
          m_hitFromBehind = true;
        }

#if NM_FIND_EXITWOUND
        rage::Vector3 direction = m_parameters.bulletVel;
        direction.Normalize();

        static float SEGMENT_LENGTH = 0.4f;
        const rage::Vector3 startPos = m_hitPointWorld + (direction * SEGMENT_LENGTH);
        const rage::phSegment segment(startPos, m_hitPointWorld);
        rage::phIntersection intersection;

        rage::phShapeTest<rage::phShapeProbe> probeTester;
        probeTester.InitProbe(segment,&intersection,1);
        probeTester.SetLevel(PHLEVEL);//MP3Rage

        if (m_character->getFirstInstance() && (probeTester.TestOneObject((*m_character->getFirstInstance())) > 0))
        {
          Assertf(m_hitPointWorld.Dist(RCC_VECTOR3(intersection.GetPosition())) <= SEGMENT_LENGTH + 0.001f, "distance is %.3f", m_hitPointWorld.Dist(RCC_VECTOR3(intersection.GetPosition())));
          m_hitPointWorld = RCC_VECTOR3(intersection.GetPosition());
          hitNormalWorld = RCC_VECTOR3(intersection.GetNormal());

          Assertf(intersection.GetComponent() < m_character->getNumberOfParts(), "component is %d", (int)intersection.GetComponent());
          m_parameters.bodyPart = intersection.GetComponent();
          //mmmmtodo Warn that body Part has changed? 
        }

        m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
        mat.UnTransform(m_hitPointWorld, m_hitPointLocal);
        NM_RS_DBG_LOGF(L"localised hit Pos");
        mat.UnTransform3x3(hitNormalWorld, m_hitNormalLocal);
#endif //#if NM_FIND_EXITWOUND
      }

#if NM_FIX_BAD_HITPOINTS
      if (!hitOK)
      {
        rage::Vector3 direction = m_parameters.bulletVel;
        direction.Normalize();

        const rage::Vector3 startPos = m_hitPointWorld - (direction);
        const rage::phSegment segment(startPos, m_hitPointWorld);
        rage::phIntersection intersection;

        rage::phShapeTest<rage::phShapeProbe> probeTester;
        probeTester.InitProbe(segment,&intersection,1);
        probeTester.SetLevel(PHLEVEL);//MP3Rage

        if (m_character->getFirstInstance() && (probeTester.TestOneObject((*m_character->getFirstInstance())) > 0))
        {
          m_hitPointWorld = RCC_VECTOR3(intersection.GetPosition());
          hitNormalWorld = RCC_VECTOR3(intersection.GetNormal());
          m_parameters.bodyPart = intersection.GetComponent();
        }

        m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
        mat.UnTransform(m_hitPointWorld, m_hitPointLocal);
        NM_RS_DBG_LOGF(L"localised hit Pos");
        mat.UnTransform3x3(hitNormalWorld, m_hitNormalLocal);
      }
#endif //#if NM_FIX_BAD_HITPOINTS

      if(m_parameters.bust && m_character->isPartInMask(bvmask_Spine2 | bvmask_Spine3, m_parameters.bodyPart))
      {
        if (m_parameters.cupBust)
          //perhaps we should clamp m_hitPointLocal to spine1 sphyl as it is smaller than spine2/3
          //However the hands would go through the clothed bust slope if we did 
          m_parameters.bodyPart = getSpine()->getSpine1Part()->getPartIndex();
        else if (m_character->isPartInMask(bvmask_Spine2, m_parameters.bodyPart))
          m_hitPointLocal.z -= m_parameters.cupSize;//0.1
        m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
        mat.Transform(m_hitPointLocal, m_hitPointWorld);
        mat.Transform3x3(m_hitNormalLocal, hitNormalWorld);
      }

      // todo use masks to make this faster.
      NmRsGenericPart *base = getSpine()->getPelvisPart();
      NmRsGenericPart *pPart = m_character->getGenericPartByIndex(m_parameters.bodyPart);
      if (pPart == getSpine()->getPelvisPart() ||
          pPart == getSpine()->getSpine0Part() ||
          pPart == getSpine()->getSpine1Part() ||
          pPart == getSpine()->getSpine2Part() ||
          pPart == getSpine()->getSpine3Part() ||
          pPart == getSpine()->getNeckPart() ||
          pPart == getSpine()->getHeadPart())
      {
        base = pPart;
      }
      if (pPart == getLeftArm()->getClaviclePart() ||
        pPart == getRightArm()->getClaviclePart() ||
        pPart == getLeftArm()->getUpperArm() ||
        pPart == getRightArm()->getUpperArm() ||
        pPart == getLeftArm()->getLowerArm() ||
        pPart == getRightArm()->getLowerArm())
      {
        base = getSpine()->getSpine3Part();
      }
      if (pPart == getLeftLeg()->getThigh() ||
        pPart == getRightLeg()->getThigh() ||
        pPart == getLeftLeg()->getShin() ||
        pPart == getRightLeg()->getShin() ||
        pPart == getLeftLeg()->getFoot() ||
        pPart == getRightLeg()->getFoot())
      {
        base = getSpine()->getPelvisPart();
      }
      rage::Matrix34 partMatrix;
      base->getMatrix(partMatrix);

      //Find the hitPoint distance from the centre and place in m_hitPointRight
      //This stops the same side arm from reaching
      //This gives the cPain twist direction/Mag
      //Should be done by not reaching for out of distance targets instead
      if (pPart == getSpine()->getPelvisPart() ||
        pPart == getSpine()->getSpine0Part() ||
        pPart == getSpine()->getSpine1Part() ||
        pPart == getSpine()->getSpine2Part() ||
        pPart == getSpine()->getSpine3Part())
      {
        m_hitPointRight = -m_hitPointLocal.y;
      }
      if ( pPart == getSpine()->getNeckPart() ||
        pPart == getSpine()->getHeadPart())
      {
        m_hitPointRight = m_hitPointLocal.x;
      }
      if (pPart == getLeftArm()->getClaviclePart() ||
          pPart == getRightArm()->getClaviclePart())
      {
        m_hitPointRight = (m_hitPointWorld-base->getPosition()).Dot(partMatrix.b);
      }
      if (pPart == getLeftArm()->getUpperArm() ||
          pPart == getLeftArm()->getLowerArm() ||
          pPart == getLeftArm()->getHand())
      {
        m_hitPointRight = -(getLeftArm()->getShoulder()->getJointPosition()-base->getPosition()).Dot(partMatrix.b);
      }
      if (pPart == getRightArm()->getUpperArm() ||
          pPart == getRightArm()->getLowerArm() ||
          pPart == getRightArm()->getHand())
      {
        m_hitPointRight = -(getRightArm()->getShoulder()->getJointPosition()-base->getPosition()).Dot(partMatrix.b);
      }
      if (pPart == getLeftLeg()->getThigh() ||
          pPart == getLeftLeg()->getShin() ||
          pPart == getLeftLeg()->getFoot())
      {
        m_hitPointRight = -(getLeftLeg()->getHip()->getJointPosition()-base->getPosition()).Dot(partMatrix.b);
      }
      if (pPart == getRightLeg()->getThigh() ||
          pPart == getRightLeg()->getShin() ||
          pPart == getRightLeg()->getFoot())
      {
        m_hitPointRight = -(getRightLeg()->getHip()->getJointPosition()-base->getPosition()).Dot(partMatrix.b);
      }
      //find the moment of the bullet around the base part (for m_exagTwist)
      rage::Vector3  bulletVel = m_parameters.bulletVel;
      m_character->levelVector(bulletVel, 0.f);
      bulletVel.Normalize();
      rage::Vector3 bodyBackLevelled  = m_character->m_COMTM.c;
      rage::Vector3 bodyRightLevelled = m_character->m_COMTM.a;
      m_character->levelVector(bodyBackLevelled, 0.f);
      m_character->levelVector(bodyRightLevelled, 0.f);
      bodyBackLevelled.Normalize();
      bodyRightLevelled.Normalize();
      rage::Vector3 twistAxis = partMatrix.a;//up for pelvis and spine
      rage::Vector3 moment;
      moment.Zero();
      if (m_parameters.bulletVel.Mag() > NM_RS_FLOATEPS)
      {
        moment = m_hitPointWorld-base->getPosition();
        moment.Cross(m_parameters.bulletVel);
        moment /= m_parameters.bulletVel.Mag();
      }
      //exaggerate
      //lean1  >= spine2 go with bullet, <spine 2 against bullet 
      //lean2 exaggerate >= spine2 go with bullet, <spine 2 against bullet 
      //twist exaggerate go with bullet
      //have a no spine exaggerate for eg feet?
      //go with bullet
      m_exagLean1 = -bodyBackLevelled.Dot(bulletVel);
      m_exagLean2 = -bodyRightLevelled.Dot(bulletVel);
      m_exagTwist = -10.f*-twistAxis.Dot(moment);

      //go against bullet
      if (pPart == getSpine()->getPelvisPart() ||
        pPart == getSpine()->getSpine0Part() ||
        pPart == getSpine()->getSpine1Part() ||
        pPart == getLeftLeg()->getThigh() ||
        pPart == getLeftLeg()->getShin() ||
        pPart == getLeftLeg()->getFoot() ||
        pPart == getRightLeg()->getThigh() ||
        pPart == getRightLeg()->getShin() ||
        pPart == getRightLeg()->getFoot())
      {
        m_exagLean1 *= -1.f;
        m_exagLean2 *= -1.f;
      }

#if NM_RS_VALIDATE_VITAL_VALUES
      {
        //Assert if hitPoint too far from bodyPart centre and put hitpoint on bodyPart centre 
        //mmmmtodo put on bound surface like above code does for above and m_hitPointRight

#if HACK_GTA4
#if __ASSERT
        // Use a max length of 2.1 to match what is tested in GTA game code
        static float fMaxLength = 2.1f;//((rage::phBoundCapsule*)(m_character->getGenericPartByIndex(gtaThigh_Left)->getBound()))->GetLength();
        Assertf(m_hitPointLocal.Mag() < fMaxLength,
          "NaturalMotion: m_hitPointLocal (pos local = %i) too far from part %i centre = %.3f (max = %.3f), %.3f, %.3f, %.3f ",
          (int) m_parameters.localHitPointInfo, m_parameters.bodyPart, m_hitPointLocal.Mag(), fMaxLength, m_hitPointLocal.x, m_hitPointLocal.y, m_hitPointLocal.z);
#endif //__ASSERT
#else //HACK_GTA4
        Assertf(m_hitPointLocal.Mag() < 0.7f,
          "NaturalMotion: m_hitPointLocal (pos local = %i) too far from part %i centre = %.3f (max = 0.7), %.3f, %.3f, %.3f ",
          (int) m_parameters.localHitPointInfo, m_parameters.bodyPart, m_hitPointLocal.Mag(), m_hitPointLocal.x, m_hitPointLocal.y, m_hitPointLocal.z);
#endif //HACK_GTA4

        Assert(m_hitPointRight == m_hitPointRight);
        Assertf(m_hitPointRight <= 2.0
          , "NaturalMotion: m_hitPointRight = %.3f (max = 2.0f), %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %d"
          , m_hitPointRight, m_character->m_COMTM.a.x, m_character->m_COMTM.a.y, m_character->m_COMTM.a.z
          , m_hitPointWorld.x, m_hitPointWorld.y, m_hitPointWorld.z, base->getPosition().x, base->getPosition().y, base->getPosition().z, m_parameters.bodyPart);
        Assertf(m_hitPointRight >= -2.0f
          , "NaturalMotion: m_hitPointRight = %.3f (min = -2.0f), %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %d"
          , m_hitPointRight, m_character->m_COMTM.a.x, m_character->m_COMTM.a.y, m_character->m_COMTM.a.z
          , m_hitPointWorld.x, m_hitPointWorld.y, m_hitPointWorld.z, base->getPosition().x, base->getPosition().y, base->getPosition().z, m_parameters.bodyPart);
        // now that we've complained about it, default the hit position to keep stupid things from happening
        // note: this will not update the normal info and all that, but will keep nastiness from occuring
        //mmmmtodo redo this? and assert if hitpoint too far from part centre
        if(m_hitPointRight >= 2.f || m_hitPointRight <= -2.f )
        {
          NmRsGenericPart *part = m_character->getGenericPartByIndex(m_parameters.bodyPart);
          m_hitPointWorld = part->getPosition();
          m_hitPointRight = m_character->m_COMTM.a.Dot(m_hitPointWorld - base->getPosition());
          rage::Matrix34 mat;
          m_character->getGenericPartByIndex(m_parameters.bodyPart)->getMatrix(mat);
          mat.UnTransform(m_hitPointWorld, m_hitPointLocal);
          NM_RS_DBG_LOGF(L"localised hit Pos");
          mat.UnTransform3x3(hitNormalWorld, m_hitNormalLocal);
        }
      }
#endif

      m_hitPointRight = rage::Clamp(m_hitPointRight, -2.0f, 2.0f);
      NM_RS_DBG_LOGF(L"NEWHIT choose reaction entry: %.3f", m_newHit);

      // choose head look reaction: velForwards is < 0, when shot from front
      // assumes that look target (reachTarget) is on front of body which it should be (i.e. exit wound if shot from behind)
      // remember exit wound is determined by game now so is not guaranteed (for RDR and MP3) 
      if (m_parameters.reachForWound && /*m_velForwards < 0.f &&*/ pPart != getSpine()->getHeadPart() && pPart != getSpine()->getNeckPart())
      {
        m_headLookAtWound = true;
        m_feedbackSent_FinishedLookingAtWound = false;
        // reset the headLook toggle-timer to ensure that for a 2nd (3rd..) hit, character starts looking at wound again
        m_headLook.toggleTimer = m_parameters.headLookAtWoundMaxTimer;
      }
      else
      {
        m_headLookAtWound = false;
        m_feedbackSent_FinishedLookingAtWound = true;
      }

      //Check whether to do injured leg reaction
      //NB: Doesn't restart the legInury if already injured
      if (pPart == getLeftLeg()->getThigh() || pPart == getLeftLeg()->getShin() || pPart == getLeftLeg()->getFoot())
        injuredLLeg = true;
      if (pPart == getRightLeg()->getThigh() || pPart == getRightLeg()->getShin() || pPart == getRightLeg()->getFoot())
        injuredRLeg = true;

      // choose reach
      // todo use masks to make this faster.
      NM_RS_DBG_LOGF(L"hitPointRight: %.3f", m_hitPointRight, L"newHit: %.3f", m_newHit);
      bool hitLArm = pPart == getLeftArm()->getClaviclePart() ||
        pPart == getLeftArm()->getUpperArm() ||
        pPart == getLeftArm()->getLowerArm() ||
        pPart == getLeftArm()->getHand();

      bool hitRArm = pPart == getRightArm()->getClaviclePart() ||
        pPart == getRightArm()->getUpperArm() ||
        pPart == getRightArm()->getLowerArm() ||
        pPart == getRightArm()->getHand();

      //don't start a new injured arm if one running.  NB. The uninjured arm can change its focus
      if (m_parameters.allowInjuredArm && !m_injuredLArm && !m_injuredRArm && hitLArm && pPart != getLeftArm()->getClaviclePart() && m_woundLPart == -1)
      {
        m_injuredLArm = true;
        NM_RS_DBG_LOGF(L"injured left arm");
      }
      if (m_parameters.allowInjuredArm && !m_injuredLArm && !m_injuredRArm && hitRArm && pPart != getRightArm()->getClaviclePart() && m_woundRPart == -1)
      {
        m_injuredRArm = true;
        NM_RS_DBG_LOGF(L"injured right arm");
      }

      // which arm to use for reach ?
      float side = 0.06f;
      bool useLeft = (m_hitPointRight > -side && !hitLArm) || hitRArm;
      bool useRight = (m_hitPointRight <= side && !hitRArm) || hitLArm;
      //for left/right pelvis hits reach with left/right hand as other hand is out of reach.
      if (pPart == getSpine()->getPelvisPart() || pPart == getSpine()->getSpine0Part())
      {
        useRight = m_hitPointRight > -side;
        useLeft = m_hitPointRight <= side;
      }

      //reach only with the arm on the same side as the injured leg
      if(injuredLLeg || injuredRLeg)
      {
        if (injuredLLeg)
        {
          useLeft = true;
          useRight = false;
        }
        if (injuredRLeg)
        {
          useRight = true;
          useLeft = false;
        }
      }

      //mmmmtodo don't we just: 
      //  favour the other hand if were holding a pistol
      //  favour the left hand if both holding a pistol and allowLeftPistolRFW and allowRightPistolRFW
      //  choose the hand without a pistol if we ending reaching with both
      //mmmmtodo should take into account the decisions made above
      //because the weaponMode can change any time it would be better to put this logic in rfw itself
      //Use the hand without a weapon for reaching for wound (given it's not the wounded arm).
      // If it is a two handed weapon or both hands have a weapon then use the left if m_parameters.allowLeftPistolRFW  
      // Overwrites above decision based on body-side of the hit point
      // Overwrites above decision based on leg hits
      //mmmtodo should be based on m_character->getCharacterConfiguration().m_leftHandState == CharacterConfiguration::eHS_Free to be consistent
      switch(m_character->getWeaponMode()) 
      {
      case kRifle:
      case kDual:
        useLeft = /*(m_parameters.allowLeftPistolRFW) &&*/ !hitLArm;
        useRight = false;
        break;
      case kPistol:
      case kPistolRight:
        useLeft = !hitLArm;
        useRight = false;
        break;
      case kPistolLeft:
        useLeft = false;
        useRight = !hitRArm;
      default:
        break;
      }

      // disable reaching for injured leg if so desired
      if((injuredLLeg || injuredRLeg) 
        && (
        (!m_parameters.allowInjuredLowerLegReach && 
        (pPart == getLeftLeg()->getShin() ||
        pPart == getRightLeg()->getShin() ||
        pPart == getLeftLeg()->getFoot() ||
        pPart == getRightLeg()->getFoot()
        ) 
        )
        || 
        (!m_parameters.allowInjuredThighReach && 
        (pPart == getLeftLeg()->getThigh() ||
        pPart == getRightLeg()->getThigh()
        )
        )
        )
        )
      {
        useLeft = false;
        useRight = false;
      }

      // disallow reaches to points too close to shoulder.
#if 0
      rage::Vector3 shoulderToHitPoint;
      //const float minimumHitDistance = 0.0225f; // sqrt 15cm
      if(useLeft)
      {
        shoulderToHitPoint.Set(m_leftArm->getShoulder()->getJointPosition() - m_hitPointWorld);
        if(shoulderToHitPoint.Mag2() < minimumHitDistance)
          useLeft = false;
      }
      if(useRight)
      {  
        shoulderToHitPoint.Set(m_rightArm->getShoulder()->getJointPosition() - m_hitPointWorld);
        if(shoulderToHitPoint.Mag2() < minimumHitDistance) 
          useRight = false;  
      }
#endif 

      // force one-handed
      if (useLeft && useRight && m_parameters.reachWithOneHand > 0)
      {
        if (m_parameters.reachWithOneHand == 2)//mmmmtodo does this sit well with a generally left available pointGun hand?
          useLeft = false;
        else
          useRight = false;
      }

      //mmmmtodo don't allow a gunHand to run a reach for the injured arm reaction?
      //Done - Don't allow the hand to reachForWound if it's holding a rifle
      //mmmmtodo m_newReachL = true on exit for restart after bcrArms messes up rifle exclusion here?
      //  therefore shotNew hit should choose the best arm but inhibit it in entry/exit conditions?
      bool injuredArm = false;//offset targets to get asymmetry if both hands are reaching for wound and it's not an arm wound
      if ((m_character->getCharacterConfiguration().m_leftHandState != CharacterConfiguration::eHS_Rifle) &&
        (useLeft || (m_injuredLArm && m_parameters.allowInjuredArm)))
      {
        if (m_injuredLArm && m_parameters.allowInjuredArm)
        {
          m_woundLPart = getLeftArm()->getClaviclePart()->getPartIndex();
          m_woundLOffset.Set(0.f, 0.1f, 0.15f);//also (upwards,inwards,forwards)(0.f, 0.f, 0.2f)
          m_woundLNormal.Set(0.f, 0.f, 1.f);
          m_hitTimeLeft = 0.0f;
          m_newReachL = true;
          injuredArm = true;
        }
        else if (!m_injuredLArm)
        {
          m_woundLPart = m_parameters.bodyPart;
          m_woundLOffset = m_hitPointLocal;
          m_woundLNormal = m_hitNormalLocal;
          m_hitTimeLeft = 0.0f;
          m_newReachL = true;
        }
      }
      if ((m_character->getCharacterConfiguration().m_rightHandState != CharacterConfiguration::eHS_Rifle) &&
        (useRight || (m_injuredRArm && m_parameters.allowInjuredArm)))
      {
        if (m_injuredRArm && m_parameters.allowInjuredArm)
        {

          m_woundRPart = getRightArm()->getClaviclePart()->getPartIndex();
          m_woundROffset.Set(0.f, 0.1f, -0.15f);//also (upwards,inwards,backwards)(0.f, 0.f, -0.2f)
          m_woundRNormal.Set(0.f, 0.f, -1.f);
          m_hitTimeRight = -0.1f;
          m_newReachR = true;
          injuredArm = true;
        }
        else if (!m_injuredRArm)
        {
          m_woundRPart = m_parameters.bodyPart;
          m_woundROffset = m_hitPointLocal;
          m_woundRNormal = m_hitNormalLocal;
          m_hitTimeRight = -0.1f;
          m_newReachR = true;
        }
      }

      bool offsetTargets = (!injuredArm) && useLeft && useRight;//offset targets to get asymmetry if both hands are reaching for wound and it's not an arm wound
      /*
      * offset grab targets for left and right (vertically) to get some asymmetry
      */
      if (offsetTargets)
      {
        // offset in part space
        // local 'up' is different for a few parts
        float offsetValue = 0.05f;
        rage::Vector3 localOffset(0.0f, 0.0f, 0.0f);
        if(m_parameters.bodyPart == getSpine()->getPelvisPart()->getPartIndex() || 
          m_parameters.bodyPart == getSpine()->getSpine0Part()->getPartIndex() ||
          m_parameters.bodyPart == getSpine()->getSpine1Part()->getPartIndex() ||
          m_parameters.bodyPart == getSpine()->getSpine2Part()->getPartIndex() ||
          m_parameters.bodyPart == getSpine()->getSpine3Part()->getPartIndex() )
        {
          localOffset.Set(1.0f, 0.0f, 0.0f);
          if(m_hitPointLocal.y < 0.f)                       // invert offset if reaching to right side of body center
            offsetValue = -0.05f;
        }
        else if(m_parameters.bodyPart == getSpine()->getNeckPart()->getPartIndex() ||
          /*m_parameters.bodyPart == m_spine->getHeadPart()->getPartIndex() ||*/
          m_parameters.bodyPart == getLeftLeg()->getThigh()->getPartIndex() || 
          m_parameters.bodyPart == getRightLeg()->getThigh()->getPartIndex() )
        {
          localOffset.Set(0.0f, 1.0f, 0.0f);
          if(m_hitPointLocal.x > 0.f)                       // invert offset if reaching to right side of body center
            offsetValue = -0.05f;
        }
        if (m_parameters.bust && m_parameters.bodyPart == getSpine()->getSpine1Part()->getPartIndex())
        {
          //no offset if on same side of body as arm, otherwise offset down (to keep out of bust area)
          if(m_hitPointLocal.x > 0.f)//right side and offsetValue is -ve
            m_woundLOffset += offsetValue*localOffset;//down
          else//left side and offsetValue is +ve
            m_woundROffset -= offsetValue*localOffset;//down
        }
        else if (m_parameters.bust && m_parameters.bodyPart == getSpine()->getSpine3Part()->getPartIndex())
        {
          //no offset if on same side of body as arm, otherwise offset up (to keep out of breast area)
          if(m_hitPointLocal.x > 0.f)//right side and offsetValue is -ve
            m_woundLOffset -= offsetValue*localOffset;//up
          else//left side and offsetValue is +ve
            m_woundROffset += offsetValue*localOffset;//up
        }
        else
        {
          //offset up if on opposite side of body to arm, otherwise offset down
          m_woundROffset += offsetValue*localOffset;
          m_woundLOffset -= offsetValue*localOffset;
        }

      }//if offsetTargets

      m_injuredLLeg = m_injuredLLeg || injuredLLeg;
      m_injuredRLeg = m_injuredRLeg || injuredRLeg;

      // shockSpin: adds an extra 'shock' of torque/lift to the spine to exaggerate impacts 
      //mmmmmtodo
      //Move functionality into bullet
      //Lift should be in up direction
      //  Should be scaled with character weight: liftForce *= m_setup.liftGain*m_character->getTotalMass()*9.81f
      //------------------------------------------------------------------------------------------------------
      if (m_parameters.addShockSpin)
      {
        int part = m_parameters.bodyPart;
        // only react if shot in trunk, or if always-on is set
        bool doShockSpin = m_parameters.alwaysAddShockSpin ||
          (part == getSpine()->getSpine0Part()->getPartIndex() ||
          part == getSpine()->getSpine1Part()->getPartIndex() ||
          part == getSpine()->getSpine2Part()->getPartIndex() ||
          part == getSpine()->getSpine3Part()->getPartIndex() ||
          part == getLeftArm()->getClaviclePart()->getPartIndex() ||
          part == getRightArm()->getClaviclePart()->getPartIndex());

        if (doShockSpin)
        {
          // only regenerate values upon a new hit if the last one has finished being applied
          // stops stacking torques and having the character spun like a top
          if (m_torqueSpinTime < 0.0f)
          {
            if (m_parameters.randomizeShockSpinDirection)
            {
              m_torqueSpin = m_character->getRandom().GetRanged(m_parameters.shockSpinMin, m_parameters.shockSpinMax);
              if (m_character->getRandom().GetBool())
                m_torqueSpin = -m_torqueSpin;
            }
            else
            {
              // work out a multiplier based on where the bullet impact hits the body
              rage::Vector3 toHitPoint = m_hitPointWorld - m_character->m_COM;
              m_character->levelVector(toHitPoint);

              if (!m_parameters.shockSpinScaleByLeverArm)
                toHitPoint.Normalize();

              // if we are in the melee and hit the head and are doing spin stuff ( the baseball bat) then we want spin. 
              if ((m_parameters.bodyPart == getSpine()->getHeadPart()->getPartIndex())&&(m_parameters.melee))
              {
                toHitPoint.Normalize();
              }

              rage::Vector3 upVec;
              upVec.Normalize(m_parameters.bulletVel);
              upVec.Cross(toHitPoint);

              m_torqueSpin = upVec.Dot(m_character->m_gUp) * 
                -m_character->getRandom().GetRanged(m_parameters.shockSpinMin, m_parameters.shockSpinMax);

              if (m_parameters.bracedSideSpinMult != 1.0f)
              {
                // If balanced and shot on a side with a forward foot, increase the shockspin to compensate for the balancer 
                // naturally resisting spin to that side
                NmRsCBUDynamicBalancer* dynamicBalancerTask = (NmRsCBUDynamicBalancer*)m_character->getTask(bvid_dynamicBalancer);  
                if (dynamicBalancerTask && dynamicBalancerTask->m_failType == dynamicBalancerTask->balOK)
                {
                  rage::Matrix34 tmCom(m_character->m_COMTM);
                  bulletVel.NormalizeSafe();
                  rage::Vector3 toShooter = -bulletVel;
                  rage::Vector3 bodyRight;
                  bodyRight.Cross(toShooter, rage::Vector3(0.0f,0.0f,1.0f));
                  bodyRight.NormalizeSafe();
                  rage::Vector3 toFoot = getLeftLeg()->getFoot()->getPosition() - tmCom.d;
                  float dotLeft = toFoot.Dot(toShooter);
                  toFoot = getRightLeg()->getFoot()->getPosition() - tmCom.d;
                  float dotRight = toFoot.Dot(toShooter);
                  float dotSide = bodyRight.Dot(toHitPoint);
                  static float dotSideMin = 0.1f;
                  if((dotRight > dotLeft && dotSide > dotSideMin) || (dotLeft > dotRight && dotSide < -dotSideMin))
                  {
                    static float bracedScalePerComponentMult = 1.0f;
                    m_torqueSpin *= m_parameters.bracedSideSpinMult;
                    m_parameters.shockSpinScalePerComponent = rage::Min(1.0f, m_parameters.shockSpinScalePerComponent+bracedScalePerComponentMult*rage::Abs(dotSide));
                  }
                }
              }

              //limit the torque being applied to sensible values just in case the lever arm calculated above is massive
              m_torqueSpin = rage::Clamp(m_torqueSpin, -0.5f*m_parameters.shockSpinMax,0.5f*m_parameters.shockSpinMax);
            }

            m_torqueSpinTime = 1.0f;
          }//if (m_torqueSpinTime < 0.0f)
        }//if (doShockSpin)
      }//if (m_parameters.addShockSpin)

      if (m_parameters.snap)
      {
        rage::Vector3* snapDirection = NULL;
        if ( m_parameters.snapUseBulletDir)
          snapDirection = &m_parameters.bulletVel;
        int bodyPart = -1;//Don't apply snap just to bodyPart
        if ( m_parameters.snapHitPart)
          bodyPart = m_parameters.bodyPart;

        if (m_parameters.unSnapRatio>=0.f)
        {
          if (m_snapDirection > 0.f)
          {
            m_snapDirection = 1.f;
            m_character->snap(m_parameters.snapMag*m_snapDirection,
              m_parameters.snapDirectionRandomness, 
              m_parameters.snapHipType,
              m_parameters.snapLeftArm,
              m_parameters.snapRightArm,
              m_parameters.snapLeftLeg,  
              m_parameters.snapRightLeg,  
              m_parameters.snapSpine,  
              m_parameters.snapNeck, 
              m_parameters.snapPhasedLegs, 
              m_parameters.snapUseTorques,
              1.f,
              bodyPart,
              snapDirection,
              m_parameters.snapMovingMult,
              m_parameters.snapBalancingMult,
              m_parameters.snapAirborneMult,
              m_parameters.snapMovingThresh);


          }
        }//if (m_parameters.unSnapRatio>=0.f)
        else
          m_character->snap(m_parameters.snapMag*m_snapDirection,
          m_parameters.snapDirectionRandomness, 
          m_parameters.snapHipType,
          m_parameters.snapLeftArm,
          m_parameters.snapRightArm,
          m_parameters.snapLeftLeg,  
          m_parameters.snapRightLeg,  
          m_parameters.snapSpine,  
          m_parameters.snapNeck, 
          m_parameters.snapPhasedLegs, 
          m_parameters.snapUseTorques,
          1.f,
          bodyPart,
          snapDirection,
          m_parameters.snapMovingMult,
          m_parameters.snapBalancingMult,
          m_parameters.snapAirborneMult,
          m_parameters.snapMovingThresh);

        m_snapDirection *= -1.f;
      }//if (m_parameters.snap)

      // If using stayUpright's lastStandMode, update the control positions
      //if (m_character->m_uprightPelvisPosition.Mag2() > 0.001f)
      //{
      //	// Update the linear positions
      //	static float deltaDist = 0.1f;
      //	rage::Vector3 deltaVec = -hitNormalWorld;
      //	deltaVec.z = 0.0f;
      //	deltaVec.NormalizeSafe();
      //	deltaVec.Scale(deltaDist);
      //	m_character->m_uprightPelvisPosition += deltaVec;
      //	m_character->m_uprightSpine2Position += deltaVec;
      //	m_character->m_uprightSpine3Position += deltaVec;

      //	// Update the orientation
      //	
      //}

      //Apply looseness to character before physics step 
      //which can be after the messages are received (shotNewHit now calls this routine)
      //but before the behaviour tick
      if (m_controlStiffnessStrengthScale >= 1.f || m_parameters.alwaysResetLooseness)
      {
        m_controlStiffnessStrengthScale = 0.01f;
        m_controlStiffnessTime = 0.0f;
      }
      if (m_controlNeckStiffnessScale >= 1.f || m_parameters.alwaysResetNeckLooseness)
      {
        m_controlNeckStiffnessScale = 0.01f;
        m_controlNeckStiffnessTime = 0.0f;
      }

      controlStiffness_tick(m_character->getLastKnownUpdateStep(), body);
    }

}

