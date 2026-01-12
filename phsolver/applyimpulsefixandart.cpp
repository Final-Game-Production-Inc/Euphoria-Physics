// 
// phsolver/applyimpulsefixandart.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverartcontacts.h"

#include "physics/manifold.h"

#if __SPU
#include "applyimpulseartandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulseArtAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
