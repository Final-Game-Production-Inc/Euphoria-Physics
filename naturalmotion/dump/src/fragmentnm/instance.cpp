// 
// fragmentnm/instance.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
//  

#include "instance.h"
#include "manager.h"

#include "crskeleton/skeleton.h"
#include "crskeleton/skeletondata.h"
#include "fragment/cache.h"
#include "fragment/drawable.h"
#include "fragment/manager.h"
#include "fragment/typechild.h"
#include "physics/archetype.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "physics/colliderdispatch.h"
#include "phsolver/contactmgr.h"
#include "profile/element.h"
#include "system/timer.h"

#include <art/ARTRockstar.h>
#include <map>
#include <vector>
#include <list>
#include <art/ARTBaseDefs.h>
#include <art/art.h>
#include <art/MessageParams.h>
#if HACK_GTA4 // To fix compile error with bankrelease.
#include <NMutils/TypeUtils.h>
#endif // HACK_GTA4
#include <rockstar/NmRsCommon.h>
#include <rockstar/NmRsInclude.h>
#include <rockstar/NmRsCharacter.h>
#include <NMutils/TypeUtils.h>
#include "fragment/typephysicslod.h"

FRAGMENT_OPTIMISATIONS()

namespace rage {

float fragInstNM::sm_ExtraAllowedRagdollPenetration = 0.0f;

fragInstNM::fragInstNM(const fragType* type, const Matrix34& matrix, u32 guid)
	: fragInst(type, matrix, guid)
	, m_AgentId(-1)
	, m_BlockNMActiavtion(false)
	, m_SimulatingAsNonNMRagdoll(false)
	, m_ZeroLastMatricesOnActivation(false)
	, m_randomSeed(0)
{
  }

fragInstNM::fragInstNM(datResource& rsc)
	: fragInst(rsc)
	, m_AgentId(-1)
	, m_BlockNMActiavtion(false)
	, m_SimulatingAsNonNMRagdoll(false)
	, m_ZeroLastMatricesOnActivation(false)
	, m_randomSeed(0)
{
}

  void fragInstNM::PostARTMessage(const char* messageName, const ART::MessageParamsBase * params) const
  {
    //Changed by RDR
    if (m_AgentId == -1)
    {
      // Ignore the message if we do not have an agent
      return;
    }

	Assertf(!params->getParameterOverflow(), "Too many parameters in ART::MessageParams, Message:%s, NumParams:%d, MaxParams:%d", messageName, params->getUsedParamCount(), params->getMaxParamCount());
    if (ART::gRockstarARTInstance->directInvoke(
      ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
      NMutils::hashString(messageName), 
      params) == false)
    {
      Assertf(0, "ART message %s not recognized", messageName);
    }

  }

  void fragInstNM::SetComponentTMsFromSkeleton(const crSkeleton& skeleton)
  {
    if(m_AgentId != -1)
    {
      const fragType* type = GetType();
      Assert(type);

      int numChildren = GetTypePhysics()->GetNumChildren();
      Matrix34* currentMatrices = FRAGNMASSETMGR->GetWorldCurrentMatrices(m_AgentId);

      // Go through each fragTypeChild/bound component ...
      for (int childIndex = 0; childIndex < numChildren; ++childIndex)
      {
        fragTypeChild* child = GetTypePhysics()->GetAllChildren()[childIndex];
        int boneIndex = type->GetBoneIndexFromID(child->GetBoneID());
        Assert(boneIndex >= 0);

        const Matrix34* pattachment = ART::getComponentToBoneTransform(m_AgentId, childIndex);
        AssertMsg(pattachment , "Failed to find attachment matrix from getComponentToBoneTransform");
        Matrix34 attachment;
        if (pattachment)
        {
          attachment = *pattachment;
          attachment.Inverse();
        }
        else
        {
          attachment.Identity();
        }

        Matrix34 boneMtx;
        skeleton.GetGlobalMtx(boneIndex, RC_MAT34V(boneMtx));
        currentMatrices[childIndex] = attachment;
        currentMatrices[childIndex].Dot(boneMtx);
        currentMatrices[childIndex].a.w = 0.0f;
        currentMatrices[childIndex].b.w = 0.0f;
        currentMatrices[childIndex].c.w = 0.0f;
        currentMatrices[childIndex].d.w = 1.0f;
      }

      ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
        currentMatrices,
        numChildren,
        ART::kITSNone,
        ART::kITSourceCurrent);

#if NM_ANIM_MATRICES && NM_TESTING_ANIM_MATRICES
#if 0
    //test blendout by putting current into blendout(fudged to be previous)
      ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
        currentMatrices,
        numChildren,
        ART::kITSNone,
        ART::kITSourceAnimation);
#endif
      //test leadIn by sending freds itm's to wilma
      if (GetARTAssetID() == 0)
        SetComponentTMsFromSkeleton(skeleton, ART::kITSourceAnimation, NULL);
#endif
    }
  }

