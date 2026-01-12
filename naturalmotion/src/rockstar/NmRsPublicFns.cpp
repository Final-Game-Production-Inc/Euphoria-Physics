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

// NOTE: this file contains implementations for the 'public' ARTRockstar.h header
// containing functions used directly by RAGE for interfacing with our engine

#include "fragment/cache.h"

#include "NmRsInclude.h"
#include "NmRsEngine.h"
#include "NmRsCharacter.h"
#include "NmRsGenericPart.h"
#include "NmRsCBU_DynamicBalancer.h"
#if ART_ENABLE_BSPY
#include "art\messageparams.h"//so that public functions can send a message to bSpy
#endif
#include "fragment/cache.h"
#include "system/memops.h"
using namespace rage;

namespace ART
{
  NmRsEngine*   gRockstarARTInstance = 0;
  rage::phInst* cachedPhInstPointer = NULL;

  bool NmRsEngine::ShouldAllowExtraPenetration(ART::AgentID agentID)
  {
	  bool bAllow = true;
	  NmRsCharacter* character = getCharacterFromAgentID(agentID);
	  if (character)
	  {
		  NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
		  if (cbuDyn)
		  {
			  if(cbuDyn->m_failType == cbuDyn->balOK)
			  {
				  bAllow = false;
			  }
		  }
	  }

	  return bAllow;
  }

