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

#ifndef COLLISION_ALGORITHM_H
#define COLLISION_ALGORITHM_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "DiscreteCollisionDetectorInterface.h"

namespace rage
{
    struct phCollisionInput;
    class phManifold;
}

struct btCollisionAlgorithmConstructionInfo
{
    int                 shapeType0;
    int                 shapeType1;
};


///btCollisionAlgorithm is an collision interface that is compatible with the Broadphase and btDispatcher.
///It is persistent over frames
class btCollisionAlgorithm
{
public:

	btCollisionAlgorithm() {};

	btCollisionAlgorithm(const btCollisionAlgorithmConstructionInfo& ci);

	virtual ~btCollisionAlgorithm() {};

//    virtual void DetectCollision (const rage::phCollisionInput& input, rage::Vector3::Param offsetA, rage::phManifold& manifold, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult) = 0;

};

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif //COLLISION_ALGORITHM_H