#if NM_ANIM_MATRICES
  void fragInstNM::SetComponentTMsFromSkeleton(const crSkeleton& skeleton, int tmType, phArticulatedBody *UNUSED_PARAM(body))
  {
    Assert(tmType == ART::kITSourceCurrent 
      || tmType == ART::kITSourcePrevious 
      || tmType == ART::kITSourceAnimation);
#if NM_TESTING_ANIM_MATRICES
    int agentId = m_AgentId;
    m_AgentId = -1;
#endif 
    //mmmmtodo Expand to all character types.   //mmmm if m_AgentId == -1 Assumes tmType = 0 or 1 i.e. fred or wilma
    //mmmmNote m_AgentId == -1 with onDemand agent means !agent->isInsertedInScene()
    if(m_AgentId != -1 || (m_AgentId == -1 && static_cast<ART::IncomingTransformSource>(tmType) == ART::kITSourceAnimation && (GetARTAssetID()==0 || GetARTAssetID()==1)))
    {
      Matrix34* currentMatrices  = NULL;
      int numChildren = GetTypePhysics()->GetNumChildren();
      const fragType* type = GetType();
      Assert(type);
      if(m_AgentId != -1)
      {
        ART::NmRsCharacter *agent = ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId); 

        if (tmType == ART::kITSourceCurrent)
          currentMatrices = FRAGNMASSETMGR->GetWorldCurrentMatrices(m_AgentId);
        else if (tmType == ART::kITSourcePrevious)
          currentMatrices = FRAGNMASSETMGR->GetWorldLastMatrices(m_AgentId);
        else if (tmType == ART::kITSourceAnimation && agent->isInsertedInScene())
          currentMatrices = FRAGNMASSETMGR->GetBlendOutAnimationMatrices(m_AgentId);
      }
#if ART_ENABLE_BSPY
      else
      {
        if (tmType == ART::kITSourceAnimation)
        {
#if NM_TESTING_ANIM_MATRICES
          currentMatrices = ART::gRockstarARTInstance->GetLeadInAnimationMatrices(1);
          ART::gRockstarARTInstance->setLeadInAnimationSentIn(1, true);
#else
          currentMatrices = ART::gRockstarARTInstance->GetLeadInAnimationMatrices(GetARTAssetID());
          ART::gRockstarARTInstance->setLeadInAnimationSentIn(GetARTAssetID(), true);
#endif
        }
      }
#endif
      //else unknown tmType
      Assert(currentMatrices);
      CheckComponentToBoneMatrices();

      // Go through each fragTypeChild/bound component ...
      for (int childIndex = 0; childIndex < numChildren; ++childIndex)
      {
        fragTypeChild* child = GetTypePhysics()->GetAllChildren()[childIndex];
        int boneIndex = type->GetBoneIndexFromID(child->GetBoneID());
        Assert(boneIndex >= 0);

        const Matrix34* pattachment = &ART::gRockstarARTInstance->m_characterTypeData[GetARTAssetID()].m_ComponentToBoneMatrices[childIndex];
        AssertMsg(pattachment , "Failed to find attachment matrix from ART::gRockstarARTInstance->m_characterTypeData[GetARTAssetID()].m_ComponentToBoneMatrices[childIndex]");
        Matrix34 attachment;
        if (pattachment)
        {
          attachment = *pattachment;
          attachment.Inverse();
        }
        else
        {
          attachment.Identity();
        }

        Matrix34 boneMtx;
        skeleton.GetGlobalMtx(boneIndex, RC_MAT34V(boneMtx));
        currentMatrices[childIndex] = attachment;
        currentMatrices[childIndex].Dot(boneMtx);
        currentMatrices[childIndex].a.w = 0.0f;
        currentMatrices[childIndex].b.w = 0.0f;
        currentMatrices[childIndex].c.w = 0.0f;
        currentMatrices[childIndex].d.w = 1.0f;
      }
      //mmmmmIsn't this done above anyway?
#if ART_ENABLE_BSPY
      if (tmType == ART::kITSourceAnimation && m_AgentId == -1)
      {
#if NM_TESTING_ANIM_MATRICES
        ART::gRockstarARTInstance->setLeadInAnimationTMs(currentMatrices, 1);
#else
        ART::gRockstarARTInstance->setLeadInAnimationTMs(currentMatrices, GetARTAssetID());
#endif
      }
      else
#endif
      {
        ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
          currentMatrices,
          numChildren,
          ART::kITSNone,
          (ART::IncomingTransformSource) tmType);
      }
    }
#if NM_TESTING_ANIM_MATRICES
    m_AgentId = agentId;
#endif 
  }
#else//NM_ANIM_MATRICES
  void fragInstNM::SetComponentTMsFromSkeleton(const crSkeleton& /*skeleton*/, int /*tmType*/, phArticulatedBody *UNUSED_PARAM(body))
  {
  }
#endif//NM_ANIM_MATRICES

  void fragInstNM::CheckComponentToBoneMatrices()
  {
    if (!ART::gRockstarARTInstance->m_characterTypeData[GetARTAssetID()].m_ComponentToBoneMatrices)
    {
      int numParts = GetTypePhysics()->GetNumChildren();
      ART::gRockstarARTInstance->m_characterTypeData[GetARTAssetID()].m_ComponentToBoneMatrices = 
        (rage::Matrix34*)ART::gRockstarARTInstance->GetArtMemoryManager()->callocate(sizeof(rage::Matrix34) * numParts, NM_MEMORY_TRACKING_ARGS);
      rage::fragTypeChild* child = NULL;

      for (int pIndex = 0; pIndex < numParts; ++pIndex)
      {
        child = GetTypePhysics()->GetAllChildren()[pIndex];
        Matrix34 currentMatrix = child->GetUndamagedEntity()->GetBoundMatrix();
        currentMatrix.Inverse();
        ART::gRockstarARTInstance->m_characterTypeData[GetARTAssetID()].m_ComponentToBoneMatrices[pIndex] = currentMatrix;
      }
    }
  }

  void fragInstNM::SetLastComponentTMsFromCurrent()
  {
    if( m_AgentId != -1 )
    {
      int numChildren = GetTypePhysics()->GetNumChildren();
      Matrix34* lastMatrices = FRAGNMASSETMGR->GetWorldLastMatrices(m_AgentId);
      Matrix34* currentMatrices = FRAGNMASSETMGR->GetWorldCurrentMatrices(m_AgentId);

      // Go through each fragTypeChild/bound component ...
      for (int childIndex = 0; childIndex < numChildren; ++childIndex)
      {
        lastMatrices[childIndex].Set(currentMatrices[childIndex]);
      }

      ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
        lastMatrices,
        numChildren,
        ART::kITSNone,
        ART::kITSourcePrevious);
    }
  }

  void fragInstNM::SetARTFeedbackInterface(ART::ARTFeedbackInterface* pInterface)
  {
    ART::gRockstarARTInstance->setAgentFeedbackInterface(m_AgentId, pInterface);
  }

  void fragInstNM::ConfigureCharacter(int characterID, bool zeroPose, int leftHandFree, int rightHandFree, float stanceBias, float COMBias)
  {
    ART::configureCharacter(characterID, zeroPose, (leftHandFree > 0), (rightHandFree > 0), stanceBias, COMBias);
  }

  void fragInstNM::ConfigureTheCharacter(int characterID, bool setZeroPose, bool setZeroPoseArms, bool configureBalancer, float stanceBias, float COMBias)
  {
    ART::configureTheCharacter(characterID, setZeroPose, setZeroPoseArms, configureBalancer, stanceBias, COMBias);
  }

