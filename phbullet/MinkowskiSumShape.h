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

#ifndef MINKOWSKI_SUM_SHAPE_H
#define MINKOWSKI_SUM_SHAPE_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "phbound/bound.h"
#include "vectormath/legacyconvert.h"

/// MinkowskiSumShape represents implicit (getSupportingVertex) based minkowski sum of two convex implicit shapes.
class MinkowskiSumShape : public rage::phBound
{

    rage::Matrix34			m_transA;
	rage::Matrix34			m_transB;
	const rage::phBound*	m_shapeA;
	const rage::phBound*	m_shapeB;

public:

	MinkowskiSumShape(const rage::phBound* shapeA,const rage::phBound* shapeB);

 	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
    rage::Vector3 LocalGetSupportingVertexWithoutMargin (rage::Vector3::Param vec, int* vertexIndex=NULL) const;
	rage::Vector3	LocalGetSupportingVertex (rage::Vector3::Param localDirection)const;

	void	SetTransformA(const rage::Matrix34&	transA) { m_transA = transA;}
	void	SetTransformB(const rage::Matrix34&	transB) { m_transB = transB;}

	const rage::Matrix34& GetTransformA()const  { return m_transA;}
	const rage::Matrix34& GetTransformB()const  { return m_transB;}

#if !__SPU
	virtual int	GetShapeType() const { return 0; }
#endif	// __!SPU

	PH_NON_SPU_VIRTUAL float	GetMargin() const;

	const rage::phBound*	GetShapeA() const { return m_shapeA;}
	const rage::phBound*	GetShapeB() const { return m_shapeB;}

#if !__SPU
	virtual const char*	GetName()const 
	{
		return "MinkowskiSum";
	}
#endif	// __!SPU
};

FORCE_INLINE_SIMPLE_SUPPORT rage::Vector3 MinkowskiSumShape::LocalGetSupportingVertexWithoutMargin(rage::Vector3::Param vec, int* UNUSED_PARAM(vertexIndex))const
{
    rage::Vector3 vecA;
    m_transA.UnTransform3x3(vec, vecA);
    rage::Vector3 supVertexA;
    m_transA.Transform3x3(VEC3V_TO_VECTOR3(m_shapeA->LocalGetSupportingVertexWithoutMarginNotInlined(vecA)), supVertexA);
    rage::Vector3 vecB;
    m_transB.UnTransform3x3(vec, vecB);
    rage::Vector3 supVertexB;
    m_transB.Transform3x3(VEC3V_TO_VECTOR3(m_shapeB->LocalGetSupportingVertexWithoutMarginNotInlined(vecB)), supVertexB);
    return supVertexA + supVertexB;
}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif //MINKOWSKI_SUM_SHAPE_H
