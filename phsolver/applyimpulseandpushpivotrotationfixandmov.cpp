// 
// phsolver/applyimpulseandpushpivotrotationfixandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverconstraints.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "applyimpulseandpushpivotrotationmovandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void ApplyImpulseAndPushPivotRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulseAndPushPivotRotationMovAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