void fragInstNM::RegisterWeapon(int characterID, int hand, int levelIndex, rage::phConstraintHandle *gunToHandConstrHandle, rage::Matrix34 &gunToHand, rage::Vector3 &gunToMuzzleInGun, rage::Vector3 &gunToButtInGun)
  {
  ART::registerWeapon(characterID, hand, levelIndex, gunToHandConstrHandle, gunToHand, gunToMuzzleInGun, gunToButtInGun);
  }

  void fragInstNM::SetWeaponMode(int characterID, int weaponMode)
  {
    ART::setWeaponMode(characterID, weaponMode);
  }

  void fragInstNM::SetWeaponBound(int characterID, rage::phBound* boundPtr, int hand, rage::Matrix34* /*boundTransform*/, float mass /* = 0 */)
  {
    SetWeaponBound(characterID, boundPtr, hand, 0, mass, 0);
  }

  void fragInstNM::SetWeaponBound(int characterID, rage::phBound* boundPtr, int hand, rage::Matrix34* /*boundTransform*/, float mass /* = 0 */, rage::Vector3* comOffset /* = 0 */)
  {
    ART::setWeaponBound(characterID, boundPtr, hand, mass, comOffset);
  }

  void fragInstNM::setIncomingAnimationVelocityScale(int characterID, float scale)
  {
    ART::setIncomingAnimationVelocityScale(characterID, scale);
  }

  void fragInstNM::ConfigureDontRegisterProbeVelocity(int characterID, float dontRegisterProbeVelocityMassBelow, float dontRegisterProbeVelocityVolBelow)
  {
    ART::configureDontRegisterProbeVelocity(characterID, dontRegisterProbeVelocityMassBelow, dontRegisterProbeVelocityVolBelow);
  }


  void fragInstNM::setProbeTypeIncludeFlags(int characterID, unsigned int flags)
  {
    ART::setProbeTypeIncludeFlags(characterID, flags);
  }

  void fragInstNM::setProbeTypeExcludeFlags(int characterID, unsigned int flags)
  {
    ART::setProbeTypeExcludeFlags(characterID, flags);
  }

  bool fragInstNM::HasCollidedWithWorld(const char *twoCCMask) const
  {
    return m_AgentId>=0 && ART::hasCollidedWithWorld(m_AgentId, twoCCMask);
  }

  bool fragInstNM::hasCollidedWithOtherCharacters(const char *twoCCMask)
  {
    return m_AgentId>=0 && ART::hasCollidedWithOtherCharacters(m_AgentId, twoCCMask);
  }

  bool fragInstNM::hasCollidedWithEnvironment(const char *twoCCMask)
  {
    return m_AgentId>=0 && ART::hasCollidedWithEnvironment(m_AgentId, twoCCMask);
  }


  void fragInstNM::ApplyInjuryMask(const char* twoCCMask, float injuryAmount)
  {
    if(m_AgentId>=0)
      ART::applyInjuryMask(m_AgentId, twoCCMask, injuryAmount);
  }

  bool fragInstNM::GetCOMVel(Vector3 &outCOMVel) const
  {
    return (m_AgentId>=0) && ART::getCOMVel(m_AgentId, outCOMVel);
  }

  bool fragInstNM::GetCOMRotVel(Vector3 &outCOMVel) const
  {
    return (m_AgentId>=0) && ART::getCOMRotVel(m_AgentId, outCOMVel);
  }

  bool fragInstNM::GetCOMTM(Matrix34 &outCOMTM) const
  {
    if (m_AgentId >= 0)
    {
  	  ART::NmRsCharacter* agent = ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId);
  	  if (agent != NULL)
  	  {
  		  outCOMTM.Set(agent->m_COMTM);
  		  return true;
  	  }
    }

    return false;
  }

  void fragInstNM::forceStayUprightSwitch(bool forceOn)
  {
    int NMid = GetNMAgentID();
    if (NMid >= 0)
    {
      if (forceOn)
      {
        ART::MessageParams msgP;

        msgP.addBool("useForces", true);
        msgP.addFloat("forceStrength", 16.0f);
        msgP.addFloat("forceDamping", -1.0f);
        msgP.addFloat("forceFeetMult", 0.6f);
        msgP.addFloat("forceLeanReduction", 0.75f);
        msgP.addFloat("forceInAirShare", 0.75f);
        msgP.addFloat("forceMin", -1.0);
        msgP.addFloat("forceMax", -1.0);

        msgP.addBool("useTorques", true);
        msgP.addFloat("torqueStrength", 16.0f);
        msgP.addFloat("torqueDamping", 0.075f);

        msgP.addBool("velocityBased", true);
        msgP.addBool("torqueOnlyInAir", false);

        msgP.addFloat("forceSaturationVel", 5.0f);
        msgP.addFloat("forceThresholdVel", 0.75f);
        msgP.addFloat("torqueSaturationVel", 5.0f);
        msgP.addFloat("torqueThresholdVel", 5.0f);

        msgP.addFloat("supportPosition", 0.0675f);
        msgP.addFloat("noSupportForceMult", 0.0675f);

        // Force stayUpright
        ART::gRockstarARTInstance->directInvoke(
          ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId), 
          NMutils::hashString("stayUpright"),
          &msgP);
      }
      else
      {
        ART::gRockstarARTInstance->directInvoke(
          ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId), 
          NMutils::hashString("stayUpright"),
          0);
      }
    }
  }

  void fragInstNM::SetNMAssetSize(int newSize)
  {
	  int currentAssetID = GetARTAssetID();
	  if (currentAssetID < 0)
	  {
		  Assertf(0, "fragInst::SwitchNMAssetSize() called on a non-NM agent");
		  return;
	  }

	  // Determine what the new asset ID should be (based on gender and current size)
	  int currentSize = currentAssetID <= 1 ? 0 : 1;
	  if (currentSize == newSize)
	  {
		  Warningf("fragInst::SwitchNMAssetSize() - Input size is already the current size");
		  return;
	  }

	  // Determine what the new asset ID should be (based on gender and current size)
	  int newID = 0;
	  if (currentAssetID == 0 || currentAssetID == 2)
		  newID = currentAssetID == 0 ? 2 : 0;
	  else
		  newID = currentAssetID == 1 ? 3 : 1;

	  // Set the new art asset ID, physics type data, and ragdoll LOD
	  Displayf("Switching NM art asset ID to %d.", newID);
	  ((fragType*) GetType())->SetARTAssetID(newID);
	  ((fragType*) GetType())->SetPhysicsLODGroup(FRAGMGR->GetPhysicsLODGroups()[GetType()->GetARTAssetID()]);
	  m_CurrentLOD = RAGDOLL_LOD_HIGH; 

	  PHSIM->GetContactMgr()->RemoveAllContactsWithInstance(this);

	  // Need to do more if there's a cache entry
	  fragCacheEntry* cacheEntry = GetCacheEntry();
	  if (cacheEntry)
	  {
		  cacheEntry->ReloadPhysicsData();
		  ASSERT_ONLY(cacheEntry->GetBound()->CheckCachedMinMaxConsistency());
	  }
  }

  int fragInstNM::GetNMAgentID() const
  {
    return m_AgentId;
  }

  float fragInstNM::GetNMImpulseModifierUpdate()
  {
    return ART::gRockstarARTInstance->getImpulseModifierUpdate(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId));
  }

  void fragInstNM::ResetNMImpulseModifierUpdate()
  {
    ART::gRockstarARTInstance->resetImpulseModifierUpdate(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId));
  }

  void fragInstNM::PreComputeImpacts (phContactIterator impacts)
  {
    //	const fragType* type = GetType();
    //	Assert(type);
    //
    //	if (type->GetARTAssetID() != -1 &&
    //		FRAGNMASSETMGR->SuccessfullyLoaded(GetARTAssetID()) &&
    //		m_AgentId != -1)
    if (m_AgentId != -1)
    {
      ART::handleCollision(m_AgentId, impacts);
    }

    fragInst::PreComputeImpacts(impacts);
  }

  struct RagdollManifold
  {
	  phManifold* manifold;
	  int component;

	  bool operator< ( const RagdollManifold &b ) const
	  {
		  return component < b.component;
	  }
  };

  namespace phNaturalMotionStats
  {
    EXT_PF_TIMER(FixStuckInGeometry);
  }

  using namespace phNaturalMotionStats;

  bool g_FixRagdollStuckInGeometry = true;
  bool g_FixRagdollStuckInGeometryElementMatch = false;
  float g_FixRagdollStuckInGeometryDepth = 0.0f;

  void fragInstNM::FixStuckInGeometry(phContactIterator iterator, int pedType)
  {
	  PF_FUNC(FixStuckInGeometry);

	  if (!g_FixRagdollStuckInGeometry)
	  {
		  return;
	  }

	  fragCacheEntry* entry = GetCacheEntry();

	  if (!entry)
	  {
		  return;
	  }

	  phArticulatedBody* body = entry->GetHierInst()->body;
	  
	  if (!body)
	  {
		  return;
	  }

	  int numBodyParts = body->GetNumBodyParts();
	  Assertf(GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE &&
		  static_cast<phBoundComposite*>(GetArchetype()->GetBound())->GetNumBounds() == numBodyParts,
		  "fragInstNM::FixStuckInGeometry is currently written to assume that there is one body part per component");

	  static const u32 MAX_NUM_RAGDOLL_MANIFOLDS = 512;
	  RagdollManifold manifolds[MAX_NUM_RAGDOLL_MANIFOLDS + 1]; // Add one so there is room for the fake manifold at the end
	  u32 numManifolds = 0;

	  // Copy all the manifolds and component numbers into a local array so we can further sort them and iterate over them faster
	  for (phContactIterator it = iterator; numManifolds < MAX_NUM_RAGDOLL_MANIFOLDS && !it.AtEnd(); it.NextManifold())
	  {
		  if (it.GetInstanceA() != it.GetInstanceB() && !it.IsConstraint() && it.GetOtherInstance()->GetClassType()!=pedType ) // Don't worry about self collisions or constraints or other ragdolls
		  {
			  RagdollManifold& rc = manifolds[numManifolds++];
			  rc.component = it.GetMyComponent();
			  rc.manifold = &it.GetCachedManifold();
		  }
	  }

	  RagdollManifold* lastContact = manifolds + numManifolds;
	  std::sort(manifolds, lastContact);

	  // Build up a map of manifold indices by component number, so we can quickly find the manifolds on any component
	  int firstManifoldByComponent[phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS + 1];
	  int partIndex = 0;

	  // Go through the sorted manifolds, and look for each place where we change component numbers
	  for (u32 manifoldIndex = 0; manifoldIndex < numManifolds && partIndex < numBodyParts; ++manifoldIndex)
	  {
		  RagdollManifold& manifold = manifolds[manifoldIndex];
		  int component = manifold.component;

		  if (!Verifyf(component <= numBodyParts + 1,
			  "Component number %d more than the number of parts of ragdoll %s, %d",
			  component, GetArchetype()->GetFilename(), numBodyParts))
		  {
			  component = numBodyParts + 1;
		  }

		  // Fill our manifold index into the map up to the next component.
		  while (component >= partIndex)
		  {
			  TrapGT(partIndex, numBodyParts + 1);
			  firstManifoldByComponent[partIndex++] = manifoldIndex;
		  }
	  }

	  // Fill in the end of the array with "numManifolds", so that any further elements without any manifolds will register
	  // as having zero manifolds, and so that the last component index will get the right number of manifolds
	  while (partIndex <= numBodyParts + 1)
	  {
		  firstManifoldByComponent[partIndex++] = numManifolds;
	  }

	  bool fixed = false; // Set if we did any fixes

	  // The strategy is, compare all contacts on each part to the contacts on its parent. If they're pointing in completely
	  // opposite directions then we likely have somehow gotten into a situation where a body part has popped through a
	  // wall and now has contacts with the back side facing away. When this happens, we reverse the normals on the child's
	  // contacts and give them some fake depth, so we'll get pushed up out of penetration.

	  for (int partIndex = 1; partIndex < numBodyParts; ++partIndex)
	  {
		  // We can look at partIndex + 1 because we took care to make sure the map was filled in one past the last body part
		  int firstManifoldOnPart = firstManifoldByComponent[partIndex];
		  int lastManifoldOnPart = firstManifoldByComponent[partIndex + 1];

		  int parentIndex = body->GetParentNum(partIndex);

		  int firstManifoldOnParent = firstManifoldByComponent[parentIndex];
		  int lastManifoldOnParent = firstManifoldByComponent[parentIndex + 1];

		  // Loop over the manifolds touching the child part
		  for (int partManifoldIndex = firstManifoldOnPart; partManifoldIndex < lastManifoldOnPart; ++partManifoldIndex)
		  {
			  Assert(manifolds[partManifoldIndex].component == partIndex);

			  phManifold& partManifold = *manifolds[partManifoldIndex].manifold;

			  bool partInstanceA = partManifold.GetInstanceA() == this;
			  int partOtherLevelIndex = partInstanceA ? partManifold.GetLevelIndexB() : partManifold.GetLevelIndexA();
			  int partOtherComponent = partInstanceA ? partManifold.GetComponentB() : partManifold.GetComponentA();

			  // Loop over the manifolds touching the parent part
			  for (int parentManifoldIndex = firstManifoldOnParent; parentManifoldIndex < lastManifoldOnParent; ++parentManifoldIndex)
			  {
				  Assert(manifolds[parentManifoldIndex].component == parentIndex);

				  phManifold& parentManifold = *manifolds[parentManifoldIndex].manifold;

				  bool parentInstanceA = parentManifold.GetInstanceA() == this;
				  int parentOtherLevelIndex = parentInstanceA ? parentManifold.GetLevelIndexB() : parentManifold.GetLevelIndexA();
				  int parentOtherComponent = parentInstanceA ? parentManifold.GetComponentB() : parentManifold.GetComponentA();

				  // No need to look at contacts if these two manifolds are touching different objects or components
				  if (partOtherLevelIndex == parentOtherLevelIndex && partOtherComponent == parentOtherComponent)
				  {
					  // Loop over the contacts in the manifold on the child part
					  int partNumContacts = partManifold.GetNumContacts();
					  for (int partContactIndex = 0; partContactIndex < partNumContacts; ++partContactIndex)
					  {
						  phContact& partContact = partManifold.GetContactPoint(partContactIndex);
						  int partOtherPartIndex = partInstanceA ? partContact.GetElementB() : partContact.GetElementA();

						  // Loop over the contacts in the manifold on the parent part
						  int parentNumContacts = parentManifold.GetNumContacts();
						  for (int parentContactIndex = 0; parentContactIndex < parentNumContacts; ++parentContactIndex)
						  {
							  phContact& parentContact = parentManifold.GetContactPoint(parentContactIndex);
							  int parentOtherPartIndex = parentInstanceA ? parentContact.GetElementB() : parentOtherPartIndex = parentContact.GetElementA();

							  // If both child and parent are in contact with the same primitive
 							  if (!g_FixRagdollStuckInGeometryElementMatch || partOtherPartIndex == parentOtherPartIndex)
							  {
								  Vec3V partNormal = partInstanceA ? partContact.GetWorldNormal() : -partContact.GetWorldNormal();
								  Vec3V parentNormal = parentInstanceA ? parentContact.GetWorldNormal() : -parentContact.GetWorldNormal();

								  // And the normals point in opposite directions
								  if (IsLessThanAll(Dot(partNormal, parentNormal), -ScalarV(V_HALF)))
								  {
									  // Then reverse the child contact's normal and give it enough depth to probably pop the child back out
									  partContact.SetWorldNormal(-partContact.GetWorldNormal());
									  partContact.SetDepth(g_FixRagdollStuckInGeometryDepth);

									  fixed = true;
								  }
							  }
						  }
					  }
				  }
			  }
		  }
	  }

	  if (fixed)
	  {
		  Warningf("Ragdoll stuck in geometry, activating emergency countermeasures!");
	  }
  }

  bool g_PreventRagdollPushCollisions = false;
  extern bool g_EnablePreventsPushCollisions; // From simulator.cpp

  phInst* fragInstNM::PrepareForActivation(phCollider **collider, phInst* otherInst, const phConstraintBase * constraint)
  {
	if(GetInstFlag(FLAG_NEVER_ACTIVATE))
	{
		return NULL;
	}

    fragType* type = const_cast<fragType*>(GetType());
    Assert(type);

    // Check that this ragdoll has a physics rig.  If not, assert and bail.
    if (!GetTypePhysics()->GetBodyType())
    {
      AssertMsg(0, "fragInstNM::PrepareForActivation - No Endorphin physics rig found");
	  return NULL;
    }

    if (m_AgentId != -1)
    {
      // This path gets hit from within the InsertAgent below, we don't want to interfere at that time
      return this;
    }

    SetManualSkeletonUpdate(false);
    phInst* result = fragInst::PrepareForActivation(collider, otherInst, constraint);
    Assert(result == this || result == NULL);
    if (!result)
      return NULL;

	// High LOD humans can copy the collision set stored in the NM manager
	if (GetType()->GetARTAssetID() >= 0 && GetCurrentPhysicsLOD() == fragInst::RAGDOLL_LOD_HIGH)
	{
		FRAGNMASSETMGR->AssignSelfCollisionSet(GetArticulatedCollider());
	}

	bool tryToActivateNM = !IsNMActivationBlocked() && GetARTAssetID() != -1 && FRAGNMASSETMGR->SuccessfullyLoaded(GetARTAssetID());

    bool NMAgentsAvailable = FRAGNMASSETMGR->GetAgentCount(0) > 0;
    if ( !(tryToActivateNM && NMAgentsAvailable) || GetCurrentPhysicsLOD() != RAGDOLL_LOD_HIGH )
    {
      SetIsSimulatingAsNonNMRagdoll(true);

      // Don't try to activate an NM agent, since the cap has been reached
      tryToActivateNM = false;
    }

    if (tryToActivateNM)
    {
      // Determine the random seed if one hasn't already been set
      if (m_randomSeed == 0)
        m_randomSeed = (u16) sysTimer::GetSystemMsTime();

      m_AgentId = FRAGNMASSETMGR->GetAgent(GetARTAssetID(), (phArticulatedCollider*) *collider, m_randomSeed);

      ART::NmRsCharacter* agent = ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId);

      // Make sure the agent has the expected number of parts:
      if( agent != NULL && 
          GetTypePhysics()->GetNumChildren() != agent->getNumberOfParts() )
      {
        Assertf(0, "The bounds for character type \"%s\" do not match the standard NaturalMotion setup.", type->GetBaseName());
        // Put our agent back in the pool for reuse
        FRAGNMASSETMGR->RecycleAgent(GetARTAssetID(), m_AgentId);
        m_AgentId = -1;

        // Set the asset ID to -1, to prevent this character from using NM
        // NOTE: I don't like the const cast.
        (const_cast<fragType*>(type))->SetARTAssetID(-1);
      }
    }

    tryToActivateNM = tryToActivateNM && (m_AgentId >= 0);

    if (tryToActivateNM)
    {
      m_BitStates |= FRAG_INSTANCE_NO_ARTIC_BODY;
    }

    if (result != NULL && tryToActivateNM)
    {
      m_BitStates &= ~FRAG_INSTANCE_NO_ARTIC_BODY;


      //Changed by RDR
      if (m_AgentId == -1)
      {
			Assertf(false, "Ragdoll in unknown state.");
			return fragInst::PrepareForActivation(collider, otherInst, constraint);
      }
      else
      {
        Assert(GetCached());

        phBoundComposite* bound = static_cast<phBoundComposite*>(GetArchetype()->GetBound());
        Assert(bound->GetType() == phBound::COMPOSITE);

        Matrix34* currentMatrices = FRAGNMASSETMGR->GetWorldCurrentMatrices(m_AgentId);
        Matrix34* lastMatrices = FRAGNMASSETMGR->GetWorldLastMatrices(m_AgentId);

        Matrix34 currMtx = RCC_MATRIX34(GetMatrix());
        Matrix34 lastMtx = RCC_MATRIX34(PHSIM->GetLastInstanceMatrix(this));

        for (int part = 0; part < GetTypePhysics()->GetNumChildren(); ++part)
        {
          currentMatrices[part].Dot(RCC_MATRIX34(bound->GetCurrentMatrix(part)), currMtx);
          currentMatrices[part].a.w = 0.0f;
          currentMatrices[part].b.w = 0.0f;
          currentMatrices[part].c.w = 0.0f;
          currentMatrices[part].d.w = 1.0f;
          lastMatrices[part].Dot(RCC_MATRIX34(bound->GetLastMatrix(part)), lastMtx);
          lastMatrices[part].a.w = 0.0f;
          lastMatrices[part].b.w = 0.0f;
          lastMatrices[part].c.w = 0.0f;
          lastMatrices[part].d.w = 1.0f;
        }

        int numChildren = GetTypePhysics()->GetNumChildren();

        ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
          currentMatrices,
          numChildren,
          ART::kITSDisable,
          ART::kITSourceCurrent);
        ART::gRockstarARTInstance->setComponentTMs(ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId),
          lastMatrices,
          numChildren,
          ART::kITSDisable,
          ART::kITSourcePrevious);

        FRAGNMASSETMGR->InsertAgent(m_AgentId);

        // Calculates ComValues
        // Performs velocity and angularVelocity clamping
        initialiseCharacter();
      }
    }
    else //RDR addition 
    {
      FRAGNMASSETMGR->RecycleAgent(GetARTAssetID(), m_AgentId);
      m_AgentId = -1;
    }

	phBoundComposite* compositeBound = static_cast<phBoundComposite*>(GetArchetype()->GetBound());
	// Zero out the positions in the composite last matrices, if requested
	if(IsZeroLastMatricesOnActivationNeeded())
	{
		Mat34V zeroMat;
		for(int i=0; i<compositeBound->GetNumBounds(); i++)
		{
			zeroMat = compositeBound->GetLastMatrix(i);
			zeroMat.SetCol3(Vec3V(V_ZERO));
			compositeBound->SetLastMatrix(i, zeroMat);
		}
		SetZeroLastMatricesOnActivation(false);
	}

	// Ensure that the composite is up to date with any changes made in PrepareForActivation
	compositeBound->CalculateCompositeExtents(true);
	if(IsInLevel())
	{
		if(compositeBound->HasBVHStructure())
		{
			PHLEVEL->RebuildCompositeBvh(GetLevelIndex());
		}
	}
	else
	{
		compositeBound->UpdateBvh(true);
	}

	// Allow extra penetration by default
	(*collider)->SetExtraAllowedPenetration(sm_ExtraAllowedRagdollPenetration);

	if (g_PreventRagdollPushCollisions)
	{
		g_EnablePreventsPushCollisions = true;
		(*collider)->PreventPushCollisions();
	}
	else
	{
		g_EnablePreventsPushCollisions = false;
	}

	return result;
}


  bool fragInstNM::PrepareForDeactivation(bool colliderManagedBySim, bool forceDeactivate) 
  {
    Assert(GetType());

    // Reset the random seed
    m_randomSeed = 0;

    SetManualSkeletonUpdate(true);

    if (GetARTAssetID() == -1 ||
      !FRAGNMASSETMGR->SuccessfullyLoaded(GetARTAssetID()) ||
      m_AgentId == -1)
    {
		Assert(m_AgentId == -1);
		SetIsSimulatingAsNonNMRagdoll(false);
		return fragInst::PrepareForDeactivation(colliderManagedBySim,forceDeactivate);
    }
    else
    {
      fragInst::PrepareForDeactivation(colliderManagedBySim,forceDeactivate);

      // Remove us from ART
      FRAGNMASSETMGR->RemoveAgent(m_AgentId);

      // Put our agent back in the pool for reuse
      FRAGNMASSETMGR->RecycleAgent(GetARTAssetID(), m_AgentId);

      m_AgentId = -1;
      if (GetCached())
      {
        PoseBoundsFromSkeleton(true, true);
      }
    }

    return true;
  }

