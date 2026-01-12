// 
// phsolver/preresponsefixandart.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#include "forcesolver.h"
#include "forcesolverartcontacts.h"

#include "physics/manifold.h"

#if __SPU
#include "preresponseartandfix.cpp"
#include "physics/manifold.cpp"
#endif // __SPU

SOLVER_OPTIMISATIONS()

namespace rage {

void PreResponseFixAndArt(phManifold& manifold, const phForceSolverGlobals& globals)
{
	CALL_MEMBER_FN(manifold, Exchange)();
	PreResponseArtAndFix(manifold, globals);
	CALL_MEMBER_FN(manifold, Exchange)();
}

} // namespace rage
