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


#ifndef MANIFOLD_RESULT_H
#define MANIFOLD_RESULT_H


#include "phbullet/DiscreteCollisionDetectorInterface.h"
#include "phcore/materialmgr.h" // phMaterialMgr::Id

namespace rage {

class phBound;
class phContact;
class phManifold;
class Matrix34;

class phManifoldResult : public DiscreteCollisionDetectorInterface::ResultProcessor
{
public:
	phManifoldResult ()
	: m_ReverseInManifold(0)
	{}
	phManifoldResult (const phBound* boundA, const phBound* boundB, phManifold* manifold);

	void Set (const phBound* boundA, const phBound* boundB, phManifold* manifoldPtr);
	void SetManifold (phManifold* manifold);
	phManifold * GetManifold();
	void AddContactPoint(Mat34V_In transformA, Mat34V_In transformB, rage::Vec::V3Param128 normalOnBInWorld,
		rage::Vec::V3Param128 pointOnAInLocal,rage::Vec::V3Param128 pointOnBInLocal,
		rage::Vec::V3Param128_After3Args separation,int elementA,int elementB TRACK_COLLISION_TIME_PARAM(float time));
	// TODO: I don't like that there are two versions of this function, just taking different parameters at the beginning.  I also don't like the way that they
	//   are using those parameters to recompute world positions and check depth, etc, etc.
	void AddContactPoint(QuatV_In orientationA, Vec3V_In positionA, QuatV_In orientationB, Vec3V_In positionB, rage::Vec::V3Param128 normalOnBInWorld,
		rage::Vec::V3Param128 pointOnAInLocal,rage::Vec::V3Param128 pointOnBInLocal,
		rage::Vec::V3Param128_After3Args separation,int elementA,int elementB TRACK_COLLISION_TIME_PARAM(float time));

	static void SetMaterialInformation(phContact& contact, phMaterialMgr::Id materialIdA, phMaterialMgr::Id materialIdB);

	void ProcessResult_NoSwap(Mat34V_In transformA, Mat34V_In transformB, const DiscreteCollisionDetectorInterface::SimpleResult &collisionDetectionResult)
	{
		FastAssert(m_ReverseInManifold == 0);
		const DiscreteCollisionDetectorInterface::SimpleResult &r = collisionDetectionResult;
		AddContactPoint(transformA, transformB, r.GetNormalOnBInWorld().GetIntrin128(), 
						r.GetPointOnAInLocal().GetIntrin128(), r.GetPointOnBInLocal().GetIntrin128(),
						r.GetDistanceV().GetIntrin128(), r.GetElementA(), r.GetElementB() TRACK_COLLISION_TIME_PARAM(r.GetTime()));
	}

	void ProcessResult(Mat34V_In transformA, Mat34V_In transformB, const DiscreteCollisionDetectorInterface::SimpleResult &collisionDetectionResult)
	{
		// Create an alias just to make the AddContactPoint() line a little more readable.
		const DiscreteCollisionDetectorInterface::SimpleResult &r = collisionDetectionResult;
		const BoolV vSelector(ScalarVFromU32(m_ReverseInManifold).GetIntrin128());
		const u32 iSelector = m_ReverseInManifold;
		const Vec3V normalForManifold = SelectFT(vSelector, r.GetNormalOnBInWorld(), Negate(r.GetNormalOnBInWorld()));

		const Vec3V pointAForManifoldLocal = SelectFT(vSelector, r.GetPointOnAInLocal(), r.GetPointOnBInLocal());
		const Vec3V pointBForManifoldLocal = SelectFT(vSelector, r.GetPointOnBInLocal(), r.GetPointOnAInLocal());
		const u32 elementAForManifold = (~iSelector & r.GetElementA()) | (iSelector & r.GetElementB());
		const u32 elementBForManifold = (~iSelector & r.GetElementB()) | (iSelector & r.GetElementA());
		AddContactPoint(transformA, transformB, normalForManifold.GetIntrin128(), 
			pointAForManifoldLocal.GetIntrin128(), pointBForManifoldLocal.GetIntrin128(),
			r.GetDistanceV().GetIntrin128(), elementAForManifold, elementBForManifold TRACK_COLLISION_TIME_PARAM(r.GetTime()));
	}