//#if __DEV
//void fragInstNM::CheckVelocities(int checkpoint)
//{
//	if (GetCacheEntry() && GetCacheEntry()->GetHierInst() && GetCacheEntry()->GetHierInst()->body)
//	{
//		phArticulatedBody *body = GetCacheEntry()->GetHierInst()->body;
//		for (int iPart = 0; iPart < body->GetNumBodyParts(); iPart++)
//		{
//			Vec3V vLinVel = body->GetLink(iPart).GetLinearVelocity();
//			Vec3V vAngVel = body->GetLink(iPart).GetAngularVelocity();
//
//			Assertf(IsLessThanAll(Abs(vLinVel), Vec3V(V_FLT_LARGE_6)) &&
//				IsLessThanAll(Abs(vAngVel), Vec3V(V_FLT_LARGE_6)), 
//				"fragInstNM::CheckVelocities(%d) - Large velocities detected.  Linear velocity = %f, %f, %f.  Angular velocity = %f, %f, %f", checkpoint, 
//				vLinVel.GetXf(), vLinVel.GetYf(), vLinVel.GetZf(), 
//				vAngVel.GetXf(), vAngVel.GetYf(), vAngVel.GetZf());
//		}
//	}
//}
//#endif

void fragInstNM::SwitchFromNMToRageRagdoll(bool bResetEffectors)
{
	Assert(GetType());

	if (GetARTAssetID() >= 0 && m_AgentId >= 0)
	{
		// Remove us from ART
		FRAGNMASSETMGR->RemoveAgent(m_AgentId);

		// Put our agent back in the pool for reuse
		FRAGNMASSETMGR->RecycleAgent(GetARTAssetID(), m_AgentId);

		m_AgentId = -1;

		SetIsSimulatingAsNonNMRagdoll(true);

		GetCacheEntry()->GetHierInst()->body->ResetAllJointLimitAdjustments();

		if (bResetEffectors)
		{
			GetCacheEntry()->GetHierInst()->body->SetEffectorsToZeroPose();
			GetCacheEntry()->GetHierInst()->body->SetDriveState(phJoint::DRIVE_STATE_FREE);
		}
	}
}

