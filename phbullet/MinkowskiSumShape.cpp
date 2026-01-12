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

#include "MinkowskiSumShape.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "phbound/support.h"

using namespace rage;

MinkowskiSumShape::MinkowskiSumShape(const phBound* shapeA,const phBound* shapeB)
:m_shapeA(shapeA),
m_shapeB(shapeB)
{
	m_transA.Identity();
	m_transB.Identity();
}

float	MinkowskiSumShape::GetMargin() const
{
	return m_shapeA->GetMargin() + m_shapeB->GetMargin();
}

static const rage::Vector3 VEC3_ONEZEROZERO( 1.0f, 0.0f, 0.0f );

Vector3	MinkowskiSumShape::LocalGetSupportingVertex(rage::Vector3::Param vec)const
{
	rage::Vector3 vecnorm(vec);
	vecnorm.NormalizeSafe(VEC3_ONEZEROZERO,0.0f);

	rage::Vector3 supVertex = LocalGetSupportingVertexWithoutMargin(vecnorm);
	rage::Vector3 margin = VEC3V_TO_VECTOR3(m_shapeA->GetMarginV() + m_shapeB->GetMarginV());

	supVertex.AddScaled(vecnorm,margin);

	return supVertex;
}

#endif // ENABLE_UNUSED_PHYSICS_CODE
