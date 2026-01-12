/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

EPA Copyright (c) Ricardo Padrela 2006

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btGjkEpaPenetrationDepthSolver.h"

#if ENABLE_EPA_PENETRATION_SOLVER_CODE

#include "btGjkEpa2.h"

#include "phbound/bound.h"

bool btGjkEpaPenetrationDepthSolver::ComputePenetrationDepth (const rage::phBound* convexA, const rage::phBound* convexB, rage::Mat34V_In transA, rage::Mat34V_In transB,
															  rage::Vec3V_InOut worldPointOnA, rage::Vec3V_InOut worldPointOnB)
{
//	const float radialmargin(0.0f);
	rage::Vector3 xAxis(1.0f, 0.0f, 0.0f);
	btGjkEpaSolver2::sResults results;
	if (btGjkEpaSolver2::
		Penetration
		(convexA,RCC_MATRIX34(transA),convexB,RCC_MATRIX34(transB),
		xAxis,results))
	{
	//	debugDraw->drawLine(results.witnesses[1],results.witnesses[1]+results.normal,btVector3(255,0,0));
	//	resultOut->addContactPoint(results.normal,results.witnesses[1],-results.depth);
		worldPointOnA = RCC_VEC3V(results.witnesses[0]);
		worldPointOnB = RCC_VEC3V(results.witnesses[1]);
		return true;		
	}

	return false;
}

#endif // ENABLE_EPA_PENETRATION_SOLVER_CODE


