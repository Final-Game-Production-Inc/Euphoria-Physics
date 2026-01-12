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

#include "btSphereSphereCollisionAlgorithm.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "btCollisionDispatcher.h"
#include "phbound/boundsphere.h"
#include "physics/collider.h"
#include "physics/collision.h"
#include "physics/simulator.h"
#include "profile/element.h"
#include "vectormath/classes.h"

namespace rage {


btSphereSphereCollisionAlgorithm::btSphereSphereCollisionAlgorithm()
{
}

btSphereSphereCollisionAlgorithm::~btSphereSphereCollisionAlgorithm()
{
}

#if 0
void btSphereSphereCollisionAlgorithm::DetectCollision (const rage::phCollisionInput& input, rage::Vector3::Param UNUSED_PARAM(offsetA), rage::phManifold& UNUSED_PARAM(manifold), DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult)
{
	const rage::phBoundSphere* sphere0 = static_cast<const rage::phBoundSphere*>(input.boundA);
	const rage::phBoundSphere* sphere1 = static_cast<const rage::phBoundSphere*>(input.boundB);

	Vec3V center0 = Transform(input.currentA, sphere0->GetCentroidOffset());

	Vec3V center1 = Transform(input.currentB, sphere1->GetCentroidOffset());

	Vec3V diff = center0 - center1;
	ScalarV len = Mag(diff);

	ScalarV radius0 = sphere0->GetRadiusV();
	ScalarV radius1 = sphere1->GetRadiusV();

	if (IsLessThanAll(len, ScalarV(V_FLT_SMALL_6)))
	{
		len = radius0 + radius1;
		diff = Vec3V(len, ScalarV(V_ZERO), ScalarV(V_ZERO));
	}

	ScalarV radius = radius0 + radius1;

	///iff distance positive, don't generate a new contact
	if ( IsGreaterThanAll(len, radius) )
		return;

	///distance (negative means penetration)
	ScalarV dist = len - radius;

	Vec3V normalOnSurfaceB = diff / len;

	///point on A (worldspace)
	Vec3V pos0 = center0 + radius0 * normalOnSurfaceB;

	///point on B (worldspace)
	Vec3V pos1 = center1 - radius1 * normalOnSurfaceB;

	// TODO: Cleanup this last part - it was altered to match a signature change to AddContactPoint while not in use
	//  f.ex. Get rid of those untransforms and possibly some of those objects/function calls
	/// report a contact. internally this will be kept persistent, and contact reduction is done
	DiscreteCollisionDetectorInterface::SimpleResult result;
	result.AddContactPoint(normalOnSurfaceB.GetIntrin128(),pos0.GetIntrin128(),pos1.GetIntrin128(),dist.GetIntrin128(),0,0
		// These are filler, but this usage also shouldn't be needing these parameters later anyway 
		, UnTransform3x3Ortho(input.currentA, Subtract(pos0, input.currentA.GetCol3())).GetIntrin128(), UnTransform3x3Ortho(input.currentB, Subtract(pos1, input.currentB.GetCol3())).GetIntrin128()
		TRACK_COLLISION_TIME_PARAM(0.0f));

	manifoldResult.ProcessResult(result);
}
#else
void btSphereSphereCollisionAlgorithm::DetectCollision (const rage::phCollisionInput& UNUSED_PARAM(input), rage::Vector3::Param UNUSED_PARAM(offsetA), rage::phManifold& UNUSED_PARAM(manifold), DiscreteCollisionDetectorInterface::ResultProcessor& UNUSED_PARAM(manifoldResult))
{
	Assertf(0, "btSphereSphereCollisionAlgorithm::DetectCollision not implemented");
}
#endif

} // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE
