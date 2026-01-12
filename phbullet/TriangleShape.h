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

#ifndef OBB_TRIANGLE_MINKOWSKI_H
#define OBB_TRIANGLE_MINKOWSKI_H


#include "CollisionMargin.h"

#include "phbound/bound.h"

#include "math/amath.h"
#include "vector/vector3.h"

#include "vectormath/vec3v.h"

namespace rage {

class TriangleShape : public phBound
{
public:
	Vec3V	m_vertices1[3];
	Vec3V	m_PolygonNormal;
	// This shape is once again non-resourceable so with COLLISION_MAY_USE_TRIANGLE_PD_SOLVER = 0
	// we can reclaim 64 of the 128 bytes used here (vert codes + padding, and edge normals)
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	Vec3V	m_EdgeNormals[3];
	u32		m_VertexNormalCodes[3];
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER

	phMaterialMgr::Id GetMaterialIdFromPartIndex(int UNUSED_PARAM(partIndex)) const                            
	{ 
		return GetPrimitiveMaterialId(); 
	}

	void SetMaterial(phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) 
	{ 
		SetPrimitiveMaterialId(materialId); 
	}

	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const
	{ 
		return  GetPrimitiveMaterialId(); 
	}

	void SetVertices(Vec3V_In v0, Vec3V_In v1, Vec3V_In v2)
	{
		m_vertices1[0] = v0;
		m_vertices1[1] = v1;
		m_vertices1[2] = v2;
	}

	void SetNormal(Vec3V_In normal)
	{
		m_PolygonNormal = normal;
	}

	const Vector3& GetVertexPtr(int index) const
	{
		return RCC_VECTOR3(m_vertices1[index]);
	}

	Vec3V_Out GetVertex(int index) const
	{
		return m_vertices1[index];
	}

	void LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 dir, SupportPoint & sp) const;

	__forceinline TriangleShape() : phBound()
	{
		m_Type = phBound::TRIANGLE;
	}

	__forceinline TriangleShape(const Vector3& p0,const Vector3& p1,const Vector3& p2) : phBound()
	{
		m_Type = phBound::TRIANGLE;

		m_vertices1[0] = VECTOR3_TO_VEC3V(p0);
		m_vertices1[1] = VECTOR3_TO_VEC3V(p1);
		m_vertices1[2] = VECTOR3_TO_VEC3V(p2);
	}

	__forceinline TriangleShape(Vec3V_In p0,Vec3V_In p1,Vec3V_In p2) : phBound()
	{
		m_Type = phBound::TRIANGLE;

		m_vertices1[0] = p0;
		m_vertices1[1] = p1;
		m_vertices1[2] = p2;
	}

	static const bool ms_RequiresDestructorOnStack = false;

	void ComputeBoundingBox ()
	{
		Vec3V boundingBoxMax = Max(m_vertices1[0], m_vertices1[1], m_vertices1[2]);
		Vec3V boundingBoxMin = Min(m_vertices1[0], m_vertices1[1], m_vertices1[2]);

		ScalarV margin = GetMarginV();
		SetBoundingBoxMin(boundingBoxMin - Vec3V(margin));
		SetBoundingBoxMax(boundingBoxMax + Vec3V(margin));
	}
};


FORCE_INLINE_SIMPLE_SUPPORT void TriangleShape::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 direction, SupportPoint & sp) const 
{
#if 0
	Vector3 v_zero(Vector3::ZeroType);
	Vector3 dir(direction);

    Vector3 dot0 = RCC_VECTOR3(m_vertices1[0]).DotV(dir);
    Vector3 dot1 = RCC_VECTOR3(m_vertices1[1]).DotV(dir);
    Vector3 dot2 = RCC_VECTOR3(m_vertices1[2]).DotV(dir);

    // dot1IsSmaller is positive if dot1 is smaller than dot0
    Vector3 dot0MinusDot1 = dot0 - dot1;

    // Compare the difference to zero
    Vector3 dot1IsSmaller = dot0MinusDot1.IsGreaterThanV(v_zero);

    // result holds the vector that is has the larger dot between 0 and 1
    Vector3 result = dot1IsSmaller.Select(RCC_VECTOR3(m_vertices1[1]), RCC_VECTOR3(m_vertices1[0]));

    // Compute the differences between the other two dot products
    Vector3 dot0MinusDot2 = dot0 - dot2;
    Vector3 dot1MinusDot2 = dot1 - dot2;

    // Whichever is bigger between dot0 and dot1, we want to compare that to dot2
    Vector3 theBiggerOfDot0andDot1MinusDot2 = dot1IsSmaller.Select(dot1MinusDot2, dot0MinusDot2);

    // dot1IsSmaller is positive if dot2 is smaller than both dot0 and dot1
    Vector3 dot2IsSmaller = theBiggerOfDot0andDot1MinusDot2.IsGreaterThanV(v_zero);

    // return result, or vector 2 if its dots are not smaller than 0 and 1
    sp.m_vertex = VECTOR3_TO_VEC3V(dot2IsSmaller.Select(RCC_VECTOR3(m_vertices1[2]), result));
	sp.m_index = DEFAULT_SUPPORT_INDEX;
#else
	const Vec4V v_zero(V_ZERO);
	const Vec4V v_one(V_INT_1);
	const Vec4V v_two(V_INT_2);
	const Vec3V dir(direction);
	const ScalarV dot0 = Dot(dir,m_vertices1[0]);
	const ScalarV dot1 = Dot(dir,m_vertices1[1]);
	const ScalarV dot2 = Dot(dir,m_vertices1[2]);
	const BoolV test0 = IsGreaterThan(dot0,dot1);
	Vec3V supVec = SelectFT(test0,m_vertices1[1],m_vertices1[0]);
	Vec4V supInd = SelectFT(test0,v_one,v_zero);
	ScalarV supDot = SelectFT(test0,dot1,dot0);
	const BoolV test1 = IsGreaterThan(supDot,dot2);
	supVec = SelectFT(test1,m_vertices1[2],supVec);
	supInd = SelectFT(test1,v_two,supInd);
	//supDot = SelectFT(test0,dot2,supDot);
	sp.m_vertex = supVec;
	sp.m_index = supInd.GetIntrin128ConstRef();
#endif
}

#if !__TOOL
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA && HACK_GTA4_64BIT_MATERIAL_ID_COLORS || OCTANT_MAP_SUPPORT_ACCEL
// Combining these two options causes geom bounds to be bigger by a quad word
CompileTimeAssert(sizeof(TriangleShape) <= phBound::MAX_BOUND_SIZE);
#else
CompileTimeAssert(sizeof(TriangleShape) == phBound::MAX_BOUND_SIZE);
#endif
#endif
} // namespace rage

#endif //OBB_TRIANGLE_MINKOWSKI_H

