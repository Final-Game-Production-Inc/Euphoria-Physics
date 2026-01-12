// 
// physics/forcesolverartdma.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#if __SPU

#include "spudma.h"

#include "system/dma.h"
#include "pharticulated/articulatedbody.h"
#include "pharticulated/bodypart.h"
#include "pharticulated/joint1dof.h"
#include "pharticulated/joint3dof.h"
#include "pharticulated/articulatedcollider.h"

namespace rage {

#define LOCAL_ALLOCATE_TTY 0

#if LOCAL_ALLOCATE_TTY
#define LOCAL_ALLOCATE_TTY_ONLY(X) X
#else
#define LOCAL_ALLOCATE_TTY_ONLY(X)
#endif

	//////////////////////////////////////////////////////////////////////////
	template<typename JointBaseT,     typename JointT,     typename LinkT,
		typename JointBaseTypeT,     typename JointTypeT>
		void ArticulatedDma<JointBaseT, JointT, LinkT, JointBaseTypeT, JointTypeT>::Init(u8* scratchPad, u32 scratchPadSize)
	{
		m_Scratchpad    = scratchPad;
		m_ScratchTop    = (u32)scratchPad;
		m_ScratchSize   = scratchPadSize;
		m_ScratchBottom = m_ScratchTop + scratchPadSize;
	}

