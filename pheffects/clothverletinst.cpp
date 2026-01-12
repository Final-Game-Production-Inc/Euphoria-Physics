//
// pheffects/clothverletinst.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//


#include "clothverletinst.h"

#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundsphere.h"
#include "phbound/support.h"
#include "physics/collider.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "physics/sleep.h"
#include "profile/page.h"
#include "profile/group.h"
#include "profile/element.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "data/safestruct.h"

#include "system/taskwaitthread.h"
#include "vectormath/classes.h"

//#pragma optimize("", off)

namespace rage {

namespace phClothStats
{
	EXT_PF_TIMER(EnvClothPostSimUpdate);
}
using namespace phClothStats;



phClothVerletBehavior::phClothVerletBehavior() 
	: m_ActivateOnHit(false)
	, m_ActivateOnHitOverridden(0)
	, m_UserDataAddress(0)
{
	m_CollisionInst.Reserve( CLOTH_MAX_COLLISION_OBJECTS );
	m_CollisionInst.Resize(0);
}

phClothVerletBehavior::~phClothVerletBehavior()
{
	bool isInSimulator = PHSIM->IsInstBehaviorInArray(this);
	
	Assertf(!isInSimulator, "Inst behaviour is being deleted while it is still in the simulator inst behaviour array. The behaviour will be removed from the simulator now, but this shouldn't be happening!");
	
	if(isInSimulator)
		PHSIM->RemoveInstBehavior(*this);
}

phClothVerletBehavior::phClothVerletBehavior(class datResource& rsc) 
	: phInstBehavior(rsc)
	, m_CollisionInst(rsc)
{
}

IMPLEMENT_PLACE( phClothVerletBehavior );

#if __DECLARESTRUCT
void phClothVerletBehavior::DeclareStruct(datTypeStruct &s)
{
	phInstBehavior::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(phClothVerletBehavior, phInstBehavior)
		SSTRUCT_FIELD(phClothVerletBehavior, m_UserDataAddress)
		SSTRUCT_FIELD(phClothVerletBehavior, m_CollisionInst)		
		SSTRUCT_CONTAINED_ARRAY(phClothVerletBehavior, m_padding1)
		SSTRUCT_FIELD(phClothVerletBehavior, m_ActivateOnHit)
		SSTRUCT_END(phClothVerletBehavior);
}
#endif



bool phClothVerletBehavior::UpdateVerletBound(phVerletCloth* cloth)
{
	PF_FUNC(EnvClothPostSimUpdate);

	Assert( cloth );

	Vector3 worldCentroid;
	float radiusOut;

	if(!m_Instance)
	{
		Assertf(false, "NULL inst in phClothVerletBehavior");
		return false;
	}

	if(!m_Instance->GetArchetype())
	{
		Assertf(false, "Inst missing archetype in phClothVerletBehavior.");
		return false;
	}

	phBound* fragInstanceBound = (m_Instance ? m_Instance->GetArchetype()->GetBound(): NULL);
//	Assert( fragInstanceBound );

#if __PS3	
	if( !fragInstanceBound )
		return false;
	// compute bounding volume moved to the SPU
	if( !cloth->GetFlag(phVerletCloth::FLAG_IS_ROPE) )
	{
  #if NO_BOUND_CENTER_RADIUS
		Vec3V center = cloth->GetCenter();
		radiusOut = cloth->GetRadius(center);
		worldCentroid = VEC3V_TO_VECTOR3(center);
  #else
		cloth->GetBoundingSphere( worldCentroid, radiusOut );
  #endif
	}
	else
#endif
	{
		cloth->ComputeBoundingVolume( fragInstanceBound, &worldCentroid, &radiusOut );
		if( !m_ActivateOnHit )
		{
			if( !fragInstanceBound )
				return false;

			if( fragInstanceBound->GetType() == phBound::COMPOSITE)
			{
				if(static_cast<phBoundComposite*>(fragInstanceBound)->HasBVHStructure())
				{
					if(m_Instance->IsInLevel())
					{
						PHLEVEL->RebuildCompositeBvh(m_Instance->GetLevelIndex());
					}
					else
					{
						static_cast<phBoundComposite*>(fragInstanceBound)->UpdateBvh(true);
					}
				}
			}
		}
	}
	if (cloth->GetFlag(phVerletCloth::FLAG_IS_ROPE))
	{
		UpdateRopeBound(cloth);		// Make the rope's composite bound match the simulated rope.
	}

	u16 levelIndex = phInst::INVALID_INDEX;
	if( !m_ActivateOnHit )
	{
		Assert( m_Instance );
		levelIndex = m_Instance->GetLevelIndex();
	}

	// Find out if this cloth is the only thing in its bound (if it's not part of another object).
	if( !m_ActivateOnHit && levelIndex != phInst::INVALID_INDEX )
	{
		bool isOnlyCloth = false;
		if( fragInstanceBound->GetType()==phBound::COMPOSITE )
		{
			phBoundComposite* composite = static_cast<phBoundComposite*>(fragInstanceBound);
			
			int firstBoundIndex = -1;
			phBound *firstBound = composite->GetFirstActiveBound(firstBoundIndex);
			isOnlyCloth = (firstBound && firstBound->GetType() == phBound::SPHERE) ? true: false;
		}
		else if (fragInstanceBound->GetType()==phBound::SPHERE )
		{
			isOnlyCloth = true;
		}
		else
		{
			Assert(0);
		}		

		if( cloth->GetFlag(phVerletCloth::FLAG_IS_ROPE) || isOnlyCloth )
		{
#if NO_BOUND_CENTER_RADIUS
			Vector3 position = VEC3V_TO_VECTOR3(cloth->GetCenter());
#else
			Vector3 position = cloth->GetBoundingCenterAndRadius().GetVector3();
#endif
			Vec4V lastReportedCenterAndRadius = PHLEVEL->GetCenterAndRadius(levelIndex);
			Vector3 lastReportedPosition = VEC4V_TO_VECTOR3(lastReportedCenterAndRadius);
			float distance = position.Dist(lastReportedPosition);

			float radius = cloth->GetRadius( VECTOR3_TO_VEC3V(position) );

			float lastReportedRadius = lastReportedCenterAndRadius.GetWf();
			if ((distance+radius>lastReportedRadius+0.01f) || (radius<0.9f*lastReportedRadius))
			{
				fragInstanceBound->SetRadiusAroundCentroid(ScalarVFromF32(radius));
				m_Instance->SetPosition( RC_VEC3V(position) );

				PHLEVEL->UpdateObjectLocationAndRadius(levelIndex, (Mat34V_Ptr)(NULL));

				return true;
			}
		}
		else
		{
#if USE_CLOTH_PHINST
			phBoundComposite* composite = static_cast<phBoundComposite*>(fragInstanceBound);			
			float radius = cloth->GetBoundingCenterAndRadius().GetW();
			int clothBoundIdx = -1;
			phBound* pClothBound = composite->GetLastActiveBound( clothBoundIdx );
			Assert( pClothBound );
			Assert( clothBoundIdx > -1 );

			Matrix34 clothPartPose(M34_IDENTITY);
			Vector3 localPosition;
			MAT34V_TO_MATRIX34(m_Instance->GetMatrix()).UnTransform( worldCentroid, localPosition );
			clothPartPose.d.Set(localPosition);			
			composite->SetCurrentMatrix(clothBoundIdx,RCC_MAT34V(clothPartPose));			
			pClothBound->SetRadiusAroundCentroid(ScalarVFromF32(radius));

			PHLEVEL->UpdateObjectLocationAndRadius(levelIndex, (Mat34V_Ptr)(NULL));
#endif // USE_CLOTH_PHINST
			return true;
		}
	}

	return false;
}

void phClothVerletBehavior::UpdateRopeBound(phVerletCloth* cloth, const phInst* clothInstance)
{
	// Use the member instance if none is supplied.
	const phInst* instance = (clothInstance ? clothInstance : m_Instance);

	// Update the previous matrices in the composite bound.
	Assert(instance && instance->GetArchetype() && instance->GetArchetype()->GetBound() && instance->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);
	if( !(instance && instance->GetArchetype() && instance->GetArchetype()->GetBound()) )
		return;
	phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(instance->GetArchetype()->GetBound());
	compositeBound.UpdateLastMatricesFromCurrent();

	Assert( cloth );
	// Get the edge-to-vertex index numbers.
	const phClothData& clothData = cloth->GetClothData();
	const atArray<phEdgeData>& ropeEdges = cloth->GetEdgeList();

	// Get the number of edges in the rope.
	Mat34V instancePose = instance->GetMatrix();
	Vec3V prevPosition,nextPosition,worldEdge,worldEdgeDirection;
	Vec3V unitX = Vec3V(V_X_AXIS_WZERO);

	// TODO: get the radius from the ropeinst somehow
	ScalarV ropeRadius = ScalarVFromF32(0.025f);

	ScalarV scalarHalf = ScalarV(V_HALF);
	ScalarV scalarEps = ScalarV(V_FLT_EPSILON);
	ScalarV minEdgeLength = ScalarVFromF32(0.01f);

	const int firstUnlockedEdge = cloth->GetNumLockedEdgesFront();
	const int lastUnlockedEdge = cloth->GetNumVertices() - cloth->GetNumLockedEdgesBack() - 1;

	for (int edgeIndex=firstUnlockedEdge; edgeIndex < lastUnlockedEdge; edgeIndex++)
	{
		// Find the position of this composite bound part.
		const int vertexIndex0 = ropeEdges[edgeIndex].m_vertIndices[0];
		const int vertexIndex1 = ropeEdges[edgeIndex].m_vertIndices[1];
		Vec3V position0 = clothData.GetVertexPosition(vertexIndex0);
		Vec3V position1 = clothData.GetVertexPosition(vertexIndex1);
		Vec3V worldEdge = Subtract(position1,position0);

		// Find the local y direction for this composite bound part.
		ScalarV edgeLength2 = MagSquared(worldEdge);
		BoolV positiveLength = IsGreaterThan(edgeLength2,scalarEps);
		ScalarV edgeLength = SelectFT(positiveLength,minEdgeLength,Sqrt(edgeLength2));
		Vec3V worldEdgeDirection = SelectFT(positiveLength,unitX,-InvScale(worldEdge,edgeLength));

		// Set the new bound part matrix to be the old bound part matrix rotated to match the new local y direction.
		Mat34V localPose = compositeBound.GetCurrentMatrix(edgeIndex);
		Vec3V xDirectionA = Cross(worldEdgeDirection,Vec3V(V_Z_AXIS_WZERO));
		Vec3V xDirectionB = Cross(Vec3V(V_Y_AXIS_WZERO),worldEdgeDirection);
		Vec3V normXA = NormalizeSafe(xDirectionA,Vec3V(V_ZERO));
		Vec3V normXB = NormalizeSafe(xDirectionB,Vec3V(V_ZERO));
		Vec3V xDirection = SelectFT(IsGreaterThan(MagSquared(xDirectionA),MagSquared(xDirectionB)),normXB,normXA);
		localPose.SetCol0(xDirection);
		localPose.SetCol1(worldEdgeDirection);
		localPose.SetCol2(Cross(xDirection,worldEdgeDirection));
		localPose.SetCol3(AddScaled(position0,worldEdge,scalarHalf));
		UnTransformFull(localPose,instancePose,localPose);

		// Update the previous and current composite bound part matrices.
		compositeBound.SetCurrentMatrix(edgeIndex,localPose);
		compositeBound.SetLastMatrix(edgeIndex,localPose);

		// Set the size of this composite bound part.
		Assert(compositeBound.GetBound(edgeIndex) && compositeBound.GetBound(edgeIndex)->GetType()==phBound::CAPSULE);
		phBoundCapsule* capsule = static_cast<phBoundCapsule*>(compositeBound.GetBound(edgeIndex));
		if (capsule)
		{
			capsule->SetCapsuleSize(ropeRadius,edgeLength);
		}

        const bool isFirstUnlockedEdge = (edgeIndex == firstUnlockedEdge ? true: false);
        if( isFirstUnlockedEdge || (edgeIndex+1) == lastUnlockedEdge )
        {
            const int beginIdx = (isFirstUnlockedEdge ? 0: lastUnlockedEdge );
            const int endIdx = (isFirstUnlockedEdge ? firstUnlockedEdge: (cloth->GetNumVertices() - 1) );

            for (int i=beginIdx; i < endIdx; ++i)
            {
                // Update the previous and current composite bound part matrices.
                compositeBound.SetCurrentMatrix(i,localPose);
                compositeBound.SetLastMatrix(i,localPose);

                // Set the size of this composite bound part.
                Assert(compositeBound.GetBound(i) && compositeBound.GetBound(i)->GetType()==phBound::CAPSULE);
                phBoundCapsule* capsule = static_cast<phBoundCapsule*>(compositeBound.GetBound(i));
                if (capsule)
                {
					capsule->SetCapsuleSize(ropeRadius,edgeLength);
                }
            }
        }
	}


	compositeBound.CalculateCompositeExtents(false);
	if(compositeBound.HasBVHStructure())
	{
		if(instance->IsInLevel())
		{
			PHLEVEL->RebuildCompositeBvh(instance->GetLevelIndex());
		}
		else
		{
			compositeBound.UpdateBvh(true);
		}
	}
}


} // namespace rage
