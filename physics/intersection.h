//
// physics/intersection.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_INTERSECTION_H
#define PHCORE_INTERSECTION_H

#include "levelnew.h"

#include "diag/debuglog.h"
#include "phcore/materialmgr.h"
#include "system/memops.h"
#include "vector/geometry.h"
#include "vectormath/classes.h"

namespace rage {

class phInst;

#define HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_INTERSECTION (1 && HACK_GTA4 && HACK_GTA4_BOUND_GEOM_SECOND_SURFACE)

/*
PURPOSE
	This class contains information about a point of intersection, including its location in space, and
	information about the object being intersected.

NOTES
 1. phIntersection is used as the argument for returning intersection information in Test functions
	(TestProbe, TestSphere, etc), which are defined in all the physics bound class types.
 2.	The TestPoint, TestProbe and TestEdge functions fill in all of the phIntersection information when an
	intersection occurs, so no initialization of phIntersection is required.
<FLAG Component>
*/
class phIntersection
{
public:
	// PURPOSE: Set all the parameters in this intersection to default values.
 	void Zero ();

	// PURPOSE: constructor
	phIntersection();

	// PURPOSE: Set all the parameters in a phIntersection except for the physics instance (PhysInst).
	// PARAMS:
	//	position	- the location of the intersection, which is normally in the local coordinates of the instance (before calling
	//					phIntersection::Transform).
	//	normal		- the direction outward from and perpendicular to the bound's surface at the position
	//					of the intersection, normally in local coordinates of the instance (before calling phIntersection::Transform)
	//	t			- the fraction of the distance along the segment at which the intersection occurs
	//	depth		- the distance of the segment's end beyond the intersection point
	//	partIndex	- the index number of the part of the bound where the intersection occurred - for polyhedrons, this is the intersected vertex,
	//					edge or polygon index number, and for other bound types it is the index used in collisions to indicate the section of the
	//					bound (0 for spheres, 0 and 1 for capsule and cylinder ends, 2 for capsule and cylinder shafts)
	//	component	- optional index number of the intersected bound in a composite bound (default is 0)
	//	materialId	- optional material manager index number for the material on the intersected part of the bound
	void Set (Vec3V_In position, Vec3V_In normal, float t, float depth, u16 partIndex, u16 component=0,
				phMaterialMgr::Id materialId=phMaterialMgr::DEFAULT_MATERIAL_ID);
	void Set (Vec3V_In position, Vec3V_In normal, ScalarV_In t, ScalarV_In depth, u16 partIndex, u16 component=0,
				phMaterialMgr::Id materialId=phMaterialMgr::DEFAULT_MATERIAL_ID);
	

	// PURPOSE: Set all the parameters in a phIntersection except for the physics instance (m_PhysInst).
	// PARAMS:
	//  levelIndex	- if specified, should be the level index of the instance intersected
	//  generationID- if specified, should be the generation ID of the level index specified above - only needed if somebody is actually going to
	//					check it
	//	position	- the location of the intersection, which is normally in the local coordinates of the instance (before calling
	//					phIntersection::Transform).
	//	normal		- the direction outward from and perpendicular to the bound's surface at the position
	//					of the intersection, normally in local coordinates of the instance (before calling phIntersection::Transform)
	//	t			- the fraction of the distance along the segment at which the intersection occurs
	//	depth		- the distance of the segment's end beyond the intersection point
	//	partIndex	- the index number of the part of the bound where the intersection occurred - for polyhedrons, this is the intersected vertex,
	//					edge or polygon index number, and for other bound types it is the index used in collisions to indicate the section of the
	//					bound (0 for spheres, 0 and 1 for capsule and cylinder ends, 2 for capsule and cylinder shafts)
	//	component	- optional index number of the intersected bound in a composite bound (default is 0)
	//	materialId	- optional material manager index number for the material on the intersected part of the bound
	void Set (u16 levelIndex, u16 generationID, Vec3V_In position, Vec3V_In normal, float t, float depth, u16 partIndex, u16 component,
				phMaterialMgr::Id materialId);

	// Versions using ScalarV for depth
	void Set (u16 levelIndex, u16 generationID, Vec3V_In position, Vec3V_In normal, ScalarV_In t, ScalarV_In depth, u16 partIndex, u16 component,
				phMaterialMgr::Id materialId);

