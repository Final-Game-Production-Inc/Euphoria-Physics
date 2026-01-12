// 
// phsolver/applyimpulsepivotrotationfixandmov.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverconstraints.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "applyimpulsepivotrotationmovandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void ApplyImpulsePivotRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulsePivotRotationMovAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}


} // namespace rage