	//////////////////////////////////////////////////////////////////////////

#define CHECK_LOCAL_ALLOC(X) \
	if ((X) == NULL) \
	{ \
	return false; \
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename JointBaseT,     typename JointT,     typename LinkT,
		typename JointBaseTypeT,   typename JointTypeT>
		bool ArticulatedDma<JointBaseT, JointT, LinkT, JointBaseTypeT, JointTypeT>::FetchToLs(phArticulatedCollider* collider, phArticulatedCollider* colliderCopy)
	{
		Assert(collider);

		phArticulatedBody* mmBody = collider->GetBody();

		if ((u32)mmBody > 0x40000) // If it's a MM address, it hasn't already been fixed up
		{
			// Body instance data (phArticulatedBody)
			phArticulatedBody* localBody = Allocate<phArticulatedBody>();
			sysDmaLargeGet(localBody, (uint64_t)mmBody, sizeof(phArticulatedBody), DMA_TAG(1));
			sysDmaWaitTagStatusAll(DMA_MASK(1));

			// Body type data (phArticulatedBodyType).  Have the instance data point to the type data.
			phArticulatedBodyType* localBodyType = colliderCopy ? colliderCopy->GetBody()->m_Type : Allocate<phArticulatedBodyType>();
			if (!colliderCopy)
				sysDmaLargeGet(localBodyType, (uint64_t)localBody->m_Type.ptr, sizeof(phArticulatedBodyType), DMA_TAG(1));
			m_BodyTypeMm = localBody->m_Type;
			localBody->m_Type = localBodyType;

			sysDmaWaitTagStatusAll(DMA_MASK(1));

			// Part instance data (phArticulatedBodyPart)
			int numParts = m_numLinks = localBody->GetNumBodyParts();
			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate phArticulatedBodyPart or phArticulatedBodyPart"));
			LinkT* localParts = Allocate<LinkT>(numParts);
			CHECK_LOCAL_ALLOC(localParts);

			int numJoints = numParts - 1;
			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate phJoint1Dof"));
			JointT* localJoints = Allocate<JointT>(numJoints);
			CHECK_LOCAL_ALLOC(localJoints);

			JointTypeT* localJointTypes = NULL;
			if (!colliderCopy)
			{
				LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate phJoint3DofType"));
				localJointTypes = Allocate<JointTypeT>(numJoints);
				CHECK_LOCAL_ALLOC(localJointTypes);
			}

			m_JointArrayMm        = Allocate<JointBaseT*>(numJoints);
			m_BodyPartArrayMm     = Allocate<LinkT*>(numParts);
			m_JointTypeArrayMm    = Allocate<JointBaseTypeT*>(numJoints);

			int maxNumJointDofs = collider->m_LimitsJointIndex.GetCount();

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate int"));
			int* limitsJointIndexArrays = Allocate<int>(maxNumJointDofs);
			CHECK_LOCAL_ALLOC(limitsJointIndexArrays);
			sysDmaLargeGet(limitsJointIndexArrays, (uint64_t)collider->m_LimitsJointIndex.GetElements(), sizeof(int) * maxNumJointDofs, DMA_TAG(1));
			m_LimitsJointIndexMm = collider->m_LimitsJointIndex.GetElements();
			collider->m_LimitsJointIndex.SetElements(limitsJointIndexArrays);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate int"));
			int* limitsDofIndexArrays = Allocate<int>(maxNumJointDofs);
			CHECK_LOCAL_ALLOC(limitsDofIndexArrays);
			sysDmaLargeGet(limitsDofIndexArrays, (uint64_t)collider->m_LimitsDofIndex.GetElements(), sizeof(int) * maxNumJointDofs, DMA_TAG(1));
			m_LimitsDofIndexMm = collider->m_LimitsDofIndex.GetElements();
			collider->m_LimitsDofIndex.SetElements(limitsDofIndexArrays);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate float"));
			float* limitsExcessHardArrays = Allocate<float>(maxNumJointDofs);
			CHECK_LOCAL_ALLOC(limitsExcessHardArrays);
			sysDmaLargeGet(limitsExcessHardArrays, (uint64_t)collider->m_LimitsExcessHard.GetElements(), sizeof(float) * maxNumJointDofs, DMA_TAG(1));
			m_LimitsExcessHardMm = collider->m_LimitsExcessHard.GetElements();
			collider->m_LimitsExcessHard.SetElements(limitsExcessHardArrays);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate float"));
			float* limitsResponseArrays = Allocate<float>(maxNumJointDofs);
			CHECK_LOCAL_ALLOC(limitsResponseArrays);
			sysDmaLargeGet(limitsResponseArrays, (uint64_t)collider->m_LimitResponse.GetElements(), sizeof(float) * maxNumJointDofs, DMA_TAG(1));
			m_LimitsResponseMm = collider->m_LimitResponse.GetElements();
			collider->m_LimitResponse.SetElements(limitsResponseArrays);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate float"));
			float* accumJointImpuseArrays = Allocate<float>(maxNumJointDofs);
			CHECK_LOCAL_ALLOC(accumJointImpuseArrays);
			sysDmaLargeGet(accumJointImpuseArrays, (uint64_t)collider->m_AccumJointImpulse.GetElements(), sizeof(float) * maxNumJointDofs, DMA_TAG(1));
			m_AccumJointImpulseMm = collider->m_AccumJointImpulse.GetElements();
			collider->m_AccumJointImpulse.SetElements(accumJointImpuseArrays);

			int maxNumBoundParts = collider->m_ComponentToLinkIndex.GetCapacity();

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate Vec3V"));
			Vec3V* accumSavedLinearVelocities = Allocate<Vec3V>(numParts);
			CHECK_LOCAL_ALLOC(accumSavedLinearVelocities);
			sysDmaLargeGet(accumSavedLinearVelocities, (uint64_t)collider->m_SavedLinearVelocities.GetElements(), sizeof(Vec3V) * numParts, DMA_TAG(1));
			m_SavedLinearVelocitiesMm = collider->m_SavedLinearVelocities.GetElements();
			collider->m_SavedLinearVelocities.SetElements(accumSavedLinearVelocities);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate Vec3V"));
			Vec3V* accumSavedAngularVelocities = Allocate<Vec3V>(numParts);
			CHECK_LOCAL_ALLOC(accumSavedAngularVelocities);
			sysDmaLargeGet(accumSavedAngularVelocities, (uint64_t)collider->m_SavedAngularVelocities.GetElements(), sizeof(Vec3V) * numParts, DMA_TAG(1));
			m_SavedAngularVelocitiesMm = collider->m_SavedAngularVelocities.GetElements();
			collider->m_SavedAngularVelocities.SetElements(accumSavedAngularVelocities);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate int"));
			int* componentToLinkIndexArrays = Allocate<int>(maxNumBoundParts);
			CHECK_LOCAL_ALLOC(componentToLinkIndexArrays);
			sysDmaLargeGet(componentToLinkIndexArrays, (uint64_t)collider->m_ComponentToLinkIndex.GetElements(), sizeof(int) * maxNumBoundParts, DMA_TAG(1));
			m_ComponentToLinkIndexMm = collider->m_ComponentToLinkIndex.GetElements();
			collider->m_ComponentToLinkIndex.SetElements(componentToLinkIndexArrays);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate phPhaseSpaceVector"));
			phPhaseSpaceVector* partVelocities = Allocate<phPhaseSpaceVector>(numParts);
			CHECK_LOCAL_ALLOC(partVelocities);
			sysDmaLargeGet(partVelocities, (uint64_t)localBody->m_PartVelocities.GetElements(), sizeof(phPhaseSpaceVector) * numParts, DMA_TAG(1));
			m_PartVelocitiesMm = localBody->m_PartVelocities.GetElements();
			localBody->m_PartVelocities.SetElements(partVelocities);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate phPhaseSpaceVector"));
			phPhaseSpaceVector* partIncrementalPartVelocitiesToPropUp = Allocate<phPhaseSpaceVector>(numParts);
			CHECK_LOCAL_ALLOC(partIncrementalPartVelocitiesToPropUp);
			sysDmaLargeGet(partIncrementalPartVelocitiesToPropUp, (uint64_t)localBody->m_VelocitiesToPropUp.GetElements(), sizeof(phPhaseSpaceVector) * numParts, DMA_TAG(1));
			m_VelocitiesToPropUpMm = localBody->m_VelocitiesToPropUp.GetElements();
			localBody->m_VelocitiesToPropUp.SetElements(partIncrementalPartVelocitiesToPropUp);

			LOCAL_ALLOCATE_TTY_ONLY(Displayf("Allocate Vec4V"));
			Vec4V* angInertiaXYZmassW = Allocate<Vec4V>(numParts);
			CHECK_LOCAL_ALLOC(angInertiaXYZmassW);
			sysDmaLargeGet(angInertiaXYZmassW, (uint64_t)localBody->m_AngInertiaXYZmassW.GetElements(), sizeof(Vec4V) * numParts, DMA_TAG(1));
			m_AngInertiaXYZmassWMm = localBody->m_AngInertiaXYZmassW.GetElements();
			localBody->m_AngInertiaXYZmassW.SetElements(angInertiaXYZmassW);

			m_Collider = collider;
			m_BodyMm   = collider->m_Body;
			collider->SetBody(localBody);

			for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
			{
				sysDmaLargeGet(localJoints + jointIndex, (uint64_t)localBody->GetJointAddr(jointIndex), sizeof(JointT), DMA_TAG(1));
				m_JointArrayMm[jointIndex] = localBody->GetJointAddr(jointIndex);
				localBody->FixUpJointPtr(jointIndex, localJoints + jointIndex);
			}

			for (int partIndex = 0; partIndex < numParts; ++partIndex)
			{
				sysDmaLargeGet(localParts + partIndex, (uint64_t)localBody->GetLinkAddr(partIndex), sizeof(LinkT), DMA_TAG(2));
			}

			sysDmaWaitTagStatusAll(DMA_MASK(1));

			// Store the MM type pointers
			for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
			{
				m_JointTypeArrayMm[jointIndex] = localJoints[jointIndex].m_Type;
			}

			if (!colliderCopy)
			{
				// Fetch the type data for links and joints
				for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
				{
					sysDmaLargeGet(localJointTypes + jointIndex, (uint64_t)localJoints[jointIndex].m_Type.ptr, sizeof(JointTypeT), DMA_TAG(1));
				}
			}

			sysDmaWaitTagStatusAll(DMA_MASK(1));

			// Set the LS type pointers
			if (colliderCopy)
			{
				phArticulatedBody *copyBody = colliderCopy->GetBody();
				for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
				{
					localJoints[jointIndex].m_Type = copyBody->GetJoint(jointIndex).m_Type;
					localBody->GetJoint(jointIndex).m_Type = copyBody->GetJoint(jointIndex).m_Type;
				}
			}
			else
			{
				for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
				{
					localJoints[jointIndex].m_Type = &localJointTypes[jointIndex];
					localBody->GetJoint(jointIndex).m_Type = &localJointTypes[jointIndex];
				}
			}

			sysDmaWaitTagStatusAll(DMA_MASK(2));

			// Finally we can update the local body with the correct pointers to the local links
			for (int partIndex = 0; partIndex < numParts; ++partIndex)
			{
				m_BodyPartArrayMm[partIndex] = localBody->GetLinkAddr(partIndex);
				localBody->FixUpLinkPtr(partIndex, localParts + partIndex);
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	const int sizeof_1DofClass = sizeof(phJoint1Dof);
	const int sizeof_3DofClass = sizeof(phJoint3Dof);
	const int sizeof_PrisClass = sizeof(phPrismaticJoint);

	//////////////////////////////////////////////////////////////////////////

	template<typename JointBaseT,     typename JointT,     typename LinkT,
		typename JointBaseTypeT,   typename JointTypeT>
		void ArticulatedDma<JointBaseT, JointT, LinkT, JointBaseTypeT,   JointTypeT>::WriteFromLs()
	{
		int maxNumJointDofs = m_Collider->m_LimitsJointIndex.GetCount();
		sysDmaLargePut(m_Collider->m_LimitsJointIndex.GetElements(), (uint64_t)m_LimitsJointIndexMm, sizeof(int) * maxNumJointDofs, DMA_TAG(1));
		m_Collider->m_LimitsJointIndex.SetElements(m_LimitsJointIndexMm);

		sysDmaLargePut(m_Collider->m_LimitsDofIndex.GetElements(), (uint64_t)m_LimitsDofIndexMm, sizeof(int) * maxNumJointDofs, DMA_TAG(1));
		m_Collider->m_LimitsDofIndex.SetElements(m_LimitsDofIndexMm);

		sysDmaLargePut(m_Collider->m_LimitsExcessHard.GetElements(), (uint64_t)m_LimitsExcessHardMm, sizeof(float) * maxNumJointDofs, DMA_TAG(1));
		m_Collider->m_LimitsExcessHard.SetElements(m_LimitsExcessHardMm);

		sysDmaLargePut(m_Collider->m_LimitResponse.GetElements(), (uint64_t)m_LimitsResponseMm, sizeof(float) * maxNumJointDofs, DMA_TAG(1));
		m_Collider->m_LimitResponse.SetElements(m_LimitsResponseMm);

		sysDmaLargePut(m_Collider->m_AccumJointImpulse.GetElements(), (uint64_t)m_AccumJointImpulseMm, sizeof(float) * maxNumJointDofs, DMA_TAG(1));
		m_Collider->m_AccumJointImpulse.SetElements(m_AccumJointImpulseMm);

		int maxNumBoundParts = m_Collider->m_ComponentToLinkIndex.GetCapacity();
		sysDmaLargePut(m_Collider->m_ComponentToLinkIndex.GetElements(), (uint64_t)m_ComponentToLinkIndexMm, sizeof(int) * maxNumBoundParts, DMA_TAG(1));
		m_Collider->m_ComponentToLinkIndex.SetElements(m_ComponentToLinkIndexMm);

		phArticulatedBody* localBody = m_Collider->GetBody();
		int numParts = localBody->GetNumBodyParts();
		int numJoints = numParts - 1;

		//sysDmaLargePut(m_Collider->m_SavedLinearVelocities.GetElements(), (uint64_t)m_SavedLinearVelocitiesMm, sizeof(Vec3V) * numParts, DMA_TAG(1));
		m_Collider->m_SavedLinearVelocities.SetElements(m_SavedLinearVelocitiesMm);

		//sysDmaLargePut(m_Collider->m_SavedAngularVelocities.GetElements(), (uint64_t)m_SavedAngularVelocitiesMm, sizeof(Vec3V) * numParts, DMA_TAG(1));
		m_Collider->m_SavedAngularVelocities.SetElements(m_SavedAngularVelocitiesMm);

		sysDmaLargePut(localBody->m_PartVelocities.GetElements(), (uint64_t)m_PartVelocitiesMm, sizeof(phPhaseSpaceVector) * numParts, DMA_TAG(1));
		localBody->m_PartVelocities.SetElements(m_PartVelocitiesMm);

		sysDmaLargePut(localBody->m_VelocitiesToPropUp.GetElements(), (uint64_t)m_VelocitiesToPropUpMm, sizeof(phPhaseSpaceVector) * numParts, DMA_TAG(1));
		localBody->m_VelocitiesToPropUp.SetElements(m_VelocitiesToPropUpMm);

		//sysDmaLargePut(localBody->m_AngInertiaXYZmassW.GetElements(), (uint64_t)m_AngInertiaXYZmassWMm, sizeof(Vec4V) * numParts, DMA_TAG(1));
		localBody->m_AngInertiaXYZmassW.SetElements(m_AngInertiaXYZmassWMm);

		for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex)
		{
			phJoint& localJoint = localBody->GetJoint(jointIndex);

			size_t jointSize;
			switch (localJoint.GetJointType())
			{
			case phJoint::JNT_1DOF:
				jointSize = sizeof_1DofClass;
				break;

			case phJoint::JNT_3DOF:
				jointSize = sizeof_3DofClass;
				break;

			default:
				Assert(0);
				// Intentional fall though
			case phJoint::PRISM_JNT:
				jointSize = sizeof_PrisClass;
				break;
			}

			// Need to set the saved joint type pointer back to the joint before writing to MM
			localBody->GetJointAddr(jointIndex)->m_Type = m_JointTypeArrayMm[jointIndex];
			localJoint.m_Type = m_JointTypeArrayMm[jointIndex];

			sysDmaLargePut(localBody->GetJointAddr(jointIndex), (uint64_t)m_JointArrayMm[jointIndex], jointSize, DMA_TAG(1));
			localBody->FixUpJointPtr(jointIndex, m_JointArrayMm[jointIndex]);
		}

		int numBodyParts = localBody->GetNumBodyParts();

		for (int partIndex = 0; partIndex < numBodyParts; ++partIndex)
		{
			sysDmaLargePut(localBody->GetLinkAddr(partIndex), (uint64_t)m_BodyPartArrayMm[partIndex], sizeof(LinkT), DMA_TAG(1));
			localBody->FixUpLinkPtr(partIndex, m_BodyPartArrayMm[partIndex]);
		}

		localBody->m_Type = m_BodyTypeMm;
		sysDmaLargePut(localBody, (uint64_t)m_BodyMm, sizeof(phArticulatedBody), DMA_TAG(1));
		m_Collider->SetBody(m_BodyMm);	

		sysDmaWaitTagStatusAll(DMA_MASK(1));
	}

	//////////////////////////////////////////////////////////////////////////
	template<typename JointBaseT,     typename JointT,     typename LinkT,
		typename JointBaseTypeT,   typename JointTypeT>
		template<typename _T> _T* ArticulatedDma<JointBaseT, JointT, LinkT, JointBaseTypeT,   JointTypeT>::Allocate(int count)
	{
		m_ScratchTop = (m_ScratchTop + 15) & ~15;
		u32 proposedNewTop = m_ScratchTop + sizeof(_T) * count;
		LOCAL_ALLOCATE_TTY_ONLY(Displayf("...%d item%s of size %d, total %d...", count, count > 1 ? "s" : "", (u32)sizeof(_T), (u32)sizeof(_T) * count));
		if(proposedNewTop <= m_ScratchBottom)
		{
			LOCAL_ALLOCATE_TTY_ONLY(Displayf("...succeeded newTop 0x%x bottom 0x%x", proposedNewTop, m_ScratchBottom));
			u32 newSpace = m_ScratchTop;
			m_ScratchTop = proposedNewTop;
			return (_T*)newSpace;
		}
		else
		{
			Errorf("ArticulatedDma::Allocate failed! proposed newTop = 0x%x  bottom = 0x%x numLinks = %d sractchSize = %d", 
				proposedNewTop, m_ScratchBottom, m_numLinks, m_ScratchSize);
			return NULL;
		}
	}


template class ArticulatedDma<phJoint,     phJoint3Dof,     phArticulatedBodyPart,
	phJointType,   phJoint3DofType>;


} // namespace rage

#endif // __SPU