	// PURPOSE: Copy all the parameters in this intersection from the given intersection.
	// PARAMS:
	//	isect -		the intersection from which to set this intersection
	void Copy (phIntersection& isect);

	// PURPOSE: Transform an intersection by the given matrix.
	// PARAMS:
	//	matrix	- the matrix by which to transform the intersection
	// NOTES:
	//	1.	This is normally used after an intersection is recorded in the local coordinates of the same
	//		physics instance used in the intersection, and that instance's coordinate matrix is then used
	//		as the argument to transform the intersection's position and normal to world coordinates.
	void Transform (Mat34V_In matrix);

	// PURPOSE: Reset this intersection for use in another intersection test.
	// NOTES:	This only sets the instance pointer to NULL.
	void Reset ();

	// PURPOSE: Set the instance pointer in this intersection.
	// PARAMS:
	//	instance - the instance pointer to set in this intersection
	void SetInstance (u16 levelIndex, u16 generationID);

	// PURPOSE: Set the position of this intersection.
	// PARAMS:
	//	position - the position to set in this intersection
	// NOTES:	This is normally in local coordinates of the intersection's instance, and phIntersection::Transform is called to put it in world coordinates.
	void SetPosition (Vec3V_In position);

	// PURPOSE: Set the unit normal vector of this intersection.
	// PARAMS:
	//	normal - the unit normal vector to set in this intersection
	// NOTES:
	//	1.	This is normally in local coordinates of the intersection's instance, and phIntersection::Transform is called to put it in world coordinates.
	//	2.	The normal vector points out from the surface of the intersected bound.
	void SetNormal (Vec3V_In normal);

	// PURPOSE: Set the unit normal of the intersected polygon.
	// PARAMS:
	//	intersectedPolyNormal - the unit normal of the intersected polygon
	void SetIntersectedPolyNormal (Vec3V_In intersectedPolyNormal);

	// PURPOSE: Set the t-value in this intersection.
	// PARAMS:
	//	t - the t-value to set in this intersection
	// NOTES:	The t-value is the fraction of the distance along the segment where the intersection occurs.
	void SetT (float t);
	void SetT (ScalarV_In t);

	// PURPOSE:	Negate the unit normal vector of this intersection.
	void NegateNormal ();

	// PURPOSE: Set the depth of this intersection.
	// PARAMS:
	//	depth - the depth to set in this intersection
	void SetDepth (float depth);
	void SetDepth (ScalarV_In depth);

	// PURPOSE: Set the material manager's material index number of this intersection.
	// PARAMS:
	//	materialId - the material manager's material index number to set in this intersection
	void SetMaterialId (phMaterialMgr::Id materialId);

	// PURPOSE: Set the part index of this intersection.
	// PARAMS:
	//	partIndex - the part index to set in this intersection
	void SetPartIndex (u16 partIndex);

	// PURPOSE: Set the component number of this intersection.
	// PARAMS:
	//	component - the component number to set in this intersection
	void SetComponent (u16 component);

	// PURPOSE: Determine whether the phIntersection was used to hold intersection information.
	// RETURN:	true if there is an intersection with a bound, false otherwise
	// NOTES:
	//	1.	To get a pointer to the bound that was intersected, call GetInstance()->GetArchetype()->GetBound().
	bool IsAHit () const;

	// PURPOSE: Get the physics instance pointer.
	// RETURN:	the physics instance pointer
	const phInst* GetInstance () const;

	// PURPOSE: Get the physics instance pointer.
	// RETURN:	the physics instance pointer
	phInst* GetInstance ();

	// PURPOSE: Get the level index of the instance hit by this intersection.
	// RETURN:  the level index of the instance hit by this intersection
	// NOTES:
	//  1. This is important if you're going to be reading from a phIntersection some time after the time that it was filled out in a games where
	//     instances can be deleted at run-time.
	//  2. If the instance pointer still points to a valid instance, this should be same value that you would get from GetInstance()->GetLevelIndex().
	u16 GetLevelIndex() const;

