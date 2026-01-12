// 
// phbound/support.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHBOUND_SUPPORT_H 
#define PHBOUND_SUPPORT_H 

#include "boundsphere.h"
#include "boundcapsule.h"
#include "boundcomposite.h"
#include "boundcylinder.h"
#include "boundtaperedcapsule.h"
#include "boundbox.h"
#include "boundpolyhedron.h"
#include "boundcurvedgeom.h"
#include "boundbvh.h"
#include "bounddisc.h"
#include "phbullet/TriangleShape.h"


namespace rage {

__forceinline Vec3V_Out phBound::LocalGetSupportingVertex (Vec::V3Param128 localDirection)const
{
	// Get the unit normal along the local direction.
	Vec3V localDir = Vec3V(localDirection);
	Vec3V vecnorm = NormalizeSafe(localDir,Vec3V(V_X_AXIS_WONE),Vec3V(V_ZERO));

	// Get the supporting vertex without including the collision margin.
	Vec3V supVertex = LocalGetSupportingVertexWithoutMarginNotInlined(vecnorm.GetIntrin128());

	// Get the collision margin.
	ScalarV margin = ScalarVFromF32(GetMargin());

	// Extent the supporting vertex along the local direction by the collision margin.
	vecnorm = Scale(vecnorm,margin);
	supVertex = Add(supVertex,vecnorm);

	// Return the supporting vertex position.
	return supVertex;
}

__forceinline int phBound::FindElementIndex(Vec::V3Param128 localNormal, Vec::V3Param128 localContact, int* vertexIndices, int numVertices) const
{
	int element = 0;
	Vector3 local(localNormal);

	switch(GetType())
	{
	case rage::phBound::GEOMETRY:
		element = static_cast<const phBoundGeometry*>(this)->FindElementIndex(local, localContact, vertexIndices, numVertices);
		break;

	default:
		element = GetIndexFromBound();
		break;
	}

	return element;
}


inline phMaterialMgr::Id phBound::GetMaterialIdFromPartIndex (int partIndex) const
{
	phMaterialMgr::Id materialId;

	switch(GetType())
	{
	case rage::phBound::SPHERE:
		materialId = static_cast<const phBoundSphere*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	case rage::phBound::CAPSULE:
		materialId = static_cast<const phBoundCapsule*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

#if USE_TAPERED_CAPSULE
	case rage::phBound::TAPERED_CAPSULE:
		materialId = static_cast<const phBoundTaperedCapsule*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;
#endif

	case rage::phBound::BOX:
		materialId = static_cast<const phBoundBox*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	case rage::phBound::GEOMETRY:
		materialId = static_cast<const phBoundGeometry*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

#if USE_GEOMETRY_CURVED
	case rage::phBound::GEOMETRY_CURVED:
		materialId = static_cast<const phBoundCurvedGeometry*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;
#endif

	case rage::phBound::BVH:
		materialId = static_cast<const phBoundBVH*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	USE_GRIDS_ONLY(case rage::phBound::GRID:)
	USE_RIBBONS_ONLY(case rage::phBound::RIBBON:)
	USE_SURFACES_ONLY(case rage::phBound::SURFACE:)
	case rage::phBound::COMPOSITE:
		materialId = phMaterialMgr::DEFAULT_MATERIAL_ID;
		break;

	case rage::phBound::TRIANGLE:
		materialId = static_cast<const TriangleShape*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	case rage::phBound::DISC:
		materialId = static_cast<const phBoundDisc*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	case rage::phBound::CYLINDER:
		materialId = static_cast<const phBoundCylinder*>(this)->GetMaterialIdFromPartIndex(partIndex);
		break;

	default:
		materialId = phMaterialMgr::DEFAULT_MATERIAL_ID;
		break;
	}

	return materialId;
}


inline phMaterialMgr::Id phBound::GetMaterialId (int materialIndex) const
{
	switch(GetType())
	{
	case phBound::SPHERE:
		{
			return static_cast<const phBoundSphere *>(this)->phBoundSphere::GetMaterialId(materialIndex);
		}
	case phBound::CAPSULE:
		{
			return static_cast<const phBoundCapsule *>(this)->phBoundCapsule::GetMaterialId(materialIndex);
		}
#if USE_TAPERED_CAPSULE
	case phBound::TAPERED_CAPSULE:
		{
			return static_cast<const phBoundTaperedCapsule *>(this)->phBoundTaperedCapsule::GetMaterialId(materialIndex);
		}
#endif
	case phBound::BOX:
		{
			return static_cast<const phBoundBox *>(this)->phBoundBox::GetMaterialId(materialIndex);
		}
	case phBound::GEOMETRY:
		{
			return static_cast<const phBoundGeometry *>(this)->phBoundGeometry::GetMaterialId(materialIndex);
		}
	case phBound::BVH:
		{
			phMaterialMgr::Id retVal = static_cast<const phBoundBVH *>(this)->phBoundBVH::GetMaterialId(materialIndex);

			return retVal;
		}
#if USE_GEOMETRY_CURVED
	case phBound::GEOMETRY_CURVED:
		{
			return static_cast<const phBoundCurvedGeometry *>(this)->phBoundCurvedGeometry::GetMaterialId(materialIndex);
		}
#endif
	case phBound::DISC:
		{
			return static_cast<const phBoundDisc *>(this)->phBoundDisc::GetMaterialId(materialIndex);
		}
	case phBound::CYLINDER:
		{
			return static_cast<const phBoundCylinder *>(this)->phBoundCylinder::GetMaterialId(materialIndex);
		}
	case phBound::TRIANGLE:
		{
			return static_cast<const TriangleShape *>(this)->TriangleShape::GetMaterialId(materialIndex);
		}
	USE_GRIDS_ONLY(case phBound::GRID:)
	case phBound::COMPOSITE:
	default:
		{
			Assertf(false, "Shouldn't be calling GetMaterialId on composite or grid bounds (it should be called on the subbound) - type %d", GetType());
			return phMaterialMgr::DEFAULT_MATERIAL_ID;
		}
	}
	//return phMaterialMgr::DEFAULT_MATERIAL_ID;
}

} // namespace rage

#endif // PHBOUND_SUPPORT_H 
