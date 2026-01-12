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

#ifndef POINT_COLLECTOR_H
#define POINT_COLLECTOR_H
#if 0
#include "DiscreteCollisionDetectorInterface.h"

#include "vector/vector3.h"

#include "vectormath/vec3v.h"

class PointCollector : public DiscreteCollisionDetectorInterface::Result
{
public:
	rage::Vec3V m_normalOnBInWorld;
	rage::Vec3V m_pointOnAInWorld;
	rage::Vec3V m_pointOnBInWorld;
	rage::Vec3V m_distance; //negative means penetration
	int		m_ElementA, m_ElementB;
	float m_Friction, m_Elasticity;

	bool	m_hasResult;

	PointCollector () 
		: m_distance(rage::Vec3V(rage::V_FLT_MAX))
		, m_hasResult(false)
	{
	}

	virtual void AddContactPoint(rage::Vec::V3Param128 normalOnBInWorld,rage::Vec::V3Param128 pointOnAInWorld,rage::Vec::V3Param128_After3Args pointOnBInWorld,rage::Vec::V3Param128 separation,int elementA,int elementB)
	{
		using namespace rage;

		Vec3V v_separation(separation);
		if ( IsGreaterThanOrEqualAll( m_distance, v_separation ) != 0 )
		{
			m_hasResult = true;
			m_normalOnBInWorld = Vec3V(normalOnBInWorld);
			m_pointOnAInWorld = Vec3V(pointOnAInWorld);
			m_pointOnBInWorld = Vec3V(pointOnBInWorld);
			//negative means penetration
			m_distance = v_separation;

			m_ElementA = elementA;
			m_ElementB = elementB;
		}
	}
};
#endif
#endif //POINT_COLLECTOR_H

