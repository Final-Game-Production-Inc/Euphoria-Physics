// 
// phsolver/applyimpulsefixandmov.cpp
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolvercontacts.h"

#include "physics/manifold.h"

#if __SPU
#include "applyimpulsemovandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

SOLVER_OPTIMISATIONS()

namespace rage {

void ApplyImpulseFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	ApplyImpulseMovAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
