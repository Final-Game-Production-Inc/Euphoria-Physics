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


#ifndef CONVEX_CAST_H
#define CONVEX_CAST_H

class MinkowskiSumShape;

#include "vector/matrix34.h"
#include "vector/vector3.h"

#include "vectormath/mat34v.h"
#include "vectormath/vec3v.h"

/// ConvexCast is an interface for Casting
class ConvexCast
{
public:
	///RayResult stores the closest result
	/// alternatively, add a callback method to decide about closest/all results
	struct	CastResult
	{
		CastResult()
			: m_fraction(rage::ScalarV(rage::V_FLT_MAX))
		{
		}

		//rage::Vector3	m_normal;
		//rage::Vector3	m_pointOnA;
		//rage::Vector3	m_pointOnB;
		//rage::Vector3	m_fraction;
		//int		m_elementA;
		//int		m_elementB;
		//rage::Matrix34 m_hitTransformA;
		//rage::Matrix34 m_hitTransformB;

		rage::Vec3V		m_normal;
		rage::Vec3V		m_pointOnA;
		rage::Vec3V		m_pointOnB;
		rage::ScalarV	m_fraction;
		int							m_elementA;
		int							m_elementB;
		float						m_friction;
		float						m_elasticity;
		rage::Mat34V	m_hitTransformA;
		rage::Mat34V	m_hitTransformB;

	};


	/// cast a convex against another convex object
//	bool ComputeTimeOfImpact (const rage::Matrix34& fromA, const rage::Matrix34& toA, const rage::Matrix34& fromB, const rage::Matrix34& toB, rage::Vector3::Param triangleNormal, CastResult& result) = 0;
};

#endif //CONVEX_CAST_H
