// 
// physics/iterator.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#include "iterator.h"

#include "inst.h"
#include "grprofile/drawmanager.h"
#include "vectormath/classes.h"


#define			FAST_ITERATOR		1


namespace rage {

bool phIterator::CheckInstance(Vector3::Vector3Param sphereCenter, Vector3::Vector3Param sphereRadius, u32 typeFlags, u32 includeFlags, u32 stateFlag) const
{
	Assert(!m_CullAgainstInstanceAABBs);

#if FAST_ITERATOR
	ScalarV sphRadiusV( sphereRadius );
#else
	float sphRadius = Vector3(sphereRadius).x;
#endif
	const bool typeIncludeFlagsPassed = m_SkipTypeIncludeFlagsTests || (phLevelNew::MatchFlags(typeFlags, m_IncludeFlags, m_TypeFlags, includeFlags) && ((typeFlags & m_TypeExcludeFlags) == 0));
	if(!typeIncludeFlagsPassed)
	{
		return false;
	}
	if((stateFlag & m_StateIncludeFlags) == 0)
	{
		return false;
	}

	return m_CullShape.CheckSphere(RCC_VEC3V(sphereCenter), sphRadiusV);
}


bool phIterator::CheckInstance(const phInst* RESTRICT inst, u32 typeFlags, u32 includeFlags, u32 stateFlag) const
{
	if(!m_CullAgainstInstanceAABBs)
	{
		Mat34V temp = inst->GetMatrix();
		const Vector3 center = VEC3V_TO_VECTOR3(inst->GetArchetype()->GetBound()->GetWorldCentroid( temp ));
		const Vector3 radius = SCALARV_TO_VECTOR3(inst->GetArchetype()->GetBound()->GetRadiusAroundCentroidV());

		return CheckInstance(center, radius, typeFlags, includeFlags, stateFlag);
	}
	else
	{
		const bool typeIncludeFlagsPassed = m_SkipTypeIncludeFlagsTests || (phLevelNew::MatchFlags(typeFlags, m_IncludeFlags, m_TypeFlags, includeFlags) && ((typeFlags & m_TypeExcludeFlags) == 0));
		if(!typeIncludeFlagsPassed)
		{
			return false;
		}
		if((stateFlag & m_StateIncludeFlags) == 0)
		{
			return false;
		}

		// Get the AABB for the OBB of the instance.
		Vec3V obbExtents, obbCenter;
		inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(obbExtents, obbCenter);
		Mat34V_ConstRef tempMat = inst->GetMatrix();
		const Vec3V aabbExtents = geomBoxes::ComputeAABBExtentsFromOBB(tempMat.GetMat33ConstRef(), obbExtents);
		const Vec3V aabbCenter = Transform(tempMat, obbCenter);

		return m_CullShape.CheckAABB(aabbCenter, aabbExtents);
	}
}


void phIterator::Clear()
{
#if __SPU || EMULATE_SPU
	m_InstanceBuffer = NULL;
	m_ArchetypeBuffer = NULL;
	m_BoundBuffer = NULL;

	m_InstanceBufferSize = 0;
	m_ArchetypeBufferSize = 0;
	m_BoundBufferSize = 0;
#endif
}

} // namespace rage