// Maps between compoments different ragdoll LODs
u8 fragInstNM::ms_RagdollComponentHighToMedMap[] =
{
	0,  // pelivs to pelvis
	1,  // thigh_l to thigh_l
	2,  // calf_l to calf_l
	2,  // foot_l to calf_l
	3,  // thigh_r to thigh_r
	4,  // calf_r to calf_r
	4,  // foot_r to calf_r
	5,  // spine_Root to spine_Root
	6,  // spine1 to spine1
	7,  // spine2 to spine2
	8,  // spine3 to spine3
	9,  // clavicle_l to clavicle_l
	10,  // upperArm_l to upperArm_l
	11,  // forearm_l to forearm_l
	11,  // hand_l to forearm_l
	12,  // clavicle_r to clavicle_r
	13,  // upperArm_r to upperArm_r
	14,  // forearm_r to forearm_r
	14,  // hand_r to forearm_r
	15,  // neck to neck
	15   // head to neck
};
u8 fragInstNM::ms_RagdollComponentHighToLowMap[] =
{
	0,  // pelivs to pelvis
	1,  // thigh_l to thigh_l
	2,  // calf_l to calf_l
	2,  // foot_l to calf_l
	3,  // thigh_r to thigh_r
	4,  // calf_r to calf_r
	4,  // foot_r to calf_r
	5,  // spine_Root to spine_Root
	5,  // spine1 to spine_Root
	6,  // spine2 to spine3
	6,  // spine3 to spine3
	7,  // clavicle_l to upperArm_l
	7,  // upperArm_l to upperArm_l
	8,  // forearm_l to forearm_l
	8,  // hand_l to forearm_l
	9,  // clavicle_r to upperArm_r
	9,  // upperArm_r to upperArm_r
	10,  // forearm_r to forearm_r
	10,  // hand_r to forearm_r
	11,  // neck to neck
	11   // head to neck
};
u8 fragInstNM::ms_RagdollComponentMedToHighMap[] =
{
	0,  // pelivs to pelvis
	1,  // thigh_l to thigh_l
	2,  // calf_l to calf_l
	4,  // thigh_r to thigh_r
	5,  // calf_r to calf_r
	7,  // spine_Root to spine_Root
	8,  // spine1 to spine1
	9,  // spine2 to spine2
	10, // spine3 to spine3
	11, // clavicle_l to clavicle_l
	12, // upperArm_l to upperArm_l
	13, // forearm_l to forearm_l
	15, // clavicle_r to clavicle_r
	16, // upperArm_r to upperArm_r
	17, // forearm_r to forearm_r
	20  // neck to head
};
u8 fragInstNM::ms_RagdollComponentLowToHighMap[] =
{
	0,  // pelivs to pelvis
	1,  // thigh_l to thigh_l
	2,  // calf_l to calf_l
	4,  // thigh_r to thigh_r
	5,  // calf_r to calf_r
	7,  // spine_Root to spine_Root
	10, // spine3 to spine3
	12, // upperArm_l to upperArm_l
	13, // forearm_l to forearm_l
	16, // upperArm_r to upperArm_r
	17, // forearm_r to forearm_r
	20  // neck to head
};
int fragInstNM::MapRagdollLODComponentCurrentToHigh(int currentLODComponent) const
{
	// This map currently only applies to humans
	if (GetARTAssetID() < 0)
		return currentLODComponent;

	Assertf(currentLODComponent >= 0, "component is %d", currentLODComponent);
	if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_HIGH) // Nothing needs to be done if the current ragdoll LOD is HIGH
	{
		Assert(currentLODComponent < HIGH_RAGDOLL_LOD_NUM_COMPONENTS);
		return currentLODComponent;
	}
	else if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_MEDIUM)
	{
		Assert(currentLODComponent < MED_RAGDOLL_LOD_NUM_COMPONENTS);
		return ms_RagdollComponentMedToHighMap[currentLODComponent];
	}
	else  if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_LOW)
	{
		Assert(currentLODComponent < LOW_RAGDOLL_LOD_NUM_COMPONENTS);
		return ms_RagdollComponentLowToHighMap[currentLODComponent];
	}
	else
	{
		Assertf(0, "unknown ragdoll LOD");
		return currentLODComponent;
	}
}
int fragInstNM::MapRagdollLODComponentCurrentToHigh(int currentLODComponent, int currentLOD) 
{
	Assertf(currentLODComponent >= 0, "component is %d", currentLODComponent);
	if (currentLOD == RAGDOLL_LOD_HIGH) // Nothing needs to be done if the current ragdoll LOD is HIGH
	{
		Assert(currentLODComponent < HIGH_RAGDOLL_LOD_NUM_COMPONENTS);
		return currentLODComponent;
	}
	else if (currentLOD == RAGDOLL_LOD_MEDIUM)
	{
		Assert(currentLODComponent < MED_RAGDOLL_LOD_NUM_COMPONENTS);
		return ms_RagdollComponentMedToHighMap[currentLODComponent];
	}
	else  if (currentLOD == RAGDOLL_LOD_LOW)
	{
		Assert(currentLODComponent < LOW_RAGDOLL_LOD_NUM_COMPONENTS);
		return ms_RagdollComponentLowToHighMap[currentLODComponent];
	}
	else
	{
		Assertf(0, "unknown ragdoll LOD");
		return currentLODComponent;
	}
}
int fragInstNM::MapRagdollLODComponentHighToCurrent(int highLODComponent) const 
{
	// This map currently only applies to humans
	if (GetARTAssetID() < 0)
		return highLODComponent;

	Assertf(highLODComponent >= 0 && highLODComponent < HIGH_RAGDOLL_LOD_NUM_COMPONENTS, "component is %d", highLODComponent);
	if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_HIGH) // Nothing needs to be done if the current ragdoll LOD is HIGH
		return highLODComponent;
	else if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_MEDIUM)
		return ms_RagdollComponentHighToMedMap[highLODComponent];
	else if (GetCurrentPhysicsLOD() == RAGDOLL_LOD_LOW)
		return ms_RagdollComponentHighToLowMap[highLODComponent];
	else
	{
		Assertf(0, "unknown ragdoll LOD");
		return highLODComponent;
	}
}

  int fragInstNM::GetARTAssetID() const
  {
    if (const fragType* type = GetType())
    {
      return type->GetARTAssetID();
    }

    return -1;
  }

  void fragInstNM::SetIsSimulatingAsNonNMRagdoll( bool set )
  { 
    if (set)
    {
      Assert(!m_SimulatingAsNonNMRagdoll);
      FRAGNMASSETMGR->IncrementNonNMRagdollCount();
    }
    else
    {
      Assert(m_SimulatingAsNonNMRagdoll);
      FRAGNMASSETMGR->DecrementNonNMRagdollCount();
    }

    m_SimulatingAsNonNMRagdoll = set; 
  }

  void fragInstNM::initialiseCharacter()
  {
    if(m_AgentId >= 0)
    {
      ART::NmRsCharacter* character = ART::gRockstarARTInstance->getCharacterFromAgentID(m_AgentId);
      character->prepareForSimulation();
    }
  }

} // namespace rage