	// PURPOSE: Get the 'generation ID' of the instance hit by this intersection.
	// RETURN: the level index of the instance hit by this intersection
	//  1. This is important if you're going to be reading from a phIntersection some time after the time that it was filled out in a games where
	//     instances can be deleted at run-time.
	u16 GetGenerationID() const;

	// PURPOSE: Get the position.
	// RETURN:	the position
	Vec3V_ConstRef GetPosition () const;

	// PURPOSE: Get the unit normal vector.
	// RETURN:	the unit normal vector
	Vec3V_ConstRef GetNormal () const;

	// PURPOSE: Get the normal of the intersected polygon.
	// RETURN:	the normal of the intersected polygon
	Vec3V_ConstRef GetIntersectedPolyNormal () const;

	// PURPOSE: Get the t-value.
	// RETURN:	the t-value
	float GetT () const;
	ScalarV_Out GetTV() const;

	// PURPOSE: Get the depth.
	// RETURN:	the depth
	float GetDepth () const;
	ScalarV_Out GetDepthV () const;

	// PURPOSE: Get the material manager's material index number.
	// RETURN:	the material manager's material index number
	phMaterialMgr::Id GetMaterialId () const;

	// PURPOSE: Get the part index.
	// RETURN:	the part index
	u16 GetPartIndex () const;

	// PURPOSE: Get the component number.
	// RETURN:	the component number
	u16 GetComponent () const;

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about this intersection.
	void DebugReplay () const;
#endif

#if __DEV
	bool AreAllValuesFinite() const;
#endif // __DEV

private:
	// PURPOSE:	
	//   Position - XYZ - the location of the intersection, which is normally in the local coordinates of the instance (before calling phIntersection::Transform)
	//   Depth - W - the distance of the segment's end beyond the intersection point - for a directed test (such as TestProbe or TestCapsule),
	//			       this is the distance the primitive (the point or the sphere) traveled after intersecting
	Vec4V m_PositionXYZDepthW;

	// PURPOSE:	
	//  Normal - XYZ - the direction outward from and perpendicular to the bound's surface at the position of the intersection, normally in local coordinates
	//		             of the instance (before calling phIntersection::Transform)
	//  TValue - W - the fraction of the distance along the segment at which the intersection occurs
	Vec4V m_NormalXYZTValueW;


	// PURPOSE: 
	//  IntersectedPolyNormal - XYZ - the direction outward from the intersected polygon. 
	//                                  If a primitive bound is intersected, this is set to the intersection normal (m_Normal).
	//  Part - W upper 16 bits - the index number of the part of the bound where the intersection occurred - for polyhedrons, this is the intersected vertex,
	//			                   edge or polygon index number, and for other bound types it is the index used in collisions to indicate the section of the
	//			                   bound (0 for spheres, 0 and 1 for capsule and cylinder ends, 2 for capsule and cylinder shafts) 
	//  Component - W lower 16 bits - index number of the intersected bound in a composite bound (default is 0)
	Vec4V m_IntersectedPolyNormalXYZPartComponentW;

protected:

	// PURPOSE:	material manager index number for the material on the intersected part of the bound
	phMaterialMgr::Id m_MaterialId;

