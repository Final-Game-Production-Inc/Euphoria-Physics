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

//#include "fragment/instance.h"
#include "fragment/cache.h"
#include "fragment/typechild.h"
#include "fragment/drawable.h"
//#include "crskeleton/skeleton.h"

#include "NmRsCharacter.h"
#include "NmRsEffectors.h"
#include "NmRsEngine.h"
//#include "NmRsPhInstNM.h"
#include "NmRsGenericPart.h"
#include "NmRsLimbs.h"

namespace ART
{
	bool NmRsCharacter::setupCharacter(rage::phArticulatedCollider *inputCollider)
	{
		Assert(inputCollider);

		int numParts = inputCollider->GetBody()->GetNumBodyParts(); 
		int numJoints = inputCollider->GetBody()->GetNumJoints(); 
		m_effectorCount = numJoints;
		m_1dofCount = m_rsEngine->m_characterTypeData[getAssetID()].m_Num1DofJoints;
		m_3dofCount = m_rsEngine->m_characterTypeData[getAssetID()].m_Num3DofJoints;;

		// create a new articulated wrapper to manage the parts
		ARTCustomPlacementNew2Arg(m_articulatedWrapper, NmRsArticulatedWrapper, numParts, inputCollider);
		rage::phArticulatedCollider *articulatedCollider = getArticulatedWrapper()->getArticulatedCollider();
		rage::phArticulatedBody *articulatedBody = getArticulatedWrapper()->getArticulatedBody();

		// Create component to bone matrices if not already done so for this character type
		if (m_rsEngine->m_characterTypeData[getAssetID()].m_ComponentToBoneMatrices == NULL)
		{
			m_rsEngine->m_characterTypeData[getAssetID()].m_ComponentToBoneMatrices = 
				(rage::Matrix34*)m_artMemoryManager->callocate(sizeof(rage::Matrix34) * numParts, NM_MEMORY_TRACKING_ARGS);
			rage::fragInst *pFragInst = (rage::fragInst*)articulatedCollider->GetInstance(); 
			rage::fragTypeChild* child = NULL;

			for (int pIndex = 0; pIndex < numParts; ++pIndex)
			{
				child = pFragInst->GetTypePhysics()->GetAllChildren()[pIndex];
        rage::Matrix34 currentMatrix = child->GetUndamagedEntity()->GetBoundMatrix();
        currentMatrix.Inverse();
        m_rsEngine->m_characterTypeData[getAssetID()].m_ComponentToBoneMatrices[pIndex] = currentMatrix;
			}
		}

		if (!strncmp(getIdentifier(), "Fred", 4)) // encompasses "FredMin" too
		{
			m_bodyIdent = gtaFred;
			m_zUp = true;
		}
		else if (!strncmp(getIdentifier(), "FrLarge", 7)) // encompasses "FredMin" too
		{
			m_bodyIdent = gtaFredLarge;
			m_zUp = true;
		}
		else if (!strncmp(getIdentifier(), "Wilma", 5))
		{
			m_bodyIdent = gtaWilma;
			m_zUp = true;
		}
		else if (!strncmp(getIdentifier(), "Cowboy", 6))
		{
			m_bodyIdent = rdrCowboy;
			m_zUp = false;
		}
		else if (!strncmp(getIdentifier(), "Cowgirl", 7))
		{
			m_bodyIdent = rdrCowgirl;
			m_zUp = false;
		}
		else if (!strncmp(getIdentifier(), "MPMed", 5))
		{
			m_bodyIdent = mp3Medium;
			m_zUp = true;
		}
		else if (!strncmp(getIdentifier(), "MPLarge", 5))
		{
			m_bodyIdent = mp3Large;
			m_zUp = true;
		}
		else
		{
			// just some scenery in Studio
			m_bodyIdent = notSpecified;
			m_zUp = true;
		}

		// restate our assumptions
		if (m_bodyIdent == gtaFred || 
			m_bodyIdent == gtaFredLarge || 
			m_bodyIdent == gtaWilma||
			m_bodyIdent == mp3Large||
			m_bodyIdent == mp3Medium)
		{
			Assert(numParts == gta_TotalHumanParts);
			Assert(m_effectorCount == gta_TotalHumanEffectors);
		}
		else if (m_bodyIdent == rdrCowboy || m_bodyIdent == rdrCowgirl)
		{
			Assert(numParts == rdr_TotalHumanParts);
			Assert(m_effectorCount == rdr_TotalHumanEffectors);
		}
    else
      Assert(false); // removed non-biped support for the moment.

		m_genericPartCount = numParts;

		allocateStorage();

		if (numParts > 0)
		{
			int dof1Index = 0, dof3Index = 0;

			for (int pIndex = 0; pIndex < numParts; ++pIndex)
			{
#if __DEV
				int parentIndex = 0;
				if (inputCollider->GetBody()->GetTypeOfJoint(pIndex) == 0)
					parentIndex = m_rsEngine->m_characterTypeData[getAssetID()].m_1DofEffectorParams->parentIndex;
				else
					parentIndex = m_rsEngine->m_characterTypeData[getAssetID()].m_3DofEffectorParams->parentIndex;
#endif
				rage::phArticulatedBodyPart* articBodyPart = const_cast<rage::phArticulatedBodyPart*> (&articulatedBody->GetLink(pIndex));

				// register into the clump against component index
				NmRsGenericPart* generic_part = addArticulated(articBodyPart, pIndex);
				generic_part->setCollisionEnabled(true);

#if ART_ENABLE_BSPY
				generic_part->setNameToken( m_rsEngine->m_characterTypeData[getAssetID()].m_PartTokens[pIndex] );
#endif // ART_ENABLE_BSPY

				// Set the initial matrix
				generic_part->setInitialMatrix(m_rsEngine->m_characterTypeData[getAssetID()].m_InitialMatrices[pIndex], true);

				// Set the component to bone matrix
				generic_part->setPartToBoneMatrix(m_rsEngine->m_characterTypeData[getAssetID()].m_ComponentToBoneMatrices[pIndex]);

				if (pIndex > 0)
				{
					int jointIndex = pIndex - 1;
					ART::NmRsEngine *engine = ART::gRockstarARTInstance;
					rage::phJoint *jointToParent = &articulatedBody->GetJoint(jointIndex);
					Assert(jointIndex == dof1Index + dof3Index);
					NmRsEffectorBase* effInst = NULL;
					if (jointToParent->GetJointType() == rage::phJoint::JNT_1DOF)
					{
						NmRs1DofEffectorParams &info = engine->m_characterTypeData[getAssetID()].m_1DofEffectorParams[dof1Index++];
						effInst = add1DofEffector(static_cast<rage::phJoint1Dof*>(jointToParent), jointIndex, dof1Index-1, info);
					}
					else 
					{
						Assert(jointToParent->GetJointType() == rage::phJoint::JNT_3DOF);
						NmRs3DofEffectorParams &info = engine->m_characterTypeData[getAssetID()].m_3DofEffectorParams[dof3Index++];
						effInst = add3DofEffector(static_cast<rage::phJoint3Dof*>(jointToParent), jointIndex, dof3Index-1, info);
					}
					(void)effInst;
#if ART_ENABLE_BSPY
					effInst->setNameToken( m_rsEngine->m_characterTypeData[getAssetID()].m_JointTokens[jointIndex] );
#endif // ART_ENABLE_BSPY
				}

				// add to the lookups
				Assert(m_parts[parentIndex]);
				Assert(&articulatedBody->GetLink(pIndex) == articBodyPart); // we're passing in the wrong indices to deserializePhJoint if these are not equal
				Assert(&articulatedBody->GetLink(parentIndex) == (rage::phArticulatedBodyPart*)m_parts[parentIndex]->getDataPtr()); // we're passing in the wrong indices to deserializePhJoint if these are not equal
			}

			// an archetype to hold all bounds + total mass from all parts
			rage::phArchetypePhys *allPartArchetype;
			allPartArchetype = (rage::phArchetypePhys *) articulatedCollider->GetInstance()->GetArchetype();

			rage::phInst *allPartInstance = articulatedCollider->GetInstance();
			m_articulatedWrapper->setInitialMatrix(RCC_MATRIX34(articulatedCollider->GetInstance()->GetMatrix()));

			m_articulatedWrapper->setArchetype(allPartArchetype);
			m_articulatedWrapper->setArticulatedPhysInstance(allPartInstance, false);

			initMaxAngSpeed();

			m_articulatedWrapper->postDeserialize();
		}

		// !hdd! i'm willing to admit that the following code isn't very nice, although
		// what it's doing is very simple... its just not using maps or hashes or owt.
		//
		// this does what the deserialized part/effector mapping tables did in lua.
		// but just a load more ugly.
	  {
#define NM_RS_IDMAP(name) partMapGTA[name] = gta##name; partMapRDR[name] = rdr##name;
		  int partMapGTA[TotalKnownParts]; int partMapRDR[TotalKnownParts];
		  NM_RS_IDMAP(Buttocks);
		  NM_RS_IDMAP(Thigh_Left);
		  NM_RS_IDMAP(Shin_Left);
		  NM_RS_IDMAP(Foot_Left);
		  NM_RS_IDMAP(Thigh_Right);
		  NM_RS_IDMAP(Shin_Right);
		  NM_RS_IDMAP(Foot_Right);
		  NM_RS_IDMAP(Spine0);
		  NM_RS_IDMAP(Spine1);
		  NM_RS_IDMAP(Spine2);
		  NM_RS_IDMAP(Spine3);
		  NM_RS_IDMAP(Neck);
		  NM_RS_IDMAP(Head);
		  NM_RS_IDMAP(Clav_Left);
		  NM_RS_IDMAP(Upper_Arm_Left);
		  NM_RS_IDMAP(Lower_Arm_Left);
		  NM_RS_IDMAP(Hand_Left);
		  NM_RS_IDMAP(Clav_Right);
		  NM_RS_IDMAP(Upper_Arm_Right);
		  NM_RS_IDMAP(Lower_Arm_Right);
		  NM_RS_IDMAP(Hand_Right);
  #undef NM_RS_IDMAP
  #define NM_RS_IDMAP(name) effMapGTA[name] = gtaJt##name; effMapRDR[name] = rdrJt##name;
		  int effMapGTA[TotalKnownHumanEffectors]; int effMapRDR[TotalKnownHumanEffectors];
		  NM_RS_IDMAP(Hip_Left);
		  NM_RS_IDMAP(Knee_Left);
		  NM_RS_IDMAP(Ankle_Left);
		  NM_RS_IDMAP(Hip_Right);
		  NM_RS_IDMAP(Knee_Right);
		  NM_RS_IDMAP(Ankle_Right);
		  NM_RS_IDMAP(Spine_0);
		  NM_RS_IDMAP(Spine_1);
		  NM_RS_IDMAP(Spine_2);
		  NM_RS_IDMAP(Spine_3);
		  NM_RS_IDMAP(Neck_Lower);
		  NM_RS_IDMAP(Neck_Upper);
		  NM_RS_IDMAP(Clav_Jnt_Left);
		  NM_RS_IDMAP(Shoulder_Left);
		  NM_RS_IDMAP(Elbow_Left);
		  NM_RS_IDMAP(Wrist_Left);
		  NM_RS_IDMAP(Clav_Jnt_Right);
		  NM_RS_IDMAP(Shoulder_Right);
		  NM_RS_IDMAP(Elbow_Right);
		  NM_RS_IDMAP(Wrist_Right);
  #undef NM_RS_IDMAP

		  int *partMap = 0, *effMap = 0;
		  if (m_bodyIdent == gtaFred || m_bodyIdent == gtaFredLarge || m_bodyIdent == gtaWilma || m_bodyIdent == mp3Medium || m_bodyIdent == mp3Large)
		  {
			  partMap = partMapGTA;
			  effMap = effMapGTA;
		  }
		  else if (m_bodyIdent == rdrCowboy || m_bodyIdent == rdrCowgirl)
		  {
			  partMap = partMapRDR;
			  effMap = effMapRDR;
		  }
		  else
		  {
			  return true;
		  }

      m_body.init(this);

      m_body.addHumanArm(
        kLeftArm,
        getEffector(effMap[Clav_Jnt_Left]),
        getEffector(effMap[Shoulder_Left]),
        getEffector(effMap[Elbow_Left]),
        getEffector(effMap[Wrist_Left]),
        getGenericPartByIndex(partMap[Spine3]),
        getGenericPartByIndex(partMap[Clav_Left]), 
        getGenericPartByIndex(partMap[Upper_Arm_Left]), 
        getGenericPartByIndex(partMap[Lower_Arm_Left]), 
        getGenericPartByIndex(partMap[Hand_Left]),
        1.f, 1.f, false);

      m_body.addHumanArm(
        kRightArm,
        getEffector(effMap[Clav_Jnt_Right]),
        getEffector(effMap[Shoulder_Right]),
        getEffector(effMap[Elbow_Right]),
        getEffector(effMap[Wrist_Right]),
        getGenericPartByIndex(partMap[Spine3]),
        getGenericPartByIndex(partMap[Clav_Right]), 
        getGenericPartByIndex(partMap[Upper_Arm_Right]), 
        getGenericPartByIndex(partMap[Lower_Arm_Right]), 
        getGenericPartByIndex(partMap[Hand_Right]),
        -1.f, 1.f, false);

      m_body.addHumanLeg(
        kLeftLeg,
        getEffector(effMap[Hip_Left]),
        getEffector(effMap[Knee_Left]),
        getEffector(effMap[Ankle_Left]),
        getGenericPartByIndex(partMap[Buttocks]),
        getGenericPartByIndex(partMap[Thigh_Left]), 
        getGenericPartByIndex(partMap[Shin_Left]), 
        getGenericPartByIndex(partMap[Foot_Left]), 
        1.f, -1.f, false);

      m_body.addHumanLeg(
        kRightLeg,
        getEffector(effMap[Hip_Right]),
        getEffector(effMap[Knee_Right]),
        getEffector(effMap[Ankle_Right]),
        getGenericPartByIndex(partMap[Buttocks]),
        getGenericPartByIndex(partMap[Thigh_Right]), 
        getGenericPartByIndex(partMap[Shin_Right]), 
        getGenericPartByIndex(partMap[Foot_Right]), 
        -1.f, -1.f, false);

      m_body.addHumanSpine(
        kSpine,
        getEffector(effMap[Spine_0]),
        getEffector(effMap[Spine_1]),
        getEffector(effMap[Spine_2]),
        getEffector(effMap[Spine_3]),
        getEffector(effMap[Neck_Lower]),
        getEffector(effMap[Neck_Upper]),
        getGenericPartByIndex(partMap[Buttocks]),
        getGenericPartByIndex(partMap[Spine0]), 
        getGenericPartByIndex(partMap[Spine1]), 
        getGenericPartByIndex(partMap[Spine2]), 
        getGenericPartByIndex(partMap[Spine3]), 
        getGenericPartByIndex(partMap[Neck]), 
        getGenericPartByIndex(partMap[Head]));
    }

    m_WorldCurrentMatrices = rage_new rage::Matrix34[m_genericPartCount];
    m_WorldLastMatrices = rage_new rage::Matrix34[m_genericPartCount];
#if NM_ANIM_MATRICES
    m_BlendOutAnimationMatrices = rage_new rage::Matrix34[m_genericPartCount];
#endif

    // phase 2 todo. are these necessary? shouldn't these be left at defaults?
    resetEffectorsToDefaults();
    setBodyStiffness(5.0f, 0.5f);

    return true;
  }
}//namespace ART
