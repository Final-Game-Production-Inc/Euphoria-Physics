/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "manifoldresult.h"

#include "manifold.h"
#include "simulator.h"

#if TRACK_COLLISION_TIME
#include "collider.h"
#endif

#include "phbound/support.h"
#include "phbullet/CollisionObject.h"

#include "vectormath/classes.h"

#define MIN_ELASTICITY 0.0f
#define MAX_ELASTICITY 0.9f

namespace rage {

#if __SPU
	void *phManifoldResult::s_PpuMaterialArray = NULL;
	u32 phManifoldResult::s_NumMaterials;
	u32 phManifoldResult::s_MaterialStride;
	u32 phManifoldResult::s_MaterialMask;
	const phMaterialPair* phManifoldResult::s_SpuMaterialOverridePairs = NULL;
	u32 phManifoldResult::s_NumMaterialOverridePairs;
#endif	// __SPU

	void phManifoldResult::AddContactPoint (Mat34V_In transformA, Mat34V_In transformB, rage::Vec::V3Param128 worldNormalOnB, 
		rage::Vec::V3Param128 pointOnAInLocal, rage::Vec::V3Param128 pointOnBInLocal,
		rage::Vec::V3Param128_After3Args separation, int elementA, int elementB TRACK_COLLISION_TIME_PARAM(float time))
	{

		// TODO: separation (and everything built from it) are most likely splatted. Assert(ScalarV(separation).IsValid()), and then pass along the ScalarV
		// instead of the Vec3V. IsLessThanOrEqualAll() is cheaper with a ScalarV, e.g.

		ScalarV v_separation(separation);

		if( IsGreaterThanAll(phManifold::GetManifoldMarginV(), v_separation) != 0 )
		{
			const Vec3V v_worldNormalOnB(worldNormalOnB);

			ScalarV depth = Negate(v_separation);
			const Vec3V localPointOnA = Vec3V(pointOnAInLocal);
			const Vec3V localPointOnB = Vec3V(pointOnBInLocal);
			phContact newPt(localPointOnA.GetIntrin128(), localPointOnB.GetIntrin128(), v_worldNormalOnB.GetIntrin128(), depth.GetIntrin128(), elementA, elementB);
			newPt.ActivateContact();

#if __ASSERT
			if( IsGreaterThanAll(Abs(Subtract(Mag(newPt.GetWorldNormal()), ScalarV(V_ONE))), ScalarVFromF32(POLYGONMAXNORMALERROR)) != 0)
			{
				m_Manifold->AssertContactNormalVerbose(newPt, __FILE__, __LINE__);
			}
#endif
			
			Assert(m_BoundA != NULL);
			phMaterialMgr::Id materialIdA = m_BoundA->GetMaterialIdFromPartIndex(elementA);
			Assert(m_BoundB);
			phMaterialMgr::Id materialIdB = m_BoundB->GetMaterialIdFromPartIndex(elementB);
			SetMaterialInformation(newPt, materialIdA, materialIdB);

			// This is not needed without alt update order -- though we may want it for debugging
#if TRACK_COLLISION_TIME
			// Update the collision time on the colliders
			if (phCollider* collider = m_Manifold->GetColliderA())
			{
				collider->DecreaseCollisionTimeSafe(time);
			}

			if (phCollider* collider = m_Manifold->GetColliderB())
			{
				collider->DecreaseCollisionTimeSafe(time);
			}
#endif

			int insertIndex = m_Manifold->GetClosestPointWithinMargin(newPt);
			if (insertIndex >= 0)
			{
				phContact& oldPoint = m_Manifold->GetContactPoint(insertIndex);

				// Keep the polygon number from the deeper of the two points. The transformation of the new contact
				// into world coordinates from bound local is necessary here because those stored in the contact
				// may correspond to the impact time found during CCD computations.
				Vec3V newWorldPosA = Transform(transformA, newPt.GetLocalPosA());
				Vec3V newWorldPosB = Transform(transformB, newPt.GetLocalPosB());
				ScalarV depthNoRewind = Dot(newWorldPosB - newWorldPosA, newPt.GetWorldNormal());
				if( IsLessThanOrEqualAll(depthNoRewind, oldPoint.GetDepthV()) != 0 )
				{
					newPt.CopyElements(oldPoint);
				}

				newPt.CopyPersistentInformation(oldPoint);
				m_Manifold->ReplaceContactPoint(newPt, insertIndex);
			}
			else
			{
				m_Manifold->AddManifoldPoint(newPt);
			}
		}
	}
	