	void ProcessResult(QuatV_In orientationA, Vec3V_In positionA, QuatV_In orientationB, Vec3V_In positionB, const DiscreteCollisionDetectorInterface::SimpleResult &collisionDetectionResult)
	{
		// Create an alias just to make the AddContactPoint() line a little more readable.
		const DiscreteCollisionDetectorInterface::SimpleResult &r = collisionDetectionResult;
		const BoolV vSelector(ScalarVFromU32(m_ReverseInManifold).GetIntrin128());
		const u32 iSelector = m_ReverseInManifold;
		const Vec3V normalForManifold = SelectFT(vSelector, r.GetNormalOnBInWorld(), Negate(r.GetNormalOnBInWorld()));

		const Vec3V pointAForManifoldLocal = SelectFT(vSelector, r.GetPointOnAInLocal(), r.GetPointOnBInLocal());
		const Vec3V pointBForManifoldLocal = SelectFT(vSelector, r.GetPointOnBInLocal(), r.GetPointOnAInLocal());
		const u32 elementAForManifold = (~iSelector & r.GetElementA()) | (iSelector & r.GetElementB());
		const u32 elementBForManifold = (~iSelector & r.GetElementB()) | (iSelector & r.GetElementA());
		AddContactPoint(orientationA, positionA, orientationB, positionB, normalForManifold.GetIntrin128(), 
			pointAForManifoldLocal.GetIntrin128(), pointBForManifoldLocal.GetIntrin128(),
			r.GetDistanceV().GetIntrin128(), elementAForManifold, elementBForManifold TRACK_COLLISION_TIME_PARAM(r.GetTime()));
	}

	void SetReverseInManifold(u32 reverseInManifold)
	{
		FastAssert(reverseInManifold == 0 || reverseInManifold == (u32)(-1));
		m_ReverseInManifold = reverseInManifold;
	}

#if __SPU
	static void SetMaterialData(void* ppuMaterialArray, u32 numMaterials, u32 materialStride, u32 materialMask, const phMaterialPair* SpuMaterialOverridePairs, u32 numMaterialOverridePairs)
	{
		s_PpuMaterialArray = ppuMaterialArray;
		s_NumMaterials = numMaterials;
		s_MaterialStride = materialStride;
		s_MaterialMask = materialMask;
		s_SpuMaterialOverridePairs = SpuMaterialOverridePairs;
		s_NumMaterialOverridePairs = numMaterialOverridePairs;
	}
#endif	// __SPU

private:
    phManifold* m_Manifold;
	const phBound* m_BoundA;
	const phBound* m_BoundB;
	u32 m_ReverseInManifold;	// Needs to be all bits set or all bit clear.
#if __SPU
	static void *s_PpuMaterialArray;
	static u32 s_NumMaterials;
	static u32 s_MaterialStride;
	static u32 s_MaterialMask;
	static const phMaterialPair* s_SpuMaterialOverridePairs;
	static u32 s_NumMaterialOverridePairs;
#endif	// __SPU
};

inline phManifoldResult::phManifoldResult(const phBound* boundA, const phBound* boundB, phManifold* manifold)
	: m_Manifold(manifold)
	, m_BoundA(boundA)
	, m_BoundB(boundB)
	, m_ReverseInManifold(0)
{
}

inline void phManifoldResult::Set (const phBound* boundA, const phBound* boundB, phManifold* manifold)
{
	m_Manifold = manifold;
	m_BoundA = boundA;
	m_BoundB = boundB;
}

inline void phManifoldResult::SetManifold (phManifold* manifold)
{
	m_Manifold = manifold;
}

inline phManifold * phManifoldResult::GetManifold()
{
	return m_Manifold;
}

} // namespace rage

#endif //MANIFOLD_RESULT_H