  rage::phBound* NmRsEngine::getBoundFromAgent(ART::AgentID agentID) const
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      return character->getArticulatedBound();  
    else
    {
      NM_RS_LOGERROR(L"Error in getBoundFromAgent - clump is invalid, deserialize failed?");
      return 0;
    }
  }

  void NmRsEngine::applyAgentImpacts(ART::AgentID agentID, rage::phContactIterator impacts)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)        
    {
      for (rage::phContactIterator impact = impacts; !impact.AtEnd(); impact++)
        character->handleCollision(impact);
    }
  }

  void NmRsEngine::AssignSelfCollisionSet(rage::phArticulatedCollider *artCollider, SelfCollisionSet *selfCollisionSet)
  {
	  if (artCollider->GetOwnsSelfCollisionElements())
	  {
		  (static_cast<fragInst*>(artCollider->GetInstance()))->GetCacheEntry()->ResetSelfCollisions();
	  }
	  else
	  {
		  artCollider->GetSelfCollisionPairsArrayRefA().Reset(false);
		  artCollider->GetSelfCollisionPairsArrayRefB().Reset(false);
	  }

	  artCollider->SetSelfCollisionPairsA(selfCollisionSet->m_PartCanCollideA);
	  artCollider->SetSelfCollisionPairsB(selfCollisionSet->m_PartCanCollideB);

	  artCollider->SetOwnsSelfCollisionElements(false);
	  artCollider->SetAnyPartsCanCollide(artCollider->GetSelfCollisionPairsArrayRefA().GetCount() > 0);
  }

  rage::Matrix34 *NmRsEngine::getComponentToBoneTransform(ART::AgentID agentID, int componentIndex) const
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character && character->getGenericPartByIndex(componentIndex))
      return (character->getGenericPartByIndex(componentIndex)->getToBoneMatrix());
    else 
    {
      NM_RS_LOGERROR(L"Error in getComponentToBoneTransform - clump is invalid, deserialize failed?");
      return 0;
    }
  }    

  int NmRsEngine::getBoundIndexFromComponentIndex(ART::AgentID agentID, int componentIndex) const
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      return character->getGenericPartByIndex(componentIndex)->getPartIndex();
    else
    {
      NM_RS_LOGERROR(L"Error in getBoundIndexFromComponentIndex - character is invalid, deserialize failed?");
    }
    return -1;
  }   

  void NmRsEngine::setPhInstForAgent(ART::AgentID agentID, rage::phInst *pInst)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      character->setArticulatedPhysInstance(pInst);
  }

  rage::phInst * NmRsEngine::getPhInstForAgent(ART::AgentID agentID)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      return character->getArticulatedPhysInstance();
    return 0;
  }

  void NmRsEngine::configureCharacter(ART::AgentID agentID, bool useZeroPose, bool leftHandFree, bool rightHandFree, float stanceBias, float COMBias)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      character->configureCharacter(useZeroPose, leftHandFree, rightHandFree, stanceBias, COMBias);
  }
  void NmRsEngine::configureTheCharacter(ART::AgentID agentID, bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      character->configureTheCharacter(setZeroPose, setZeroPoseArms, configureBalancer, stanceBias, COMBias);
  }

  void balancerForceFail(int characterID)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
    if (cbuDyn)
    {
      cbuDyn->forceFail();
    }
  }

  void balancerSetIgnoreFailure(int characterID, bool bValue)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
    if (cbuDyn)
    {
      cbuDyn->setIgnoreFailure(bValue);
    }
  }

  bool balancerHasFailed(int characterID)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    NmRsCBUDynamicBalancer* cbuDyn = (NmRsCBUDynamicBalancer*)character->getTask(bvid_dynamicBalancer);
    if (cbuDyn)
    {
      return cbuDyn->hasFailed();
    }

    return false;
  }

    void NmRsEngine::registerWeapon(ART::AgentID characterID, int hand, int levelIndex, rage::phConstraintHandle *constrHandle, rage::Matrix34 &gunToHand, rage::Vector3 &gunToMuzzleInGun, rage::Vector3 &gunToButtInGun)
  {
    NmRsCharacter* character = getCharacterFromAgentID(characterID);
    if (character)
    {
#if ART_ENABLE_BSPY
      //Output this as a message to bSpy - //mmmmtodo we should do this with all global functions
      char parameterName [80];
      char str [80];
      ART::MessageParams params;

      params.reset();
      sprintf(parameterName,"hand");
      if(hand == NmRsCharacter::kLeftHand)
        sprintf(str,"left");
      else
        sprintf(str,"right");
      params.addString(parameterName, str);
      sprintf(parameterName,"levelIndex");
      params.addInt(parameterName, levelIndex);
      sprintf(parameterName,"gunToHand.a");
      params.addVector3(parameterName, gunToHand.a.x, gunToHand.a.y, gunToHand.a.z);
      sprintf(parameterName,"gunToHand.b");
      params.addVector3(parameterName, gunToHand.b.x, gunToHand.b.y, gunToHand.b.z);
      sprintf(parameterName,"gunToHand.c");
      params.addVector3(parameterName, gunToHand.c.x, gunToHand.c.y, gunToHand.c.z);
      sprintf(parameterName,"gunToHand.d");
      params.addVector3(parameterName, gunToHand.d.x, gunToHand.d.y, gunToHand.d.z);
      sprintf(parameterName,"gunToMuzzleInGun");
      params.addVector3(parameterName, gunToMuzzleInGun.x, gunToMuzzleInGun.y, gunToMuzzleInGun.z);
      sprintf(parameterName,"gunToButtInGun");
      params.addVector3(parameterName, gunToButtInGun.x, gunToButtInGun.y, gunToButtInGun.z);
      sprintf(parameterName,"gunToHandConstraint");
      params.addReference(parameterName, constrHandle);
      sprintf(parameterName,"registerWeapon_G");
      character->sendDirectInvoke(parameterName,&params);
#endif

        character->registerWeapon(hand, levelIndex, constrHandle, gunToHand, gunToMuzzleInGun, gunToButtInGun);
    }
  }

  void NmRsEngine::setWeaponMode(ART::AgentID agentID, int weaponMode)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      character->setWeaponMode(weaponMode);
  }

#if NM_SET_WEAPON_MASS
  void NmRsEngine::setWeaponBound(ART::AgentID agentID, rage::phBound* boundPtr, int hand, float mass /* = 0 */, rage::Vector3* comOffset /* = 0 */)
#else
  void NmRsEngine::setWeaponBound(ART::AgentID agentID, rage::phBound* boundPtr, int hand, float /*mass*/ /* = 0 */, rage::Vector3* /* comOffset */ /* = 0 */)