	void phManifoldResult::AddContactPoint (QuatV_In orientationA, Vec3V_In positionA, QuatV_In orientationB, Vec3V_In positionB, rage::Vec::V3Param128 worldNormalOnB, 
		rage::Vec::V3Param128 pointOnAInLocal, rage::Vec::V3Param128 pointOnBInLocal,
		rage::Vec::V3Param128_After3Args separation, int elementA, int elementB TRACK_COLLISION_TIME_PARAM(float time))
	{

		// TODO: separation (and everything built from it) are most likely splatted. Assert(ScalarV(separation).IsValid()), and then pass along the ScalarV
		// instead of the Vec3V. IsLessThanOrEqualAll() is cheaper with a ScalarV, e.g.

		ScalarV v_separation(separation);

		if( IsGreaterThanAll(phManifold::GetManifoldMarginV(), v_separation) != 0 )
		{
			const Vec3V v_worldNormalOnB(worldNormalOnB);

			ScalarV depth = Negate(v_separation);
			Vec3V localPointOnA = Vec3V(pointOnAInLocal);
			Vec3V localPointOnB = Vec3V(pointOnBInLocal);
			phContact newPt(localPointOnA.GetIntrin128(), localPointOnB.GetIntrin128(), v_worldNormalOnB.GetIntrin128(), depth.GetIntrin128(), elementA, elementB);
			newPt.ActivateContact();			

			Assert(m_BoundA != NULL);
			phMaterialMgr::Id materialIdA = m_BoundA->GetMaterialIdFromPartIndex(elementA);
			Assert(m_BoundB);
			phMaterialMgr::Id materialIdB = m_BoundB->GetMaterialIdFromPartIndex(elementB);
			SetMaterialInformation(newPt, materialIdA, materialIdB);

#if TRACK_COLLISION_TIME
			// Update the collision time on the colliders
			if (phCollider* collider = m_Manifold->GetColliderA())
			{
				collider->DecreaseCollisionTimeSafe(time);
			}

			if (phCollider* collider = m_Manifold->GetColliderB())
			{
				collider->DecreaseCollisionTimeSafe(time);
			}
#endif

			int insertIndex = m_Manifold->GetClosestPointWithinMargin(newPt);
			if (insertIndex >= 0)
			{
				phContact& oldPoint = m_Manifold->GetContactPoint(insertIndex);

				// Keep the polygon number from the deeper of the two points. The transformation of the new contact
				// into world coordinates from bound local is necessary here because those stored in the contact
				// may correspond to the impact time found during CCD computations.
				Vec3V newWorldPosA = Transform(orientationA, positionA, newPt.GetLocalPosA());
				Vec3V newWorldPosB = Transform(orientationB, positionB, newPt.GetLocalPosB());
				ScalarV depthNoRewind = Dot(newWorldPosB - newWorldPosA, newPt.GetWorldNormal());
				if( IsLessThanOrEqualAll(depthNoRewind, oldPoint.GetDepthV()) != 0 )
				{
					newPt.CopyElements(oldPoint);
				}

				newPt.CopyPersistentInformation(oldPoint);
				m_Manifold->ReplaceContactPoint(newPt, insertIndex);
			}
			else
			{
				m_Manifold->AddManifoldPoint(newPt);
			}
		}
	}

	void phManifoldResult::SetMaterialInformation(phContact& contact, phMaterialMgr::Id materialIdA, phMaterialMgr::Id materialIdB)
	{
		contact.SetMaterialIds(materialIdA, materialIdB);

		float friction;
		float elasticity;

#if !__SPU
		MATERIALMGR.FindCombinedFrictionAndElasticity(materialIdA,materialIdB,friction,elasticity);
#else	// !__SPU
		Assert(s_PpuMaterialArray != NULL);
		Assert(s_SpuMaterialOverridePairs != NULL || s_NumMaterialOverridePairs == 0);

		// Get the index out of the Id
		phMaterialMgr::Id materialIndexA = materialIdA & s_MaterialMask;
		phMaterialMgr::Id materialIndexB = materialIdB & s_MaterialMask;

		rage::u8 materialBufferA[sizeof(rage::phMaterial)] ALIGNED(128);
		rage::u8 materialBufferB[sizeof(rage::phMaterial)] ALIGNED(128);
		Assert(materialIndexA < s_NumMaterials);
		Assert(materialIndexB < s_NumMaterials);

		// Material index A is always less than index B in the override pairs
		if(materialIndexA > materialIndexB)
		{
			SwapEm(materialIndexA, materialIndexB);
		}

		// Find a material override pair if it exists
		bool materialOverrideFound = false;
		for(u32 materialPairIndex = 0; materialPairIndex < s_NumMaterialOverridePairs; ++materialPairIndex)
		{
			const phMaterialPair& materialPair = s_SpuMaterialOverridePairs[materialPairIndex];
			if(materialPair.m_MaterialIndexA == materialIndexA && materialPair.m_MaterialIndexB == materialIndexB)
			{
				friction = materialPair.m_CombinedFriction;
				elasticity = materialPair.m_CombinedElasticity;
				materialOverrideFound = true;
				break;
			}
		}

		// If there was no material override, calculate it normally
		if(!materialOverrideFound)
		{
			const uint64_t materialAddrA = uint64_t(s_PpuMaterialArray) + (materialIndexA) * s_MaterialStride;
			sysDmaGet(materialBufferA, materialAddrA, sizeof(rage::phMaterial), DMA_TAG(1));
			const uint64_t materialAddrB = uint64_t(s_PpuMaterialArray) + (materialIndexB) * s_MaterialStride;
			sysDmaGet(materialBufferB, materialAddrB, sizeof(rage::phMaterial), DMA_TAG(1));
			sysDmaWaitTagStatusAll(DMA_MASK(1));

			const phMaterial &materialA = *(rage::phMaterial*)materialBufferA;
			const phMaterial &materialB = *(rage::phMaterial*)materialBufferB;

			friction = phMaterialMgr::CombineFriction(materialA.GetFriction(), materialB.GetFriction());
			elasticity = phMaterialMgr::CombineElasticity(materialA.GetElasticity(), materialB.GetElasticity());
		}
#endif // !__SPU

		contact.SetFriction(friction);
		contact.SetElasticity(elasticity);
	}


} // namespace rage
