// 
// phsolver/preresponserotationfixandmov.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverconstraints.h"

#include "physics/manifold.h"

SOLVER_OPTIMISATIONS()

#if __SPU
#include "preresponserotationmovandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

namespace rage {

void PreResponseRotationFixAndMov(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	PreResponseRotationMovAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
