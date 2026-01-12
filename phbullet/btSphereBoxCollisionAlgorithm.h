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

#ifndef SPHERE_BOX_COLLISION_ALGORITHM_H
#define SPHERE_BOX_COLLISION_ALGORITHM_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "btCollisionAlgorithm.h"
#include "btCollisionCreateFunc.h"
#include "vector/vector3.h"

namespace rage
{
    class phManifold;
}

/// btSphereBoxCollisionAlgorithm  provides sphere-box collision detection.
/// Other features are frame-coherency (persistent data) and collision response.
class btSphereBoxCollisionAlgorithm : public btCollisionAlgorithm
{	
public:

	btSphereBoxCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);

	virtual ~btSphereBoxCollisionAlgorithm();

    virtual void DetectCollision (const rage::phCollisionInput& input, rage::Vector3::Param offsetA, rage::phManifold& manifold, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult);

    float getSphereDistance( rage::Vector3& v3PointOnBox, rage::Vector3& v3PointOnSphere, const rage::Vector3& v3SphereCenter, float fRadius );

    float getSpherePenetration( rage::Vector3& v3PointOnBox, rage::Vector3& v3PointOnSphere, const rage::Vector3& v3SphereCenter, float fRadius, const rage::Vector3& aabbMin, const rage::Vector3& aabbMax);
};

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif //SPHERE_BOX_COLLISION_ALGORITHM_H