	// PURPOSE: information about the physics instance hit in this intersection
	u16 m_LevelIndex;
	u16 m_GenerationID;
};


inline void phIntersection::Zero ()
{
	sysMemZeroBytes<sizeof(phIntersection)>(this);
	m_LevelIndex = (u16)-1;
}


inline phIntersection::phIntersection ()
{
	Zero();
}


inline void phIntersection::Set (Vec3V_In position, Vec3V_In normal, float t, float depth, u16 partIndex, u16 component, phMaterialMgr::Id materialId)
{
	Set(position,normal,ScalarVFromF32(t),ScalarVFromF32(depth),partIndex,component,materialId);
}


inline void phIntersection::Set (Vec3V_In position, Vec3V_In normal, ScalarV_In t, ScalarV_In depth, u16 partIndex, u16 component, phMaterialMgr::Id materialId)
{
	m_PositionXYZDepthW = GetFromTwo<Vec::X1,Vec::Y1,Vec::Z1,Vec::W2>(Vec4V(position),Vec4V(depth));
	m_NormalXYZTValueW = GetFromTwo<Vec::X1,Vec::Y1,Vec::Z1,Vec::W2>(Vec4V(normal),Vec4V(t));

	(Vec3V_Ref)m_IntersectedPolyNormalXYZPartComponentW = normal;
	SetPartIndex(partIndex);
	SetComponent(component);
	m_MaterialId = materialId;
}

inline void phIntersection::Set (u16 levelIndex, u16 generationID, Vec3V_In position, Vec3V_In normal, float t, float depth, u16 partIndex, u16 component, phMaterialMgr::Id materialId)
{
	Set(levelIndex,generationID,position,normal,ScalarVFromF32(t),ScalarVFromF32(depth),partIndex,component,materialId);
}


inline void phIntersection::Set (u16 levelIndex, u16 generationID, Vec3V_In position, Vec3V_In normal, ScalarV_In t, ScalarV_In depth, u16 partIndex, u16 component, phMaterialMgr::Id materialId)
{
	// Set everything except the physics instance in this intersection.
	Set(position,normal,t,depth,partIndex,component,materialId);

	// Set the instance-related information.
	m_LevelIndex = levelIndex;
	m_GenerationID = generationID;
}

inline void phIntersection::Copy (phIntersection& isect)
{
	*this = isect;
}


inline void phIntersection::Transform (Mat34V_In matrix)
{
	//m_Position = Transform(matrix, m_Position); // Doesn't work because Transform is a member of this class...
	SetPosition(rage::Transform(matrix,GetPosition()));
	SetNormal(Transform3x3(matrix, GetNormal()));
	SetIntersectedPolyNormal(Transform3x3(matrix, GetIntersectedPolyNormal()));
}


inline void phIntersection::Reset ()
{
	m_LevelIndex = (u16)(-1);
	m_GenerationID = 0;
}


inline void phIntersection::SetInstance (u16 levelIndex, u16 generationID)
{
	m_LevelIndex = levelIndex;
	m_GenerationID = generationID;
}

inline void phIntersection::SetPosition (Vec3V_In position)
{
	m_PositionXYZDepthW.SetXYZ(position);
}


inline void phIntersection::SetNormal (Vec3V_In normal)
{
	m_NormalXYZTValueW.SetXYZ(normal);
}


inline void phIntersection::SetIntersectedPolyNormal (Vec3V_In intersectedPolyNormal)
{
	m_IntersectedPolyNormalXYZPartComponentW.SetXYZ(intersectedPolyNormal);
}


inline void phIntersection::SetT (float t)
{
	m_NormalXYZTValueW.SetWf(t);
}

inline void phIntersection::SetT (ScalarV_In t)
{
	m_NormalXYZTValueW.SetW(t);
}


inline void phIntersection::NegateNormal ()
{
	SetNormal(Negate(GetNormal()));
}


inline void phIntersection::SetDepth (float depth)
{
	m_PositionXYZDepthW.SetWf(depth);
}

inline void phIntersection::SetDepth (ScalarV_In depth)
{
	m_PositionXYZDepthW.SetW(depth);
}

inline void phIntersection::SetMaterialId (phMaterialMgr::Id materialId)
{
	m_MaterialId = materialId;
}


inline void phIntersection::SetPartIndex (u16 partIndex)
{
	((u16*)&m_IntersectedPolyNormalXYZPartComponentW)[3*2] = partIndex;
}


inline void phIntersection::SetComponent (u16 component)
{
	((u16*)&m_IntersectedPolyNormalXYZPartComponentW)[(3*2)+1] = component;
}

inline bool phIntersection::IsAHit () const
{
#if __SPU
	FastAssert(0); // Shouldn't be calling this on SPU.
	return false;
#else // __SPU
	return (PHLEVEL->LegitLevelIndex(m_LevelIndex) && PHLEVEL->GetState(m_LevelIndex)!=phLevelBase::OBJECTSTATE_NONEXISTENT)
		? PHLEVEL->GetInstance(m_LevelIndex)!=NULL : false;
#endif // __SPU
}


inline const phInst* phIntersection::GetInstance () const
{
#if __SPU
	FastAssert(0); // Shouldn't be calling this on SPU.
	return NULL;
#else // __SPU
	if(PHLEVEL->LegitLevelIndex(m_LevelIndex) && PHLEVEL->GetState(m_LevelIndex)!=phLevelBase::OBJECTSTATE_NONEXISTENT)
	{
		// NOTE: I think the phLevelBase::OBJECTSTATE_NONEXISTENT might be taken care of by IsLevelIndexGenerationIDCurrent
		if(PHLEVEL->IsLevelIndexGenerationIDCurrent(m_LevelIndex,m_GenerationID))
		{
			return PHLEVEL->GetInstance(m_LevelIndex);
		}
	}
	return NULL;
#endif // __SPU
}


inline phInst* phIntersection::GetInstance ()
{
#if __SPU
	FastAssert(0); // Shouldn't be calling this on SPU.
	return NULL;
#else // __SPU
	if(PHLEVEL->LegitLevelIndex(m_LevelIndex) && PHLEVEL->GetState(m_LevelIndex)!=phLevelBase::OBJECTSTATE_NONEXISTENT)
	{
		// NOTE: I think the phLevelBase::OBJECTSTATE_NONEXISTENT might be taken care of by IsLevelIndexGenerationIDCurrent
		if(PHLEVEL->IsLevelIndexGenerationIDCurrent(m_LevelIndex,m_GenerationID))
		{
			return PHLEVEL->GetInstance(m_LevelIndex);
		}
	}
	return NULL;
#endif // __SPU
}


inline u16 phIntersection::GetLevelIndex() const
{
	return m_LevelIndex;
}


inline u16 phIntersection::GetGenerationID() const
{
	return m_GenerationID;
}


inline Vec3V_ConstRef phIntersection::GetPosition () const
{
	return (Vec3V_ConstRef)m_PositionXYZDepthW;
}


inline Vec3V_ConstRef phIntersection::GetNormal () const
{
	return (Vec3V_ConstRef)m_NormalXYZTValueW;
}


inline Vec3V_ConstRef phIntersection::GetIntersectedPolyNormal () const
{
	return (Vec3V_ConstRef)m_IntersectedPolyNormalXYZPartComponentW;
}


inline float phIntersection::GetT () const
{
	return m_NormalXYZTValueW.GetWf();
}


inline ScalarV_Out phIntersection::GetTV () const
{
	return m_NormalXYZTValueW.GetW();
}


inline float phIntersection::GetDepth () const
{
	return m_PositionXYZDepthW.GetWf();
}


inline ScalarV_Out phIntersection::GetDepthV () const
{
	return m_PositionXYZDepthW.GetW();
}


inline phMaterialMgr::Id phIntersection::GetMaterialId () const
{
	return m_MaterialId;
}


inline u16 phIntersection::GetPartIndex () const
{
	return ((u16*)&m_IntersectedPolyNormalXYZPartComponentW)[(3*2)];
}

inline u16 phIntersection::GetComponent () const
{
	return ((u16*)&m_IntersectedPolyNormalXYZPartComponentW)[(3*2)+1];
}

#if __DEV
inline bool phIntersection::AreAllValuesFinite() const
{
	const VecBoolV vIsFinite =	IsFinite(m_PositionXYZDepthW) & 
								IsFinite(m_NormalXYZTValueW) & 
								IsFinite(m_IntersectedPolyNormalXYZPartComponentW & Vec4V(V_MASKXYZ));
	return IsTrueAll(vIsFinite);
}
#endif // __DEV

#if __DEBUGLOG
inline void phIntersection::DebugReplay () const
{
	diagDebugLog(diagDebugLogPhysics, 'pIPs', &m_PositionXYZDepthW);
	diagDebugLog(diagDebugLogPhysics, 'pINr', &m_NormalXYZTValueW);
	diagDebugLog(diagDebugLogPhysics, 'pIIN', &m_IntersectedPolyNormalXYZPartComponentW);
	diagDebugLog(diagDebugLogPhysics, 'pITV', &m_NormalXYZTValueW[3]);
	diagDebugLog(diagDebugLogPhysics, 'pITp', &m_PositionXYZDepthW[3]);
	diagDebugLog(diagDebugLogPhysics, 'pIMI', &m_MaterialId);
}
#endif

} // namespace rage

#endif // end of #ifndef PHCORE_INTERSECTION_H