#endif
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
    {
      character->setHandBound(hand, boundPtr);
#if NM_SET_WEAPON_MASS
      character->setHandMass(hand, mass, comOffset);
#endif
    }
  }

  void NmRsEngine::configureDontRegisterProbeVelocity(ART::AgentID agentID, float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow)
  {
    NmRsCharacter* character = getCharacterFromAgentID(agentID);
    if (character)
      character->setDontRegisterProbeVelocity(dontRegisterProbeVelocityMassBelow, dontRegisterProbeVelocityVolBelow);
  }

  bool setRockstarEnvironment(rage::phSimulator* sim, rage::phLevelNew* level)
  { 
    if (gRockstarARTInstance)
    {
      gRockstarARTInstance->setRockstarEnvironment(sim, level);
      return true;
    }

    return false;
  }

  bool setIncomingAnimationVelocityScale(float scale)
  {
    if (gRockstarARTInstance)
    {
      gRockstarARTInstance->setIncomingAnimationVelocityScale(scale);
      return true;
    }

    return false;
  }

  bool setIncomingAnimationVelocityScale(ART::AgentID agentID, float scale)
  {
    if(gRockstarARTInstance)
    {
      NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
      if (character)
      {
        character->setIncomingAnimationVelocityScale(scale);
        return true;
      }
      else
        return false;
    }
    return false;
  }

  void handleCollision(ART::AgentID agentID, phContactIterator impacts)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);

    for (phContactIterator impact = impacts; !impact.AtEnd(); impact++)
    {
      character->handleCollision(impact);
    }
  }

  rage::phBound *getBoundFromAgent(ART::AgentID agentID)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->getBoundFromAgent(agentID);
  }

  void applyAgentImpacts(ART::AgentID agentID, rage::phContactIterator impacts)
  {
    Assert(gRockstarARTInstance);
    gRockstarARTInstance->applyAgentImpacts(agentID, impacts);
  }

  rage::Matrix34 *getComponentToBoneTransform(ART::AgentID agentID, int componentIndex)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->getComponentToBoneTransform(agentID, componentIndex);
  }        

  void setPhInstForNextAgent(rage::phInst *pInst)
  {
    Assert(gRockstarARTInstance);
    cachedPhInstPointer = pInst;
  }

  void setPhInstForAgent(ART::AgentID agentID, rage::phInst *pInst)
  {
    Assert(gRockstarARTInstance);
    gRockstarARTInstance->setPhInstForAgent(agentID, pInst);
  }

  rage::phInst * getPhInstForAgent(ART::AgentID agentID)
  {
    Assert(gRockstarARTInstance);
    if(gRockstarARTInstance)
      return gRockstarARTInstance->getPhInstForAgent(agentID);
    return 0;
  }

  int getBoundIndexFromComponentIndex(ART::AgentID agentID, int componentIndex)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->getBoundIndexFromComponentIndex(agentID, componentIndex);
  }

  void configureCharacter(ART::AgentID agentID, bool useZeroPose, bool leftHandFree, bool rightHandFree, float stanceBias, float COMBias)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->configureCharacter(agentID, useZeroPose, leftHandFree, rightHandFree, stanceBias, COMBias);
  }

  void configureTheCharacter(ART::AgentID agentID, bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->configureTheCharacter(agentID, setZeroPose, setZeroPoseArms, configureBalancer, stanceBias, COMBias);
  }

    void registerWeapon(ART::AgentID characterID, int hand, int levelIndex, rage::phConstraintHandle *constrHandle, rage::Matrix34 &gunToHand, rage::Vector3 &gunToMuzzleInGun, rage::Vector3 &gunToButtInGun)
  {
    Assert(gRockstarARTInstance);
      return gRockstarARTInstance->registerWeapon(characterID, hand, levelIndex, constrHandle, gunToHand, gunToMuzzleInGun, gunToButtInGun);
  }

  void setWeaponMode(ART::AgentID agentID, int weaponMode)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->setWeaponMode(agentID, weaponMode);
  }

  void setWeaponBound(ART::AgentID agentID, rage::phBound* boundPtr, int hand, float mass /* = 0 */, rage::Vector3* comOffset /* = 0 */)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->setWeaponBound(agentID, boundPtr, hand, mass, comOffset);
  }

  void configureDontRegisterProbeVelocity(ART::AgentID agentID, float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow)
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->configureDontRegisterProbeVelocity(agentID, dontRegisterProbeVelocityMassBelow, dontRegisterProbeVelocityVolBelow);
  }

  void setProbeIncludeFlags(ART::AgentID agentID, unsigned int flags)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);
    character->m_probeTypeIncludeFlags = flags;
  }

  void setProbeTypeIncludeFlags(ART::AgentID characterID, unsigned int flags)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    character->m_probeTypeIncludeFlags = flags;
  }
  void setProbeTypeExcludeFlags(ART::AgentID characterID, unsigned int flags)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    character->m_probeTypeExcludeFlags = flags;
  }

  bool directInvoke(ART::AgentID characterID, InvokeUID iUID, const MessageParamsBase* const params)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(characterID);
    Assert(character);
    return character->handleDirectInvoke(iUID, params);
  }

  void setTaskScheduler(int schedulerIndex)
  {
    Assert(gRockstarARTInstance);
    gRockstarARTInstance->setSchedulerIndex(schedulerIndex);
  }

  void setDistributedTasksEnabled(bool enabled)
  {
    Assert(gRockstarARTInstance);
    gRockstarARTInstance->setDistributedTasksEnabled(enabled);
  }

  bool areDistributedTasksEnabled()
  {
    Assert(gRockstarARTInstance);
    return gRockstarARTInstance->areDistributedTasksEnabled();
  }

