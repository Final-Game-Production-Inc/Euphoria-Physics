// 
// phsolver/applyimpulsepivotrotationfixandart.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverartconstraints.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "applyimpulsepivotrotationartandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void ApplyImpulsePivotRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulsePivotRotationArtAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}


} // namespace rage
