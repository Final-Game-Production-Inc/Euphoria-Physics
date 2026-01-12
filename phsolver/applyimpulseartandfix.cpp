// 
// phsolver/applyimpulseartandfix.cpp  
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/constants.h"
#include "phcore/phmath.h"
#include "physics/collider.h"
#include "physics/contact.h"
#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseArtAndFix(phManifold& manifold, const phForceSolverGlobals& globals)
{
	phArticulatedCollider* colliderA = (phArticulatedCollider*)manifold.GetColliderA();
	Assert(colliderA->IsArticulated());
	phArticulatedBody* bodyA = colliderA->GetBody();

	for(int c=0;c<manifold.GetNumContacts();c++)
	{
		phContact& cp = manifold.GetContactPoint(c);
		if (Likely(cp.IsContactActive()))
		{
			Vec3V localPosA = cp.GetWorldPosA() - colliderA->GetPosition();
			int link = colliderA->GetLinkFromComponent(manifold.GetComponentA());

			Mat33V constraintAxis;
			MakeOrthonormals(cp.GetWorldNormal(), constraintAxis.GetCol1Ref(), constraintAxis.GetCol2Ref());
			constraintAxis.SetCol0(cp.GetWorldNormal());

			Vec3V localVelocityA = CALL_MEMBER_FN(*bodyA, GetLocalVelocity)(link, localPosA.GetIntrin128());

			// 	Vec3V constraintVelocity = GetProjectedVelocity();
			Vec3V velocityArticulated = UnTransformOrtho(constraintAxis,localVelocityA);

			velocityArticulated = Subtract(velocityArticulated, cp.GetTargetRelVelocity());

#if !USE_PRECOMPUTE_SEPARATEBIAS
			// Comment out the manifold separate bias for now since it defaults to zero even when pushes are off
			ScalarV separateBias = globals.separateBias /** ScalarVFromF32(manifold.GetSeparateBias())*/;
			ScalarV separate = separateBias * Max(ScalarV(V_ZERO), Min(cp.GetDepthV() - globals.halfAllowedPenetration, globals.halfAllowedPenetration));
#else
			(void)globals;
			const ScalarV separate = /*globals.separateBias * */ cp.GetSeparateBias_();
#endif 
			velocityArticulated -= Vec3V(V_X_AXIS_WZERO) * separate;

			// 	Vec3V f = CalculateConstraintImpulse(constraintVelocity);
			Mat33V dinv = cp.GetDinv();
			Vec3V f = Multiply(dinv,velocityArticulated);
			f = Negate(f);
			Vec3V totalConstraintImpulse(cp.GetPreviousSolution());

			f = Subtract( f, totalConstraintImpulse );

			Vec3V frictionPlaneProjection = cp.GetFrictionPlaneProjection();

			Vec3V ff = INTRIN_TO_VEC3V(SolveWithFriction( VEC3V_TO_INTRIN(f), VEC3V_TO_INTRIN(frictionPlaneProjection), cp.GetFrictionV().GetIntrin128() ));

			ff = Add( ff, totalConstraintImpulse );

			cp.SetPreviousSolution(Subtract( totalConstraintImpulse, ff ));

			Vec3V worldImpulse = Multiply( constraintAxis, ff );

			CALL_MEMBER_FN(*bodyA, ApplyImpulse)(link, worldImpulse, localPosA);
		}
	}
}

} // namespace rage