#if ART_ENABLE_BSPY
  void setbSpyObject(int levelIndex)
  {
    Assert(gRockstarARTInstance);
    gRockstarARTInstance->setbSpyObject(levelIndex);
  }
#else
  void setbSpyObject(int /*levelIndex*/)
  {
  }
#endif

  bool setFromAnimationMaxSpeed(float speed)
  {
    if (gRockstarARTInstance)
    {
      gRockstarARTInstance->setFromAnimationMaxSpeed(speed);
      return true;
    }

    return false;
  }

  bool setFromAnimationMaxAngSpeed(float angSpeed)
  {
    if (gRockstarARTInstance)
    {
      gRockstarARTInstance->setFromAnimationMaxAngSpeed(angSpeed);
      return true;
    }

    return false;
  }

  bool setFromAnimationAverageVel(const rage::Vector3 &vel, bool reset)
  {
    if (gRockstarARTInstance)
    {
      gRockstarARTInstance->setFromAnimationAverageVel(vel, reset);
      return true;
    }

    return false;
  }

  bool hasCollidedWithWorld(int agentID, const char *twoCCMask)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);
    return character->hasCollidedWithWorld(character->nameToMask(twoCCMask));
  }

  bool hasCollidedWithOtherCharacters(int agentID, const char *twoCCMask)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);
    return character->hasCollidedWithOtherCharacters(character->nameToMask(twoCCMask));
  }

  bool hasCollidedWithEnvironment(int agentID, const char *twoCCMask)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);
    return character->hasCollidedWithEnvironment(character->nameToMask(twoCCMask));
  }

  void getAttachedWeaponIndices(int agentID, int& leftHandWeaponIndex, int& rightHandWeaponIndex)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    if(character)
      character->getAttachedWeaponIndices(leftHandWeaponIndex, rightHandWeaponIndex);
  }

  void applyInjuryMask(int agentID, const char* twoCCMask, float injuryAmount)
  {
    Assert(gRockstarARTInstance);
    NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
    Assert(character);
    character->applyInjuryMask(character->nameToMask(twoCCMask), injuryAmount);
  }

  bool getCOMVel(int agentID, rage::Vector3 &outVel)
  {
    if(gRockstarARTInstance)
    {
      NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
      if(character)
      {
        outVel.Set(character->m_COMvel);
        return true;
      }
    }
    return false;
  }

  bool getCOMVelRelative(int agentID, rage::Vector3 &outVel)
  {
    if(gRockstarARTInstance)
    {
      NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
      if(character)
      {
        outVel.Set(character->m_COMvelRelative);
        return true;
      }
    }
    return false;
  }

  bool getCOMRotVel(int agentID, rage::Vector3 &outVel)
  {
    if(gRockstarARTInstance)
    {
      NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
      if(character)
      {
        outVel.Set(character->m_COMrotvel);
        return true;
      }
    }
    return false;
  }

  void setCollisionEnabledOnComponent(int agentID, int partIndex, bool enabled)
  {
    if(gRockstarARTInstance)
    {
      NmRsCharacter* character = gRockstarARTInstance->getCharacterFromAgentID(agentID);
      if(character)
      {
        character->getGenericPartByIndex(partIndex)->setCollisionEnabled(enabled);
      }
    }
  }
}

