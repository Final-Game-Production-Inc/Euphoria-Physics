// 
// phsolver/applyimpulsefixedrotationfixandart.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverartconstraints.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "applyimpulsefixedrotationartandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void ApplyImpulseFixedRotationFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulseFixedRotationArtAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
